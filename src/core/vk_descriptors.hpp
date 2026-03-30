#pragma once
#include <vulkan/vulkan_raii.hpp>
#include <vector>

#include "types.hpp"

class ResourceManager;

class DescriptorManager {
public:

    DescriptorManager(const vk::raii::Device& device,
                      ResourceManager& resourceManager,
                      const std::vector<vk::raii::Buffer>& uniformBuffers,
                      const vk::raii::Sampler& textureSampler,
                      const vk::raii::ImageView& textureImageView,
                      const vk::ImageViewCreateInfo& textureImageViewCreateInfo,
                      const HardwareCapabilities& capabilities);

    ~DescriptorManager() = default;

    void init();

    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSets();

    const vk::raii::Device& device;
    ResourceManager& resourceManager;
    const std::vector<vk::raii::Buffer>& uniformBuffers;
    const vk::raii::Sampler& textureSampler;
    const vk::raii::ImageView& textureImageView;
    const vk::ImageViewCreateInfo& textureImageViewCreateInfo;
    const HardwareCapabilities& capabilities;

    void createHeaps();
    void writeDescriptors();

    // Cached descriptor sizes from device properties
    uint32_t minResourceHeapReservedRange = capabilities.descriptorHeap.minResourceHeapReservedRange;
    uint32_t minSamplerHeapReservedRange = capabilities.descriptorHeap.minSamplerHeapReservedRange;

    uint32_t bufferDescriptorSize = capabilities.descriptorHeap.bufferDescriptorSize;
    uint32_t samplerDescriptorSize = capabilities.descriptorHeap.samplerDescriptorSize;
    uint32_t imageDescriptorSize = capabilities.descriptorHeap.imageDescriptorSize;

    uint32_t bufferDescriptorAlignment = capabilities.descriptorHeap.bufferDescriptorAlignment;
    uint32_t samplerDescriptorAlignment = capabilities.descriptorHeap.samplerDescriptorAlignment;
    uint32_t imageDescriptorAlignment = capabilities.descriptorHeap.imageDescriptorAlignment;

    vk::raii::Buffer resourceHeapBuffer = nullptr;
    vk::raii::Buffer samplerHeapBuffer = nullptr;

    VmaAllocation resourceHeapMemory = nullptr;
    VmaAllocation samplerHeapMemory = nullptr;

    void* mappedResourceHeapPtr = nullptr;
    void* mappedSamplerHeapPtr = nullptr;

    vk::BindHeapInfoEXT resourceHeapInfo{};
    vk::BindHeapInfoEXT samplerHeapInfo{};

    std::vector<vk::DeviceSize> uboDescriptorOffsets;
    vk::DeviceSize textureDescriptorOffset = 0;
    vk::DeviceSize samplerDescriptorOffset = 0;
    // vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
    // vk::raii::DescriptorPool descriptorPool = nullptr;
    // std::vector<vk::raii::DescriptorSet> descriptorSets;

};
