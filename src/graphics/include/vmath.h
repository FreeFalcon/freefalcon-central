// Much of the stuff here was borrowed from the Unreal SDK

#pragma once

#include <math.h>
#include <d3dtypes.h>
#include "vTempl.h"


// TODO
// - Optimize matrix mults (SIMD, 3DNow etc)

namespace D3DFrame
{

    // Forw decls
    /////////////////////////////////////////////////////////////////////////////

    class Matrix;
    class Plane;

    inline FLOAT FSnap(FLOAT Location, FLOAT Grid);
    inline Matrix operator*(const Matrix& a, const Matrix& b);

    // Substitutions
    /////////////////////////////////////////////////////////////////////////////

#define appSqrt sqrt
#define appFloor floor

    // Constants.
    /////////////////////////////////////////////////////////////////////////////

#undef  PI
#define PI  (3.1415926535897932F)
#define PI_FRAC (PI / 180.0)
#define SMALL_NUMBER (1.e-8)
#define KINDA_SMALL_NUMBER (1.e-4)

#define THRESH_POINT_ON_PLANE (0.10) /* Thickness of plane for front/back/inside test */
#define THRESH_VECTORS_ARE_PARALLEL (0.02) /* Vectors are parallel if dot product varies less than this */

    // Vector
    /////////////////////////////////////////////////////////////////////////////

    class Vector
    {
    public:
        Vector()
        {
            x = y = z = 0.0f;
        }

        Vector(float _x, float _y, float _z)
        {
            x = _x;
            y = _y;
            z = _z;
        }

        // Attributes
    public:
        float x, y, z;

        // Implementation
        Vector& operator=(Vector v)
        {
            x = v.x;
            y = v.y;
            z = v.z;
            return *this;
        }

        // Binary math operators.
        Vector operator^(const Vector& V) const // Cross product
        {
            return Vector(y * V.z - z * V.y, z * V.x - x * V.z, x * V.y - y * V.x);
        }

        float operator|(const Vector& V) const // Dot product
        {
            return x * V.x + y * V.y + z * V.z;
        }

        friend Vector operator*(float Scale, const Vector& V)
        {
            return Vector(V.x * Scale, V.y * Scale, V.z * Scale);
        }

        Vector operator+(const Vector& V) const
        {
            return Vector(x + V.x, y + V.y, z + V.z);
        }

        Vector operator-(const Vector& V) const
        {
            return Vector(x - V.x, y - V.y, z - V.z);
        }

        Vector operator*(float Scale) const
        {
            return Vector(x * Scale, y * Scale, z * Scale);
        }

        Vector operator/(float Scale) const
        {
            float RScale = 1.0f / Scale;
            return Vector(x * RScale, y * RScale, z * RScale);
        }

        Vector operator*(const Vector& V) const
        {
            return Vector(x * V.x, y * V.y, z * V.z);
        }

        // Binary comparison operators.
        bool operator==(const Vector& V) const
        {
            return x == V.x && y == V.y && z == V.z;
        }

        bool operator!=(const Vector& V) const
        {
            return x != V.x || y != V.y || z != V.z;
        }

        // Unary operators.
        Vector operator-() const
        {
            return Vector(-x, -y, -z);
        }

        // Assignment operators.
        Vector operator+=(const Vector& V)
        {
            x += V.x;
            y += V.y;
            z += V.z;
            return *this;
        }

        Vector operator-=(const Vector& V)
        {
            x -= V.x;
            y -= V.y;
            z -= V.z;
            return *this;
        }

        Vector operator*=(float Scale)
        {
            x *= Scale;
            y *= Scale;
            z *= Scale;
            return *this;
        }

        Vector operator/=(float V)
        {
            float RV = 1.0f / V;
            x *= RV;
            y *= RV;
            z *= RV;
            return *this;
        }

        Vector operator*=(const Vector& V)
        {
            x *= V.x;
            y *= V.y;
            z *= V.z;
            return *this;
        }

        Vector operator/=(const Vector& V)
        {
            x /= V.x;
            y /= V.y;
            z /= V.z;
            return *this;
        }

        // Simple functions.
        float Magnitude() const
        {
            return (float) appSqrt(x * x + y * y + z * z);
        }

        float Size() const
        {
            return Magnitude();
        }

        float SizeSquared() const
        {
            return x * x + y * y + z * z;
        }

        float Size2D() const
        {
            return (float) appSqrt(x * x + y * y);
        }

        float SizeSquared2D() const
        {
            return x * x + y * y;
        }

        int IsNearlyZero() const
        {
            return fabs(x) < KINDA_SMALL_NUMBER && fabs(y) < KINDA_SMALL_NUMBER && fabs(z) < KINDA_SMALL_NUMBER;
        }

        bool IsZero() const
        {
            return x == 0.0 && y == 0.0 && z == 0.0;
        }

        bool Normalize()
        {
            float SquareSum = x * x + y * y + z * z;

            if (SquareSum >= SMALL_NUMBER)
            {
                float Scale = 1.0f / (float) appSqrt(SquareSum);
                x *= Scale;
                y *= Scale;
                z *= Scale;
                return 1;
            }
            else return 0;
        }

        Vector SafeNormal() const
        {
            float SquareSum = x * x + y * y + z * z;

            if (SquareSum >= SMALL_NUMBER)
            {
                float Scale = 1.0f / (float) appSqrt(SquareSum);
                return Vector(x * Scale, y * Scale, z * Scale);
            }

            return Vector(0, 0, 0);
        }

        Vector UnsafeNormal() const
        {
            float Scale = 1.0f / (float) appSqrt(x * x + y * y + z * z);
            return Vector(x * Scale, y * Scale, z * Scale);
        }

        Vector Projection() const
        {
            float RZ = 1.0f / z;
            return Vector(x * RZ, y * RZ, 1);
        }

        Vector GridSnap(const Vector& Grid)
        {
            return Vector(FSnap(x, Grid.x), FSnap(y, Grid.y), FSnap(z, Grid.z));
        }

        Vector BoundToCube(float Radius)
        {
            return Vector
                   (
                       Clamp(x, -Radius, Radius),
                       Clamp(y, -Radius, Radius),
                       Clamp(z, -Radius, Radius)
                   );
        }

        void AddBounded(const Vector& V, float Radius = 0x7fff)
        {
            *this = (*this + V).BoundToCube(Radius);
        }

        float& Component(INT Index)
        {
            return (&x)[Index];
        }

        inline Vector MirrorByVector(const Vector& MirrorNormal) const;
        inline Vector MirrorByPlane(const Plane& Plane) const;
        friend float PointPlaneDist(const Vector& Point, const Vector& PlaneBase, const Vector& PlaneNormal);
        friend bool Parallel(const Vector& Normal1, const Vector& Normal2);
        friend bool Coplanar(const Vector& Base1, const Vector& Normal1, const Vector& Base2, const Vector& Normal2);

        // Return a boolean that is based on the vector's direction.
        // When V==(0.0.0) Booleanize(0)=1.
        // Otherwise Booleanize(V) <-> !Booleanize(!B).
        bool Booleanize()
        {
            return
                x >  0.0 ? 1 :
                x <  0.0 ? 0 :
                y >  0.0 ? 1 :
                y <  0.0 ? 0 :
                z >= 0.0 ? 1 : 0;
        }
    };

    // Plane
    /////////////////////////////////////////////////////////////////////////////

    class Plane : public Vector
    {
    public:
        Plane()
        {
            w = 0.0f;
        }

        Plane(const Plane& P) :
            Vector(P),
            w(P.w)
        {
        }

        Plane(const Vector& V) :
            Vector(V),
            w(0)
        {
        }

        Plane(float InX, float InY, float InZ, float InW) :
            Vector(InX, InY, InZ),
            w(InW)
        {
        }

        Plane(Vector vA, Vector vB, Vector vC) :
            Vector(((vB - vA) ^ (vC - vA)).SafeNormal()),
            w(vA | ((vB - vA) ^ (vC - vA)).SafeNormal())
        {
        }

        // Attributes
    public:
        float w;

        // Implementation
        float VectorDistance(Vector& v)
        {
            return x * v.x + y * v.y + z * v.z - w;
        }

        void ConstructFromPoints(Vector& vA, Vector &vB, Vector &vC)
        {
            Vector v = ((vB - vA) ^ (vC - vA)).SafeNormal();
            x = v.x;
            y = v.y;
            z = v.z;
            w = (vA | ((vB - vA) ^ (vC - vA)).SafeNormal());
        }

        float PlaneDot(const Vector &P) const
        {
            return x * P.x + y * P.y + z * P.z - w;
        }

        Plane Flip() const
        {
            return Plane(-y, -y, -z, -w);
        }
    };


    // Matrix
    /////////////////////////////////////////////////////////////////////////////

    class Matrix
    {
    public:
        Matrix& operator=(const Matrix &mx)
        {
            memcpy(m, mx.m, sizeof(m));
            return *this;
        }

        // Attributes
        float m[4][4];

        // Implementation
        inline void InitIdentity()
        {
            m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0f;
            m[0][1] = m[0][2] = m[0][3] = m[3][0] = 0.0f;
            m[1][0] = m[1][2] = m[1][3] = m[3][1] = 0.0f;
            m[2][0] = m[2][1] = m[2][3] = m[3][2] = 0.0f;
        }

        inline void Translate(float dx, float dy, float dz)
        {
            InitIdentity();
            m[3][0] = dx;
            m[3][1] = dy;
            m[3][2] = dz;
        }

        void Translate(Vector v)
        {
            Translate(v.x, v.y, v.z);
        }

        inline void Scale(float dx, float dy, float dz)
        {
            InitIdentity();
            m[0][0] = dx;
            m[1][1] = dy;
            m[2][2] = dz;
        }

        void Scale(Vector v)
        {
            Scale(v.x, v.y, v.z);
        }

        void RotateRadX(float radsx)
        {
            InitIdentity();
            m[1][1] = m[2][2] = (FLOAT) cos(radsx);
            m[1][2] = (FLOAT) sin(radsx);
            m[2][1] = -m[1][2];
        }

        void RotateRadY(float radsy)
        {
            InitIdentity();
            m[0][0] =  m[2][2] = (FLOAT) cos(radsy);
            m[0][2] = -(FLOAT) sin(radsy);
            m[2][0] =  -m[0][2];
        }

        void RotateRadZ(float radsz)
        {
            InitIdentity();
            m[0][0]  = m[1][1] = (FLOAT) cos(radsz);
            m[0][1]  = (FLOAT) sin(radsz);
            m[1][0]  = -m[0][1];
        }

        void RotateRad(float radsx, float radsy, float radsz)
        {
            InitIdentity();
            Matrix mx, my, mz;
            mx.RotateRadX(radsx);
            my.RotateRadY(radsy);
            mz.RotateRadZ(radsz);

            *this = mz * my * mx;
        }

        void RotateX(float degsx)
        {
            RotateRadX((float)(degsx * PI_FRAC));
        }

        void RotateY(float degsy)
        {
            RotateRadY((float)(degsy * PI_FRAC));
        }

        void RotateZ(float degsz)
        {
            RotateRadZ((float)(degsz * PI_FRAC));
        }

        inline void Rotate(float degsx, float degsy, float degsz)
        {
            RotateRad((float)(degsx * PI_FRAC), float(degsy * PI_FRAC), float(degsz * PI_FRAC));
        }

        void Rotate(Vector v)
        {
            Rotate(v.x, v.y, v.z);
        }

        void SetViewMatrix(Vector& vFrom, Vector& vAt, Vector& vWorldUp)
        {
            // Get the z basis vector, which points straight ahead. This is the difference from the eyepoint to the lookat point.
            Vector vView = vAt - vFrom;
            float fLength = vView.Size();

            if (fLength < 1e-6f) return;

            vView /= fLength; // Normalize the z basis vector

            // Get the dot product, and calculate the projection of the z basis vector onto the up vector. The projection is the y basis vector.
            FLOAT fDotProduct = vWorldUp | vView;
            Vector vUp = vWorldUp - fDotProduct * vView;

            // If this vector has near-zero length because the input specified a bogus up vector, let's try a default up vector
            if (1e-6f > (fLength = vUp.Size()))
            {
                vUp = Vector(0.0f, 1.0f, 0.0f) - vView.y * vView;

                if (1e-6f > (fLength = vUp.Size())) // If we still have near-zero length, resort to a different axis.
                {
                    vUp = Vector(0.0f, 0.0f, 1.0f) - vView.z * vView;

                    if (1e-6f > (fLength = vUp.Size())) return;
                }
            }

            vUp /= fLength; // Normalize the y basis vector

            // The x basis vector is found simply with the cross product of the y and z basis vectors
            Vector vRight = vUp ^ vView;

            // Start building the matrix. The first three rows contains the basis vectors used to rotate the view to point at the lookat point
            InitIdentity();
            m[0][0] = vRight.x;
            m[0][1] = vUp.x;
            m[0][2] = vView.x;
            m[1][0] = vRight.y;
            m[1][1] = vUp.y;
            m[1][2] = vView.y;
            m[2][0] = vRight.z;
            m[2][1] = vUp.z;
            m[2][2] = vView.z;

            // Do the translation values (rotations are still about the eyepoint)
            m[3][0] = -(vFrom | vRight);
            m[3][1] = -(vFrom | vUp);
            m[3][2] = -(vFrom | vView);
        }

        //JAM 03Jan03
        void SetViewMatrix(float fPitch, float fRoll, float fYaw, Vector vPos)
        {
            Matrix mT, mX, mY, mZ;

            mT.InitIdentity();
            mT.m[3][0] = -vPos.x;
            mT.m[3][1] = -vPos.y;
            mT.m[3][2] = -vPos.z;

            mX.InitIdentity();
            mX.m[0][0] = 1;
            mX.m[0][1] = 0;
            mX.m[0][2] = 0;
            mX.m[1][0] = 0;
            mX.m[1][1] = (float)cos(fPitch);
            mX.m[1][2] = (float)sin(fPitch);
            mX.m[2][0] = 0;
            mX.m[2][1] = -(float)sin(fPitch);
            mX.m[2][2] = (float)cos(fPitch);

            mY.InitIdentity();
            mY.m[0][0] = (float)cos(fYaw);
            mY.m[0][1] = 0;
            mY.m[0][2] = -(float)sin(fYaw);
            mY.m[1][0] = 0;
            mY.m[1][1] = 1;
            mY.m[1][2] = 0;
            mY.m[2][0] = (float)sin(fYaw);
            mY.m[2][1] = 0;
            mY.m[2][2] = (float)cos(fYaw);

            mZ.InitIdentity();
            mZ.m[0][0] = (float)cos(fRoll);
            mZ.m[0][1] = (float)sin(fRoll);
            mZ.m[0][2] = 0;
            mZ.m[1][0] = -(float)sin(fRoll);
            mZ.m[1][1] = (float)cos(fRoll);
            mZ.m[1][2] = 0;
            mZ.m[2][0] = 0;
            mZ.m[2][1] = 0;
            mZ.m[2][2] = 1;

            InitIdentity();
            *this = mX * mY * mZ * mT;
        }
        //JAM

        void SetProjectionMatrix(float fFOV, float fAspect, float fNearPlane, float fFarPlane)
        {
            fFOV *= (float) PI_FRAC;

            if (fabs(fFarPlane - fNearPlane) < 0.01f) return;

            if (fabs(sin(fFOV / 2)) < 0.01f) return;

            FLOAT w = fAspect * (FLOAT)(cos(fFOV / 2) / sin(fFOV / 2));
            FLOAT h =   1.0f  * (FLOAT)(cos(fFOV / 2) / sin(fFOV / 2));
            FLOAT Q = fFarPlane / (fFarPlane - fNearPlane);

            InitIdentity();
            m[0][0] = w;
            m[1][1] = h;
            m[2][2] = Q;
            m[2][3] = 1.0f;
            m[3][2] = -Q * fNearPlane;
        }
    };

    // TranslationMatix
    /////////////////////////////////////////////////////////////////////////////

    class TranslationMatrix : public Matrix
    {
    public:
        TranslationMatrix(Vector &v)
        {
            Translate(v);
        }

        TranslationMatrix(float x, float y, float z)
        {
            Translate(x, y, z);
        }
    };

    // RotationMatrix
    /////////////////////////////////////////////////////////////////////////////

    class RotationMatrix : public Matrix
    {
    public:
        RotationMatrix(Vector &v)
        {
            Rotate(v);
        }

        RotationMatrix(float x, float y, float z)
        {
            Rotate(x, y, z);
        }
    };

    // ScaleMatrix
    /////////////////////////////////////////////////////////////////////////////

    class ScaleMatrix : public Matrix
    {
    public:
        ScaleMatrix(Vector &v)
        {
            Scale(v);
        }

        ScaleMatrix(float x, float y, float z)
        {
            Scale(x, y, z);
        }
    };

    // Vertex
    /////////////////////////////////////////////////////////////////////////////

    class Vertex : public Vector
    {
    public:
        Vertex()
        {
        }

        Vertex(Vector vLoc, Vector vNorm) : Vector(vLoc)
        {
            m_vNormal = vNorm;
        }

        Vector m_vNormal;
    };

    // LitVertex
    /////////////////////////////////////////////////////////////////////////////

    class LitVertex : public Vertex
    {
    public:
        LitVertex()
        {
            m_colDiffuse = 0xff << 16 | 0xff << 8 | 0xff;
            m_fTu = 0.0f;
            m_fTv = 0.0f;
        }

        LitVertex(Vector vLoc, Vector vNorm, float fTu = 0.0f, float fTv = 0.0f, DWORD colDiffuse = 0xFFFFFFFF) : Vertex(vLoc, vNorm)
        {
            m_colDiffuse = colDiffuse;
            m_fTu = fTu;
            m_fTv = fTv;
        }

        float m_fTu, m_fTv;
        DWORD m_colDiffuse;
    };

    // TVertex
    /////////////////////////////////////////////////////////////////////////////

    class TVertex : public Vector
    {
    public:
        TVertex(Vector& vV, Vector vN, float tu, float tv) : Vector(vV)
        {
            vNormal = vN;
            u = tu;
            v = tv;
        }

        Vector vNormal;
        float u, v;
    };

    // Coords
    /////////////////////////////////////////////////////////////////////////////

    class Coords
    {
        // TODO
        // - Setup the final matrix directly without matrix mults
        // - Use sin/cos tabs

    public:
        Coords(Vector& vPos, Vector& vTurn)
        {
            m_vLoc = vPos;
            m_vRot = vTurn;
            m_vScale = Vector(1.0f, 1.0f, 1.0f);
            m_bMaxtrixDirty = true;
        }

        Coords()
        {
            Vector v(0.0f, 0.0f, 0.0f);
            m_vLoc = v;
            m_vRot = v;
            m_vScale = Vector(1.0f, 1.0f, 1.0f);
            m_bMaxtrixDirty = true;
        }

    protected:
        Vector m_vLoc;
        Vector m_vRot; // in degrees
        Vector m_vScale;
        bool m_bMaxtrixDirty;
        Matrix m_mx;

    protected:
        virtual void UpdateMatrix()
        {
            DWORD dwCase = 0;

            if (!m_vLoc.IsZero()) dwCase |= 1;

            if (!m_vRot.IsZero()) dwCase |= 2;

            if (!m_vScale.IsZero()) dwCase |= 4;

            switch (dwCase)
            {
                    // Simple cases
                case 1:
                    m_mx.Translate(m_vLoc);
                    break;

                case 2:
                    m_mx.Rotate(m_vRot);
                    break;

                case 4:
                    m_mx.Scale(m_vScale);
                    break;

                    // Two pass cases
                case 3:
                {
                    // Translate / Rotate
                    Matrix mR, mT;
                    mR.Rotate(m_vRot);
                    mT.Translate(m_vLoc);
                    m_mx = mR * mT;
                    break;
                }

                case 5:
                {
                    // Translate / Scale
                    Matrix mT, mS;
                    mS.Scale(m_vScale);
                    mT.Translate(m_vLoc);
                    m_mx = mS * mT;
                    break;
                }

                case 6:
                {
                    // Rotate / Scale
                    Matrix mR, mS;
                    mS.Scale(m_vScale);
                    mR.Rotate(m_vRot);
                    m_mx = mS * mR;
                    break;
                }

                // Three pass cases
                case 7:
                {
                    Matrix mR, mS, mT;
                    mR.Rotate(m_vRot);
                    mS.Scale(m_vScale);
                    mT.Translate(m_vLoc);
                    m_mx = mS * mR * mT; // Note the order is reversed compared to the right-to-left rule
                    break;
                }
            }

            m_bMaxtrixDirty = false;
        }

    public:
        Vector& GetLocation()
        {
            return m_vLoc;
        }
        Vector& GetRotation()
        {
            return m_vRot;
        }
        Vector& GetScale()
        {
            return m_vScale;
        }

        void SetScale(Vector& vScale) // Sets the new Scaleition
        {
            m_vScale = vScale;
            m_bMaxtrixDirty = true;
        }

        void SetScale(float x, float y, float z) // Sets the new Scaleition
        {
            m_vScale.x = x;
            m_vScale.y = y;
            m_vScale.z = z;
            m_bMaxtrixDirty = true;
        }

        void SetLocation(Vector& vPos) // Sets the new position
        {
            m_vLoc = vPos;
            m_bMaxtrixDirty = true;
        }

        void SetLocation(float x, float y, float z) // Sets the new position
        {
            m_vLoc.x = x;
            m_vLoc.y = y;
            m_vLoc.z = z;
            m_bMaxtrixDirty = true;
        }

        void SetRotation(Vector& vTurn) // Sets the new orientation (in degrees)
        {
            m_vRot = vTurn;
            m_bMaxtrixDirty = true;
        }

        void SetRotation(float x, float y, float z) // Sets the new orientation (in degrees)
        {
            m_vRot.x = x;
            m_vRot.y = y;
            m_vRot.z = z;
            m_bMaxtrixDirty = true;
        }

        void DeltaLocation(Vector& vPos)
        {
            m_vLoc += vPos;
            m_bMaxtrixDirty = true;
        }

        void DeltaLocation(float x, float y, float z)
        {
            m_vLoc.x += x;
            m_vLoc.y += y;
            m_vLoc.z += z;
            m_bMaxtrixDirty = true;
        }

        void DeltaRotation(Vector& vTurn)
        {
            m_vRot += vTurn;
            m_bMaxtrixDirty = true;
        }

        void DeltaRotation(float x, float y, float z)
        {
            m_vRot.x += x;
            m_vRot.y += y;
            m_vRot.z += z;
            m_bMaxtrixDirty = true;
        }

        void ClampRotation()
        {
            m_vRot.x -= (((int) m_vRot.x / 360) * 360);
            m_vRot.y -= (((int) m_vRot.y / 360) * 360);
            m_vRot.z -= (((int) m_vRot.z / 360) * 360);
        }

        void RideX(float fDelta)
        {
            m_vLoc.x += fDelta * (float) cos(m_vRot.y * PI_FRAC);
            m_vLoc.y += fDelta * (float) sin(m_vRot.y * PI_FRAC);
            m_vLoc.z += fDelta * (float) sin(m_vRot.z * PI_FRAC);

            m_bMaxtrixDirty = true;
        }

        void RideY(float fDelta)
        {
            m_vLoc.y += fDelta * (float) cos(m_vRot.z * PI_FRAC);
            m_vLoc.x += fDelta * (float) sin(m_vRot.z * PI_FRAC);
            m_vLoc.z += fDelta * (float) sin(m_vRot.x * PI_FRAC);

            m_bMaxtrixDirty = true;
        }

        void RideZ(float fDelta)
        {
            m_vLoc.z += fDelta * (float) cos(m_vRot.y * PI_FRAC);
            m_vLoc.x += fDelta * (float) sin(m_vRot.y * PI_FRAC);
            m_vLoc.y += fDelta * (float) sin(-m_vRot.x * PI_FRAC);

            m_bMaxtrixDirty = true;
        }

        Matrix& GetMatrix()
        {
            if (m_bMaxtrixDirty) UpdateMatrix();

            return m_mx;
        }

        operator const Matrix()
        {
            return GetMatrix();
        }

        operator D3DMATRIX *()
        {
            return (D3DMATRIX *) &GetMatrix();
        }
    };

    // Camera
    /////////////////////////////////////////////////////////////////////////////

    class Camera : public Coords
    {
        // The differences:
        // - sign inverted
        // - Matrices are multiplied in the (correct) right-to-left order

    protected:
        virtual void UpdateMatrix()
        {
            // CheckTurn();
            Matrix mR, mT;
            mR.Rotate(-m_vRot.x, -m_vRot.y, -m_vRot.z);
            mT.Translate(-m_vLoc.x, -m_vLoc.y, -m_vLoc.z);
            m_mx = mT * mR;

            m_bMaxtrixDirty = false;
        }
    };

    // Misc operators
    /////////////////////////////////////////////////////////////////////////////

    // Mirror a vector about a normal vector.
    inline Vector Vector::MirrorByVector(const Vector& MirrorNormal) const
    {
        return *this - MirrorNormal * (2.0f * (*this | MirrorNormal));
    }

    // Mirror a vector about a plane.
    inline Vector Vector::MirrorByPlane(const Plane& Plane) const
    {
        return *this - Plane * (2.0f * Plane.PlaneDot(*this));
    }

    // Helpers
    /////////////////////////////////////////////////////////////////////////////

    inline float FSnap(FLOAT Location, FLOAT Grid)
    {
        if (Grid == 0.0) return Location;
        else return (float) appFloor((Location + 0.5 * Grid) / Grid) * Grid;
    }

    // Find the intersection of an infinite line (defined by two points) and
    // a plane.  Assumes that the line and plane do indeed intersect; you must
    // make sure they're not parallel before calling.

    inline Vector LinePlaneIntersection(const Vector &Point1, const Vector &Point2,
                                        const Vector &PlaneOrigin, const Vector &PlaneNormal)
    {
        return Point1 + (Point2 - Point1) * (((PlaneOrigin - Point1) | PlaneNormal) / ((Point2 - Point1) | PlaneNormal));
    }

    inline Vector LinePlaneIntersection(const Vector &Point1, const Vector &Point2, const Plane &Plane)
    {
        return Point1 + (Point2 - Point1) * ((Plane.w - (Point1 | Plane)) / ((Point2 - Point1) | Plane));
    }

    // Compute intersection point and direction of line joining two planes.
    // Return 1 if valid, 0 if infinite.
    inline int IntersectPlanes2(Vector& I, Vector& D, const Plane& P1, const Plane& P2)
    {
        // Compute line direction, perpendicular to both plane normals.
        D = P1 ^ P2;
        float DD = D.SizeSquared();

        if (DD < Square(0.001))
        {
            // Parallel or nearly parallel planes.
            D = I = Vector(0, 0, 0);
            return 0;
        }
        else
        {
            // Compute intersection.
            I = (P1.w * (P2 ^ D) + P2.w * (D ^ P1)) / DD;
            D.Normalize();
            return 1;
        }
    }

    // Calculate the signed distance (in the direction of the normal) between a point and a plane.
    inline float PointPlaneDist(const Vector &Point, const Vector &PlaneBase, const Vector &PlaneNormal)
    {
        return (Point - PlaneBase) | PlaneNormal;
    }

    // See if two normal vectors (or plane normals) are nearly parallel.
    inline bool Parallel(const Vector &Normal1, const Vector &Normal2)
    {
        float NormalDot = Normal1 | Normal2;
        return (Abs(NormalDot - 1.0) <= THRESH_VECTORS_ARE_PARALLEL);
    }

    // See if two planes are coplanar.
    inline bool Coplanar(const Vector &Base1, const Vector &Normal1, const Vector &Base2, const Vector &Normal2)
    {
        if (!Parallel(Normal1, Normal2)) return false;
        else if (PointPlaneDist(Base2, Base1, Normal1) > THRESH_POINT_ON_PLANE) return false;
        else return true;
    }

    inline Matrix operator*(const Matrix& a, const Matrix& b)
    {
        Matrix ret;

        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                ret.m[i][j] = 0.0f;

                for (int k = 0; k < 4; k++)
                    ret.m[i][j] += a.m[i][k] * b.m[k][j];
            }
        }

        return ret;
    }

    inline Vector operator*(Vector &v, Matrix &mat)
    {
        float tx = v.x * mat.m[0][0] + v.y * mat.m[1][0] + v.z * mat.m[2][0] + mat.m[3][0];
        float ty = v.x * mat.m[0][1] + v.y * mat.m[1][1] + v.z * mat.m[2][1] + mat.m[3][1];
        float tz = v.x * mat.m[0][2] + v.y * mat.m[1][2] + v.z * mat.m[2][2] + mat.m[3][2];
        return Vector(tx, ty, tz);
    }

}; // namespace D3DFrame
