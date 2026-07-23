cbuffer MatrixBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix proj;
    matrix lightViewProj;
};

cbuffer Bones : register(b2)
{
    matrix boneMatrices[256]; // або MAX_BONES
};

cbuffer OutlineParams : register(b3)
{
    float thickness;
    float3 outlineColor;
};


struct VS_INPUT {
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texcoord : TEXCOORD0;
    int4   boneIDs  : BLENDINDICES;
    float4 weights  : BLENDWEIGHT;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
};

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;

    // Skinning
    float4 skinnedPos = float4(0, 0, 0, 0);
    float3 skinnedNorm = float3(0, 0, 0);
    for (int i = 0; i < 4; ++i)
    {
        int id = input.boneIDs[i];
        float w = input.weights[i];
        if (w > 0)
        {
            matrix bm = boneMatrices[id];
            skinnedPos += mul(float4(input.position, 1.0f), bm) * w;
            skinnedNorm += mul(input.normal, (float3x3) bm) * w;
        }
    }

    skinnedNorm = normalize(skinnedNorm);

    //-- no mul(..., world) - boneMatrices already have global transform
    float3 worldPos = skinnedPos.xyz + skinnedNorm * thickness;

    float4 viewPos = mul(float4(worldPos, 1.0f), view);
    output.pos = mul(viewPos, proj);

    return output;
}

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    return float4(outlineColor, 1.0f);
}
