//----------------------------------------------------------------------------
//-- COgfChunkWriter.h
//-- Minimal write-only counterpart to COgfChunkedReader: builds a buffer of
//-- repeating { u32 id; u32 size; u8 data[size] } records, with support for
//-- nesting (a chunk's payload can itself contain chunks) via open_chunk()/
//-- close_chunk(). Sizes are backpatched in close_chunk() once the payload
//-- length is known, so containers (OGF_CHILDREN, OGF_S_MOTIONS) just need
//-- open_chunk/close_chunk called for each sub-record in order.
//----------------------------------------------------------------------------
#pragma once
#include <cstdint>
#include <string>
#include <vector>

class COgfChunkWriter
{
public:
    //-- writes the chunk id + a 0 size placeholder, remembers where the size
    //-- field is so close_chunk() can patch it in once the payload is known
    void open_chunk(uint32_t id);

    //-- backpatches the most recently opened (and not yet closed) chunk's
    //-- size field with (current buffer size - payload start)
    void close_chunk();

    void w_u8(uint8_t v);
    void w_u16(uint16_t v);
    void w_u32(uint32_t v);
    void w_float(float v);
    void w_fvector2(const float v[2]);
    void w_fvector3(const float v[3]);

    //-- null-terminated string (matches COgfChunkedReader::r_stringZ)
    void w_stringZ(const std::string& s);
    //-- '\n' (0x0A) terminated string, used for motion marks (matches r_stringA)
    void w_stringA(const std::string& s);

    void w(const void* data, size_t bytes);

    const std::vector<uint8_t>& buffer() const { return m_buffer; }
    size_t size() const { return m_buffer.size(); }

private:
    std::vector<uint8_t> m_buffer;
    std::vector<size_t> m_sizeFieldStack; // positions of pending size fields, one per open_chunk not yet closed
};
