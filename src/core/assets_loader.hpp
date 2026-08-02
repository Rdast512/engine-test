#pragma once
#include "object_storage.hpp"
#include "types.hpp"
#include "texture_manager.hpp"

#include <vector>
#include <string>
#include <string_view>


class AssetsLoader
{
public:
    explicit AssetsLoader(ObjectStorage& objectStorage, TextureManager& textureManager);
    ~AssetsLoader() = default;

    // Load a model into CPU mesh vectors and create a SoA entity in objectStorage.
    void loadModel(std::string modelPath, glm::vec3 xyz);

    void processVertexData(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes);
    void loadMaterials(const std::string& path, const std::vector<tinyobj::material_t>& materials);

    // Mesh data (direct access after load)
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    ObjectStorage& objectStorage;
    TextureManager& textureManager;

private:
    bool loadGltfModel(const std::string& modelPath, glm::vec3 xyz);
    bool loadObjModel(const std::string& modelPath, glm::vec3 xyz);
    uint32_t currentIndex = 0;
};
