#ifndef _VECTOR3_ENVELOPE_CURVE_H
#define _VECTOR3_ENVELOPE_CURVE_H
//------------------------------------------------------------------------------
//  CLASS
//  vector3 envelope curve -- simple modulation function for float3's
//------------------------------------------------------------------------------
#include "nmath.h"
#include "nvector.h"

//------------------------------------------------------------------------------
//  nColorEnvelopeVurve
//------------------------------------------------------------------------------
class nVector3EnvelopeCurve
{
public:
    /// constructor 1
    nVector3EnvelopeCurve();
    /// constructor 2
    nVector3EnvelopeCurve(const vector3& keyFrameValue0, const vector3& keyFrameValue1,
        const vector3& keyFrameValue2, const vector3& keyFrameValue3,
        const float keyFramePos1, const float keyFramePos2);
    // set all parameters
    void SetParameters(const vector3& keyFrameValue0, const vector3& keyFrameValue1,
        const vector3& keyFrameValue2, const vector3& keyFrameValue3,
        const float keyFramePos1, const float keyFramePos2);
    // assign to other color envelope curve
    void SetParameters(const nVector3EnvelopeCurve& src);
    /// get the function value; pos must be between 0 and 1
    vector3 GetValue(float pos) const;

    vector3 keyFrameValues[4];
    float keyFramePos1, keyFramePos2;   // 0 through 1
    float frequency, amplitude;         // parameters of the sinus function
};

//------------------------------------------------------------------------------
/**
*/
inline
nVector3EnvelopeCurve::nVector3EnvelopeCurve() :
    keyFramePos1(.2f),
    keyFramePos2(.8f)
{
    keyFrameValues[0] = vector3(1.0f, 1.0f, 1.0f);
    keyFrameValues[1] = vector3(1.0f, 1.0f, 1.0f);
    keyFrameValues[2] = vector3(1.0f, 1.0f, 1.0f);
    keyFrameValues[3] = vector3(1.0f, 1.0f, 1.0f);
}

//------------------------------------------------------------------------------
/**
*/
inline
nVector3EnvelopeCurve::nVector3EnvelopeCurve(const vector3& keyFrameValue0, 
        const vector3& keyFrameValue1, const vector3& keyFrameValue2,
        const vector3& keyFrameValue3, const float keyFramePos1,
        const float keyFramePos2) :
    keyFramePos1(keyFramePos1),
    keyFramePos2(keyFramePos2)
{
    this->keyFrameValues[0] = keyFrameValue0;
    this->keyFrameValues[1] = keyFrameValue1;
    this->keyFrameValues[2] = keyFrameValue2;
    this->keyFrameValues[3] = keyFrameValue3;
}

//------------------------------------------------------------------------------
/**
*/
inline
void nVector3EnvelopeCurve::SetParameters(const vector3& keyFrameValue0, const vector3& keyFrameValue1,
    const vector3& keyFrameValue2, const vector3& keyFrameValue3,
    const float keyFramePos1, const float keyFramePos2)
{
    this->keyFrameValues[0] = keyFrameValue0;
    this->keyFrameValues[1] = keyFrameValue1;
    this->keyFrameValues[2] = keyFrameValue2;
    this->keyFrameValues[3] = keyFrameValue3;
    this->keyFramePos1 = keyFramePos1;
    this->keyFramePos2 = keyFramePos2;
}
//------------------------------------------------------------------------------
/**
*/
inline
void nVector3EnvelopeCurve::SetParameters(const nVector3EnvelopeCurve& src)
{
    this->keyFrameValues[0] = src.keyFrameValues[0];
    this->keyFrameValues[1] = src.keyFrameValues[1];
    this->keyFrameValues[2] = src.keyFrameValues[2];
    this->keyFrameValues[3] = src.keyFrameValues[3];
    this->keyFramePos1 = src.keyFramePos1;
    this->keyFramePos2 = src.keyFramePos2;
}

//------------------------------------------------------------------------------
/**
*/
inline
vector3 nVector3EnvelopeCurve::GetValue(float pos) const
{
    _assert(pos >= 0.0);
    _assert(pos <= 1.0);

    vector3 linearValue;

    if (pos < this->keyFramePos1)
    {
        linearValue = this->keyFrameValues[1];
        linearValue.lerp(this->keyFrameValues[0], 
            (pos / this->keyFramePos1));
    }
    else if (pos < this->keyFramePos2)
    {
        linearValue = this->keyFrameValues[2];
        linearValue.lerp(this->keyFrameValues[1], 
            (pos-this->keyFramePos1) / (this->keyFramePos2-this->keyFramePos1));
    }
    else
    {
        linearValue = this->keyFrameValues[3];
        linearValue.lerp(this->keyFrameValues[2],
            (pos-this->keyFramePos2) / (1.0f-this->keyFramePos2));
    }

    return linearValue;
}

//------------------------------------------------------------------------------
#endif
