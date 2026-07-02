#pragma once
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "types.hpp"

class ResourceManager;

class DescriptorManager
{
    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSets();
    void createHeaps();
    void createHeapDescriptors();
    void createHeapBuffers(vk::DeviceSize resourceHeapSize, vk::DeviceSize samplerHeapSize);
    vk::DeviceSize minResourceHeapReservedRange = 0;
    vk::DeviceSize minSamplerHeapReservedRange = 0;

    vk::DeviceSize bufferDescriptorSize = 0;
    vk::DeviceSize samplerDescriptorSize = 0;
    vk::DeviceSize imageDescriptorSize = 0;

    vk::DeviceSize bufferDescriptorAlignment = 0;
    vk::DeviceSize samplerDescriptorAlignment = 0;
    vk::DeviceSize imageDescriptorAlignment = 0;

public:
    DescriptorManager(const vk::raii::Device& device, ResourceManager& resourceManager,
                      const HardwareCapabilities& capabilities);

    ~DescriptorManager();


    void init();


    [[nodiscard]] auto usesDescriptorHeaps() const -> bool
    {
        return descriptorBindingMode == DescriptorBindingMode::DescriptorHeaps;
    }
    [[nodiscard]] auto getDescriptorBindingMode() const -> DescriptorBindingMode { return descriptorBindingMode; }

    [[nodiscard]] auto getResourceHeapInfo() const -> const vk::BindHeapInfoEXT& { return resourceHeapInfo; }
    [[nodiscard]] auto getSamplerHeapInfo() const -> const vk::BindHeapInfoEXT& { return samplerHeapInfo; }
    [[nodiscard]] auto getTextureDescriptorIndex() const -> uint32_t;
    [[nodiscard]] auto getSamplerDescriptorIndex() const -> uint32_t;
    [[nodiscard]] auto writeImageDescriptor(const vk::ImageViewCreateInfo& imageView) -> uint32_t;



    const vk::raii::Device& device;
    ResourceManager& resourceManager;
    const std::vector<vk::raii::Buffer>& uniformBuffers;
    const HardwareCapabilities& capabilities;
    DescriptorBindingMode descriptorBindingMode = DescriptorBindingMode::DescriptorHeaps;


    vk::raii::Buffer resourceHeapBuffer = nullptr;
    vk::raii::Buffer samplerHeapBuffer = nullptr;

    VmaAllocation resourceHeapMemory = nullptr;
    VmaAllocation samplerHeapMemory = nullptr;

    void* mappedResourceHeapPtr = nullptr;
    void* mappedSamplerHeapPtr = nullptr;

    vk::BindHeapInfoEXT resourceHeapInfo{};
    vk::BindHeapInfoEXT samplerHeapInfo{};

    vk::DeviceSize textureDescriptorOffset = 0;
    vk::DeviceSize samplerDescriptorOffset = 0;
    vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
    vk::raii::DescriptorPool descriptorPool = nullptr;
    std::vector<vk::raii::DescriptorSet> descriptorSets;
};
