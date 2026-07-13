cbuffer MatrixBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix proj;
};

struct VS_INPUT {
    float3 position : POSITION;
    float3 normal   : NORMAL;
};

struct VS_OUTPUT {
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;
    float4 worldPos = mul(float4(input.position, 1.0f), world);
    output.pos = mul(mul(worldPos, view), proj);

    // Трансформація нормалі
    output.normal = mul(input.normal, (float3x3)world);
    return output;
}

cbuffer LightBuffer : register(b1)
{
    float3 lightDir;
    float  pad;        // щоб вирівняти до 16 байт
    float4 lightColor;
};

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    float3 normal = normalize(input.normal);
    float diffuse = saturate(dot(normal, -lightDir));

    return diffuse * lightColor;
}

