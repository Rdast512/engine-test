#pragma once

#include "types.hpp"

#include <vector>

// ---------------------------------------------------------------------------
// Object - a renderable entity with its own transform and per-frame uniform
//          buffer resources.  Follows RAII: the destructor releases all GPU
//          allocations automatically through vk::raii wrappers.
// ---------------------------------------------------------------------------
class Object
{
public:
    Object() = default;

    // All GPU resources are released automatically (reverse declaration order).
    ~Object() = default;

    // --- Move semantics (required because vk::raii types are move-only) ---
    Object(Object&& other) noexcept = default;
    Object& operator=(Object&& other) noexcept = default;

    // Copying is not meaningful for GPU resources.
    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;

    // --- Transform accessors ---
    [[nodiscard]] const glm::vec3& getPosition() const noexcept { return position; }
    [[nodiscard]] const glm::vec3& getRotation() const noexcept { return rotation; }
    [[nodiscard]] const glm::vec3& getScale() const noexcept { return scale; }

    void setPosition(const glm::vec3& pos) noexcept { position = pos; }
    void setRotation(const glm::vec3& rot) noexcept { rotation = rot; }
    void setScale(const glm::vec3& scl) noexcept { scale = scl; }

    [[nodiscard]] glm::mat4 getModelMatrix() const;

    // --- GPU resource accessors (read-only) ---
    [[nodiscard]] const std::vector<vk::raii::Buffer>& getUniformBuffers() const noexcept { return uniformBuffers; }
    [[nodiscard]] const std::vector<VmaAllocation>& getUniformBuffersMemory() const noexcept
    {
        return uniformBuffersMemory;
    }
    [[nodiscard]] const std::vector<void*>& getUniformBuffersMapped() const noexcept { return uniformBuffersMapped; }
    [[nodiscard]] const std::vector<vk::DeviceAddress>& getUboAddresses() const noexcept { return uboAddresses; }

    // --- Ownership transfer: used during initialisation to hand GPU buffers
    //     to the Object.  After this call the Object owns all resources. ---
    // void setUniformBuffers(std::vector<vk::raii::Buffer> buffers, std::vector<vk::raii::DeviceMemory> memory,
    //                        std::vector<void*> mapped, std::vector<vk::DeviceAddress> ubo_addresses) noexcept;
    // --- Transform ---
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};
    glm::vec3 scale{1.0f};

    // vk::DeviceAddress vertexAdress;


    // --- GPU resources (owned) ---
    // Destruction order (reverse declaration) matters:
    //   1. uniformBuffersMapped  (no-op for void*)
    //   2. uniformBuffers        (vkDestroyBuffer)
    //   3. uniformBuffersMemory  (vkFreeMemory, which implicitly unmaps)
    std::string name;


    std::vector<void*> uniformBuffersMapped;
    std::vector<vk::raii::Buffer> uniformBuffers;
    std::vector<VmaAllocation> uniformBuffersMemory;

    std::vector<vk::DeviceAddress> uboAddresses;

private:

};
