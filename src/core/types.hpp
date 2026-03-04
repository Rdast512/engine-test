#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <vulkan/vulkan.hpp>
#include <array>

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


struct EngineSettings
{
    uint8_t x_resolution;
    uint8_t y_resolution;
    uint8_t mipmapLevel;
    bool hdr;
    bool fullscreen;
    bool vsync;
    bool debug;
    bool windowed;
};

struct HardwareCapabilities
{
    // Core/core-promoted properties
    vk::PhysicalDeviceProperties2 properties2;
    vk::PhysicalDeviceVulkan11Properties vulkan11;
    vk::PhysicalDeviceVulkan12Properties vulkan12;
    vk::PhysicalDeviceVulkan13Properties vulkan13;
    vk::PhysicalDeviceVulkan14Properties vulkan14;

    // Extension / advanced properties (as requested)
    vk::PhysicalDeviceBlendOperationAdvancedPropertiesEXT blendOperationAdvanced;
    vk::PhysicalDeviceDescriptorHeapPropertiesEXT descriptorHeap;
    vk::PhysicalDeviceDescriptorIndexingPropertiesEXT descriptorIndexing;
    vk::PhysicalDeviceMeshShaderPropertiesEXT meshShader;
    vk::PhysicalDeviceDeviceGeneratedCommandsPropertiesEXT deviceGeneratedCommands;
    vk::PhysicalDeviceMultiDrawPropertiesEXT multiDraw;
    vk::PhysicalDeviceHostImageCopyPropertiesEXT hostImageCopy;
    vk::PhysicalDeviceTexelBufferAlignmentPropertiesEXT texelBufferAlignment;

    // KHR / other properties
    vk::PhysicalDeviceFragmentShadingRatePropertiesKHR fragmentShadingRate;
    vk::PhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructure;
    vk::PhysicalDeviceDepthStencilResolveProperties depthStencilResolve;
    vk::PhysicalDeviceDriverProperties driverProperties;
    vk::PhysicalDeviceMaintenance3Properties maintenance3;
    vk::PhysicalDeviceMaintenance4Properties maintenance4;
    vk::PhysicalDeviceMaintenance5Properties maintenance5;
    vk::PhysicalDeviceMaintenance6Properties maintenance6;
    vk::PhysicalDeviceMaintenance7PropertiesKHR maintenance7;
    vk::PhysicalDeviceMaintenance9PropertiesKHR maintenance9;
    vk::PhysicalDeviceMaintenance10PropertiesKHR maintenance10;
    vk::PhysicalDevicePipelineBinaryPropertiesKHR pipelineBinary;
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipeline;
};

struct EngineContext
{
    EngineSettings settings;
    HardwareCapabilities hardwareCapabilities;
};