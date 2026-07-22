#pragma once

#include "vk_device.hpp"
#include "types.hpp"
#include "../Constants.h"
#include "vk_allocator.hpp"
#include "object_storage.hpp"
#include "../util/debug.hpp"
#include "../util/vk_tracy.hpp"
#include "../util/vk_utils.hpp"
#include "../static_headers/logger.hpp"
#include "vk_descriptors.hpp"
#include "ktxvulkan.h"



// Handles loading a single texture and exposes sampler + view for pipelines.
class TextureManager {
public:
    explicit TextureManager(Device &deviceWrapper, const VkAllocator &allocator, DescriptorManager &descriptorManager);
    ~TextureManager();

    void init();

    [[nodiscard]] const vk::raii::Sampler &getTextureSampler() const { return textureSampler; }
    [[nodiscard]] const vk::ImageViewCreateInfo &gettextureImageViewCreateInfo() const { return textureImageViewCreateInfo; }
    [[nodiscard]] uint32_t getMipLevels() const { return mipLevels; }
    [[nodiscard]] uint32_t createTextureImage(std::string texturePath_);

    // ── format-detecting loader (non-integrated) ──────────
    // Inspects the file extension and routes to the KTX or
    // stb (PNG/etc.) pipeline accordingly.
    [[nodiscard]] uint32_t loadTexture(std::string_view texturePath);

private:
    void createTextureSampler();

    auto findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) -> uint32_t;
    void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
                      vk::raii::Buffer &buffer, VmaAllocation &bufferMemory,
                      std::string_view memoryDebugBaseName = "TextureBufferMemory");
    vk::ImageCreateInfo createImage(uint32_t width, uint32_t height, uint32_t mipLevelsIn, vk::Format format,
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
    const VkAllocator &allocator;
    DescriptorManager &descriptorManager;
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
