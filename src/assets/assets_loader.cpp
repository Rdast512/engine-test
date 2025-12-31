
#include "assets_loader.hpp"
#include "../Constants.h"
#include <fstream>
#include <format>
#include <unordered_map>
#include <utility>
#include "../util/logger.hpp"

static std::vector<char> readFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
    std::vector<char> buffer(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    file.close();

    return buffer;
}



AssetsLoader::AssetsLoader(ModelStorage &storage) : modelStorage(storage) {
    log_info("AssetsLoader initialized");
    loadModel();
}

void AssetsLoader::loadModel() {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    std::unordered_map<Vertex, uint32_t> uniqueVertices{};
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    const auto modelPath = MODEL_PATH.string();

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, modelPath.c_str())) {
        throw std::runtime_error(err);
    }
    for (const auto &shape: shapes) {
        for (const auto &index: shape.mesh.indices) {
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = {1.0f, 1.0f, 1.0f};

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }
            indices.push_back(uniqueVertices[vertex]);
        }
    }
    modelStorage.setModelData(std::move(vertices), std::move(indices));
    log_info(std::format("Model loaded: {} | vertices: {} | indices: {}", modelPath, modelStorage.getVertices().size(),
                         modelStorage.getIndices().size()));
}
