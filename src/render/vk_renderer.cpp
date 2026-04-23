#include "vk_renderer.hpp"

#include "push_data.hpp"

#include "../Constants.h"
#include "../static_headers/logger.hpp"
#include "../util/vk_tracy.hpp"
#include "imgui.h"
#include "imgui_impl_vulkan.h"

#include <format>

Renderer::Renderer(Device& device, SwapChain& swapChain, ResourceManager& resourceManager,
                   DescriptorManager& descriptorManager, Pipeline& pipeline, VkTracyContext* tracyContext,
                   bool imguiEnabled) :
    device(device), swapChain(swapChain), resourceManager(resourceManager), descriptorManager(descriptorManager),
    pipeline(pipeline), tracyContext(tracyContext), imguiEnabled(imguiEnabled)
{
}

void Renderer::setTracyContext(VkTracyContext* tracyContextIn) { tracyContext = tracyContextIn; }

void Renderer::rebuildSwapchainResources() const
{
    resourceManager.updateSwapChainExtent(swapChain.swapChainExtent);
    resourceManager.updateSwapChainImageFormat(swapChain.swapChainImageFormat);
    resourceManager.setSwapChainImageCount(static_cast<uint32_t>(swapChain.swapChainImages.size()));
    resourceManager.createColorResources();
    resourceManager.createDepthResources();
}

void Renderer::drawFrame()
{
    ZoneScoped;
    auto& deviceRef = device.getDevice();
    auto& graphicsQueue = device.getGraphicsQueue();
    auto& presentQueue = device.getPresentQueue();
    auto& swapChainKHR = swapChain.swapChain;
    auto& fence = *resourceManager.inFlightFences[currentFrame];
    auto& presentSemaphore = *resourceManager.presentCompleteSemaphore[currentFrame];
    auto& commandBuffer = resourceManager.commandBuffers[currentFrame];

    TracyPlot("Vulkan/SwapchainImagesInUse", static_cast<double>(swapChain.swapChainImages.size()));
    TracyPlot("Vulkan/CommandBuffersInUse", static_cast<double>(resourceManager.commandBuffers.size()));
    TracyPlot("Vulkan/UniformBuffersInUse", static_cast<double>(resourceManager.uniformBuffers.size()));
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
        resourceManager.updateUniformBuffer(currentFrame);
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

    const auto indexCount = resourceManager.indices.size();
    commandBuffers[currentFrame].begin({});

    const vk::ImageSubresourceRange colorRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
    {
        ZoneScopedN("TransitionToRender");
#ifdef TRACY_ENABLE
        if (tracyContext && tracyContext->active()) {
            TracyVkZone(tracyContext->handle(), *commandBuffers[currentFrame], "GPU_TransitionToRender");
        }
#endif
        resourceManager.transitionImageLayout(
            &commandBuffers[currentFrame], swapChain.swapChainImages[imageIndex], 1, vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal, colorRange, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            vk::PipelineStageFlagBits2::eTopOfPipe, vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eNone, vk::AccessFlagBits2::eColorAttachmentWrite);
    }

    const vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
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
        if (tracyContext && tracyContext->active()) {
            TracyVkZone(tracyContext->handle(), *commandBuffers[currentFrame], "GPU_DrawCalls");
        }
#endif
        commandBuffers[currentFrame].beginRendering(renderingInfo);
        commandBuffers[currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline.graphicsPipeline);
        commandBuffers[currentFrame].bindVertexBuffers(0, *resourceManager.vertexBuffer, {0});
        commandBuffers[currentFrame].bindIndexBuffer(*resourceManager.indexBuffer, 0, vk::IndexType::eUint32);
        commandBuffers[currentFrame].setViewport(
            0,
            vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChain.swapChainExtent.width),
                         static_cast<float>(swapChain.swapChainExtent.height), 0.0f, 1.0f));
        commandBuffers[currentFrame].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChain.swapChainExtent));

        // TODO optimize branch prediction by spliting one rendering function into two and just decinding what to use.
        // After that use a pointer to that function.
        if (descriptorManager.usesDescriptorHeaps()) {
            const auto& resourceHeapInfo = descriptorManager.getResourceHeapInfo();
            const auto& samplerHeapInfo = descriptorManager.getSamplerHeapInfo();
            commandBuffers[currentFrame].bindResourceHeapEXT(resourceHeapInfo);
            commandBuffers[currentFrame].bindSamplerHeapEXT(samplerHeapInfo);

            PushData pushData{};
            const VkBufferDeviceAddressInfo uboAddressInfo = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                .pNext = nullptr,
                .buffer = *resourceManager.uniformBuffers[currentFrame],
            };
            pushData.uboAddress = vkGetBufferDeviceAddress(*device.getDevice(), &uboAddressInfo);
            pushData.texture = {
                .resourceIndex = descriptorManager.getTextureDescriptorIndex(),
                .samplerIndex = 0,
            };
            pushData.samplerHandle = {
                .resourceIndex = descriptorManager.getSamplerDescriptorIndex(),
                .samplerIndex = 0,
            };

            vk::PushDataInfoEXT pushDataInfo = {
                .sType = vk::StructureType::ePushDataInfoEXT,
                .pNext = nullptr,
                .offset = 0,
                .data = vk::HostAddressRangeConstEXT{.address = &pushData, .size = sizeof(PushData)}};
            commandBuffers[currentFrame].pushDataEXT(pushDataInfo);

        } else {
            commandBuffers[currentFrame].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.pipelineLayout,
                                                            0, *descriptorManager.descriptorSets[currentFrame], {});
        }


        commandBuffers[currentFrame].drawIndexed(static_cast<uint32_t>(indexCount), 1, 0, 0, 0);
    }

    if (imguiEnabled) {
        ZoneScopedN("RenderImGui");
#ifdef TRACY_ENABLE
        if (tracyContext && tracyContext->active()) {
            TracyVkZone(tracyContext->handle(), *commandBuffers[currentFrame], "GPU_ImGui");
        }
#endif
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *commandBuffers[currentFrame]);
    }

    commandBuffers[currentFrame].endRendering();
    {
        ZoneScopedN("TransitionToPresent");
#ifdef TRACY_ENABLE
        if (tracyContext && tracyContext->active()) {
            TracyVkZone(tracyContext->handle(), *commandBuffers[currentFrame], "GPU_TransitionToPresent");
        }
#endif
        resourceManager.transitionImageLayout(
            &commandBuffers[currentFrame], swapChain.swapChainImages[imageIndex], 1,
            vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR, colorRange,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::PipelineStageFlagBits2::eBottomOfPipe, vk::AccessFlagBits2::eColorAttachmentWrite, {});
    }

    if (tracyContext) {
        tracyContext->collect(commandBuffers[currentFrame]);
    }

    commandBuffers[currentFrame].end();
}

void Renderer::waitIdle() const { device.getDevice().waitIdle(); }
