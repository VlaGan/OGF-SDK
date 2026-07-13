Texture2D sceneTex : register(t0); // Твоє зображення після освітлення
Texture2D aoTex : register(t1); // SSAO-текстура
SamplerState samp : register(s0);

cbuffer BlurParams : register(b0)
{
    float2 invTexSize; // (1.0 / width, 1.0 / height)
    float aoPower; // наскільки сильно множити AO (0.0...1.0)
    float aoIntensity; // інтенсивність AO (0.0...1.0)
};


float blurAO(float2 uv)
{
    float weights[5] = { 0.204164f, 0.304005f, 0.093913f, 0.010381f, 0.000336f };

    float ao = aoTex.Sample(samp, uv).r * weights[0];

    for (int i = 1; i < 5; ++i)
    {
        float2 offset = float2(i, 0) * invTexSize;
        ao += aoTex.Sample(samp, uv + offset).r * weights[i];
        ao += aoTex.Sample(samp, uv - offset).r * weights[i];

        offset = float2(0, i) * invTexSize;
        ao += aoTex.Sample(samp, uv + offset).r * weights[i];
        ao += aoTex.Sample(samp, uv - offset).r * weights[i];
    }

    return ao;
}

float4 PSMain(float4 pos : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET
{
    float3 color = sceneTex.Sample(samp, uv).rgb;

    float ao = blurAO(uv);
    ao = saturate(lerp(aoPower, 1.0, ao)); // від 0.5 до 1.0 наприклад

    color *= lerp(1.0, ao, aoIntensity); // інтенсивність впливу AO
    return float4(color, 1.0);
}