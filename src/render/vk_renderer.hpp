#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class Renderer {
	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

public:
	Renderer();
	~Renderer();

	void beginFrame();
	void endFrame();
	void drawFrame();
	void waitIdle();
};
