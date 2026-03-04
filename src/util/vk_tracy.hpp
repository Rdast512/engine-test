#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_raii.hpp>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>


class VkTracyContext
{
public:
    VkTracyContext() = default;
	~VkTracyContext();

	VkTracyContext(const VkTracyContext&) = delete;
	VkTracyContext& operator=(const VkTracyContext&) = delete;

	void init(const vk::raii::Instance& instance,
			  const vk::raii::PhysicalDevice& physicalDevice,
			  const vk::raii::Device& device,
			  const vk::raii::Queue& queue,
			  const vk::raii::CommandBuffer& setupCommandBuffer,
			  const char* contextName = "Graphics Queue");

	void collect(const vk::raii::CommandBuffer& commandBuffer) const;
	void shutdown();

	[[nodiscard]] bool active() const noexcept;

#ifdef TRACY_ENABLE
	[[nodiscard]] TracyVkCtx handle() const noexcept { return context_; }
#endif

private:
#ifdef TRACY_ENABLE
	TracyVkCtx context_ = nullptr;
#endif
};
