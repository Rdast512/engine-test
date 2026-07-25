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

    Object(uint32_t current_index, uint32_t index_count, uint32_t create_texture_image)
        : firstIndex(current_index), indexCount(index_count), textureIndex(create_texture_image) {}

    // Frees VMA uniform-buffer allocations via the stored allocator handle.
    ~Object();

    // --- Move semantics (required because vk::raii types are move-only) ---
    // Custom move: VmaAllocation/void* are trivial and would be shallow-copied by
    // =default, leaving the moved-from Object able to double-free in ~Object.
    // Object(Object&& other) noexcept;
    // Object& operator=(Object&& other) noexcept;
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
    // Cleanup order (handled by the destructor body):
    //   1. vk::raii::Buffer handles are released
    //   2. VMA allocations are freed via vmaDestroyBuffer
    //   3. Member destructors run (no-op for released/null handles)
    std::string name;

    std::array<void*, 2> uniformBuffersMapped = {nullptr, nullptr};
    std::array<vk::raii::Buffer, 2> uniformBuffers = {nullptr, nullptr};
    std::array<VmaAllocation, 2> uniformBuffersMemory = {nullptr, nullptr};
    std::array<vk::DeviceAddress, 2> uboAddresses = {0, 0};

    // VMA allocator handle — set by ResourceManager when uniform buffers are
    // allocated. Used by the destructor to free VMA-backed resources.
    VmaAllocator vmaAllocator = nullptr;
    // draw inderect will replace this data
    uint32_t firstIndex;          // Offset into the global index buffer
    uint32_t indexCount;          // How many indices to draw

    uint32_t textureIndex;

private:

};
