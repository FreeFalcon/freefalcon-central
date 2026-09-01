// Artscout - 2026 (#104, Linux Ф1): a MINIMAL DirectXMath (namespace DirectX) covering exactly the subset that
// the project's d3dxmath.h wrapper uses -- scalar, no SIMD, ROW-MAJOR / ROW-VECTOR (v' = v*M) / LEFT-HANDED, to
// match DirectXMath's conventions bit-for-bit so d3dxmath.h's reinterpret_casts and results stay correct. Not the
// full library; add functions error-driven if a Linux-compiled file needs more. (The heavy DirectXMath use is in
// the D3D12 renderer, which is Windows-only -- Linux uses the Vulkan backend.) Linux-only shim.
#ifndef FF_WIN32SHIM_DIRECTXMATH_H
#define FF_WIN32SHIM_DIRECTXMATH_H
#include <cmath>

namespace DirectX
{

struct XMFLOAT2
{
    float x, y;
    XMFLOAT2()
    {
    }
    XMFLOAT2(float _x, float _y) : x(_x), y(_y)
    {
    }
};
struct XMFLOAT3
{
    float x, y, z;
    XMFLOAT3()
    {
    }
    XMFLOAT3(float _x, float _y, float _z) : x(_x), y(_y), z(_z)
    {
    }
};
struct XMFLOAT4
{
    float x, y, z, w;
    XMFLOAT4()
    {
    }
    XMFLOAT4(float _x, float _y, float _z, float _w)
        : x(_x), y(_y), z(_z), w(_w)
    {
    }
};
struct XMFLOAT4X4
{
    float m[4][4];
};

struct XMVECTOR
{
    float f[4];
};
struct XMMATRIX
{
    XMVECTOR r[4];
};
// DirectXMath's fast-call aliases -- on non-Windows we just pass by value/const-ref; semantics are identical.
typedef const XMVECTOR FXMVECTOR;
typedef const XMVECTOR& GXMVECTOR;
typedef const XMMATRIX& FXMMATRIX;
typedef const XMMATRIX& CXMMATRIX;

static inline XMVECTOR XMVectorSet(float x, float y, float z, float w)
{
    XMVECTOR v = {{x, y, z, w}};
    return v;
}
static inline float XMVectorGetX(FXMVECTOR v)
{
    return v.f[0];
}
static inline float XMVectorGetY(FXMVECTOR v)
{
    return v.f[1];
}
static inline float XMVectorGetZ(FXMVECTOR v)
{
    return v.f[2];
}
static inline float XMVectorGetW(FXMVECTOR v)
{
    return v.f[3];
}

static inline XMVECTOR XMLoadFloat3(const XMFLOAT3* p)
{
    XMVECTOR v = {{p->x, p->y, p->z, 1.0f}};
    return v;
}
static inline XMVECTOR XMLoadFloat4(const XMFLOAT4* p)
{
    XMVECTOR v = {{p->x, p->y, p->z, p->w}};
    return v;
}
static inline void XMStoreFloat3(XMFLOAT3* p, FXMVECTOR v)
{
    p->x = v.f[0];
    p->y = v.f[1];
    p->z = v.f[2];
}
static inline void XMStoreFloat4(XMFLOAT4* p, FXMVECTOR v)
{
    p->x = v.f[0];
    p->y = v.f[1];
    p->z = v.f[2];
    p->w = v.f[3];
}
static inline XMMATRIX XMLoadFloat4x4(const XMFLOAT4X4* p)
{
    XMMATRIX M;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            M.r[i].f[j] = p->m[i][j];
    return M;
}
static inline void XMStoreFloat4x4(XMFLOAT4X4* p, FXMMATRIX M)
{
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            p->m[i][j] = M.r[i].f[j];
}

static inline XMMATRIX XMMatrixIdentity(void)
{
    XMMATRIX M;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            M.r[i].f[j] = (i == j) ? 1.0f : 0.0f;
    return M;
}
// C = A * B (row-major: C[i][j] = sum_k A[i][k]*B[k][j])
static inline XMMATRIX XMMatrixMultiply(FXMMATRIX A, CXMMATRIX B)
{
    XMMATRIX C;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
        {
            float s = 0.0f;
            for (int k = 0; k < 4; ++k)
                s += A.r[i].f[k] * B.r[k].f[j];
            C.r[i].f[j] = s;
        }
    return C;
}
static inline XMMATRIX XMMatrixTranslation(float x, float y, float z)
{
    XMMATRIX M = XMMatrixIdentity();
    M.r[3].f[0] = x;
    M.r[3].f[1] = y;
    M.r[3].f[2] = z;
    return M;
}
static inline XMMATRIX XMMatrixRotationX(float a)
{
    float c = cosf(a), s = sinf(a);
    XMMATRIX M = XMMatrixIdentity();
    M.r[1].f[1] = c;
    M.r[1].f[2] = s;
    M.r[2].f[1] = -s;
    M.r[2].f[2] = c;
    return M;
}
static inline XMMATRIX XMMatrixRotationY(float a)
{
    float c = cosf(a), s = sinf(a);
    XMMATRIX M = XMMatrixIdentity();
    M.r[0].f[0] = c;
    M.r[0].f[2] = -s;
    M.r[2].f[0] = s;
    M.r[2].f[2] = c;
    return M;
}
static inline XMMATRIX XMMatrixRotationZ(float a)
{
    float c = cosf(a), s = sinf(a);
    XMMATRIX M = XMMatrixIdentity();
    M.r[0].f[0] = c;
    M.r[0].f[1] = s;
    M.r[1].f[0] = -s;
    M.r[1].f[1] = c;
    return M;
}
// Left-handed perspective, row-major (matches DirectX::XMMatrixPerspectiveFovLH).
static inline XMMATRIX XMMatrixPerspectiveFovLH(float fovY, float aspect,
                                                float zn, float zf)
{
    float h = 1.0f / tanf(fovY * 0.5f);
    float w = h / aspect;
    float q = zf / (zf - zn);
    XMMATRIX M;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            M.r[i].f[j] = 0.0f;
    M.r[0].f[0] = w;
    M.r[1].f[1] = h;
    M.r[2].f[2] = q;
    M.r[2].f[3] = 1.0f;
    M.r[3].f[2] = -zn * q;
    return M;
}
static inline XMVECTOR XMVector3Dot(FXMVECTOR a, FXMVECTOR b)
{
    float d = a.f[0] * b.f[0] + a.f[1] * b.f[1] + a.f[2] * b.f[2];
    XMVECTOR v = {{d, d, d, d}};
    return v;
}
static inline XMVECTOR XMVector3Normalize(FXMVECTOR a)
{
    float l = sqrtf(a.f[0] * a.f[0] + a.f[1] * a.f[1] + a.f[2] * a.f[2]);
    if (l < 1e-12f)
        l = 1.0f;
    XMVECTOR v = {{a.f[0] / l, a.f[1] / l, a.f[2] / l, 0.0f}};
    return v;
}
static inline XMVECTOR XMVector3Length(FXMVECTOR a)
{
    float l = sqrtf(a.f[0] * a.f[0] + a.f[1] * a.f[1] + a.f[2] * a.f[2]);
    XMVECTOR v = {{l, l, l, l}};
    return v;
}
// (v,1) * M, then divide by w -- row-vector transform of a point.
static inline XMVECTOR XMVector3TransformCoord(FXMVECTOR v, FXMMATRIX M)
{
    float o[4];
    for (int j = 0; j < 4; ++j)
        o[j] = v.f[0] * M.r[0].f[j] + v.f[1] * M.r[1].f[j] +
               v.f[2] * M.r[2].f[j] + M.r[3].f[j];
    float iw = (o[3] != 0.0f) ? 1.0f / o[3] : 1.0f;
    XMVECTOR r = {{o[0] * iw, o[1] * iw, o[2] * iw, 1.0f}};
    return r;
}
static inline XMVECTOR XMVector3TransformNormal(FXMVECTOR v, FXMMATRIX M)
{
    float o[3];
    for (int j = 0; j < 3; ++j)
        o[j] =
            v.f[0] * M.r[0].f[j] + v.f[1] * M.r[1].f[j] + v.f[2] * M.r[2].f[j];
    XMVECTOR r = {{o[0], o[1], o[2], 0.0f}};
    return r;
}

} // namespace DirectX
#endif // FF_WIN32SHIM_DIRECTXMATH_H
