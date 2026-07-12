//-----------------------------------------------------------------------------
// D3D11GeometryBuffer.cpp -- see header. PHASE 4 of the D3D7->D3D11 port.
//-----------------------------------------------------------------------------
#include "stdafx.h"
#include "D3D11GeometryBuffer.h"
#include "D3D11Renderer.h"
#include "Graphics\DXEngine\D3D11Backend.h"

#include <d3d11.h>

extern "C" void MonoPrint(char *fmt, ...);

D3D11GeometryBuffer* g_pD3D11Geometry = NULL;

static inline void SafeRel(IUnknown* p) { if (p) p->Release(); }

D3D11GeometryBuffer::D3D11GeometryBuffer()
	: m_pDev(0), m_pCtx(0), m_pVB(0), m_pIB(0), m_nIBCount(0),
	  m_maxVerts(0), m_nextVert(0) {}

D3D11GeometryBuffer::~D3D11GeometryBuffer() { Release(); }

bool D3D11GeometryBuffer::Init(int maxVertices)
{
	if (!g_pD3D11Backend || !g_pD3D11Backend->IsValid())
	{
		MonoPrint("D3D11GeometryBuffer::Init - backend not ready\n");
		return false;
	}
	m_pDev = g_pD3D11Backend->GetDevice();
	m_pCtx = g_pD3D11Backend->GetContext();
	m_maxVerts = maxVertices;
	m_nextVert = 0;

	// CPU-writable shared vertex buffer (models are uploaded via Map regions).
	D3D11_BUFFER_DESC bd; ZeroMemory(&bd, sizeof(bd));
	bd.ByteWidth      = maxVertices * sizeof(D3D11VertexEx);
	bd.Usage          = D3D11_USAGE_DYNAMIC;
	bd.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	if (FAILED(m_pDev->CreateBuffer(&bd, NULL, &m_pVB)))
	{
		MonoPrint("D3D11GeometryBuffer::Init - CreateBuffer(VB) failed\n");
		return false;
	}
	MonoPrint("D3D11GeometryBuffer::Init - %d verts (%d bytes)\n",
	          maxVertices, bd.ByteWidth);
	return true;
}

int D3D11GeometryBuffer::GetBase(unsigned long id) const
{
	std::map<unsigned long, int>::const_iterator it = m_models.find(id);
	return (it == m_models.end()) ? -1 : it->second;
}

int D3D11GeometryBuffer::SetupModel(unsigned long id, const D3D11VertexEx* verts, int count)
{
	if (!m_pVB || count <= 0) return -1;

	int existing = GetBase(id);
	if (existing >= 0) return existing;

	if (m_nextVert + count > m_maxVerts)
	{
		MonoPrint("D3D11GeometryBuffer::SetupModel - VB full (%d + %d > %d)\n",
		          m_nextVert, count, m_maxVerts);
		return -1;
	}

	int base = m_nextVert;

	// Map the whole buffer with NO_OVERWRITE so prior model data is preserved,
	// and copy this model's vertices into its sub-range.
	D3D11_MAPPED_SUBRESOURCE ms;
	D3D11_MAP mapType = (base == 0) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
	if (FAILED(m_pCtx->Map(m_pVB, 0, mapType, 0, &ms)))
		return -1;
	D3D11VertexEx* dst = (D3D11VertexEx*)ms.pData + base;
	memcpy(dst, verts, count * sizeof(D3D11VertexEx));
	m_pCtx->Unmap(m_pVB, 0);

	m_nextVert += count;
	m_models[id] = base;
	return base;
}

void D3D11GeometryBuffer::Reset()
{
	m_nextVert = 0;
	m_models.clear();
}

bool D3D11GeometryBuffer::EnsureIB(int indices)
{
	if (m_pIB && m_nIBCount >= indices) return true;
	SafeRel(m_pIB); m_pIB = NULL;
	int want = 256; while (want < indices) want <<= 1;
	D3D11_BUFFER_DESC bd; ZeroMemory(&bd, sizeof(bd));
	bd.ByteWidth      = want * sizeof(unsigned short);
	bd.Usage          = D3D11_USAGE_DYNAMIC;
	bd.BindFlags      = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	if (FAILED(m_pDev->CreateBuffer(&bd, NULL, &m_pIB))) return false;
	m_nIBCount = want;
	return true;
}

void D3D11GeometryBuffer::BeginObjectBatch()
{
	if (!m_pCtx || !g_pD3D11Renderer) return;
	g_pD3D11Renderer->BeginObjectPass();

	UINT stride = sizeof(D3D11VertexEx), offset = 0;
	m_pCtx->IASetVertexBuffers(0, 1, &m_pVB, &stride, &offset);
}

// D3DPRIMITIVETYPE: 1=POINTLIST 2=LINELIST 3=LINESTRIP 4=TRIANGLELIST
//                   5=TRIANGLESTRIP 6=TRIANGLEFAN
void D3D11GeometryBuffer::DrawSurface(int prim, int baseVertex, int vcount,
                                      const float* worldRowMajor)
{
	if (!m_pCtx || vcount <= 0) return;

	if (worldRowMajor && g_pD3D11Renderer)
		g_pD3D11Renderer->SetWorld(worldRowMajor);

	switch (prim)
	{
	case 1:
		m_pCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		m_pCtx->Draw(vcount, baseVertex);
		break;
	case 2:
		m_pCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
		m_pCtx->Draw(vcount, baseVertex);
		break;
	case 3:
		m_pCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
		m_pCtx->Draw(vcount, baseVertex);
		break;
	case 4:
		m_pCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_pCtx->Draw(vcount, baseVertex);
		break;
	case 5:
		m_pCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		m_pCtx->Draw(vcount, baseVertex);
		break;
	case 6: // TRIANGLEFAN -> indexed triangle list (indices relative to baseVertex)
	{
		int tris = vcount - 2;
		if (tris <= 0) break;
		int nIdx = tris * 3;
		if (!EnsureIB(nIdx)) break;

		D3D11_MAPPED_SUBRESOURCE im;
		if (FAILED(m_pCtx->Map(m_pIB, 0, D3D11_MAP_WRITE_DISCARD, 0, &im))) break;
		unsigned short* idx = (unsigned short*)im.pData;
		for (int i = 0; i < tris; ++i)
		{
			idx[i*3+0] = 0;
			idx[i*3+1] = (unsigned short)(i + 1);
			idx[i*3+2] = (unsigned short)(i + 2);
		}
		m_pCtx->Unmap(m_pIB, 0);

		m_pCtx->IASetIndexBuffer(m_pIB, DXGI_FORMAT_R16_UINT, 0);
		m_pCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		// baseVertex shifts the relative indices to this model's vertices.
		m_pCtx->DrawIndexed(nIdx, 0, baseVertex);
		break;
	}
	default:
		break;
	}
}

void D3D11GeometryBuffer::Release()
{
	SafeRel(m_pVB); m_pVB = 0;
	SafeRel(m_pIB); m_pIB = 0;
	m_models.clear();
	m_nextVert = 0;
	m_pDev = 0; m_pCtx = 0;
}
