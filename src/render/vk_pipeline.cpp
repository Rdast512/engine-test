#include "vk_pipeline.hpp"
#include "push_data.hpp"
#include "../core/vk_descriptors.hpp"
#include "../util/debug.hpp"
#include "../util/vk_tracy.hpp"

#include <array>
#include <limits>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace
{
    // Local shader loader identical to original Utils helper.
    std::vector<char> readFile(const std::string& filename)
    {
        ZoneScopedN("Pipeline::readFile");
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }
        std::vector<char> buffer(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        file.close();

        return buffer;
    }
} // namespace

Pipeline::Pipeline(ResourceManager* resourceManager,
                   DescriptorManager* descriptorManager,
                   const vk::raii::Device& device,
                   const vk::Extent2D& swapChainExtent, const vk::Format& swapChainImageFormat) :
    device(device), swapChainExtent(swapChainExtent), swapChainImageFormat(swapChainImageFormat),
    resourceManager(resourceManager), descriptorManager(descriptorManager)
{
}

void Pipeline::init()
{
    ZoneScopedN("Pipeline::init");
    createGraphicsPipeline();
}

void Pipeline::createGraphicsPipeline()
{
    ZoneScopedN("Pipeline::createGraphicsPipeline");
    const auto shaderDir = std::filesystem::path(ENGINE_SHADER_DIR);
    const auto shaderPath = (shaderDir / "base" / "shader.spv").string();
    vk::raii::ShaderModule shaderModule = resourceManager->createShaderModule(readFile(shaderPath));
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    const bool useDescriptorHeaps = descriptorManager->usesDescriptorHeaps();
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ .pNext = nullptr,
        .stage = vk::ShaderStageFlagBits::eVertex, .module = shaderModule, .pName = "vertMain"};
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ .pNext = nullptr,
        .stage = vk::ShaderStageFlagBits::eFragment, .module = shaderModule, .pName = "fragMain"};

    // TODO rewiew code, understand it and decide if add or not
    // std::array<vk::DescriptorSetAndBindingMappingEXT, 1> vertMappings{};
    // vk::ShaderDescriptorSetAndBindingMappingInfoEXT vertMappingInfo{};
    // std::array<vk::DescriptorSetAndBindingMappingEXT, 2> fragMappings{};
    // vk::ShaderDescriptorSetAndBindingMappingInfoEXT fragMappingInfo{};
    //
    // if (useDescriptorHeaps) {
    //     if (descriptorManager->bufferDescriptorSize > std::numeric_limits<uint32_t>::max() ||
    //         descriptorManager->imageDescriptorSize > std::numeric_limits<uint32_t>::max() ||
    //         descriptorManager->samplerDescriptorSize > std::numeric_limits<uint32_t>::max()) {
    //         throw std::runtime_error("Descriptor size exceeds supported push-index stride range");
    //     }
    //
    //     constexpr uint32_t descriptorHandleStride = sizeof(uint32_t) * 2;
    //     constexpr uint32_t uboPushOffset = 0;
    //     constexpr uint32_t texturePushOffset = descriptorHandleStride;
    //     constexpr uint32_t samplerPushOffset = descriptorHandleStride * 2;
    //
    //     const auto makeHeapPushIndexMapping = [](uint32_t descriptorSet,
    //                                              vk::SpirvResourceTypeFlagsEXT resourceMask,
    //                                              uint32_t pushOffset,
    //                                              uint32_t heapIndexStride) {
    //         vk::DescriptorSetAndBindingMappingEXT mapping{};
    //         mapping.descriptorSet = descriptorSet;
    //         mapping.firstBinding = 0;
    //         mapping.bindingCount = 1;
    //         mapping.resourceMask = resourceMask;
    //         mapping.source = vk::DescriptorMappingSourceEXT::eHeapWithPushIndex;
    //         mapping.sourceData.pushIndex = {
    //             .heapOffset = 0,
    //             .pushOffset = pushOffset,
    //             .heapIndexStride = heapIndexStride,
    //             .heapArrayStride = 0,
    //             .pEmbeddedSampler = nullptr,
    //             .useCombinedImageSamplerIndex = vk::False,
    //             .samplerHeapOffset = 0,
    //             .samplerPushOffset = 0,
    //             .samplerHeapIndexStride = 0,
    //             .samplerHeapArrayStride = 0,
    //         };
    //         return mapping;
    //     };
    //
    //     vertMappings[0] = makeHeapPushIndexMapping(
    //         100,
    //         vk::SpirvResourceTypeFlagBitsEXT::eUniformBuffer,
    //         uboPushOffset,
    //         static_cast<uint32_t>(descriptorManager->bufferDescriptorSize));
    //     vertMappingInfo.mappingCount = static_cast<uint32_t>(vertMappings.size());
    //     vertMappingInfo.pMappings = vertMappings.data();
    //
    //     fragMappings[0] = makeHeapPushIndexMapping(
    //         100,
    //         vk::SpirvResourceTypeFlagBitsEXT::eSampledImage,
    //         texturePushOffset,
    //         static_cast<uint32_t>(descriptorManager->imageDescriptorSize));
    //     fragMappings[1] = makeHeapPushIndexMapping(
    //         101,
    //         vk::SpirvResourceTypeFlagBitsEXT::eSampler,
    //         samplerPushOffset,
    //         static_cast<uint32_t>(descriptorManager->samplerDescriptorSize));
    //     fragMappingInfo.mappingCount = static_cast<uint32_t>(fragMappings.size());
    //     fragMappingInfo.pMappings = fragMappings.data();
    //
    //     vertShaderStageInfo.pNext = &vertMappingInfo;
    //     fragShaderStageInfo.pNext = &fragMappingInfo;
    // }

    std::array shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

    std::vector dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data()};
    vk::PipelineDynamicStateCreateInfo dynamicState{.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
                                                    .pDynamicStates = dynamicStates.data()};
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{.topology = vk::PrimitiveTopology::eTriangleList};
    const vk::Viewport viewport{
        0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f};
    const vk::Rect2D scissor{vk::Offset2D{0, 0}, vk::Extent2D{swapChainExtent.width, swapChainExtent.height}};
    vk::PipelineViewportStateCreateInfo viewportState{
        .viewportCount = 1, .pViewports = &viewport, .scissorCount = 1, .pScissors = &scissor};
    vk::PipelineRasterizationStateCreateInfo rasterizer{.depthClampEnable = vk::False,
                                                        .rasterizerDiscardEnable = vk::False,
                                                        .polygonMode = vk::PolygonMode::eFill,
                                                        .cullMode = vk::CullModeFlagBits::eBack,
                                                        .frontFace = vk::FrontFace::eCounterClockwise,
                                                        .depthBiasEnable = vk::False,
                                                        .depthBiasSlopeFactor = 1.0f,
                                                        .lineWidth = 1.0f};
    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = resourceManager->msaaSamples,
        .sampleShadingEnable = vk::True,
    };
    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = vk::False;

    vk::PipelineColorBlendStateCreateInfo colorBlending{.logicOpEnable = vk::False,
                                                        .logicOp = vk::LogicOp::eCopy,
                                                        .attachmentCount = 1,
                                                        .pAttachments = &colorBlendAttachment};
    vk::PushConstantRange pushDataRange{
        .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        .offset = 0,
        .size = static_cast<uint32_t>(sizeof(PushData)),
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    if (useDescriptorHeaps) {
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushDataRange;
    } else {
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &*descriptorManager->descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
    }

    vk::PipelineDepthStencilStateCreateInfo depthStencil{.depthTestEnable = vk::True,
                                                         .depthWriteEnable = vk::True,
                                                         .depthCompareOp = vk::CompareOp::eLess,
                                                         .depthBoundsTestEnable = vk::False,
                                                         .stencilTestEnable = vk::False};
    vk::Format depthFormat = resourceManager->findDepthFormat();

    if (useDescriptorHeaps) {
        pipelineLayout = VK_NULL_HANDLE;
    } else {
        pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);
    }
    setDebugName(device, shaderModule, "ShaderModule_Main");
    setDebugName(device, pipelineLayout, "PipelineLayout_Main");
    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{.colorAttachmentCount = 1,
                                                                .pColorAttachmentFormats = &swapChainImageFormat,
                                                                .depthAttachmentFormat = depthFormat};

    vk::PipelineCreateFlags2CreateInfoKHR pipelineFlags2CreateInfo{
        .pNext = &pipelineRenderingCreateInfo,
        .flags = vk::PipelineCreateFlagBits2KHR::eDescriptorHeapEXT
    };
    const void* pipelinePNext = useDescriptorHeaps
        ? static_cast<const void*>(&pipelineFlags2CreateInfo)
        : static_cast<const void*>(&pipelineRenderingCreateInfo);


    vk::GraphicsPipelineCreateInfo pipelineInfo{
        .pNext               = pipelinePNext,
        .stageCount          = static_cast<uint32_t>(shaderStages.size()),
        .pStages             = shaderStages.data(),
        .pVertexInputState   = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState      = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState   = &multisampling,
        .pDepthStencilState  = &depthStencil,
        .pColorBlendState    = &colorBlending,
        .pDynamicState       = &dynamicState,
        .layout              = *pipelineLayout,
        .renderPass          = nullptr
    };

    graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
    setDebugName(device, graphicsPipeline, "GraphicsPipeline_Main");
}

#ifndef ENGINE_SHADER_DIR
#define ENGINE_SHADER_DIR "./shaders"
#endif
