#include "vk_engine.hpp"
#include "../Constants.h"
#include "../static_headers/logger.hpp"
#include "../util/debug.hpp"
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

    if (window == nullptr)
    {
        throw std::runtime_error("Failed to create window");
    }

    SDL_SetWindowRelativeMouseMode(window, true);

    if (enableImGui)
    {
        // Setup Platform/Renderer backends
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
    }
    else
    {
        log_info("ImGui disabled by Engine::enableImGui toggle", "Engine");
    }

    objects = std::vector<Object>();
    device = std::make_unique<Device>(window, false);
    device->init();

    allocator = std::make_unique<VkAllocator>(*device);

    descriptorManager = std::make_unique<DescriptorManager>(device->vkdevice,
                                                            allocator->allocator,
                                                            device->queueFamilyIndices,
                                                            device->capabilities);
    descriptorManager->init();

    swapChain = std::make_unique<SwapChain>(window, *device);
    swapChain->init();

    camera = std::make_unique<Camera>(*swapChain);
    textureManager = std::make_unique<TextureManager>(*device, *allocator, *descriptorManager);
    textureManager->init();

    assetsLoader = std::make_unique<AssetsLoader>(objects, *textureManager);

    assetsLoader->loadModel(MODEL_PATH.string(),{0.0f, 0.0f, 0.0f});
    resourceManager =
        std::make_unique<ResourceManager>(*device, *allocator, assetsLoader->vertices, assetsLoader->indices, objects);
    resourceManager->init();
    resourceManager->createCameraBuffers(*camera);

    tracyContext = std::make_unique<VkTracyContext>();
    {
        const vk::CommandBufferAllocateInfo tracySetupCommandBufferAllocateInfo{
            .commandPool = *resourceManager->commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1,
        };
        vk::raii::CommandBuffers tracySetupCommandBuffers(device->vkdevice, tracySetupCommandBufferAllocateInfo);
        tracyContext->init(device->instance, device->physicalDevice, device->vkdevice,
                           device->graphicsQueue, tracySetupCommandBuffers.front(), "Graphics Queue");
    }


    if (enableImGui)
    {
        createImGuiDescriptorPool();
    }


    pipeline = std::make_unique<Pipeline>(*resourceManager,
                                          *descriptorManager,
                                          device->vkdevice,
                                          swapChain->swapChainExtent,
                                          swapChain->swapChainImageFormat);
    pipeline->init();

    renderer = std::make_unique<Renderer>(*device,
                                          *swapChain,
                                          *resourceManager,
                                          *descriptorManager,
                                          *pipeline,
                                          *camera,
                                          tracyContext.get(),
                                          enableImGui);
    renderer->rebuildSwapchainResources();


    if (enableImGui)
    {
        // Setup Platform/Renderer backends
        ImGui_ImplSDL3_InitForVulkan(window);
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.ApiVersion = VK_API_VERSION_1_4;
        init_info.Instance = *device->instance;
        init_info.PhysicalDevice = *device->physicalDevice;
        init_info.Device = *device->vkdevice;
        init_info.QueueFamily = device->graphicsIndex;
        init_info.Queue = *device->graphicsQueue;
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
        init_info.PipelineInfoMain.MSAASamples = static_cast<VkSampleCountFlagBits>(device->msaaSamples);
        init_info.PipelineInfoMain.PipelineRenderingCreateInfo = imguiPipelineRenderingInfo;
        if (!ImGui_ImplVulkan_Init(&init_info))
        {
            throw std::runtime_error("ImGui_ImplVulkan_Init failed");
        }
    }

    initialized = true;
}

void Engine::createImGuiDescriptorPool()
{
    // Backends require separate sampled image + sampler descriptors (not combined); follow ImGui recommendations.
    auto& vkDevice = device->vkdevice;
    const uint32_t imguiSampledImageMin = std::max<uint32_t>(IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE, 1000);
    const uint32_t imguiSamplerMin = std::max<uint32_t>(IMGUI_IMPL_VULKAN_MINIMUM_SAMPLER_POOL_SIZE, 1000);
    std::array poolSizes{vk::DescriptorPoolSize{.type = vk::DescriptorType::eSampler, .descriptorCount = imguiSamplerMin},
                         vk::DescriptorPoolSize{.type = vk::DescriptorType::eSampledImage, .descriptorCount = imguiSampledImageMin},
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
    setDebugName(vkDevice, imguiDescriptorPool, "ImGuiDescriptorPool");
}

void Engine::run()
{
    if (!initialized)
    {
        initialize();
    }


    bool quit = false;
    bool minimized = false;
    bool mouseCaptured = true;   // tracks SDL relative-mouse state
    lastTime = std::chrono::high_resolution_clock::now();
    fpsTime = lastTime;
    const double targetMs = 1000.0 / 60.0; // 60 FPS
    auto& deviceRef = device->vkdevice;

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
                if (enableImGui)
                {
                    ImGui_ImplSDL3_ProcessEvent(&e);
                }

                if (e.type == SDL_EVENT_QUIT)
                {
                    quit = true;
                }
                else if (e.type == SDL_EVENT_KEY_DOWN && e.key.scancode == SDL_SCANCODE_ESCAPE)
                {
                    // Toggle relative mouse mode so the user can free the
                    // cursor to interact with ImGui panels.
                    mouseCaptured = !mouseCaptured;
                    SDL_SetWindowRelativeMouseMode(window, mouseCaptured);
                }
                else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && !mouseCaptured &&
                         e.button.button == SDL_BUTTON_LEFT)
                {
                    // Click-to-capture: re-enable relative mode when the user
                    // clicks back into the window after alt-tabbing away.
                    mouseCaptured = true;
                    SDL_SetWindowRelativeMouseMode(window, true);
                }
                else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && mouseCaptured &&
                         e.button.button == SDL_BUTTON_LEFT)
                {
                    // Re-assert relative mode on click — SDL may silently drop
                    // it on focus loss, and it only takes effect on mouse click.
                    SDL_SetWindowRelativeMouseMode(window, true);
                }
                else if (e.type == SDL_EVENT_MOUSE_MOTION && mouseCaptured)
                {
                    camera->rotate(-e.motion.xrel, e.motion.yrel);
                }
                else if (e.type == SDL_EVENT_MOUSE_WHEEL && mouseCaptured)
                {
                    camera->addFov(-e.wheel.y * 2.0f);   // scroll up = zoom in (narrower FOV)
                }
                else if (e.type == SDL_EVENT_WINDOW_FOCUS_GAINED)
                {
                    SDL_SetWindowRelativeMouseMode(window, mouseCaptured);
                }
                else if (e.type == SDL_EVENT_WINDOW_RESIZED)
                {
                    if (swapChain && renderer)
                    {
                        swapChain->recreateSwapChain();
                        renderer->rebuildSwapchainResources();
                    }
                    // Aspect ratio changed — force projection rebuild next frame.
                    camera->setFov(camera->getFovDegrees());
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

        if (enableImGui)
        {
            drawImGui();
        }

        if (!minimized && !quit)
        {
            // ── Camera keyboard movement ──────────────────────────
            // WASD        → forward / back / left / right
            // LShift      → up (+Y)
            // LCtrl       → down (-Y)
            {
                const float dt = std::chrono::duration<float>(currentTime - lastTime).count();
                const float speed = 5.0f;
                const float step = speed * dt;

                int keyCount = 0;
                const bool* keys = SDL_GetKeyboardState(&keyCount);

                if (keys[SDL_SCANCODE_W])      camera->moveForward( step);
                if (keys[SDL_SCANCODE_S])      camera->moveForward(-step);
                if (keys[SDL_SCANCODE_A])      camera->moveRight  (-step);
                if (keys[SDL_SCANCODE_D])      camera->moveRight  ( step);
                if (keys[SDL_SCANCODE_LSHIFT]) camera->moveUp     ( step);
                if (keys[SDL_SCANCODE_LCTRL])  camera->moveUp     (-step);
            }

            ZoneScopedN("DrawFrame");
            renderer->drawFrame();
            camera->updateCameraData(renderer->currentFrame);
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
        lastTime = currentTime;
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

void Engine::drawImGui()
{
    ZoneScopedN("ImGuiNewFrame");
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    // Simple ImGui UI: asset discovery, selection, and placeholder loading controls.
    ImGui::Begin("Engine Controls");
    ImGui::InputText("Assets Path", &assetsPathInput[0], IM_ARRAYSIZE(assetsPathInput));
    if (ImGui::Button("Scan Folder"))
    {
        scanFolder();
    }

    if (discoveredAssets.empty())
    {
        ImGui::TextUnformatted("No .gltf or .glb assets found yet. Scan a folder to populate the dropdown.");
    }
    else
    {
        if (selectedAssetIndex < 0 || static_cast<std::size_t>(selectedAssetIndex) >= discoveredAssets.size())
        {
            selectedAssetIndex = 0;
        }

        const std::size_t selectedIndex = static_cast<std::size_t>(selectedAssetIndex);
        const std::string preview = discoveredAssets.at(selectedIndex).string();
        if (ImGui::BeginCombo("Discovered Assets", preview.c_str()))
        {
            for (std::size_t i = 0; i < discoveredAssets.size(); ++i)
            {
                const bool isSelected = (selectedIndex == i);
                const std::string itemLabel = discoveredAssets.at(i).string();
                if (ImGui::Selectable(itemLabel.c_str(), isSelected))
                {
                    selectedAssetIndex = static_cast<int>(i);
                }

                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }
    }

    ImGui::InputFloat3("Model Position", &loadedModelPosition[0]);
    if (ImGui::Button("Load Object"))
    {
        loadObject();
    }
    ImGui::End();
    ImGui::Render();
}


void Engine::scanFolder()
{
    discoveredAssets.clear();
    selectedAssetIndex = -1;

    std::filesystem::path rootPath = std::filesystem::path(assetsPathInput).make_preferred();
    if (rootPath.empty())
    {
        rootPath = std::filesystem::path(ENGINE_MODELS_DIR).make_preferred();
    }

    std::error_code errorCode;
    if (!std::filesystem::exists(rootPath, errorCode) || !std::filesystem::is_directory(rootPath, errorCode))
    {
        log_info("ImGui scanFolder failed: invalid asset directory", "Engine");
        return;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(rootPath, std::filesystem::directory_options::skip_permission_denied, errorCode))
    {
        if (errorCode)
        {
            break;
        }

        if (!entry.is_regular_file(errorCode))
        {
            continue;
        }

        std::string extension = entry.path().extension().string();
        std::ranges::transform(extension, extension.begin(), [](unsigned char character) -> char {
            return static_cast<char>(std::tolower(character));
        });

        if (extension == ".gltf" || extension == ".glb")
        {
            auto assetPath = entry.path();
            assetPath.make_preferred();
            discoveredAssets.emplace_back(std::move(assetPath));
        }
    }

    std::ranges::sort(discoveredAssets);
    if (!discoveredAssets.empty())
    {
        selectedAssetIndex = 0;
    }

    log_info("ImGui scanFolder found assets", "Engine");
}

void Engine::loadObject()
{
    if (selectedAssetIndex < 0 || static_cast<std::size_t>(selectedAssetIndex) >= discoveredAssets.size())
    {
        log_info("Load Object: no selected asset", "Engine");
        return;
    }

    const std::string assetPath = discoveredAssets[selectedAssetIndex].string();
    log_info("Load Object started", "Engine");
    device->vkdevice.waitIdle();
    assetsLoader->loadModel(assetPath, glm::make_vec3(loadedModelPosition));
    resourceManager->recreateObjectsBuffers();
    resourceManager->createUniformBuffer(*(objects.end()-1));
}

void Engine::shutdown() { cleanup(); }

void Engine::cleanup()
{
    if (!initialized) return;
    auto& deviceRef = device->vkdevice;
    deviceRef.waitIdle();

    if (tracyContext)
    {
        tracyContext->shutdown();
        tracyContext.reset();
    }

    if (enableImGui)
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        // clear() destroys the VkDescriptorPool. reset() is vkResetDescriptorPool and
        // leaves the raii handle (and its DeviceDispatcher*) alive past device.reset().
        imguiDescriptorPool.clear();
    }

    objects.clear();
    log_info("Objects cleared", "Engine");
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
    resourceManager.reset(); // before assetsLoader: holds refs to its vertex/index vectors
    assetsLoader.reset();
    camera.reset();
    log_info("Resources cleaned up", "Engine");
    swapChain.reset();
    log_info("Swap chain cleaned up", "Engine");
    allocator.reset();
    log_info("Allocator cleaned up", "Engine");
    SDL_DestroyWindow(window);
    log_info("Window destroyed", "Engine");
    window = nullptr;
    SDL_Quit();
    device.reset();
    log_info("Device cleaned up", "Engine");
    initialized = false;
    log_info("Engine cleaned up", "Engine");
}
