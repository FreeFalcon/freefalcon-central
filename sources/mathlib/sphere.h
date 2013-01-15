#ifndef _SPHERE_H
#define _SPHERE_H

#include "nvector.h"
#include "bbox.h"

//-------------------------------------------------------------------
class sphere {
public:
    vector3 p;      // position
    float   r;      // radius

    //--- constructors ----------------------------------------------
    sphere() : r(1.0f) {};
    sphere(const vector3& _p, float _r) : p(_p), r(_r) {};
    sphere(const sphere& s) : p(s.p), r(s.r) {};
    sphere(float _x, float _y, float _z, float _r) 
        : r(_r)
    {
        p.set(_x,_y,_z);
    };

    //--- set position and radius ---
    void set(const vector3& _p, float _r)
    {
        p = _p;
        r = _r;
    };
    void set(float _x, float _y, float _z, float _r)
    {
        p.set(_x, _y, _z);
        r = _r;
    };

    //--- check if 2 spheres overlap, without contact point ---------
    bool intersects(const sphere& s) const 
    {
        vector3 d(s.p-p);
        float rsum = s.r+r;
        if (d.lensquared() <= (rsum*rsum)) return true;
        else                               return false;
    };

    /**
        @brief Check if sphere intersects with box.
        Taken from "Simple Intersection Tests For Games",
        Gamasutra, Oct 18 1999
    */
    bool intersects(const bbox3& box) const
    {
        float s, d = 0;

        // find the square of the distance
        // from the sphere to the box,
        if (p.x < box.vmin.x)
        {
            s = p.x - box.vmin.x;
            d += s*s;
        }
        else if (p.x > box.vmax.x)
        {
            s = p.x - box.vmax.x;
            d += s*s;
        }

        if (p.y < box.vmin.y)
        {
            s = p.y - box.vmin.y;
            d += s*s;
        }
        else if (p.y > box.vmax.y)
        {
            s = p.y - box.vmax.y;
            d += s*s;
        }

        if (p.z < box.vmin.z)
        {
            s = p.z - box.vmin.z;
            d += s*s;
        }
        else if (p.z > box.vmax.z)
        {
            s = p.z - box.vmax.z;
            d += s*s;
        }

        return d <= r*r;
    }

    //--- check if 2 moving spheres have contact --------------------
    //--- taken from "Simple Intersection Tests For Games" ----------
    //--- article in Gamasutra, Oct 18 1999 -------------------------
    bool intersect_sweep(const vector3& va,     // in: distance travelled by 'this'
                         const sphere&  sb,     // in: the other sphere
                         const vector3& vb,     // in: distance travelled by 'sb'
                         float& u0,             // out: normalized intro contact u0
                         float& u1)             // out: normalized outro contact u1
    {
        vector3 vab(vb - va);
        vector3 ab(sb.p - p);
        float rab = r + sb.r;

        // check if spheres are currently overlapping...
        if ((ab % ab) <= (rab*rab)) {
            u0 = 0.0f;
            u1 = 0.0f;
            return true;
        } else {
            // check if they hit each other
            float a = vab % vab;
            if ((a<-TINY) || (a>+TINY)) {
                // if a is '0' then the objects don't move relative to each other
                float b = (vab % ab) * 2.0f;
                float c = (ab % ab) - (rab * rab);
                float q = b*b - 4*a*c;
                if (q >= 0.0f) {
                    // 1 or 2 contacts
                    float sq = (float) sqrt(q);
                    float d  = 1.0f / (2.0f*a);
                    float r1 = (-b + sq) * d;
                    float r2 = (-b - sq) * d;
                    if (r1 < r2) {
                        u0 = r1;
                        u1 = r2;
                    } else {
                        u0 = r2;
                        u1 = r1;
                    }
                    return true;
                } else return false;
            } else return false;
        }
    };
};

//-------------------------------------------------------------------
#endif
