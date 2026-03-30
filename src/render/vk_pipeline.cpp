#include "vk_pipeline.hpp"
#include "../util/vk_tracy.hpp"

#include <array>
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

Pipeline::Pipeline(ResourceManager* resourceManager, const vk::raii::Device& device,
                   const vk::Extent2D& swapChainExtent, const vk::Format& swapChainImageFormat) :
    device(device), swapChainExtent(swapChainExtent), swapChainImageFormat(swapChainImageFormat),
    resourceManager(resourceManager)
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
    const auto shaderPath = (shaderDir / "shader.spv").string();
    vk::raii::ShaderModule shaderModule = resourceManager->createShaderModule(readFile(shaderPath));
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    auto code = readFile(shaderPath);
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ .pNext = nullptr,
        .stage = vk::ShaderStageFlagBits::eVertex, .module = shaderModule, .pName = "vertMain"};
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ .pNext = nullptr,
        .stage = vk::ShaderStageFlagBits::eFragment, .module = shaderModule, .pName = "fragMain"};
    vk::ShaderCreateInfoEXT fragShaderCreateInfo = {
        .flags      = vk::ShaderCreateFlagBitsEXT::eDescriptorHeap, // Tell the driver this shader uses the heap
        .stage      = vk::ShaderStageFlagBits::eFragment,
        .codeType   = vk::ShaderCodeTypeEXT::eSpirv,
        .codeSize   = code.size() * sizeof(char),
        .pCode      = reinterpret_cast<const uint32_t*>(code.data()),
        .pName      = "fragMain",
        // pNext is completely NULL or points to other modern extensions.
        // DO NOT attach vk::ShaderDescriptorSetAndBindingMappingInfoEXT here!
    };
    vk::ShaderCreateInfoEXT vertShaderCreateInfo = {
        .flags      = vk::ShaderCreateFlagBitsEXT::eDescriptorHeap, // Tell the driver this shader uses the heap
        .stage      = vk::ShaderStageFlagBits::eVertex,
        .codeType   = vk::ShaderCodeTypeEXT::eSpirv,
        .codeSize   = code.size() * sizeof(char),
        .pCode      = reinterpret_cast<const uint32_t*>(code.data()),
        .pName      = "vertMain",
        // pNext is completely NULL or points to other modern extensions.
        // DO NOT attach vk::ShaderDescriptorSetAndBindingMappingInfoEXT here!
    };
    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    std::vector dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

    vk::ShaderEXT fragShader = device.createShaderEXT(vertShaderCreateInfo);
    vk::ShaderEXT vertShader = device.createShaderEXT(fragShaderCreateInfo);
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
        .size = static_cast<uint32_t>(sizeof(uint32_t) * 3),
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
        .setLayoutCount = 0,         // No Descriptor Sets anymore!
        .pSetLayouts = nullptr,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushDataRange
    };

    vk::PipelineDepthStencilStateCreateInfo depthStencil{.depthTestEnable = vk::True,
                                                         .depthWriteEnable = vk::True,
                                                         .depthCompareOp = vk::CompareOp::eLess,
                                                         .depthBoundsTestEnable = vk::False,
                                                         .stencilTestEnable = vk::False};
    vk::Format depthFormat = resourceManager->findDepthFormat();


    pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);
    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{.colorAttachmentCount = 1,
                                                                .pColorAttachmentFormats = &swapChainImageFormat,
                                                                .depthAttachmentFormat = depthFormat};
    VkPipelineCreateFlags2CreateInfo pipelineFlags2CreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO,
        .pNext = &pipelineRenderingCreateInfo,
        .flags = VK_PIPELINE_CREATE_2_DESCRIPTOR_HEAP_BIT_EXT,
    };
    vk::GraphicsPipelineCreateInfo pipelineInfo{.pNext = &pipelineFlags2CreateInfo,
                                                .stageCount = 2,
                                                .pStages = shaderStages,
                                                .pVertexInputState = &vertexInputInfo,
                                                .pInputAssemblyState = &inputAssembly,
                                                .pViewportState = &viewportState,
                                                .pRasterizationState = &rasterizer,
                                                .pMultisampleState = &multisampling,
                                                .pDepthStencilState = &depthStencil,
                                                .pColorBlendState = &colorBlending,
                                                .pDynamicState = &dynamicState,
                                                .layout = *pipelineLayout,
                                                .renderPass = nullptr};
    graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
}

#ifndef ENGINE_SHADER_DIR
#define ENGINE_SHADER_DIR "./shaders"
#endif
