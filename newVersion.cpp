#define VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE 1
#include <memory>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <map>
#include <optional>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <assert.h>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vk_platform.h>
#include <vector>
#include <unordered_map>
#include <ranges>
#include <algorithm>
#include <ThirdParty/termcolor.hpp>
#include <fstream>
#include <chrono>
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define VMA_VULKAN_VERSION 1004000 // Corrected for Vulkan 1.4
#define VMA_IMPLEMENTATION
#include "vma/vk_mem_alloc.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <ThirdParty/tiny_obj_loader.h>

// TODO add functions to ownership
// TODO rewrite sync functions to async (like for ones what fill up command buffers)
// TODO add support for GLTF and KTX2
// TODO Imgui integration
// TODO Instanced rendering
// TODO Dynamic uniforms
// TODO Pipeline cache
// TODO Multi-threaded command buffer generation
// TODO Multiple subpasses
// TODO Integrate VMA for memory management
// TODO Integrate Volk
// TODO Rebase project structure to use different files and dirs
// TODO (createVertexBuffer, createIndexBuffer, createTextureImage) are still using the "single-time command" pattern


constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
constexpr uint64_t FenceTimeout = 100000000;
const std::string MODEL_PATH = "../../models/room.obj";
const std::string TEXTURE_PATH = "../../textures/viking_room.png";


const std::vector validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};
#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = false;
#endif


const vk::ImageSubresourceRange DEFAULT_COLOR_SUBRESOURCE_RANGE = {
    vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
};

static std::vector<char> readFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
    std::vector<char> buffer(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    file.close();

    return buffer;
}


struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    bool operator==(const Vertex &other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }

    static vk::VertexInputBindingDescription getBindingDescription() {
        return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
    }

    static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
            vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord))
        };
    }
};

namespace std {
    template<>
    struct hash<Vertex> {
        size_t operator()(Vertex const &vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                     (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                   (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};


class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    // Member Variables
    SDL_Window *window = nullptr;
    vk::raii::Context context;
    vk::raii::Instance instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr;
    vk::raii::PhysicalDevice physicalDevice = nullptr;
    vk::raii::SurfaceKHR surface = nullptr;
    vk::raii::Device device = nullptr;
    vk::PhysicalDeviceFeatures deviceFeatures;
    vk::raii::Queue graphicsQueue = nullptr;
    vk::raii::Queue presentQueue = nullptr;
    vk::raii::Queue transferQueue = nullptr;
    vk::raii::SwapchainKHR swapChain = nullptr;
    vk::SurfaceFormatKHR swapChainSurfaceFormat;
    vk::Extent2D swapChainExtent;
    std::vector<vk::Image> swapChainImages;
    vk::Format swapChainImageFormat = vk::Format::eUndefined;
    std::vector<vk::raii::ImageView> swapChainImageViews;
    vk::raii::PipelineLayout pipelineLayout = nullptr;
    vk::raii::Pipeline graphicsPipeline = nullptr;
    vk::raii::CommandPool commandPool = nullptr;
    vk::raii::CommandPool transferCommandPool = nullptr;
    std::vector<vk::raii::CommandBuffer> commandBuffers;
    std::vector<vk::raii::CommandBuffer> transferCommandBuffer;
    vk::raii::Buffer vertexBuffer = nullptr;
    vk::raii::DeviceMemory vertexBufferMemory = nullptr;
    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;
    vk::raii::Buffer indexBuffer = nullptr;
    vk::raii::DeviceMemory indexBufferMemory = nullptr;
    std::vector<vk::raii::Buffer> uniformBuffers;
    std::vector<vk::raii::DeviceMemory> uniformBuffersMemory;
    std::vector<void *> uniformBuffersMapped;
    std::vector<vk::raii::Semaphore> presentCompleteSemaphore;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphore;
    std::vector<vk::raii::Fence> inFlightFences;
    vk::raii::DescriptorSetLayout descriptorSetLayout = nullptr;
    vk::raii::DescriptorPool descriptorPool = nullptr;
    std::vector<vk::raii::DescriptorSet> descriptorSets;
    vk::raii::Image textureImage = nullptr;
    vk::raii::DeviceMemory textureImageMemory = nullptr;
    std::vector<uint32_t> queueFamilyIndices;
    vk::raii::ImageView textureImageView = nullptr;
    vk::raii::Sampler textureSampler = nullptr;
    vk::raii::Image depthImage = nullptr;
    vk::raii::DeviceMemory depthImageMemory = nullptr;
    vk::raii::ImageView depthImageView = nullptr;


    uint32_t transferIndex = 0;
    uint32_t graphicsIndex = 0;
    uint32_t presentIndex = 0;
    uint32_t currentFrame = 0;
    uint32_t semaphoreIndex = 0;
    uint32_t mipLevels;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    bool framebufferResized = false;
    std::chrono::high_resolution_clock::time_point lastTime;
    std::chrono::high_resolution_clock::time_point fpsTime;
    int frameCount = 0;
    float fps = 0.0f;

    std::vector<const char *> requiredDeviceExtension = {
        vk::KHRSwapchainExtensionName,
        vk::KHRMaintenance7ExtensionName,
        vk::KHRMaintenance8ExtensionName
    };

    //================================================================================
    // 1. Initialization Flow
    //================================================================================

    void initWindow() {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            throw std::runtime_error("Failed to initialize SDL");
        }

        window = SDL_CreateWindow("Vulkan", WIDTH, HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
        if (!window) {
            throw std::runtime_error("Failed to create window");
        }
    }

    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createCommandPool();
        createCommandBuffers();
        createDepthResources();
        createTextureImage();
        createTextureImageView();
        createTextureSampler();
        loadModel();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createSyncObjects();
    }

    //--------------------------------------------------------------------------------
    // 1a. initVulkan() Sub-steps
    //--------------------------------------------------------------------------------

    void createInstance() {
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
            extensions.push_back(vk::EXTDebugUtilsExtensionName);
        }


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

    void setupDebugMessenger() {
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

    void createSurface() {
        VkSurfaceKHR _surface;
        if (!SDL_Vulkan_CreateSurface(window, *instance, nullptr, &_surface)) {
            throw std::runtime_error("Failed to create surface");
        }
        surface = vk::raii::SurfaceKHR(instance, _surface);
    }

    void pickPhysicalDevice() {
        auto devices = instance.enumeratePhysicalDevices();
        if (devices.empty()) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }
        std::multimap<int, vk::raii::PhysicalDevice> candidates;
        for (const auto &device: devices) {
            auto deviceProperties = device.getProperties();
            auto deviceFeatures = device.getFeatures();
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
        std::cout << termcolor::green << "Using physical device: " << physicalDevice.getProperties().deviceName <<
                std::endl;
        std::cout << termcolor::green << "Queue amount: " << ret.size() << std::endl;
        for (const auto &qfp: ret) {
            std::cout << termcolor::green << "Queue family: " << qfp.queueFamilyProperties.queueCount
                    << " graphics: " << ((qfp.queueFamilyProperties.queueFlags & vk::QueueFlagBits::eGraphics) !=
                                         static_cast<vk::QueueFlags>(0))
                    << " compute: " << ((qfp.queueFamilyProperties.queueFlags & vk::QueueFlagBits::eCompute) !=
                                        static_cast<vk::QueueFlags>(0))
                    << " transfer: " << ((qfp.queueFamilyProperties.queueFlags & vk::QueueFlagBits::eTransfer) !=
                                         static_cast<vk::QueueFlags>(0))
                    << " present: " << qfp.queueFamilyProperties.queueCount << std::endl;
        }
    }

    void createLogicalDevice() {
        // find the index of the first queue family that supports graphics
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

        // get the first index into queueFamilyProperties which supports graphics
        auto graphicsQueueFamilyProperty = std::ranges::find_if(queueFamilyProperties, [](auto const &qfp) {
            return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
        });

        graphicsIndex = static_cast<uint32_t>(
            std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

        // determine a queueFamilyIndex that supports present
        // first check if the graphicsIndex is good enough
        auto presentIndex = physicalDevice.getSurfaceSupportKHR(graphicsIndex, *surface)
                                ? graphicsIndex
                                : static_cast<uint32_t>(queueFamilyProperties.size());
        if (presentIndex == queueFamilyProperties.size()) {
            // the graphicsIndex doesn't support present -> look for another family index that supports both
            // graphics and present
            for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
                if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
                    physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *surface)) {
                    graphicsIndex = static_cast<uint32_t>(i);
                    presentIndex = graphicsIndex;
                    break;
                }
            }
            if (presentIndex == queueFamilyProperties.size()) {
                // there's nothing like a single family index that supports both graphics and present -> look for another
                // family index that supports present
                for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
                    if (physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *surface)) {
                        presentIndex = static_cast<uint32_t>(i);
                        break;
                    }
                }
            }
        }
        if ((graphicsIndex == queueFamilyProperties.size()) || (presentIndex == queueFamilyProperties.size())) {
            throw std::runtime_error("Could not find a queue for graphics or present -> terminating");
        }

        // Find dedicated transfer queue family (transfer-only, no graphics)
        transferIndex = static_cast<uint32_t>(queueFamilyProperties.size());
        for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
            if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eTransfer) &&
                !(queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
                !(queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eCompute)) {
                transferIndex = static_cast<uint32_t>(i);
                break;
            }
        }

        // query for Vulkan features
        vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features,
            vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
            vk::PhysicalDeviceShaderFloatControls2FeaturesKHR> featureChain = {
            {.features = {.samplerAnisotropy = true}}, // vk::PhysicalDeviceFeatures2
            {.shaderDrawParameters = true},
            {.synchronization2 = true, .dynamicRendering = true, .maintenance4 = true},
            // vk::PhysicalDeviceVulkan13Features
            {.extendedDynamicState = true}, // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
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
        if (transferIndex != queueFamilyProperties.size() && transferIndex != graphicsIndex) {
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

        device = vk::raii::Device(physicalDevice, deviceCreateInfo);

        // Print queue family usage
        if (graphicsIndex == presentIndex) {
            std::cout << termcolor::green << "\nUsing single queue for graphics and present : " << graphicsIndex <<
                    std::endl;
        } else {
            std::cout << termcolor::green << "Using separate queues for graphics and present" << std::endl;
        }

        if (transferIndex != queueFamilyProperties.size() && transferIndex != graphicsIndex) {
            std::cout << termcolor::green << "Using dedicated transfer queue family " << transferIndex << std::endl;
        } else {
            std::cout << termcolor::yellow << "No dedicated transfer queue found, will use graphics queue for transfers"
                    << std::endl;
        }

        if (transferIndex != queueFamilyProperties.size() && transferIndex != graphicsIndex) {
            transferQueue = vk::raii::Queue(device, transferIndex, 0);
        }
        if (transferIndex == queueFamilyProperties.size()) {
            transferIndex = UINT32_MAX; // Mark as invalid
        }
        graphicsQueue = vk::raii::Queue(device, graphicsIndex, 0);
        presentQueue = vk::raii::Queue(device, presentIndex, 0);
        std::cout << termcolor::green << "Using graphics queue: " << graphicsIndex << "\nUsing present queue: " <<
                presentIndex
                << "\nUsing transfer queue: " << (transferIndex != UINT32_MAX ? std::to_string(transferIndex) : "N/A")
                << std::endl;

        queueFamilyIndices.push_back(graphicsIndex);

        // Only add transfer index if it's valid and different from graphics
        if (transferIndex != UINT32_MAX && transferIndex != graphicsIndex) {
            queueFamilyIndices.push_back(transferIndex);
        }

        if (graphicsIndex != presentIndex) {
            queueFamilyIndices.push_back(presentIndex);
        }
    }


    static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
        for (const auto &availableFormat: availableFormats) {
            if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace ==
                vk::ColorSpaceKHR::eSrgbNonlinear) {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes) {
        for (const auto &availablePresentMode: availablePresentModes) {
            if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
                return availablePresentMode;
            }
        }
        return vk::PresentModeKHR::eFifo;
    }

    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities) const {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        int width, height;
        SDL_GetWindowSizeInPixels(window, &width, &height);
        std::cout << termcolor::green << "Window size: " << width << "x" << height << std::endl;

        return {
            std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
        };
    }

    void createSwapChain() {
        auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
        swapChainSurfaceFormat = chooseSwapSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(surface));
        swapChainImageFormat = swapChainSurfaceFormat.format;
        swapChainExtent = chooseSwapExtent(surfaceCapabilities);
        auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);

        minImageCount = (surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount)
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
            .queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size()),
            .pQueueFamilyIndices = queueFamilyIndices.data(),
            .preTransform = surfaceCapabilities.currentTransform,
            .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
            .presentMode = chooseSwapPresentMode(physicalDevice.getSurfacePresentModesKHR(surface)),
            .clipped = true,
            .oldSwapchain = nullptr
        };

        swapChain = vk::raii::SwapchainKHR(device, swapChainCreateInfo);
        swapChainImages = swapChain.getImages();
    }

    void createImageViews() {
        vk::ImageViewCreateInfo imageViewCreateInfo{
            .viewType = vk::ImageViewType::e2D, .format = swapChainImageFormat,
            .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
        };
        for (auto image: swapChainImages) {
            imageViewCreateInfo.image = image;
            swapChainImageViews.emplace_back(device, imageViewCreateInfo);
        }
    }


    [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char> &code) const {
        vk::ShaderModuleCreateInfo createInfo{
            .codeSize = code.size() * sizeof(char), .pCode = reinterpret_cast<const uint32_t *>(code.data())
        };
        vk::raii::ShaderModule shaderModule{device, createInfo};
        return shaderModule;
    }

    void createDescriptorSetLayout() {
        std::array bindings = {
            vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex,
                                           nullptr),
            vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1,
                                           vk::ShaderStageFlagBits::eFragment, nullptr)
        };

        vk::DescriptorSetLayoutCreateInfo layoutInfo{
            .bindingCount = static_cast<uint32_t>(bindings.size()), .pBindings = bindings.data()
        };
        descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
    }

    void createGraphicsPipeline() {
        vk::raii::ShaderModule shaderModule = createShaderModule(readFile("shaders/shader.spv"));
        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();
        vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = shaderModule, .pName = "vertMain"
        };
        vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = shaderModule, .pName = "fragMain"
        };
        vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
        std::vector dynamicStates = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };


        vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
            .vertexBindingDescriptionCount = 1, .pVertexBindingDescriptions = &bindingDescription,
            .vertexAttributeDescriptionCount = attributeDescriptions.size(),
            .pVertexAttributeDescriptions = attributeDescriptions.data()
        };
        vk::PipelineDynamicStateCreateInfo dynamicState{
            .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
            .pDynamicStates = dynamicStates.data()
        };
        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{.topology = vk::PrimitiveTopology::eTriangleList};
        vk::Viewport{
            0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f,
            1.0f
        };
        vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1, .scissorCount = 1};
        vk::PipelineRasterizationStateCreateInfo rasterizer{
            .depthClampEnable = vk::False, .rasterizerDiscardEnable = vk::False,
            .polygonMode = vk::PolygonMode::eFill, .cullMode = vk::CullModeFlagBits::eBack,
            .frontFace = vk::FrontFace::eCounterClockwise, .depthBiasEnable = vk::False,
            .depthBiasSlopeFactor = 1.0f, .lineWidth = 1.0f
        };
        vk::PipelineMultisampleStateCreateInfo multisampling{
            .rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False
        };
        vk::PipelineColorBlendAttachmentState colorBlendAttachment;
        colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR |
                                              vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
                                              vk::ColorComponentFlagBits::eA;
        colorBlendAttachment.blendEnable = vk::False;

        vk::PipelineColorBlendStateCreateInfo colorBlending{
            .logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1,
            .pAttachments = &colorBlendAttachment
        };
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
            .setLayoutCount = 1,
            .pSetLayouts = &*descriptorSetLayout,
            .pushConstantRangeCount = 0
        };
        vk::PipelineDepthStencilStateCreateInfo depthStencil{
            .depthTestEnable = vk::True,
            .depthWriteEnable = vk::True,
            .depthCompareOp = vk::CompareOp::eLess,
            .depthBoundsTestEnable = vk::False,
            .stencilTestEnable = vk::False
        };
        vk::Format depthFormat = findDepthFormat();
        pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);
        vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &swapChainImageFormat,
            .depthAttachmentFormat = depthFormat
        };
        vk::GraphicsPipelineCreateInfo pipelineInfo{
            .pNext = &pipelineRenderingCreateInfo,
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pDepthStencilState = &depthStencil,
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamicState,
            .layout = pipelineLayout,
            .renderPass = nullptr
        };
        graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
    }

    void createCommandPool() {
        vk::CommandPoolCreateInfo poolInfo{
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, .queueFamilyIndex = graphicsIndex
        };
        commandPool = vk::raii::CommandPool(device, poolInfo);
        vk::CommandPoolCreateInfo transferPoolInfo{
            .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, .queueFamilyIndex = transferIndex
        };
        if (transferIndex != graphicsIndex) {
            transferCommandPool = vk::raii::CommandPool(device, transferPoolInfo);
        }
    }

    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
        vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
                      vk::raii::Buffer &buffer, vk::raii::DeviceMemory &bufferMemory) {
        vk::BufferCreateInfo bufferInfo{
            .size = size, .usage = usage, .sharingMode = vk::SharingMode::eConcurrent,
            .queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size()),
            .pQueueFamilyIndices = queueFamilyIndices.data()
        };
        buffer = vk::raii::Buffer(device, bufferInfo);
        vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo{
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
        };
        bufferMemory = vk::raii::DeviceMemory(device, allocInfo);
        buffer.bindMemory(*bufferMemory, 0);
    }

    void copyBuffer(vk::raii::Buffer &srcBuffer, vk::raii::Buffer &dstBuffer, vk::DeviceSize size) {
        transferCommandBuffer[0].begin(vk::CommandBufferBeginInfo{
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        });
        transferCommandBuffer[0].copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, 0, size));
        transferCommandBuffer[0].end();
        vk::CommandBufferSubmitInfo commandBufferInfo = {
            .commandBuffer = *transferCommandBuffer[0]
        };
        const vk::SubmitInfo2 submitInfo{
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &commandBufferInfo
        };
        transferQueue.submit2(submitInfo, nullptr);
        transferQueue.waitIdle();
    }

    void createVertexBuffer() {
        std::cout << termcolor::green << "Creating vertex buffer with " << vertices.size() << " vertices." << std::endl;

        vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();


        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                     stagingBuffer, stagingBufferMemory);

        void *dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
        memcpy(dataStaging, vertices.data(), bufferSize);
        stagingBufferMemory.unmapMemory();

        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                     vk::MemoryPropertyFlagBits::eDeviceLocal, vertexBuffer, vertexBufferMemory);

        copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
    }

    void createIndexBuffer() {
        vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                     stagingBuffer, stagingBufferMemory);

        void *data = stagingBufferMemory.mapMemory(0, bufferSize);
        memcpy(data, indices.data(), (size_t) bufferSize);
        stagingBufferMemory.unmapMemory();

        createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                     vk::MemoryPropertyFlagBits::eDeviceLocal, indexBuffer, indexBufferMemory);

        copyBuffer(stagingBuffer, indexBuffer, bufferSize);
    }

    void createUniformBuffers() {
        uniformBuffers.clear();
        uniformBuffersMemory.clear();
        uniformBuffersMapped.clear();

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
            vk::raii::Buffer buffer({});
            vk::raii::DeviceMemory bufferMem({});
            createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
                         vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer,
                         bufferMem);
            uniformBuffers.emplace_back(std::move(buffer));
            uniformBuffersMemory.emplace_back(std::move(bufferMem));
            uniformBuffersMapped.emplace_back(uniformBuffersMemory[i].mapMemory(0, bufferSize));
        }
    }

    void createDescriptorPool() {
        std::array poolSize{
            vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT),
            vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT)
        };
        vk::DescriptorPoolCreateInfo poolInfo{
            .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            .maxSets = MAX_FRAMES_IN_FLIGHT,
            .poolSizeCount = static_cast<uint32_t>(poolSize.size()),
            .pPoolSizes = poolSize.data()
        };
        descriptorPool = vk::raii::DescriptorPool(device, poolInfo);
    }


    void createDescriptorSets() {
        std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        vk::DescriptorSetAllocateInfo allocInfo{
            .descriptorPool = descriptorPool,
            .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
            .pSetLayouts = layouts.data()
        };

        descriptorSets.clear();
        descriptorSets = device.allocateDescriptorSets(allocInfo);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vk::DescriptorBufferInfo bufferInfo{
                .buffer = uniformBuffers[i],
                .offset = 0,
                .range = sizeof(UniformBufferObject)
            };
            vk::DescriptorImageInfo imageInfo{
                .sampler = textureSampler,
                .imageView = textureImageView,
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
            };
            std::array descriptorWrites{
                vk::WriteDescriptorSet{
                    .dstSet = descriptorSets[i],
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = vk::DescriptorType::eUniformBuffer,
                    .pBufferInfo = &bufferInfo
                },
                vk::WriteDescriptorSet{
                    .dstSet = descriptorSets[i],
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                    .pImageInfo = &imageInfo
                }
            };
            device.updateDescriptorSets(descriptorWrites, {});
        }
    }


    void createTextureImage() {
        int texWidth, texHeight, texChannels;
        stbi_uc *pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        vk::DeviceSize imageSize = texWidth * texHeight * 4;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        // Calculate mip levels
        mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        // Check if the format supports linear blitting for mipmap generation. Do this before any work.
        vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(vk::Format::eR8G8B8A8Srgb);
        if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
            throw std::runtime_error("Texture image format does not support linear blitting!");
        }

        // Create and fill the staging buffer (same as before)
        createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc,
                     vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                     stagingBuffer, stagingBufferMemory);
        void *data = stagingBufferMemory.mapMemory(0, imageSize);
        memcpy(data, pixels, imageSize);
        stagingBufferMemory.unmapMemory();
        stbi_image_free(pixels);

        // Create the final image with all mip levels and correct usage flags
        createImage(texWidth, texHeight, mipLevels, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal,
                    vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst |
                    vk::ImageUsageFlagBits::eSampled,
                    vk::MemoryPropertyFlagBits::eDeviceLocal,
                    textureImage, textureImageMemory);

        auto &graphicsCmd = commandBuffers[0];
        graphicsCmd.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        // Transition the entire image to be a transfer destination
        // This prepares all mip levels to be written to.
        transitionImageLayout(&graphicsCmd, *textureImage, mipLevels,
                              vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

        // Copy the staging buffer to the first mip level (level 0)
        copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth),
                          static_cast<uint32_t>(texHeight));
        endCommandBuffer(graphicsCmd, graphicsQueue);
        generateMipmaps(textureImage, vk::Format::eR8G8B8A8Srgb, texWidth, texHeight, mipLevels);
    }

    void generateMipmaps(vk::raii::Image &image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight,
                         uint32_t mipLevels) {
        // Check for blit support
        vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(imageFormat);
        if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
            throw std::runtime_error("Texture image format does not support linear blitting!");
        }

        auto &graphicsCmd = commandBuffers[0];
        graphicsCmd.begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++) {
            // Transition the previous mip level (i-1) from DST to SRC
            vk::ImageMemoryBarrier barrier_to_src = {
                .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
                .dstAccessMask = vk::AccessFlagBits::eTransferRead,
                .oldLayout = vk::ImageLayout::eTransferDstOptimal,
                .newLayout = vk::ImageLayout::eTransferSrcOptimal,
                .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
                .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
                .image = *image,
                .subresourceRange = { vk::ImageAspectFlagBits::eColor, i - 1, 1, 0, 1 }
            };
            graphicsCmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier_to_src);

            // Create and set up the blit structure
            vk::ImageBlit blit{};
            blit.srcSubresource = { vk::ImageAspectFlagBits::eColor, i - 1, 0, 1 };
            blit.srcOffsets[0] = vk::Offset3D(0, 0, 0);
            blit.srcOffsets[1] = vk::Offset3D(mipWidth, mipHeight, 1);
            blit.dstSubresource = { vk::ImageAspectFlagBits::eColor, i, 0, 1 };
            blit.dstOffsets[0] = vk::Offset3D(0, 0, 0);
            blit.dstOffsets[1] = vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1);

            graphicsCmd.blitImage(*image, vk::ImageLayout::eTransferSrcOptimal, *image,
                                  vk::ImageLayout::eTransferDstOptimal, {blit}, vk::Filter::eLinear);

            // Update dimensions for the next iteration
            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        // Transition the last mip level from DST to SRC to unify layouts
        vk::ImageMemoryBarrier barrier_last = {
            .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
            .dstAccessMask = vk::AccessFlagBits::eTransferRead,
            .oldLayout = vk::ImageLayout::eTransferDstOptimal,
            .newLayout = vk::ImageLayout::eTransferSrcOptimal,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .image = *image,
            .subresourceRange = { vk::ImageAspectFlagBits::eColor, mipLevels-1, 1, 0, 1 }
        };
        graphicsCmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                    vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier_last);

        // Final transition: all mip levels to shader read-only
        vk::ImageMemoryBarrier final_barrier = {
            .srcAccessMask = vk::AccessFlagBits::eTransferRead,
            .dstAccessMask = vk::AccessFlagBits::eShaderRead,
            .oldLayout = vk::ImageLayout::eTransferSrcOptimal,
            .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
            .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
            .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
            .image = *image,
            .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0, 1 }
        };

        graphicsCmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                    vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, final_barrier);

        endCommandBuffer(graphicsCmd, graphicsQueue);
    }

    void endCommandBuffer(vk::raii::CommandBuffer &commandBuffer, vk::raii::Queue &queue) {
        commandBuffer.end();
        vk::SubmitInfo submitInfo{.commandBufferCount = 1, .pCommandBuffers = &*commandBuffer};
        queue.submit(submitInfo, nullptr);
        queue.waitIdle();
    }

    void copyBufferToImage(const vk::raii::Buffer &buffer, vk::raii::Image &image, uint32_t width, uint32_t height) {
        vk::BufferImageCopy region{
            .bufferOffset = 0, .bufferRowLength = 0, .bufferImageHeight = 0,
            .imageSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
            .imageOffset = {0, 0, 0}, .imageExtent = {width, height, 1}
        };
        commandBuffers[0].copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, {region});
        // transferCommandBuffer[0].end();
        // vk::SubmitInfo submitInfo{ .commandBufferCount = 1, .pCommandBuffers = &*transferCommandBuffer[0] };
        // transferQueue.submit(submitInfo, nullptr);
        // transferQueue.waitIdle();
    }

    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::Format format, vk::ImageTiling tiling,
                     vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Image &image,
                     vk::raii::DeviceMemory &imageMemory) {
        // Determine sharing mode based on usage
        vk::SharingMode sharingMode = vk::SharingMode::eExclusive;
        std::vector<uint32_t> queueIndices;

        // Only use concurrent sharing for transfer operations between different queue families
        if ((usage & vk::ImageUsageFlagBits::eTransferSrc || usage & vk::ImageUsageFlagBits::eTransferDst) &&
            transferIndex != UINT32_MAX && transferIndex != graphicsIndex) {
            sharingMode = vk::SharingMode::eConcurrent;
            queueIndices = queueFamilyIndices;
        }

        vk::ImageCreateInfo imageInfo{
            .imageType = vk::ImageType::e2D,
            .format = format,
            .extent = {width, height, 1},
            .mipLevels = mipLevels,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = tiling,
            .usage = usage,
            .sharingMode = sharingMode,
            .queueFamilyIndexCount = static_cast<uint32_t>(queueIndices.size()),
            .pQueueFamilyIndices = queueIndices.data()
        };

        image = vk::raii::Image(device, imageInfo);

        vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
        vk::MemoryAllocateInfo allocInfo{
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
        };
        imageMemory = vk::raii::DeviceMemory(device, allocInfo);
        image.bindMemory(imageMemory, 0);
    }

    vk::raii::ImageView createImageView(vk::raii::Image &image, vk::Format format, vk::ImageAspectFlags aspectFlags
                                        , uint32_t mipLevels) {
        vk::ImageViewCreateInfo viewInfo{
            .image = image,
            .viewType = vk::ImageViewType::e2D,
            .format = format,
            .subresourceRange = {aspectFlags, 0, mipLevels, 0, 1}
        };
        return vk::raii::ImageView(device, viewInfo);
    }

    void createTextureImageView() {
        textureImageView = createImageView(textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor,
                                           mipLevels);
    }

    void createTextureSampler() {
        vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
        vk::SamplerCreateInfo samplerInfo{
            .magFilter = vk::Filter::eLinear,
            .minFilter = vk::Filter::eLinear,
            .mipmapMode = vk::SamplerMipmapMode::eLinear,
            .addressModeU = vk::SamplerAddressMode::eRepeat,
            .addressModeV = vk::SamplerAddressMode::eRepeat,
            .addressModeW = vk::SamplerAddressMode::eRepeat,
            .mipLodBias = 0.0f,
            .anisotropyEnable = vk::True,
            .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
            .compareEnable = vk::False,
            .compareOp = vk::CompareOp::eAlways
        };
        textureSampler = vk::raii::Sampler(device, samplerInfo);
    }

    void loadModel() {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;
        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, MODEL_PATH.c_str())) {
            throw std::runtime_error(err);
        }
        for (const auto &shape: shapes) {
            for (const auto &index: shape.mesh.indices) {
                Vertex vertex{};

                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                vertex.color = {1.0f, 1.0f, 1.0f};

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }
                indices.push_back(uniqueVertices[vertex]);
            }
        }
    }

    void createCommandBuffers() {
        commandBuffers.clear();
        vk::CommandBufferAllocateInfo allocInfo{
            .commandPool = commandPool, .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = MAX_FRAMES_IN_FLIGHT
        };
        commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
        if (transferIndex != UINT32_MAX && transferIndex != graphicsIndex) {
            vk::CommandBufferAllocateInfo transferAllocInfo{
                .commandPool = transferCommandPool, .level = vk::CommandBufferLevel::ePrimary,
                .commandBufferCount = 1
            };
            transferCommandBuffer = vk::raii::CommandBuffers(device, transferAllocInfo);
        }
        std::cout << termcolor::green << "Command buffers allocated: " << commandBuffers.size() << std::endl;
        std::cout << termcolor::green << "Transfer command buffers allocated: " << transferCommandBuffer.size() <<
                std::endl;
    }

    vk::Format findSupportedFormat(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling,
                                   vk::FormatFeatureFlags features) {
        for (const auto format: candidates) {
            vk::FormatProperties props = physicalDevice.getFormatProperties(format);

            if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    bool hasStencilComponent(vk::Format format) {
        return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
    }

    vk::Format findDepthFormat() {
        return findSupportedFormat({vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
                                   vk::ImageTiling::eOptimal,
                                   vk::FormatFeatureFlagBits::eDepthStencilAttachment);
    }

    void createDepthResources() {
        vk::Format depthFormat = findDepthFormat();
        createImage(swapChainExtent.width, swapChainExtent.height, 1, depthFormat, vk::ImageTiling::eOptimal,
                    vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal,
                    depthImage, depthImageMemory);
        depthImageView = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth, 1);
        commandBuffers[0].begin({.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        transitionImageLayout(&commandBuffers[0], depthImage, 1,
                              vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal
        );
        endCommandBuffer(commandBuffers[0], graphicsQueue);
    }

    void createSyncObjects() {
        presentCompleteSemaphore.clear();
        renderFinishedSemaphore.clear();
        inFlightFences.clear();

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            presentCompleteSemaphore.emplace_back(device, vk::SemaphoreCreateInfo());
            renderFinishedSemaphore.emplace_back(device, vk::SemaphoreCreateInfo());
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            inFlightFences.emplace_back(device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
        }
    }

    //================================================================================
    // 2. Main Loop Flow
    //================================================================================

    void mainLoop() {
        bool quit = false;
        bool minimized = false;
        lastTime = std::chrono::high_resolution_clock::now();
        fpsTime = lastTime;

        while (!quit) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            frameCount++;

            // Update FPS every second
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - fpsTime);
            if (duration.count() >= 1000) {
                fps = frameCount * 1000.0f / duration.count();
                frameCount = 0;
                fpsTime = currentTime;

                // Update window title with FPS
                std::string title = "Vulkan Triangle - FPS: " + std::to_string(static_cast<int>(fps));
                SDL_SetWindowTitle(window, title.c_str());
            }

            SDL_Event e;
            while (SDL_PollEvent(&e) != 0) {
                if (e.type == SDL_EVENT_QUIT) {
                    quit = true;
                } else if (e.type == SDL_EVENT_WINDOW_MINIMIZED) {
                    minimized = true;
                } else if (e.type == SDL_EVENT_WINDOW_RESTORED) {
                    minimized = false;
                }
            }

            if (!minimized && !quit) {
                drawFrame();
            } else {
                SDL_Delay(100);
            }
        }
        device.waitIdle();
    }

    //--------------------------------------------------------------------------------
    // 2a. mainLoop() Sub-steps (Drawing a Frame)
    //--------------------------------------------------------------------------------

    void drawFrame() {
        while (vk::Result::eTimeout == device.waitForFences(*inFlightFences[currentFrame], vk::True, UINT64_MAX));
        auto [result, imageIndex] = swapChain.acquireNextImage(
            UINT64_MAX, *presentCompleteSemaphore[semaphoreIndex], nullptr);

        if (result == vk::Result::eErrorOutOfDateKHR) {
            recreateSwapChain();
            return;
        }
        if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        device.resetFences(*inFlightFences[currentFrame]);
        commandBuffers[currentFrame].reset();
        recordCommandBuffer(imageIndex);

        // Define which semaphore to wait on, and at which pipeline stage.
        vk::SemaphoreSubmitInfo waitSemaphoreInfo = {
            .semaphore = *presentCompleteSemaphore[semaphoreIndex],
            .stageMask = vk::PipelineStageFlagBits2::eTopOfPipe
        };

        // Define the command buffer to be submitted.
        vk::CommandBufferSubmitInfo commandBufferInfo = {
            .commandBuffer = *commandBuffers[currentFrame]
        };

        // Define which semaphore to signal when the command buffer is done.
        vk::SemaphoreSubmitInfo signalSemaphoreInfo = {
            .semaphore = *renderFinishedSemaphore[imageIndex],
            .stageMask = vk::PipelineStageFlagBits2::eBottomOfPipe // Signal when all pipeline stages are complete
        };

        updateUniformBuffer(currentFrame);
        // Use the modern vk::SubmitInfo2 struct
        const vk::SubmitInfo2 submitInfo{
            .waitSemaphoreInfoCount = 1,
            .pWaitSemaphoreInfos = &waitSemaphoreInfo,
            .commandBufferInfoCount = 1,
            .pCommandBufferInfos = &commandBufferInfo,
            .signalSemaphoreInfoCount = 1,
            .pSignalSemaphoreInfos = &signalSemaphoreInfo
        };

        // Use vkQueueSubmit2 (via vulkan-hpp's dispatch)
        graphicsQueue.submit2(submitInfo, *inFlightFences[currentFrame]);

        const vk::PresentInfoKHR presentInfoKHR{
            .waitSemaphoreCount = 1, .pWaitSemaphores = &*renderFinishedSemaphore[imageIndex],
            .swapchainCount = 1, .pSwapchains = &*swapChain, .pImageIndices = &imageIndex
        };
        result = graphicsQueue.presentKHR(presentInfoKHR);

        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
            recreateSwapChain();
        } else if (result != vk::Result::eSuccess) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        switch (result) {
            case vk::Result::eSuccess: break;
            default: break;
        }
        semaphoreIndex = (semaphoreIndex + 1) % presentCompleteSemaphore.size();
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void updateUniformBuffer(uint32_t currentImage) {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        UniformBufferObject ubo{};
        ubo.model = rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f),
                                    static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.
                                        height), 0.1f, 10.0f);
        ubo.proj[1][1] *= -1;
        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }


    void transitionImageLayout(
        vk::raii::CommandBuffer *commandBuffer,
        vk::Image image,
        uint32_t mipLevels,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        const vk::ImageSubresourceRange &subresourceRange = DEFAULT_COLOR_SUBRESOURCE_RANGE,
        uint32_t srcQueueFamily = VK_QUEUE_FAMILY_IGNORED,
        uint32_t dstQueueFamily = VK_QUEUE_FAMILY_IGNORED,
        std::optional<vk::PipelineStageFlags2> srcStageMaskOverride = std::nullopt,
        std::optional<vk::PipelineStageFlags2> dstStageMaskOverride = std::nullopt,
        std::optional<vk::AccessFlags2> srcAccessMaskOverride = std::nullopt,
        std::optional<vk::AccessFlags2> dstAccessMaskOverride = std::nullopt
    ) {
        // Create a local copy that we can modify
        vk::ImageSubresourceRange localSubresourceRange = subresourceRange;

        vk::PipelineStageFlags2 srcStageMask;
        vk::PipelineStageFlags2 dstStageMask;
        vk::AccessFlags2 srcAccessMask;
        vk::AccessFlags2 dstAccessMask;

        // --- Automatic Barrier Deduction ---
        if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
            srcAccessMask = vk::AccessFlagBits2::eNone;
            dstAccessMask = vk::AccessFlagBits2::eTransferWrite;
            srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
            dstStageMask = vk::PipelineStageFlagBits2::eTransfer;
        } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout ==
                   vk::ImageLayout::eShaderReadOnlyOptimal) {
            srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
            dstAccessMask = vk::AccessFlagBits2::eShaderRead;
            srcStageMask = vk::PipelineStageFlagBits2::eTransfer;
            dstStageMask = vk::PipelineStageFlagBits2::eFragmentShader | vk::PipelineStageFlagBits2::eComputeShader;
        } else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
            srcAccessMask = vk::AccessFlagBits2::eNone;
            dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
            srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
            dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
        } else if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal && newLayout ==
                   vk::ImageLayout::ePresentSrcKHR) {
            srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
            dstAccessMask = vk::AccessFlagBits2::eNone;
            srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
            dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe;
        } else if (oldLayout == vk::ImageLayout::eUndefined && newLayout ==
                   vk::ImageLayout::eDepthStencilAttachmentOptimal) {
            srcAccessMask = vk::AccessFlagBits2::eNone;
            dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
            srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
            dstStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests;

            // Modify the local copy for depth layouts
            localSubresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
            if (hasStencilComponent(findDepthFormat())) {
                localSubresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
            }
        } else if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
            // Handle other transitions to depth layout
            localSubresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
            if (hasStencilComponent(findDepthFormat())) {
                localSubresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
            }
        } else {
            throw std::runtime_error("Unsupported automatic layout transition!");
        }
        localSubresourceRange.levelCount = mipLevels;
        vk::ImageMemoryBarrier2 imageBarrier = {
            .srcStageMask = srcStageMaskOverride.value_or(srcStageMask),
            .srcAccessMask = srcAccessMaskOverride.value_or(srcAccessMask),
            .dstStageMask = dstStageMaskOverride.value_or(dstStageMask),
            .dstAccessMask = dstAccessMaskOverride.value_or(dstAccessMask),
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .srcQueueFamilyIndex = srcQueueFamily,
            .dstQueueFamilyIndex = dstQueueFamily,
            .image = image,
            .subresourceRange = localSubresourceRange // Use the local copy
        };

        vk::DependencyInfo dependencyInfo = {
            .dependencyFlags = {},
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &imageBarrier
        };

        commandBuffer->pipelineBarrier2(dependencyInfo);
    }

    void recordCommandBuffer(uint32_t imageIndex) {
        commandBuffers[currentFrame].begin({});
        // Before starting rendering, transition the swapchain image to COLOR_ATTACHMENT_OPTIMAL
        transitionImageLayout(
            &commandBuffers[currentFrame],
            swapChainImages[imageIndex],
            1,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            DEFAULT_COLOR_SUBRESOURCE_RANGE,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            vk::PipelineStageFlagBits2::eTopOfPipe, // srcStage
            vk::PipelineStageFlagBits2::eColorAttachmentOutput, // dstStage
            {}, // srcAccessMask (no need to wait for previous operations)
            vk::AccessFlagBits2::eColorAttachmentWrite // dstAccessMask

        );

        vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
        vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);
        vk::RenderingAttachmentInfo colorAttachmentInfo = {
            .imageView = swapChainImageViews[imageIndex],
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clearColor
        };
        vk::RenderingAttachmentInfo depthAttachmentInfo = {
            .imageView = depthImageView,
            .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eDontCare,
            .clearValue = clearDepth
        };

        vk::RenderingInfo renderingInfo = {
            .renderArea = {.offset = {0, 0}, .extent = swapChainExtent},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentInfo,
            .pDepthAttachment = &depthAttachmentInfo
        };

        commandBuffers[currentFrame].beginRendering(renderingInfo);
        commandBuffers[currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
        commandBuffers[currentFrame].bindVertexBuffers(0, *vertexBuffer, {0});
        commandBuffers[currentFrame].bindIndexBuffer(*indexBuffer, 0, vk::IndexType::eUint32);
        commandBuffers[currentFrame].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout,
                                                        0, *descriptorSets[currentFrame], nullptr);
        commandBuffers[currentFrame].setViewport(0, vk::Viewport(0.0f, 0.0f,
                                                                 static_cast<float>(swapChainExtent.width),
                                                                 static_cast<float>(swapChainExtent.height), 0.0f,
                                                                 1.0f));
        commandBuffers[currentFrame].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent));
        commandBuffers[currentFrame].drawIndexed(indices.size(), 1, 0, 0, 0);
        commandBuffers[currentFrame].endRendering();
        // After rendering, transition the swapchain image to PRESENT_SRC
        transitionImageLayout(
            &commandBuffers[currentFrame],
            swapChainImages[imageIndex],
            1,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            DEFAULT_COLOR_SUBRESOURCE_RANGE,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput, // srcStage
            vk::PipelineStageFlagBits2::eBottomOfPipe, // dstStage
            vk::AccessFlagBits2::eColorAttachmentWrite, // srcAccessMask
            {} // dstAccessMask
        );
        commandBuffers[currentFrame].end();
    }

    //--------------------------------------------------------------------------------
    // 2b. Swapchain Recreation (during main loop)
    //--------------------------------------------------------------------------------

    void cleanupSwapChain() {
        swapChainImageViews.clear();
        swapChain = nullptr;
    }

    void recreateSwapChain() {
        device.waitIdle();

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createDepthResources();
    }

    //================================================================================
    // 3. Cleanup Flow
    //================================================================================

    void cleanup() {
        device.waitIdle();

        // Unmap uniform buffer memory before destruction
        for (auto &memory: uniformBuffersMemory) {
            memory.unmapMemory();
        }
        // Clean up SDL
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

int main() {
    try {
        HelloTriangleApplication app;
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
