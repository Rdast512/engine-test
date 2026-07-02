#include "vk_descriptors.hpp"

#include "../Constants.h"
#include "../core/types.hpp"
#include "../util/debug.hpp"
#include "../util/vk_tracy.hpp"
#include "../util/vk_utils.hpp"
#include "_deps/debug/ktxsoftware-src/external/basisu/transcoder/basisu_containers.h"
#include "logger.hpp"
#include "vk_allocator.hpp"
#include "vk_resource_manager.hpp"

static constexpr vk::DeviceSize alignUp(vk::DeviceSize size, vk::DeviceSize alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}

DescriptorManager::DescriptorManager(const vk::raii::Device& device, ResourceManager& resourceManager,
                                     const HardwareCapabilities& capabilities) :
    device(device), resourceManager(resourceManager), uniformBuffers(resourceManager.getUniformBuffers()),
    capabilities(capabilities)
{
    minResourceHeapReservedRange = capabilities.descriptorHeap.minResourceHeapReservedRange;
    minSamplerHeapReservedRange = capabilities.descriptorHeap.minSamplerHeapReservedRange;

    bufferDescriptorSize = capabilities.descriptorHeap.bufferDescriptorSize;
    samplerDescriptorSize = capabilities.descriptorHeap.samplerDescriptorSize;
    imageDescriptorSize = capabilities.descriptorHeap.imageDescriptorSize;

    bufferDescriptorAlignment = capabilities.descriptorHeap.bufferDescriptorAlignment;
    samplerDescriptorAlignment = capabilities.descriptorHeap.samplerDescriptorAlignment;
    imageDescriptorAlignment = capabilities.descriptorHeap.imageDescriptorAlignment;
}

DescriptorManager::~DescriptorManager()
{
    if (mappedResourceHeapPtr != nullptr && resourceHeapMemory != nullptr) {
        vmaUnmapMemory(resourceManager.allocator.allocator, resourceHeapMemory);
        mappedResourceHeapPtr = nullptr;
    }
    if (mappedSamplerHeapPtr != nullptr && samplerHeapMemory != nullptr) {
        vmaUnmapMemory(resourceManager.allocator.allocator, samplerHeapMemory);
        mappedSamplerHeapPtr = nullptr;
    }

    if (resourceHeapMemory != nullptr) {
        VkBuffer rawResourceHeap = resourceHeapBuffer.release();
        vmaDestroyBuffer(resourceManager.allocator.allocator, rawResourceHeap, resourceHeapMemory);
        resourceHeapMemory = nullptr;
    }
    if (samplerHeapMemory != nullptr) {
        VkBuffer rawSamplerHeap = samplerHeapBuffer.release();
        vmaDestroyBuffer(resourceManager.allocator.allocator, rawSamplerHeap, samplerHeapMemory);
        samplerHeapMemory = nullptr;
    }
}

// New: perform descriptor-related initialization after construction
void DescriptorManager::init()
{
    ZoneScopedN("DescriptorManager::init");
    createHeaps();

}


void DescriptorManager::createHeaps()
{
    ZoneScopedN("DescriptorManager::createHeaps");

    if (samplerDescriptorSize == 0 || imageDescriptorSize == 0 || samplerDescriptorAlignment == 0 ||
        imageDescriptorAlignment == 0) {
        throw std::runtime_error("Descriptor heap properties are invalid for heap mode");
    }

    const vk::DeviceSize resourceReservedOffsetAlignment = imageDescriptorAlignment;

    vk::DeviceSize resourceHeapSize = capabilities.descriptorHeap.maxResourceHeapSize;
    // resourceHeapSize = alignUp(resourceHeapSize, imageDescriptorAlignment);
    // resourceHeapSize += imageDescriptorSize;
    // resourceHeapSize = alignUp(resourceHeapSize, resourceReservedOffsetAlignment);
    // resourceHeapSize += minResourceHeapReservedRange;

    log_info(std::format("Creating resource descriptor heap: size={} imageDesc={} reserve={}", resourceHeapSize,
                         imageDescriptorSize, minResourceHeapReservedRange));

    vk::DeviceSize samplerHeapSize = capabilities.descriptorHeap.maxSamplerHeapSize;

    // samplerHeapSize = alignUp(samplerHeapSize, samplerDescriptorAlignment);
    // samplerHeapSize += samplerDescriptorSize;
    // samplerHeapSize = alignUp(samplerHeapSize, samplerDescriptorAlignment);
    // samplerHeapSize += minSamplerHeapReservedRange;

    log_info(std::format("Creating sampler descriptor heap: size={} samplerDesc={} reserve={}", samplerHeapSize,
                         samplerDescriptorSize, minSamplerHeapReservedRange));

    createHeapBuffers(resourceHeapSize, samplerHeapSize);
    // createHeapDescriptors();
}

auto DescriptorManager::writeImageDescriptor(const vk::ImageViewCreateInfo& imageViewCreateInfo)
    -> uint32_t
{
    auto resources = std::vector<vk::ResourceDescriptorInfoEXT>();
    auto descriptors = std::vector<vk::HostAddressRangeEXT>();

    const vk::DeviceSize currentResOffset = alignUp(textureDescriptorOffset, imageDescriptorAlignment);
    textureDescriptorOffset = currentResOffset;
    const auto descriptorImageInfo = vk::ImageDescriptorInfoEXT{
        .sType = vk::StructureType::eImageDescriptorInfoEXT,
        .pNext = nullptr,
        .pView = &imageViewCreateInfo,
        .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
    };
    const auto imageInfo = vk::ResourceDescriptorInfoEXT{
        .sType = vk::StructureType::eResourceDescriptorInfoEXT,
        .pNext = nullptr,
        .type = vk::DescriptorType::eSampledImage,
        .data = vk::ResourceDescriptorDataEXT{&descriptorImageInfo},
    };
    const auto imageWrite = vk::HostAddressRangeEXT{
        .address = static_cast<uint8_t*>(mappedResourceHeapPtr) + currentResOffset,
        .size = imageDescriptorSize,
    };
    resources.push_back(imageInfo);
    descriptors.push_back(imageWrite);

    {
        ZoneScopedN("DescriptorManager::createHeapDescriptors::writeResourceDescriptorsEXT");
        device.writeResourceDescriptorsEXT(resources, descriptors);
    }
    return static_cast<uint32_t>(currentResOffset / imageDescriptorAlignment);
}

void DescriptorManager::createHeapDescriptors()
{
    ZoneScopedN("DescriptorManager::createHeapDescriptors");

    const vk::DeviceSize currentSampOffset = alignUp(0, samplerDescriptorAlignment);
    samplerDescriptorOffset = currentSampOffset;

    const auto maxSamplerAnisotropy = capabilities.properties2.properties.limits.maxSamplerAnisotropy;
    const auto mipLevels = 1;

    const vk::SamplerCreateInfo samplerInfo{.magFilter = vk::Filter::eLinear,
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
    const vk::HostAddressRangeEXT samplerWrite{
        .address = static_cast<uint8_t*>(mappedSamplerHeapPtr) + currentSampOffset,
        .size = samplerDescriptorSize,
    };
    {
        ZoneScopedN("DescriptorManager::createHeapDescriptors::writeSamplerDescriptorsEXT");
        device.writeSamplerDescriptorsEXT(samplerInfo, samplerWrite);
    }

    log_info(std::format("[T1] Descriptor heap layout imageDescSize={} samplerDescSize={} imageAlign={} "
                         "samplerAlign={} textureIndex={} samplerIndex={}",
                         imageDescriptorSize, samplerDescriptorSize, imageDescriptorAlignment,
                         samplerDescriptorAlignment, getTextureDescriptorIndex(), getSamplerDescriptorIndex()));
}

void DescriptorManager::createHeapBuffers(vk::DeviceSize resourceHeapSize, vk::DeviceSize samplerHeapSize)
{
    createBuffer(
        resourceHeapSize,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress |
            vk::BufferUsageFlagBits::eDescriptorHeapEXT,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, resourceHeapBuffer,
        resourceHeapMemory, resourceManager.allocator.allocator, resourceManager.device,
        resourceManager.queueFamilyIndices, "DescriptorHeapResourceMemory",
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
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

    createBuffer(
        samplerHeapSize,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress |
            vk::BufferUsageFlagBits::eDescriptorHeapEXT,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, samplerHeapBuffer,
        samplerHeapMemory, resourceManager.allocator.allocator, resourceManager.device,
        resourceManager.queueFamilyIndices, "DescriptorHeapSamplerMemory",
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
    const VkResult samplerHeapMapResult =
        vmaMapMemory(resourceManager.allocator.allocator, samplerHeapMemory, &mappedSamplerHeapPtr);
    if (samplerHeapMapResult != VK_SUCCESS || mappedSamplerHeapPtr == nullptr) {
        throw std::runtime_error(std::format("Failed to map sampler descriptor heap memory (VkResult={})",
                                             static_cast<int>(samplerHeapMapResult)));
    }
    setDebugName(device, samplerHeapBuffer, "DescriptorHeap_Sampler");

    VkBufferDeviceAddressInfo const samplerHeapAddressInfo = {
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





// uint32_t DescriptorManager::getUboDescriptorIndex(uint32_t frameIndex) const
// {
//     if (frameIndex >= uboDescriptorOffsets.size() || bufferDescriptorSize == 0) {
//         return 0;
//     }
//     return static_cast<uint32_t>(uboDescriptorOffsets[frameIndex] / bufferDescriptorSize);
// }

uint32_t DescriptorManager::getTextureDescriptorIndex() const
{
    if (imageDescriptorSize == 0) {
        return 0;
    }
    return static_cast<uint32_t>(textureDescriptorOffset / imageDescriptorSize);
}

uint32_t DescriptorManager::getSamplerDescriptorIndex() const
{
    if (samplerDescriptorSize == 0) {
        return 0;
    }
    return static_cast<uint32_t>(samplerDescriptorOffset / samplerDescriptorSize);
}
