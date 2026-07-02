#pragma once

#include "vk_device.hpp"
#include "types.hpp"
#include "../Constants.h"
#include "vk_allocator.hpp"
#include "object_storage.hpp"
#include <cstdint>
#include <string_view>
#include "../../ThirdParty/stb_image.h"
#include "../util/debug.hpp"
#include "../util/vk_tracy.hpp"
#include "../util/vk_utils.hpp"
#include "../static_headers/logger.hpp"
#include "vk_descriptors.hpp"


struct TextureAsset {
    vk::raii::Image textureImage = nullptr;
    vk::raii::ImageView textureImageView = nullptr;;
    VmaAllocation textureImageMemory = nullptr;
    uint32_t descriptorHeapIndex;
};


// Handles loading a single texture and exposes sampler + view for pipelines.
class TextureManager {
public:
    explicit TextureManager(Device &deviceWrapper, VkAllocator &allocator);
    ~TextureManager();

    void init();

    [[nodiscard]] const vk::raii::Sampler &getTextureSampler() const { return textureSampler; }
    [[nodiscard]] const vk::ImageViewCreateInfo &gettextureImageViewCreateInfo() const { return textureImageViewCreateInfo; }
    [[nodiscard]] uint32_t getMipLevels() const { return mipLevels; }

private:
    void createTextureImageView(std::unordered_map<std::string, TextureAsset>::mapped_type* texture_asset);
    void createTextureImage(Object obj);
    void createTextureImageView();
    void createTextureSampler();

    auto findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) -> uint32_t;
    void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
                      vk::raii::Buffer &buffer, VmaAllocation &bufferMemory,
                      std::string_view memoryDebugBaseName = "TextureBufferMemory");
    [[nodiscard]] vk::ImageCreateInfo createImage(uint32_t width, uint32_t height, uint32_t mipLevelsIn, vk::Format format,
                     vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties,
                     vk::raii::Image &image, VmaAllocation &imageMemory,
                     std::string_view memoryDebugBaseName = "TextureImageMemory");

    auto beginSingleTimeCommands(const vk::raii::Queue &queue) -> vk::raii::CommandBuffer;
    void endSingleTimeCommands(vk::raii::CommandBuffer &commandBuffer, const vk::raii::Queue &queue);
    // void transitionImageLayout(vk::raii::CommandBuffer &commandBuffer, vk::Image image, uint32_t mipLevels,
    //                            vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
    void copyBufferToImage(vk::raii::CommandBuffer &commandBuffer, const vk::raii::Buffer &buffer,
                           const vk::raii::Image &image, uint32_t width, uint32_t height);
    void generateMipmaps(vk::raii::Image &image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight,
                         uint32_t mipLevels);

    Device &deviceWrapper;
    VkAllocator &allocator;
    const vk::raii::PhysicalDevice &physicalDevice;
    const vk::raii::Device &device;
    const vk::raii::Queue &graphicsQueue;
    const vk::raii::Queue &transferQueue;
    uint32_t graphicsQueueFamilyIndex;
    uint32_t transferQueueFamilyIndex;

    std::unordered_map<std::string, TextureAsset> loadedTextures;
    vk::raii::CommandPool commandPool = nullptr;
    vk::raii::Buffer stagingBuffer = nullptr;
    VmaAllocation stagingBufferMemory = nullptr;
    vk::raii::Sampler textureSampler = nullptr;
    vk::ImageViewCreateInfo textureImageViewCreateInfo;
    uint32_t mipLevels = 0;
};
