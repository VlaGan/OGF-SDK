//----------------------------------------------------------------------------
//-- CConstantBuffer.h
//-- VlaGan: Basic constant buufer class .header
//----------------------------------------------------------------------------
#pragma once
#include <d3d11.h>
#include <string>

class CConstantBuffer {
public:
    CConstantBuffer();
    ~CConstantBuffer();
    void Release();

    void Create(ID3D11Device* device, UINT byteWidth);
    void Create(ID3D11Device* device, D3D11_BUFFER_DESC cbd, D3D11_SUBRESOURCE_DATA sd);

    void VSSet(ID3D11DeviceContext* context, UINT slotid);
    void PSSet(ID3D11DeviceContext* context, UINT slotid);
    void VSPSSet(ID3D11DeviceContext* context, UINT slotid);

    void Update(ID3D11DeviceContext* context, UINT DstSubresource, const D3D11_BOX* pDstBox,
        const void* pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch);

    void SetName(std::string name) { m_CBName = name; }
    void SetName(const char* name) { m_CBName = name; }
    void SetID(UINT slotid) { m_SlotID = slotid; }

    std::string GetName() { return m_CBName; }
    UINT GetSlotID() { return m_SlotID; }
    UINT GetByteWidth() { return m_ByteWidth; }

    ID3D11Buffer* GetBuffer() { return m_CBuffer; }

private:
    std::string m_CBName;
    UINT m_SlotID;
    UINT m_ByteWidth;

    ID3D11Buffer* m_CBuffer;
};
