cbuffer Constants : register(b0)
{
    matrix ViewProj;
};

struct VSInput
{
    float3 Pos : POSITION;
    float4 Color : COLOR;
};

struct PSInput
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    output.Pos = mul(ViewProj, float4(input.Pos, 1.0)); 
    output.Color = input.Color;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(input.Color.rgb, 1.0);
}
