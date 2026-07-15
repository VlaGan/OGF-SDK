//----------------------------------------------------------------------------
//-- _defines.h
//-- VlaGan: basic solution defines and macroses
//----------------------------------------------------------------------------
#pragma once
#include <cstdint>
#include <DirectXMath.h>
#include "Core/CLog.h"

#define RELEASE(ptr) if (ptr) { ptr->Release(); ptr = nullptr;}
#define INL inline //-- IC conflicts with llama-cpp

#define VPUSH2(vector) vector.x, vector.y 
#define VPUSH3(vector) vector.x, vector.y, vector.z
#define VPUSH4(vector) vector.x, vector.y, vector.z, vector.w

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;


static float deg2rad(float deg) { return deg * (DirectX::XM_PI / 180.f); }
static float rad2deg(float rad) { return rad * (180.f / DirectX::XM_PI); }
