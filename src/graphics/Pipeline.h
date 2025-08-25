#pragma once
#include "../core/Utils.h"
#include "../resources/ResourceManager.h"

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


    ResourceManager *resourceManager;
    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::raii::Pipeline graphicsPipeline = nullptr;
};