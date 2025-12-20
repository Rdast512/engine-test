#include "vk_pipeline.hpp"
#include <filesystem>
#include <vulkan/vulkan.hpp>

#ifndef ENGINE_SHADER_DIR
#define ENGINE_SHADER_DIR "./shaders"
#endif

static std::vector<char> readFile(const std::string &filename) {
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

Pipeline::Pipeline(ResourceManager *resourceManager,
                   const vk::raii::Device &device,
                   const vk::Extent2D &swapChainExtent,
                   const vk::Format &swapChainImageFormat,
                   const vk::raii::DescriptorSetLayout &descriptorSetLayout) : device(device),
                                                                               swapChainExtent(swapChainExtent),
                                                                               swapChainImageFormat(swapChainImageFormat),
                                                                               descriptorSetLayout(descriptorSetLayout),
                                                                               resourceManager(resourceManager) {
    context = resourceManager->context;
}

void Pipeline::init() {
    // perform initialization that was previously in constructor
    createGraphicsPipeline();
}

    void Pipeline::createGraphicsPipeline() {
        const auto shaderDir = std::filesystem::path(ENGINE_SHADER_DIR);
        const auto shaderPath = (shaderDir / "shader.spv").string();
        vk::raii::ShaderModule shaderModule = resourceManager->createShaderModule(readFile(shaderPath));
        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();
        vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
            .stage = vk::ShaderStageFlagBits::eVertex,
            .module = shaderModule, .pName = "vertMain"
        };
        vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = shaderModule, .pName = "fragMain"
        };
        vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
        std::vector dynamicStates = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        };


        vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
            .vertexBindingDescriptionCount = 1, .pVertexBindingDescriptions = &bindingDescription,
            .vertexAttributeDescriptionCount = attributeDescriptions.size(),
            .pVertexAttributeDescriptions = attributeDescriptions.data()
        };
        vk::PipelineDynamicStateCreateInfo dynamicState{
            .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
            .pDynamicStates = dynamicStates.data()
        };
        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{.topology = vk::PrimitiveTopology::eTriangleList};
        const vk::Viewport viewport{
            0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f,
            1.0f
        };
        const vk::Rect2D scissor{vk::Offset2D{0, 0}, vk::Extent2D{swapChainExtent.width, swapChainExtent.height}};
        vk::PipelineViewportStateCreateInfo viewportState{
            .viewportCount = 1,
            .pViewports = &viewport,
            .scissorCount = 1,
            .pScissors = &scissor
        };
        vk::PipelineRasterizationStateCreateInfo rasterizer{
            .depthClampEnable = vk::False, .rasterizerDiscardEnable = vk::False,
            .polygonMode = vk::PolygonMode::eFill, .cullMode = vk::CullModeFlagBits::eBack,
            .frontFace = vk::FrontFace::eCounterClockwise, .depthBiasEnable = vk::False,
            .depthBiasSlopeFactor = 1.0f, .lineWidth = 1.0f
        };
        vk::PipelineMultisampleStateCreateInfo multisampling{
            .rasterizationSamples = context->msaaSamples, .sampleShadingEnable = vk::True,
        };
        vk::PipelineColorBlendAttachmentState colorBlendAttachment;
        colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR |
                                              vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
                                              vk::ColorComponentFlagBits::eA;
        colorBlendAttachment.blendEnable = vk::False;

        vk::PipelineColorBlendStateCreateInfo colorBlending{
            .logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy, .attachmentCount = 1,
            .pAttachments = &colorBlendAttachment
        };
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
            .setLayoutCount = 1,
            .pSetLayouts = &*descriptorSetLayout,
            .pushConstantRangeCount = 0
        };
        vk::PipelineDepthStencilStateCreateInfo depthStencil{
            .depthTestEnable = vk::True,
            .depthWriteEnable = vk::True,
            .depthCompareOp = vk::CompareOp::eLess,
            .depthBoundsTestEnable = vk::False,
            .stencilTestEnable = vk::False
        };
        vk::Format depthFormat = resourceManager->findDepthFormat();
        pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);
        vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{
            .colorAttachmentCount = 1,
            .pColorAttachmentFormats = &swapChainImageFormat,
            .depthAttachmentFormat = depthFormat
        };
        vk::GraphicsPipelineCreateInfo pipelineInfo{
            .pNext = &pipelineRenderingCreateInfo,
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
            .layout = pipelineLayout,
            .renderPass = nullptr
        };
        graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
    }
