//======  Constant buffers  ===================================================
cbuffer MatrixBuffer : register(b0)
{
    matrix world; // модель‑>world
    matrix view; // world -> view
    matrix proj; // view  -> clip
    matrix lightViewProj;
};

cbuffer Bones : register(b2)
{
    matrix boneMatrices[256]; // максимум 256 кісток
};

//======  входи/виходи  =======================================================
struct VS_IN
{
    float3 pos      : POSITION;
    float3 normal   : NORMAL;
    float2 texcoord : TEXCOORD0;
    int4   boneIDs  : BLENDINDICES;
    float4 weights  : BLENDWEIGHT;
};

struct VS_OUT
{
    float4 posCS        : SV_POSITION;   // позиція у clip‑space (для растеризатора)
    float3 posWS        : TEXCOORD0;     // позиція у world‑space  (до PS)
    float3 normalWS     : TEXCOORD1;     // нормаль  у world‑space  (до PS)
};

//======  Vertex shader  ======================================================
VS_OUT VSMain (VS_IN v)
{
    //-----  skinned vertex  ---------------------------------------------------
    float4 skinnedPos   = 0;
    float3 skinnedNorm  = 0;

    [unroll]             // 4 ваги на вершину
    for (int i = 0; i < 4; ++i)
    {
        const uint  id =  v.boneIDs[i];
        const float w  =  v.weights[i];
        if (w == 0) continue;

        float4x4 M = boneMatrices[id];
        skinnedPos  += mul(float4(v.pos,    1.0), M) * w;
        skinnedNorm += mul((float3x3)M, v.normal) * w;
    }

    skinnedNorm = normalize(skinnedNorm);

    //-----  world / clip transforms  -----------------------------------------
    float4 posWS = mul(skinnedPos, world);
    float4 posCS = mul(mul(posWS, view), proj);

    VS_OUT o;
    o.posCS     = posCS;
    o.posWS     = posWS.xyz;
    o.normalWS  = normalize(mul((float3x3)world, skinnedNorm));  // якщо world містить масштаб/ротацію
    return o;
}

//======  Pixel shader  =======================================================
struct PS_OUT
{
    float4 gPos   : SV_Target0;   // world position
    float4 gNorm  : SV_Target1;   // world normal, 0‑1 encoded
};

PS_OUT PSMain (VS_OUT i)
{
    PS_OUT o;

    // 1) позиція –– просто world XYZ, A=1
    o.gPos  = float4(i.posWS, 1.0);

    // 2) нормаль –– (-1…1) -> (0…1)
    float3 n = normalize(i.normalWS);
    o.gNorm = float4(n * 0.5f + 0.5f, 1.0);

    return o;
}
