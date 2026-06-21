#pragma once
#include "../core/types.hpp"
#include "object_storage.hpp"
#include "tiny_obj_loader.h"
// tiny_obj_loader.h is provided via the precompiled header


class AssetsLoader
{
public:
    explicit AssetsLoader();
    ~AssetsLoader() = default;

    void loadModel();

    [[nodiscard]] const std::vector<uint32_t>& getIndices() const { return indices; }
    [[nodiscard]] const std::vector<Vertex>& getVertices() const { return vertices; }

    void processVertexData(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes);
    void loadMaterials(const std::string& path, const std::vector<tinyobj::material_t>& materials);

private:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};
