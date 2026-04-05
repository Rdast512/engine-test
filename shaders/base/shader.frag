#version 460 core
#extension GL_EXT_descriptor_heap : require
#extension GL_EXT_nonuniform_qualifier : require

// In GLSL descriptor heaps, separated textures and samplers 
// live in their respective unsized arrays.
layout(descriptor_heap) uniform texture2D textureHeap[];
layout(descriptor_heap) uniform sampler samplerHeap[];

layout(push_constant) uniform PushData {
    uvec2 ubo;
    uvec2 texture;
    uvec2 samplerHandle;
} push;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() 
{
    uint textureIndex = nonuniformEXT(push.texture.x);
    uint samplerIndex = nonuniformEXT(push.samplerHandle.x);

    // GLSL natively constructs a combined sampler2D directly from 
    // the separate texture and sampler heap arrays
    outColor = texture(sampler2D(textureHeap[textureIndex], samplerHeap[samplerIndex]), fragTexCoord);
}