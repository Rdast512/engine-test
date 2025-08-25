
#include "ResourceManager.h"


ResourceManager::ResourceManager(
    const VulkanContext* context,
    const AssetsLoader* assetsLoader
) : context(context), physicalDevice(context->physicalDevice), device(context->device),
    queueFamilyIndices(context->queueFamilyIndices), graphicsIndex(context->graphicsIndex),
    graphicsQueue(context->graphicsQueue), transferQueue(context->transferQueue), transferIndex(context->transferIndex),
    indices(assetsLoader->getIndices()), vertices(assetsLoader->getVertices()) {
    std::cout<< termcolor::green << "ResourceManager initialized" << std::endl;
}

void ResourceManager::init() {
    createCommandPool();
    createCommandBuffers();
    createDepthResources();
    createUniformBuffers();
    createVertexBuffer();
    createIndexBuffer();
}

void ResourceManager::createSyncObjects() {
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

void ResourceManager::updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    UniformBufferObject ubo{};
    ubo.model = rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f),
                                static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.
                                    height), 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;
    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}


void ResourceManager::createCommandPool() {
    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, .queueFamilyIndex = graphicsIndex
    };
    commandPool = vk::raii::CommandPool(device, poolInfo);
    vk::CommandPoolCreateInfo transferPoolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, .queueFamilyIndex = transferIndex
    };
    if (transferIndex != graphicsIndex) {
        transferCommandPool = vk::raii::CommandPool(device, transferPoolInfo);
    }
}

void ResourceManager::createCommandBuffers() {
    commandBuffers.clear();
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };
    commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
    if (transferIndex != UINT32_MAX && transferIndex != graphicsIndex) {
        vk::CommandBufferAllocateInfo transferAllocInfo{
            .commandPool = transferCommandPool, .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
        };
        transferCommandBuffer = vk::raii::CommandBuffers(device, transferAllocInfo);
    }
    std::cout << termcolor::green << "Command buffers allocated: " << commandBuffers.size() << std::endl;
    std::cout << termcolor::green << "Transfer command buffers allocated: " << transferCommandBuffer.size() <<
            std::endl;
}

[[nodiscard]] vk::raii::ShaderModule ResourceManager::createShaderModule(const std::vector<char> &code) const {
    vk::ShaderModuleCreateInfo createInfo{
        .codeSize = code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t *>(code.data())
    };
    vk::raii::ShaderModule shaderModule{device, createInfo};
    return shaderModule;
}

void ResourceManager::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
                      vk::raii::Buffer &buffer, vk::raii::DeviceMemory &bufferMemory) {
    vk::BufferCreateInfo bufferInfo{
        .size = size, .usage = usage, .sharingMode = vk::SharingMode::eConcurrent,
        .queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size()),
        .pQueueFamilyIndices = queueFamilyIndices.data()
    };
    buffer = vk::raii::Buffer(device, bufferInfo);
    vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
    };
    bufferMemory = vk::raii::DeviceMemory(device, allocInfo);
    buffer.bindMemory(*bufferMemory, 0);
}

void ResourceManager::copyBuffer(vk::raii::Buffer &srcBuffer, vk::raii::Buffer &dstBuffer, vk::DeviceSize size) {
    transferCommandBuffer[0].begin(vk::CommandBufferBeginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    });
    transferCommandBuffer[0].copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, 0, size));
    transferCommandBuffer[0].end();
    vk::CommandBufferSubmitInfo commandBufferInfo = {
        .commandBuffer = *transferCommandBuffer[0]
    };
    const vk::SubmitInfo2 submitInfo{
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &commandBufferInfo
    };
    transferQueue.submit2(submitInfo, nullptr);
    transferQueue.waitIdle();
}


void ResourceManager::endCommandBuffer(vk::raii::CommandBuffer &commandBuffer, const vk::raii::Queue &queue) {
    commandBuffer.end();
    vk::SubmitInfo submitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandBuffer};
    queue.submit(submitInfo, nullptr);
    queue.waitIdle();
}

void ResourceManager::createVertexBuffer() {
    std::cout << termcolor::green << "Creating vertex buffer with " << vertices.size() << " vertices." << std::endl;

    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();


    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                 stagingBuffer, stagingBufferMemory);

    void *dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(dataStaging, vertices.data(), bufferSize);
    stagingBufferMemory.unmapMemory();

    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                 vk::MemoryPropertyFlagBits::eDeviceLocal, vertexBuffer, vertexBufferMemory);

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
}

void ResourceManager::createIndexBuffer() {
    std::cout << termcolor::green << "Creating index buffer with " << indices.size() << " indices." << std::endl;
    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                 stagingBuffer, stagingBufferMemory);

    void *data = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(data, indices.data(), (size_t) bufferSize);
    stagingBufferMemory.unmapMemory();

    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                 vk::MemoryPropertyFlagBits::eDeviceLocal, indexBuffer, indexBufferMemory);

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);
}

void ResourceManager::createUniformBuffers() {
    uniformBuffers.clear();
    uniformBuffersMemory.clear();
    uniformBuffersMapped.clear();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
        vk::raii::Buffer buffer({});
        vk::raii::DeviceMemory bufferMem({});
        createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer,
                     bufferMem);
        uniformBuffers.emplace_back(std::move(buffer));
        uniformBuffersMemory.emplace_back(std::move(bufferMem));
        uniformBuffersMapped.emplace_back(uniformBuffersMemory[i].mapMemory(0, bufferSize));
    }
}

vk::Format ResourceManager::findSupportedFormat(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling,
                               vk::FormatFeatureFlags features) {
    for (const auto format: candidates) {
        vk::FormatProperties props = physicalDevice.getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format!");
}

uint32_t ResourceManager::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

vk::raii::ImageView ResourceManager::createImageView(vk::raii::Image &image, vk::Format format, vk::ImageAspectFlags aspectFlags
                                    , uint32_t mipLevels) {
    vk::ImageViewCreateInfo viewInfo{
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange = {aspectFlags, 0, mipLevels, 0, 1}
    };
    return vk::raii::ImageView(device, viewInfo);
}

void ResourceManager::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format, vk::ImageTiling tiling,
                 vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Image &image,
                 vk::raii::DeviceMemory &imageMemory) {
    // Determine sharing mode based on usage
    vk::SharingMode sharingMode = vk::SharingMode::eExclusive;
    std::vector<uint32_t> queueIndices;

    // Only use concurrent sharing for transfer operations between different queue families
    if ((usage & vk::ImageUsageFlagBits::eTransferSrc || usage & vk::ImageUsageFlagBits::eTransferDst) &&
        transferIndex != UINT32_MAX && transferIndex != graphicsIndex) {
        sharingMode = vk::SharingMode::eConcurrent;
        queueIndices = queueFamilyIndices;
        }

    vk::ImageCreateInfo imageInfo{
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = {width, height, 1},
        .mipLevels = mipLevels,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = sharingMode,
        .queueFamilyIndexCount = static_cast<uint32_t>(queueIndices.size()),
        .pQueueFamilyIndices = queueIndices.data()
    };

    image = vk::raii::Image(device, imageInfo);

    vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
    };
    imageMemory = vk::raii::DeviceMemory(device, allocInfo);
    image.bindMemory(imageMemory, 0);
}

vk::Format ResourceManager::findDepthFormat() {
    return findSupportedFormat({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
                               vk::ImageTiling::eOptimal,
                               vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

void ResourceManager::updateSwapChainExtent(const vk::Extent2D newExtent) {
    swapChainExtent = newExtent;
}

void ResourceManager::createDepthResources() {
    vk::Format depthFormat = findDepthFormat();
    createImage(swapChainExtent.width, swapChainExtent.height, 1, depthFormat, vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal,
                depthImage, depthImageMemory);
    depthImageView = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth, 1);
    commandBuffers[0].begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    transitionImageLayout(&commandBuffers[0], depthImage, 1,
                          vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal
    );
    endCommandBuffer(commandBuffers[0], graphicsQueue);
}

bool ResourceManager::hasStencilComponent(vk::Format format) {
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

void ResourceManager::copyBufferToImage(const vk::raii::Buffer &buffer, vk::raii::Image &image, uint32_t width, uint32_t height) {
    vk::BufferImageCopy region{
        .bufferOffset = 0, .bufferRowLength = 0, .bufferImageHeight = 0,
        .imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        .imageOffset = {0, 0, 0}, .imageExtent = {width, height, 1}
    };
    commandBuffers[0].copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, {region});
    // transferCommandBuffer[0].end();
    // vk::SubmitInfo submitInfo{ .commandBufferCount = 1, .pCommandBuffers = &*transferCommandBuffer[0] };
    // transferQueue.submit(submitInfo, nullptr);
    // transferQueue.waitIdle();
}

void ResourceManager::generateMipmaps(vk::raii::Image &image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight,
                         uint32_t mipLevels) {
        // Check for blit support
        vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(imageFormat);
        if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
            throw std::runtime_error("Texture image format does not support linear blitting!");
        }

        auto &graphicsCmd = commandBuffers[0];
        graphicsCmd.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++) {
            // Transition the previous mip level (i-1) from DST to SRC
            vk::ImageMemoryBarrier barrier_to_src = {
                .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
                .dstAccessMask = vk::AccessFlagBits::eTransferRead,
                .oldLayout = vk::ImageLayout::eTransferDstOptimal,
                .newLayout = vk::ImageLayout::eTransferSrcOptimal,
                .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
                .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
                .image = *image,
                .subresourceRange = { vk::ImageAspectFlagBits::eColor, i - 1, 1, 0, 1 }
            };
            graphicsCmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier_to_src);

            // Create and set up the blit structure
            vk::ImageBlit blit{};
            blit.srcSubresource = { vk::ImageAspectFlagBits::eColor, i - 1, 0, 1 };
            blit.srcOffsets[0] = vk::Offset3D(0, 0, 0);
            blit.srcOffsets[1] = vk::Offset3D(mipWidth, mipHeight, 1);
            blit.dstSubresource = { vk::ImageAspectFlagBits::eColor, i, 0, 1 };
            blit.dstOffsets[0] = vk::Offset3D(0, 0, 0);
            blit.dstOffsets[1] = vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1);

            graphicsCmd.blitImage(*image, vk::ImageLayout::eTransferSrcOptimal, *image,
                                  vk::ImageLayout::eTransferDstOptimal, {blit}, vk::Filter::eLinear);

            // Update dimensions for the next iteration
            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        // Transition the last mip level from DST to SRC to unify layouts
        vk::ImageMemoryBarrier barrier_last = {
            .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
            .dstAccessMask = vk::AccessFlagBits::eTransferRead,
            .oldLayout = vk::ImageLayout::eTransferDstOptimal,
            .newLayout = vk::ImageLayout::eTransferSrcOptimal,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .image = *image,
            .subresourceRange = { vk::ImageAspectFlagBits::eColor, mipLevels-1, 1, 0, 1 }
        };
        graphicsCmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                    vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier_last);

        // Final transition: all mip levels to shader read-only
        vk::ImageMemoryBarrier final_barrier = {
            .srcAccessMask = vk::AccessFlagBits::eTransferRead,
            .dstAccessMask = vk::AccessFlagBits::eShaderRead,
            .oldLayout = vk::ImageLayout::eTransferSrcOptimal,
            .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .image = *image,
            .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0, 1 }
        };

        graphicsCmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                    vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, final_barrier);

        endCommandBuffer(graphicsCmd, graphicsQueue);
    }

void ResourceManager::transitionImageLayout(
        vk::raii::CommandBuffer *commandBuffer,
        vk::Image image,
        uint32_t mipLevels,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        const vk::ImageSubresourceRange &subresourceRange,
        uint32_t srcQueueFamily,
        uint32_t dstQueueFamily,
        std::optional<vk::PipelineStageFlags2> srcStageMaskOverride,
        std::optional<vk::PipelineStageFlags2> dstStageMaskOverride ,
        std::optional<vk::AccessFlags2> srcAccessMaskOverride,
        std::optional<vk::AccessFlags2> dstAccessMaskOverride
    ) {
        // Create a local copy that we can modify
        vk::ImageSubresourceRange localSubresourceRange = subresourceRange;

        vk::PipelineStageFlags2 srcStageMask;
        vk::PipelineStageFlags2 dstStageMask;
        vk::AccessFlags2 srcAccessMask;
        vk::AccessFlags2 dstAccessMask;

        // --- Automatic Barrier Deduction ---
        if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
            srcAccessMask = vk::AccessFlagBits2::eNone;
            dstAccessMask = vk::AccessFlagBits2::eTransferWrite;
            srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
            dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
        } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout ==
                   vk::ImageLayout::eShaderReadOnlyOptimal) {
            srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
            dstAccessMask = vk::AccessFlagBits2::eShaderRead;
            srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
            dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader | vk::PipelineStageFlagBits2::eComputeShader;
        } else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
            srcAccessMask = vk::AccessFlagBits2::eNone;
            dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
            srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
            dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
        } else if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal && newLayout ==
                   vk::ImageLayout::ePresentSrcKHR) {
            srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
            dstAccessMask = vk::AccessFlagBits2::eNone;
            srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
            dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe;
        } else if (oldLayout == vk::ImageLayout::eUndefined && newLayout ==
                   vk::ImageLayout::eDepthStencilAttachmentOptimal) {
            srcAccessMask = vk::AccessFlagBits2::eNone;
            dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
            srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
            dstStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests;

            // Modify the local copy for depth layouts
            localSubresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
            if (hasStencilComponent(findDepthFormat())) {
                localSubresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
            }
        } else if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
            // Handle other transitions to depth layout
            localSubresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
            if (hasStencilComponent(findDepthFormat())) {
                localSubresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
            }
        } else {
            throw std::runtime_error("Unsupported automatic layout transition!");
        }
        localSubresourceRange.levelCount = mipLevels;
        vk::ImageMemoryBarrier2 imageBarrier = {
            .srcStageMask = srcStageMaskOverride.value_or(srcStageMask),
            .srcAccessMask = srcAccessMaskOverride.value_or(srcAccessMask),
            .dstStageMask = dstStageMaskOverride.value_or(dstStageMask),
            .dstAccessMask = dstAccessMaskOverride.value_or(dstAccessMask),
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = srcQueueFamily,
            .dstQueueFamilyIndex = dstQueueFamily,
            .image = image,
            .subresourceRange = localSubresourceRange // Use the local copy
        };

        vk::DependencyInfo dependencyInfo = {
            .dependencyFlags = {},
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &imageBarrier
        };

        commandBuffer->pipelineBarrier2(dependencyInfo);
    }