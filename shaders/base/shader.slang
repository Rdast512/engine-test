struct UBO
{
    float4x4 model;
    float4x4 view;
    float4x4 proj;
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