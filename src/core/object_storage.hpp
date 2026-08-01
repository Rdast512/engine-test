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

    Object(uint32_t currentIndex, uint32_t indexCount, uint32_t createTextureImage)
        : firstIndex(currentIndex), indexCount(indexCount), textureIndex(createTextureImage) {}

    // Frees VMA uniform-buffer allocations via the stored allocator handle.
    ~Object();

    // --- Move semantics (required because vk::raii types are move-only) ---
    Object(Object&& other) noexcept = default;
    Object& operator=(Object&& other) noexcept = default;
    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;

    // --- Transform (mutable values: accessors) ---
    [[nodiscard]] const glm::vec3& getPosition() const noexcept { return position; }
    [[nodiscard]] const glm::vec3& getRotation() const noexcept { return rotation; }
    [[nodiscard]] const glm::vec3& getScale() const noexcept { return scale; }

    void setPosition(const glm::vec3& pos) noexcept { position = pos; }
    void setRotation(const glm::vec3& rot) noexcept { rotation = rot; }
    void setScale(const glm::vec3& scl) noexcept { scale = scl; }

    [[nodiscard]] glm::mat4 getModelMatrix() const;

    // --- GPU resources (stable handles: direct access) ---
    std::string name;

    std::array<void*, 2> uniformBuffersMapped = {nullptr, nullptr};
    std::array<vk::raii::Buffer, 2> uniformBuffers = {nullptr, nullptr};
    std::array<VmaAllocation, 2> uniformBuffersMemory = {nullptr, nullptr};
    std::array<vk::DeviceAddress, 2> uboAddresses = {0, 0};
    glm::mat4 modelMatrix{};
    // VMA allocator handle — set by ResourceManager when uniform buffers are
    // allocated. Used by the destructor to free VMA-backed resources.
    VmaAllocator vmaAllocator = nullptr;
    // draw inderect will replace this data
    uint32_t firstIndex = 0; // Offset into the global index buffer
    uint32_t indexCount = 0; // How many indices to draw

    uint32_t textureIndex = 0;

private:
    glm::vec3 position{1.0f, 1.0f, 3.0f};
    glm::vec3 rotation{0.0f};
    glm::vec3 scale{1.0f};
};
