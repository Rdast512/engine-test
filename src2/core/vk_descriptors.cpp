#include "vk_descriptors.hpp"
#include "../core/RenderTypes.h"
#include "../core/Constants.h"

DescriptorManager::DescriptorManager(const vk::raii::Device &device,
                                     const std::vector<vk::raii::Buffer>& uniformBuffers,
                                     const vk::raii::Sampler &textureSampler,
                                     const vk::raii::ImageView &textureImageView)
    : device(device),
      uniformBuffers(uniformBuffers),
      textureSampler(textureSampler),
      textureImageView(textureImageView) {
    // ...constructor now only assigns members (no function calls)...
}

// New: perform descriptor-related initialization after construction
void DescriptorManager::init() {
    createDescriptorSetLayout();
    createDescriptorPool();
    createDescriptorSets();
}

void DescriptorManager::createDescriptorSetLayout() {
    std::array bindings = {
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex,
                                       nullptr),
        vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1,
                                       vk::ShaderStageFlagBits::eFragment, nullptr)
    };

    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = static_cast<uint32_t>(bindings.size()), .pBindings = bindings.data()
    };
    descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
}

void DescriptorManager::createDescriptorPool() {
    std::array poolSize{
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT),
        vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT)
    };
    vk::DescriptorPoolCreateInfo poolInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = static_cast<uint32_t>(poolSize.size()),
        .pPoolSizes = poolSize.data()
    };
    descriptorPool = vk::raii::DescriptorPool(device, poolInfo);
}


void DescriptorManager::createDescriptorSets() {
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()
    };

    descriptorSets.clear();
    descriptorSets = device.allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo bufferInfo{
            .buffer = uniformBuffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject)
        };
        vk::DescriptorImageInfo imageInfo{
            .sampler = textureSampler,
            .imageView = textureImageView,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        };
        std::array descriptorWrites{
            vk::WriteDescriptorSet{
                .dstSet = descriptorSets[i],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .pBufferInfo = &bufferInfo
            },
            vk::WriteDescriptorSet{
                .dstSet = descriptorSets[i],
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo = &imageInfo
            }
        };
        device.updateDescriptorSets(descriptorWrites, {});
    }
}
