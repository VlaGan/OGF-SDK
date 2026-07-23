//----------------------------------------------------------------------------
//-- COgfChunkWriter.cpp
//----------------------------------------------------------------------------
#include "COgfChunkWriter.h"
#include <cstring>

void COgfChunkWriter::open_chunk(uint32_t id)
{
    w_u32(id);
    m_sizeFieldStack.push_back(m_buffer.size());
    w_u32(0); // placeholder, patched in close_chunk()
}

void COgfChunkWriter::close_chunk()
{
    if (m_sizeFieldStack.empty())
        return; // unbalanced open/close - defensive, shouldn't happen if callers are correct

    const size_t sizeFieldPos = m_sizeFieldStack.back();
    m_sizeFieldStack.pop_back();

    const uint32_t payloadSize = static_cast<uint32_t>(m_buffer.size() - (sizeFieldPos + 4));
    std::memcpy(&m_buffer[sizeFieldPos], &payloadSize, sizeof(payloadSize));
}

void COgfChunkWriter::w_u8(uint8_t v) { m_buffer.push_back(v); }

void COgfChunkWriter::w_u16(uint16_t v)
{
    const size_t pos = m_buffer.size();
    m_buffer.resize(pos + 2);
    std::memcpy(&m_buffer[pos], &v, 2);
}

void COgfChunkWriter::w_u32(uint32_t v)
{
    const size_t pos = m_buffer.size();
    m_buffer.resize(pos + 4);
    std::memcpy(&m_buffer[pos], &v, 4);
}

void COgfChunkWriter::w_float(float v)
{
    const size_t pos = m_buffer.size();
    m_buffer.resize(pos + 4);
    std::memcpy(&m_buffer[pos], &v, 4);
}

void COgfChunkWriter::w_fvector2(const float v[2]) { w_float(v[0]); w_float(v[1]); }
void COgfChunkWriter::w_fvector3(const float v[3]) { w_float(v[0]); w_float(v[1]); w_float(v[2]); }

void COgfChunkWriter::w_stringZ(const std::string& s)
{
    w(s.data(), s.size());
    w_u8(0);
}

void COgfChunkWriter::w_stringA(const std::string& s)
{
    w(s.data(), s.size());
    w_u8(0x0A);
}

void COgfChunkWriter::w(const void* data, size_t bytes)
{
    if (bytes == 0) return;
    const size_t pos = m_buffer.size();
    m_buffer.resize(pos + bytes);
    std::memcpy(&m_buffer[pos], data, bytes);
}
