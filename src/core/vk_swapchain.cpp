#include "vk_swapchain.hpp"

#include <SDL3/SDL.h>
#include "../util/debug.hpp"
#include "../util/logger.hpp"
#include <format>
#include <algorithm>
#include <limits>

SwapChain::SwapChain(SDL_Window *window, Device *device)
    : window(window),
      device(device),
      physicalDevice(device->getPhysicalDevice()),
      surface(device->getSurface()),
      vkdevice(device->getDevice()),
      queueFamilyIndices(device->getQueueFamilyIndices()) {}

void SwapChain::init() {
    log_info("Initialized SwapChain");
    createSwapChain();
    createImageViews();
}

vk::SurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
    for (const auto &availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

vk::PresentModeKHR SwapChain::chooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR> &availablePresentModes) {
    for (const auto &availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D SwapChain::chooseSwapExtent(
    const vk::SurfaceCapabilitiesKHR &capabilities) const {
    if (capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    int width, height;
    SDL_GetWindowSizeInPixels(window, &width, &height);
    log_info(std::format("Window size: {}x{}", width, height));

    return {std::clamp<uint32_t>(static_cast<uint32_t>(width), capabilities.minImageExtent.width,
                                 capabilities.maxImageExtent.width),
            std::clamp<uint32_t>(static_cast<uint32_t>(height), capabilities.minImageExtent.height,
                                 capabilities.maxImageExtent.height)};
}


void SwapChain::createSwapChain() {

    auto surfaceCapabilities =
        physicalDevice.getSurfaceCapabilitiesKHR(surface);
    swapChainSurfaceFormat =
        chooseSwapSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(surface));
    swapChainImageFormat = swapChainSurfaceFormat.format;
    swapChainExtent = chooseSwapExtent(surfaceCapabilities);
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);

    minImageCount = (surfaceCapabilities.maxImageCount > 0 &&
                     minImageCount > surfaceCapabilities.maxImageCount)
                        ? surfaceCapabilities.maxImageCount
                        : minImageCount;

    vk::SwapchainCreateInfoKHR swapChainCreateInfo{
        .flags = vk::SwapchainCreateFlagsKHR(),
        .surface = surface,
        .minImageCount = minImageCount,
        .imageFormat = swapChainSurfaceFormat.format,
        .imageColorSpace = swapChainSurfaceFormat.colorSpace,
        .imageExtent = swapChainExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = queueFamilyIndices.size() > 1
                                ? vk::SharingMode::eConcurrent
                                : vk::SharingMode::eExclusive,
        .queueFamilyIndexCount =
            static_cast<uint32_t>(queueFamilyIndices.size()),
        .pQueueFamilyIndices = queueFamilyIndices.data(),
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = chooseSwapPresentMode(
            physicalDevice.getSurfacePresentModesKHR(surface)),
        .clipped = true,
        .oldSwapchain = nullptr};

    swapChain = vk::raii::SwapchainKHR(vkdevice, swapChainCreateInfo);
    swapChainImages = swapChain.getImages();
    for (size_t i = 0; i < swapChainImages.size(); ++i) {
        setDebugName(vkdevice, swapChainImages[i], std::format("SwapchainImage_{}", i));
    }
 }


 void SwapChain::createImageViews() {
     vk::ImageViewCreateInfo imageViewCreateInfo{
         .viewType = vk::ImageViewType::e2D,
         .format = swapChainImageFormat,
         .subresourceRange = {vk::ImageAspectFlagBits::eColor,  0, 1, 0, 1}};
     for (auto image : swapChainImages) {
         imageViewCreateInfo.image = image;
         swapChainImageViews.emplace_back(vkdevice, imageViewCreateInfo);
         setDebugName(vkdevice, swapChainImageViews.back(), std::format("SwapchainImageView_{}", swapChainImageViews.size() - 1));
     }
 }

void SwapChain::cleanupSwapChain() {
    swapChainImageViews.clear();
    swapChain = nullptr;
}

void SwapChain::recreateSwapChain() {
    vkdevice.waitIdle();

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
}