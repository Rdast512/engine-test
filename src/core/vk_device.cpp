#include "vk_device.hpp"
#include "../util/debug.hpp"
#include "../static_headers/logger.hpp"
#include <format>

const std::vector validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};


Device::Device(SDL_Window *window, bool enableValidationLayers) : window(window),
enableValidationLayers(enableValidationLayers)
   {
}

void Device::init() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
}

void Device::createInstance() {
    constexpr vk::ApplicationInfo appInfo{
        .pApplicationName = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = vk::ApiVersion14
    };


    Uint32 count_instance_extensions;
    const char *const *instance_extensions = SDL_Vulkan_GetInstanceExtensions(&count_instance_extensions);

    auto extensionProperties = context.enumerateInstanceExtensionProperties();

    // Check if all required SDL3 extensions are supported
    for (uint32_t i = 0; i < count_instance_extensions; ++i) {
        if (std::ranges::none_of(extensionProperties,
                                 [glfwExtension = instance_extensions[i]](auto const &extensionProperty) {
                                     return strcmp(extensionProperty.extensionName, glfwExtension) == 0;
                                 })) {
            throw std::runtime_error(
                "Required SDL3 extension not supported: " + std::string(instance_extensions[i]));
        }
    }
    std::cout << "available extensions:\n";
    for (const auto &extension: extensionProperties) {
        std::cout << "  " << extension.extensionName << std::endl;
    }

    // Create a vector to hold the required extensions
    std::vector extensions(instance_extensions, instance_extensions + count_instance_extensions);
    if (enableValidationLayers) {
        // extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }
    // Always add debug utils extension for easier debugging
    extensions.push_back(vk::EXTDebugUtilsExtensionName);

    // Get the required layers
    std::vector<char const *> requiredLayers;
    if (enableValidationLayers) {
        requiredLayers.assign(validationLayers.begin(), validationLayers.end());
    }
    auto layerProperties = context.enumerateInstanceLayerProperties();
    if (std::ranges::any_of(requiredLayers, [&layerProperties](auto const &requiredLayer) {
        return std::ranges::none_of(layerProperties,
                                    [requiredLayer](auto const &layerProperty) {
                                        return strcmp(layerProperty.layerName, requiredLayer) == 0;
                                    });
    })) {
        throw std::runtime_error("One or more required layers are not supported!");
    }


    vk::InstanceCreateInfo createInfo{
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames = requiredLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };
    instance = vk::raii::Instance(context, createInfo);

    try {
        instance = vk::raii::Instance(context, createInfo);
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to create Vulkan instance: " + std::string(err.what()));
    }
}

static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                                      vk::DebugUtilsMessageTypeFlagsEXT type,
                                                      const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                      void *) {
    std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;

    return vk::False;
}


void Device::setupDebugMessenger() {
    if (!enableValidationLayers) return;

    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
        .messageSeverity = severityFlags,
        .messageType = messageTypeFlags,
        .pfnUserCallback = &debugCallback
    };
    debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
}


void Device::createSurface() {
    VkSurfaceKHR _surface;
    if (!SDL_Vulkan_CreateSurface(window, *instance, nullptr, &_surface)) {
        throw std::runtime_error("Failed to create surface");
    }
    surface = vk::raii::SurfaceKHR(instance, _surface);
}

vk::SampleCountFlagBits Device::getMaxUsableSampleCount() {
    vk::PhysicalDeviceProperties physicalDeviceProperties = physicalDevice.getProperties2().properties;

    vk::SampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
                                  physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
    if (counts & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
    if (counts & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
    if (counts & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
    if (counts & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
    if (counts & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }

    return vk::SampleCountFlagBits::e1;
}


void Device::pickPhysicalDevice() {
    auto devices = instance.enumeratePhysicalDevices();
    if (devices.empty()) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    std::multimap<int, vk::raii::PhysicalDevice> candidates;
    for (const auto &device: devices) {
        auto props2 = device.getProperties2();
        auto deviceProperties = props2.properties;
        auto features2 = device.getFeatures2();
        deviceFeatures = features2.features;
        uint32_t score = 0;

        // Discrete GPUs have a significant performance advantage
        if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            score += 1000;
        }

        // Maximum possible size of textures affects graphics quality
        score += deviceProperties.limits.maxImageDimension2D;

        // Application can't function without geometry shaders
        if (!deviceFeatures.geometryShader) {
            continue;
        }
        candidates.insert(std::make_pair(score, device));
    }
    if (candidates.rbegin()->first > 0) {
        physicalDevice = candidates.rbegin()->second;
    } else {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
    auto ret = physicalDevice.getQueueFamilyProperties2();

    log_info(std::format("Using physical device: {}", std::string_view(physicalDevice.getProperties2().properties.deviceName.data())));
    log_info(std::format("Queue amount: {}", ret.size()));
    for (const auto &qfp: ret) {
        const bool graphics = (qfp.queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
        const bool compute = (qfp.queueFamilyProperties.queueFlags & vk::QueueFlagBits::eCompute) != static_cast<vk::QueueFlags>(0);
        const bool transfer = (qfp.queueFamilyProperties.queueFlags & vk::QueueFlagBits::eTransfer) != static_cast<vk::QueueFlags>(0);
        log_info(std::format("Queue family count: {} graphics: {} compute: {} transfer: {}", qfp.queueFamilyProperties.queueCount,
                             graphics, compute, transfer));
    }
}

void Device::createLogicalDevice() {
    // find the index of the first queue family that supports graphics

    auto queueFamilyProperties2 = physicalDevice.getQueueFamilyProperties2();

    // get the first index into queueFamilyProperties which supports graphics
    auto graphicsQueueFamilyProperty = std::ranges::find_if(queueFamilyProperties2, [](auto const &qfp) {
        return (qfp.queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
    });
    graphicsIndex = static_cast<uint32_t>(
        std::distance(queueFamilyProperties2.begin(), graphicsQueueFamilyProperty));


    // determine a queueFamilyIndex that supports present
    // first check if the graphicsIndex is good enough
    auto presentIndex = physicalDevice.getSurfaceSupportKHR(graphicsIndex, *surface)
                            ? graphicsIndex
                            : static_cast<uint32_t>(queueFamilyProperties2.size());
    if (presentIndex == queueFamilyProperties2.size()) {
        // the graphicsIndex doesn't support present -> look for another family index that supports both
        // graphics and present
        for (size_t i = 0; i < queueFamilyProperties2.size(); i++) {
            if ((queueFamilyProperties2[i].queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics) &&
                physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *surface)) {
                graphicsIndex = static_cast<uint32_t>(i);
                presentIndex = graphicsIndex;
                break;
            }
        }
        if (presentIndex == queueFamilyProperties2.size()) {
            // there's nothing like a single family index that supports both graphics and present -> look for another
            // family index that supports present
            for (size_t i = 0; i < queueFamilyProperties2.size(); i++) {
                if (physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *surface)) {
                    presentIndex = static_cast<uint32_t>(i);
                    break;
                }
            }
        }
    }
    if ((graphicsIndex == queueFamilyProperties2.size()) || (presentIndex == queueFamilyProperties2.size())) {
        throw std::runtime_error("Could not find a queue for graphics or present -> terminating");
    }

    // Find dedicated transfer queue family (transfer-only, no graphics)
    transferIndex = static_cast<uint32_t>(queueFamilyProperties2.size());
    for (size_t i = 0; i < queueFamilyProperties2.size(); i++) {
        if ((queueFamilyProperties2[i].queueFamilyProperties.queueFlags & vk::QueueFlagBits::eTransfer) &&
            !(queueFamilyProperties2[i].queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics)) {
            transferIndex = static_cast<uint32_t>(i);
            break;
        }
    }

    // query for Vulkan features
    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT, vk::PhysicalDeviceDescriptorHeapFeaturesEXT,
        vk::PhysicalDeviceShaderFloatControls2FeaturesKHR> featureChain = {
        {.features = {.sampleRateShading = true, .samplerAnisotropy = true}}, // vk::PhysicalDeviceFeatures2
        {.shaderDrawParameters = true},
        {.synchronization2 = true, .dynamicRendering = true}, // vk::PhysicalDeviceVulkan13Features
        {.extendedDynamicState = true}, // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
        {.descriptorHeap = true}, // vk::PhysicalDeviceDescriptorHeapFeaturesEXT
        {.shaderFloatControls2 = true} // vk::PhysicalDeviceShaderFloatControls2FeaturesKHR
    };

    // Create queue create infos for both graphics and transfer queues
    float queuePriority = 0.0f;
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

    // Graphics queue
    queueCreateInfos.push_back({
        .queueFamilyIndex = graphicsIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    });

    // Transfer queue (if available and different from graphics)
    if (transferIndex != queueFamilyProperties2.size() && transferIndex != graphicsIndex) {
        queueCreateInfos.push_back({
            .queueFamilyIndex = transferIndex,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority
        });
    }

    // create a Device
    vk::DeviceCreateInfo deviceCreateInfo{
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size()),
        .ppEnabledExtensionNames = requiredDeviceExtension.data()
    };

    vkdevice = vk::raii::Device(physicalDevice, deviceCreateInfo);

    setDebugName(vkdevice, instance, "Instance");
    setDebugName(vkdevice, physicalDevice, "PhysicalDevice");
    setDebugName(vkdevice, surface, "WindowSurface");
    setDebugName(vkdevice, vkdevice, "LogicalDevice");

    // Cache supported MSAA sample count for downstream components (e.g., pipelines, resources).
    msaaSamples = getMaxUsableSampleCount();

    // Print queue family usage
    if (graphicsIndex == presentIndex) {
        log_info(std::format("Using single queue for graphics and present: {}", graphicsIndex));
    } else {
        log_info("Using separate queues for graphics and present");
    }

    if (transferIndex != queueFamilyProperties2.size() && transferIndex != graphicsIndex) {
        log_info(std::format("Using dedicated transfer queue family {}", transferIndex));
    } else {
        log_info("No dedicated transfer queue found, will use graphics queue for transfers");
    }

    if (transferIndex != queueFamilyProperties2.size() && transferIndex != graphicsIndex) {
        transferQueue = vk::raii::Queue(vkdevice, transferIndex, 0);
    }
    if (transferIndex == queueFamilyProperties2.size()) {
        transferIndex = UINT32_MAX; // Mark as invalid
    }
    graphicsQueue = vk::raii::Queue(vkdevice, graphicsIndex, 0);
    presentQueue = vk::raii::Queue(vkdevice, presentIndex, 0);

    setDebugName(vkdevice, graphicsQueue, "GraphicsQueue");
    setDebugName(vkdevice, presentQueue, "PresentQueue");
    if (transferIndex != UINT32_MAX) {
        setDebugName(vkdevice, transferQueue, "TransferQueue");
    }
    log_info(std::format("Using graphics queue: {} | present queue: {} | transfer queue: {}", graphicsIndex,
                         presentIndex, (transferIndex != UINT32_MAX ? std::to_string(transferIndex) : "N/A")));

    queueFamilyIndices.push_back(graphicsIndex);

    // Only add transfer index if it's valid and different from graphics
    if (transferIndex != UINT32_MAX && transferIndex != graphicsIndex) {
        queueFamilyIndices.push_back(transferIndex);
    }

    if (graphicsIndex != presentIndex) {
        queueFamilyIndices.push_back(presentIndex);
    }

}
