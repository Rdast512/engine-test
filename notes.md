Keep hot loops and their helpers in the same DLL (e.g., resource uploads, command buffer recording, descriptor updates, draw submission helpers all in engine_core or engine_render).
Make DLL APIs coarse-grained: “record frame,” “upload model batch,” “build pipeline,” not per-element or per-descriptor calls.
If you need shared math or small utilities, either:
build them as a static lib linked into each DLL (allows inlining), or
keep them in the same DLL as the hot callers.
For unavoidable cross-DLL calls in hot paths, reduce frequency (batch work), avoid tiny leaf functions, and prefer data buffers over many fine-grained calls.

TODO
Move frame/render loop work from vk_engine.cpp into vk_renderer.cpp: drawFrame, recordCommandBuffer, sync-per-frame flow.
Keep vk_engine.cpp as orchestrator only: init/shutdown, SDL events, high-level module wiring.
Move shader-module and shader-file utilities from vk_resource_manager.cpp into vk_shaders.cpp.
Move camera math/UBO matrix generation from vk_resource_manager.cpp into vk_camera.cpp and optionally shared math to maths.cpp.
Fill src/render/vk_materials.* with descriptor/material binding logic currently spread between descriptors + pipeline.
Clean allocator split: implementation is in vk_allocator.hpp while vk_allocator.cpp is empty; either move impl to .cpp or make it explicitly header-only and remove from target sources.


    

struct UniformBuffer
{
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

struct PushData
{
    DescriptorHandle<StructuredBuffer<float4x4>> ubo;
    DescriptorHandle<Texture2D> texture;
    DescriptorHandle<SamplerState> sampler;
};

[[vk::push_constant]] PushData push;

struct VSInput
{
    float3 inPosition;
    float3 inColor;
    float2 inTexCoord;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float3 fragColor;
    float2 fragTexCoord;
};

[shader("vertex")]
VSOutput vertMain(VSInput input)
{
    VSOutput output;
    StructuredBuffer<float4x4> transforms = getDescriptorFromHandle(push.ubo);
    float4x4 model = transforms[0];
    float4x4 view = transforms[1];
    float4x4 proj = transforms[2];

    output.pos =
        mul(proj,
        mul(view,
        mul(model, float4(input.inPosition, 1.0))));

    output.fragColor = input.inColor;
    output.fragTexCoord = input.inTexCoord;
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET
{
    Texture2D texture = getDescriptorFromHandle(push.texture);
    SamplerState samplerState = getDescriptorFromHandle(push.sampler);
    return texture.Sample(samplerState, vertIn.fragTexCoord);
}