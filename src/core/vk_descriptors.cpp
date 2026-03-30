#include "vk_descriptors.hpp"

#include "../Constants.h"
#include "../core/types.hpp"
#include "../util/debug.hpp"
#include "../util/vk_tracy.hpp"
#include "_deps/debug/ktxsoftware-src/external/basisu/transcoder/basisu_containers.h"
#include "logger.hpp"
#include "vk_allocator.hpp"
#include "vk_resource_manager.hpp"

constexpr vk::DeviceSize alignUp(vk::DeviceSize size, vk::DeviceSize alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}

DescriptorManager::DescriptorManager(const vk::raii::Device& device, ResourceManager& resourceManager,
                                     const std::vector<vk::raii::Buffer>& uniformBuffers,
                                     const vk::raii::Sampler& textureSampler,
                                     const vk::raii::ImageView& textureImageView,
                                     const vk::ImageViewCreateInfo& textureImageViewCreateInfo,
                                     const HardwareCapabilities& capabilities) :
    device(device), resourceManager(resourceManager), uniformBuffers(uniformBuffers), textureSampler(textureSampler),
    textureImageView(textureImageView), textureImageViewCreateInfo(textureImageViewCreateInfo),
    capabilities(capabilities)
{
    ZoneScopedN("DescriptorManager::DescriptorManager");
    // hardcoded for now since only one copy of things
    vk::DeviceSize resourceHeapSize = 0;
    resourceHeapSize = alignUp(resourceHeapSize, bufferDescriptorAlignment);
    resourceHeapSize += 2 * bufferDescriptorSize;
    resourceHeapSize = alignUp(resourceHeapSize, imageDescriptorAlignment);
    resourceHeapSize += 1 * imageDescriptorSize;
    resourceHeapSize += minResourceHeapReservedRange;

    log_info(std::format("Creating resource descriptor heap: size={} bufferDesc={} imageDesc={} reserve={}",
                         resourceHeapSize, bufferDescriptorSize, imageDescriptorSize, minResourceHeapReservedRange));

    resourceManager.createBuffer(resourceHeapSize,
                                 vk::BufferUsageFlagBits::eResourceDescriptorBufferEXT |
                                     vk::BufferUsageFlagBits::eStorageBuffer |
                                     vk::BufferUsageFlagBits::eShaderDeviceAddress
                                     | vk::BufferUsageFlagBits::eDescriptorHeapEXT,
                                 vk::MemoryPropertyFlagBits::eHostVisible |
                                 vk::MemoryPropertyFlagBits::eHostCoherent,
                                 resourceHeapBuffer, resourceHeapMemory);
    const VkResult resourceHeapMapResult =
        vmaMapMemory(resourceManager.allocator.allocator, resourceHeapMemory, &mappedResourceHeapPtr);
    if (resourceHeapMapResult != VK_SUCCESS || mappedResourceHeapPtr == nullptr) {
        throw std::runtime_error(std::format("Failed to map resource descriptor heap memory (VkResult={})",
                                             static_cast<int>(resourceHeapMapResult)));
    }
    setDebugName(device, resourceHeapBuffer, "DescriptorHeap_Resource");

    VkBufferDeviceAddressInfo resourceHeapAddressInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext = nullptr,
        .buffer = *resourceHeapBuffer,
    };
    VkDeviceAddress resourceHeapAddress = vkGetBufferDeviceAddress(*device, &resourceHeapAddressInfo);
    resourceHeapInfo = vk::BindHeapInfoEXT{
        .sType = vk::StructureType::eBindHeapInfoEXT,
        .pNext = nullptr,
        .heapRange = vk::DeviceAddressRangeEXT{.address = resourceHeapAddress, .size = resourceHeapSize},
        .reservedRangeOffset = resourceHeapSize - minResourceHeapReservedRange,
        .reservedRangeSize = minResourceHeapReservedRange,
    };
    log_info(std::format("Resource heap GPU address=0x{:016x}, reservedOffset={}, reservedSize={}", resourceHeapAddress,
                         resourceHeapInfo.reservedRangeOffset, resourceHeapInfo.reservedRangeSize));

    vk::DeviceSize samplerHeapSize = 0;
    samplerHeapSize = alignUp(samplerHeapSize, samplerDescriptorAlignment);
    samplerHeapSize += 1 * samplerDescriptorSize;
    samplerHeapSize += minSamplerHeapReservedRange;

    log_info(std::format("Creating sampler descriptor heap: size={} samplerDesc={} reserve={}", samplerHeapSize,
                         samplerDescriptorSize, minSamplerHeapReservedRange));

    resourceManager.createBuffer(samplerHeapSize,
                                 vk::BufferUsageFlagBits::eSamplerDescriptorBufferEXT |
                                     vk::BufferUsageFlagBits::eStorageBuffer |
                                     vk::BufferUsageFlagBits::eShaderDeviceAddress
                                     | vk::BufferUsageFlagBits::eDescriptorHeapEXT,
                                 vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                 samplerHeapBuffer, samplerHeapMemory);
    const VkResult samplerHeapMapResult =
        vmaMapMemory(resourceManager.allocator.allocator, samplerHeapMemory, &mappedSamplerHeapPtr);
    if (samplerHeapMapResult != VK_SUCCESS || mappedSamplerHeapPtr == nullptr) {
        throw std::runtime_error(std::format("Failed to map sampler descriptor heap memory (VkResult={})",
                                             static_cast<int>(samplerHeapMapResult)));
    }
    setDebugName(device, samplerHeapBuffer, "DescriptorHeap_Sampler");

    VkBufferDeviceAddressInfo samplerHeapAddressInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext = nullptr,
        .buffer = *samplerHeapBuffer,
    };
    VkDeviceAddress samplerHeapAddress = vkGetBufferDeviceAddress(*device, &samplerHeapAddressInfo);
    samplerHeapInfo = vk::BindHeapInfoEXT{
        .sType = vk::StructureType::eBindHeapInfoEXT,
        .pNext = nullptr,
        .heapRange = vk::DeviceAddressRangeEXT{.address = samplerHeapAddress, .size = samplerHeapSize},
        .reservedRangeOffset = samplerHeapSize - minSamplerHeapReservedRange,
        .reservedRangeSize = minSamplerHeapReservedRange,
    };
    log_info(std::format("Sampler heap GPU address=0x{:016x}, reservedOffset={}, reservedSize={}", samplerHeapAddress,
                         samplerHeapInfo.reservedRangeOffset, samplerHeapInfo.reservedRangeSize));
}

// New: perform descriptor-related initialization after construction
void DescriptorManager::init()
{
    ZoneScopedN("DescriptorManager::init");
    createHeaps();

    // createDescriptorSetLayout();
    // createDescriptorPool();
    // createDescriptorSets();
}


void DescriptorManager::createHeaps()
{
    ZoneScopedN("DescriptorManager::createHeaps");
    // new extension usage
    auto resources = std::vector<vk::ResourceDescriptorInfoEXT>();
    auto descriptors = std::vector<vk::HostAddressRangeEXT>();
    uboDescriptorOffsets.clear();
    uboDescriptorOffsets.reserve(MAX_FRAMES_IN_FLIGHT);

    vk::DeviceSize currentResOffset = 0;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        ZoneScopedN("DescriptorManager::createHeaps::UniformBufferDescriptor");
        currentResOffset = alignUp(currentResOffset, bufferDescriptorAlignment);

        VkBufferDeviceAddressInfo bufferAddressInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                                                       .buffer = *uniformBuffers[i]};
        VkDeviceAddress uboBaseAddress = vkGetBufferDeviceAddress(*device, &bufferAddressInfo);
        log_info(std::format("Uniform buffer device address: 0x{:016x}", uboBaseAddress));
        vk::DeviceAddressRangeEXT uboAddressRange = {.address = uboBaseAddress, .size = sizeof(UniformBufferObject)};
        auto uboInfo = vk::ResourceDescriptorInfoEXT{
            .sType = vk::StructureType::eResourceDescriptorInfoEXT,
            .pNext = nullptr,
            .type = vk::DescriptorType::eUniformBuffer,
            .data = vk::ResourceDescriptorDataEXT{&uboAddressRange},
        };
        uboDescriptorOffsets.push_back(currentResOffset);
        auto uboWrite = vk::HostAddressRangeEXT{
            .address = static_cast<uint8_t*>(mappedResourceHeapPtr) + currentResOffset, .size = bufferDescriptorSize};
        log_info(std::format("UBO descriptor[{}] heapOffset={} writeSize={}", i, currentResOffset, uboWrite.size));
        resources.push_back(uboInfo);
        descriptors.push_back(uboWrite);
        currentResOffset += bufferDescriptorSize;
    }

    currentResOffset = alignUp(currentResOffset, imageDescriptorAlignment);
    textureDescriptorOffset = currentResOffset;
    auto descriptorImageInfo = vk::ImageDescriptorInfoEXT{.sType = vk::StructureType::eImageDescriptorInfoEXT,
                                                          .pNext = nullptr,
                                                          .pView = &textureImageViewCreateInfo,
                                                          .layout = vk::ImageLayout::eShaderReadOnlyOptimal};
    auto imageInfo = vk::ResourceDescriptorInfoEXT{
        .sType = vk::StructureType::eResourceDescriptorInfoEXT,
        .pNext = nullptr,
        .type = vk::DescriptorType::eSampledImage,
        .data = vk::ResourceDescriptorDataEXT{&descriptorImageInfo},
    };
    auto imageWrite = vk::HostAddressRangeEXT{
        .address = static_cast<uint8_t*>(mappedResourceHeapPtr) + currentResOffset, .size = imageDescriptorSize};
    log_info(std::format("Texture descriptor heapOffset={} writeSize={}", textureDescriptorOffset, imageWrite.size));
    resources.push_back(imageInfo);
    descriptors.push_back(imageWrite);

    {
        ZoneScopedN("DescriptorManager::createHeaps::writeResourceDescriptorsEXT");
        log_info(std::format("Writing {} resource descriptors", resources.size()));
        device.writeResourceDescriptorsEXT(resources, descriptors);
    }

    vk::DeviceSize currentSampOffset = 0;

    currentSampOffset = alignUp(currentSampOffset, samplerDescriptorAlignment);
    samplerDescriptorOffset = currentSampOffset;

    const auto maxSamplerAnisotropy = capabilities.properties2.properties.limits.maxSamplerAnisotropy;
    const auto mipLevels = textureImageViewCreateInfo.subresourceRange.levelCount;

    vk::SamplerCreateInfo samplerInfo{.magFilter = vk::Filter::eLinear,
                                      .minFilter = vk::Filter::eLinear,
                                      .mipmapMode = vk::SamplerMipmapMode::eLinear,
                                      .addressModeU = vk::SamplerAddressMode::eRepeat,
                                      .addressModeV = vk::SamplerAddressMode::eRepeat,
                                      .addressModeW = vk::SamplerAddressMode::eRepeat,
                                      .mipLodBias = 0.0f,
                                      .anisotropyEnable = vk::True,
                                      .maxAnisotropy = maxSamplerAnisotropy,
                                      .compareEnable = vk::False,
                                      .compareOp = vk::CompareOp::eAlways,
                                      .minLod = 0.0f,
                                      .maxLod = static_cast<float>(mipLevels)};
    vk::HostAddressRangeEXT samplerWrite = {.address = static_cast<uint8_t*>(mappedSamplerHeapPtr) + currentSampOffset,
                                            .size = samplerDescriptorSize};
    {
        ZoneScopedN("DescriptorManager::createHeaps::writeSamplerDescriptorsEXT");
        log_info(std::format("Writing sampler descriptor heapOffset={} writeSize={} mipLevels={} maxAniso={}",
                             samplerDescriptorOffset, samplerWrite.size, mipLevels, maxSamplerAnisotropy));
        device.writeSamplerDescriptorsEXT(samplerInfo, samplerWrite);
    }

    log_info(std::format("Descriptor heaps ready: uboCount={} textureOffset={} samplerOffset={}",
                         uboDescriptorOffsets.size(), textureDescriptorOffset, samplerDescriptorOffset));
    // end
}

// void DescriptorManager::createDescriptorSetLayout()
// {
//     ZoneScopedN("DescriptorManager::createDescriptorSetLayout");
//     std::array bindings = {vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1,
//                                                           vk::ShaderStageFlagBits::eVertex, nullptr),
//                            vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1,
//                                                           vk::ShaderStageFlagBits::eFragment, nullptr)};
//
//     vk::DescriptorSetLayoutCreateInfo layoutInfo{.bindingCount = static_cast<uint32_t>(bindings.size()),
//                                                  .pBindings = bindings.data()};
//     descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
//     setDebugName(device, descriptorSetLayout, "DescriptorSetLayout");
// }

// void DescriptorManager::createDescriptorPool()
// {
//     ZoneScopedN("DescriptorManager::createDescriptorPool");
//     std::array poolSize{vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT),
//                         vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT)};
//     vk::DescriptorPoolCreateInfo poolInfo{.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
//                                           .maxSets = MAX_FRAMES_IN_FLIGHT,
//                                           .poolSizeCount = static_cast<uint32_t>(poolSize.size()),
//                                           .pPoolSizes = poolSize.data()};
//     descriptorPool = vk::raii::DescriptorPool(device, poolInfo);
//     setDebugName(device, descriptorPool, "DescriptorPool");
// }


void DescriptorManager::createDescriptorSets()
{
    ZoneScopedN("DescriptorManager::createDescriptorSets");


    // for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    // {
    //     ZoneScopedN("DescriptorManager::writeDescriptorSet");
    //     const std::string setName = "DescriptorSet_" + std::to_string(i);
    //     setDebugName(device, descriptorSets[i], setName);
    //     vk::DescriptorBufferInfo bufferInfo{
    //         .buffer = uniformBuffers[i], .offset = 0, .range = sizeof(UniformBufferObject)};
    //     vk::DescriptorImageInfo imageInfo{.sampler = textureSampler,
    //                                       .imageView = textureImageView,
    //                                       .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};
    //     std::array descriptorWrites{vk::WriteDescriptorSet{.dstSet = descriptorSets[i],
    //                                                        .dstBinding = 0,
    //                                                        .dstArrayElement = 0,
    //                                                        .descriptorCount = 1,
    //                                                        .descriptorType = vk::DescriptorType::eUniformBuffer,
    //                                                        .pBufferInfo = &bufferInfo},
    //                                 vk::WriteDescriptorSet{.dstSet = descriptorSets[i],
    //                                                        .dstBinding = 1,
    //                                                        .dstArrayElement = 0,
    //                                                        .descriptorCount = 1,
    //                                                        .descriptorType =
    //                                                        vk::DescriptorType::eCombinedImageSampler, .pImageInfo =
    //                                                        &imageInfo}};
    //     device.updateDescriptorSets(descriptorWrites, {});
    // }
}
