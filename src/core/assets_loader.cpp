#include "assets_loader.hpp"
#include "../Constants.h"
#include "../static_headers/logger.hpp"


// ── glTF external-reference detection ───────────────────────

namespace {

struct GltfExternalRef {
    std::string_view element;
    uint32_t         index;
    std::string_view uri;
};

struct GltfReferenceReport {
    bool fullySelfContained{true};
    std::vector<GltfExternalRef> externalRefs;

    uint32_t totalBuffers{0};
    uint32_t embeddedBuffers{0};
    uint32_t totalImages{0};
    uint32_t embeddedImages{0};
    uint32_t externalImageCount{0};
};

// Scans every glTF 2.0 element that can carry a URI and classifies the
// asset as fully self-contained (all data embedded) or as having external
// file references (e.g. separate .bin / .png files).
GltfReferenceReport detectGltfReferences(const tg3_model& model)
{
    GltfReferenceReport report;
    report.totalBuffers = model.buffers_count;
    report.totalImages  = model.images_count;

    // ── buffers ──────────────────────────────────────────
    //   empty uri  → GLB binary chunk (embedded)
    //   "data:"    → base64 inline (embedded)
    //   anything else → external .bin file reference
    for (uint32_t i = 0; i < model.buffers_count; ++i) {
        const tg3_buffer& buf = model.buffers[i];
        const std::string_view uri{buf.uri.data, buf.uri.len};

        if (!uri.empty() && !uri.starts_with("data:")) {
            report.fullySelfContained = false;
            report.externalRefs.push_back({"buffer", i, uri});
        } else {
            ++report.embeddedBuffers;
        }
    }

    // ── images ───────────────────────────────────────────
    //   bufferView >= 0 → embedded raw pixel data
    //   "data:"         → base64 inline
    //   empty uri       → GLB-embedded (interpreted by the runtime)
    //   anything else   → external image file (png, jpg, …)
    for (uint32_t i = 0; i < model.images_count; ++i) {
        const tg3_image& img = model.images[i];
        const std::string_view uri{img.uri.data, img.uri.len};

        if (img.buffer_view >= 0 || uri.starts_with("data:") || uri.empty()) {
            ++report.embeddedImages;
        } else {
            report.fullySelfContained = false;
            report.externalRefs.push_back({"image", i, uri});
            ++report.externalImageCount;
        }
    }

    return report;
}

// ── glTF accessor helpers ───────────────────────────────────

// Read float data from a glTF accessor. Returns an empty vector
// on any error (missing buffer, unsupported type, etc.).
static std::vector<float> readAccessorFloats(const tg3_model& model, int32_t accessorIdx)
{
    if (accessorIdx < 0 || static_cast<uint32_t>(accessorIdx) >= model.accessors_count)
        return {};

    const tg3_accessor& acc = model.accessors[accessorIdx];
    if (acc.buffer_view < 0 || static_cast<uint32_t>(acc.buffer_view) >= model.buffer_views_count)
        return {};

    const tg3_buffer_view& bv = model.buffer_views[acc.buffer_view];
    if (bv.buffer < 0 || static_cast<uint32_t>(bv.buffer) >= model.buffers_count)
        return {};

    const tg3_buffer& buf = model.buffers[bv.buffer];
    if (!buf.data.data) return {};

    const int32_t compSize = tg3_component_size(acc.component_type);
    const int32_t numComp  = tg3_num_components(acc.type);
    if (compSize < 0 || numComp < 0) return {};

    const int32_t stride = tg3_accessor_byte_stride(&acc, &bv);
    if (stride < 0) return {};

    const auto offset = static_cast<size_t>(bv.byte_offset) + static_cast<size_t>(acc.byte_offset);
    const uint8_t* src = buf.data.data + offset;
    const auto elemCount = static_cast<size_t>(acc.count);

    std::vector<float> result;
    result.reserve(elemCount * static_cast<size_t>(numComp));

    // Fast path: tightly-packed float data
    if (acc.component_type == TG3_COMPONENT_TYPE_FLOAT && compSize == 4 && stride == compSize * numComp) {
        const auto* floats = reinterpret_cast<const float*>(src);
        result.assign(floats, floats + elemCount * numComp);
        return result;
    }

    // General path — per-element, per-component read with normalisation
    for (size_t elem = 0; elem < elemCount; ++elem) {
        const uint8_t* elemSrc = src + elem * static_cast<size_t>(stride);
        for (int32_t c = 0; c < numComp; ++c) {
            const uint8_t* compSrc = elemSrc + static_cast<size_t>(c) * compSize;
            float val = 0.0f;
            switch (acc.component_type) {
            case TG3_COMPONENT_TYPE_FLOAT:
                val = *reinterpret_cast<const float*>(compSrc);
                break;
            case TG3_COMPONENT_TYPE_UNSIGNED_SHORT:
                val = static_cast<float>(*reinterpret_cast<const uint16_t*>(compSrc)) / 65535.0f;
                break;
            case TG3_COMPONENT_TYPE_UNSIGNED_BYTE:
                val = static_cast<float>(*reinterpret_cast<const uint8_t*>(compSrc)) / 255.0f;
                break;
            case TG3_COMPONENT_TYPE_SHORT:
                val = static_cast<float>(*reinterpret_cast<const int16_t*>(compSrc)) / 32767.0f;
                break;
            default:
                // unsupported component type — return what we have so far
                return result;
            }
            result.push_back(val);
        }
    }

    return result;
}

// Read index data from a glTF accessor. Supports UINT32, UINT16, and UINT8.
static std::vector<uint32_t> readAccessorIndices(const tg3_model& model, int32_t accessorIdx)
{
    if (accessorIdx < 0 || static_cast<uint32_t>(accessorIdx) >= model.accessors_count)
        return {};

    const tg3_accessor& acc = model.accessors[accessorIdx];
    if (acc.buffer_view < 0 || static_cast<uint32_t>(acc.buffer_view) >= model.buffer_views_count)
        return {};

    const tg3_buffer_view& bv = model.buffer_views[acc.buffer_view];
    if (bv.buffer < 0 || static_cast<uint32_t>(bv.buffer) >= model.buffers_count)
        return {};

    const tg3_buffer& buf = model.buffers[bv.buffer];
    if (!buf.data.data) return {};

    const auto offset = static_cast<size_t>(bv.byte_offset) + static_cast<size_t>(acc.byte_offset);
    const uint8_t* src = buf.data.data + offset;

    std::vector<uint32_t> result;
    result.reserve(static_cast<size_t>(acc.count));

    for (uint32_t i = 0; i < acc.count; ++i) {
        switch (acc.component_type) {
        case TG3_COMPONENT_TYPE_UNSIGNED_INT:
            result.push_back(reinterpret_cast<const uint32_t*>(src)[i]);
            break;
        case TG3_COMPONENT_TYPE_UNSIGNED_SHORT:
            result.push_back(static_cast<uint32_t>(reinterpret_cast<const uint16_t*>(src)[i]));
            break;
        case TG3_COMPONENT_TYPE_UNSIGNED_BYTE:
            result.push_back(static_cast<uint32_t>(src[i]));
            break;
        default:
            return {};  // unsupported index type
        }
    }

    return result;
}

} // anonymous namespace

static std::vector<char> readFile(const std::string& filename)
{
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


AssetsLoader::AssetsLoader(std::vector<Object> &objectsIn, TextureManager &textureManager) : vertices(), indices(), objects(objectsIn), textureManager(textureManager) { log_info("AssetsLoader initialized", "AssetLoader"); }


void AssetsLoader::loadModel(std::string modelPath, glm::vec3 xyz)
{
    // Normalise to native separators once so every loader receives a
    // clean, OS-consistent path regardless of how it was supplied.
    const std::string path = std::filesystem::path(modelPath).make_preferred().string();

    const bool isGltf = path.ends_with(".gltf") || path.ends_with(".glb");
    const bool isObj  = path.ends_with(".obj");

    if (isGltf)
    {
        loadGltfModel(path, xyz);
        return;
    }

    if (isObj)
    {
        loadObjModel(path, xyz);
        return;
    }

    assert(false && "Unsupported model format");
}

bool AssetsLoader::loadGltfModel(const std::string& modelPath, glm::vec3 xyz)
{
    // glTF uses forward-slash URIs internally; normalise the base path
    // to avoid mixed separators when the library resolves external .bin
    // references (e.g. "models/AnimatedCube.bin" under "models\" on Windows).
    const std::string normalizedPath = std::filesystem::path(modelPath).generic_string();

    tg3_model model{};
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    tg3_parse_options opts;
    tg3_parse_options_init(&opts);
    opts.parse_float32 = 1;

    const tg3_error_code rc = tg3_parse_file(
        &model, &errors, normalizedPath.c_str(),
        static_cast<uint32_t>(normalizedPath.size()), &opts);

    if (rc != TG3_OK || model.meshes_count == 0)
    {
        const uint32_t errorCount = tg3_errors_count(&errors);
        if (errorCount > 0)
        {
            std::string details;
            for (uint32_t i = 0; i < errorCount; ++i)
            {
                const tg3_error_entry* entry = tg3_errors_get(&errors, i);
                details += std::format("  [{}/{}] {}", static_cast<int>(entry->severity),
                                       static_cast<int>(entry->code), entry->message);
                if (entry->json_path && entry->json_path[0] != '\0')
                    details += std::format(" (at {})", entry->json_path);
                details += '\n';
            }
            log_error(std::format("Failed to parse glTF (rc={}):\n{}", static_cast<int>(rc), details), "AssetLoader");
        }
        else
        {
            log_error(std::format("Failed to parse glTF: rc={}", static_cast<int>(rc)), "AssetLoader");
        }
        tg3_model_free(&model);
        tg3_error_stack_free(&errors);
        return false;
    }

    log_info(std::format("Loading glTF: {} meshes, {} nodes",
                         model.meshes_count, model.nodes_count), "AssetLoader");

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};
    uint32_t indexCount = 0;

    for (uint32_t mi = 0; mi < model.meshes_count; ++mi) {
        const tg3_mesh& mesh = model.meshes[mi];

        for (uint32_t pi = 0; pi < mesh.primitives_count; ++pi) {
            const tg3_primitive& prim = mesh.primitives[pi];

            int32_t posAcc = -1;
            int32_t tcAcc  = -1;
            const int32_t idxAcc = prim.indices;

            for (uint32_t ai = 0; ai < prim.attributes_count; ++ai) {
                const tg3_str_int_pair& attr = prim.attributes[ai];
                if (tg3_str_equals_cstr(attr.key, "POSITION"))
                    posAcc = attr.value;
                else if (tg3_str_equals_cstr(attr.key, "TEXCOORD_0"))
                    tcAcc = attr.value;
            }

            if (posAcc < 0) continue;

            const std::vector<float> positions = readAccessorFloats(model, posAcc);
            if (positions.empty()) continue;

            const std::vector<float> texcoords  = readAccessorFloats(model, tcAcc);
            const std::vector<uint32_t> idxData = readAccessorIndices(model, idxAcc);

            const uint32_t posComps =
                static_cast<uint32_t>(tg3_num_components(model.accessors[posAcc].type));
            const uint32_t tcComps =
                tcAcc >= 0 ? static_cast<uint32_t>(tg3_num_components(model.accessors[tcAcc].type)) : 0;
            const uint32_t vertexCount = model.accessors[posAcc].count;

            auto emitVertex = [&](uint32_t vi) -> void {
                if (vi >= vertexCount) return;
                Vertex vertex{};
                vertex.pos = {positions[vi * posComps + 0],
                              positions[vi * posComps + 1],
                              posComps >= 3 ? positions[vi * posComps + 2] : 0.0f};
                if (tcComps >= 2) {
                    vertex.texCoord = {texcoords[vi * tcComps + 0],
                                       1.0f - texcoords[vi * tcComps + 1]};
                }
                vertex.color = {1.0f, 1.0f, 1.0f};

                if (!uniqueVertices.contains(vertex)) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }
                indices.push_back(uniqueVertices[vertex]);
                ++indexCount;
            };

            if (!idxData.empty()) {
                for (const uint32_t vid : idxData) { emitVertex(vid);}
            } else {
                for (uint32_t vid = 0; vid < vertexCount; ++vid) { emitVertex(vid);}
            }
        }
    }

    // Resolve the glTF image URI relative to the model file's directory.
    // Embedded images (bufferView / data URI) are not handled here — the
    // texture manager would receive an empty path and fail gracefully.
    std::string imagePath;
    if (model.images_count > 0 && model.images[0].uri.data && model.images[0].uri.len > 0)
    {
        const std::string_view uri(model.images[0].uri.data, model.images[0].uri.len);
        if (!uri.starts_with("data:"))
        {
            const auto modelDir = std::filesystem::path(modelPath).parent_path();
            imagePath = (modelDir / uri).string();
        }
    }
    Object object(currentIndex, indexCount, textureManager.loadTexture(imagePath));
    object.setPosition(glm::vec3{xyz[0], xyz[1], xyz[2]});
    log_info(std::format("Loaded model current index: {} | index count: {}", currentIndex, indexCount), "AssetLoader");
    objects.push_back(std::move(object));
    currentIndex += indexCount;

    log_info(std::format("Model loaded (glTF): {} | vertices: {} | indices: {}",
                         modelPath, vertices.size(), indices.size()), "AssetLoader");

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
    return true;
}

bool AssetsLoader::loadObjModel(const std::string& modelPath, glm::vec3 xyz)
{
    log_info(std::format("Loading OBJ: {}", modelPath), "AssetLoader");
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    // tinyobj wraps standard C file I/O — native separators are correct.
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, modelPath.c_str()))
    {
        log_error(std::format("Failed to load OBJ: {}", err), "AssetLoader");
        return false;
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};
    uint32_t indexCount = 0;

    for (const auto& [name, mesh] : shapes) {
        for (const auto& index : mesh.indices) {
            Vertex vertex{};

            vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
                          attrib.vertices[3 * index.vertex_index + 1],
                          attrib.vertices[3 * index.vertex_index + 2]};

            vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
                               1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

            vertex.color = {1.0f, 1.0f, 1.0f};

            if (!uniqueVertices.contains(vertex)) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }
            indices.push_back(uniqueVertices[vertex]);
            indexCount++;
        }
    }
    Object object(currentIndex, indexCount, textureManager.loadTexture(TEXTURE_PATH.string()));
    object.setPosition(glm::vec3{xyz[0], xyz[1], xyz[2]});
    log_info(std::format("Loaded model current index: {} | index count: {}", currentIndex, indexCount));
    objects.push_back(std::move(object));
    currentIndex += indexCount;
    log_info(std::format("Model loaded (OBJ): {} | vertices: {} | indices: {}",
                         modelPath, vertices.size(), indices.size()), "AssetLoader");
    return true;
}
