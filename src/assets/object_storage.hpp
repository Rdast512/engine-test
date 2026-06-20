#pragma once

#include "../core/types.hpp"

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
    [[nodiscard]] const std::vector<vk::raii::DeviceMemory>& getUniformBuffersMemory() const noexcept
    {
        return uniformBuffersMemory;
    }
    [[nodiscard]] const std::vector<void*>& getUniformBuffersMapped() const noexcept { return uniformBuffersMapped; }
    [[nodiscard]] vk::DeviceAddress getUboAddress() const noexcept { return uboAddress; }

    // --- Ownership transfer: used during initialisation to hand GPU buffers
    //     to the Object.  After this call the Object owns all resources. ---
    void setUniformBuffers(std::vector<vk::raii::Buffer> buffers, std::vector<vk::raii::DeviceMemory> memory,
                           std::vector<void*> mapped, vk::DeviceAddress ubo_addr) noexcept;

private:
    // --- Transform ---
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};
    glm::vec3 scale{1.0f};

    // --- GPU resources (owned) ---
    // Destruction order (reverse declaration) matters:
    //   1. uniformBuffersMapped_  (no-op for void*)
    //   2. uniformBuffers_        (vkDestroyBuffer)
    //   3. uniformBuffersMemory_  (vkFreeMemory, which implicitly unmaps)
    std::vector<void*> uniformBuffersMapped;
    std::vector<vk::raii::Buffer> uniformBuffers;
    std::vector<vk::raii::DeviceMemory> uniformBuffersMemory;

    vk::DeviceAddress uboAddress{};
};


// ---------------------------------------------------------------------------
// ObjectStorage - central vertex / index data store shared by all objects.
//                 Rule of Zero: std::vector members handle their own lifetime.
// it can have its own tree in ui for navigating
// ---------------------------------------------------------------------------
class ObjectStorage
{
public:
    ObjectStorage() = default;
    ~ObjectStorage() = default;

    // Movable, non-copyable.
    ObjectStorage(ObjectStorage&&) noexcept = default;
    ObjectStorage& operator=(ObjectStorage&&) noexcept = default;

    ObjectStorage(const ObjectStorage&) = delete;
    ObjectStorage& operator=(const ObjectStorage&) = delete;

    void clear() noexcept;
    [[nodiscard]] bool empty() const noexcept;

    // Replaces the entire model data in one shot (sink parameters — moved from).
    void setModelData(std::vector<Vertex> verts, std::vector<uint32_t> idx);

    [[nodiscard]] const std::vector<Vertex>& getVertices() const noexcept { return vertices; }
    [[nodiscard]] const std::vector<uint32_t>& getIndices() const noexcept { return indices; }

    [[nodiscard]] const std::vector<Object>& getObjects() const noexcept { return objects; }

private:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    std::vector<Object> objects;
};
