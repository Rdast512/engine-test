#pragma once
#include "../graphics/SwapChain.h"
#include "../graphics/VulkanContext.h"
#include "../graphics/DescriptorManager.h"
#include "../graphics/Pipeline.h"
#include "../resources/TextureManager.h"

// Todo pointer to smart pointers



class Application {
public:
    Application();
    ~Application();

    void run();

private:
    SDL_Window *window = nullptr;
    std::unique_ptr<VulkanContext> context;
    std::unique_ptr<SwapChain> swapChain;
    std::unique_ptr<ResourceManager> resourceManager;
    std::unique_ptr<AssetsLoader> assetsLoader;
    std::unique_ptr<DescriptorManager> descriptorManager;
    std::unique_ptr<TextureManager> textureManager;
    std::unique_ptr<Pipeline> pipeline;
    uint32_t currentFrame = 0;
    uint32_t semaphoreIndex = 0;

    bool framebufferResized = false;
    std::chrono::high_resolution_clock::time_point lastTime;
    std::chrono::high_resolution_clock::time_point fpsTime;
    int frameCount = 0;
    float fps = 0.0f;

    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();
    void drawFrame();
    void recordCommandBuffer(uint32_t imageIndex);

};