struct PSInput {
    float4 position : SV_Position;
    float3 fragColor : COLOR0;
    float2 fragTexCoord : TEXCOORD0;
};

[[vk::binding(1, 0)]]
Texture2D<float4> texSampler;
[[vk::binding(1, 0)]]
SamplerState texSamplerState;

[shader("fragment")]
float4 main(PSInput input) : SV_Target
{
    return texSampler.Sample(texSamplerState, input.fragTexCoord);
}