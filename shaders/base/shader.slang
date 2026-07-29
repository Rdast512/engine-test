struct UBO
{
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

struct InstanceData
{
    float4x4 modelMatrix;
    float4x4 prevModelMatrix;

    // uint64_t vertexBufferAddress;
    // uint64_t indexBufferAddress;

    float4 boundingSphere;

    uint32_t materialID;
    uint32_t instanceFlags;
    uint32_t baseVertex;
    uint32_t baseIndex;
};

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

struct PushData
{
    uint64_t uboAddress;
    DescriptorHandle<Texture2D> texture;
    DescriptorHandle<SamplerState> samplerHandle;
};

[[vk::push_constant]] ConstantBuffer<PushData> push;

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
    UBO* ubo = reinterpret<UBO*>(push.uboAddress);

    float4x4 model = ubo->model;
    float4x4 view  = ubo->view;
    float4x4 proj  = ubo->proj;

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
    // Heap handles are lowered to uint2 indices on SPIR-V descriptor-heap path.
    Texture2D texture = getDescriptorFromHandle(push.texture);
    SamplerState samplerState = getDescriptorFromHandle(push.samplerHandle);
    
    return texture.Sample(samplerState, vertIn.fragTexCoord);
}