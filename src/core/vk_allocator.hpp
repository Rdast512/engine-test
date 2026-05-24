#pragma once

#include "../util/debug.hpp"
#include "vk_device.hpp"

#include <atomic>
#include <format>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_set>


class VkAllocator
{
public:
    VmaAllocator allocator;
    const vk::raii::PhysicalDevice& physicalDevice;
    const vk::raii::Device& device;
    const vk::raii::Instance& instance;

    explicit VkAllocator(Device& deviceWrapper) :
        physicalDevice(deviceWrapper.getPhysicalDevice()), device(deviceWrapper.getDevice()),
        instance(deviceWrapper.getInstance())
    {

        // Get the dynamic dispatcher for proper Vulkan function access
        VmaVulkanFunctions vulkanFunctions = {};
        vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT |
            VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT | VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT |
            VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT | VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT |
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

    ~VkAllocator() { vmaDestroyAllocator(allocator); }


    void alocateBuffer(const vk::BufferCreateInfo& bufferInfo, const VmaAllocationCreateInfo& allocInfo,
                       vk::raii::Buffer& buffer, VmaAllocation& allocation,
                       std::string_view allocationDebugBaseName = "BufferMemory") const
    {
        VkBuffer rawBuffer{};
        if (vmaCreateBuffer(allocator, bufferInfo, &allocInfo, &rawBuffer, &allocation, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate buffer with VMA");
        }
        buffer = vk::raii::Buffer(device, rawBuffer);
        nameAllocation(allocation, allocationDebugBaseName);
    }

    void alocateImage(const vk::ImageCreateInfo& imageInfo, const VmaAllocationCreateInfo& allocInfo,
                      vk::raii::Image& image, VmaAllocation& allocation,
                      std::string_view allocationDebugBaseName = "ImageMemory") const
    {
        VkImage rawImage{};
        if (vmaCreateImage(allocator, imageInfo, &allocInfo, &rawImage, &allocation, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate image with VMA");
        }
        image = vk::raii::Image(device, rawImage);
        nameAllocation(allocation, allocationDebugBaseName);
    }

    // Non-copyable, non-movable: manages a unique VMA allocator handle.
    VkAllocator(const VkAllocator&) = delete;
    VkAllocator(VkAllocator&&) = delete;
    VkAllocator& operator=(const VkAllocator&) = delete;
    VkAllocator& operator=(VkAllocator&&) = delete;

private:
    void nameAllocation(VmaAllocation allocation, std::string_view baseName) const
    {
        if (allocation == nullptr) {
            return;
        }

        static std::atomic<uint64_t> allocationCounter = 0;
        const uint64_t id = allocationCounter.fetch_add(1, std::memory_order_relaxed);
        const std::string uniqueName = std::format("{}_{}", baseName.empty() ? "DeviceMemory" : baseName, id);

        vmaSetAllocationName(allocator, allocation, uniqueName.c_str());

        VmaAllocationInfo allocInfo{};
        vmaGetAllocationInfo(allocator, allocation, &allocInfo);
        if (allocInfo.deviceMemory != VK_NULL_HANDLE) {
            const uint64_t memoryHandle = reinterpret_cast<uint64_t>(allocInfo.deviceMemory);
            static std::mutex namedMemoryHandlesMutex;
            static std::unordered_set<uint64_t> namedMemoryHandles;

            bool shouldNameMemory = false;
            {
                const std::lock_guard<std::mutex> lock(namedMemoryHandlesMutex);
                shouldNameMemory = namedMemoryHandles.insert(memoryHandle).second;
            }

            if (shouldNameMemory) {
                const std::string memoryName = std::format("{}_DeviceMemory", uniqueName);
                setDebugName(device, vk::DeviceMemory(allocInfo.deviceMemory), memoryName);
            }
        }
    }
};
