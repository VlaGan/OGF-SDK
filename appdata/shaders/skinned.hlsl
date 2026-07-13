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

//-- oh shit
float4 PSMain(PS_INPUT input) : SV_TARGET
{
    float4 texColor = tex.Sample(samp, input.uv);
    //if (texColor.a < 0.01)
    //    discard;
    float3 normal = normalize(input.normal);
    
    // Додаткове освітлення (необов'язково)
    float3 lightDirN = normalize(-lightDir);
    float NdotL = saturate(dot(normal, lightDirN));
    float lighting = NdotL * 0.2 + 0.2; // Ambient
    
    float shade = pow(saturate(dot(normal, lightDirN)), 1.0);
    shade = saturate(shade * 0.5 + 0.5);
    
    float3 finalColor = texColor.rgb;
    
    // Gamma correction
    finalColor = pow(finalColor, 1.0 / 1.3);

    return float4(finalColor, texColor.a); // Gamma correction
    
}
