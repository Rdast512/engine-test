#pragma once

#include "types.hpp"


// ---------------------------------------------------------------------------
// Object - a renderable entity with its own transform and per-frame uniform
//          buffer resources.  Follows RAII: the destructor releases all GPU
//          allocations automatically through vk::raii wrappers.
// ---------------------------------------------------------------------------
class Object
{
public:
    Object() = default;



    Object(uint32_t current_index, uint32_t index_count, uint32_t create_texture_image) : firstIndex(current_index), indexCount(index_count), textureIndex(create_texture_image) {}
    ;

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
    [[nodiscard]] const std::array<vk::raii::Buffer, 2>& getUniformBuffers() const noexcept { return uniformBuffers; }
    [[nodiscard]] const std::array<VmaAllocation, 2>& getUniformBuffersMemory() const noexcept
    {
        return uniformBuffersMemory;
    }
    [[nodiscard]] const std::array<void*, 2>& getUniformBuffersMapped() const noexcept { return uniformBuffersMapped; }
    [[nodiscard]] const std::array<vk::DeviceAddress, 2>& getUboAddresses() const noexcept { return uboAddresses; }

    // --- Ownership transfer: used during initialisation to hand GPU buffers
    //     to the Object.  After this call the Object owns all resources. ---
    // void setUniformBuffers(std::vector<vk::raii::Buffer> buffers, std::vector<vk::raii::DeviceMemory> memory,
    //                        std::vector<void*> mapped, std::vector<vk::DeviceAddress> ubo_addresses) noexcept;
    // --- Transform ---
    glm::vec3 position{1.0f, 1.0f, 3.0f};
    glm::vec3 rotation{0.0f};
    glm::vec3 scale{1.0f};

    // vk::DeviceAddress vertexAdress;


    // --- GPU resources (owned) ---
    // Destruction order (reverse declaration) matters:
    //   1. uniformBuffersMapped  (no-op for void*)
    //   2. uniformBuffers        (vkDestroyBuffer)
    //   3. uniformBuffersMemory  (vkFreeMemory, which implicitly unmaps)
    std::string name;

    std::array<void*, 2> uniformBuffersMapped = {nullptr, nullptr};
    std::array<vk::raii::Buffer, 2> uniformBuffers = {nullptr, nullptr};
    std::array<VmaAllocation, 2> uniformBuffersMemory = {nullptr, nullptr};
    std::array<vk::DeviceAddress, 2> uboAddresses = {0, 0};
    // draw inderect will replace this data
    uint32_t firstIndex;          // Offset into the global index buffer
    uint32_t indexCount;          // How many indices to draw

    uint32_t textureIndex;

private:

};
