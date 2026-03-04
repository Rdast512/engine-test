#pragma once
#include "types.hpp"

/**
 * @brief Manages the Vulkan device stack: instance, physical device, logical device, and queues.
 *
 * Device encapsulates the full Vulkan device initialisation sequence. It owns the RAII
 * wrappers for every core Vulkan object and exposes accessor methods consumed by the rest
 * of the engine (swapchain, renderer, resource manager, etc.).
 *
 * Initialisation order (called via init()):
 *   1. createInstance()       – Vulkan instance + validation layers
 *   2. setupDebugMessenger()  – VK_EXT_debug_utils messenger (when validation is on)
 *   3. createSurface()        – SDL3 window surface
 *   4. pickPhysicalDevice()   – GPU selection by score
 *   5. createLogicalDevice()  – logical device + queues
 */
class Device{
    /**
     * @brief Required device extensions enabled on every logical device.
     *
     * Includes ray-tracing (KHR_acceleration_structure, KHR_ray_tracing_pipeline,
     * KHR_ray_query), mesh shaders (EXT_mesh_shader), variable-rate shading
     * (KHR_fragment_shading_rate), pipeline binaries, swapchain maintenance,
     * memory-budget/priority helpers, and GPU-driven rendering extensions.
     *
     * @note KHR_deferred_host_operations is a hard dependency of KHR_acceleration_structure.
     * @note KHR_present_id2 and KHR_calibrated_timestamps are required by EXT_present_timing.
     * @note KHR_pipeline_library is required by EXT_graphics_pipeline_library.
     */
    std::vector<const char *> requiredDeviceExtension = {
        //KHR
        vk::KHRSwapchainExtensionName,
        vk::KHRMaintenance7ExtensionName,
        vk::KHRMaintenance8ExtensionName,
        vk::KHRMaintenance9ExtensionName,
        vk::KHRMaintenance10ExtensionName,
        vk::KHRDeferredHostOperationsExtensionName, // required by KHR_acceleration_structure
        vk::KHRAccelerationStructureExtensionName,
        vk::KHRRayTracingPipelineExtensionName,
        vk::KHRPipelineBinaryExtensionName,
        vk::KHRFragmentShadingRateExtensionName,
        vk::KHRRayQueryExtensionName,
        vk::KHRSwapchainMaintenance1ExtensionName,
        vk::KHRRayTracingMaintenance1ExtensionName,
        vk::KHRPresentId2ExtensionName,           // required by EXT_present_timing
        vk::KHRCalibratedTimestampsExtensionName,  // required by EXT_present_timing
        vk::KHRPipelineLibraryExtensionName,       // required by EXT_graphics_pipeline_library
        vk::KHRPresentModeFifoLatestReadyExtensionName,
        //EXT
        vk::EXTMemoryBudgetExtensionName,
        vk::EXTMemoryPriorityExtensionName,
        vk::EXTDescriptorHeapExtensionName,
        vk::EXTBlendOperationAdvancedExtensionName,
        vk::EXTMeshShaderExtensionName,
        vk::EXTDeviceGeneratedCommandsExtensionName,
        vk::EXTMultiDrawExtensionName,
        vk::EXTPageableDeviceLocalMemoryExtensionName,
        vk::EXTShaderObjectExtensionName,
        vk::EXTGraphicsPipelineLibraryExtensionName,
        vk::EXTPresentTimingExtensionName,
        vk::EXTRayTracingInvocationReorderExtensionName
    };

    SDL_Window* window = nullptr;              ///< Non-owning pointer to the SDL3 window used for surface creation.
    bool enableValidationLayers = false;       ///< Whether Khronos validation layers are active.

    vk::raii::Context context;                 ///< Top-level RAII context; loads the Vulkan loader at construction.
    vk::raii::Instance instance = nullptr;     ///< Vulkan instance owning all per-application state.
    vk::raii::DebugUtilsMessengerEXT debugMessenger = nullptr; ///< Debug messenger; null when validation is disabled.
    vk::raii::PhysicalDevice physicalDevice = nullptr;         ///< Selected GPU handle (highest-score candidate).
    vk::raii::SurfaceKHR surface = nullptr;    ///< Platform surface created from the SDL3 window.
    vk::raii::Device vkdevice = nullptr;       ///< Logical device; primary interface for GPU commands and resource creation.
    vk::PhysicalDeviceFeatures deviceFeatures; ///< Feature set reported by the selected physical device.

    vk::raii::Queue graphicsQueue = nullptr;   ///< Queue supporting graphics operations.
    vk::raii::Queue presentQueue = nullptr;    ///< Queue supporting presentation; may alias graphicsQueue.
    vk::raii::Queue transferQueue = nullptr;   ///< Dedicated DMA/transfer queue when available.
    vk::raii::Queue computeQueue = nullptr;    ///< Dedicated async-compute queue when available.

    /// Unique queue family indices used by this device (passed to resource sharing mode setup).
    std::vector<uint32_t> queueFamilyIndices;

    /// Maximum MSAA sample count supported by both colour and depth attachments on this GPU.
    vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;

    uint32_t transferIndex = 0; ///< Queue family index of the transfer queue (UINT32_MAX if unavailable).
    uint32_t computeIndex  = 0; ///< Queue family index of the compute queue  (UINT32_MAX if unavailable).
    uint32_t graphicsIndex = 0; ///< Queue family index of the graphics queue.
    uint32_t presentIndex  = 0; ///< Queue family index of the present queue.

    HardwareCapabilities capabilities = HardwareCapabilities{}; ///< Cached hardware capability support flags (e.g., ray-tracing, mesh shaders).

    /** @brief Creates the Vulkan instance with required SDL3 and debug extensions. */
    void createInstance();

    /** @brief Registers the VK_EXT_debug_utils messenger for validation output (no-op when validation is off). */
    void setupDebugMessenger();

    /** @brief Creates the Vulkan surface backed by the SDL3 window. */
    void createSurface();

    /**
     * @brief Selects the best available GPU using a simple scoring heuristic.
     *
     * Discrete GPUs receive +1000 points; the device's maxImageDimension2D is added
     * on top.  Devices without geometry shader support are excluded outright.
     */
    void pickPhysicalDevice();

    /**
     * @brief Resolves graphics, present, transfer, and compute queue family indices.
     *
     * Prefers dedicated families for transfer and compute.  Falls back to a shared
     * transfer+compute family, and ultimately to the graphics family, when dedicated
     * ones are not available.
     *
     * @param queueFamilyProperties2 Queue family properties returned by the physical device.
     * @throws std::runtime_error if no graphics or present queue family can be found.
     */
    void findQueueFamilies(const std::vector<vk::QueueFamilyProperties2>& queueFamilyProperties2);

    /**
     * @brief Creates the logical device and retrieves all queue handles.
     *
     * Builds the full feature-chain (Vulkan 1.1–1.4 core plus every required EXT/KHR
     * device feature), populates queueCreateInfos for all distinct queue families, and
     * assigns debug names to queues and device objects.
     */
    void createLogicalDevice();

public:
    /**
     * @brief Constructs a Device object.
     * @param window                SDL3 window used for surface creation; must outlive this object.
     * @param enableValidationLayers Enable Khronos validation layers and debug messenger output.
     */
    Device(SDL_Window *window, bool enableValidationLayers = false);
    ~Device() = default;

    /**
     * @brief Runs the full Vulkan device initialisation sequence.
     *
     * Must be called once before any other method.  Calls createInstance(),
     * setupDebugMessenger(), createSurface(), pickPhysicalDevice(), and
     * createLogicalDevice() in order.
     */
    void init();

    // -------------------------------------------------------------------------
    // Accessors
    // -------------------------------------------------------------------------

    /// Returns the SDL3 window pointer passed at construction.
    SDL_Window* getWindow() const { return window; }
    /// Returns true if the Khronos validation layers were requested at construction.
    bool isValidationLayersEnabled() const { return enableValidationLayers; }

    const vk::raii::Context&              getContext()       const { return context; }
    const vk::raii::Instance&             getInstance()      const { return instance; }
    const vk::raii::DebugUtilsMessengerEXT& getDebugMessenger() const { return debugMessenger; }
    const vk::raii::PhysicalDevice&       getPhysicalDevice() const { return physicalDevice; }
    const vk::raii::SurfaceKHR&           getSurface()       const { return surface; }
    const vk::raii::Device&               getDevice()        const { return vkdevice; }
    const vk::PhysicalDeviceFeatures&     getDeviceFeatures() const { return deviceFeatures; }

    const vk::raii::Queue& getGraphicsQueue() const { return graphicsQueue; }
    const vk::raii::Queue& getPresentQueue()  const { return presentQueue; }
    const vk::raii::Queue& getTransferQueue() const { return transferQueue; }
    const vk::raii::Queue& getComputeQueue()  const { return computeQueue; }

    /// Returns the deduplicated list of queue family indices used by this device.
    const std::vector<uint32_t>& getQueueFamilyIndices() const { return queueFamilyIndices; }

    /**
     * @brief Queries and returns the maximum MSAA sample count supported by the GPU.
     *
     * Takes the intersection of framebufferColorSampleCounts and
     * framebufferDepthSampleCounts to ensure both attachment types can use the
     * returned value.
     */
    vk::SampleCountFlagBits getMaxUsableSampleCount();
    /// Returns the cached MSAA sample count determined during device creation.
    vk::SampleCountFlagBits getMsaaSamples() const { return msaaSamples; }

    uint32_t getGraphicsQueueFamilyIndex() const { return graphicsIndex; }
    uint32_t getTransferIndex() const { return transferIndex; }
    uint32_t getComputeIndex()  const { return computeIndex; }
    uint32_t getGraphicsIndex() const { return graphicsIndex; }
    uint32_t getPresentIndex()  const { return presentIndex; }

    /// Returns the list of required device extension names enabled on the logical device.
    const std::vector<const char*>& getRequiredDeviceExtensions() const { return requiredDeviceExtension; }
};