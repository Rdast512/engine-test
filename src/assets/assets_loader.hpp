#pragma once
#include "../core/types.hpp"
#include "model_storage.hpp"
#include "tiny_obj_loader.h"


class AssetsLoader {
public:

    explicit AssetsLoader(ModelStorage &storage);
    ~AssetsLoader() = default;

    void loadModel();

    [[nodiscard]] const std::vector<uint32_t>& getIndices() const { return modelStorage.getIndices(); }
    [[nodiscard]] const std::vector<Vertex>& getVertices() const { return modelStorage.getVertices(); }

    void processVertexData(const tinyobj::attrib_t &attrib, const std::vector<tinyobj::shape_t> &shapes);
    void loadMaterials(const std::string &path, const std::vector<tinyobj::material_t> &materials);
private:
    ModelStorage &modelStorage;
};
