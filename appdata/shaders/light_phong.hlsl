cbuffer MatrixBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix proj;
}

cbuffer LightBuffer : register(b1)
{
    float3 lightDir;
    float  padding;
    float4 lightColor;
    float3 camPos;
    float  specularPower;
}

Texture2D tex : register(t0);
SamplerState samp : register(s0);

struct VS_INPUT {
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texcoord : TEXCOORD0;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
    float3 normal : TEXCOORD2;
};

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;

    float4 worldPos = mul(float4(input.position, 1.0f), world);
    output.pos = mul(mul(worldPos, view), proj);
    output.worldPos = worldPos.xyz;

    float3 normalW = mul((float3x3)world, input.normal);
    output.normal = normalize(normalW);

    output.uv = input.texcoord;
    return output;
}

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    float3 norm = normalize(input.normal);
    float3 lightDirN = normalize(-lightDir);

    // -- Ambient
    float3 ambient = float3(0.75f, 0.75f, 0.75f);

    // -- Diffuse
    float diff = saturate(dot(norm, lightDirN));

    // -- Specular
    float3 viewDir = normalize(camPos - input.worldPos);
    float3 halfway = normalize(lightDirN + viewDir);
    float spec = pow(saturate(dot(norm, halfway)), specularPower);

    // -- Texture
    float4 texColor = tex.Sample(samp, input.uv);

    float3 finalColor = texColor.rgb * (ambient + diff) + spec;
    return float4(finalColor, texColor.a);
}
