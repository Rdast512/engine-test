#pragma once
#include <string>
#include <string_view>
#include <vector>
#include "object_storage.hpp"
#include "texture_manager.hpp"
#include "types.hpp"


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

    // Meshlet data (CPU only for now; GPU upload later)
    std::vector<MeshletDesc> meshlets;
    std::vector<uint32_t> meshletVertices;
    std::vector<uint8_t> meshletTriangles;

    ObjectStorage& objectStorage;
    TextureManager& textureManager;

private:
    bool loadGltfModel(const std::string& modelPath, glm::vec3 xyz);
    bool loadObjModel(const std::string& modelPath, glm::vec3 xyz);

    // Builds meshlets for indices[firstIndex, firstIndex + indexCount) into the global meshlet arrays.
    [[nodiscard]] MeshletDraw buildMeshletsForRange(uint32_t firstIndex, uint32_t indexCount);

    uint32_t currentIndex = 0;
};
