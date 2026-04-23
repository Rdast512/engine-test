#pragma once

#include "../core/types.hpp"

struct Object
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);


    std::vector<vk::raii::Buffer> uniformBuffers;
    std::vector<vk::raii::DeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    vk::DeviceAddress uboAddress;

    glm::mat4 getModelMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);
        return model;
    }
};




// Central storage for loaded 3D model data (vertices + indices).
class ObjectStorage {
public:
    ObjectStorage() = default;
    ~ObjectStorage() = default;

    void setModelData(std::vector<Vertex> verticesIn, std::vector<uint32_t> indicesIn);
    void clear();
    [[nodiscard]] bool empty() const;

    [[nodiscard]] const std::vector<Vertex>& getVertices() const { return vertices; }
    [[nodiscard]] const std::vector<uint32_t>& getIndices() const { return indices; }

private:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};
