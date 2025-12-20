#pragma once
#include "../core/vk_resource_manager.hpp"
#include "../core/types.hpp"

class Pipeline {
public:

    Pipeline(ResourceManager* resourceManager,
         const vk::raii::Device& device,
         const vk::Extent2D& swapChainExtent,
         const vk::Format& swapChainImageFormat,
         const vk::raii::DescriptorSetLayout& descriptorSetLayout);
    ~Pipeline() = default;

    void init();
    void createGraphicsPipeline();


    const vk::raii::Device& device;
    const vk::Extent2D& swapChainExtent;
    const vk::Format& swapChainImageFormat;
    const vk::raii::DescriptorSetLayout& descriptorSetLayout;
    const VulkanContext* context;


    ResourceManager *resourceManager;
    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::raii::Pipeline graphicsPipeline = nullptr;
};