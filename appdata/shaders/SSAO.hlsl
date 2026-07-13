//----------------------------------------------------------------------------
// SSAO.hlsl
//----------------------------------------------------------------------------

Texture2D gPosTex : register(t0); // view–space position xyz, 1
Texture2D gNormTex : register(t1); // view–space normal   xyz, 1
Texture2D gNoiseTex : register(t2); // 256×256 RG  ‑1..1,   tiled
SamplerState gSampler : register(s0);

cbuffer SSAO_CB : register(b0)
{
    float4 kernel[64]; // kernel directions
    float4 projInfo; // елементи матриці projection для відновлення координат
    float2 invTexSize;
    float radius;
    float bias;
};

// ===========  FULLSCREEN‑QUAD VS  ==========================================
struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VS_OUT VSMain(uint id : SV_VertexID)
{
    float2 pos[6] =
    {
        float2(-1, 1), float2(1, 1), float2(-1, -1),
        float2(-1, -1), float2(1, 1), float2(1, -1)
    };
    float2 uv[6] =
    {
        float2(0, 0), float2(1, 0), float2(0, 1),
        float2(0, 1), float2(1, 0), float2(1, 1)
    };

    VS_OUT o;
    o.pos = float4(pos[id], 0, 1);
    o.uv = uv[id];
    return o;
}

// ===========  HELPERS  =====================================================
float3 randomVec(float2 uv)
{
    return normalize(float3(gNoiseTex.Sample(gSampler, uv * 256).xy, 0.0));
}

// побудова TBN‐матриці у view‑space
float3x3 makeTBN(float3 N, float3 rand)
{
    float3 T = normalize(rand - N * dot(rand, N));
    float3 B = cross(N, T);
    return float3x3(T, B, N);
}

// ===========  PIXEL SHADER  ===============================================
float4 PSMain(VS_OUT In) : SV_TARGET
{
    float3 fragPos = gPosTex.Sample(gSampler, In.uv).xyz; // view‑space
    float3 normal = normalize(gNormTex.Sample(gSampler, In.uv).xyz);

    // Tangent‑space basis (view‑space)
    float3 randDir = randomVec(In.uv);
    float3x3 TBN = makeTBN(normal, randDir);

    float occlusion = 0.0;

    [unroll]         // 64 рази
    for (int i = 0; i < 64; ++i)
    {
        float3 sampVec = mul(kernel[i].xyz, TBN); // into view‑space
        float3 samplePos = fragPos + sampVec * radius;

        // проектуємо samplePos назад у screen‑UV, щоб витягти depth
        float2 offsetUV = samplePos.xy / -samplePos.z; // (x/z , y/z)  perspective divide in view‑space
        offsetUV = offsetUV * 0.5f + 0.5f; // NDC→UV

        // відкидаємо семпли поза екрана
        if (offsetUV.x < 0 || offsetUV.x > 1 ||
            offsetUV.y < 0 || offsetUV.y > 1)
            continue;

        float sampleDepth = gPosTex.Sample(gSampler, offsetUV).z;

        // якщо перед нами щось ближче (depth < з bias), рахуємо як «закрито»
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - occlusion / 64.0; // нормалізуємо; 1 = світло, 0 = темно
    
    return float4(occlusion, occlusion, occlusion, 1.0);
}
