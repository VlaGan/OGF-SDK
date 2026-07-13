// Vertex shader
Texture2D tex : register(t0);
SamplerState samp : register(s0);

cbuffer MatrixBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix proj;
};

cbuffer LightBuffer : register(b1)
{
    float3 lightDir;
    float  pad;        // щоб вирівняти до 16 байт
    float4 lightColor;
};

struct VS_INPUT {
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texcoord : TEXCOORD0;
};

struct VS_OUTPUT {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
    float3 normal : NORMAL;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;
    output.pos = mul(mul(mul(float4(input.position, 1.0f), world), view), proj);
    output.normal = normalize(mul(input.normal, (float3x3)world));
    output.uv = input.texcoord;
    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    float3 normal = normalize(input.normal);
    float diffuse = saturate(dot(normal, -lightDir));
    float4 texColor = tex.Sample(samp, input.uv);

    float3 ambient = float3(0.75f, 0.75f, 0.75f);
    return float4((ambient + diffuse) * texColor.rgb, texColor.a);
}
