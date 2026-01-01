#include "vk_engine.hpp"
#include "../Constants.h"

// Helper to rebuild swapchain-dependent resources (color/depth) after swapchain changes.
static void rebuildSwapchainResources(ResourceManager &resourceManager, SwapChain &swapChain) {
    resourceManager.updateSwapChainExtent(swapChain.swapChainExtent);
    resourceManager.updateSwapChainImageFormat(swapChain.swapChainImageFormat);
    resourceManager.setSwapChainImageCount(static_cast<uint32_t>(swapChain.swapChainImages.size()));
    resourceManager.createColorResources();
    resourceManager.createDepthResources();
}

void Engine::initialize() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error("Failed to initialize SDL: " + std::string(SDL_GetError()));
    }

    window = SDL_CreateWindow("Vulkan", static_cast<int>(WIDTH), static_cast<int>(HEIGHT),
                              SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        throw std::runtime_error("Failed to create window");
    }

    device = std::make_unique<Device>(window, false);
    device->init();

    allocator = std::make_unique<VkAllocator>(device.get());

    swapChain = std::make_unique<SwapChain>(window, device.get());
    swapChain->init();

    modelStorage = std::make_unique<ModelStorage>();
    assetsLoader = std::make_unique<AssetsLoader>(*modelStorage);

    resourceManager = std::make_unique<ResourceManager>(device.get(), assetsLoader.get(), allocator.get());
    resourceManager->init();
    rebuildSwapchainResources(*resourceManager, *swapChain);

    textureManager = std::make_unique<TextureManager>(*device, *resourceManager);
    textureManager->init();

    descriptorManager = std::make_unique<DescriptorManager>(
        device->getDevice(),
        resourceManager->getUniformBuffers(),
        textureManager->getTextureSampler(),
        textureManager->getTextureImageView());
    descriptorManager->init();

    pipeline = std::make_unique<Pipeline>(
        resourceManager.get(),
        device->getDevice(),
        swapChain->swapChainExtent,
        swapChain->swapChainImageFormat,
        descriptorManager->getDescriptorSetLayout());
    pipeline->init();

    initialized = true;
}

void Engine::run() {
    if (!initialized) {
        initialize();
    }

    bool quit = false;
    bool minimized = false;
    lastTime = std::chrono::high_resolution_clock::now();
    fpsTime = lastTime;
    const double targetMs = 1000.0 / 60.0; // 60 FPS
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
            } else if (e.type == SDL_EVENT_WINDOW_RESIZED) {
                if (swapChain && resourceManager) {
                    swapChain->recreateSwapChain();
                    rebuildSwapchainResources(*resourceManager, *swapChain);
                }
            } else if (e.type == SDL_EVENT_WINDOW_MINIMIZED) {
                minimized = true;
            } else if (e.type == SDL_EVENT_WINDOW_RESTORED) {
                minimized = false;
            }
        }

        if (!minimized && !quit) {
            drawFrame();
        } else {
            SDL_Delay(100);
        }
        auto frameEndTime = std::chrono::high_resolution_clock::now();
        auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEndTime - currentTime).count();
        if (frameDuration < targetMs) {
            SDL_Delay(static_cast<Uint32>(targetMs - frameDuration));
        }
    }
    deviceRef.waitIdle();
}

void Engine::drawFrame() {
    auto &deviceRef = device->getDevice();
    auto &graphicsQueue = device->getGraphicsQueue();
    auto &presentQueue = device->getPresentQueue();
    auto &swapChainKHR = swapChain->swapChain;
    auto &fence = *resourceManager->inFlightFences[currentFrame];
    auto &presentSemaphore = *resourceManager->presentCompleteSemaphore[currentFrame];
    auto &commandBuffer = resourceManager->commandBuffers[currentFrame];

    while (vk::Result::eTimeout == deviceRef.waitForFences(fence, vk::True, UINT64_MAX));

    auto [result, imageIndex] = swapChainKHR.acquireNextImage(UINT64_MAX, presentSemaphore, nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR) {
        swapChain->recreateSwapChain();
        rebuildSwapchainResources(*resourceManager, *swapChain);
        return;
    }

    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    deviceRef.resetFences(fence);
    commandBuffer.reset();
    recordCommandBuffer(imageIndex);

    vk::SemaphoreSubmitInfo waitSemaphoreInfo = {
        .semaphore = presentSemaphore,
        .stageMask = vk::PipelineStageFlagBits2::eTopOfPipe};

    vk::CommandBufferSubmitInfo commandBufferInfo = {.commandBuffer = *commandBuffer};

    auto &renderSemaphore = *resourceManager->renderFinishedSemaphore[imageIndex];
    vk::SemaphoreSubmitInfo signalSemaphoreInfo = {
        .semaphore = renderSemaphore,
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

    if (result == vk::Result::eErrorOutOfDateKHR ||
        result == vk::Result::eSuboptimalKHR) {
        swapChain->recreateSwapChain();
        rebuildSwapchainResources(*resourceManager, *swapChain);
    } else if (result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    semaphoreIndex = (semaphoreIndex + 1) % resourceManager->presentCompleteSemaphore.size();
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Engine::recordCommandBuffer(uint32_t imageIndex) {
    auto &commandBuffers = resourceManager->commandBuffers;
    auto indexCount = (resourceManager->indices.size());
    commandBuffers[currentFrame].begin({});

    const vk::ImageSubresourceRange colorRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
    resourceManager->transitionImageLayout(
        &commandBuffers[currentFrame], swapChain->swapChainImages[imageIndex],
        1, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        colorRange,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        vk::PipelineStageFlagBits2::eTopOfPipe,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eNone,
        vk::AccessFlagBits2::eColorAttachmentWrite
    );

    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);
    vk::RenderingAttachmentInfo colorAttachmentInfo = {
        .imageView          = resourceManager->colorImageView,
        .imageLayout        = vk::ImageLayout::eColorAttachmentOptimal,
        .resolveMode        = vk::ResolveModeFlagBits::eAverage,
        .resolveImageView   = swapChain->swapChainImageViews[imageIndex],
        .resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp             = vk::AttachmentLoadOp::eClear,
        .storeOp            = vk::AttachmentStoreOp::eStore,
        .clearValue         = clearColor};
    vk::RenderingAttachmentInfo depthAttachmentInfo = {
        .imageView = resourceManager->depthImageView,
        .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .clearValue = clearDepth};

    vk::RenderingInfo renderingInfo = {
        .renderArea = {.offset = {0, 0}, .extent = swapChain->swapChainExtent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentInfo,
        .pDepthAttachment = &depthAttachmentInfo};

    commandBuffers[currentFrame].beginRendering(renderingInfo);
    commandBuffers[currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics,
                                              *pipeline->graphicsPipeline);
    commandBuffers[currentFrame].bindVertexBuffers(
        0, *resourceManager->vertexBuffer, {0});
    commandBuffers[currentFrame].bindIndexBuffer(*resourceManager->indexBuffer,
                                                 0, vk::IndexType::eUint32);
    commandBuffers[currentFrame].bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, pipeline->pipelineLayout, 0,
        *descriptorManager->descriptorSets[currentFrame], nullptr);
    commandBuffers[currentFrame].setViewport(
        0, vk::Viewport(0.0f, 0.0f,
                        static_cast<float>(swapChain->swapChainExtent.width),
                        static_cast<float>(swapChain->swapChainExtent.height),
                        0.0f, 1.0f));
    commandBuffers[currentFrame].setScissor(
        0, vk::Rect2D(vk::Offset2D(0, 0), swapChain->swapChainExtent));
    commandBuffers[currentFrame].drawIndexed(static_cast<uint32_t>(indexCount), 1, 0, 0, 0);
    commandBuffers[currentFrame].endRendering();
    // After rendering, transition the swapchain image to PRESENT_SRC
    resourceManager->transitionImageLayout(
        &commandBuffers[currentFrame], swapChain->swapChainImages[imageIndex],
        1, vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR, colorRange,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // srcStage
        vk::PipelineStageFlagBits2::eBottomOfPipe,          // dstStage
        vk::AccessFlagBits2::eColorAttachmentWrite,         // srcAccessMask
        {}                                                  // dstAccessMask
    );
    commandBuffers[currentFrame].end();
}

void Engine::render() {
    drawFrame();
}

void Engine::shutdown() {
    cleanup();
}

void Engine::cleanup() {
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
}
