//----------------------------------------------------------------------------
//-- _render_structs.h
//-- VlaGan: vector implementation
//----------------------------------------------------------------------------
#pragma once

#include <DirectXMath.h>

//-- max bones num that can influence on mesh
#define MAX_BONE_INFLUENCE 4

//-- maximum bones in skeleton
#define MAX_BONES 256

//-- vertex for model defining
struct Vertex {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 texcoord;

    int boneIDs[MAX_BONE_INFLUENCE] = { 0,0,0,0 };
    float weights[MAX_BONE_INFLUENCE] = { 0.f,0.f,0.f,0.f };
};

//-- const buffer struct for shaders
struct MatrixBuffer
{
    DirectX::XMMATRIX world;
    DirectX::XMMATRIX view;
    DirectX::XMMATRIX proj;
    DirectX::XMMATRIX lightVP;
};

//-- other buffer structs
struct FXAAParams {
    DirectX::XMFLOAT2 invTexSize;
    DirectX::XMFLOAT2 padding;
};

struct LightCB {
    DirectX::XMFLOAT3 dir;
    float pad0;
    DirectX::XMFLOAT4 color;
    DirectX::XMFLOAT3 camPos;
    float specularPower;
};


struct SSAO_CB {
    DirectX::XMFLOAT4 kernel[64];     // 64 sample dirs inside view-space
    DirectX::XMFLOAT4 projInfo;       // proj matrix 2-3 row elements
    DirectX::XMFLOAT2 invTexSize;     // 1 / ssaort_width, 1 / ssaort_height
    float radius;                     // max sample ray lenght 
    float bias;                       // min dist vs self-shadow
};

struct SSAO_Blur {
    DirectX::XMFLOAT2 invTexSize; // (1.0 / width, 1.0 / height)
    float  aoPower;    // наскільки сильно множити AO (0.0...1.0)
    float  aoIntensity; // інтенсивність AO (0.0...1.0)
};
