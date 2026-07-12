//-----------------------------------------------------------------------------
// D3D11GeometryBuffer.h  -- managed D3D11 vertex storage for the object/BSP
// path. PHASE 4 of the D3D7 -> D3D11 port.
//
// The legacy CDXVbManager (DXVbManager.h) packs object models into shared
// IDirect3DVertexBuffer7's and draws them with DrawPrimitiveVB(baseOffset,
// count). This module provides the equivalent D3D11 plumbing: one large vertex
// buffer of D3DVERTEXEX, per-model sub-allocation (by model ID), and a draw
// call that binds the object program (g_pD3D11Renderer->BeginObjectPass),
// sets the per-object world matrix, and issues Draw/DrawIndexed with the right
// topology. TRIANGLEFAN (no native D3D11 topology) is emulated via indices.
//
// The full CDXVbManager (draw-request queue, model encryption, multiple buffer
// classes) is rewritten on top of this during integration; this is the
// reusable, self-contained core.
//-----------------------------------------------------------------------------
#ifndef _D3D11GEOMETRYBUFFER_H_
#define _D3D11GEOMETRYBUFFER_H_

#include <windows.h>
#include <map>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Buffer;

// Must match D3DVERTEXEX (DXVbManager.h): 40 bytes, matches the object input
// layout created in D3D11Renderer.
struct D3D11VertexEx
{
	float vx, vy, vz;
	float nx, ny, nz;
	unsigned long dwColour;     // D3DCOLOR
	unsigned long dwSpecular;   // D3DCOLOR
	float tu, tv;
};

class D3D11GeometryBuffer
{
public:
	D3D11GeometryBuffer();
	~D3D11GeometryBuffer();

	// maxVertices sizes the shared vertex buffer.
	bool Init(int maxVertices = 1 << 20);
	void Release();
	bool IsValid() const { return m_pVB != 0; }

	// Upload a model's vertices once; returns the base vertex index in the
	// shared VB, or -1 on failure. If the ID was already uploaded, returns the
	// existing base (no re-upload).
	int  SetupModel(unsigned long id, const D3D11VertexEx* verts, int count);
	bool HasModel(unsigned long id) const { return m_models.count(id) != 0; }
	int  GetBase(unsigned long id) const;
	void Reset();   // drop all allocations (keep the buffer)

	// Bind the shared VB + object program. Call once per object batch.
	void BeginObjectBatch();

	// Draw a surface of a model. primTypeD3D7 is a D3DPRIMITIVETYPE
	// (1=POINTLIST..6=TRIANGLEFAN). worldRowMajor is 16 floats (may be NULL to
	// keep the current world). baseVertex from SetupModel + the surface's local
	// first-vertex offset.
	void DrawSurface(int primTypeD3D7, int baseVertex, int vcount,
	                 const float* worldRowMajor = 0);

private:
	bool EnsureIB(int indices);

	ID3D11Device*        m_pDev;
	ID3D11DeviceContext* m_pCtx;
	ID3D11Buffer*        m_pVB;
	ID3D11Buffer*        m_pIB;          // trifan emulation
	int                  m_nIBCount;

	int                  m_maxVerts;
	int                  m_nextVert;     // bump allocator cursor

	std::map<unsigned long, int> m_models;   // id -> base vertex
};

extern D3D11GeometryBuffer* g_pD3D11Geometry;

#endif // _D3D11GEOMETRYBUFFER_H_
