#pragma once
#include "../core/Utils.h"

class Renderer {
private:

    // VulkanContext& vulkanContext;
    // SwapChain& swapChain;
    // Pipeline& pipeline;
    // DescriptorManager& descriptorManager;
    
    // Command buffer management
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    
public:
    void beginFrame();
    void endFrame();
    void drawFrame();
    void waitIdle();

    Renderer();
    ~Renderer();
};