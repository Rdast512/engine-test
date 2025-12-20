#include "vk_engine.hpp"

int WIDTH = 800;
int HEIGHT = 600;

void Engine::initialize() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error("Failed to initialize SDL: " + std::string(SDL_GetError()));
    }

    window = SDL_CreateWindow("Vulkan", WIDTH, HEIGHT,
                              SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        throw std::runtime_error("Failed to create window");
    }
    // Additional initialization code for Device, SwapChain, ResourceManager, etc. goes here
    device = std::make_unique<Device>(window, true);
    
}

void Engine::run() {
    bool quit = false;
    bool minimized = false;
    lastTime = std::chrono::high_resolution_clock::now();
    fpsTime = lastTime;

    auto &deviceRef = device->getDevice();

    while (!quit) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        frameCount++;

        // Update FPS every second
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - fpsTime);
        if (duration.count() >= 1000) {
            fps = frameCount * 1000.0f / duration.count();
            frameCount = 0;
            fpsTime = currentTime;

            // Update window title with FPS
            std::string title = "Vulkan Triangle - FPS: " +
                                std::to_string(static_cast<int>(fps));
            SDL_SetWindowTitle(window, title.c_str());
        }

        SDL_Event e;
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_EVENT_QUIT) {
                quit = true;
            } else if (e.type == SDL_EVENT_WINDOW_MINIMIZED) {
                minimized = true;
            } else if (e.type == SDL_EVENT_WINDOW_RESTORED) {
                minimized = false;
            }
        }

        if (!minimized && !quit) {
            // drawFrame();
        } else {
            SDL_Delay(100);
        }
    }
    deviceRef.waitIdle();
}

void Engine::cleanup() {
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
}
