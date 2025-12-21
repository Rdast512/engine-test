#pragma once
#include <vulkan/vulkan_raii.hpp>

class Device{

    std::vector<const char *> requiredDeviceExtension = {
        vk::KHRSwapchainExtensionName,
        vk::KHRMaintenance7ExtensionName,
        vk::KHRMaintenance8ExtensionName
    };

    SDL_Window *window = nullptr;
    bool enableValidationLayers = false;
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    vk::raii::SurfaceKHR surface = nullptr;
    vk::raii::Device vkdevice = nullptr;
    vk::PhysicalDeviceFeatures deviceFeatures;

    vk::raii::Queue graphicsQueue = nullptr;
    vk::raii::Queue presentQueue = nullptr;
    vk::raii::Queue transferQueue = nullptr;
    std::vector<uint32_t> queueFamilyIndices;
    vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;
    
    uint32_t transferIndex = 0;
    uint32_t graphicsIndex = 0;
    uint32_t presentIndex = 0;
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
public:
    Device(SDL_Window *window, bool enableValidationLayers = false);
    ~Device() = default;
    void init();

    // Getters
    SDL_Window* getWindow() const { return window; }
    bool isValidationLayersEnabled() const { return enableValidationLayers; }
    const vk::raii::Context& getContext() const { return context; }
    const vk::raii::Instance& getInstance() const { return instance; }
    const vk::raii::DebugUtilsMessengerEXT& getDebugMessenger() const { return debugMessenger; }
    const vk::raii::PhysicalDevice& getPhysicalDevice() const { return physicalDevice; }
    const vk::raii::SurfaceKHR& getSurface() const { return surface; }
    const vk::raii::Device& getDevice() const { return vkdevice; }
    const vk::PhysicalDeviceFeatures& getDeviceFeatures() const { return deviceFeatures; }
    
    const vk::raii::Queue& getGraphicsQueue() const { return graphicsQueue; }
    const vk::raii::Queue& getPresentQueue() const { return presentQueue; }
    const vk::raii::Queue& getTransferQueue() const { return transferQueue; }
    const std::vector<uint32_t>& getQueueFamilyIndices() const { return queueFamilyIndices; }

    vk::SampleCountFlagBits getMaxUsableSampleCount();
    vk::SampleCountFlagBits getMsaaSamples() const { return msaaSamples; }
    uint32_t getTransferIndex() const { return transferIndex; }
    uint32_t getGraphicsIndex() const { return graphicsIndex; }
    uint32_t getPresentIndex() const { return presentIndex; }
    
    const std::vector<const char*>& getRequiredDeviceExtensions() const { return requiredDeviceExtension; }

    
    

};