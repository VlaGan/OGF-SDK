cbuffer MatrixBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix proj;
    matrix lightViewProj;
};

cbuffer LightBuffer : register(b1)
{
    float3 lightDir;
    float  padding;
    float4 lightColor;
    float3 camPos;
    float  specularPower;
};

Texture2D tex : register(t0);
SamplerState samp : register(s0);

Texture2D<float> ShadowMap : register(t1);
SamplerState ShadowSampler : register(s1);
SamplerState LinearSampler : register(s2); // для Sample()

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
    float4 shadowPos : TEXCOORD3;
};

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;

    float4 worldPos = mul(float4(input.position, 1.0f), world);
    output.pos = mul(mul(worldPos, view), proj);
    output.normal = normalize(mul(input.normal, (float3x3)world));
    output.uv = input.texcoord;
    output.worldPos = worldPos.xyz;
    output.shadowPos = mul(worldPos, lightViewProj);
    
    return output;
}

float CalcShadowSoft(float4 shadowPos, float3 normal, float3 lightDirN)
{
    float3 projCoords = shadowPos.xyz / shadowPos.w;
    projCoords = projCoords * 0.5f + 0.5f;

    if (projCoords.x < 0 || projCoords.x > 1 || projCoords.y < 0 || projCoords.y > 1)
        return 1.0f;

    float bias = max(0.005 * (1.0 - dot(normal, lightDirN)), 0.001);
    float currentDepth = projCoords.z - bias;

    float shadow = 0.0f;
    float2 texelSize = float2(1.0 / 4096.0, 1.0 / 4096.0);

    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float2 offset = float2(x, y) * texelSize;
            float sampledDepth = ShadowMap.Sample(ShadowSampler, projCoords.xy + offset);
            if (currentDepth <= sampledDepth)
                shadow += 1.0f;
        }
    }

    shadow /= 9.0f;
    return shadow;
}

float SimpleShadow(float4 shadowPos)
{
    // Перетворення в простір текстури [0,1]
    float3 projCoords = shadowPos.xyz / shadowPos.w;
    projCoords.xy = projCoords.xy * 0.5f + 0.5f;
    projCoords.y = 1.0f - projCoords.y; // Інвертуємо Y для DirectX
    
    // Перевірка меж
    if (projCoords.x < 0 || projCoords.x > 1 ||
       projCoords.y < 0 || projCoords.y > 1)
        return 1.0f;
    
    // Зчитуємо глибину з ShadowMap
    float depthFromMap = ShadowMap.Sample(LinearSampler, projCoords.xy).r;
    float currentDepth = projCoords.z;
    
    // Просте порівняння глибин
    return (currentDepth <= depthFromMap + 0.005f) ? 1.0f : 0.5f;
}

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    //float shadow = SimpleShadow(input.shadowPos);
    float4 texColor = tex.Sample(samp, input.uv);
    return float4(texColor.rgb /* shadow*/, texColor.a);
}

float4 PSMainTest(PS_INPUT input) : SV_TARGET
{
    float3 norm = normalize(input.normal);
    float3 lightDirN = normalize(-lightDir);
    float NdotL = saturate(dot(norm, lightDirN));
    float shade = NdotL > 0.5 ? 1.0 : 1.0;

    float4 shadowCoord = input.shadowPos / input.shadowPos.w;
    shadowCoord.xy = shadowCoord.xy * 0.5f + 0.5f; // [-1,1] -> [0,1]
    float depthFromMap = ShadowMap.Sample(ShadowSampler, shadowCoord.xy).r;
    float bias = 0.0f;
    float shadow = (shadowCoord.z - bias) < depthFromMap ? 1.0f : 0.5f;
    
    //shadow = CalcShadowSoft(input.shadowPos, norm, lightDirN);
    shadow = SimpleShadow(input.shadowPos);
    shade *= shadow;

    float4 texColor = tex.Sample(samp, input.uv);
    if (texColor.a < 0.01f) discard;

    float3 color = texColor.rgb * shade;
    color = pow(color, 1.0 / 1.9);

    return float4(color, texColor.a);
}
