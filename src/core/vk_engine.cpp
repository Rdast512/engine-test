#include "vk_engine.hpp"
#include "../Constants.h"
#include "../static_headers/logger.hpp"
#include "../util/vk_tracy.hpp"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"


Engine::~Engine() { cleanup(); }

void Engine::initialize()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        throw std::runtime_error("Failed to initialize SDL: " + std::string(SDL_GetError()));
    }

    window = SDL_CreateWindow("Vulkan", static_cast<int>(WIDTH), static_cast<int>(HEIGHT),
                              SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

    if (!window)
    {
        throw std::runtime_error("Failed to create window");
    }


    // Setup Platform/Renderer backends
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls


    device = std::make_unique<Device>(window, false);
    device->init();

    allocator = std::make_unique<VkAllocator>(*device);

    swapChain = std::make_unique<SwapChain>(window, *device);
    swapChain->init();

    modelStorage = std::make_unique<ModelStorage>();
    assetsLoader = std::make_unique<AssetsLoader>(*modelStorage);

    resourceManager =
        std::make_unique<ResourceManager>(*device, *allocator, assetsLoader->getVertices(), assetsLoader->getIndices());
    resourceManager->init();

    tracyContext = std::make_unique<VkTracyContext>();
    {
        const vk::CommandBufferAllocateInfo tracySetupCommandBufferAllocateInfo{
            .commandPool = *resourceManager->commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1,
        };
        vk::raii::CommandBuffers tracySetupCommandBuffers(device->getDevice(), tracySetupCommandBufferAllocateInfo);
        tracyContext->init(device->getInstance(), device->getPhysicalDevice(), device->getDevice(),
                           device->getGraphicsQueue(), tracySetupCommandBuffers.front(), "Graphics Queue");
    }

    textureManager = std::make_unique<TextureManager>(*device, *resourceManager, *allocator);
    textureManager->init();

    descriptorManager =
        std::make_unique<DescriptorManager>(device->getDevice(), resourceManager->getUniformBuffers(),
                                            textureManager->getTextureSampler(), textureManager->getTextureImageView(), textureManager->gettextureImageViewCreateInfo());
    descriptorManager->init();

    createImGuiDescriptorPool();

    pipeline = std::make_unique<Pipeline>(resourceManager.get(), device->getDevice(), swapChain->swapChainExtent,
                                          swapChain->swapChainImageFormat, descriptorManager->getDescriptorSetLayout());
    pipeline->init();

    renderer = std::make_unique<Renderer>(*device,
                                          *swapChain,
                                          *resourceManager,
                                          *descriptorManager,
                                          *pipeline,
                                          tracyContext.get());
    renderer->rebuildSwapchainResources();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForVulkan(window);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.ApiVersion = VK_API_VERSION_1_4;
    init_info.Instance = *device->getInstance();
    init_info.PhysicalDevice = *device->getPhysicalDevice();
    init_info.Device = *device->getDevice();
    init_info.QueueFamily = device->getGraphicsQueueFamilyIndex();
    init_info.Queue = *device->getGraphicsQueue();
    init_info.DescriptorPool = *imguiDescriptorPool;
    const auto imageCount = static_cast<uint32_t>(swapChain->swapChainImages.size());
    init_info.MinImageCount = std::max<uint32_t>(imageCount, 2);
    init_info.ImageCount = std::max<uint32_t>(imageCount, init_info.MinImageCount);
    init_info.UseDynamicRendering = true;
    imguiColorFormat = static_cast<VkFormat>(swapChain->swapChainImageFormat);
    imguiDepthFormat = static_cast<VkFormat>(resourceManager->findDepthFormat());
    imguiPipelineRenderingInfo = VkPipelineRenderingCreateInfoKHR{};
    imguiPipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
    imguiPipelineRenderingInfo.colorAttachmentCount = 1;
    imguiPipelineRenderingInfo.pColorAttachmentFormats = &imguiColorFormat;
    imguiPipelineRenderingInfo.depthAttachmentFormat = imguiDepthFormat;
    imguiPipelineRenderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
    init_info.PipelineInfoMain.RenderPass = VK_NULL_HANDLE;
    init_info.PipelineInfoMain.Subpass = 0;
    init_info.PipelineInfoMain.MSAASamples = static_cast<VkSampleCountFlagBits>(device->getMsaaSamples());
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo = imguiPipelineRenderingInfo;
    if (!ImGui_ImplVulkan_Init(&init_info))
    {
        throw std::runtime_error("ImGui_ImplVulkan_Init failed");
    }

    initialized = true;
}

void Engine::createImGuiDescriptorPool()
{
    // Backends require a pool with ample combined image samplers; follow ImGui recommendations.
    auto& vkDevice = device->getDevice();
    const uint32_t imguiSamplerMin = std::max<uint32_t>(IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE, 1000);
    std::array poolSizes{vk::DescriptorPoolSize{.type = vk::DescriptorType::eSampler, .descriptorCount = 1000},
                         vk::DescriptorPoolSize{.type = vk::DescriptorType::eCombinedImageSampler, .descriptorCount = imguiSamplerMin},
                         vk::DescriptorPoolSize{.type = vk::DescriptorType::eSampledImage, .descriptorCount = 1000},
                         vk::DescriptorPoolSize{.type = vk::DescriptorType::eStorageImage, .descriptorCount = 1000},
                         vk::DescriptorPoolSize{.type = vk::DescriptorType::eUniformTexelBuffer, .descriptorCount = 1000},
                         vk::DescriptorPoolSize{.type = vk::DescriptorType::eStorageTexelBuffer, .descriptorCount = 1000},
                         vk::DescriptorPoolSize{.type = vk::DescriptorType::eUniformBuffer, .descriptorCount = 1000},
                         vk::DescriptorPoolSize{.type = vk::DescriptorType::eStorageBuffer, .descriptorCount = 1000},
                         vk::DescriptorPoolSize{.type = vk::DescriptorType::eUniformBufferDynamic, .descriptorCount = 1000},
                         vk::DescriptorPoolSize{.type = vk::DescriptorType::eStorageBufferDynamic, .descriptorCount = 1000},
                         vk::DescriptorPoolSize{.type = vk::DescriptorType::eInputAttachment, .descriptorCount = 1000}};


    const uint32_t maxSets = 1000 * static_cast<uint32_t>(poolSizes.size());
    vk::DescriptorPoolCreateInfo poolInfo{.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                                          .maxSets = maxSets,
                                          .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
                                          .pPoolSizes = poolSizes.data()};
    imguiDescriptorPool = vk::raii::DescriptorPool(vkDevice, poolInfo);
}

void Engine::run()
{
    if (!initialized)
    {
        initialize();
    }


    bool quit = false;
    bool minimized = false;
    lastTime = std::chrono::high_resolution_clock::now();
    fpsTime = lastTime;
    const double targetMs = 1000.0 / 60.0; // 60 FPS
    auto& deviceRef = device->getDevice();

    while (!quit)
    {
        auto currentTime = std::chrono::high_resolution_clock::now();
        frameCount++;

        // Update FPS every second
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - fpsTime);
        if (duration.count() >= 1000)
        {
            fps = frameCount * 1000.0f / duration.count();
            frameCount = 0;
            fpsTime = currentTime;

            // Update window title with FPS
            std::string title = "Vulkan Triangle - FPS: " + std::to_string(static_cast<int>(fps));
            SDL_SetWindowTitle(window, title.c_str());
        }

        {
            ZoneScopedN("EventPoll");
            SDL_Event e{};
            while (SDL_PollEvent(&e) != 0)
            {
                ImGui_ImplSDL3_ProcessEvent(&e);

                if (e.type == SDL_EVENT_QUIT)
                {
                    quit = true;
                }
                else if (e.type == SDL_EVENT_WINDOW_RESIZED)
                {
                    if (swapChain && renderer)
                    {
                        swapChain->recreateSwapChain();
                        renderer->rebuildSwapchainResources();
                    }
                }
                else if (e.type == SDL_EVENT_WINDOW_MINIMIZED)
                {
                    minimized = true;
                }
                else if (e.type == SDL_EVENT_WINDOW_RESTORED)
                {
                    minimized = false;
                }
            }
        }

        {
            ZoneScopedN("ImGuiNewFrame");
            // Start ImGui frame
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();
            ImGui::ShowDemoWindow();
            ImGui::Render();
        }

        if (!minimized && !quit)
        {
            ZoneScopedN("DrawFrame");
            renderer->drawFrame();
        }
        else
        {
            SDL_Delay(100);
        }

        auto frameEndTime = std::chrono::high_resolution_clock::now();
        auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEndTime - currentTime).count();
        if (frameDuration < targetMs)
        {
            SDL_Delay(static_cast<Uint32>(targetMs - frameDuration));
        }
        FrameMark;
    }
    deviceRef.waitIdle();
}

void Engine::render()
{
    if (renderer)
    {
        renderer->drawFrame();
    }
}

void Engine::shutdown() { cleanup(); }

void Engine::cleanup()
{
    if (!initialized)
        return;
    auto& deviceRef = device->getDevice();
    deviceRef.waitIdle();

    if (tracyContext)
    {
        tracyContext->shutdown();
        tracyContext.reset();
    }

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    imguiDescriptorPool.reset();

    // Explicitly clear command buffers before destroying other resources
    if (resourceManager)
    {
        resourceManager->commandBuffers.clear();
        resourceManager->transferCommandBuffer.clear();
    }

    renderer.reset();
    pipeline.reset();
    descriptorManager.reset();
    textureManager.reset();
    resourceManager.reset();
    log_info("Resources cleaned up");
    swapChain.reset();
    log_info("Swap chain cleaned up");
    allocator.reset();
    log_info("Allocator cleaned up");
    device.reset();
    log_info("Device cleaned up");
    SDL_DestroyWindow(window);
    log_info("Window destroyed");
    window = nullptr;
    SDL_Quit();
    initialized = false;
    log_info("Engine cleaned up");
}
