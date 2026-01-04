#pragma once
#include <SDL3/SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include <memory>
#include <chrono>
#include "vk_device.hpp"
#include "vk_swapchain.hpp"
#include "vk_resource_manager.hpp"
#include "vk_descriptors.hpp"
#include "../assets/texture_manager.hpp"
#include "../assets/assets_loader.hpp"
#include "../assets/model_storage.hpp"
#include "../render/vk_pipeline.hpp"
#include "vk_allocator.hpp"


class Engine{
    SDL_Window *window = nullptr;
    std::unique_ptr<Device> device;
    std::unique_ptr<VkAllocator> allocator;
    std::unique_ptr<SwapChain> swapChain;
    std::unique_ptr<ModelStorage> modelStorage;
    std::unique_ptr<AssetsLoader> assetsLoader;
    std::unique_ptr<ResourceManager> resourceManager;
    std::unique_ptr<TextureManager> textureManager;
    std::unique_ptr<DescriptorManager> descriptorManager;
    std::unique_ptr<Pipeline> pipeline;
    uint32_t currentFrame = 0;
    uint32_t semaphoreIndex = 0;
    bool framebufferResized = false;
    bool initialized = false;
    std::chrono::high_resolution_clock::time_point lastTime;
    std::chrono::high_resolution_clock::time_point fpsTime;
    int frameCount = 0;
    float fps = 0.0f;

    vk::raii::DescriptorPool imguiDescriptorPool = nullptr;

    void drawFrame();
    void recordCommandBuffer(uint32_t imageIndex);
    void createImGuiDescriptorPool();

public:
    void initialize();
    void run();
    void render();
    void cleanup();
    void shutdown();
};