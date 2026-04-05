#version 460 core
#extension GL_EXT_descriptor_heap : require
#extension GL_EXT_nonuniform_qualifier : require

// Declare the bindless Descriptor Heap for Uniform Buffers.
// Using 'uniform' here guarantees the GPU routes the read through the 
// ultra-fast L0 constant broadcast cache, completely eliminating the lag.
layout(descriptor_heap) uniform UniformBuffer {
    mat4 model;
    mat4 view;
    mat4 proj;
} uboHeap[];

// We use uvec2 here to perfectly match your 8-byte C++ Handle struct 
// (x = index, y = unused). This prevents any memory alignment mismatches.
layout(push_constant) uniform PushData {
    uvec2 ubo;
    uvec2 texture;
    uvec2 samplerHandle;
} push;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() 
{
    // Fetch the index from push constants
    uint uboIndex = nonuniformEXT(push.ubo.x);

    // Directly index the heap! 
    mat4 model = uboHeap[uboIndex].model;
    mat4 view  = uboHeap[uboIndex].view;
    mat4 proj  = uboHeap[uboIndex].proj;

    gl_Position = proj * view * model * vec4(inPosition, 1.0);
    
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}