#include "stdhdr.h"
#include "simbase.h"
#include "simmover.h"
#include "object.h"

extern float SimLibLastMajorFrameTime;

#ifdef USE_SH_POOLS
MEM_POOL ObjectGeometry::pool;
#endif

void CalcTransformMatrix(SimBaseClass* theObject);

// -----------------------------------------------------
//
// CalcRelAzEl()
//
// Added by Vince 7/1/98.  I needed a way to get the
// azimuth and elevation of a point relative to the
// player entity.  I would have used the functions
// TargetAz() and TargetEl() but these don't seem to
// take the platform transformation into consideration.
// Also I would have relied upon the az and el that
// CalcRelGeom() calculates for the target list elements,
// but CalcRelGeom() only does this for vehicles and not
// features. I need to get az and el for features and
// possibly arbitrary points (Gilman mentioned that it would
// be nice if the player can padlock a marked point on the
// ground (cringe).  Have a FreeFalcon day.
//
// -----------------------------------------------------

void CalcRelAzEl(SimBaseClass* ownObject, float x, float y, float z, float* az, float* el)
{
    float xft;
    float yft;
    float zft;
    float rx;
    float ry;
    float rz;

    xft = x - ownObject->XPos();
    yft = y - ownObject->YPos();
    zft = z - ownObject->ZPos();

    rx = ownObject->dmx[0][0] * xft + ownObject->dmx[0][1] * yft + ownObject->dmx[0][2] * zft;
    ry = ownObject->dmx[1][0] * xft + ownObject->dmx[1][1] * yft + ownObject->dmx[1][2] * zft;
    rz = ownObject->dmx[2][0] * xft + ownObject->dmx[2][1] * yft + ownObject->dmx[2][2] * zft;

    *az = (float)atan2(ry, rx);
    *el = (float)atan2(-rz, sqrt(rx * rx + ry * ry));
}



// -----------------------------------------------------
//
// CalcRelValues()
//
// Added by Vince 7/1/98.  I needed a way to get ATA
// and DROLL of a point relative to the
// player entity.  This is needed for the EFOV padlocking
// because the target list doesn't include feature entities
// and therefore the relative geometry doesn't get
// calculated for us each frame for those entities.
// Instead we need to calculate ATA and DROLL explicitly.
//
// -----------------------------------------------------

void CalcRelValues(SimBaseClass* ownObject, FalconEntity* target, float* az, float* el, float* ata, float* ataFrom, float* droll)
{
    float xft;
    float yft;
    float zft;
    float rx;
    float ry;
    float rz;
    float range;
    float xfrm00;
    float xfrm01;
    float xfrm02;
    float rx_t;
    mlTrig trigtha;
    mlTrig trigpsi;

    xft = target->XPos() - ownObject->XPos();
    yft = target->YPos() - ownObject->YPos();
    zft = target->ZPos() - ownObject->ZPos();

    rx = ownObject->dmx[0][0] * xft + ownObject->dmx[0][1] * yft + ownObject->dmx[0][2] * zft;
    ry = ownObject->dmx[1][0] * xft + ownObject->dmx[1][1] * yft + ownObject->dmx[1][2] * zft;
    rz = ownObject->dmx[2][0] * xft + ownObject->dmx[2][1] * yft + ownObject->dmx[2][2] * zft;

    range = (float)(xft * xft + yft * yft + zft * zft);

    mlSinCos(&trigtha, target->Pitch());
    mlSinCos(&trigpsi, target->Yaw());

    xfrm00 = trigpsi.cos * trigtha.cos;
    xfrm01 = trigpsi.sin * trigtha.cos;
    xfrm02 = -trigtha.sin;

    rx_t = -(xfrm00 * xft + xfrm01 * yft + xfrm02 * zft);

    rz = -rz;

    *ata = (float)atan2(sqrt(range - rx * rx), rx);
    *ataFrom = (float)atan2(sqrt(range - rx_t * rx_t), rx_t);
    *droll = (float)atan2(ry, rz);
    *az = (float)atan2(ry, rx);
    *el = (float)atan2(rz, sqrt(rx * rx + ry * ry));
}

// -----------------------------------------------------
//
// KCK: This is for ground vehicles, who only need Az, El, Range and ATAFrom
// This assumes ONLY Yaw() is considered for orientation.  The
// orientation matrix is ignored.
// NOTE:  This will result in misaimed weapons on ground vehicles
// shooting from sloped terrain.
//
// YIKES:  This defines az differently than the original "CalcRelGeom"
// below (and as assumed by most of the rest of the sim).  In this case,
// az is "bearing from North" whereas it used to be "relative az" or
// "angle off my nose".
//
// -----------------------------------------------------
void CalcRelAzElRangeAta(SimBaseClass* ownObject, SimObjectType* targetPtr)
{
    float xft, yft, zft, rx_t;
    float range;
    FalconEntity* theObject;
    mlTrig psi;

    theObject = targetPtr->BaseData();

    if ( not theObject)
        return;

    xft = theObject->XPos() - ownObject->XPos();
    yft = theObject->YPos() - ownObject->YPos();
    zft = theObject->ZPos() - ownObject->ZPos();

    // This is the only place CalcTransformMatrix() happens (other than Init()) for
    // ground vehicles, I think. It's quite possible we can do away with it entirely,
    // as Ed isn't using it anywhere any how.
    // CalcTransformMatrix();

    mlSinCos(&psi, theObject->Yaw());
    rx_t = -(psi.cos * xft + psi.sin * yft);

    range = (float)(xft * xft + yft * yft + zft * zft);

    targetPtr->localData->range = (float)sqrt(range);
    targetPtr->localData->ataFrom = (float)atan2(sqrt(range - rx_t * rx_t), rx_t);
    targetPtr->localData->az = (float)atan2(yft, xft);
    targetPtr->localData->el = (float)atan2(-zft, sqrt(range - zft * zft));
}

void CalcRelGeom(SimBaseClass* ownObject, SimObjectType* targetList, TransformMatrix vmat, float elapsedTimeInverse)
{
    float ata, ataFrom;
    float azFrom, elFrom;
    float xft, yft, zft, rx, ry, rz, rx_t, ry_t, rz_t;
    float range;
    int fucked_count;
    unsigned long lastUpdate;
    SimObjectType* obj;
    FalconEntity* theObject;
    SimObjectLocalData* objData;
    float inverseTimeDelta;

    /*---------------------------------------*/
    /* Velocity Vector transformation matrix */
    /*---------------------------------------*/
    if (vmat)
    {
        vmat[0][0] = ownObject->platformAngles.cossig *
                     ownObject->platformAngles.cosgam;
        vmat[0][1] = ownObject->platformAngles.sinsig *
                     ownObject->platformAngles.cosgam;
        vmat[0][2] = -ownObject->platformAngles.singam;

        vmat[1][0] = -ownObject->platformAngles.sinsig *
                     ownObject->platformAngles.cosmu +
                     ownObject->platformAngles.cossig *
                     ownObject->platformAngles.singam *
                     ownObject->platformAngles.sinmu;
        vmat[1][1] = ownObject->platformAngles.cossig *
                     ownObject->platformAngles.cosmu +
                     ownObject->platformAngles.sinsig *
                     ownObject->platformAngles.singam *
                     ownObject->platformAngles.sinmu;
        vmat[1][2] = ownObject->platformAngles.cosgam *
                     ownObject->platformAngles.sinmu;

        vmat[2][0] = ownObject->platformAngles.sinsig *
                     ownObject->platformAngles.sinmu +
                     ownObject->platformAngles.cossig *
                     ownObject->platformAngles.singam *
                     ownObject->platformAngles.cosmu;
        vmat[2][1] = -ownObject->platformAngles.cossig *
                     ownObject->platformAngles.sinmu +
                     ownObject->platformAngles.sinsig *
                     ownObject->platformAngles.singam *
                     ownObject->platformAngles.cosmu;
        vmat[2][2] = ownObject->platformAngles.cosgam *
                     ownObject->platformAngles.cosmu;
    }

    /*------------------------------*/
    /* Do each plane execpt ownship */
    /*------------------------------*/
    fucked_count = 0; // HACK HACK HACK HACK HACK HACK HACK hackerydoodar HACK
    obj = targetList;

    while (obj)
    {
        objData = obj->localData;

        theObject = obj->BaseData();

        if (theObject == NULL)
        {
            obj = obj->next;
            continue;
        }

        lastUpdate = theObject->LastUpdateTime();

        if (lastUpdate == vuxGameTime)
            inverseTimeDelta = elapsedTimeInverse;
        else
        {
            inverseTimeDelta = 1.0F / SimLibLastMajorFrameTime;
        }

        xft = theObject->XPos() - ownObject->XPos();
        yft = theObject->YPos() - ownObject->YPos();
        zft = theObject->ZPos() - ownObject->ZPos();

        rx = ownObject->dmx[0][0] * xft + ownObject->dmx[0][1] * yft + ownObject->dmx[0][2] * zft;
        ry = ownObject->dmx[1][0] * xft + ownObject->dmx[1][1] * yft + ownObject->dmx[1][2] * zft;
        rz = ownObject->dmx[2][0] * xft + ownObject->dmx[2][1] * yft + ownObject->dmx[2][2] * zft;

        if (theObject->IsSim())
        {
            rx_t = -(((SimBaseClass*)theObject)->dmx[0][0] * xft + ((SimBaseClass*)theObject)->dmx[0][1] * yft +
                     ((SimBaseClass*)theObject)->dmx[0][2] * zft);
            ry_t = -(((SimBaseClass*)theObject)->dmx[1][0] * xft + ((SimBaseClass*)theObject)->dmx[1][1] * yft +
                     ((SimBaseClass*)theObject)->dmx[1][2] * zft);
            rz_t = -(((SimBaseClass*)theObject)->dmx[2][0] * xft + ((SimBaseClass*)theObject)->dmx[2][1] * yft +
                     ((SimBaseClass*)theObject)->dmx[2][2] * zft);
        }
        else
        {
            mlTrig psi;

            // KCK: Optimized because Roll() and Pitch() are always zero for campaign objects
            mlSinCos(&psi, theObject->Yaw());
            rx_t = -(psi.cos * xft + psi.sin * yft);
            ry_t = -(-psi.sin * xft + psi.cos * yft);
            rz_t = -(zft);
        }

        range = (float)(xft * xft + yft * yft + zft * zft);
        range = max(range, 1.0F);
        rz    = -rz;
        rz_t = -rz_t;

        ata            = (float)atan2(sqrt(range - rx * rx), rx);
        ataFrom        = (float)atan2(sqrt(range - rx_t * rx_t), rx_t);

        objData->az    = (float)atan2(ry, rx);
        objData->droll = (float)atan2(ry, rz);
        objData->el    = (float)atan2(rz, sqrt(range - rz * rz));
        azFrom         = (float)atan2(ry_t, rx_t);
        elFrom         = (float)atan2(rz_t, sqrt(range - rz_t * rz_t));

        range = (float)sqrt(range);
        objData->azFromdot  = (azFrom - objData->azFrom) * inverseTimeDelta;
        objData->elFromdot  = (elFrom - objData->elFrom) * inverseTimeDelta;
        objData->atadot      = (ata - objData->ata) * inverseTimeDelta;
        objData->ataFromdot = (ataFrom - objData->ataFromdot) * inverseTimeDelta;
        objData->ata = ata;
        objData->range = range;
        objData->azFrom = azFrom;
        objData->elFrom = elFrom;
        objData->ataFrom = ataFrom;

        // Range dot, roughly frame rate invariant
        rx = theObject->XDelta() - ownObject->XDelta();
        ry = theObject->YDelta() - ownObject->YDelta();
        rz = theObject->ZDelta() - ownObject->ZDelta();
        objData->rangedot = (rx * xft + ry * yft + rz * zft) / range;

        obj = obj->next;
    }
}

// Azimuth from af1 to af2, assuming no roll
float TargetAz(FalconEntity* af1, FalconEntity* af2)
{
    float xft, yft, rx, ry, az;
    mlTrig psiTrig;

    mlSinCos(&psiTrig, af1->Yaw());

    xft = af2->XPos() - af1->XPos();
    yft = af2->YPos() - af1->YPos();

    rx =  psiTrig.cos * xft + psiTrig.sin * yft;
    ry = -psiTrig.sin * xft + psiTrig.cos * yft;

    az = (float)atan2(ry, rx);

    return(az);
}

float TargetAz(FalconEntity* af1, SimObjectType* af2)
{
    float xft, yft, rx, ry, az;
    mlTrig psiTrig;

    mlSinCos(&psiTrig, af1->Yaw());

    xft = af2->BaseData()->XPos() - af1->XPos();
    yft = af2->BaseData()->YPos() - af1->YPos();

    rx =  psiTrig.cos * xft + psiTrig.sin * yft;
    ry = -psiTrig.sin * xft + psiTrig.cos * yft;

    az = (float)atan2(ry, rx);

    return(az);
}

float TargetAz(FalconEntity* af1, float x, float y)
{
    float xft, yft, rx, ry, az;
    mlTrig psiTrig;

    mlSinCos(&psiTrig, af1->Yaw());

    xft = x - af1->XPos();
    yft = y - af1->YPos();

    rx =  psiTrig.cos * xft + psiTrig.sin * yft;
    ry = -psiTrig.sin * xft + psiTrig.cos * yft;

    az = (float)atan2(ry, rx);

    return(az);
}

// Elevation from af1 to af2, assuming no roll
float TargetEl(FalconEntity* af1, FalconEntity* af2)
{
    float rx, ry, rz, el;

    rx = af2->XPos() - af1->XPos();
    ry = af2->YPos() - af1->YPos();
    rz = af2->ZPos() - af1->ZPos();

    /* sqrt returns positive, so this is cool */
    el = (float)atan2(-rz, sqrt(rx * rx + ry * ry));

    return(el);
}

float TargetEl(FalconEntity* af1, SimObjectType* af2)
{
    float rx, ry, rz, el;

    rx = af2->BaseData()->XPos() - af1->XPos();
    ry = af2->BaseData()->YPos() - af1->YPos();
    rz = af2->BaseData()->ZPos() - af1->ZPos();

    /* sqrt returns positive, so this is cool */
    el = (float)atan2(-rz, sqrt(rx * rx + ry * ry));

    return(el);
}

float TargetEl(FalconEntity* af1, float x, float y, float z)
{
    float rx, ry, rz, el;

    rx = x - af1->XPos();
    ry = y - af1->YPos();
    rz = z - af1->ZPos();

    /* sqrt returns positive, so this is cool */
    el = (float)atan2(-rz, sqrt(rx * rx + ry * ry));

    return(el);
}

void TargetAzEl(FalconEntity* af1, float x, float y, float z, float &az, float &el)
{
    mlTrig psiTrig;
    float rx, ry, rz;
    float rx2, ry2;

    mlSinCos(&psiTrig, af1->Yaw());

    rx = x - af1->XPos();
    ry = y - af1->YPos();
    rz = z - af1->ZPos();

    /* sqrt returns positive, so this is cool */
    el = (float)atan2(-rz, sqrt(rx * rx + ry * ry));
    az = (float)atan2(ry, rx);

    rx2 =  psiTrig.cos * rx + psiTrig.sin * ry;
    ry2 = -psiTrig.sin * rx + psiTrig.cos * ry;

    az = (float)atan2(ry2, rx2);
}



void GetXYZ(SimBaseClass *platform,
            float az, float el, float range, float *x, float *y, float *z)
{
    float rx, ry, rz;
    float xft, yft, zft;
    float xy_range;
    mlTrig trigAz, trigEl;

    mlSinCos(&trigAz, az);
    mlSinCos(&trigEl, el);
    /*--------------------------*/
    /* Math done more than once */
    /*--------------------------*/
    xy_range = range * trigEl.cos;

    /*---------------------------------*/
    /* Three equations, three unknowns */
    /* This gives relative x, y, z     */
    /*---------------------------------*/
    rz = -range * trigEl.sin;
    rx = xy_range * trigAz.cos;
    ry = xy_range * trigAz.sin;

    /*--------------------------------------------*/
    /* The transformation matrix is the transpose */
    /* of the AC/ Inertial matrix                 */
    /*--------------------------------------------*/
    xft = platform->dmx[0][0] * rx + platform->dmx[1][0] * ry + platform->dmx[2][0] * rz;
    yft = platform->dmx[0][1] * rx + platform->dmx[1][1] * ry + platform->dmx[2][1] * rz;
    zft = platform->dmx[0][2] * rx + platform->dmx[1][2] * ry + platform->dmx[2][2] * rz;

    *x = xft + platform->XPos();
    *y = yft + platform->YPos();
    *z = zft + platform->ZPos();
}

int FindCollisionPoint(SimBaseClass* obj, SimBaseClass* ownShip, vector* collPoint, float speedBoost)
{
    int retval;
    vector q;  /* really a point */
    float a;
    float b;
    float c;
    float underRad;
    float minT;
    float maxT;
    float ownSpeed;

    /* put line seg equation into parametric form S = Q+tV,
       translated into sphere's coord system.

       Note (this is important) that the way we have defined it,
       the line segment includes all values of t where (0 <= t <= 1)
    */

    q.x = obj->XPos() - ownShip->XPos();
    q.y = obj->YPos() - ownShip->YPos();
    q.z = obj->ZPos() - ownShip->ZPos();

    /* Calculate ownships speed */
    ownSpeed = ownShip->XDelta() * ownShip->XDelta() +
               ownShip->YDelta() * ownShip->YDelta() +
               ownShip->ZDelta() * ownShip->ZDelta() + speedBoost;

    /* solve for t on sphere's surface,  ie. where S.S = R*R, or

       t*t(objVel.objVel) + t(2objVel.Q) + (Q.Q - ownSpeed*t * ownSpeed*t ) = 0

       which is quadratic in t such that:

       a = V.V - ownSpeed*ownSpeed
       b = 2V.Q
       c = Q.Q

       First, see if there is a real solution (ie.  (b*b - 4*a*c) >= 0 )
    */

    a = obj->XDelta() * obj->XDelta() + obj->YDelta() * obj->YDelta() +
        obj->ZDelta() * obj->ZDelta() - ownSpeed;
    b = (obj->XDelta() * q.x + obj->YDelta() * q.y + obj->ZDelta() * q.z) * 2.0F;
    c = q.x * q.x + q.y * q.y + q.z * q.z;

    underRad = b * b - 4.0F * a * c;

    if (underRad < 0)
    {
        /* line does not intersect sphere */
        retval = FALSE;
    }
    /* find the points where the intersection(s) happen */
    else
    {
        retval = TRUE;

        if (underRad == 0)
        {
            minT = maxT = -b / (2.0F * a);
            collPoint->x = obj->XPos() + obj->XDelta() * minT;
            collPoint->y = obj->YPos() + obj->YDelta() * minT;
            collPoint->z = obj->ZPos() + obj->ZDelta() * minT;
        }
        else
        {
            minT = (-b - (float)sqrt(underRad)) / (2.0F * a);
            maxT = (-b + (float)sqrt(underRad)) / (2.0F * a);

            if (minT < 0.0F)
            {
                minT = maxT;
            }

            if (minT < 0.0F)
            {
                retval = FALSE;
            }
            else
            {
                collPoint->x = obj->XPos() + obj->XDelta() * minT;
                collPoint->y = obj->YPos() + obj->YDelta() * minT;
                collPoint->z = obj->ZPos() + obj->ZDelta() * minT;
            }
        }
    }

    return (retval);
}

float FindMinDistance(vector* a, vector* aDot, vector* b, vector* bDot)
{
    float rangeSquared, tmp, distance;
    int i;
#define FRAMES_PER_TEST   16
#define FRACTION_PER_TEST  (float)(1.0 / FRAMES_PER_TEST)

    rangeSquared = (a->x - b->x) * (a->x - b->x) +
                   (a->y - b->y) * (a->y - b->y) +
                   (a->z - b->z) * (a->z - b->z);

    aDot->x *= FRACTION_PER_TEST;
    aDot->y *= FRACTION_PER_TEST;
    aDot->z *= FRACTION_PER_TEST;
    bDot->x *= FRACTION_PER_TEST;
    bDot->y *= FRACTION_PER_TEST;
    bDot->z *= FRACTION_PER_TEST;

    for (i = 0; i < FRAMES_PER_TEST; i++)
    {
        a->x -= aDot->x;
        a->y -= aDot->y;
        a->z -= aDot->z;

        b->x -= bDot->x;
        b->y -= bDot->y;
        b->z -= bDot->z;

        tmp = (a->x - b->x) * (a->x - b->x) +
              (a->y - b->y) * (a->y - b->y) +
              (a->z - b->z) * (a->z - b->z);

        if (tmp > rangeSquared)
            break;

        rangeSquared = tmp;
    }

    distance = (float)sqrt(rangeSquared);

    return (distance);
}

void Trigenometry(SimMoverClass *platform)
{
    float t1, t2;
    float alpharad, betarad;
    float gmma, sigma, mu;
    mlTrig trig;

    alpharad = platform->GetAlpha() * DTR;
    betarad  = platform->GetBeta() * DTR;

    mlSinCos(&trig, platform->Yaw());
    platform->platformAngles.cospsi = trig.cos;
    platform->platformAngles.sinpsi = trig.sin;

    mlSinCos(&trig, platform->Roll());
    platform->platformAngles.cosphi = trig.cos;
    platform->platformAngles.sinphi = trig.sin;

    mlSinCos(&trig, platform->Pitch());
    platform->platformAngles.costhe = trig.cos;
    platform->platformAngles.sinthe = trig.sin;

    mlSinCos(&trig, alpharad);
    platform->platformAngles.cosalp = trig.cos;
    platform->platformAngles.sinalp = trig.sin;

    mlSinCos(&trig, betarad);
    platform->platformAngles.cosbet = trig.cos;
    platform->platformAngles.sinbet = trig.sin;

    platform->platformAngles.tanbet = (float)tan(betarad);

    /*-----------------------------*/
    /* velocity vector orientation */
    /*-----------------------------*/

    /*-------*/
    /* gamma */
    /*-------*/
    platform->platformAngles.singam = (platform->platformAngles.sinthe *
                                       platform->platformAngles.cosalp - platform->platformAngles.costhe *
                                       platform->platformAngles.cosphi * platform->platformAngles.sinalp) *
                                      platform->platformAngles.cosbet - platform->platformAngles.costhe *
                                      platform->platformAngles.sinphi * platform->platformAngles.sinbet;

    platform->platformAngles.cosgam = (float)sqrt(1.0f -
                                      platform->platformAngles.singam * platform->platformAngles.singam);


    gmma = (float)atan2(platform->platformAngles.singam, platform->platformAngles.cosgam);

    /*----*/
    /* mu */
    /*----*/
    t1 = platform->platformAngles.costhe * platform->platformAngles.sinphi *
         platform->platformAngles.cosbet + (platform->platformAngles.sinthe *
                                            platform->platformAngles.cosalp - platform->platformAngles.costhe *
                                            platform->platformAngles.cosphi * platform->platformAngles.sinalp) *
         platform->platformAngles.sinbet;

    t2 = platform->platformAngles.costhe * platform->platformAngles.cosphi *
         platform->platformAngles.cosalp + platform->platformAngles.sinthe *
         platform->platformAngles.sinalp;


    mu = (float)(1.0 / sqrt(t1 * t1 + t2 * t2));

    platform->platformAngles.sinmu = t1;
    platform->platformAngles.cosmu = t2;


    /*-------*/
    /* sigma */
    /*-------*/
    t1 = (-platform->platformAngles.sinphi *
          platform->platformAngles.sinalp * platform->platformAngles.cosbet +
          platform->platformAngles.cosphi * platform->platformAngles.sinbet) *
         platform->platformAngles.cospsi + ((platform->platformAngles.costhe *
                                            platform->platformAngles.cosalp + platform->platformAngles.sinthe *
                                            platform->platformAngles.cosphi * platform->platformAngles.sinalp) *
                                            platform->platformAngles.cosbet + platform->platformAngles.sinthe *
                                            platform->platformAngles.sinphi * platform->platformAngles.sinbet) *
         platform->platformAngles.sinpsi;

    t2 = ((platform->platformAngles.costhe *
           platform->platformAngles.cosalp + platform->platformAngles.sinthe *
           platform->platformAngles.cosphi * platform->platformAngles.sinalp) *
          platform->platformAngles.cosbet + platform->platformAngles.sinthe *
          platform->platformAngles.sinphi * platform->platformAngles.sinbet) *
         platform->platformAngles.cospsi + (platform->platformAngles.sinphi *
                                            platform->platformAngles.sinalp * platform->platformAngles.cosbet -
                                            platform->platformAngles.cosphi * platform->platformAngles.sinbet) *
         platform->platformAngles.sinpsi;


    sigma  = (float)(1.0 / sqrt(t1 * t1 + t2 * t2));

    platform->platformAngles.sinsig = t1;
    platform->platformAngles.cossig = t2;;

}

void CalcTransformMatrix(SimBaseClass* theObject)
{
    // KCK: I changed this to use the math lib's mlTrig function. Hope I didn't fuck it up.
    mlTrig tha, phi, psi;
    //since we calculate them here anyways, and they are so useful to have,
    //I go ahead and store the sine and cosine of all the angles (DSP)

    mlSinCos(&tha, theObject->Pitch());
    mlSinCos(&phi, theObject->Roll());
    mlSinCos(&psi, theObject->Yaw());

    theObject->dmx[0][0] =  psi.cos * tha.cos;
    theObject->dmx[0][1] =  psi.sin * tha.cos;
    theObject->dmx[0][2] = -tha.sin;

    theObject->dmx[1][0] = -psi.sin * phi.cos + psi.cos * tha.sin * phi.sin;
    theObject->dmx[1][1] =  psi.cos * phi.cos + psi.sin * tha.sin * phi.sin;
    theObject->dmx[1][2] =  tha.cos * phi.sin;

    theObject->dmx[2][0] =  psi.sin * phi.sin + psi.cos * tha.sin * phi.cos;
    theObject->dmx[2][1] = -psi.cos * phi.sin + psi.sin * tha.sin * phi.cos;
    theObject->dmx[2][2] =  tha.cos * phi.cos;

    theObject->platformAngles.cospsi = psi.cos;
    theObject->platformAngles.sinpsi = psi.sin;

    theObject->platformAngles.costhe = tha.cos;
    theObject->platformAngles.sinthe = tha.sin;

    theObject->platformAngles.cosphi = phi.cos;
    theObject->platformAngles.sinphi = phi.sin;

    /*
       costha = (float)cos (theObject->Pitch());
       sintha = (float)sin (theObject->Pitch());
       cosphi = (float)cos (theObject->Roll());
       sinphi = (float)sin (theObject->Roll());
       cospsi = (float)cos (theObject->Yaw());
       sinpsi = (float)sin (theObject->Yaw());

       theObject->dmx[0][0] = cospsi*costha;
       theObject->dmx[0][1] = sinpsi*costha;
       theObject->dmx[0][2] = -sintha;

       theObject->dmx[1][0] = -sinpsi*cosphi + cospsi*sintha*sinphi;
       theObject->dmx[1][1] = cospsi*cosphi + sinpsi*sintha*sinphi;
       theObject->dmx[1][2] = costha*sinphi;

       theObject->dmx[2][0] = sinpsi*sinphi + cospsi*sintha*cosphi;
       theObject->dmx[2][1] = -cospsi*sinphi + sinpsi*sintha*cosphi;
       theObject->dmx[2][2] = costha*cosphi;
    */
}
