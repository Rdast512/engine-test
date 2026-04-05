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
                      const HardwareCapabilities& capabilities,
                      DescriptorBindingMode descriptorBindingMode);

    ~DescriptorManager();

    void init();

    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSets();
    void createHeaps();
    void createHeapBuffers(vk::DeviceSize resourceHeapSize, vk::DeviceSize samplerHeapSize);

    bool usesDescriptorHeaps() const { return descriptorBindingMode == DescriptorBindingMode::DescriptorHeaps; }
    DescriptorBindingMode getDescriptorBindingMode() const { return descriptorBindingMode; }

    const vk::BindHeapInfoEXT& getResourceHeapInfo() const { return resourceHeapInfo; }
    const vk::BindHeapInfoEXT& getSamplerHeapInfo() const { return samplerHeapInfo; }
    uint32_t getUboDescriptorIndex(uint32_t frameIndex) const;
    uint32_t getTextureDescriptorIndex() const;
    uint32_t getSamplerDescriptorIndex() const;

    const vk::raii::Device& device;
    ResourceManager& resourceManager;
    const std::vector<vk::raii::Buffer>& uniformBuffers;
    const vk::raii::Sampler& textureSampler;
    const vk::raii::ImageView& textureImageView;
    const vk::ImageViewCreateInfo& textureImageViewCreateInfo;
    const HardwareCapabilities& capabilities;
    DescriptorBindingMode descriptorBindingMode = DescriptorBindingMode::LegacySets;

    vk::DeviceSize minResourceHeapReservedRange = 0;
    vk::DeviceSize minSamplerHeapReservedRange = 0;

    vk::DeviceSize bufferDescriptorSize = 0;
    vk::DeviceSize samplerDescriptorSize = 0;
    vk::DeviceSize imageDescriptorSize = 0;

    vk::DeviceSize bufferDescriptorAlignment = 0;
    vk::DeviceSize samplerDescriptorAlignment = 0;
    vk::DeviceSize imageDescriptorAlignment = 0;

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
    vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
    vk::raii::DescriptorPool descriptorPool = nullptr;
    std::vector<vk::raii::DescriptorSet> descriptorSets;

};
