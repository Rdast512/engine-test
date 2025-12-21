#include "model_storage.hpp"

#include <utility>

void ModelStorage::setModelData(std::vector<Vertex> verticesIn, std::vector<uint32_t> indicesIn) {
    vertices = std::move(verticesIn);
    indices = std::move(indicesIn);
}

void ModelStorage::clear() {
    vertices.clear();
    indices.clear();
}

bool ModelStorage::empty() const {
    return vertices.empty() || indices.empty();
}
