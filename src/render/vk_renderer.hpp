#pragma once

#include "../core/vk_descriptors.hpp"
#include "../core/vk_device.hpp"
#include "../core/vk_resource_manager.hpp"
#include "../core/vk_swapchain.hpp"
#include "vk_pipeline.hpp"

class VkTracyContext;

class Renderer
{
public:
	Renderer(Device& device,
			 SwapChain& swapChain,
			 ResourceManager& resourceManager,
			 DescriptorManager& descriptorManager,
			 Pipeline& pipeline,
			 VkTracyContext* tracyContext = nullptr);

	void setTracyContext(VkTracyContext* tracyContextIn);
	void rebuildSwapchainResources();
	void drawFrame();
	void waitIdle() const;

private:
	void recordCommandBuffer(uint32_t imageIndex);

	Device& device;
	SwapChain& swapChain;
	ResourceManager& resourceManager;
	DescriptorManager& descriptorManager;
	Pipeline& pipeline;
	VkTracyContext* tracyContext = nullptr;

	uint32_t currentFrame = 0;
	uint32_t semaphoreIndex = 0;
};
