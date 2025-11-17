#pragma once
#include "../core/Utils.h"
#include "../graphics/VulkanContext.h"
#include "AssetsLoader.h"
#include <vulkan/vulkan_enums.hpp>

// Command pools and buffers (rendering operations)
// Synchronization objects (semaphores, fences)
// Buffers and memory (vertex, index, uniform)
// Images and image views

const vk::ImageSubresourceRange DEFAULT_COLOR_SUBRESOURCE_RANGE = {
    vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
};

class ResourceManager {
public:
    ResourceManager(
        const VulkanContext* context,
        const AssetsLoader* assetsLoader
        );
    ~ResourceManager() = default;
    void init();
    void createSyncObjects();
    void createCommandPool();
    void createCommandBuffers();
    void createDepthResources();
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();
    void createColorResources();
    void setSwapChainImageCount(uint32_t count){ swapChainImageCount = count; createSyncObjects(); };
    void updateUniformBuffer(uint32_t currentImage);
    void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
                      vk::raii::Buffer &buffer, vk::raii::DeviceMemory &bufferMemory);
    [[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char> &code) const;
    const std::vector<vk::raii::Buffer>& getUniformBuffers() const { return uniformBuffers; }    // std::vector<void *> getUniformBuffersMapped() const { return uniformBuffersMapped; }
    vk::Extent2D getSwapChainExtent() const { return swapChainExtent; }
    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits samples, vk::Format format, vk::ImageTiling tiling,
                        vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Image &image,
                        vk::raii::DeviceMemory &imageMemory);
    vk::raii::ImageView createImageView(vk::raii::Image &image, vk::Format format, vk::ImageAspectFlags aspectFlags
                                        , uint32_t mipLevels);
    void copyBuffer(vk::raii::Buffer &srcBuffer, vk::raii::Buffer &dstBuffer, vk::DeviceSize size);
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

    vk::Format findSupportedFormat(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling,
                                   vk::FormatFeatureFlags features);
    vk::Format findDepthFormat();
    void copyBufferToImage(const vk::raii::Buffer &buffer, vk::raii::Image &image, uint32_t width, uint32_t height);
    static bool hasStencilComponent(vk::Format format);
    void generateMipmaps(vk::raii::Image &image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight,
                         uint32_t mipLevels);
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
    );
    static void endCommandBuffer(vk::raii::CommandBuffer &commandBuffer, const vk::raii::Queue &queue);
    void updateSwapChainExtent(vk::Extent2D newExtent);
    void updateSwapChainImageFormat(vk::Format newFormat) { swapChainImageFormat = newFormat; }

    const VulkanContext *context;
    const vk::raii::PhysicalDevice& physicalDevice;
    const vk::raii::Device& device;
    const std::vector<uint32_t>& queueFamilyIndices;
    const vk::raii::Queue& graphicsQueue;
    const vk::raii::Queue& transferQueue;
    const uint32_t& graphicsIndex;
    const uint32_t& transferIndex;
    vk::Extent2D swapChainExtent;
    const std::vector<Vertex>& vertices;
    const std::vector<uint32_t>& indices;
    uint32_t swapChainImageCount;
    vk::Format swapChainImageFormat;
    

    std::vector<vk::raii::Semaphore> presentCompleteSemaphore;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphore;
    std::vector<vk::raii::Fence> inFlightFences;
    vk::raii::Image depthImage = nullptr;
    vk::raii::DeviceMemory depthImageMemory = nullptr;
    vk::raii::ImageView depthImageView = nullptr;
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
    vk::raii::Image colorImage = nullptr;
    vk::raii::DeviceMemory colorImageMemory = nullptr;
    vk::raii::ImageView colorImageView = nullptr;
    
private:


};
