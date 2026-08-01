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

Object::~Object()
{
    for (uint32_t i = 0; i < uniformBuffersMemory.size(); ++i)
    {
        // 1. Release every vk::raii::Buffer wrapper unconditionally.
        //    This nulls the internal dispatcher pointer so the wrapper
        //    destructor never touches the Device after device.reset().
        VkBuffer rawBuf = VK_NULL_HANDLE;
        if (*uniformBuffers[i] != VK_NULL_HANDLE)
        {
            rawBuf = uniformBuffers[i].release();
        }

        // If the buffer handle was already null (moved-from state
        // after a std::vector reallocation), the VMA allocation was
        // transferred to the new owner — skip teardown for this slot.
        if (rawBuf == VK_NULL_HANDLE) continue;
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

