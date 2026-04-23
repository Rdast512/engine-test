#include "object_storage.hpp"


void ObjectStorage::setModelData(std::vector<Vertex> verticesIn, std::vector<uint32_t> indicesIn) {
    vertices = std::move(verticesIn);
    indices = std::move(indicesIn);
}

void ObjectStorage::clear() {
    vertices.clear();
    indices.clear();
}

bool ObjectStorage::empty() const {
    return vertices.empty() || indices.empty();
}
