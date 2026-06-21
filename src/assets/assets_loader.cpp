#include "assets_loader.hpp"
#include <string_view>
#include "../Constants.h"
#include "../static_headers/logger.hpp"
#include "tiny_gltf_v3.h"

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


AssetsLoader::AssetsLoader() : vertices(), indices()
{
    log_info("AssetsLoader initialized");
    loadModel();
}

void parseGLTFModel(const std::string& modelPath)
{
    const auto path = modelPath.empty() ? MODEL_PATH_GLTF.string() : modelPath;
    tg3_model model{};
    tg3_error_stack errors;
    tg3_error_stack_init(&errors);

    tg3_parse_options opts;
    tg3_parse_options_init(&opts);
    opts.parse_float32 = 1; // optional speed mode

    const tg3_error_code rc = tg3_parse_file(&model, &errors, path.c_str(), static_cast<uint32_t>(path.size()), &opts);
    if (rc != TG3_OK) {
        std::printf("Failed to load model: %s (code=%d)\n", path.c_str(), static_cast<int>(rc));

        const uint32_t errorCount = tg3_errors_count(&errors);
        for (uint32_t i = 0; i < errorCount; ++i) {
            const tg3_error_entry* e = tg3_errors_get(&errors, i);
            if (!e) {
                continue;
            }
            std::printf("  [%u] %s\n", i, e->message ? e->message : "(no message)");
        }

        tg3_model_free(&model);
        tg3_error_stack_free(&errors);
        return;
    }

    const auto toStringView = [](tg3_str value) -> std::string_view
    { return value.data != nullptr ? std::string_view{value.data, value.len} : std::string_view{}; };

    log_info(
        std::format("Model loaded: {} | meshes: {} | nodes: {} | materials: {} | textures: {} | accessors: {} | "
                    "buffers: {} | images: {} | animations: {} | cameras: {} | skins: {} | scenes: {} | samplers: {} | "
                    "lights: {} | audio_emitters: {} | audio_sources: {}",
                    path, model.meshes_count, model.nodes_count, model.materials_count, model.textures_count,
                    model.accessors_count, model.buffers_count, model.images_count, model.animations_count,
                    model.cameras_count, model.skins_count, model.scenes_count, model.samplers_count,
                    model.lights_count, model.audio_emitters_count, model.audio_sources_count));

    std::vector<tg3_accessor> accessors{};
    std::vector<tg3_animation> animations{};
    std::vector<tg3_buffer_view> bufferViews{};
    std::vector<tg3_buffer> buffers{};
    std::vector<tg3_camera> cameras{};
    std::vector<tg3_image> images{};
    std::vector<tg3_material> materials{};
    std::vector<tg3_mesh> meshes{};
    std::vector<tg3_node> nodes{};
    std::vector<tg3_sampler> samplers{};
    std::vector<tg3_scene> scenes{};
    std::vector<tg3_skin> skins{};
    std::vector<tg3_texture> textures{};
    std::vector<tg3_light> lights{};
    std::vector<tg3_audio_emitter> audioEmitters{};
    std::vector<tg3_audio_source> audioSources{};
    std::vector<tg3_str> extensionsUsed{};
    std::vector<tg3_str> extensionsRequired{};
    std::vector<tg3_extension> rootExtensions{};
    std::vector<tg3_value> rootExtras{};

    extensionsUsed.reserve(model.extensions_used_count);
    for (uint32_t i = 0; i < model.extensions_used_count; ++i) {
        extensionsUsed.emplace_back(model.extensions_used[i]);
    }

    extensionsRequired.reserve(model.extensions_required_count);
    for (uint32_t i = 0; i < model.extensions_required_count; ++i) {
        extensionsRequired.emplace_back(model.extensions_required[i]);
    }

    rootExtensions.reserve(model.ext.extensions_count);
    for (uint32_t i = 0; i < model.ext.extensions_count; ++i) {
        rootExtensions.emplace_back(model.ext.extensions[i]);
    }
    if (model.ext.extras != nullptr) {
        rootExtras.emplace_back(*model.ext.extras);
    }

    for (uint32_t i = 0; i < model.accessors_count; ++i) {
        const auto accessor = model.accessors[i];
        accessors.emplace_back(accessor);
        log_info(std::format("Accessor {}: name: {} | type: {} | buffer_view: {} | count: {} | component_type: {}", i,
                             toStringView(accessor.name), accessor.type, accessor.buffer_view, accessor.count,
                             accessor.component_type));
    }

    for (uint32_t i = 0; i < model.animations_count; ++i) {
        const auto animation = model.animations[i];
        animations.emplace_back(animation);
        log_info(std::format("Animation {}: name: {} | channels: {} | samplers: {}", i, toStringView(animation.name),
                             animation.channels_count, animation.samplers_count));
    }

    for (uint32_t i = 0; i < model.buffers_count; ++i) {
        const auto buffer = model.buffers[i];
        buffers.emplace_back(buffer);
        const auto bufferName = toStringView(buffer.name);
        const auto bufferUri = toStringView(buffer.uri);
        if (bufferUri.starts_with("data:application/octet-stream;base64")) {
            log_info(std::format("Buffer {}: name: {} | uri: (embedded data uri) | bytes: {}", i, bufferName,
                                 buffer.data.count));
        } else {
            log_info(
                std::format("Buffer {}: name: {} | uri: {} | bytes: {}", i, bufferName, bufferUri, buffer.data.count));
        }
    }

    for (uint32_t i = 0; i < model.buffer_views_count; ++i) {
        const auto bufferView = model.buffer_views[i];
        bufferViews.emplace_back(bufferView);
        log_info(std::format("BufferView {}: name: {} | buffer: {} | offset: {} | length: {} | stride: {}", i,
                             toStringView(bufferView.name), bufferView.buffer, bufferView.byte_offset,
                             bufferView.byte_length, bufferView.byte_stride));
    }

    for (uint32_t i = 0; i < model.cameras_count; ++i) {
        const auto camera = model.cameras[i];
        cameras.emplace_back(camera);
        log_info(std::format("Camera {}: name: {} | type: {} | perspective(yfov: {}, znear: {}, zfar: {}) | "
                             "orthographic(xmag: {}, ymag: {}, znear: {}, zfar: {})",
                             i, toStringView(camera.name), toStringView(camera.type), camera.perspective.yfov,
                             camera.perspective.znear, camera.perspective.zfar, camera.orthographic.xmag,
                             camera.orthographic.ymag, camera.orthographic.znear, camera.orthographic.zfar));
    }

    for (uint32_t i = 0; i < model.images_count; ++i) {
        const auto image = model.images[i];
        images.emplace_back(image);
        log_info(std::format("Image {}: name: {} | uri: {} | as_is: {}", i, toStringView(image.name),
                             toStringView(image.uri), image.as_is));
    }

    for (uint32_t i = 0; i < model.materials_count; ++i) {
        const auto material = model.materials[i];
        materials.emplace_back(material);
        log_info(std::format("Material {}: name: {} | alpha_mode: {} | double_sided: {} | base_color_texture: {} | "
                             "normal_texture: {} | emissive_texture: {}",
                             i, toStringView(material.name), toStringView(material.alpha_mode), material.double_sided,
                             material.pbr_metallic_roughness.base_color_texture.index, material.normal_texture.index,
                             material.emissive_texture.index));
    }

    for (uint32_t i = 0; i < model.meshes_count; ++i) {
        const auto mesh = model.meshes[i];
        meshes.emplace_back(mesh);
        log_info(std::format("Mesh {}: name: {} | primitives: {} | weights: {}", i, toStringView(mesh.name),
                             mesh.primitives_count, mesh.weights_count));
    }

    for (uint32_t i = 0; i < model.nodes_count; ++i) {
        const auto node = model.nodes[i];
        nodes.emplace_back(node);
        log_info(std::format("Node {}: name: {} | mesh: {} | camera: {} | skin: {} | children: {} | weights: {} | "
                             "has_matrix: {}",
                             i, toStringView(node.name), node.mesh, node.camera, node.skin, node.children_count,
                             node.weights_count, node.has_matrix));
    }

    for (uint32_t i = 0; i < model.samplers_count; ++i) {
        const auto sampler = model.samplers[i];
        samplers.emplace_back(sampler);
        log_info(std::format("Sampler {}: name: {} | mag_filter: {} | min_filter: {} | wrap_s: {} | wrap_t: {}", i,
                             toStringView(sampler.name), sampler.mag_filter, sampler.min_filter, sampler.wrap_s,
                             sampler.wrap_t));
    }

    for (uint32_t i = 0; i < model.scenes_count; ++i) {
        const auto scene = model.scenes[i];
        scenes.emplace_back(scene);
        log_info(std::format("Scene {}: name: {} | nodes: {} | audio_emitters: {}", i, toStringView(scene.name),
                             scene.nodes_count, scene.audio_emitters_count));
    }

    for (uint32_t i = 0; i < model.skins_count; ++i) {
        const auto skin = model.skins[i];
        skins.emplace_back(skin);
        log_info(std::format("Skin {}: name: {} | inverse_bind_matrices: {} | skeleton: {} | joints: {}", i,
                             toStringView(skin.name), skin.inverse_bind_matrices, skin.skeleton, skin.joints_count));
    }

    for (uint32_t i = 0; i < model.textures_count; ++i) {
        const auto texture = model.textures[i];
        textures.emplace_back(texture);
        log_info(std::format("Texture {}: name: {} | sampler: {} | source: {}", i, toStringView(texture.name),
                             texture.sampler, texture.source));
    }

    for (uint32_t i = 0; i < model.lights_count; ++i) {
        const auto light = model.lights[i];
        lights.emplace_back(light);
        log_info(std::format("Light {}: name: {} | type: {} | intensity: {} | color: [{}, {}, {}]", i,
                             toStringView(light.name), toStringView(light.type), light.intensity, light.color[0],
                             light.color[1], light.color[2]));
    }

    for (uint32_t i = 0; i < model.audio_emitters_count; ++i) {
        const auto audioEmitter = model.audio_emitters[i];
        audioEmitters.emplace_back(audioEmitter);
        log_info(std::format("AudioEmitter {}: name: {} | type: {} | source: {} | gain: {} | loop: {} | playing: {}", i,
                             toStringView(audioEmitter.name), toStringView(audioEmitter.type), audioEmitter.source,
                             audioEmitter.gain, audioEmitter.loop, audioEmitter.playing));
    }

    for (uint32_t i = 0; i < model.audio_sources_count; ++i) {
        const auto audioSource = model.audio_sources[i];
        audioSources.emplace_back(audioSource);
        log_info(std::format("AudioSource {}: name: {} | uri: {} | buffer_view: {} | mime_type: {}", i,
                             toStringView(audioSource.name), toStringView(audioSource.uri), audioSource.buffer_view,
                             toStringView(audioSource.mime_type)));
    }

    if (model.default_scene >= 0) {
        log_info(std::format("Default scene index: {}", model.default_scene));
    }

    log_info(std::format("Root extensions: used={} required={} extras={} root_extension_entries={}",
                         extensionsUsed.size(), extensionsRequired.size(), rootExtras.size(), rootExtensions.size()));

    tg3_model_free(&model);
    tg3_error_stack_free(&errors);
}

void AssetsLoader::loadModel()
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    std::unordered_map<Vertex, uint32_t> uniqueVertices{};
    parseGLTFModel("");
    const auto modelPath = MODEL_PATH.string();

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, modelPath.c_str())) {
        throw std::runtime_error(err);
    }
    for (const auto& [name, mesh] : shapes) {
        for (const auto& index : mesh.indices) {
            Vertex vertex{};

            vertex.pos = {attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1],
                          attrib.vertices[3 * index.vertex_index + 2]};

            vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
                               1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

            vertex.color = {1.0f, 1.0f, 1.0f};

            if (uniqueVertices.contains(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }
            indices.push_back(uniqueVertices[vertex]);
        }
    }

    log_info(std::format("Model loaded: {} | vertices: {} | indices: {}", modelPath, vertices.size(), indices.size()));
}
