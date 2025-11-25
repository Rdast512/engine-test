#pragma once
#include "../core/Utils.h"
#include "../core/RenderTypes.h"


class AssetsLoader {
public:

    AssetsLoader();
    ~AssetsLoader() = default;

    void loadModel();

    const std::vector<uint32_t>& getIndices() const { return indices; }  // Reference, not copy
    const std::vector<Vertex>& getVertices() const { return vertices; }   // Reference, not copy

    void processVertexData(const tinyobj::attrib_t &attrib, const std::vector<tinyobj::shape_t> &shapes);
    void loadMaterials(const std::string &path, const std::vector<tinyobj::material_t> &materials);
private:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};
