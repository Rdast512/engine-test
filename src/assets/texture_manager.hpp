#pragma once

#include "../core/vk_device.hpp"
#include "../core/types.hpp"
#include "../Constants.h"
#include "src/core/vk_resource_manager.hpp"

#include <vulkan/vulkan_raii.hpp>
#include <cstdint>

// Handles loading a single texture and exposes sampler + view for pipelines.
class TextureManager {
public:
    explicit TextureManager(Device &deviceWrapper, ResourceManager &resourceManager);
    ~TextureManager() = default;

    void init();

    const vk::raii::Sampler &getTextureSampler() const { return textureSampler; }
    const vk::raii::ImageView &getTextureImageView() const { return textureImageView; }
    uint32_t getMipLevels() const { return mipLevels; }

private:
    void createTextureImage();
    void createTextureImageView();
    void createTextureSampler();

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
    void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
                      vk::raii::Buffer &buffer, vk::raii::DeviceMemory &bufferMemory);
    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format,
                     vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties,
                     vk::raii::Image &image, vk::raii::DeviceMemory &imageMemory);

    vk::raii::CommandBuffer beginSingleTimeCommands(const vk::raii::Queue &queue);
    void endSingleTimeCommands(vk::raii::CommandBuffer &commandBuffer, const vk::raii::Queue &queue);
    // void transitionImageLayout(vk::raii::CommandBuffer &commandBuffer, vk::Image image, uint32_t mipLevels,
    //                            vk::ImageLayout oldLayout, vk::ImageLayout newLayout);
    void copyBufferToImage(vk::raii::CommandBuffer &commandBuffer, const vk::raii::Buffer &buffer,
                           const vk::raii::Image &image, uint32_t width, uint32_t height);
    void generateMipmaps(vk::raii::Image &image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight,
                         uint32_t mipLevels);

    Device &deviceWrapper;
    ResourceManager &resourceManager;
    const vk::raii::PhysicalDevice &physicalDevice;
    const vk::raii::Device &device;
    const vk::raii::Queue &graphicsQueue;
    const vk::raii::Queue &transferQueue;
    uint32_t graphicsQueueFamilyIndex;
    uint32_t transferQueueFamilyIndex;

    vk::raii::CommandPool commandPool = nullptr;
    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;
    vk::raii::Sampler textureSampler = nullptr;
    vk::raii::Image textureImage = nullptr;
    vk::raii::DeviceMemory textureImageMemory = nullptr;
    vk::raii::ImageView textureImageView = nullptr;
    uint32_t mipLevels = 0;
};
