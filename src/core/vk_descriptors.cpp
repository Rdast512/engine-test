#include "vk_descriptors.hpp"
#include "../Constants.h"
#include "../core/types.hpp"
#include "../util/debug.hpp"


DescriptorManager::DescriptorManager(const vk::raii::Device& device,
                                     const std::vector<vk::raii::Buffer>& uniformBuffers,
                                     const vk::raii::Sampler& textureSampler,
                                     const vk::raii::ImageView& textureImageView,
                                     const vk::ImageViewCreateInfo& textureImageViewCreateInfo) :
    device(device), uniformBuffers(uniformBuffers), textureSampler(textureSampler), textureImageView(textureImageView), textureImageViewCreateInfo(textureImageViewCreateInfo)
{
    // ...constructor now only assigns members (no function calls)...
}

// New: perform descriptor-related initialization after construction
void DescriptorManager::init()
{
    createDescriptorSetLayout();
    createDescriptorPool();
    createDescriptorSets();
}

void DescriptorManager::createDescriptorSetLayout()
{
    std::array bindings = {vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1,
                                                          vk::ShaderStageFlagBits::eVertex, nullptr),
                           vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1,
                                                          vk::ShaderStageFlagBits::eFragment, nullptr)};

    vk::DescriptorSetLayoutCreateInfo layoutInfo{.bindingCount = static_cast<uint32_t>(bindings.size()),
                                                 .pBindings = bindings.data()};
    descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
    setDebugName(device, descriptorSetLayout, "DescriptorSetLayout");
}

void DescriptorManager::createDescriptorPool()
{
    std::array poolSize{vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT),
                        vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT)};
    vk::DescriptorPoolCreateInfo poolInfo{.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                                          .maxSets = MAX_FRAMES_IN_FLIGHT,
                                          .poolSizeCount = static_cast<uint32_t>(poolSize.size()),
                                          .pPoolSizes = poolSize.data()};
    descriptorPool = vk::raii::DescriptorPool(device, poolInfo);
    setDebugName(device, descriptorPool, "DescriptorPool");
}


void DescriptorManager::createDescriptorSets()
{
    // new extension usage
    // VkBufferDeviceAddressInfo bufferAddressInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = *uniformBuffers[0]};
    // VkDeviceAddress uboBaseAddress = vkGetBufferDeviceAddress(*device, &bufferAddressInfo);
    // vk::DeviceAddressRangeEXT uboAddressRange = {.address = uboBaseAddress, .size = sizeof(UniformBufferObject)};
    // auto bufferInfo = vk::ResourceDescriptorInfoEXT {
    // .sType = vk::StructureType::eResourceDescriptorInfoEXT,
    // .pNext = nullptr,
    //     .type = vk::DescriptorType::eUniformBuffer,
    //     .data = vk::ResourceDescriptorDataEXT{&uboAddressRange},
    // };
    // auto descriptorImageInfo = vk::ImageDescriptorInfoEXT{
    // .sType = vk::StructureType::eImageDescriptorInfoEXT,
    //     .pNext = nullptr,
    //     .pView = &textureImageViewCreateInfo,
    //     .layout = vk::ImageLayout::eShaderReadOnlyOptimal
    // };
    // auto imageInfo = vk::ResourceDescriptorInfoEXT {
    // .sType = vk::StructureType::eResourceDescriptorInfoEXT,
    // .pNext = nullptr,
    //     .type = vk::DescriptorType::eSampledImage,
    //     .data = vk::ResourceDescriptorDataEXT{&descriptorImageInfo},
    // };


    // end
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo{.descriptorPool = descriptorPool,
                                            .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
                                            .pSetLayouts = layouts.data()};

    descriptorSets.clear();
    descriptorSets = device.allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        const std::string setName = "DescriptorSet_" + std::to_string(i);
        setDebugName(device, descriptorSets[i], setName);
        vk::DescriptorBufferInfo bufferInfo{
            .buffer = uniformBuffers[i], .offset = 0, .range = sizeof(UniformBufferObject)};
        vk::DescriptorImageInfo imageInfo{.sampler = textureSampler,
                                          .imageView = textureImageView,
                                          .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};
        std::array descriptorWrites{vk::WriteDescriptorSet{.dstSet = descriptorSets[i],
                                                           .dstBinding = 0,
                                                           .dstArrayElement = 0,
                                                           .descriptorCount = 1,
                                                           .descriptorType = vk::DescriptorType::eUniformBuffer,
                                                           .pBufferInfo = &bufferInfo},
                                    vk::WriteDescriptorSet{.dstSet = descriptorSets[i],
                                                           .dstBinding = 1,
                                                           .dstArrayElement = 0,
                                                           .descriptorCount = 1,
                                                           .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                                                           .pImageInfo = &imageInfo}};
        device.updateDescriptorSets(descriptorWrites, {});
    }
}
