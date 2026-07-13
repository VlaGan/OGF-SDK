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

cbuffer Bones : register(b2)
{
    matrix boneMatrices[256]; // або MAX_BONES
};

Texture2D tex : register(t0);
SamplerState samp : register(s0);

Texture2D<float> ShadowMap : register(t1);
SamplerComparisonState ShadowSampler : register(s1);
SamplerState LinearSampler : register(s2); // для Sample()

struct VS_INPUT {
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texcoord : TEXCOORD0;
    int4   boneIDs  : BLENDINDICES;
    float4 weights  : BLENDWEIGHT;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
    float3 normal : TEXCOORD2;
    float4 shadowPos : TEXCOORD3; // нове
};

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;

    // Обчислюємо skinned position та normal
    float4 skinnedPos = float4(0,0,0,0);
    float3 skinnedNorm = float3(0,0,0);
    for (int i = 0; i < 4; ++i)
    {
        int id = input.boneIDs[i];
        float w = input.weights[i];
        if (w > 0)
        {
            matrix bm = boneMatrices[id];
            float4 p = mul(float4(input.position, 1.0f), bm);
            skinnedPos += p * w;

            float3 n = mul(input.normal, (float3x3)bm); // або окремо з врахуванням трансформи без трансляції
            skinnedNorm += n * w;
        }
    }
    skinnedNorm = normalize(skinnedNorm);

    // Далі звичайне трансформування у світ: world може вже включати модельну трансформацію окремо
    float4 worldPos = skinnedPos; //mul(skinnedPos, world);
    output.pos = mul(mul(worldPos, view), proj);
    output.normal = skinnedNorm; //normalize(mul(skinnedNorm, (float3x3) world));
    output.uv = input.texcoord;

    //output.shadowPos = mul(worldPos, lightViewProj);
    output.shadowPos = mul(worldPos, lightViewProj);

    //output.shadowPos = mul(skinnedPos, world);
    //output.shadowPos = mul(output.shadowPos, lightViewProj);

    return output;
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



// Константи для PCSS
//#define USE_PCSS
#define BLOCKER_SEARCH_SAMPLES 16
#define PCF_SAMPLES 16
#define LIGHT_SIZE 0.05  // Розмір джерела світла (для м'яких тіней)

// Допоміжні функції
float GetShadowMapDepth(float2 uv)
{
    return ShadowMap.Sample(LinearSampler, uv).r;
}

static const float2 PoissonDisk[16] =
{
    float2(-0.94201624, -0.39906216),
    float2(0.94558609, -0.76890725),
    float2(-0.094184101, -0.92938870),
    float2(0.34495938, 0.29387760),
    float2(-0.91588581, 0.45771432),
    float2(-0.81544232, -0.87912464),
    float2(-0.38277543, 0.27676845),
    float2(0.97484398, 0.75648379),
    float2(0.44323325, -0.97511554),
    float2(0.53742981, -0.47373420),
    float2(-0.26496911, -0.41893023),
    float2(0.79197514, 0.19090188),
    float2(-0.24188840, 0.99706507),
    float2(-0.81409955, 0.91437590),
    float2(0.19984126, 0.78641367),
    float2(0.14383161, -0.14100790)
};

// Пошук блокерів (об'єктів, що затуляють світло)
float FindBlockerDistance(float2 uv, float depth)
{
    float blockerSum = 0.0;
    int numBlockers = 0;
    
    for (int i = 0; i < BLOCKER_SEARCH_SAMPLES; i++)
    {
        float2 offset = PoissonDisk[i] * 0.01;
        float shadowDepth = GetShadowMapDepth(uv + offset);
        
        if (shadowDepth < depth)
        {
            blockerSum += shadowDepth;
            numBlockers++;
        }
    }
    
    return (numBlockers > 0) ? (blockerSum / numBlockers) : -1.0;
}

// PCSS - основний алгоритм
float PCSS_Shadow(float4 shadowPos, float3 normal)
{
    float3 projCoords = shadowPos.xyz / shadowPos.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    projCoords.y = 1.0 - projCoords.y;
    
    if (projCoords.x < 0 || projCoords.x > 1 ||
        projCoords.y < 0 || projCoords.y > 1)
        return 1.0;
    
    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - dot(normal, -lightDir)), 0.001);
    currentDepth -= bias;
    
    // Крок 1: Пошук блокерів
    float avgBlockerDepth = FindBlockerDistance(projCoords.xy, currentDepth);
    
    // Якщо блокерів немає - немає тіні
    if (avgBlockerDepth < 0.0)
        return 1.0;
        
    // Крок 2: Розрахунок розміру півтіні
    float penumbraSize = (currentDepth - avgBlockerDepth) / avgBlockerDepth * LIGHT_SIZE;
    penumbraSize = clamp(penumbraSize, 0.001, 0.1);
    
    // Крок 3: PCF фільтрація з динамічним розміром ядра
    float shadow = 0.0;
    for (int i = 0; i < PCF_SAMPLES; i++)
    {
        float2 offset = PoissonDisk[i] * penumbraSize;
        shadow += GetShadowMapDepth(projCoords.xy + offset) < currentDepth ? 0.0 : 1.0;
    }
    
    return shadow / PCF_SAMPLES;
}

// Оновлена функція тіні
float CalculateShadow(float4 shadowPos, float3 normal)
{
#ifdef USE_PCSS
        return PCSS_Shadow(shadowPos, normal);
#else
        // Простий варіант для порівняння
    float3 projCoords = shadowPos.xyz / shadowPos.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    projCoords.y = 1.0 - projCoords.y;
        
    float depth = GetShadowMapDepth(projCoords.xy);
    float currentDepth = projCoords.z - 0.005;
    return (currentDepth <= depth) ? 1.0 : 0.5;
#endif
}

//#define RIM_LIGHT
//#define RIM_ONLY_LIGHT
float3 RimLight(float NdotL, float3 wp, float3 norm)
{
    float3 viewDir = normalize(camPos - wp);
    float rim = 1.0 - saturate(dot(viewDir, normalize(norm)));
    rim = pow(rim, 2.0);

    //-- color && intensity (need to export and config using imgui)
    float3 rimColor = float3(0.0, 1.0, 0.0);
    float rimIntensity = 0.5;
#ifdef RIM_ONLY_LIGHT
    return NdotL > 0.5 ? rimColor * rim * rimIntensity : float3(0.0, 0.0, 0.0);
#else
    return rimColor * rim * rimIntensity;
#endif
}

//-- oh shit
float4 PSMain(PS_INPUT input) : SV_TARGET
{
    float4 texColor = tex.Sample(samp, input.uv);
    //if (texColor.a < 0.01)
    //    discard;

    float3 normal = normalize(input.normal);
    float shadow = CalculateShadow(input.shadowPos, normal);
    
    // Додаткове освітлення (необов'язково)
    float3 lightDirN = normalize(-lightDir);
    float NdotL = saturate(dot(normal, lightDirN));
    float lighting = NdotL * 0.2 + 0.2; // Ambient
    
    float shade = pow(saturate(dot(normal, lightDirN)), 1.0);
    shade = saturate(shade * 0.5 + 0.5);
    
    float3 finalColor = texColor.rgb * shadow;
    
#ifdef RIM_LIGHT
    finalColor += RimLight(NdotL, input.worldPos, input.normal);
#endif
    
    // Gamma correction
    finalColor = pow(finalColor, 1.0 / 1.3);

    return float4(finalColor, texColor.a); // Gamma correction
    
}
