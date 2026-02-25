#pragma once
#include "vk_device.hpp"
#include "../core/types.hpp"
#include "../Constants.h"
#include <vulkan/vulkan_raii.hpp>
#include <optional>
#include "vk_allocator.hpp"

// Manages GPU resources (buffers, images, command pools) using Device + Assets data.
class ResourceManager {
public:
	ResourceManager(const Device &deviceWrapper,
			   const VkAllocator &allocator,
			   const std::vector<Vertex> &verticesIn,
			   const std::vector<uint32_t> &indicesIn);
	~ResourceManager();

	void init();
	void createSyncObjects();
	void createCommandPool();
	void createCommandBuffers();
	void createDepthResources();
	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffers();
	void createColorResources();
	void setSwapChainImageCount(uint32_t count) { swapChainImageCount = count; createSyncObjects(); }
	void updateUniformBuffer(uint32_t currentImage);

	void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
					  vk::raii::Buffer &buffer, VmaAllocation &bufferMemory);
	[[nodiscard]] vk::raii::ShaderModule createShaderModule(const std::vector<char> &code) const;

	const std::vector<vk::raii::Buffer>& getUniformBuffers() const { return uniformBuffers; }
	vk::Extent2D getSwapChainExtent() const { return swapChainExtent; }

	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, vk::SampleCountFlagBits samples,
					 vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage,
					 vk::MemoryPropertyFlags properties, vk::raii::Image &image, VmaAllocation &imageMemory);
	vk::raii::ImageView createImageView(vk::raii::Image &image, vk::Format format, vk::ImageAspectFlags aspectFlags,
										uint32_t mipLevels);
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
		const vk::ImageSubresourceRange &subresourceRange = {
			vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1
		},
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

	const Device &deviceWrapper;
	const VkAllocator &allocator;
	const vk::raii::PhysicalDevice &physicalDevice;
	const vk::raii::Device &device;
	const std::vector<uint32_t> &queueFamilyIndices;
	const vk::raii::Queue &graphicsQueue;
	const vk::raii::Queue &transferQueue;
	uint32_t graphicsIndex;
	uint32_t transferIndex;
	vk::SampleCountFlagBits msaaSamples;
	vk::Extent2D swapChainExtent{};
	const std::vector<Vertex> &vertices;
	const std::vector<uint32_t> &indices;
	uint32_t swapChainImageCount = 0;
	vk::Format swapChainImageFormat = vk::Format::eUndefined;

	std::vector<vk::raii::Semaphore> presentCompleteSemaphore;
	std::vector<vk::raii::Semaphore> renderFinishedSemaphore;
	std::vector<vk::raii::Fence> inFlightFences;
	vk::raii::Image depthImage = nullptr;
	VmaAllocation depthImageMemory = nullptr;
	vk::raii::ImageView depthImageView = nullptr;
	vk::raii::CommandPool commandPool = nullptr;
	vk::raii::CommandPool transferCommandPool = nullptr;
	std::vector<vk::raii::CommandBuffer> commandBuffers;
	std::vector<vk::raii::CommandBuffer> transferCommandBuffer;
	vk::raii::Buffer vertexBuffer = nullptr;
	VmaAllocation vertexBufferMemory = nullptr;
	vk::raii::Buffer stagingBuffer = nullptr;
	VmaAllocation stagingBufferMemory = nullptr;
	vk::raii::Buffer indexBuffer = nullptr;
	VmaAllocation indexBufferMemory = nullptr;
	std::vector<vk::raii::Buffer> uniformBuffers;
	std::vector<VmaAllocation> uniformBuffersMemory;
	std::vector<void *> uniformBuffersMapped;
	vk::raii::Image colorImage = nullptr;
	VmaAllocation colorImageMemory = nullptr;
	vk::raii::ImageView colorImageView = nullptr;
};

