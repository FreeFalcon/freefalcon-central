#ifndef _TRANSFORM44_H
#define _TRANSFORM44_H

#include "matrix.h"
#include "vector.h"

//------------------------------------------------------------------------------
class transform44
{
public:
    /// constructor
    transform44();
    /// set translation
    void settranslation(const vector3& v);
    /// get translation
    const vector3& gettranslation() const;
    /// set euler rotation
    void seteulerrotation(const vector3& v);
    /// get euler rotation
    const vector3& geteulerrotation() const;
    /// set quaternion rotation
    void setquatrotation(const quaternion& q);
    /// get quaternion rotation
    const quaternion& getquatrotation() const;
    /// return true if euler rotation is used (otherwise quaternion rotation is used)
    bool iseulerrotation() const;
    /// return true if the transformation matrix is dirty
    bool isdirty() const;
    /// set scale
    void setscale(const vector3& v);
    /// get scale
    const vector3& getscale() const;
    /// get resulting 4x4 matrix
    const matrix44& getmatrix();

private:
    enum
    {
        Dirty = (1<<0),
        UseEuler = (1<<1),
    };
    vector3 translation;
    vector3 euler;
    quaternion quat;
    vector3 scale;
    matrix44 matrix;
    uchar flags;
};

//------------------------------------------------------------------------------
/**
*/
inline
transform44::transform44() :
    scale(1.0f, 1.0f, 1.0f),
    flags(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
transform44::settranslation(const vector3& v)
{
    this->translation = v;
    this->flags |= Dirty;
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector3&
transform44::gettranslation() const
{
    return this->translation;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
transform44::seteulerrotation(const vector3& v)
{
    this->euler = v;
    this->flags |= (Dirty | UseEuler);
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector3&
transform44::geteulerrotation() const
{
    return this->euler;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
transform44::setquatrotation(const quaternion& q)
{
    this->quat = q;
    this->flags |= Dirty;
    this->flags &= ~UseEuler;
}

//------------------------------------------------------------------------------
/**
*/
inline
const quaternion&
transform44::getquatrotation() const
{
    return this->quat;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
transform44::setscale(const vector3& v)
{
    this->scale = v;
    this->flags |= Dirty;
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector3&
transform44::getscale() const
{
    return this->scale;
}

//------------------------------------------------------------------------------
/**
*/
inline
const matrix44&
transform44::getmatrix()
{
    if (this->flags & Dirty)
    {
        if (this->flags & UseEuler)
        {
            this->matrix.ident();
            this->matrix.scale(this->scale);
            this->matrix.rotate_x(this->euler.x);
            this->matrix.rotate_y(this->euler.y);
            this->matrix.rotate_z(this->euler.z);
            this->matrix.translate(this->translation);
        }
        else
        {
            this->matrix.ident();
            this->matrix.scale(this->scale);
            this->matrix.mult_simple(matrix44(this->quat));
            this->matrix.translate(this->translation);
        }
        this->flags &= ~Dirty;
    }
    return this->matrix;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
transform44::iseulerrotation() const
{
    return (0 != (this->flags & UseEuler));
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
transform44::isdirty() const
{
    return (0 != (this->flags & Dirty));
}

//------------------------------------------------------------------------------
#endif

    
    