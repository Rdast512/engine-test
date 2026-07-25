#include "../core/object_storage.hpp"

#include "logger.hpp"


// ============================================================================
// Object
// ============================================================================

glm::mat4 Object::getModelMatrix() const
{
    glm::mat4 model{1.0f};
    model = glm::translate(model, position);
    model = glm::rotate(model, rotation.x, glm::vec3{1.0f, 0.0f, 0.0f});
    model = glm::rotate(model, rotation.y, glm::vec3{0.0f, 1.0f, 0.0f});
    model = glm::rotate(model, rotation.z, glm::vec3{0.0f, 0.0f, 1.0f});
    model = glm::scale(model, scale);
    return model;
}

//
// Object::Object(Object&& other) noexcept
//     : position(other.position),
//       rotation(other.rotation),
//       scale(other.scale),
//       name(std::move(other.name)),
//       uniformBuffersMapped(other.uniformBuffersMapped),
//       uniformBuffers(std::move(other.uniformBuffers)),
//       uniformBuffersMemory(other.uniformBuffersMemory),
//       uboAddresses(other.uboAddresses),
//       vmaAllocator(other.vmaAllocator),
//       firstIndex(other.firstIndex),
//       indexCount(other.indexCount),
//       textureIndex(other.textureIndex)
// {
//     other.uniformBuffersMapped = {nullptr, nullptr};
//     other.uniformBuffersMemory = {nullptr, nullptr};
//     other.uboAddresses = {0, 0};
//     other.vmaAllocator = nullptr;
// }
//
// Object& Object::operator=(Object&& other) noexcept
// {
//     if (this == &other)
//     {
//         return *this;
//     }
//
//     // Free currently owned GPU resources, then take ownership.
//     for (uint32_t i = 0; i < uniformBuffersMemory.size(); ++i)
//     {
//         VkBuffer rawBuf = VK_NULL_HANDLE;
//         if (*uniformBuffers[i] != VK_NULL_HANDLE)
//         {
//             rawBuf = uniformBuffers[i].release();
//         }
//         if (vmaAllocator != nullptr && uniformBuffersMemory[i] != nullptr)
//         {
//             if (uniformBuffersMapped[i] != nullptr)
//             {
//                 vmaUnmapMemory(vmaAllocator, uniformBuffersMemory[i]);
//             }
//             vmaDestroyBuffer(vmaAllocator, rawBuf, uniformBuffersMemory[i]);
//         }
//         uniformBuffersMapped[i] = nullptr;
//         uniformBuffersMemory[i] = nullptr;
//         uboAddresses[i] = 0;
//     }
//     vmaAllocator = nullptr;
//
//     position = other.position;
//     rotation = other.rotation;
//     scale = other.scale;
//     name = std::move(other.name);
//     uniformBuffersMapped = other.uniformBuffersMapped;
//     uniformBuffers = std::move(other.uniformBuffers);
//     uniformBuffersMemory = other.uniformBuffersMemory;
//     uboAddresses = other.uboAddresses;
//     vmaAllocator = other.vmaAllocator;
//     firstIndex = other.firstIndex;
//     indexCount = other.indexCount;
//     textureIndex = other.textureIndex;
//
//     other.uniformBuffersMapped = {nullptr, nullptr};
//     other.uniformBuffersMemory = {nullptr, nullptr};
//     other.uboAddresses = {0, 0};
//     other.vmaAllocator = nullptr;
//     return *this;
// }

Object::~Object()
{
    for (uint32_t i = 0; i < uniformBuffersMemory.size(); ++i)
    {
        // 1. Release every vk::raii::Buffer wrapper unconditionally.
        //    This nulls the internal dispatcher pointer so the wrapper
        //    destructor never touches the Device after device.reset().
        //    Must happen BEFORE any VMA teardown that may fail early.
        VkBuffer rawBuf = VK_NULL_HANDLE;
        if (*uniformBuffers[i] != VK_NULL_HANDLE)
        {
            rawBuf = uniformBuffers[i].release();
        }

        if (vmaAllocator == nullptr) continue;
        if (uniformBuffersMemory[i] == nullptr) continue;

        // 2. Unmap — VMA requires m_MapCount == 0 before destroy.
        if (uniformBuffersMapped[i] != nullptr)
        {
            vmaUnmapMemory(vmaAllocator, uniformBuffersMemory[i]);
            uniformBuffersMapped[i] = nullptr;
        }

        // 3. Single vmaDestroyBuffer call — frees both the Vulkan buffer
        //    and the VMA allocation.
        vmaDestroyBuffer(vmaAllocator, rawBuf, uniformBuffersMemory[i]);
        uniformBuffersMemory[i] = nullptr;
    }
}

