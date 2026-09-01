//-----------------------------------------------------------------------------
// d3dxmath.h  -- modern replacement for the legacy DirectX 7 D3DX math header.
//
// PHASE 1.5 of the D3D7 -> D3D11 port (see project memory: modern-sdk-migration).
//
// The original <d3dxmath.h> shipped with the old DirectX 7 SDK, which is NOT
// part of the modern Windows SDK. This drop-in replacement re-implements the
// exact types and functions the project uses, backed by DirectXMath (header-
// only, ships with the Windows SDK). It is self-contained and x64-clean: it
// pulls in no DX7/DDraw headers, so files that include only this header (e.g.
// RedMacros.h) keep compiling.
//
// Layout is bit-identical to the DX7 D3DX types (and to D3DMATRIX/D3DVECTOR),
// so the existing pointer reinterpret-casts in the code -- (LPD3DMATRIX)&mat,
// (D3DXVECTOR3*)&d3dvector -- keep working unchanged.
//
// Field naming matches DX7 D3DX exactly: matrix members are m00..m33 (NOT the
// d3dx9-era _11.._44), available both as named fields and as m[4][4].
// Convention: row-major, row-vector (v' = v * M), left-handed -- same as the
// DX7 D3DX it replaces.
//-----------------------------------------------------------------------------
#ifndef _REDVIPER_D3DXMATH_H_
#define _REDVIPER_D3DXMATH_H_

#include <directxmath.h>

#ifndef D3DX_PI
#define D3DX_PI (3.14159265358979323846f)
#endif

//=============================================================================
// D3DXVECTOR3 -- standalone POD, layout-compatible with D3DVECTOR (3 floats).
//=============================================================================
struct D3DXVECTOR3
{
    float x, y, z;

    D3DXVECTOR3()
    {
    }
    D3DXVECTOR3(float _x, float _y, float _z) : x(_x), y(_y), z(_z)
    {
    }

    operator float*()
    {
        return &x;
    }
    operator const float*() const
    {
        return &x;
    }

    D3DXVECTOR3 operator+(const D3DXVECTOR3& v) const
    {
        return D3DXVECTOR3(x + v.x, y + v.y, z + v.z);
    }
    D3DXVECTOR3 operator-(const D3DXVECTOR3& v) const
    {
        return D3DXVECTOR3(x - v.x, y - v.y, z - v.z);
    }
    D3DXVECTOR3 operator-() const
    {
        return D3DXVECTOR3(-x, -y, -z);
    }
    D3DXVECTOR3 operator*(float s) const
    {
        return D3DXVECTOR3(x * s, y * s, z * s);
    }
    D3DXVECTOR3 operator*(const D3DXVECTOR3& v) const
    {
        return D3DXVECTOR3(x * v.x, y * v.y, z * v.z);
    } // component-wise (like the old _D3DVECTOR)

    D3DXVECTOR3& operator+=(const D3DXVECTOR3& v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }
    D3DXVECTOR3& operator-=(const D3DXVECTOR3& v)
    {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }
    D3DXVECTOR3& operator*=(float s)
    {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }
};

//=============================================================================
// D3DXMATRIX -- standalone POD, 16 floats row-major. Layout-compatible with
// D3DMATRIX (so (LPD3DMATRIX)& casts work). DX7 D3DX field naming: m00..m33.
//=============================================================================
struct D3DXMATRIX
{
    union
    {
        struct
        {
            float m00, m01, m02, m03;
            float m10, m11, m12, m13;
            float m20, m21, m22, m23;
            float m30, m31, m32, m33;
        };
        float m[4][4];
    };

    D3DXMATRIX()
    {
    }

    float& operator()(unsigned r, unsigned c)
    {
        return m[r][c];
    }
    float operator()(unsigned r, unsigned c) const
    {
        return m[r][c];
    }

    operator float*()
    {
        return &m00;
    }
    operator const float*() const
    {
        return &m00;
    }
};

//=============================================================================
// Internal helpers: reinterpret the POD types as DirectXMath storage types.
// Layouts are identical (row-major), so this is safe and copy-free.
//=============================================================================
namespace _redviper_d3dx_detail
{
inline DirectX::XMMATRIX LoadM(const D3DXMATRIX* p)
{
    return DirectX::XMLoadFloat4x4(
        reinterpret_cast<const DirectX::XMFLOAT4X4*>(p));
}
inline void StoreM(D3DXMATRIX* p, DirectX::FXMMATRIX m)
{
    DirectX::XMStoreFloat4x4(reinterpret_cast<DirectX::XMFLOAT4X4*>(p), m);
}
inline DirectX::XMVECTOR LoadV3(const D3DXVECTOR3* p)
{
    return DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(p));
}
inline void StoreV3(D3DXVECTOR3* p, DirectX::FXMVECTOR v)
{
    DirectX::XMStoreFloat3(reinterpret_cast<DirectX::XMFLOAT3*>(p), v);
}
}

//=============================================================================
// Matrix functions. All return pOut (matching the DX7 D3DX contract). Output
// may alias the inputs -- loads complete before the store, so it is safe.
//=============================================================================
inline D3DXMATRIX* D3DXMatrixIdentity(D3DXMATRIX* pOut)
{
    _redviper_d3dx_detail::StoreM(pOut, DirectX::XMMatrixIdentity());
    return pOut;
}

inline D3DXMATRIX* D3DXMatrixMultiply(D3DXMATRIX* pOut, const D3DXMATRIX* pM1,
                                      const D3DXMATRIX* pM2)
{
    using namespace DirectX;
    XMMATRIX r = XMMatrixMultiply(_redviper_d3dx_detail::LoadM(pM1),
                                  _redviper_d3dx_detail::LoadM(pM2));
    _redviper_d3dx_detail::StoreM(pOut, r);
    return pOut;
}

inline D3DXMATRIX* D3DXMatrixTranslation(D3DXMATRIX* pOut, float x, float y,
                                         float z)
{
    _redviper_d3dx_detail::StoreM(pOut, DirectX::XMMatrixTranslation(x, y, z));
    return pOut;
}

inline D3DXMATRIX* D3DXMatrixRotationX(D3DXMATRIX* pOut, float angle)
{
    _redviper_d3dx_detail::StoreM(pOut, DirectX::XMMatrixRotationX(angle));
    return pOut;
}

inline D3DXMATRIX* D3DXMatrixRotationY(D3DXMATRIX* pOut, float angle)
{
    _redviper_d3dx_detail::StoreM(pOut, DirectX::XMMatrixRotationY(angle));
    return pOut;
}

inline D3DXMATRIX* D3DXMatrixRotationZ(D3DXMATRIX* pOut, float angle)
{
    _redviper_d3dx_detail::StoreM(pOut, DirectX::XMMatrixRotationZ(angle));
    return pOut;
}

// DX7 D3DX used the unsuffixed name with left-handed convention.
inline D3DXMATRIX* D3DXMatrixPerspectiveFov(D3DXMATRIX* pOut, float fovy,
                                            float aspect, float zn, float zf)
{
    _redviper_d3dx_detail::StoreM(
        pOut, DirectX::XMMatrixPerspectiveFovLH(fovy, aspect, zn, zf));
    return pOut;
}

//=============================================================================
// Vector functions.
//=============================================================================
inline D3DXVECTOR3* D3DXVec3TransformCoord(D3DXVECTOR3* pOut,
                                           const D3DXVECTOR3* pV,
                                           const D3DXMATRIX* pM)
{
    using namespace DirectX;
    XMVECTOR r = XMVector3TransformCoord(_redviper_d3dx_detail::LoadV3(pV),
                                         _redviper_d3dx_detail::LoadM(pM));
    _redviper_d3dx_detail::StoreV3(pOut, r);
    return pOut;
}

inline D3DXVECTOR3* D3DXVec3Normalize(D3DXVECTOR3* pOut, const D3DXVECTOR3* pV)
{
    using namespace DirectX;
    _redviper_d3dx_detail::StoreV3(
        pOut, XMVector3Normalize(_redviper_d3dx_detail::LoadV3(pV)));
    return pOut;
}

inline float D3DXVec3Dot(const D3DXVECTOR3* pV1, const D3DXVECTOR3* pV2)
{
    using namespace DirectX;
    return XMVectorGetX(XMVector3Dot(_redviper_d3dx_detail::LoadV3(pV1),
                                     _redviper_d3dx_detail::LoadV3(pV2)));
}

#endif // _REDVIPER_D3DXMATH_H_
