#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <memory>

class VulkanContext; // Forward declaration

class Texture {
public:
    // Texture type enumeration
    enum class Type {
        e2D,
        eCubemap,
        e3D,
        e2DArray
    };

    // Texture usage patterns
    enum class Usage {
        eSampled,           // Used as shader input (most common)
        eColorAttachment,   // Render target color buffer
        eDepthStencil,      // Depth/stencil buffer
        eStorage,           // Read/write in compute shaders
        eTransferSrc,       // Source for copy operations
        eTransferDst        // Destination for copy operations
    };

    // Texture properties (immutable after creation)
    struct Properties {
        VkFormat format = VK_FORMAT_UNDEFINED;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t depth = 1; // For 3D textures
        uint32_t mipLevels = 1;
        uint32_t arrayLayers = 1;
        Type type = Type::e2D;
        Usage usage = Usage::eSampled;
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
        bool generateMipmaps = false;
    };

    // Create from existing Vulkan resources (import case)
    Texture(VulkanContext& context,
            VkImage image,
            VkImageView imageView,
            VkDeviceMemory memory,
            const Properties& properties,
            bool ownsResources = false);
    
    // Create a new texture (empty)
    Texture(VulkanContext& context, const Properties& properties);
    
    // Create from file (most common usage)
    Texture(VulkanContext& context, 
            const std::string& filePath,
            const Properties& properties = Properties{.generateMipmaps = true});
    
    // Create from raw pixel data
    Texture(VulkanContext& context,
            const void* pixelData,
            size_t pixelDataSize,
            const Properties& properties);
    
    ~Texture();

    // Non-copyable, movable
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    // Getters
    VkImage getImage() const { return m_image; }
    VkImageView getImageView() const { return m_imageView; }
    const Properties& getProperties() const { return m_properties; }
    VkImageLayout getCurrentLayout() const { return m_currentLayout; }
    
    // Resource management
    void cleanup();
    
    // Texture operations
    void transitionLayout(VkImageLayout newLayout, 
                          VkCommandBuffer cmdBuffer = VK_NULL_HANDLE);
    void generateMipmaps(VkCommandBuffer cmdBuffer = VK_NULL_HANDLE);

private:
    // Internal helpers
    void createImage();
    void createImageView();
    void createFromFilePath(const std::string& filePath);
    void createFromPixelData(const void* pixelData, size_t pixelDataSize);
    void copyToImage(const void* pixelData, size_t pixelDataSize);
    
    VulkanContext& m_context;
    
    // Vulkan resources
    VkImage m_image = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    
    // Texture properties
    Properties m_properties;
    
    // Runtime state
    VkImageLayout m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    bool m_ownsResources = true; // If false, we don't clean up Vulkan resources
};
