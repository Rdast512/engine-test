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
    explicit AssetsLoader(std::vector<Object> &objects, TextureManager &textureManager);
    ~AssetsLoader() = default;

    // Load a model into the provided tinyobj containers. The function fills
    // the caller-provided attrib/shapes/materials vectors similar to
    // tinyobj::LoadObj which accepts references and populates them.
    void loadModel(std::string modelPath, glm::vec3 xyz);

    // Parse a glTF model into caller-provided output containers.
    // Returns true on success, false on failure (error message in err).
    // Works like tinyobj::LoadObj - caller provides pointers to vectors that get populated.
    // All output parameters are optional (default to nullptr).

    void processVertexData(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes);
    void loadMaterials(const std::string& path, const std::vector<tinyobj::material_t>& materials);

    // Mesh data (direct access after load)
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<Object> &objects;
    TextureManager &textureManager;

private:
    // Format-specific model-loading helpers called by loadModel.
    // Returns true on success (no fallback needed).
    bool loadGltfModel(const std::string& modelPath, glm::vec3 xyz);
    bool loadObjModel(const std::string& modelPath, glm::vec3 xyz);
    uint32_t currentIndex = 0;
};
