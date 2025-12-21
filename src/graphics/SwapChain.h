#pragma once
#include "../resources/ResourceManager.h"
#include "VulkanContext.h"

class SwapChain
{
public:
    SwapChain(SDL_Window *window, VulkanContext *context, ResourceManager *resourceManager);
    ~SwapChain() = default;

    void init();

    SDL_Window *window = nullptr;
    VulkanContext *context = nullptr;
    ResourceManager *resourceManager = nullptr;

    vk::raii::SwapchainKHR swapChain = nullptr;
    vk::SurfaceFormatKHR swapChainSurfaceFormat;
    vk::Extent2D swapChainExtent;
    std::vector<vk::Image> swapChainImages;
    vk::Format swapChainImageFormat = vk::Format::eUndefined;
    std::vector<vk::raii::ImageView> swapChainImageViews;
    void setResourceManager(ResourceManager *rm) { resourceManager = rm; }

    const vk::raii::PhysicalDevice &physicalDevice = context->getPhysicalDevice();
    const vk::raii::SurfaceKHR &surface = context->getSurface();
    const vk::raii::Device &device = context->getDevice();
    const std::vector<uint32_t> &queueFamilyIndices = context->getQueueFamilyIndices();

    void recreateSwapChain();
    void cleanupSwapChain();

private:
    static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats);
    static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes);
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) const;
    void createSwapChain();
    void createImageViews();
};