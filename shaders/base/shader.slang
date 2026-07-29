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
// Layout:
//   +0  cameraAddress    uint64_t  (8 bytes)
//   +8  ObjectUBAddress  uint64_t  (8 bytes)
//  +16  texture          uint2     (8 bytes – descriptor heap handle)
//  +24  samplerHandle    uint2     (8 bytes – descriptor heap handle)
// Total: 32 bytes
struct PushData
{
    uint64_t cameraAddress;
    uint64_t objectUBAddress;
    DescriptorHandle<Texture2D> texture;
    DescriptorHandle<SamplerState> samplerHandle;
};

[[vk::push_constant]] ConstantBuffer<PushData> push;

// ── Vertex I/O ───────────────────────────────────────────────────
struct VSInput
{
    float3 inPosition : POSITION;
    float3 inColor    : COLOR;
    float2 inTexCoord : TEXCOORD;
};

struct VSOutput
{
    float4 pos        : SV_Position;
    float3 fragColor  : COLOR;
    float2 fragTexCoord : TEXCOORD;
};

// ── Vertex shader ────────────────────────────────────────────────
[shader("vertex")]
VSOutput vertMain(VSInput input)
{
    ObjectUB*   object = reinterpret<ObjectUB*>(push.objectUBAddress);
    CameraData* camera = reinterpret<CameraData*>(push.cameraAddress);

    float4x4 model = object->modelMatrix;
    float4x4 view  = camera->view;
    float4x4 proj  = camera->proj;

    VSOutput output;
    output.pos =
        mul(proj,
        mul(view,
        mul(model, float4(input.inPosition, 1.0))));

    output.fragColor    = input.inColor;
    output.fragTexCoord = input.inTexCoord;
    return output;
}

// ── Fragment shader ──────────────────────────────────────────────
[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET
{
    Texture2D    texture      = getDescriptorFromHandle(push.texture);
    SamplerState samplerState = getDescriptorFromHandle(push.samplerHandle);

    return texture.Sample(samplerState, vertIn.fragTexCoord);
}
