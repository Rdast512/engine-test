// Mesh-only pipeline (no task stage).
// One workgroup per meshlet: drawMeshTasksEXT(meshletCount, 1, 1).
// Geometry is loaded via buffer-device addresses in MeshPushData (no vertex input).

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

    float4 cameraParams; // reserved (matches C++ CameraData)
};

// ── Per-object buffer (matches ObjectUB in types.hpp) ────────────
struct ObjectUB
{
    float4x4 modelMatrix;
    float4x4 prevModelMatrix;

    float4 boundingSphere;

    uint materialID;
    uint instanceFlags;
    uint baseVertex;
    uint baseIndex;
};

// GPU vertex — matches C++ Vertex (glm::vec3/vec3/vec2, 32 B, no std430 padding).
// Requires slangc -fvk-use-scalar-layout and device scalarBlockLayout (Vulkan 1.2).
// Without scalar layout, SPIR-V puts color at offset 16 and validation fails BDA loads.
struct Vertex
{
    float3 pos;      // offset 0
    float3 color;    // offset 12
    float2 texCoord; // offset 24
};

// Matches C++ MeshletDesc (alignas(16), 32 B)
struct MeshletDesc
{
    uint vertexOffset;    // into meshletVertices
    uint triangleOffset;  // into meshletTriangles (byte / corner index)
    uint vertexCount;
    uint triangleCount;
    float4 boundingSphere; // xyz = center, w = radius (object space)
};

// ── Push constants (matches MeshPushData in push_data.hpp) ───────
// Layout (std430 / natural, 8-byte aligned device addresses):
//   +0  cameraAddress      uint64
//   +8  objectUbAddress    uint64
//  +16  vertices           uint64  (Vertex*)
//  +24  meshlets           uint64  (MeshletDesc*)
//  +32  meshletVertices    uint64  (uint*)
//  +40  meshletTriangles   uint64  (uint8*)
//  +48  firstMeshlet       uint
//  +52  meshletCount       uint
//  +56  texture            DescriptorHandle (uint2)
//  +64  samplerHandle      DescriptorHandle (uint2)
// Total: 72 bytes
struct MeshPushData
{
    uint64_t cameraAddress;
    uint64_t objectUbAddress;
    uint64_t vertices;
    uint64_t meshlets;
    uint64_t meshletVertices;
    uint64_t meshletTriangles;
    uint firstMeshlet;
    uint meshletCount;
    DescriptorHandle<Texture2D> texture;
    DescriptorHandle<SamplerState> samplerHandle;
};

[[vk::push_constant]] ConstantBuffer<MeshPushData> push;

struct MeshVertexOut
{
    float4 pos          : SV_Position;
    float3 fragColor    : COLOR;
    float2 fragTexCoord : TEXCOORD;
};

// Match meshopt build limits in assets_loader.cpp
static const uint kMaxMeshletVertices = 64;
static const uint kMaxMeshletTriangles = 126;
static const uint kGroupSize = 32;

// ── Mesh shader ──────────────────────────────────────────────────
[shader("mesh")]
[numthreads(kGroupSize, 1, 1)]
[outputtopology("triangle")]
void meshMain(
    uint gtid : SV_GroupThreadID,
    uint gid  : SV_GroupID,
    out vertices MeshVertexOut verts[kMaxMeshletVertices],
    out indices uint3 tris[kMaxMeshletTriangles])
{
    // One workgroup per meshlet in [firstMeshlet, firstMeshlet + meshletCount).
    if (gid >= push.meshletCount)
    {
        SetMeshOutputCounts(0, 0);
        return;
    }

    MeshletDesc* meshletTable = reinterpret<MeshletDesc*>(push.meshlets);
    MeshletDesc meshlet = meshletTable[push.firstMeshlet + gid];

    const uint vertCount = min(meshlet.vertexCount, kMaxMeshletVertices);
    const uint primCount = min(meshlet.triangleCount, kMaxMeshletTriangles);
    SetMeshOutputCounts(vertCount, primCount);

    if (vertCount == 0)
    {
        return;
    }

    ObjectUB* object = reinterpret<ObjectUB*>(push.objectUbAddress);
    CameraData* camera = reinterpret<CameraData*>(push.cameraAddress);
    Vertex* vertexTable = reinterpret<Vertex*>(push.vertices);
    uint* meshletVertTable = reinterpret<uint*>(push.meshletVertices);
    uint8_t* meshletTriTable = reinterpret<uint8_t*>(push.meshletTriangles);

    float4x4 mvp = mul(camera->proj, mul(camera->view, object->modelMatrix));

    // Cooperative vertex fetch + transform.
    for (uint vi = gtid; vi < vertCount; vi += kGroupSize)
    {
        uint globalVertexIndex = meshletVertTable[meshlet.vertexOffset + vi];
        Vertex v = vertexTable[globalVertexIndex];

        verts[vi].pos = mul(mvp, float4(v.pos, 1.0));
        verts[vi].fragColor = v.color;
        verts[vi].fragTexCoord = v.texCoord;
    }

    // Cooperative primitive assembly (packed uint8 local indices, 3 per triangle).
    // Host buffer is uint8_t[]; requires device shaderInt8 + storageBuffer8BitAccess.
    for (uint ti = gtid; ti < primCount; ti += kGroupSize)
    {
        uint base = meshlet.triangleOffset + ti * 3;
        uint i0 = uint(meshletTriTable[base + 0]);
        uint i1 = uint(meshletTriTable[base + 1]);
        uint i2 = uint(meshletTriTable[base + 2]);
        tris[ti] = uint3(i0, i1, i2);
    }
}

// ── Fragment shader ──────────────────────────────────────────────
[shader("fragment")]
float4 fragMain(MeshVertexOut vertIn) : SV_TARGET
{
    Texture2D texture = getDescriptorFromHandle(push.texture);
    SamplerState samplerState = getDescriptorFromHandle(push.samplerHandle);

    return texture.Sample(samplerState, vertIn.fragTexCoord);
}
