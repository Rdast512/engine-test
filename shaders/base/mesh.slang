// Mesh-only pipeline shader (no task stage).
// Hardcoded NDC triangle for pipeline bring-up; replace with meshlet loads later.

// ── Camera buffer (matches CameraData in types.hpp) ──────────────
struct CameraData
{
    float4x4 view;
    float4x4 proj;
    float4x4 viewProj;

    float4x4 invView;
    float4x4 invProj;
    float4x4 invViewProj;

    float4x4 prevViewProj;

    float3 cameraPos;
    float nearZ;

    float2 renderTargetSize;
    float2 invRenderTargetSize;

    float2 jitterOffset;
    float farZ;
    float frameDeltaTime;
};

// ── Per-object buffer (matches ObjectUB in types.hpp) ────────────
struct ObjectUB
{
    float4x4 modelMatrix;
    float4x4 prevModelMatrix;

    float4 boundingSphere;

    uint32_t materialID;
    uint32_t instanceFlags;
    uint32_t baseVertex;
    uint32_t baseIndex;
};

// ── Push constants (matches PushData2 in push_data.hpp) ──────────
struct PushData
{
    uint64_t cameraAddress;
    uint64_t objectUBAddress;
    DescriptorHandle<Texture2D> texture;
    DescriptorHandle<SamplerState> samplerHandle;
};

[[vk::push_constant]] ConstantBuffer<PushData> push;

struct MeshVertexOut
{
    float4 pos          : SV_Position;
    float3 fragColor    : COLOR;
    float2 fragTexCoord : TEXCOORD;
};

// ── Mesh shader (replaces vertex input assembly) ─────────────────
[shader("mesh")]
[numthreads(1, 1, 1)]
[outputtopology("triangle")]
void meshMain(out vertices MeshVertexOut verts[3], out indices uint3 tris[1])
{
    SetMeshOutputCounts(3, 1);

    // Clip-space triangle so drawMeshTasksEXT(1,1,1) works without meshlets yet.
    verts[0].pos = float4(-0.5, -0.5, 0.0, 1.0);
    verts[0].fragColor = float3(1.0, 0.0, 0.0);
    verts[0].fragTexCoord = float2(0.0, 0.0);

    verts[1].pos = float4(0.5, -0.5, 0.0, 1.0);
    verts[1].fragColor = float3(0.0, 1.0, 0.0);
    verts[1].fragTexCoord = float2(1.0, 0.0);

    verts[2].pos = float4(0.0, 0.5, 0.0, 1.0);
    verts[2].fragColor = float3(0.0, 0.0, 1.0);
    verts[2].fragTexCoord = float2(0.5, 1.0);

    tris[0] = uint3(0, 1, 2);
}

// ── Fragment shader ──────────────────────────────────────────────
[shader("fragment")]
float4 fragMain(MeshVertexOut vertIn) : SV_TARGET
{
    Texture2D texture = getDescriptorFromHandle(push.texture);
    SamplerState samplerState = getDescriptorFromHandle(push.samplerHandle);

    return texture.Sample(samplerState, vertIn.fragTexCoord);
}
