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
    void loadModel(std::string modelPath);

    // Parse a glTF model into caller-provided output containers.
    // Returns true on success, false on failure (error message in err).
    // Works like tinyobj::LoadObj - caller provides pointers to vectors that get populated.
    // All output parameters are optional (default to nullptr).
    static bool parseGLTFModel(
        const char* filename,
        std::vector<tg3_accessor>* accessors = nullptr,
        std::vector<tg3_buffer_view>* bufferViews = nullptr,
        std::vector<tg3_buffer>* buffers = nullptr,
        std::vector<tg3_camera>* cameras = nullptr,
        std::vector<tg3_image>* images = nullptr,
        std::vector<tg3_material>* materials = nullptr,
        std::vector<tg3_mesh>* meshes = nullptr,
        std::vector<tg3_node>* nodes = nullptr,
        std::vector<tg3_sampler>* samplers = nullptr,
        std::vector<tg3_scene>* scenes = nullptr,
        std::vector<tg3_skin>* skins = nullptr,
        std::vector<tg3_texture>* textures = nullptr,
        std::vector<tg3_light>* lights = nullptr,
        std::vector<tg3_str>* extensionsUsed = nullptr,
        std::vector<tg3_str>* extensionsRequired = nullptr,
        std::vector<tg3_extension>* rootExtensions = nullptr,
        std::vector<tg3_value>* rootExtras = nullptr,
        tg3_asset* asset = nullptr,
        int32_t* defaultScene = nullptr,
        std::string* err = nullptr,
        const tg3_parse_options* options = nullptr);

    // helper functions to
    [[nodiscard]] const std::vector<Object>& getObjects() const { return objects; }
    [[nodiscard]] const std::vector<uint32_t>& getIndices() const { return indices; }
    [[nodiscard]] const std::vector<Vertex>& getVertices() const { return vertices; }

    void processVertexData(const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes);
    void loadMaterials(const std::string& path, const std::vector<tinyobj::material_t>& materials);

private:
    // Format-specific model-loading helpers called by loadModel.
    // Returns true on success (no fallback needed).
    bool loadGltfModel(const std::string& modelPath);
    bool loadObjModel(const std::string& modelPath);
    uint32_t currentIndex = 0;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<Object> &objects;
    TextureManager &textureManager;
};
