#pragma once
#include <vulkan/vulkan_raii.hpp>
#include "vk_device.hpp"
#include <vulkan/vulkan_enums.hpp>

// TODO add callback for window resize

class SwapChain
{
    static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats);
    static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes);
    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) const;
    void createSwapChain();
    void createImageViews();
public:
    SwapChain(SDL_Window *window, Device *device);
    ~SwapChain() = default;

    void init();

    SDL_Window *window = nullptr;
    Device *device = nullptr;
    
    
    vk::raii::SwapchainKHR swapChain = nullptr;
    vk::SurfaceFormatKHR swapChainSurfaceFormat;
    vk::Extent2D swapChainExtent;
    std::vector<vk::Image> swapChainImages;
    vk::Format swapChainImageFormat = vk::Format::eUndefined;
    std::vector<vk::raii::ImageView> swapChainImageViews;
    
    const vk::raii::PhysicalDevice &physicalDevice = device->getPhysicalDevice();
    const vk::raii::SurfaceKHR &surface = device->getSurface();
    const vk::raii::Device &vkdevice = device->getDevice();
    const std::vector<uint32_t> &queueFamilyIndices = device->getQueueFamilyIndices();

    void recreateSwapChain();
    void cleanupSwapChain();


};