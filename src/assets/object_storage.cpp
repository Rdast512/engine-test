#include "object_storage.hpp"

#include <glm/gtc/matrix_transform.hpp>

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

void Object::setUniformBuffers(std::vector<vk::raii::Buffer> buffers, std::vector<vk::raii::DeviceMemory> memory,
                               std::vector<void*> mapped, vk::DeviceAddress ubo_addr) noexcept
{
    uniformBuffers = std::move(buffers);
    uniformBuffersMemory = std::move(memory);
    uniformBuffersMapped = std::move(mapped);
    uboAddress = ubo_addr;
}

// ============================================================================
// ObjectStorage
// ============================================================================

void ObjectStorage::clear() noexcept
{
    vertices.clear();
    indices.clear();
}

bool ObjectStorage::empty() const noexcept { return vertices.empty() && indices.empty(); }

void ObjectStorage::setModelData(std::vector<Vertex> verts, std::vector<uint32_t> idx)
{
    vertices = std::move(verts);
    indices = std::move(idx);
}
