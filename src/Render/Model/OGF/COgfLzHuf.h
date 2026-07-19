//----------------------------------------------------------------------------
//-- COgfLzHuf.h
//-- Decompression side only of X-Ray's "LzHuf" chunk compression (a classic
//-- adaptive-Huffman + LZSS scheme; OGSR-Engine: xrCore/LzHuf.cpp).
//--
//-- Some .ogf/.omf chunks are stored compressed: the chunk id has its top bit
//-- (0x80000000) set, and the chunk's raw bytes are this format instead of
//-- the plain chunk content. COgfChunkedReader::open_chunk() calls this
//-- transparently whenever it sees that bit, so callers never need to think
//-- about compression at all.
//----------------------------------------------------------------------------
#pragma once
#include <cstdint>
#include <vector>

//-- Decompresses `src` (compressed payload, `srcSize` bytes) into `out`.
//-- The decompressed size is self-described by the first 4 bytes of the
//-- compressed stream (matches OGSR-Engine's format exactly) - `out` is
//-- resized to that size automatically. Returns false on any corruption
//-- (declared size 0, or the bitstream running out of input).
bool OgfDecompressLZ(const uint8_t* src, size_t srcSize, std::vector<uint8_t>& out);
