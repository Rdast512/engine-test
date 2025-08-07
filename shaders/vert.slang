struct UniformBufferObject {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

struct VSInput {
    float3 inPosition : POSITION;
    float3 inColor : COLOR0;
    float2 inTexCoord : TEXCOORD0;
};

struct VSOutput {
    float4 position : SV_Position;
    float3 fragColor : COLOR0;
    float2 fragTexCoord : TEXCOORD0;
};

[[vk::binding(0, 0)]]
ConstantBuffer<UniformBufferObject> ubo;

[shader("vertex")]
VSOutput main(VSInput input)
{
    VSOutput output;
    output.position = mul(mul(mul(ubo.proj, ubo.view), ubo.model), float4(input.inPosition, 1.0));
    output.fragColor = input.inColor;
    output.fragTexCoord = input.inTexCoord;
    return output;
}