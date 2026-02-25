#include "vk_engine.hpp"
#include "../Constants.h"
#include "../static_headers/logger.hpp"
#include "../util/vk_tracy.hpp"


// Helper to rebuild swapchain-dependent resources (color/depth) after swapchain changes.
static void rebuildSwapchainResources(ResourceManager& resourceManager, SwapChain& swapChain)
{
    resourceManager.updateSwapChainExtent(swapChain.swapChainExtent);
    resourceManager.updateSwapChainImageFormat(swapChain.swapChainImageFormat);
    resourceManager.setSwapChainImageCount(static_cast<uint32_t>(swapChain.swapChainImages.size()));
    resourceManager.createColorResources();
    resourceManager.createDepthResources();
}


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
    rebuildSwapchainResources(*resourceManager, *swapChain);

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
                if (swapChain && resourceManager)
                {
                    swapChain->recreateSwapChain();
                    rebuildSwapchainResources(*resourceManager, *swapChain);
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


        // Start ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        ImGui::ShowDemoWindow();
        ImGui::Render();

        if (!minimized && !quit)
        {
            drawFrame();
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
    }
    deviceRef.waitIdle();
}

void Engine::drawFrame()
{
    auto& deviceRef = device->getDevice();
    auto& graphicsQueue = device->getGraphicsQueue();
    auto& presentQueue = device->getPresentQueue();
    auto& swapChainKHR = swapChain->swapChain;
    auto& fence = *resourceManager->inFlightFences[currentFrame];
    auto& presentSemaphore = *resourceManager->presentCompleteSemaphore[currentFrame];
    auto& commandBuffer = resourceManager->commandBuffers[currentFrame];

    while (vk::Result::eTimeout == deviceRef.waitForFences(fence, vk::True, UINT64_MAX))
        ;

    auto [result, imageIndex] = swapChainKHR.acquireNextImage(UINT64_MAX, presentSemaphore, nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR)
    {
        swapChain->recreateSwapChain();
        rebuildSwapchainResources(*resourceManager, *swapChain);
        return;
    }

    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    deviceRef.resetFences(fence);
    commandBuffer.reset();
    recordCommandBuffer(imageIndex);

    vk::SemaphoreSubmitInfo waitSemaphoreInfo = {.semaphore = presentSemaphore,
                                                 .stageMask = vk::PipelineStageFlagBits2::eTopOfPipe};

    vk::CommandBufferSubmitInfo commandBufferInfo = {.commandBuffer = *commandBuffer};

    auto& renderSemaphore = *resourceManager->renderFinishedSemaphore[imageIndex];
    vk::SemaphoreSubmitInfo signalSemaphoreInfo = {.semaphore = renderSemaphore,
                                                   .stageMask = vk::PipelineStageFlagBits2::eBottomOfPipe};

    resourceManager->updateUniformBuffer(currentFrame);

    const vk::SubmitInfo2 submitInfo{.waitSemaphoreInfoCount = 1,
                                     .pWaitSemaphoreInfos = &waitSemaphoreInfo,
                                     .commandBufferInfoCount = 1,
                                     .pCommandBufferInfos = &commandBufferInfo,
                                     .signalSemaphoreInfoCount = 1,
                                     .pSignalSemaphoreInfos = &signalSemaphoreInfo};

    graphicsQueue.submit2(submitInfo, fence);

    const vk::PresentInfoKHR presentInfoKHR{.waitSemaphoreCount = 1,
                                            .pWaitSemaphores = &renderSemaphore,
                                            .swapchainCount = 1,
                                            .pSwapchains = &*swapChainKHR,
                                            .pImageIndices = &imageIndex};
    result = presentQueue.presentKHR(presentInfoKHR);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
    {
        swapChain->recreateSwapChain();
        rebuildSwapchainResources(*resourceManager, *swapChain);
    }
    else if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    semaphoreIndex = (semaphoreIndex + 1) % resourceManager->presentCompleteSemaphore.size();
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Engine::recordCommandBuffer(uint32_t imageIndex)
{
    ZoneScoped;
    auto& commandBuffers = resourceManager->commandBuffers;

    auto indexCount = (resourceManager->indices.size());
    commandBuffers[currentFrame].begin({});

    const vk::ImageSubresourceRange colorRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
    resourceManager->transitionImageLayout(
        &commandBuffers[currentFrame], swapChain->swapChainImages[imageIndex], 1, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal, colorRange, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        vk::PipelineStageFlagBits2::eTopOfPipe, vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eNone, vk::AccessFlagBits2::eColorAttachmentWrite);

    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);
    vk::RenderingAttachmentInfo colorAttachmentInfo = {.imageView = resourceManager->colorImageView,
                                                       .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                                                       .resolveMode = vk::ResolveModeFlagBits::eAverage,
                                                       .resolveImageView = swapChain->swapChainImageViews[imageIndex],
                                                       .resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                                                       .loadOp = vk::AttachmentLoadOp::eClear,
                                                       .storeOp = vk::AttachmentStoreOp::eStore,
                                                       .clearValue = clearColor};
    vk::RenderingAttachmentInfo depthAttachmentInfo = {.imageView = resourceManager->depthImageView,
                                                       .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
                                                       .loadOp = vk::AttachmentLoadOp::eClear,
                                                       .storeOp = vk::AttachmentStoreOp::eDontCare,
                                                       .clearValue = clearDepth};

    vk::RenderingInfo renderingInfo = {.renderArea = {.offset = {0, 0}, .extent = swapChain->swapChainExtent},
                                       .layerCount = 1,
                                       .colorAttachmentCount = 1,
                                       .pColorAttachments = &colorAttachmentInfo,
                                       .pDepthAttachment = &depthAttachmentInfo};

    commandBuffers[currentFrame].beginRendering(renderingInfo);
    commandBuffers[currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline->graphicsPipeline);
    commandBuffers[currentFrame].bindVertexBuffers(0, *resourceManager->vertexBuffer, {0});
    commandBuffers[currentFrame].bindIndexBuffer(*resourceManager->indexBuffer, 0, vk::IndexType::eUint32);
    commandBuffers[currentFrame].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline->pipelineLayout, 0,
                                                    *descriptorManager->descriptorSets[currentFrame], nullptr);
    commandBuffers[currentFrame].setViewport(
        0,
        vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain->swapChainExtent.width),
                     static_cast<float>(swapChain->swapChainExtent.height), 0.0f, 1.0f));
    commandBuffers[currentFrame].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain->swapChainExtent));
    commandBuffers[currentFrame].drawIndexed(static_cast<uint32_t>(indexCount), 1, 0, 0, 0);

    // Render ImGui
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *commandBuffers[currentFrame]);

    commandBuffers[currentFrame].endRendering();
    // After rendering, transition the swapchain image to PRESENT_SRC
    resourceManager->transitionImageLayout(&commandBuffers[currentFrame], swapChain->swapChainImages[imageIndex], 1,
                                           vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
                                           colorRange, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                                           vk::PipelineStageFlagBits2::eColorAttachmentOutput, // srcStage
                                           vk::PipelineStageFlagBits2::eBottomOfPipe, // dstStage
                                           vk::AccessFlagBits2::eColorAttachmentWrite, // srcAccessMask
                                           {} // dstAccessMask
    );
    commandBuffers[currentFrame].end();
}

void Engine::render() { drawFrame(); }

void Engine::shutdown() { cleanup(); }

void Engine::cleanup()
{
    if (!initialized)
        return;
    auto& deviceRef = device->getDevice();
    deviceRef.waitIdle();

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
