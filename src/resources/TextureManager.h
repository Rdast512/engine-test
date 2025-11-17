#pragma once
#include "ResourceManager.h"
#include "../graphics/VulkanContext.h"

class TextureManager {
public:

    TextureManager(
        ResourceManager *resourceManager,
        const vk::raii::PhysicalDevice& physicalDevice,
        const vk::raii::Device& device
    );
    ~TextureManager() = default;

    void init();

    void createTextureImage();
    void createTextureImageView();
    void createTextureSampler();
    const vk::raii::Sampler& getTextureSampler() const { return textureSampler; }
    const vk::raii::ImageView& getTextureImageView() const { return textureImageView; }


    const vk::raii::PhysicalDevice& physicalDevice;
    const vk::raii::Device& device;
    vk::raii::Buffer& stagingBuffer;
    vk::raii::DeviceMemory& stagingBufferMemory;
    std::vector<vk::raii::CommandBuffer> &commandBuffers;
    std::vector<vk::raii::CommandBuffer> &transferCommandBuffer;
    const vk::raii::Queue &graphicsQueue;
    const vk::raii::Queue &transferQueue;
    const VulkanContext* context;



    vk::raii::Sampler textureSampler = nullptr;
    ResourceManager *resourceManager;
    uint32_t mipLevels = 0;
    vk::raii::Image textureImage = nullptr;
    vk::raii::DeviceMemory textureImageMemory = nullptr;
    vk::raii::ImageView textureImageView = nullptr;
};

