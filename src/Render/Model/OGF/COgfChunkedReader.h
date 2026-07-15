//----------------------------------------------------------------------------
//-- COgfChunkedReader.h
//-- Minimal read-only reader for X-Ray's generic chunked binary format:
//--   repeating records of { u32 id; u32 size; u8 data[size]; }
//-- possibly nested (a chunk's payload can itself contain chunks).
//--
//-- This intentionally mirrors the small subset of OGSR-Engine's IReader
//-- API that the .ogf loader needs (find_chunk/open_chunk/r_u32/r_stringZ...),
//-- so the loading code in COgfLoader.cpp reads like the OGSR-Engine
//-- original it was ported from.
//----------------------------------------------------------------------------
#pragma once
#include <cstdint>
#include <cstring>
#include <string>

class COgfChunkedReader
{
public:
    COgfChunkedReader() = default;
    COgfChunkedReader(const uint8_t* data, size_t size) : m_data(data), m_size(size) {}

    bool valid() const { return m_data != nullptr; }

    //-- Search for a chunk with the given id among the chunks stored at the
    //-- top level of *this* reader's buffer, and if found, point `out` at
    //-- that chunk's payload (position 0 = start of payload).
    //-- Does not modify `this`'s own cursor - can be called repeatedly / out of order.
    bool open_chunk(uint32_t id, COgfChunkedReader& out) const;

    //-- sequential primitive reads from this reader's own cursor
    uint8_t r_u8();
    uint16_t r_u16();
    uint32_t r_u32();
    float r_float();
    void r_fvector2(float out[2]);
    void r_fvector3(float out[3]);
    std::string r_stringZ();
    void r(void* dst, size_t bytes);
    void skip(size_t bytes);

    const uint8_t* pointer() const { return (m_pos <= m_size) ? m_data + m_pos : nullptr; }
    size_t remaining() const { return (m_pos <= m_size) ? m_size - m_pos : 0; }
    size_t size() const { return m_size; }

private:
    const uint8_t* m_data = nullptr;
    size_t m_size = 0;
    size_t m_pos = 0;
};
