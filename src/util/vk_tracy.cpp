#include "vk_tracy.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>


void tracyResourceAlloc(const void* ptr, size_t size, const char* poolName)
{
#ifdef TRACY_ENABLE
	if (ptr == nullptr || size == 0) {
		return;
	}
	TracyAllocN(ptr, size, poolName != nullptr ? poolName : "GPU/Unknown");
#else
	(void)ptr;
	(void)size;
	(void)poolName;
#endif
}

void tracyResourceFree(const void* ptr, const char* poolName)
{
#ifdef TRACY_ENABLE
	if (ptr == nullptr) {
		return;
	}
	TracyFreeN(ptr, poolName != nullptr ? poolName : "GPU/Unknown");
#else
	(void)ptr;
	(void)poolName;
#endif
}

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
	if (context)
	{
		return;
	}

#if defined(TRACY_VK_USE_SYMBOL_TABLE)
	context = TracyVkContext(static_cast<VkInstance>(*instance), static_cast<VkPhysicalDevice>(*physicalDevice),
							  static_cast<VkDevice>(*device), static_cast<VkQueue>(*queue),
							  static_cast<VkCommandBuffer>(*setupCommandBuffer), nullptr, nullptr);
#else
	context = TracyVkContext(static_cast<VkPhysicalDevice>(*physicalDevice), static_cast<VkDevice>(*device),
							  static_cast<VkQueue>(*queue), static_cast<VkCommandBuffer>(*setupCommandBuffer));
#endif

	if (context && contextName)
	{
		const auto len = static_cast<uint16_t>(std::min<size_t>(std::strlen(contextName), UINT16_MAX));
		TracyVkContextName(context, contextName, len);
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
	if (!context)
	{
		return;
	}

	TracyVkCollect(context, static_cast<VkCommandBuffer>(*commandBuffer));
#else
	(void)commandBuffer;
#endif
}

void VkTracyContext::shutdown()
{
#ifdef TRACY_ENABLE
	if (!context)
	{
		return;
	}

	TracyVkDestroy(context);
	context = nullptr;
#endif
}

bool VkTracyContext::active() const noexcept
{
#ifdef TRACY_ENABLE
	return context != nullptr;
#else
	return false;
#endif
}
