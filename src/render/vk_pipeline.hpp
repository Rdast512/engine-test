#pragma once

#include "../core/vk_resource_manager.hpp"
#include "../core/types.hpp"
#include <vulkan/vulkan_raii.hpp>
#include <filesystem>

class DescriptorManager;

class Pipeline {
public:
    Pipeline(ResourceManager *resourceManager,
             DescriptorManager* descriptorManager,
             const vk::raii::Device &device,
             const vk::Extent2D &swapChainExtent,
             const vk::Format &swapChainImageFormat);
    ~Pipeline() = default;

    void init();
    void createGraphicsPipeline();

    const vk::raii::Device &device;
    const vk::Extent2D &swapChainExtent;
    const vk::Format &swapChainImageFormat;
    ResourceManager *resourceManager;
    DescriptorManager* descriptorManager;
    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::raii::Pipeline graphicsPipeline = nullptr;
};