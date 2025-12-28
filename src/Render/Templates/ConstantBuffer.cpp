//----------------------------------------------------------------------------
//-- CConstantBuffer.cpp
//-- VlaGan: Basic constant buufer class
//----------------------------------------------------------------------------
#include "ConstantBuffer.h"
#include "../../_defines.h"

//extern void LogMsg(const char* fmt, ...);


CConstantBuffer::CConstantBuffer() {
    m_CBName = "CBDefault";
    m_SlotID = 0;
    m_ByteWidth = 0;
    m_CBuffer = nullptr;
}

CConstantBuffer::~CConstantBuffer() {
	Release();
}

void CConstantBuffer::Release() {
	RELEASE(m_CBuffer);
}

//-- default creating
void CConstantBuffer::Create(ID3D11Device* device, UINT byteWidth) {

    D3D11_BUFFER_DESC cbd = {};
    cbd.Usage = D3D11_USAGE_DEFAULT;
    cbd.ByteWidth = byteWidth;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = 0;

    m_ByteWidth = byteWidth;
    device->CreateBuffer(&cbd, nullptr, &m_CBuffer);
}

//-- create cb by data
void CConstantBuffer::Create(ID3D11Device* device, D3D11_BUFFER_DESC cbd, D3D11_SUBRESOURCE_DATA sd) {
    m_ByteWidth = cbd.ByteWidth;
    device->CreateBuffer(&cbd, &sd, &m_CBuffer);
}

void CConstantBuffer::VSSet(ID3D11DeviceContext* context, UINT slotid) {
    m_SlotID = slotid;
    context->VSSetConstantBuffers(m_SlotID, 1, &m_CBuffer);
}

void CConstantBuffer::PSSet(ID3D11DeviceContext* context, UINT slotid) {
    m_SlotID = slotid;
    context->PSSetConstantBuffers(m_SlotID, 1, &m_CBuffer);
}

void CConstantBuffer::VSPSSet(ID3D11DeviceContext* context, UINT slotid) {
    m_SlotID = slotid;
    context->VSSetConstantBuffers(m_SlotID, 1, &m_CBuffer);
    context->PSSetConstantBuffers(m_SlotID, 1, &m_CBuffer);
}

void CConstantBuffer::Update(ID3D11DeviceContext* context, UINT DstSubresource, const D3D11_BOX* pDstBox,
    const void* pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch) {

    if (!pSrcData) {
        //LogMsg("!CConstantBuffer::UpdateSubResource pSrcData == nullptr");
        return;
    }

    if (!m_CBuffer || !context) {
        for (int i{}; i < 100; ++i)

        //LogMsg("--CConstantBuffer::Update - ERROR OCCURED!");
        return;
    }

    //-- OMG need to catch why it can be crushed only on start
    try {
        context->UpdateSubresource(m_CBuffer, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);
    }
    catch (...)
    {
        //LogMsg("--CConstantBuffer::Update - ERROR OCCURED!");
    }
    //context->UpdateSubresource(m_CBuffer, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);
}
