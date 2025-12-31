#include "texture_manager.hpp"

#include "../../ThirdParty/stb_image.h"
#include "../util/debug.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <print>
#include "../util/logger.hpp"

TextureManager::TextureManager(Device &deviceWrapper, ResourceManager &resourceManager)
    : deviceWrapper(deviceWrapper), physicalDevice(deviceWrapper.getPhysicalDevice()),
      device(deviceWrapper.getDevice()), graphicsQueue(deviceWrapper.getGraphicsQueue()),
      transferQueue(deviceWrapper.getTransferQueue()),
      graphicsQueueFamilyIndex(deviceWrapper.getGraphicsIndex()),
      transferQueueFamilyIndex(deviceWrapper.getTransferIndex()),
      resourceManager(resourceManager) {
    log_info("TextureManager::TextureManager() started");
    vk::CommandPoolCreateInfo poolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = graphicsQueueFamilyIndex
    };
    commandPool = vk::raii::CommandPool(device, poolInfo);
}

void TextureManager::init() {
    log_info("TextureManager::init() started");
    log_info("TextureManager initialized");
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
}

void TextureManager::createTextureImage() {
    log_info("TextureManager::createTextureImage() started");
    int texWidth = 0;
    int texHeight = 0;
    int texChannels = 0;
    const auto texturePath = TEXTURE_PATH.string();

    stbi_uc *pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("failed to load texture image: " + texturePath);
    }

    vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(texWidth) * static_cast<vk::DeviceSize>(texHeight) * 4;
    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc,
                 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                 stagingBuffer, stagingBufferMemory);
    setDebugName(device, stagingBuffer, "TextureStagingBuffer");

    void *data = stagingBufferMemory.mapMemory(0, imageSize);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    stagingBufferMemory.unmapMemory();
    stbi_image_free(pixels);

    createImage(static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), mipLevels,
                vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst |
                    vk::ImageUsageFlagBits::eSampled,
                vk::MemoryPropertyFlagBits::eDeviceLocal, textureImage, textureImageMemory);
    setDebugName(device, textureImage, "TextureImage");

    auto commandBuffer = beginSingleTimeCommands(graphicsQueue);
    resourceManager.transitionImageLayout(&commandBuffer, *textureImage, mipLevels, vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eTransferDstOptimal);
    copyBufferToImage(commandBuffer, stagingBuffer, textureImage, static_cast<uint32_t>(texWidth),
                      static_cast<uint32_t>(texHeight));
    endSingleTimeCommands(commandBuffer, graphicsQueue);

    generateMipmaps(textureImage, vk::Format::eR8G8B8A8Srgb, texWidth, texHeight, mipLevels);
}

void TextureManager::createTextureImageView() {
    log_info("TextureManager::createTextureImageView() started");
    vk::ImageViewCreateInfo viewInfo{
        .image = textureImage,
        .viewType = vk::ImageViewType::e2D,
        .format = vk::Format::eR8G8B8A8Srgb,
        .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0, 1}
    };
    textureImageView = vk::raii::ImageView(device, viewInfo);
}

void TextureManager::createTextureSampler() {
    log_info("TextureManager::createTextureSampler() started");
    vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
    vk::SamplerCreateInfo samplerInfo{
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .mipLodBias = 0.0f,
        .anisotropyEnable = vk::True,
        .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
        .compareEnable = vk::False,
        .compareOp = vk::CompareOp::eAlways,
        .minLod = 0.0f,
        .maxLod = static_cast<float>(mipLevels)
    };
    textureSampler = vk::raii::Sampler(device, samplerInfo);
}

uint32_t TextureManager::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    log_info("TextureManager::findMemoryType() started");
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1u << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type");
}

void TextureManager::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
                                  vk::raii::Buffer &buffer, vk::raii::DeviceMemory &bufferMemory) {
    log_info("TextureManager::createBuffer() started");
    const bool needsConcurrent = (usage & vk::BufferUsageFlagBits::eTransferSrc ||
                                  usage & vk::BufferUsageFlagBits::eTransferDst) &&
                                 transferQueueFamilyIndex != UINT32_MAX &&
                                 transferQueueFamilyIndex != graphicsQueueFamilyIndex;

    vk::BufferCreateInfo bufferInfo{
        .size = size,
        .usage = usage,
        .sharingMode = needsConcurrent ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
        .queueFamilyIndexCount = needsConcurrent ? 2u : 0u,
        .pQueueFamilyIndices = nullptr
    };

    uint32_t families[2] = {graphicsQueueFamilyIndex, transferQueueFamilyIndex};
    if (needsConcurrent) {
        bufferInfo.pQueueFamilyIndices = families;
    }

    buffer = vk::raii::Buffer(device, bufferInfo);
    vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
    };
    bufferMemory = vk::raii::DeviceMemory(device, allocInfo);
    buffer.bindMemory(*bufferMemory, 0);
}

void TextureManager::createImage(uint32_t width, uint32_t height, uint32_t mipLevelsIn, vk::Format format,
                                 vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                                 vk::MemoryPropertyFlags properties, vk::raii::Image &image,
                                 vk::raii::DeviceMemory &imageMemory) {
    log_info("TextureManager::createImage() started");
    const bool needsConcurrent = (usage & vk::ImageUsageFlagBits::eTransferSrc ||
                                  usage & vk::ImageUsageFlagBits::eTransferDst) &&
                                 transferQueueFamilyIndex != UINT32_MAX &&
                                 transferQueueFamilyIndex != graphicsQueueFamilyIndex;

    uint32_t families[2] = {graphicsQueueFamilyIndex, transferQueueFamilyIndex};

    vk::ImageCreateInfo imageInfo{
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = {width, height, 1},
        .mipLevels = mipLevelsIn,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = needsConcurrent ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
        .queueFamilyIndexCount = needsConcurrent ? 2u : 0u,
        .pQueueFamilyIndices = needsConcurrent ? families : nullptr
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

vk::raii::CommandBuffer TextureManager::beginSingleTimeCommands(const vk::raii::Queue &queue) {
    log_info("TextureManager::beginSingleTimeCommands() started");
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };
    auto commandBuffers = device.allocateCommandBuffers(allocInfo);
    vk::raii::CommandBuffer commandBuffer = std::move(commandBuffers[0]);
    vk::CommandBufferBeginInfo beginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    };
    commandBuffer.begin(beginInfo);
    return commandBuffer;
}

void TextureManager::endSingleTimeCommands(vk::raii::CommandBuffer &commandBuffer, const vk::raii::Queue &queue) {
    log_info("TextureManager::endSingleTimeCommands() started");
    commandBuffer.end();
    vk::SubmitInfo submitInfo{
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffer
    };
    queue.submit(submitInfo, nullptr);
    queue.waitIdle();
}

// void TextureManager::transitionImageLayout(vk::raii::CommandBuffer &commandBuffer, vk::Image image,
//                                            uint32_t mipLevelsIn, vk::ImageLayout oldLayout,
//                                            vk::ImageLayout newLayout) {
//     std::println("TextureManager::transitionImageLayout() started");
//     vk::ImageMemoryBarrier barrier{
//         .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//         .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
//         .image = image,
//         .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, mipLevelsIn, 0, 1}
//     };
//
//     vk::PipelineStageFlags sourceStage;
//     vk::PipelineStageFlags destinationStage;
//
//     if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
//         barrier.srcAccessMask = vk::AccessFlagBits::eNone;
//         barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
//         sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
//         destinationStage = vk::PipelineStageFlagBits::eTransfer;
//     } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
//                newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
//         barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
//         barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
//         sourceStage = vk::PipelineStageFlagBits::eTransfer;
//         destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
//     } else {
//         throw std::invalid_argument("unsupported layout transition");
//     }
//
//     commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, nullptr, nullptr, barrier);
// }

void TextureManager::copyBufferToImage(vk::raii::CommandBuffer &commandBuffer, const vk::raii::Buffer &buffer,
                                       const vk::raii::Image &image, uint32_t width, uint32_t height) {
    log_info("TextureManager::copyBufferToImage() started");
    vk::BufferImageCopy region{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
        .imageOffset = {0, 0, 0},
        .imageExtent = {width, height, 1}
    };

    commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, {region});
}

void TextureManager::generateMipmaps(vk::raii::Image &image, vk::Format imageFormat, int32_t texWidth,
                                     int32_t texHeight, uint32_t mipLevelsIn) {
    log_info("TextureManager::generateMipmaps() started");
    vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(imageFormat);
    if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        throw std::runtime_error("Texture image format does not support linear blitting!");
    }

    auto commandBuffer = beginSingleTimeCommands(graphicsQueue);

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevelsIn; i++) {
        vk::ImageMemoryBarrier barrier{
            .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
            .dstAccessMask = vk::AccessFlagBits::eTransferRead,
            .oldLayout = vk::ImageLayout::eTransferDstOptimal,
            .newLayout = vk::ImageLayout::eTransferSrcOptimal,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image,
            .subresourceRange = {vk::ImageAspectFlagBits::eColor, i - 1, 1, 0, 1}
        };

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
                                      {}, nullptr, nullptr, barrier);

        vk::ImageBlit blit{};
        blit.srcSubresource = {vk::ImageAspectFlagBits::eColor, i - 1, 0, 1};
        blit.srcOffsets[0] = vk::Offset3D(0, 0, 0);
        blit.srcOffsets[1] = vk::Offset3D(mipWidth, mipHeight, 1);
        blit.dstSubresource = {vk::ImageAspectFlagBits::eColor, i, 0, 1};
        blit.dstOffsets[0] = vk::Offset3D(0, 0, 0);
        blit.dstOffsets[1] = vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1,
                                          mipHeight > 1 ? mipHeight / 2 : 1, 1);

        commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image,
                                vk::ImageLayout::eTransferDstOptimal, {blit}, vk::Filter::eLinear);

        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                      vk::PipelineStageFlagBits::eFragmentShader, {}, nullptr, nullptr, barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    vk::ImageMemoryBarrier barrier{
        .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
        .dstAccessMask = vk::AccessFlagBits::eShaderRead,
        .oldLayout = vk::ImageLayout::eTransferDstOptimal,
        .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {vk::ImageAspectFlagBits::eColor, mipLevelsIn - 1, 1, 0, 1}
    };

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                                  {}, nullptr, nullptr, barrier);

    endSingleTimeCommands(commandBuffer, graphicsQueue);
}

