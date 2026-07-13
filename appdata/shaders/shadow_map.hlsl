cbuffer MatrixBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix proj;
    matrix lightViewProj;
};

struct VS_INPUT
{
    float3 position : POSITION;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;

    float4 worldPos = mul(float4(input.position, 1.0f), world);
    output.pos = mul(worldPos, lightViewProj);

    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    //float shadowDepth = input.pos.z / input.pos.w;
    //return float4(shadowDepth, shadowDepth, shadowDepth, 1.0);
    return input.pos.z / input.pos.w;
}
