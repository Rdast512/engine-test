#include "vk_renderer.hpp"
#include "push_data.hpp"
#include "../Constants.h"
#include "../util/vk_tracy.hpp"
#include "../util/vk_utils.hpp"
#if ENGINE_ENABLE_IMGUI
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#endif

#include <format>

Renderer::Renderer(Device& device, SwapChain& swapChain, ResourceManager& resourceManager,
                   DescriptorManager& descriptorManager, Pipeline& pipeline, Camera& camera, VkTracyContext* tracyContext,
                   bool imguiEnabled) :
    device(device), swapChain(swapChain), resourceManager(resourceManager), descriptorManager(descriptorManager),
    pipeline(pipeline), tracyContext(tracyContext), imguiEnabled(imguiEnabled), camera(camera)
{
}

void Renderer::setTracyContext(VkTracyContext* tracyContextIn) { tracyContext = tracyContextIn; }

void Renderer::rebuildSwapchainResources() const
{
    ZoneScopedN("SwapchainRecreate");
    resourceManager.updateSwapChainExtent(swapChain.swapChainExtent);
    resourceManager.updateSwapChainImageFormat(swapChain.swapChainImageFormat);
    resourceManager.setSwapChainImageCount(static_cast<uint32_t>(swapChain.swapChainImages.size()));
    resourceManager.createColorResources();
    resourceManager.createDepthResources();
}

void Renderer::drawFrame()
{
    ZoneScopedN("Renderer::drawFrame");
    auto& deviceRef = device.vkdevice;
    auto& graphicsQueue = device.graphicsQueue;
    auto& presentQueue = device.presentQueue;
    auto& swapChainKHR = swapChain.swapChain;
    auto& fence = *resourceManager.inFlightFences[currentFrame];
    auto& presentSemaphore = *resourceManager.presentCompleteSemaphore[currentFrame];
    auto& commandBuffer = resourceManager.commandBuffers[currentFrame];

    TracyPlot("Vulkan/SwapchainImagesInUse", static_cast<double>(swapChain.swapChainImages.size()));
    TracyPlot("Vulkan/CommandBuffersInUse", static_cast<double>(resourceManager.commandBuffers.size()));
    TracyPlot("Vulkan/InstanceCapacity", static_cast<double>(resourceManager.instanceCapacity));
    TracyPlot("Vulkan/EntityCount", static_cast<double>(resourceManager.objectStorage.size()));
    TracyPlot("Vulkan/VerticesInUse", static_cast<double>(resourceManager.vertices.size()));
    TracyPlot("Vulkan/IndicesInUse", static_cast<double>(resourceManager.indices.size()));
    TracyPlot("Vulkan/VertexBytesInUse", static_cast<double>(resourceManager.vertices.size() * sizeof(Vertex)));
    TracyPlot("Vulkan/IndexBytesInUse", static_cast<double>(resourceManager.indices.size() * sizeof(uint32_t)));

    {
        ZoneScopedN("FenceWait");
        while (vk::Result::eTimeout == deviceRef.waitForFences(fence, vk::True, UINT64_MAX))
            ;
    }

    vk::Result result;
    uint32_t imageIndex;
    {
        ZoneScopedN("AcquireImage");
        auto acquired = swapChainKHR.acquireNextImage(UINT64_MAX, presentSemaphore, nullptr);
        result = acquired.result;
        imageIndex = acquired.value;
    }

    if (result == vk::Result::eErrorOutOfDateKHR) {
        ZoneScopedN("SwapchainRecreate_Acquire");
        swapChain.recreateSwapChain();
        rebuildSwapchainResources();
        return;
    }

    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    deviceRef.resetFences(fence);
    commandBuffer.reset();
    {
        ZoneScopedN("RecordCommandBuffer");
        recordCommandBuffer(imageIndex);
    }

    vk::SemaphoreSubmitInfo waitSemaphoreInfo = {.semaphore = presentSemaphore,
                                                 .stageMask = vk::PipelineStageFlagBits2::eTopOfPipe};

    vk::CommandBufferSubmitInfo commandBufferInfo = {.commandBuffer = *commandBuffer};

    auto& renderSemaphore = *resourceManager.renderFinishedSemaphore[imageIndex];
    vk::SemaphoreSubmitInfo signalSemaphoreInfo = {.semaphore = renderSemaphore,
                                                   .stageMask = vk::PipelineStageFlagBits2::eBottomOfPipe};

    {
        ZoneScopedN("UpdateUBO");
        resourceManager.updateUniformBuffers(currentFrame);
    }

    const vk::SubmitInfo2 submitInfo{.waitSemaphoreInfoCount = 1,
                                     .pWaitSemaphoreInfos = &waitSemaphoreInfo,
                                     .commandBufferInfoCount = 1,
                                     .pCommandBufferInfos = &commandBufferInfo,
                                     .signalSemaphoreInfoCount = 1,
                                     .pSignalSemaphoreInfos = &signalSemaphoreInfo};
    {
        ZoneScopedN("QueueSubmit");
        graphicsQueue.submit2(submitInfo, fence);
    }

    const vk::PresentInfoKHR presentInfoKHR{.waitSemaphoreCount = 1,
                                            .pWaitSemaphores = &renderSemaphore,
                                            .swapchainCount = 1,
                                            .pSwapchains = &*swapChainKHR,
                                            .pImageIndices = &imageIndex};
    {
        ZoneScopedN("Present");
        result = presentQueue.presentKHR(presentInfoKHR);
    }

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        ZoneScopedN("SwapchainRecreate_Present");
        swapChain.recreateSwapChain();
        rebuildSwapchainResources();
    } else if (result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    semaphoreIndex = (semaphoreIndex + 1) % resourceManager.presentCompleteSemaphore.size();
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::recordCommandBuffer(uint32_t imageIndex)
{
    ZoneScoped;
    auto& commandBuffers = resourceManager.commandBuffers;
    auto& cmd = commandBuffers[currentFrame];

#ifdef TRACY_ENABLE
    const bool gpuTrace = tracyContext != nullptr && tracyContext->active();
    TracyVkCtx const gpuCtx = gpuTrace ? tracyContext->handle() : nullptr;
#endif

    cmd.begin({});

    const vk::ImageSubresourceRange colorRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
    {
        ZoneScopedN("TransitionToRender");
#ifdef TRACY_ENABLE
        TracyVkNamedZone(gpuCtx, gpuZoneTransitionToRender, *cmd, "GPU_TransitionToRender", gpuTrace);
#endif
        transitionImageLayout(&cmd, swapChain.swapChainImages[imageIndex], 1,
                              vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, colorRange,
                              VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, vk::PipelineStageFlagBits2::eTopOfPipe,
                              vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eNone,
                              vk::AccessFlagBits2::eColorAttachmentWrite);
    }

    // NVIDIA compressed clear requires all-0 or all-1 components for sRGB targets
    // (BestPractices-NVIDIA-ClearColor-NotCompressed). Alpha 1.0 blocked compression.
    const vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f);
    const vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);
    vk::RenderingAttachmentInfo colorAttachmentInfo = {.imageView = resourceManager.colorImageView,
                                                       .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                                                       .resolveMode = vk::ResolveModeFlagBits::eAverage,
                                                       .resolveImageView = swapChain.swapChainImageViews[imageIndex],
                                                       .resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                                                       .loadOp = vk::AttachmentLoadOp::eClear,
                                                       .storeOp = vk::AttachmentStoreOp::eStore,
                                                       .clearValue = clearColor};
    vk::RenderingAttachmentInfo depthAttachmentInfo = {.imageView = resourceManager.depthImageView,
                                                       .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
                                                       .loadOp = vk::AttachmentLoadOp::eClear,
                                                       .storeOp = vk::AttachmentStoreOp::eDontCare,
                                                       .clearValue = clearDepth};

    vk::RenderingInfo renderingInfo = {.renderArea = {.offset = {0, 0}, .extent = swapChain.swapChainExtent},
                                       .layerCount = 1,
                                       .colorAttachmentCount = 1,
                                       .pColorAttachments = &colorAttachmentInfo,
                                       .pDepthAttachment = &depthAttachmentInfo};

    {
        ZoneScopedN("DrawCalls");
#ifdef TRACY_ENABLE
        TracyVkNamedZone(gpuCtx, gpuZoneDrawCalls, *cmd, "GPU_DrawCalls", gpuTrace);
#endif
        cmd.beginRendering(renderingInfo);
        // TODO remove all the old pipeline since target is mesh only
        if (*pipeline.meshPipeline) {
            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline.meshPipeline);
        } else {
            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline.graphicsPipeline);
        }
        cmd.bindVertexBuffers(0, *resourceManager.vertexBuffer, {0});
        cmd.bindIndexBuffer(*resourceManager.indexBuffer, 0, vk::IndexType::eUint32);
        cmd.setViewport(
            0,
            vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
                         static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
        cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

        const auto& resourceHeapInfo = descriptorManager.resourceHeapInfo;
        const auto& samplerHeapInfo = descriptorManager.samplerHeapInfo;
        cmd.bindResourceHeapEXT(resourceHeapInfo);
        cmd.bindSamplerHeapEXT(samplerHeapInfo);

        const auto& storage = resourceManager.objectStorage;
        const uint32_t entityCount = storage.size();

        for (EntityId id = 0; id < entityCount; ++id)
        {
            if ((storage.flags[id] & EntityFlag::Active) == 0)
            {
                continue;
            }

            PushData2 pushData{};
            pushData.ObjectUBAddress = resourceManager.instanceUboAddress(currentFrame, id);
            pushData.cameraAddress = camera.cameraBufferAddresses[currentFrame];
            pushData.texture = {
                .resourceIndex = storage.materials[id].textureIndex,
                .samplerIndex = 0,
            };
            pushData.samplerHandle = {
                .resourceIndex = descriptorManager.getSamplerDescriptorIndex(),
                .samplerIndex = 0,
            };

            vk::PushDataInfoEXT const pushDataInfo = {
                .sType = vk::StructureType::ePushDataInfoEXT,
                .pNext = nullptr,
                .offset = 0,
                .data = vk::HostAddressRangeConstEXT{.address = &pushData, .size = sizeof(PushData2)}};
            cmd.pushDataEXT(pushDataInfo);

            const MeshDraw& draw = storage.meshDraws[id];

            if (*pipeline.meshPipeline) {
                cmd.drawMeshTasksEXT(1, 1, 1);
            } else {
                cmd.drawIndexed(draw.indexCount, 1, draw.firstIndex, static_cast<int32_t>(draw.baseVertex), 0);
            }
        }
    }

#if ENGINE_ENABLE_IMGUI
    if (imguiEnabled && imguiVisible) {
        ZoneScopedN("RenderImGui");
#ifdef TRACY_ENABLE
        TracyVkNamedZone(gpuCtx, gpuZoneImGui, *cmd, "GPU_ImGui", gpuTrace);
#endif
        if (ImGui::GetDrawData() != nullptr) {
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *cmd);
        }
    }
#endif

    cmd.endRendering();
    {
        ZoneScopedN("TransitionToPresent");
#ifdef TRACY_ENABLE
        TracyVkNamedZone(gpuCtx, gpuZoneTransitionToPresent, *cmd, "GPU_TransitionToPresent", gpuTrace);
#endif
        transitionImageLayout(
            &cmd, swapChain.swapChainImages[imageIndex], 1,
            vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR, colorRange,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eBottomOfPipe, vk::AccessFlagBits2::eColorAttachmentWrite, {});
    }

    if (tracyContext) {
        ZoneScopedN("TracyVkCollect");
        tracyContext->collect(cmd);
    }

    cmd.end();
}

void Renderer::waitIdle() const { device.vkdevice.waitIdle(); }
