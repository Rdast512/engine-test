#include "vk_tracy.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>


VkTracyContext::~VkTracyContext()
{
	shutdown();
}

void VkTracyContext::init(const vk::raii::Instance& instance,
						 const vk::raii::PhysicalDevice& physicalDevice,
						 const vk::raii::Device& device,
						 const vk::raii::Queue& queue,
						 const vk::raii::CommandBuffer& setupCommandBuffer,
						 const char* contextName)
{
#ifdef TRACY_ENABLE
	if (context_)
	{
		return;
	}

#if defined(TRACY_VK_USE_SYMBOL_TABLE)
	context_ = TracyVkContext(static_cast<VkInstance>(*instance), static_cast<VkPhysicalDevice>(*physicalDevice),
							  static_cast<VkDevice>(*device), static_cast<VkQueue>(*queue),
							  static_cast<VkCommandBuffer>(*setupCommandBuffer), nullptr, nullptr);
#else
	context_ = TracyVkContext(static_cast<VkPhysicalDevice>(*physicalDevice), static_cast<VkDevice>(*device),
							  static_cast<VkQueue>(*queue), static_cast<VkCommandBuffer>(*setupCommandBuffer));
#endif

	if (context_ && contextName)
	{
		const auto len = static_cast<uint16_t>(std::min<size_t>(std::strlen(contextName), UINT16_MAX));
		TracyVkContextName(context_, contextName, len);
	}
#else
	(void)instance;
	(void)physicalDevice;
	(void)device;
	(void)queue;
	(void)setupCommandBuffer;
	(void)contextName;
#endif
}

void VkTracyContext::collect(const vk::raii::CommandBuffer& commandBuffer) const
{
#ifdef TRACY_ENABLE
	if (!context_)
	{
		return;
	}

	TracyVkCollect(context_, static_cast<VkCommandBuffer>(*commandBuffer));
#else
	(void)commandBuffer;
#endif
}

void VkTracyContext::shutdown()
{
#ifdef TRACY_ENABLE
	if (!context_)
	{
		return;
	}

	TracyVkDestroy(context_);
	context_ = nullptr;
#endif
}

bool VkTracyContext::active() const noexcept
{
#ifdef TRACY_ENABLE
	return context_ != nullptr;
#else
	return false;
#endif
}
