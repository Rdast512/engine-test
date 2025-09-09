#include "Application.h"
#include "src/resources/ResourceManager.h"

// TDOD move resources like Texture data to its own texture class
// TODO add functions to ownership
// TODO rewrite sync functions to async (like for ones what fill up command
// buffers)
// TODO add support for GLTF and KTX2
// TODO Imgui integration
// TODO Instanced rendering
// TODO Dynamic uniforms and descriptors
// TODO Pipeline cache
// TODO Multi-threaded command buffer generation
// TODO Multiple subpasses
// TODO Integrate VMA for memory management
// TODO Integrate Volk
// TODO Rebase project structure to use different files and dirs
// TODO (createVertexBuffer, createIndexBuffer, createTextureImage) are still
// using the "single-time command" pattern

Application::Application() {}

Application::~Application() {}

void Application::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void Application::initWindow() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error("Failed to initialize SDL");
    }

    window = SDL_CreateWindow("Vulkan", WIDTH, HEIGHT,
                              SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        throw std::runtime_error("Failed to create window");
    }
}

void Application::initVulkan() {
    context = std::make_unique<VulkanContext>(window, true);
    context->init();

    assetsLoader = std::make_unique<AssetsLoader>();

    resourceManager =
        std::make_unique<ResourceManager>(context.get(), assetsLoader.get());

    swapChain = std::make_unique<SwapChain>(window, context.get(),
                                            resourceManager.get());
    swapChain->init();
    resourceManager->updateSwapChainExtent(swapChain->swapChainExtent);
    resourceManager->setSwapChainImageCount(
        static_cast<uint32_t>(swapChain->swapChainImages.size()));
    resourceManager->init();

    textureManager = std::make_unique<TextureManager>(
        resourceManager.get(), context->physicalDevice, context->device);
    textureManager->init();
    descriptorManager = std::make_unique<DescriptorManager>(
        context->device, resourceManager->getUniformBuffers(),
        textureManager->getTextureSampler(),
        textureManager->getTextureImageView());
    descriptorManager->init();
    pipeline = std::make_unique<Pipeline>(
        resourceManager.get(), context->device, swapChain->swapChainExtent,
        swapChain->swapChainImageFormat,
        descriptorManager->getDescriptorSetLayout());
    pipeline->init();
}

void Application::drawFrame() {

    // --- Local references for clarity ---
    auto &device = context->device;
    auto &graphicsQueue = context->graphicsQueue;
    auto &swapChain_KHR = this->swapChain->swapChain;
    auto &fence = *resourceManager->inFlightFences[currentFrame];
    auto &presentSemaphore =
        *resourceManager->presentCompleteSemaphore[currentFrame];
    auto &commandBuffer = resourceManager->commandBuffers[currentFrame];

    while (vk::Result::eTimeout ==
           device.waitForFences(fence, vk::True, UINT64_MAX))
        ;
    // --- Frame logic using references ---
    auto [result, imageIndex] =
        swapChain_KHR.acquireNextImage(UINT64_MAX, presentSemaphore, nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR) {
        swapChain->recreateSwapChain();
        return;
    }
    if (result != vk::Result::eSuccess &&
        result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    device.resetFences(fence);
    commandBuffer.reset();
    recordCommandBuffer(imageIndex);

    // Define which semaphore to wait on, and at which pipeline stage.
    vk::SemaphoreSubmitInfo waitSemaphoreInfo = {
        .semaphore = presentSemaphore,
        .stageMask = vk::PipelineStageFlagBits2::eTopOfPipe};

    // Define the command buffer to be submitted.
    vk::CommandBufferSubmitInfo commandBufferInfo = {.commandBuffer =
                                                         *commandBuffer};

    // Define which semaphore to signal when the command buffer is done.
    auto &renderSemaphore =
        *resourceManager->renderFinishedSemaphore[imageIndex];
    vk::SemaphoreSubmitInfo signalSemaphoreInfo = {
        .semaphore = renderSemaphore,
        .stageMask = vk::PipelineStageFlagBits2::eBottomOfPipe};

    resourceManager->updateUniformBuffer(currentFrame);

    const vk::SubmitInfo2 submitInfo{.waitSemaphoreInfoCount = 1,
                                     .pWaitSemaphoreInfos = &waitSemaphoreInfo,
                                     .commandBufferInfoCount = 1,
                                     .pCommandBufferInfos = &commandBufferInfo,
                                     .signalSemaphoreInfoCount = 1,
                                     .pSignalSemaphoreInfos =
                                         &signalSemaphoreInfo};

    graphicsQueue.submit2(submitInfo, fence);

    const vk::PresentInfoKHR presentInfoKHR{.waitSemaphoreCount = 1,
                                            .pWaitSemaphores = &renderSemaphore,
                                            .swapchainCount = 1,
                                            .pSwapchains = &*swapChain_KHR,
                                            .pImageIndices = &imageIndex};
    result = graphicsQueue.presentKHR(presentInfoKHR);

    if (result == vk::Result::eErrorOutOfDateKHR ||
        result == vk::Result::eSuboptimalKHR) {
        swapChain->recreateSwapChain();
    } else if (result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    switch (result) {
    case vk::Result::eSuccess:
        break;
    default:
        break;
    }
    semaphoreIndex =
        (semaphoreIndex + 1) % resourceManager->presentCompleteSemaphore.size();
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Application::recordCommandBuffer(uint32_t imageIndex) {
    auto &commandBuffers = resourceManager->commandBuffers;
    auto indexCount = (resourceManager->indices.size());
    commandBuffers[currentFrame].begin({});
    // Before starting rendering, transition the swapchain image to
    // COLOR_ATTACHMENT_OPTIMAL
    resourceManager->transitionImageLayout(
        &commandBuffers[currentFrame], swapChain->swapChainImages[imageIndex],
        1, vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        DEFAULT_COLOR_SUBRESOURCE_RANGE, VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        vk::PipelineStageFlagBits2::eTopOfPipe,             // srcStage
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // dstStage
        {}, // srcAccessMask (no need to wait for previous operations)
        vk::AccessFlagBits2::eColorAttachmentWrite // dstAccessMask

    );

    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);
    vk::RenderingAttachmentInfo colorAttachmentInfo = {
        .imageView = swapChain->swapChainImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor};
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
    commandBuffers[currentFrame].drawIndexed(indexCount, 1, 0, 0, 0);
    commandBuffers[currentFrame].endRendering();
    // After rendering, transition the swapchain image to PRESENT_SRC
    resourceManager->transitionImageLayout(
        &commandBuffers[currentFrame], swapChain->swapChainImages[imageIndex],
        1, vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR, DEFAULT_COLOR_SUBRESOURCE_RANGE,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, // srcStage
        vk::PipelineStageFlagBits2::eBottomOfPipe,          // dstStage
        vk::AccessFlagBits2::eColorAttachmentWrite,         // srcAccessMask
        {}                                                  // dstAccessMask
    );
    commandBuffers[currentFrame].end();
}

void Application::mainLoop() {
    bool quit = false;
    bool minimized = false;
    lastTime = std::chrono::high_resolution_clock::now();
    fpsTime = lastTime;

    auto &device = context->device;

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
            drawFrame();
        } else {
            SDL_Delay(100);
        }
    }
    device.waitIdle();
}

void Application::cleanup() {
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
}
