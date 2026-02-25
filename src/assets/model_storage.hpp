#pragma once

#include "../core/types.hpp"

// Central storage for loaded 3D model data (vertices + indices).
class ModelStorage {
public:
    ModelStorage() = default;
    ~ModelStorage() = default;

    void setModelData(std::vector<Vertex> verticesIn, std::vector<uint32_t> indicesIn);
    void clear();
    [[nodiscard]] bool empty() const;

    [[nodiscard]] const std::vector<Vertex>& getVertices() const { return vertices; }
    [[nodiscard]] const std::vector<uint32_t>& getIndices() const { return indices; }

private:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};
