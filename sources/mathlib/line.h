#ifndef _LINE_H
#define _LINE_H

#include "nvector.h"

//-------------------------------------------------------------------
class line2 {
public:
    vector2 b;
    vector2 m;

    line2() {};
    line2(const vector2& v0, const vector2& v1) : b(v0), m(v1-v0) {};
    line2(const line2& l) : b(l.b), m(l.m) {};

    //--- minimal distance of point to line -------------------------
/*
    float distance(const vector2&)
    {
        return 0.0f;
    };
*/

    const vector2& start(void) const
    {
        return b;
    };
    vector2 end(void) const
    {
        return (b+m);
    };
    float len(void) const
    {
        return m.len();
    };
    //--- get 3d point on line given t ------------------------------
    vector2 ipol(const float t) const
    {
        return vector2(b + m*t);
    };
};

/**
    @class line3
    @ingroup NebulaMathDataTypes

    a line in 3d space

     - 04-Dec-01   floh    bugfix: line3::len() was broken
*/
//-------------------------------------------------------------------
class line3 {
public:
    vector3 b;
    vector3 m;

    line3() {};
    line3(const vector3& v0, const vector3& v1) : b(v0), m(v1-v0) {};
    line3(const line3& l) : b(l.b), m(l.m) {};

    void set(const vector3& v0, const vector3& v1)
    {
        b = v0;
        m = v1-v0;
    };
    const vector3& start(void) const
    {
        return b;
    };
    vector3 end(void) const
    {
        return (b+m);
    };
    float len(void) const
    {
        return m.len();
    };
    float len_squared(void) const
    {
        return m.lensquared();
    }
    //--- minimal distance of point to line -------------------------
    float distance(const vector3& p) {
        vector3 diff(p-b);
        float l = (m % m);
        if (l > 0.0f) {
            float t = (m % diff) / l;
            diff = diff - m*t;
            return diff.len();
        } else {
            // line is really a point...
            vector3 v(p-b);
            return v.len();
        }
    };

    //--- get 3d point on line given t ------------------------------
    vector3 ipol(const float t) const
    {
        return vector3(b + m*t);
    };
};
//-------------------------------------------------------------------
#endif
