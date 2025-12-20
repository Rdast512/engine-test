#pragma once
#include <SDL3/SDL.h>
#include "vk_device.hpp"
#include "vk_swapchain.hpp"


class Engine{
    SDL_Window *window = nullptr;
    std::unique_ptr<Device> device;
    std::unique_ptr<SwapChain> swapChain;
    // std::unique_ptr<ResourceManager> resourceManager;
    // std::unique_ptr<AssetsLoader> assetsLoader;
    // std::unique_ptr<DescriptorManager> descriptorManager;
    // std::unique_ptr<TextureManager> textureManager;
    // std::unique_ptr<Pipeline> pipeline;
    // uint32_t currentFrame = 0;
    // uint32_t semaphoreIndex = 0;

    bool framebufferResized = false;
    std::chrono::high_resolution_clock::time_point lastTime;
    std::chrono::high_resolution_clock::time_point fpsTime;
    int frameCount = 0;
    float fps = 0.0f;

public:
    void initialize();
    void run();
    void render();
    void cleanup();
    void shutdown();
};