//----------------------------------------------------------------------------
//-- _defines.h
//-- VlaGan: basic solution defines and macroses
//----------------------------------------------------------------------------
#pragma once
#include <cstdint>
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
