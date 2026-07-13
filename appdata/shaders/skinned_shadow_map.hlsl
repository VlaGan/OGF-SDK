cbuffer MatrixBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix proj;
    matrix lightViewProj;
}

cbuffer LightBuffer : register(b1)
{
    float3 lightDir;
    float  padding;
    float4 lightColor;
    float3 camPos;
    float  specularPower;
}

cbuffer Bones : register(b2)
{
    matrix bones[256];
};


struct VS_INPUT {
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texcoord : TEXCOORD0;
    int4   boneIDs  : BLENDINDICES;
    float4 weights  : BLENDWEIGHT;
};

struct VS_OUTPUT {
    float4 pos : SV_POSITION;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;

    float4 skinnedPos = float4(0, 0, 0, 0);
    for (int i = 0; i < 4; ++i)
    {
        int id = input.boneIDs[i];
        float w = input.weights[i];
        if (w > 0)
        {
            float4 p = mul(float4(input.position, 1.0f), bones[id]);
            skinnedPos += p * w;
        }
    }

    float4 worldPos = skinnedPos;
    //mul(skinnedPos, world);
    output.pos = mul(worldPos, lightViewProj);
    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_TARGET
{
    // return depth
    //float3 c = input.pos.xyz / input.pos.w;
    //return float4(c.z, c.z, c.z, 1.0); // глибина
    
    //float d = saturate((input.pos.z / input.pos.w - 0.1f) / (10.0f - 0.1f));

    //return float4(d, d, d, 1.0);
    
    return input.pos.z / input.pos.w;
}