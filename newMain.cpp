#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstdlib>
#include "VkBootstrap.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif


struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

using namespace std;

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    SDL_Window* window = nullptr;
    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSwapchainKHR swapChain;
    VkExtent2D swapChainExtent;
    VkPipelineLayout pipelineLayout;
    VkFormat swapChainImageFormat;
    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;


    vkb::Instance vkb_instance;
    vkb::PhysicalDevice vkb_physical_device;
    vkb::Device vkb_device;
    vkb::Swapchain vkb_swapchain;

    void initWindow() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            throw std::runtime_error("SDL could not initialize!");
        }
        SDL_PropertiesID properties = SDL_CreateProperties();
        if (!properties) {
            throw std::runtime_error("Failed to create SDL properties");
        }

        SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_VULKAN_BOOLEAN, true);
        SDL_SetStringProperty(properties, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "My Window");
        SDL_SetBooleanProperty(properties, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
        SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 640);
        SDL_SetNumberProperty(properties, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 480);

        window = SDL_CreateWindowWithProperties(properties);
        if (!window) {
            throw std::runtime_error("Failed to create window");
        }
        SDL_DestroyProperties(properties);
    }



    void initVulkan() {
        createInstance();
        createSurface();
        createPhysicalDevice();
        createLogicalDevice();
        setupQueues();
        createSwapChain();
        createImageViews();
        createGraphicsPipeline();
    }

    void createInstance() {
        uint32_t extensionCount = 0;

        vkb::InstanceBuilder builder;
        Uint32 count_instance_extensions;
        const char * const *instance_extensions= SDL_Vulkan_GetInstanceExtensions(&count_instance_extensions);
        cout<< "Available instance extensions: " << endl;
        for (Uint32 i = 0; i < count_instance_extensions; ++i) {
            cout << "  " << instance_extensions[i] << endl;
            builder.enable_extension(instance_extensions[i]);
        }
        builder.require_api_version(1, 3, 0);
        auto system_info_ret = vkb::SystemInfo::get_system_info();
        if (!system_info_ret) {
            throw std::runtime_error(system_info_ret.error().message());
        }
        auto system_info = system_info_ret.value();
        builder.enable_layer("VK_LAYER_KHRONOS_validation");
        if (system_info.validation_layers_available) {
            builder.enable_validation_layers();
        }

        std::vector<const char*> extensions;
        for (auto& ext : system_info.available_extensions) {
            extensions.push_back(ext.extensionName);
        }

        auto inst_builder = builder.set_app_name ("Example Vulkan Application");


        inst_builder.add_debug_messenger_severity(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                                          | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        .add_debug_messenger_type(VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                                      | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                      | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
        .set_debug_callback([](VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                   VkDebugUtilsMessageTypeFlagsEXT type,
                                   const VkDebugUtilsMessengerCallbackDataEXT* callback,
                                   void*) -> VkBool32 {
                std::cerr << "validation layer: " << callback->pMessage << std::endl;
                return VK_FALSE;
            });;



        auto inst_ret = inst_builder.build();
        if (!inst_ret) {
            cout << "Failed to create Vulkan instance: " << inst_ret.error().message() << endl;
        }
        vkb_instance = inst_ret.value();
        instance = vkb_instance.instance;
    }

    void createSurface() {
        if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
            throw std::runtime_error("Failed to create surface");
        }
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        return true;
    }

    void createPhysicalDevice() {
        vkb::PhysicalDeviceSelector selector(vkb_instance);
        VkPhysicalDeviceFeatures features{};
        features.depthClamp = VK_TRUE;
        auto phys_ret = selector.set_surface(surface)
                                .set_required_features(features)
                                .require_dedicated_transfer_queue()
                                .add_required_extensions({VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME})
                                .select();
        if (!phys_ret) {
            throw std::runtime_error("Failed to select Physical Device: " + phys_ret.error().message());
        }
        vkb_physical_device = phys_ret.value();
        physicalDevice = vkb_physical_device.physical_device;
    }


    void createLogicalDevice() {
        vkb::DeviceBuilder device_builder(vkb_physical_device);
        auto dev_ret = device_builder.build();
        if (!dev_ret) {
            throw std::runtime_error("Failed to create Logical Device: " + dev_ret.error().message());
        }
        vkb_device = dev_ret.value();
        device = vkb_device.device;
    }

    void setupQueues() {
        auto gqueue_ret = vkb_device.get_queue(vkb::QueueType::graphics);
        if (!gqueue_ret) {
            throw std::runtime_error("Failed to get graphics queue: " + gqueue_ret.error().message());
        }
        graphicsQueue = gqueue_ret.value();

        auto pqueue_ret = vkb_device.get_queue(vkb::QueueType::present);
        if (!pqueue_ret) {
            throw std::runtime_error("Failed to get present queue: " + pqueue_ret.error().message());
        }
        presentQueue = pqueue_ret.value();
    }

    void createSwapChain(vkb::Swapchain* oldSwapchain = nullptr) {
        vkb::SwapchainBuilder builder{ vkb_device, surface };
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);
        if (oldSwapchain) {
            builder.set_old_swapchain(*oldSwapchain);
        }
        uint32_t imageCount = capabilities.minImageCount;
        builder.use_default_format_selection();
        builder.use_default_present_mode_selection();
        builder.set_desired_min_image_count(imageCount + 1);
        builder.use_default_image_usage_flags();
        builder.set_image_array_layer_count(1);
        builder.set_clipped(true);
        int width = 0, height = 0;
        SDL_GetWindowSizeInPixels(window, &width, &height);
        builder.set_desired_extent(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        auto swap_ret = builder.build();
        if (!swap_ret) {
            throw std::runtime_error("Failed to create swapchain: " + swap_ret.error().message());
        }
        vkb_swapchain = swap_ret.value();
        swapChain = vkb_swapchain.swapchain;
        swapChainImageFormat = vkb_swapchain.image_format;
        swapChainExtent = vkb_swapchain.extent;
        presentMode = vkb_swapchain.present_mode;
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
    }

    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;
            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }
        }
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }
        return shaderModule;
    }


    void createGraphicsPipeline() {
        auto vertShaderCode = readFile("shaders/vert.spv");
        auto fragShaderCode = readFile("shaders/frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);


    }

    void mainLoop() {
        bool quit = false;
        while (!quit) {
            SDL_Event e;
            while (SDL_PollEvent(&e) != 0) {
                if (e.type == SDL_EVENT_QUIT) {
                    quit = true;
                }
            }
        }
    }

    void cleanup() {
        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }
        vkb::destroy_swapchain(vkb_swapchain);
        vkb::destroy_device(vkb_device);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkb::destroy_instance(vkb_instance);

        SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}