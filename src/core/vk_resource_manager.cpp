#include "vk_resource_manager.hpp"
#include <chrono>
#include <format>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
#include "Constants.h"
#include "static_headers/logger.hpp"
#include "util/debug.hpp"
#include "util/vk_tracy.hpp"
#include "util/vk_utils.hpp"

ResourceManager::ResourceManager(const Device& deviceWrapper, const VkAllocator& allocator, const std::vector<Vertex>& verticesIn, const std::vector<uint32_t>& indicesIn, std::vector<Object>& objectsIn):
    deviceWrapper(deviceWrapper), allocator(allocator), physicalDevice(deviceWrapper.physicalDevice),
    device(deviceWrapper.vkdevice), queueFamilyIndices(deviceWrapper.queueFamilyIndices),
    graphicsIndex(deviceWrapper.graphicsIndex), graphicsQueue(deviceWrapper.graphicsQueue),
    transferQueue(deviceWrapper.transferQueue), transferIndex(deviceWrapper.transferIndex),
    msaaSamples(deviceWrapper.msaaSamples), vertices(verticesIn), indices(indicesIn), objects(objectsIn), hardwareCapabilities(deviceWrapper.capabilities)
{
    log_info("Initialized", "ResourceManager");
}

ResourceManager::~ResourceManager()
{
    log_info("Destructor called", "ResourceManager");
    {
        if (vertexBufferMemory)  { VkBuffer raw = vertexBuffer.release();  vmaDestroyBuffer(allocator.allocator, raw, vertexBufferMemory); }
        if (indexBufferMemory)   { VkBuffer raw = indexBuffer.release();   vmaDestroyBuffer(allocator.allocator, raw, indexBufferMemory); }
        if (colorImageMemory)    { VkImage  raw = colorImage.release();    vmaDestroyImage (allocator.allocator, raw, colorImageMemory); }
        if (depthImageMemory)    { VkImage  raw = depthImage.release();    vmaDestroyImage (allocator.allocator, raw, depthImageMemory); }
    }
}

void ResourceManager::init()
{
    ZoneScopedN("ResourceManager::init");
    log_info("init() started", "ResourceManager");
    createObjectStorage();
    createCommandPool();
    createCommandBuffers();
    createUniformBuffers();
    createVertexBuffer();
    createIndexBuffer();
}


void ResourceManager::createObjectStorage()
{
    ZoneScopedN("ResourceManager::createObjectStorage");
    log_info("createObjectStorage() started", "ResourceManager");
}

void ResourceManager::createSyncObjects()
{
    ZoneScopedN("ResourceManager::createSyncObjects");
    log_info("createSyncObjects() started", "ResourceManager");
    presentCompleteSemaphore.clear();
    renderFinishedSemaphore.clear();
    inFlightFences.clear();
    for (size_t i = 0; i < swapChainImageCount; i++) {
        presentCompleteSemaphore.emplace_back(device, vk::SemaphoreCreateInfo());
        renderFinishedSemaphore.emplace_back(device, vk::SemaphoreCreateInfo());
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        inFlightFences.emplace_back(device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }
}

void ResourceManager::updateUniformBuffers(uint32_t currentImage)
{
    ZoneScopedN("ResourceManager::updateUniformBuffer");
    // Camera and projection matrices (shared by all objects)
    for (auto& gameObject : objects) {
        // Apply continuous rotation to the object
        auto rotation = gameObject.getRotation();
        rotation.y += 0.01f; // Slow rotation around Y axis
        gameObject.setRotation(rotation);

        // Get the model matrix for this object
        glm::mat4 initialRotation = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 model = gameObject.getModelMatrix() * initialRotation;


        ObjectUB const uboData{
            .modelMatrix = model,
            .prevModelMatrix = gameObject.modelMatrix,
        };
        gameObject.modelMatrix = model; // Update the previous model matrix for the next frame

        // Copy the UBO data to the mapped memory
        memcpy(gameObject.uniformBuffersMapped[currentImage], &uboData, sizeof(uboData));
    }
}


void ResourceManager::createCommandPool()
{
    ZoneScopedN("ResourceManager::createCommandPool");
    log_info("createCommandPool() started", "ResourceManager");
    vk::CommandPoolCreateInfo poolInfo{.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                       .queueFamilyIndex = graphicsIndex};
    commandPool = vk::raii::CommandPool(device, poolInfo);
    setDebugName(device, commandPool, "GraphicsCommandPool");
    vk::CommandPoolCreateInfo transferPoolInfo{.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                               .queueFamilyIndex = transferIndex};
    if (transferIndex != graphicsIndex && transferIndex != UINT32_MAX) {
        transferCommandPool = vk::raii::CommandPool(device, transferPoolInfo);
        setDebugName(device, transferCommandPool, "TransferCommandPool");
    }
}

void ResourceManager::createCommandBuffers()
{
    ZoneScopedN("ResourceManager::createCommandBuffers");
    log_info("createCommandBuffers() started", "ResourceManager");
    commandBuffers.clear();
    vk::CommandBufferAllocateInfo allocInfo{.commandPool = commandPool,
                                            .level = vk::CommandBufferLevel::ePrimary,
                                            .commandBufferCount = MAX_FRAMES_IN_FLIGHT};
    commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
    for (size_t i = 0; i < commandBuffers.size(); ++i) {
        setDebugName(device, commandBuffers[i], std::format("GraphicsCommandBuffer_{}", i));
    }
    if (transferIndex != UINT32_MAX && transferIndex != graphicsIndex) {
        vk::CommandBufferAllocateInfo transferAllocInfo{.commandPool = transferCommandPool,
                                                        .level = vk::CommandBufferLevel::ePrimary,
                                                        .commandBufferCount = MAX_FRAMES_IN_FLIGHT};
        transferCommandBuffer = vk::raii::CommandBuffers(device, transferAllocInfo);
        for (size_t i = 0; i < transferCommandBuffer.size(); ++i) {
            setDebugName(device, transferCommandBuffer[i], std::format("TransferCommandBuffer_{}", i));
        }
    }
    log_info(std::format("Command buffers allocated: {}", commandBuffers.size()), "ResourceManager");
    log_info(std::format("Transfer command buffers allocated: {}", transferCommandBuffer.size()), "ResourceManager");
}

[[nodiscard]] vk::raii::ShaderModule ResourceManager::createShaderModule(const std::vector<char>& code) const
{
    log_info("createShaderModule() started", "ResourceManager");
    vk::ShaderModuleCreateInfo createInfo{.codeSize = code.size() * sizeof(char),
                                          .pCode = reinterpret_cast<const uint32_t*>(code.data())};
    vk::raii::ShaderModule shaderModule{device, createInfo};
    return shaderModule;
}

void ResourceManager::copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size)
{
    log_info("copyBuffer() started", "ResourceManager");
    transferCommandBuffer[0].begin(vk::CommandBufferBeginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    transferCommandBuffer[0].copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, 0, size));
    transferCommandBuffer[0].end();
    vk::CommandBufferSubmitInfo commandBufferInfo = {.commandBuffer = *transferCommandBuffer[0]};
    const vk::SubmitInfo2 submitInfo{.commandBufferInfoCount = 1, .pCommandBufferInfos = &commandBufferInfo};
    transferQueue.submit2(submitInfo, nullptr);
    transferQueue.waitIdle();
}


void ResourceManager::endCommandBuffer(vk::raii::CommandBuffer& commandBuffer, const vk::raii::Queue& queue)
{
    ZoneScopedN("ResourceManager::endCommandBuffer");
    log_info("endCommandBuffer() started", "ResourceManager");
    commandBuffer.end();
    // Prefer synchronization2 submit (avoids WARNING-deprecation-sync2 / legacy QueueSubmit).
    const vk::CommandBufferSubmitInfo commandBufferInfo{.commandBuffer = *commandBuffer};
    const vk::SubmitInfo2 submitInfo{.commandBufferInfoCount = 1, .pCommandBufferInfos = &commandBufferInfo};
    queue.submit2(submitInfo, nullptr);
    queue.waitIdle();
}

void ResourceManager::createVertexBuffer()
{
    ZoneScopedN("ResourceManager::createVertexBuffer");
    log_info("createVertexBuffer() started", "ResourceManager");
    log_info(std::format("Creating vertex buffer with {} vertices", vertices.size()), "ResourceManager");

    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    hardwareCapabilities.properties2.properties.limits;
    // Ensure previous staging allocation is released before reusing the member buffer
    // if (stagingBufferMemory != nullptr) {
    //     VkBuffer rawStaging = stagingBuffer.release();
    //     vmaDestroyBuffer(allocator.allocator, rawStaging, stagingBufferMemory);
    //     stagingBufferMemory = nullptr;
    // }

    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer,
                 stagingBufferMemory, allocator.allocator, device, queueFamilyIndices, "VertexStagingBufferMemory");
    setDebugName(device, stagingBuffer, "VertexStagingBuffer");

    void* dataStaging = nullptr;
    vmaMapMemory(allocator.allocator, stagingBufferMemory, &dataStaging);
    memcpy(dataStaging, vertices.data(), bufferSize);
    vmaUnmapMemory(allocator.allocator, stagingBufferMemory);

    if (vertexBufferMemory != nullptr) {
        VkBuffer rawVertex = vertexBuffer.release();
        vmaDestroyBuffer(allocator.allocator, rawVertex, vertexBufferMemory);
        vertexBufferMemory = nullptr;
    }

    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                 vk::MemoryPropertyFlagBits::eDeviceLocal, vertexBuffer, vertexBufferMemory, allocator.allocator, device, queueFamilyIndices, "VertexBufferMemory");
    setDebugName(device, vertexBuffer, "VertexBuffer");

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    // Free staging buffer after use to avoid leaking allocations
    if (stagingBufferMemory != nullptr) {
        VkBuffer rawStaging = stagingBuffer.release();
        vmaDestroyBuffer(allocator.allocator, rawStaging, stagingBufferMemory);
        stagingBufferMemory = nullptr;
    }
}

void ResourceManager::createIndexBuffer()
{
    ZoneScopedN("ResourceManager::createIndexBuffer");
    log_info("createIndexBuffer() started", "ResourceManager");
    log_info(std::format("Creating index buffer with {} indices", indices.size()), "ResourceManager");
    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    // Ensure previous staging allocation is released before reusing the member buffer
    // if (stagingBufferMemory != nullptr) {
    //     VkBuffer rawStaging = stagingBuffer.release();
    //     vmaDestroyBuffer(allocator.allocator, rawStaging, stagingBufferMemory);
    //     stagingBufferMemory = nullptr;
    // }

    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer,
                 stagingBufferMemory, allocator.allocator, device, queueFamilyIndices, "IndexStagingBufferMemory");
    setDebugName(device, stagingBuffer, "IndexStagingBuffer");

    void* data = nullptr;
    vmaMapMemory(allocator.allocator, stagingBufferMemory, &data);
    memcpy(data, indices.data(), bufferSize);
    vmaUnmapMemory(allocator.allocator, stagingBufferMemory);

    if (indexBufferMemory != nullptr) {
        VkBuffer rawIndex = indexBuffer.release();
        vmaDestroyBuffer(allocator.allocator, rawIndex, indexBufferMemory);
        indexBufferMemory = nullptr;
    }

    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                 vk::MemoryPropertyFlagBits::eDeviceLocal, indexBuffer, indexBufferMemory, allocator.allocator, device, queueFamilyIndices, "IndexBufferMemory");
    setDebugName(device, indexBuffer, "IndexBuffer");

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    // Free staging buffer after use to avoid leaking allocations
    if (stagingBufferMemory != nullptr) {
        VkBuffer rawStaging = stagingBuffer.release();
        vmaDestroyBuffer(allocator.allocator, rawStaging, stagingBufferMemory);
        stagingBufferMemory = nullptr;
    }
}

void ResourceManager::createCameraBuffers(Camera& camera)
{
    ZoneScopedN("ResourceManager::createCameraBuffers");
    log_info("createCameraBuffers() started", "ResourceManager");
    camera.allocator = allocator.allocator;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DeviceSize bufferSize = sizeof(CameraData);
        vk::raii::Buffer buffer({});
        VmaAllocation bufferMem = nullptr;
        createBuffer(bufferSize,
                     vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eStorageBuffer |
                         vk::BufferUsageFlagBits::eShaderDeviceAddress,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer,
                     bufferMem, allocator.allocator, device, queueFamilyIndices,
                     std::format("CameraUniformBufferMemory_{}", i));
        camera.cameraBuffers[i] = std::move(buffer);
        camera.cameraBuffersMemory[i] = bufferMem;
        void* data = nullptr;
        vmaMapMemory(allocator.allocator, bufferMem, &data);
        camera.cameraBuffersMapped[i] = data;
        camera.cameraBufferAddresses[i] = device.getBufferAddress({.buffer = *camera.cameraBuffers[i]});
    }
}

void ResourceManager::createUniformBuffer(Object& obj)
{
    ZoneScopedN("ResourceManager::createUniformBuffers");
    log_info("createUniformBuffers() started", "ResourceManager");
    obj.vmaAllocator = allocator.allocator;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DeviceSize bufferSize = sizeof(ObjectUB);
        vk::raii::Buffer buffer({});
        VmaAllocation bufferMem = nullptr;
        createBuffer(bufferSize,
                     vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eStorageBuffer |
                         vk::BufferUsageFlagBits::eShaderDeviceAddress,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer,
                     bufferMem, allocator.allocator, device, queueFamilyIndices,
                     std::format("UniformBufferMemory_{}_{}", i, obj.name));
        obj.uniformBuffers[i] = std::move(buffer);
        obj.uniformBuffersMemory[i] = bufferMem;
        void* data = nullptr;
        vmaMapMemory(allocator.allocator, bufferMem, &data);
        obj.uniformBuffersMapped[i] = data;
        obj.uboAddresses[i] = device.getBufferAddress({.buffer = *obj.uniformBuffers[i]});
    }
}

// when recreating swapchain this should be called, otherwise ubos should be created for each obj individually.
void ResourceManager::createUniformBuffers()
{
    ZoneScopedN("ResourceManager::createUniformBuffers");
    log_info("createUniformBuffers() started", "ResourceManager");
    for (auto& object : objects) {
        object.vmaAllocator = allocator.allocator;
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vk::DeviceSize bufferSize = sizeof(ObjectUB);
            vk::raii::Buffer buffer({});
            VmaAllocation bufferMem = nullptr;
            createBuffer(bufferSize,
                         vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eStorageBuffer |
                             vk::BufferUsageFlagBits::eShaderDeviceAddress,
                         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer,
                         bufferMem, allocator.allocator, device, queueFamilyIndices,
                         std::format("UniformBufferMemory_{}_{}", i, object.name));
            object.uniformBuffers[i] = std::move(buffer);
            object.uniformBuffersMemory[i] = bufferMem;
            void* data = nullptr;
            vmaMapMemory(allocator.allocator, bufferMem, &data);
            object.uniformBuffersMapped[i] = data;
            object.uboAddresses[i] = device.getBufferAddress({.buffer = *object.uniformBuffers[i]});
            // setDebugName(device, object.uniformBuffers.back(), std::format("UniformBuffer_{}_{}", i, object.name));
        }
    }
    // uniformBuffers.clear();
    // uniformBuffersMemory.clear();
    // uniformBuffersMapped.clear();
    //
    // for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    // {
    //     vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
    //     vk::raii::Buffer buffer({});
    //     VmaAllocation bufferMem = nullptr;
    //     createBuffer(bufferSize,
    //              vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eStorageBuffer |
    //                  vk::BufferUsageFlagBits::eShaderDeviceAddress,
    //                  vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer,
    //                  bufferMem, std::format("UniformBufferMemory_{}", i));
    //     uniformBuffers.emplace_back(std::move(buffer));
    //     uniformBuffersMemory.emplace_back(bufferMem);
    //
    //     void* mappedData = nullptr;
    //     vmaMapMemory(allocator.allocator, bufferMem, &mappedData);
    //     uniformBuffersMapped.emplace_back(mappedData);
    //
    //     setDebugName(device, uniformBuffers.back(), std::format("UniformBuffer_{}", i));
    // }
}

void ResourceManager::recreateObjectsBuffers()
{
    ZoneScopedN("ResourceManager::recreateVertexIndexBuffers");
    log_info("recreateVertexIndexBuffers() started", "ResourceManager");
    createVertexBuffer();
    createIndexBuffer();
}

vk::Format ResourceManager::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
                                                vk::FormatFeatureFlags features)
{
    log_info("findSupportedFormat() started", "ResourceManager");
    for (const auto format : candidates) {
        vk::FormatProperties props = physicalDevice.getFormatProperties2(format).formatProperties;

        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format!");
}

uint32_t ResourceManager::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    log_info("findMemoryType() started", "ResourceManager");
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties2().memoryProperties;
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

vk::raii::ImageView ResourceManager::createImageView(vk::raii::Image& image, vk::Format format,
                                                     vk::ImageAspectFlags aspectFlags, uint32_t mipLevels)
{
    log_info("createImageView() started", "ResourceManager");
    vk::ImageViewCreateInfo viewInfo{.image = image,
                                     .viewType = vk::ImageViewType::e2D,
                                     .format = format,
                                     .subresourceRange = {.aspectMask = aspectFlags,
                                                          .baseMipLevel = 0,
                                                          .levelCount = mipLevels,
                                                          .baseArrayLayer = 0,
                                                          .layerCount = 1}};
    return vk::raii::ImageView(device, viewInfo);
}

void ResourceManager::createColorResources()
{
    ZoneScopedN("ResourceManager::createColorResources");
    log_info("createColorResources() started", "ResourceManager");
    if (swapChainImageFormat == vk::Format::eUndefined) {
        return;
    }

    // Destroy previous color resources before recreating
    if (colorImageMemory != nullptr) {
        VkImage raw = colorImage.release();
        vmaDestroyImage(allocator.allocator, raw, colorImageMemory);
        colorImageMemory = nullptr;
        colorImageView = nullptr;
    }
    vk::Format colorFormat = swapChainImageFormat;

    createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, colorFormat, vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
                vk::MemoryPropertyFlagBits::eDeviceLocal, colorImage, colorImageMemory, "ColorImageMemory");
    setDebugName(device, colorImage, "ColorImage");
    commandBuffers[0].begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    transitionImageLayout(&commandBuffers[0], colorImage, 1, vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eColorAttachmentOptimal,
                          {.aspectMask = vk::ImageAspectFlagBits::eColor,
                           .baseMipLevel = 0,
                           .levelCount = 1,
                           .baseArrayLayer = 0,
                           .layerCount = 1});
    endCommandBuffer(commandBuffers[0], graphicsQueue);
    colorImageView = createImageView(colorImage, colorFormat, vk::ImageAspectFlagBits::eColor, 1);
}

void ResourceManager::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits Samples,
                                  vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                                  vk::MemoryPropertyFlags properties, vk::raii::Image& image,
                                  VmaAllocation& imageMemory, std::string_view memoryDebugBaseName)
{
    ZoneScopedN("ResourceManager::createImage");
    log_info("createImage() started", "ResourceManager");
    // Determine sharing mode based on usage
    vk::SharingMode sharingMode = vk::SharingMode::eExclusive;
    std::vector<uint32_t> queueIndices;

    // Only use concurrent sharing for transfer operations between different queue families
    if ((usage & vk::ImageUsageFlagBits::eTransferSrc || usage & vk::ImageUsageFlagBits::eTransferDst) &&
        transferIndex != UINT32_MAX && transferIndex != graphicsIndex) {
        sharingMode = vk::SharingMode::eConcurrent;
        queueIndices = queueFamilyIndices;
    }

    vk::ImageCreateInfo const imageInfo{.imageType = vk::ImageType::e2D,
                                        .format = format,
                                        .extent = {.width = width, .height = height, .depth = 1},
                                        .mipLevels = mipLevels,
                                        .arrayLayers = 1,
                                        .samples = Samples,
                                        .tiling = tiling,
                                        .usage = usage,
                                        .sharingMode = sharingMode,
                                        .queueFamilyIndexCount = static_cast<uint32_t>(queueIndices.size()),
                                        .pQueueFamilyIndices = queueIndices.data()};

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    if (properties & vk::MemoryPropertyFlagBits::eHostVisible) {
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        allocInfo.priority = 0.25f;
    } else if (usage & (vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eDepthStencilAttachment |
                        vk::ImageUsageFlagBits::eTransientAttachment)) {
        // Color/depth attachments should demote last (NVIDIA memory priority best practice).
        allocInfo.priority = 1.0f;
    } else {
        allocInfo.priority = 0.9f;
    }

    allocator.alocateImage(imageInfo, allocInfo, image, imageMemory, memoryDebugBaseName);
}

vk::Format ResourceManager::findDepthFormat()
{
    log_info("findDepthFormat() started", "ResourceManager");
    // Prefer D24/D16 over D32 (BestPractices-NVIDIA-CreateImage-Depth32Format).
    // Fall back to D32 only if the preferred formats are unsupported.
    return findSupportedFormat(
        {vk::Format::eD24UnormS8Uint, vk::Format::eD16Unorm, vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint},
        vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

void ResourceManager::updateSwapChainExtent(const vk::Extent2D newExtent)
{
    log_info("updateSwapChainExtent() started", "ResourceManager");
    swapChainExtent = newExtent;
}

void ResourceManager::createDepthResources()
{
    ZoneScopedN("ResourceManager::createDepthResources");
    log_info("createDepthResources() started", "ResourceManager");
    vk::Format depthFormat = findDepthFormat();
    log_info(std::format("Depth format selected: {}", vk::to_string(depthFormat)), "ResourceManager");

    // Destroy previous depth resources before recreating
    if (depthImageMemory != nullptr) {
        VkImage raw = depthImage.release();
        vmaDestroyImage(allocator.allocator, raw, depthImageMemory);
        depthImageMemory = nullptr;
        depthImageView = nullptr;
    }
    createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, depthFormat, vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, depthImage,
                depthImageMemory, "DepthImageMemory");
    setDebugName(device, depthImage, "DepthImage");
    // View can be depth-only for the attachment; barriers must still cover both aspects
    // when the format is packed depth/stencil and separateDepthStencilLayouts is off
    // (VUID-VkImageMemoryBarrier2-image-03320).
    depthImageView = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth, 1);
    vk::ImageAspectFlags barrierAspects = vk::ImageAspectFlagBits::eDepth;
    if (hasStencilComponent(depthFormat)) {
        barrierAspects |= vk::ImageAspectFlagBits::eStencil;
    }
    commandBuffers[0].begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    transitionImageLayout(&commandBuffers[0], depthImage, 1, vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eDepthStencilAttachmentOptimal,
                          {.aspectMask = barrierAspects,
                           .baseMipLevel = 0,
                           .levelCount = 1,
                           .baseArrayLayer = 0,
                           .layerCount = 1});
    endCommandBuffer(commandBuffers[0], graphicsQueue);
}

bool ResourceManager::hasStencilComponent(vk::Format format)
{
    log_info("hasStencilComponent() started", "ResourceManager");
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint ||
           format == vk::Format::eD16UnormS8Uint;
}

void ResourceManager::copyBufferToImage(const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width,
                                        uint32_t height)
{
    ZoneScopedN("ResourceManager::copyBufferToImage");
    log_info("copyBufferToImage() started", "ResourceManager");

    vk::BufferImageCopy region{.bufferOffset = 0,
                               .bufferRowLength = 0,
                               .bufferImageHeight = 0,
                               .imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
                               .imageOffset = {0, 0, 0},
                               .imageExtent = {width, height, 1}};
    commandBuffers[0].copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, {region});
}

void ResourceManager::generateMipmaps(vk::raii::Image& image, vk::Format imageFormat, int32_t texWidth,
                                      int32_t texHeight, uint32_t mipLevels)
{
    ZoneScopedN("ResourceManager::generateMipmaps");
    log_info("generateMipmaps() started", "ResourceManager");
    // Check for blit support
    vk::FormatProperties formatProperties = physicalDevice.getFormatProperties2(imageFormat).formatProperties;
    if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        throw std::runtime_error("Texture image format does not support linear blitting!");
    }

    auto& graphicsCmd = commandBuffers[0];
    graphicsCmd.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});


    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) {
        vk::ImageMemoryBarrier2 barrier_to_src = {
            .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
            .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
            .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
            .dstAccessMask = vk::AccessFlagBits2::eTransferRead,
            .oldLayout = vk::ImageLayout::eTransferDstOptimal,
            .newLayout = vk::ImageLayout::eTransferSrcOptimal,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .image = *image,
            .subresourceRange = {vk::ImageAspectFlagBits::eColor, i - 1, 1, 0, 1}};
        vk::DependencyInfo depInfoToSrc{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier_to_src};
        graphicsCmd.pipelineBarrier2(depInfoToSrc);

        vk::ImageBlit blit{};
        blit.srcSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = i - 1, .baseArrayLayer = 0, .layerCount = 1};
        blit.srcOffsets[0] = vk::Offset3D(0, 0, 0);
        blit.srcOffsets[1] = vk::Offset3D(mipWidth, mipHeight, 1);
        blit.dstSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = i, .baseArrayLayer = 0, .layerCount = 1};
        blit.dstOffsets[0] = vk::Offset3D(0, 0, 0);
        blit.dstOffsets[1] = vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1);

        graphicsCmd.blitImage(*image, vk::ImageLayout::eTransferSrcOptimal, *image,
                              vk::ImageLayout::eTransferDstOptimal, {blit}, vk::Filter::eLinear);

        if (mipWidth > 1)
            mipWidth /= 2;
        if (mipHeight > 1)
            mipHeight /= 2;
    }

    vk::ImageMemoryBarrier2 barrier_last = {.srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
                                            .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
                                            .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
                                            .dstAccessMask = vk::AccessFlagBits2::eTransferRead,
                                            .oldLayout = vk::ImageLayout::eTransferDstOptimal,
                                            .newLayout = vk::ImageLayout::eTransferSrcOptimal,
                                            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
                                            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
                                            .image = *image,
                                            .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                                                                 .baseMipLevel = mipLevels - 1,
                                                                 .levelCount = 1,
                                                                 .baseArrayLayer = 0,
                                                                 .layerCount = 1}};
    vk::DependencyInfo depInfoLast{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier_last};
    graphicsCmd.pipelineBarrier2(depInfoLast);

    vk::ImageMemoryBarrier2 final_barrier = {.srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
                                             .srcAccessMask = vk::AccessFlagBits2::eTransferRead,
                                             .dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader,
                                             .dstAccessMask = vk::AccessFlagBits2::eShaderRead,
                                             .oldLayout = vk::ImageLayout::eTransferSrcOptimal,
                                             .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
                                             .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
                                             .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
                                             .image = *image,
                                             .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                                                                  .baseMipLevel = 0,
                                                                  .levelCount = mipLevels,
                                                                  .baseArrayLayer = 0,
                                                                  .layerCount = 1}};
    vk::DependencyInfo depInfoFinal{.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &final_barrier};
    graphicsCmd.pipelineBarrier2(depInfoFinal);

    endCommandBuffer(graphicsCmd, graphicsQueue);
}
