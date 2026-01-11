#pragma once

#include "pch.h"
#include "vk_device.hpp"
#include <vma/vk_mem_alloc.h>


class VkAllocator
{
public:

    VmaAllocator allocator;
    const vk::raii::PhysicalDevice &physicalDevice;
    const vk::raii::Device &device;
    const vk::raii::Instance &instance;

    VkAllocator(Device &deviceWrapper) : physicalDevice(deviceWrapper.getPhysicalDevice()), device(deviceWrapper.getDevice()), instance(deviceWrapper.getInstance()) {

        // Get the dynamic dispatcher for proper Vulkan function access
        VmaVulkanFunctions vulkanFunctions = {};
        vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.flags =
            VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT |
            VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT |
            VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT |
            VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT |
            VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT;
            
        allocatorInfo.pVulkanFunctions = &vulkanFunctions;
        allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_4;
        allocatorInfo.physicalDevice = *physicalDevice;
        allocatorInfo.device = *device;
        allocatorInfo.instance = *instance;

        if (vmaCreateAllocator(&allocatorInfo, &allocator) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create VMA allocator");
        }
    }

    ~VkAllocator() {
        vmaDestroyAllocator(allocator);
    }


    void alocateBuffer(const vk::BufferCreateInfo &bufferInfo, const VmaAllocationCreateInfo &allocInfo,
                       vk::raii::Buffer &buffer, VmaAllocation &allocation) const {
        VkBuffer rawBuffer{};
        if (vmaCreateBuffer(allocator, reinterpret_cast<const VkBufferCreateInfo*>(&bufferInfo), &allocInfo,
                            &rawBuffer, &allocation, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate buffer with VMA");
        }
        buffer = vk::raii::Buffer(device, rawBuffer);
    }

    void alocateImage(const vk::ImageCreateInfo &imageInfo, const VmaAllocationCreateInfo &allocInfo,
                      vk::raii::Image &image, VmaAllocation &allocation) const {
        VkImage rawImage{};
        if (vmaCreateImage(allocator, reinterpret_cast<const VkImageCreateInfo*>(&imageInfo), &allocInfo,
                           &rawImage, &allocation, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate image with VMA");
        }
        image = vk::raii::Image(device, rawImage);
    }

    // Delete copy constructor and assignment operator to enforce singleton property
    VkAllocator(const VkAllocator&) = delete;
    VkAllocator& operator=(const VkAllocator&) = delete;
};