struct PSInput {
    float4 position : SV_Position;   // Required for pixel shader input
    float3 color : COLOR;           // Matches the vertex shader output
};

[shader("pixel")]
float4 main(PSInput input) : SV_Target
{
    return float4(input.color, 1.0);
}