#pragma once
#include <xmmintrin.h>
#define VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE 1
#include <cstdlib>
#include <string>
#include <memory>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <map>
#include <optional>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <assert.h>
#include <vulkan/vulkan_raii.hpp>
#include <vulkan/vk_platform.h>
#include <vector>
#include <unordered_map>
#include <ranges>
#include "../../ThirdParty/termcolor.hpp"
#include <fstream>
#include <chrono>
#include <filesystem>
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define VMA_VULKAN_VERSION 1004000 // Corrected for Vulkan 1.4
#if defined(__linux__)
#include <vk_mem_alloc.h>
#else
#include <vma/vk_mem_alloc.h>
#endif
#if defined(__linux__)
#include <stb_image.h>
#else
#include <stb_image.h>
#endif
#include "../../ThirdParty/tiny_obj_loader.h"

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
constexpr uint64_t FenceTimeout = 100000000;

#ifndef ENGINE_SHADER_DIR
#define ENGINE_SHADER_DIR "./shaders"
#endif
#ifndef ENGINE_MODELS_DIR
#define ENGINE_MODELS_DIR "./models"
#endif
#ifndef ENGINE_TEXTURES_DIR
#define ENGINE_TEXTURES_DIR "./textures"
#endif

inline const std::filesystem::path MODEL_PATH = std::filesystem::path(ENGINE_MODELS_DIR) / "room.obj";
inline const std::filesystem::path TEXTURE_PATH = std::filesystem::path(ENGINE_TEXTURES_DIR) / "viking_room.png";

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    bool operator==(const Vertex &other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }

    static vk::VertexInputBindingDescription getBindingDescription() {
        return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
    }

    static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
            vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord))
        };
    }
};

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



namespace std {
    template<>
    struct hash<Vertex> {
        size_t operator()(Vertex const &vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                     (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                   (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};
