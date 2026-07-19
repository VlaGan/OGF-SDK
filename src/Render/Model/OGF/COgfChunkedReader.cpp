//----------------------------------------------------------------------------
//-- COgfChunkedReader.cpp
//----------------------------------------------------------------------------
#include "COgfChunkedReader.h"
#include "COgfLzHuf.h" // OgfDecompressLZ is kept available but NOT auto-triggered - see note below

bool COgfChunkedReader::open_chunk(uint32_t id, COgfChunkedReader& out) const
{
    if (!m_data)
        return false;

    size_t pos = 0;
    while (pos + 8 <= m_size)
    {
        uint32_t chunkId = 0, chunkSize = 0;
        std::memcpy(&chunkId, m_data + pos, 4);
        std::memcpy(&chunkSize, m_data + pos + 4, 4);
        pos += 8;

        //-- The top bit of the chunk id (0x80000000, X-Ray's CFS_CompressMark
        //-- in xrCore/FS.h) is masked off for comparison purposes only.
        //--
        //-- IMPORTANT: this bit is NOT treated as "this payload is
        //-- LZ-compressed" here, even though that's what it means in other
        //-- X-Ray contexts (.db archives, compiled level geometry). Real
        //-- model files (.ogf/.omf, both vanilla and Gunslinger-modified
        //-- ones with their own 16-bit-id chunk header variant) have been
        //-- observed setting this bit on plain, uncompressed OGF_HEADER /
        //-- OGF_S_DESC chunks - decompressing them produces garbage
        //-- (verified: format_version/type came out as nonsense numbers on
        //-- a real wpn_*_hud.ogf, while reading the SAME bytes raw gave a
        //-- perfectly sane format_version=4/type=3). So: mask it, don't act
        //-- on it. OgfDecompressLZ() is kept around in case a genuinely
        //-- compressed chunk ever turns up, but nothing calls it right now.
        constexpr uint32_t kCompressMark = 0x80000000u;
        const uint32_t maskedId = chunkId & ~kCompressMark;

        if (pos + chunkSize > m_size)
            break; // truncated / corrupted file, stop scanning

        if (maskedId == id)
        {
            out = COgfChunkedReader(m_data + pos, chunkSize);
            return true;
        }

        pos += chunkSize;
    }
    return false;
}

uint8_t COgfChunkedReader::r_u8()
{
    uint8_t v = 0;
    if (m_pos + 1 <= m_size)
        v = m_data[m_pos];
    m_pos += 1;
    return v;
}

uint16_t COgfChunkedReader::r_u16()
{
    uint16_t v = 0;
    if (m_pos + 2 <= m_size)
        std::memcpy(&v, m_data + m_pos, 2);
    m_pos += 2;
    return v;
}

uint32_t COgfChunkedReader::r_u32()
{
    uint32_t v = 0;
    if (m_pos + 4 <= m_size)
        std::memcpy(&v, m_data + m_pos, 4);
    m_pos += 4;
    return v;
}

float COgfChunkedReader::r_float()
{
    float v = 0.f;
    if (m_pos + 4 <= m_size)
        std::memcpy(&v, m_data + m_pos, 4);
    m_pos += 4;
    return v;
}

void COgfChunkedReader::r_fvector2(float out[2])
{
    out[0] = r_float();
    out[1] = r_float();
}

void COgfChunkedReader::r_fvector3(float out[3])
{
    out[0] = r_float();
    out[1] = r_float();
    out[2] = r_float();
}

std::string COgfChunkedReader::r_stringZ()
{
    std::string s;
    while (m_pos < m_size)
    {
        const char c = static_cast<char>(m_data[m_pos++]);
        if (c == '\0')
            break;
        s.push_back(c);
    }
    return s;
}

void COgfChunkedReader::r(void* dst, size_t bytes)
{
    if (m_pos + bytes <= m_size)
        std::memcpy(dst, m_data + m_pos, bytes);
    else
        std::memset(dst, 0, bytes);
    m_pos += bytes;
}

void COgfChunkedReader::skip(size_t bytes)
{
    m_pos += bytes;
}
