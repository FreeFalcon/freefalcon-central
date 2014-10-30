#include "stdhdr.h"
#include <float.h>
#include "lantirn.h"
#include "Graphics/Include/renderir.h"
#include "Graphics/Include/RViewPnt.h"
#include "otwdrive.h"
#include "simdrive.h"
#include "aircrft.h"
#include "airframe.h"
#include "sms.h"

#include "campbase.h"
#include "camplist.h"
#include "../../graphics/include/drawbsp.h"
#include "FalcLib/include/dispopts.h"

extern bool g_bTFRFixes;

LantirnClass *theLantirn;

LantirnClass::LantirnClass()  : DrawableClass()
{
    display = NULL;
    m_flags = AVAILABLE;
    m_tfr_alt = 1000;
    m_tfr_ride = TFR_SOFT;
    m_tfrmode = TFR_STBY;
    scanpos = 0.0f;
    scandir = 1.0f;
    scanrate = 60.0F * DTR;
    gdist = holdheight = turnradius = 0.0f;
    evasize = 0;
    pitch = 0.0f;
    m_fscale = 1.10f; // don't ask - they appear to work.
    m_dpitch = -0.0872665f;

    PID_MX = 0.0F;
    PID_lastErr = 0.0F;
    PID_error = 0.0F;
    PID_Output = 0.0F;
    MaxG = 0.0F;
    MinG = 0.0F;
    gAlt = 0.0F;
    lookingAngle = 0.0F;
    gDist2 = 0.0F;
    gammaCorr = 0.0F;
    gamma = 0.0F;
    min_Radius = 0.0F;
    min_safe_dist = 0.0F;
    SpeedUp = FALSE;
    roll = 0.0F;
    featureDistance = 0.0F;
    featureDistance2  = 0.0F;
    featureDistance3  = 0.0F;
    featureHeight = 0.0F;
    featureHeight2 = 0.0F;
    featureHeight3 = 0.0F;
    featureAngle = 0.0F;
    featureAngle2 = 0.0F;
    featureAngle3 = 0.0F;
}

LantirnClass::~LantirnClass()
{
    DisplayExit();
}

void LantirnClass::DisplayInit(ImageBuffer* image)
{
    RenderIR *irrend = (RenderIR *) privateDisplay;

    if (irrend and irrend->GetImageBuffer() == image)
        return;

    DisplayExit();
    irrend = new RenderIR;
    privateDisplay = irrend;
    irrend->Setup(image, OTWDriver.GetViewpoint());
    irrend->SetColor(0xff00ff00);
    irrend->SetFOV(10.0F * DTR);
}


void LantirnClass::Display(VirtualDisplay *newDisplay)
{
    display = newDisplay;
    DrawTerrain();
}


void LantirnClass::DrawTerrain()
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    Tpoint cameraPos;
    GetCameraPos(&cameraPos);

    ((RenderIR*)display)->StartDraw();
#if 0
    display->SetColor(0x03000000);
    display->Tri(-1.0F, -1.0F, -1.0F, 1.0F, 1.0F, 1.0F);
    display->Tri(-1.0F, -1.0F, 1.0F, -1.0F, 1.0F, 1.0F);
    display->SetColor(0xff00ff00);
#endif
    Trotation viewRotation;
    mlTrig tha, psi, phi;
    mlSinCos(&tha, playerAC->Pitch() + m_dpitch);
    mlSinCos(&psi, playerAC->Yaw());
    mlSinCos(&phi, playerAC->Roll());


    viewRotation.M11 = psi.cos * tha.cos;
    viewRotation.M21 = psi.sin * tha.cos;
    viewRotation.M31 = -tha.sin;

    viewRotation.M12 = -psi.sin * phi.cos + psi.cos * tha.sin * phi.sin;
    viewRotation.M22 = psi.cos * phi.cos + psi.sin * tha.sin * phi.sin;
    viewRotation.M32 = tha.cos * phi.sin;

    viewRotation.M13 = psi.sin * phi.sin + psi.cos * tha.sin * phi.cos;
    viewRotation.M23 = -psi.cos * phi.sin + psi.sin * tha.sin * phi.cos;
    viewRotation.M33 = tha.cos * phi.cos;

    Tpoint p;
    Trotation *r = &viewRotation;
    p.x = cameraPos.x;
    p.y = cameraPos.y;
    p.z = cameraPos.z;
    cameraPos.x = p.x * r->M11 + p.y * r->M12 + p.z * r->M13;
    cameraPos.y = p.x * r->M21 + p.y * r->M22 + p.z * r->M23;
    cameraPos.z = p.x * r->M31 + p.y * r->M32 + p.z * r->M33;

    ((RenderIR*)display)->DrawScene(&cameraPos, &viewRotation);

    //JAM 12Dec03 - ZBUFFERING OFF
    if (DisplayOptions.bZBuffering)
        ((RenderIR*)display)->context.FlushPolyLists();

    //    ((RenderIR*)display)->PostSceneCloudOcclusion();
    ((RenderIR*)display)->EndDraw();
}

void LantirnClass::SetFOV(float fov)
{
    if (privateDisplay)
        ((Render3D*)privateDisplay)->SetFOV(fov * m_fscale);
}

float LantirnClass::GetFov()
{
    if (privateDisplay)
        return ((Render3D*)privateDisplay)->GetFOV() * m_fscale;

    return 0;
}

void LantirnClass::ToggleFLIR()
{
    if (m_flags bitand FLIR_ON)
    {
        DisplayExit();
        display = NULL;
    }

    m_flags xor_eq FLIR_ON;
}

void LantirnClass::StepTFRRide()
{
    switch (m_tfr_ride)
    {
        case TFR_SOFT:
            m_tfr_ride = TFR_MED;
            break;

        case TFR_MED:
            m_tfr_ride = TFR_HARD;
            break;

        case TFR_HARD:
            m_tfr_ride = TFR_SOFT;
            break;
    }
}

void LantirnClass::StepTFRMode()
{
    switch (m_tfrmode)
    {
        case TFR_NORM:
            m_tfrmode = TFR_LP1;
            break;

        case TFR_LP1:
            m_tfrmode = TFR_STBY;
            break;

        case TFR_STBY:
            m_tfrmode = TFR_WX;
            break;

        case TFR_WX:
            m_tfrmode = TFR_ECCM;
            break;

        case TFR_ECCM:
        default:
            m_tfrmode = TFR_NORM;
            break;
    }
}

void LantirnClass::GetCameraPos(Tpoint *pos)
{
    //MI externalized
#if 0
    pos->x = Origin.x;
    pos->y = Origin.y;
    pos->z = Origin.z;

    // JB 010325 This places the camera directly below the aircraft.  The
    // aircraft will itself be seen in the camera when pointed directly up.
    //The correct fix would be to project a point from the Origin based on
    //the aircraft's yaw, pitch, and size.
    pos->z += 10.0f;
#else
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (playerAC)
    {
        pos->x = playerAC->af->GetLantirnCameraX() * OTWDriver.Scale(); //external variables in a/c .dat file
        pos->y = playerAC->af->GetLantirnCameraY() * OTWDriver.Scale(); //that describe camera position for each
        pos->z = playerAC->af->GetLantirnCameraZ() * OTWDriver.Scale(); //aircraft that has LANTIRN
    }
    else
    {
        pos->x = Origin.x * OTWDriver.Scale();
        pos->y = Origin.y * OTWDriver.Scale();
        pos->z = Origin.z * OTWDriver.Scale();
        pos->z += 10.0f;
    }

#endif
}

// JB 010325 Rewritten
void LantirnClass::Exec(AircraftClass* self)
{
    if (g_bTFRFixes)
    {
        SpeedUp = FALSE;
        gamma = self->af->gmma;

        if (m_tfrmode == TFR_STBY)
        {
            evasize = 0; //MI
            return;
        }

        MoveBeam();

        //Flash SLOW warrning if we get too slow
        if (self->GetKias() < self->af->GetTFR_Corner() * self->af->GetSlowPercent())
            SpeedUp = TRUE;

        float groundAlt = -self->af->groundZ;

        float vt = self->GetVt();
        pitch = self->Pitch();

        turnradius = vt * vt / (GRAVITY * (GetGLimit() - 1));
        //Calculate absolute minimum radius...
        min_Radius = vt * vt / (GRAVITY * 0.8F * (self->af->MaxGs() - 1));
        min_safe_dist = 1.2F * min_Radius * (float)sin(-self->af->gmma);
        //min_safe_dist = min_Radius * (1.2F - (float)cos(self->af->gmma));
        //min_safe_dist *= min_safe_dist;
        //min_safe_dist = (float) sqrt(min_safe_dist - min_Radius * min_Radius);

        holdheight = m_tfr_alt + groundAlt;
        gdist = -1.0f;

        //float angle = self->af->gmma;
        //if(self->af->gmma > 0)
        // angle /= 2.0F;
        //make sure we pass at least TFR_Clearance ft above top of the hill
        gAlt = -self->ZPos() + self->af->groundZ;

        if (gAlt > self->af->GetTFR_Clearance())
            gdist = GetGroundDistance(self, self->af->GetTFR_Clearance(), self->Yaw(), self->af->gmma);
        else
            gdist = GetGroundDistance(self, gAlt / 2.0F, self->Yaw(), self->af->gmma);

        //get all the important distances to features.
        //Measure distance to feature we're about to collide with, consider all objectives within 5 miles,
        //Feature box size is increased by a safety margin (50%).
        //Check 3 times, first at our position to see which feature our vector is pointing at.
        //Then also at TFR_Clearance below us to be sure we pass over it, and also to calculate inclination needed to overfly it.
        //And finally at current holdheight as a backup fallback to make sure we don't miss a feature under us.
        featureDistance  = FeatureCollisionPrediction(self, 0.0F, FALSE, FALSE, self->af->GetTFR_Clearance(), 5.0F, 1.5F, &featureHeight);
        featureDistance2 = FeatureCollisionPrediction(self, self->af->GetTFR_Clearance(),  TRUE,  TRUE, self->af->GetTFR_Clearance(), 5.0F, 1.5F, &featureHeight2);
        featureDistance3 = FeatureCollisionPrediction(self, gAlt - m_tfr_alt,  TRUE, FALSE, self->af->GetTFR_Clearance(), 5.0F, 1.5F, &featureHeight3);

        featureAngle = featureAngle2 = featureAngle3 = 0.0F;

        if (featureDistance  > 0)
            featureAngle  = RTD * (float)atan2(featureHeight + self->af->GetTFR_Clearance() - gAlt, featureDistance);

        if (featureDistance2 > 0)
            featureAngle2 = RTD * (float)atan2(featureHeight2 + self->af->GetTFR_Clearance() - gAlt, featureDistance2);

        if (featureDistance3 > 0)
            featureAngle3 = RTD * (float)atan2(featureHeight3 + self->af->GetTFR_Clearance() - gAlt, featureDistance3);

        //Set new holdheight value to be the greatest of feature heights
        float featureHeightOffset = 0;

        if (featureDistance  < 2.5 * min_Radius) // now would be a good time to start avoiding feature...
            featureHeightOffset = max(featureHeightOffset, featureHeight);

        if (featureDistance2 < 2.5 * min_Radius) // now would be a good time to start avoiding feature...
            featureHeightOffset = max(featureHeightOffset, featureHeight2);

        if (featureDistance3 < 2.5 * min_Radius) // now would be a good time to start avoiding feature...
            featureHeightOffset = max(featureHeightOffset, featureHeight3);

        holdheight += featureHeightOffset;

        //calculate ground inclination (gammaCorr)
        lookingAngle = RTD * (float) atan2(gAlt, self->af->GetTFR_lookAhead());
        gDist2 = GetGroundDistance(self, 0.0F, self->Yaw(), -lookingAngle * DTR);

        //compare feature distance and ground distance
        //If distance to feature is closer then ground distance use feature distance instead
        if (featureDistance > 0 and featureDistance < gdist and featureAngle > 0)
            if (featureHeight > gAlt)
                gdist = featureDistance;

        if (gDist2 > 0.0F)
            gammaCorr = RTD * (float) atan2(gAlt - gDist2 * sin(lookingAngle * DTR), gDist2 * cos(lookingAngle * DTR));
        else
            gammaCorr = 0;

        //adjust gamma correction according to feature distance
        //Adjust gammaCorr for the angle to avoid feature if we are below it.
        if ((featureDistance2 < 2.5 * min_Radius and featureDistance2 > 0 and featureAngle2 > 0) or featureAngle2 > 1.0F)
        {
            gammaCorr += featureAngle2 * 1.3F;
        }

        evasize = -1;

        if (gdist > 0)
        {
            evasize = 0; // assume we're ok...

            //use the new GetEVAFactor() function
            if (gdist < turnradius * GetEVAFactor(self, 1)) // coming up to something
                evasize = 1;

            if (gdist < turnradius * GetEVAFactor(self, 2)) // danger danger
                evasize = 2;

            if (gdist < min_Radius * 1.2) // danger danger danger
                evasize = 3;

            if (gdist < min_safe_dist) // danger danger danger
                evasize = 3;
        }
        else if (-self->ZPos() - groundAlt < 0.5 * m_tfr_alt)
            evasize = 1;
    }
    else
    {
        if (m_tfrmode == TFR_STBY)
            return;

        MoveBeam();

        float groundAlt = -self->af->groundZ;

        float vt = self->GetVt();
        pitch = self->Pitch();

        turnradius = vt * vt / (GRAVITY * (GetGLimit() - 1));

        holdheight = m_tfr_alt + groundAlt;
        gdist = -1.0f;

        int type = 0;
        gdist = GetGroundIntersection(self, self->Yaw(), pitch - self->GetAlpha() * DTR, groundAlt, type);

        evasize = 0;

        if (gdist >= 0)
        {
            float rangetopos = sqrt(gdist * gdist + m_tfr_alt * m_tfr_alt);

            if (gdist > 0 and rangetopos < turnradius and type) // danger danger
                evasize = 2;
            else if (rangetopos - m_tfr_alt * 2 > turnradius)
                evasize = 0;
            else if (rangetopos < turnradius + m_tfr_alt) // coming up to something
                evasize = 1;
        }
        else if (-self->ZPos() - groundAlt < m_tfr_alt)
            evasize = 1;
    }
}

void LantirnClass::MoveBeam()
{
    if (scanpos > 1.0f) scandir = -1.0f;

    if (scanpos < -1.0f) scandir = 1.0f;

    scanpos += scandir * scanrate * SimLibMajorFrameTime;
}

float LantirnClass::GetGLimit()
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (g_bTFRFixes)
    {
        if (playerAC and playerAC->af)
        {
            switch (m_tfr_ride)
            {
                case TFR_SOFT:
                    return playerAC->af->GetTFR_SoftG();

                default:
                case TFR_MED:
                    return playerAC->af->GetTFR_MedG();

                case TFR_HARD:
                    return playerAC->af->GetTFR_HardG();
            }
        }
        else
        {
            switch (m_tfr_ride)
            {
                case TFR_SOFT:
                    return 2.0f;

                default:
                case TFR_MED:
                    return 4.0f;

                case TFR_HARD:
                    return 6.0f;
            }
        }
    }
    else
    {
        switch (m_tfr_ride)
        {
            case TFR_SOFT:
                return 2.0f;

            default:
            case TFR_MED:
                return 4.0f;

            case TFR_HARD:
                return 6.0f;
        }
    }
}

// JB 010325 Rewritten
float LantirnClass::GetGroundIntersection(AircraftClass* self, float yaw, float pitch, float galt, int &type)
{
    type = 1;
    float dx;
    float dy;
    float dz;
    float gdist1 = -1;
    float gdist2 = -1;

    Tpoint airframepos;
    airframepos.x = self->XPos();
    airframepos.y = self->YPos();
    airframepos.z = self->ZPos();

    RViewPoint* vpp;
    vpp = OTWDriver.GetViewpoint();
    vpp->Update(&airframepos);

    Tpoint tfrviewDir;
    Tpoint point;
    mlTrig trigYaw, trigPitch;

    mlSinCos(&trigYaw,   yaw);
    mlSinCos(&trigPitch, pitch);

    tfrviewDir.x =  trigYaw.cos * trigPitch.cos;
    tfrviewDir.y =  trigYaw.sin * trigPitch.cos;
    tfrviewDir.z = -trigPitch.sin;

    if (vpp->GroundIntersection(&tfrviewDir, &point))
    {
        dx = self->XPos() - point.x;
        dy = self->YPos() - point.y;
        dz = self->ZPos() - point.z;
        gdist1 = (float)sqrt(dx * dx + dy * dy);
    }

    if (airframepos.z + m_tfr_alt > galt)
        return 0;

    if (pitch < 0)
        pitch = 0;
    else
        pitch /= 4;

    mlSinCos(&trigPitch, pitch);

    tfrviewDir.x =  trigYaw.cos * trigPitch.cos;
    tfrviewDir.y =  trigYaw.sin * trigPitch.cos;
    tfrviewDir.z = -trigPitch.sin;

    airframepos.z += m_tfr_alt;
    vpp->Update(&airframepos);
    int res = vpp->GroundIntersection(&tfrviewDir, &point);

    if ( not res)
    {
        type = 0;
        return gdist1;
    }

    dx = self->XPos() - point.x;
    dy = self->YPos() - point.y;
    dz = self->ZPos() - point.z;
    gdist2 = (float)sqrt(dx * dx + dy * dy);

    if (gdist1 >= 0 and gdist1 < gdist2)
        return gdist1;

    return gdist2;
}

float LantirnClass::GetGroundDistance(AircraftClass* self, float zOffset, float yaw, float pitch)
{
    float dx;
    float dy;
    float dz;
    float gdist = -1;

    Tpoint airframepos;
    airframepos.x = self->XPos();
    airframepos.y = self->YPos();
    airframepos.z = self->ZPos() + zOffset;

    RViewPoint* vpp;
    vpp = OTWDriver.GetViewpoint();
    vpp->Update(&airframepos);

    Tpoint tfrviewDir;
    Tpoint point;
    mlTrig trigYaw, trigPitch;

    mlSinCos(&trigYaw,   yaw);
    mlSinCos(&trigPitch, pitch);

    tfrviewDir.x =  trigYaw.cos * trigPitch.cos;
    tfrviewDir.y =  trigYaw.sin * trigPitch.cos;
    tfrviewDir.z = -trigPitch.sin;

    if (vpp->GroundIntersection(&tfrviewDir, &point))
    {
        dx = self->XPos() - point.x;
        dy = self->YPos() - point.y;
        dz = self->ZPos() - point.z;
        gdist = (float)sqrt(dx * dx + dy * dy + dz * dz);
    }

    return gdist;
}
float LantirnClass::FeatureCollisionPrediction(AircraftClass* self, float zOffset, BOOL MeasureHorizontally,
        BOOL GreatestAspect, float Clearance, float GridSizeNM,
        float boxScale, float *featureHeight)
{
    CampBaseClass* objective;
    //get all objectives within GridSizeNM miles
#ifdef VU_GRID_TREE_Y_MAJOR
    VuGridIterator gridIt(ObjProxList, self->YPos(), self->XPos(), GridSizeNM * NM_TO_FT);
#else
    VuGridIterator gridIt(ObjProxList, self->XPos(), self->YPos(), GridSizeNM * NM_TO_FT);
#endif
    SimBaseClass *foundFeature = NULL;
    SimBaseClass *testFeature;
    float radius;
    Tpoint pos, fpos, vec, p3, collide;
    BOOL firstFeature;
    float MaxDistance = 2 * GridSizeNM * NM_TO_FT;
    float ClosestDistance = 10 * GridSizeNM * NM_TO_FT;
    float move_vector;
    float Aspect = -90.0F;

    *featureHeight = 0.0F;
    // get the 1st objective and iterate through the list
    objective = (CampBaseClass*)gridIt.GetFirst();

    while (objective)
    {
        if (objective->GetComponents())
        {
            pos.x = self->XPos();
            pos.y = self->YPos();
            pos.z = self->ZPos() + zOffset;

            //check for MaxDistance in the direction of our vector
            vec.x = self->XDelta();
            vec.y = self->YDelta();
            vec.z = self->ZDelta();
            move_vector = (float)sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
            vec.x *= MaxDistance / move_vector;
            vec.y *= MaxDistance / move_vector;
            vec.z *= MaxDistance / move_vector;

            if (MeasureHorizontally)
                vec.z = 0;

            p3.x = (float)fabs(vec.x);
            p3.y = (float)fabs(vec.y);
            p3.z = (float)fabs(vec.z);

            // loop thru each element in the objective
            VuListIterator featureWalker(objective->GetComponents());
            testFeature = (SimBaseClass*) featureWalker.GetFirst();
            firstFeature = TRUE;

            while (testFeature)
            {
                if (testFeature->drawPointer)
                {
                    // get feature's radius and position
                    radius = testFeature->drawPointer->Radius();

                    if (self->drawPointer)
                        radius += self->drawPointer->Radius();

                    radius += Clearance;
                    testFeature->drawPointer->GetPosition(&fpos);

                    // test with gross level bounds of object
                    if (fabs(pos.x - fpos.x) < radius + p3.x and 
                        fabs(pos.y - fpos.y) < radius + p3.y and 
                        fabs(pos.z - fpos.z) < radius + p3.z)
                    {
                        //Check for tall objects when doing horizontal check
                        float sizeX, sizeY, sizeZ;
                        sizeX = ((DrawableBSP*)(testFeature->drawPointer))->instance.BoxFront();
                        sizeX -= ((DrawableBSP*)(testFeature->drawPointer))->instance.BoxBack();
                        sizeX = (float)(fabs)(sizeX);
                        sizeY = ((DrawableBSP*)(testFeature->drawPointer))->instance.BoxRight();
                        sizeY -= ((DrawableBSP*)(testFeature->drawPointer))->instance.BoxLeft();
                        sizeY = (float)(fabs)(sizeY);
                        sizeZ = ((DrawableBSP*)(testFeature->drawPointer))->instance.BoxTop();
                        sizeZ -= ((DrawableBSP*)(testFeature->drawPointer))->instance.BoxBottom();
                        sizeZ = (float)(fabs)(sizeZ);
                        float groundRadius = max(sizeX, sizeY);
                        //only for horizontal checks
                        float NewBoxScale = boxScale;

                        if ((groundRadius < 2.2F * Clearance) and MeasureHorizontally)
                            NewBoxScale = boxScale * (2.2F * Clearance + groundRadius) / groundRadius;

                        //Check to see if out flight vector line intersects feature's box * boxScale (safety margin)
                        if (testFeature->drawPointer->GetRayHit(&pos, &vec, &collide, NewBoxScale))
                        {
                            collide.x = (float)fabs(collide.x - pos.x);
                            collide.y = (float)fabs(collide.y - pos.y);
                            collide.z = (float)fabs(collide.z - pos.z);
                            float Distance = (float)sqrt(collide.x * collide.x + collide.y * collide.y + collide.z * collide.z);

                            //Remember this one, either closest one, or tallest one from our point of view.
                            if ( not GreatestAspect)
                            {
                                if (Distance < ClosestDistance)
                                {
                                    ClosestDistance = Distance;
                                    *featureHeight = sizeZ;
                                }
                            }
                            else
                            {
                                float newAspect = RTD * (float)atan2(sizeZ - (-self->ZPos() + self->af->groundZ), Distance);

                                if (newAspect > Aspect and Distance < 2 * MaxDistance)
                                {
                                    Aspect = newAspect;
                                    ClosestDistance = Distance;
                                    *featureHeight = sizeZ;
                                }
                            }
                        }
                    }
                }

                testFeature = (SimBaseClass*) featureWalker.GetNext();
                firstFeature = FALSE;
            }
        }

        objective = (CampBaseClass*)gridIt.GetNext();
    }

    if (ClosestDistance < 2 * MaxDistance)
        return ClosestDistance; //yes We found a feature within range...
    else
        return -1.0F; //we ain't found sh*t...
}
float LantirnClass::GetEVAFactor(AircraftClass* self, int eva = 1)
{
    if (eva == 1)
    {
        switch (m_tfr_ride)
        {
            case TFR_SOFT:
                return self->af->GetEVA1_SoftFactor();

            case TFR_MED:
                return self->af->GetEVA1_MedFactor();

            case TFR_HARD:
                return self->af->GetEVA1_HardFactor();
        }
    }
    else if (eva == 2)
    {
        switch (m_tfr_ride)
        {
            case TFR_SOFT:
                return self->af->GetEVA2_SoftFactor();

            case TFR_MED:
                return self->af->GetEVA2_MedFactor();

            case TFR_HARD:
                return self->af->GetEVA2_HardFactor();
        }
    }

    return 1.0F;
}
