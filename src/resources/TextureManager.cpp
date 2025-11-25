//
// Created by rdast on 24.08.2025.
//

#include "TextureManager.h"
#include "../core/Constants.h"

TextureManager::TextureManager(
    ResourceManager *resourceManager,
    const vk::raii::PhysicalDevice& physicalDevice,
    const vk::raii::Device& device
) : physicalDevice(physicalDevice), device(device),
    stagingBuffer(resourceManager->stagingBuffer),
    stagingBufferMemory(resourceManager->stagingBufferMemory),
    commandBuffers(resourceManager->commandBuffers), transferCommandBuffer(resourceManager->transferCommandBuffer),
    graphicsQueue(resourceManager->graphicsQueue),
    transferQueue(resourceManager->transferQueue),
    resourceManager(resourceManager) {
    // ...constructor now only assigns members (no function calls)...
}

// New: perform texture-related initialization after construction
void TextureManager::init() {
    std::cout << termcolor::green << "TextureManager initialized" << std::endl;
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
}

void TextureManager::createTextureSampler() {
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
        .compareOp = vk::CompareOp::eAlways
    };
    textureSampler = vk::raii::Sampler(device, samplerInfo);
}

void TextureManager::createTextureImageView() {
    textureImageView = resourceManager->createImageView(textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor,
                                       mipLevels);
}

    void TextureManager::createTextureImage() {
        int texWidth, texHeight, texChannels;
        const auto texturePath = TEXTURE_PATH.string();
        stbi_uc *pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        vk::DeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image: " + texturePath);
        }

        // Calculate mip levels
        mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        // Check if the format supports linear blitting for mipmap generation. Do this before any work.
        vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(vk::Format::eR8G8B8A8Srgb);
        if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
            throw std::runtime_error("Texture image format does not support linear blitting!");
        }

        // Create and fill the staging buffer (same as before)
        resourceManager->createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                     stagingBuffer, stagingBufferMemory);
        void *data = stagingBufferMemory.mapMemory(0, imageSize);
        memcpy(data, pixels, imageSize);
        stagingBufferMemory.unmapMemory();
        stbi_image_free(pixels);

        // Create the final image with all mip levels and correct usage flags
        resourceManager->createImage(texWidth, texHeight, mipLevels, vk::SampleCountFlagBits::e1, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal,
                    vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst |
                    vk::ImageUsageFlagBits::eSampled,
                    vk::MemoryPropertyFlagBits::eDeviceLocal,
                    textureImage, textureImageMemory);

    auto &graphicsCmd = commandBuffers[0];
    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    graphicsCmd.begin(beginInfo);
        // Transition the entire image to be a transfer destination
        // This prepares all mip levels to be written to.
        resourceManager->transitionImageLayout(&graphicsCmd, *textureImage, mipLevels,
                              vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

        // Copy the staging buffer to the first mip level (level 0)
        resourceManager->copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth),
                          static_cast<uint32_t>(texHeight));
        resourceManager->endCommandBuffer(graphicsCmd, graphicsQueue);
        resourceManager->generateMipmaps(textureImage, vk::Format::eR8G8B8A8Srgb, texWidth, texHeight, mipLevels);
    }