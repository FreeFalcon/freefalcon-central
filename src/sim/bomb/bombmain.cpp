#include "Graphics/Include/drawbsp.h"
#include "Graphics/Include/rviewpnt.h"
#include "Graphics/Include/terrtex.h"
#include "stdhdr.h"
#include "bomb.h"
#include "bombdata.h"
#include "falcmesg.h"
#include "otwdrive.h"
#include "initdata.h"
#include "waypoint.h"
#include "object.h"
#include "simdrive.h"
#include "fsound.h"
#include "soundfx.h"
#include "MsgInc/DamageMsg.h"
#include "MsgInc/MissileEndMsg.h"
#include "campBase.h"
#include "campweap.h"
#include "camp2sim.h"
#include "sfx.h"
#include "fakerand.h"
#include "aircrft.h"
#include "acmi/src/include/acmirec.h"
#include "classtbl.h"
#include "Feature.h"
#include "falcsess.h"
#include "persist.h"
#include "entity.h"
#include "camplist.h"
#include "weather.h"
#include "team.h"
#include "sms.h"//me123 status test. addet
#include "profiler.h" // MLR 5/21/2004 - 
#include "digi.h"

//sfr: added checks
#include "InvalidBufferException.h"

/* 2001-09-07 S.G. RP5 */
extern bool g_bRP5Comp;
//#define WEAP_LGB_3RD_GEN 0x40 moved to campwp.h and changed to 0x80

// this means that every sec, the horizontal velocity loses this much
float BombClass::dragConstant = 140.0f;

//extern bool g_bArmingDelay;//me123 MI replaced with g_bRealisticAvionics
extern bool g_bRealisticAvionics;
extern bool g_bEnableWindsAloft;
#include "fcc.h"

extern float g_fNukeStrengthFactor;
extern float g_fNukeDamageMod;
extern float g_fNukeDamageRadius;

extern float g_fJDAMLift; //Wombat778 3-12-04
extern float g_fAIJSOWmaxRange; // Cobra

void CalcTransformMatrix(SimBaseClass* theObject);


#ifdef USE_SH_POOLS
MEM_POOL BombClass::pool;
#endif

BombClass::BombClass(VU_BYTE** stream, long *rem) : SimWeaponClass(stream, rem)
{
    BombType bt;
    memcpychk(&bt, stream, sizeof(bt), rem);
    InitLocalData(bt);

    if (parent and not IsLocal())
    {
        flags or_eq FirstFrame;
        //VuReferenceEntity (parent);
        //parentReferenced = TRUE;
        SetYPR(parent->Yaw(), parent->Pitch(), parent->Roll());
    }
}

BombClass::BombClass(FILE* filePtr) : SimWeaponClass(filePtr)
{
    BombType bt;
    fread(&bt, sizeof(bt), 1, filePtr);
    InitLocalData(bt);
}

BombClass::BombClass(int type, BombType btype) : SimWeaponClass(type)
{
    InitLocalData(btype);
}

BombClass::~BombClass()
{
    CleanupLocalData();
}

int BombClass::SaveSize()
{
    return SimWeaponClass::SaveSize() + sizeof(BombType);
}

int BombClass::Save(VU_BYTE **stream)
{
    int saveSize = SimWeaponClass::Save(stream);

    if (flags bitand IsChaff)
        bombType = Chaff;
    else if (flags bitand IsFlare)
        bombType = Flare;

    memcpy(*stream, &bombType, sizeof(int));
    *stream += sizeof(int);
    return (saveSize + sizeof(int));
}

int BombClass::Save(FILE *file)
{
    int saveSize = SimWeaponClass::Save(file);

    if (flags bitand IsChaff)
        bombType = Chaff;
    else if (flags bitand IsFlare)
        bombType = Flare;

    fwrite(&bombType, sizeof(int), 1, file);
    return (saveSize + sizeof(int));
}

void BombClass::InitData()
{
    SimWeaponClass::InitData();
    InitLocalData(static_cast<BombType>(bombType));
}

void BombClass::InitLocalData(BombType btype)
{
    bombType = btype;
    lauTimer = 0; // MLR 3/5/2004 -
    lauWeaponId = 0;
    burstHeight = 0.0F;
    detonateHeight = 0.0F;
    timeOfDeath = 0;
    specialData.flags or_eq MOTION_BMB_AI;
    flags = 0;
    dragCoeff = 0.0f;

    desDxPrev = 0.0f;
    desDyPrev = 0.0f;
    desDzPrev = 0.0f;
}

void BombClass::CleanupLocalData()
{
    // empty
}

void BombClass::CleanupData()
{
    CleanupLocalData();
    SimWeaponClass::CleanupData();
}

void BombClass::Init(SimInitDataClass* initData)
{
    if (initData == NULL)
    {
        Init();
    }
}

void BombClass::Init()
{
    DrawableObject* tmpObject;

    tmpObject = drawPointer;
    drawPointer = NULL;
    SimWeaponClass::Init();

    if (tmpObject)
    {
        delete drawPointer;
        drawPointer = tmpObject;
    }

    Falcon4EntityClassType* classPtr;
    WeaponClassDataType* wc;
    SimWeaponDataType* wpnDefinition;
    int dataIdx;

    // MLR 2003-11-10 cut bitand paste from MissleClass
    auxData = NULL;
    classPtr = (Falcon4EntityClassType*)EntityType();
    wc = (WeaponClassDataType*)classPtr->dataPtr;
    wpnDefinition = &SimWeaponDataTable[classPtr->vehicleDataIndex];
    dataIdx = wpnDefinition->dataIdx;
    ReadInput(dataIdx);

    LauInit(); // MLR 3/5/2004 -


    // Am I an LGB
    if (EntityType()->classInfo_[VU_STYPE] == STYPE_BOMB_GUIDED)
        flags or_eq IsLGB;

    //Wombat778 3-09-04 Is this a GPS bomb?
    if (EntityType()->classInfo_[VU_STYPE] == STYPE_BOMB_GPS)
        flags or_eq IsGPS;

    // Cobra - GPS/JSOW
    if (EntityType()->classInfo_[VU_STYPE] == STYPE_BOMB_JSOW)
        flags or_eq (IsGPS bitor IsJSOW);

}

int BombClass::Wake()
{
    int retval = 0;

    // KCK: Sets up this object to become sim aware
    if (IsAwake())
    {
        return retval;
    }

    InitTrail();
    ExtraGraphics();
    SimWeaponClass::Wake();

    // Change the last update time to force and exec next frame;
    // sfr: need to check if this has an effect
    //AlignTimeSubtract (1);
    return retval;
}

int BombClass::Sleep()
{
    return SimWeaponClass::Sleep();
}

void BombClass::Start(vector* pos, vector* rate, float cD, SimObjectType *targetPtr)
{
    Falcon4EntityClassType *classPtr;
    WeaponClassDataType *wc;

    // 2002-02-26 ADDED BY S.G. If we passed a targetPtr,
    // keep note of it in case the AI target is aggregated.
    // That way we can send a Damage message to the 2D engine to take care of the target
    if (targetPtr)
        SetTarget(targetPtr);

    // END OF ADDED SECTION

    if (parent)
    {
        flags or_eq FirstFrame;
        //VuReferenceEntity (parent);
        //parentReferenced = TRUE;
        SetYPR(parent->Yaw(), parent->Pitch(), parent->Roll());
    }
    else
    {
        SetYPR((float)atan2(rate->y, rate->x), 0.0F, 0.0F);
    }

    x = pos->x;
    y = pos->y;
    z = pos->z;

    dragCoeff = cD;

    // edg hack.  drag coeff of 1.0f we assume to be a durandal
    if (cD >= 1.0f)
        flags or_eq IsDurandal;

    SetPosition(x, y, z);
    CalcTransformMatrix(this);

    SetDelta(rate->x, rate->y, rate->z);
    SetYPRDelta(0.0F, 0.0F, 0.0F);

    // sound effect
    //SoundPos.Sfx( SFX_BOMBDROP, 0, 1, 0); // MLR 6/4/2004 - This won't work here anymore

    // if the bomb is altitude detonated (AGL), check to make sure we're
    // 2.0 sec above the detonation alt when dropped.  If not, we don't fuse and
    // do a ground impact....

    // hack for testing
    // burstHeight *= 1.2f;

    if (burstHeight > 0.0f)
    {
        // get entity and weapon info
        classPtr = &Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE];
        wc = (WeaponClassDataType *)classPtr->dataPtr;

        // if we're not a cluster type, we should have no burst height
        if ( not (wc->Flags bitand WEAP_CLUSTER))
        {
            burstHeight = 0.0f;
        }
    }

    hitObj = NULL;

    // for alt fuse
    timeOfDeath = SimLibElapsedTime;
}

void BombClass::ExtraGraphics(void)
{
}

int BombClass::Exec(void)
{
    // FalconDamageMessage* message;
    float terrainHeight;
    float delta;
    ACMIGenPositionRecord genPos;
    FalconMissileEndMessage* endMessage;
    float radical, tFall, desDx, desDy;
    float deltaX, deltaY, deltaZ, vt;
    float rx, ry, rz, range;
    mlTrig trigYaw, trigPitch;
    float bheight;
    float grav;
    float armingdelay = 0.0f;//me123 done this way to awoid a crash when player dies in matchplay

    // Debub ==========================
    static FILE *fp = NULL;
    //if ( not fp)
    //fp = fopen("g:\\JSOWtrgtFinal.txt", "w");
    //=================================


    SoundPos.UpdatePos(this);

    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    if (playerAC and playerAC->IsSetFlag(MOTION_OWNSHIP))
    {
        armingdelay = playerAC->Sms->armingdelay;//me123
    }


    if (IsDead() or (flags bitand FirstFrame))
    {
        flags and_eq compl FirstFrame;
        return TRUE;
    }

    UpdateTrail();

    // ACMI Output
    if (gACMIRec.IsRecording() and (SimLibFrameCount bitand 0x07) == 0)
    {
        genPos.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
        genPos.data.type = Type();
        genPos.data.uniqueID = ACMIIDTable->Add(Id(), NULL, TeamInfo[GetTeam()]->GetColor()); //.num_;
        genPos.data.x = XPos();
        genPos.data.y = YPos();
        genPos.data.z = ZPos();
        genPos.data.roll = Roll();
        genPos.data.pitch = Pitch();
        genPos.data.yaw = Yaw();

        if (flags bitand IsFlare)
            gACMIRec.FlarePositionRecord((ACMIFlarePositionRecord *)&genPos);
        else if (flags bitand IsChaff)
            gACMIRec.ChaffPositionRecord((ACMIChaffPositionRecord *)&genPos);
        else
            gACMIRec.GenPositionRecord(&genPos);
    }

    if ( not IsLocal())
    {
        return FALSE;
    }

    if (IsExploding())
    {
        DoExplosion();
        return TRUE;
    }

    else if (SimDriver.MotionOn())
    {
        SpecialGraphics();

        x = XPos() + XDelta() * SimLibMajorFrameTime;
        y = YPos() + YDelta() * SimLibMajorFrameTime;
        z = ZPos() + ZDelta() * SimLibMajorFrameTime;

        float dx, dy, ratx, raty, vdrag, horiz;
        //dumb bomb algorithm
        /*y = Tan(theta) * x - (g * x^2)/(2(*v*cos(theta)^2)*/

        horiz = (float)sqrt(XDelta() * XDelta() + YDelta() * YDelta() + 0.1f);
        ratx = (float)fabs(XDelta()) / horiz;
        raty = (float)fabs(YDelta()) / horiz;

        // for the 1st sec don't apply drag
        grav = GRAVITY;

        if (SimLibElapsedTime - timeOfDeath > 1 * SEC_TO_MSEC)
        {
            vdrag = dragCoeff * dragConstant * SimLibMajorFrameTime;

            // super high drag will mean also reduced gravity due to
            // air friction....
            if (dragCoeff >= 1.0f)
                grav = GRAVITY * 0.65F;
        }
        else
        {
            vdrag = 0.0f;
        }

        if (fabs(XDelta()) < vdrag * ratx)
        {
            dx = 0.0f;
        }
        else
        {
            if (XDelta() > 0.0f)
                dx = XDelta() - vdrag * ratx;
            else
                dx = XDelta() + vdrag * ratx;
        }

        if (fabs(YDelta()) < vdrag * raty)
        {
            dy = 0.0f;
        }
        else
        {
            if (YDelta() > 0.0f)
                dy = YDelta() - vdrag * raty;
            else
                dy = YDelta() + vdrag * raty;
        }

        //me123 lets add the wind change effect to the bomb fall
        //MI.... check for our variable...
        if (g_bEnableWindsAloft)
        {
            mlTrig trigWind;
            float wind;
            Tpoint pos;
            pos.x = x;
            pos.y = y;
            pos.z = z;

            // current wind
            mlSinCos(&trigWind, ((WeatherClass*)realWeather)->WindHeadingAt(&pos));
            wind = ((WeatherClass*)realWeather)->WindSpeedInFeetPerSecond(&pos);
            float winddx = trigWind.cos * wind;
            float winddy = trigWind.sin * wind;

            //the wind last time we checked
            pos.z = z - ZDelta() * SimLibMajorFrameTime;
            mlSinCos(&trigWind, ((WeatherClass*)realWeather)->WindHeadingAt(&pos));
            wind = ((WeatherClass*)realWeather)->WindSpeedInFeetPerSecond(&pos);
            float lastwinddx = trigWind.cos * wind;
            float lastwinddy = trigWind.sin * wind;

            //factor in the change
            dx += (winddx - lastwinddx) * 0.9f; //not all the wind since no inertie is factored in
            dy += (winddy - lastwinddy) * 0.9f; //not all the wind since no inertie is factored in
        }


        // SetDelta (XDelta(), YDelta(), ZDelta() + GRAVITY * SimLibMajorFrameTime);

        // RV - Biker - Give some extra high gravity for the first ms
        //SetDelta (dx, dy, ZDelta() + grav * SimLibMajorFrameTime);
        if ((SimLibElapsedTime - timeOfDeath) <= (0.25f * SEC_TO_MSEC))
            SetDelta(XDelta(), YDelta(), ZDelta() + GRAVITY * SimLibMajorFrameTime * 2.0f);
        else if ((SimLibElapsedTime - timeOfDeath) <= (2.0f * SEC_TO_MSEC))
            SetDelta(XDelta(), YDelta(), ZDelta() + GRAVITY * SimLibMajorFrameTime);
        else
            SetDelta(dx, dy, ZDelta() + grav * SimLibMajorFrameTime);

        vt = (float)sqrt(XDelta() * XDelta() + YDelta() * YDelta() + ZDelta() * ZDelta());
        SetYPR((float)atan2(YDelta(), XDelta()), -(float)asin(ZDelta() / vt), Roll());

        // NOTE:  Yaw never changes, so we could avoid much of this...
        mlSinCos(&trigYaw, Yaw());
        mlSinCos(&trigPitch, Pitch());

        dmx[0][0] = trigYaw.cos * trigPitch.cos;
        dmx[0][1] = trigYaw.sin * trigPitch.cos;
        dmx[0][2] = -trigPitch.sin;

        dmx[1][0] = -trigYaw.sin;
        dmx[1][1] = trigYaw.cos;
        dmx[1][2] = 0.0F;

        dmx[2][0] = trigYaw.cos * trigPitch.sin;
        dmx[2][1] = trigYaw.sin * trigPitch.sin;
        dmx[2][2] = trigPitch.cos;

        // special case durandal -- when fired remove chute
        if ((flags bitand IsDurandal) and 
            (flags bitand FireDurandal) and 
            drawPointer and 
            ((DrawableBSP*)drawPointer)->GetNumSwitches() > 0)
        {
            ((DrawableBSP *)drawPointer)->SetSwitchMask(0, 0);
        }

        // special case durandal.  If x and y vel reaches 0 we fire it
        // by starting the special effect
        if ((flags bitand IsDurandal) and 
 not (flags bitand FireDurandal) and 
            dx == 0.0f and 
            dy == 0.0f)
        {
            flags or_eq FireDurandal;

            // accelerate towards ground
            SetDelta(dx, dy, ZDelta() + 500.0f);
            endMessage = new FalconMissileEndMessage(Id(), FalconLocalGame);
            endMessage->RequestReliableTransmit();
            endMessage->RequestOutOfBandTransmit();
            endMessage->dataBlock.fEntityID  = parent->Id();
            endMessage->dataBlock.fCampID    = parent->GetCampID();
            endMessage->dataBlock.fSide      = parent->GetCountry();
            endMessage->dataBlock.fPilotID   = (uchar)shooterPilotSlot;
            endMessage->dataBlock.fIndex     = parent->Type();
            endMessage->dataBlock.dEntityID  = FalconNullId;
            endMessage->dataBlock.dCampID    = 0;
            endMessage->dataBlock.dSide      = 0;
            endMessage->dataBlock.dPilotID   = 0;
            endMessage->dataBlock.dIndex     = 0;
            endMessage->dataBlock.fWeaponUID = Id();
            endMessage->dataBlock.wIndex   = Type();
            endMessage->dataBlock.x    = XPos() + XDelta() * SimLibMajorFrameTime * 2.0f;
            endMessage->dataBlock.y    = YPos() + YDelta() * SimLibMajorFrameTime * 2.0f;
            endMessage->dataBlock.z    = ZPos() + ZDelta() * SimLibMajorFrameTime * 2.0f;
            endMessage->dataBlock.xDelta    = XDelta();
            endMessage->dataBlock.yDelta    = YDelta();
            endMessage->dataBlock.zDelta    = ZDelta();
            endMessage->dataBlock.groundType    = -1;
            endMessage->dataBlock.endCode    = FalconMissileEndMessage::BombImpact;

            endMessage->SetParticleEffectName(auxData->psBombImpact); // MLR 6/26/2004 -

            FalconSendMessage(endMessage, FALSE);
        }

        /////////////  - RED - *************
        //MI for refined LGB stuff
        // 2002-01-06 MODIFIED BY S.G.
        // In the odd chance that there is no parent, neither this or the
        // realistic section would be ran. If no parent, run this
        // There is no danger of a CTD if parent is NULL because the
        // OR will have it enter the if statement without running the 'IsPlayer'.
        //   if( not g_bRealisticAvionics or ( parent and not ((AircraftClass *)parent)->IsPlayer()))
        // Cobra - Forcing all non-Player (AI) into this section causes their bombs
        // to not be guided, thus randon hit pattern
        //   if( not g_bRealisticAvionics or not parent)
        /////////////////////////////////////////////////////

        // RED -  enough enter when it's not a guided Bomb or LGB for AI
        // ( the targeting sysem would CTD if AI managed by player code,
        // As the PlayerEntity is not the one to use )
        if (
 not g_bRealisticAvionics or not parent or not (flags bitand GUIDED_BOMB)
            or (((( not ((AircraftClass *)parent.get())->IsPlayer())
                  or (((AircraftClass *)parent.get())->IsPlayer())
                 and ((AircraftClass *)parent.get())->AutopilotType() == AircraftClass::CombatAP))
               and (flags bitand IsLGB))
        )
        {
            // RV - Biker - Add 2.0 sec delay for guidance
            if (flags bitand IsLGB and (SimLibElapsedTime - timeOfDeath) > (2.0f * SEC_TO_MSEC))
            {
                if (targetPtr)
                {
                    // JPO - target might be higher than us...
                    if (targetPtr->BaseData()->ZPos() <= ZPos())
                    {
                        radical = 0;
                    }
                    else
                    {
                        radical = (float)sqrt(ZDelta() * ZDelta() + 2.0F * GRAVITY * (targetPtr->BaseData()->ZPos() - ZPos()));
                    }

                    tFall = -ZDelta() - radical;

                    if (tFall < 0.0F)
                        tFall = -ZDelta() + radical;

                    tFall /= GRAVITY;

                    deltaX = targetPtr->BaseData()->XPos() - XPos();
                    deltaY = targetPtr->BaseData()->YPos() - YPos();
                    deltaZ = (float)fabs(targetPtr->BaseData()->ZPos() - ZPos());

                    rx    = dmx[0][0] * deltaX + dmx[0][1] * deltaY + dmx[0][2] * deltaZ;
                    ry    = dmx[1][0] * deltaX + dmx[1][1] * deltaY + dmx[1][2] * deltaZ;
                    rz    = dmx[2][0] * deltaX + dmx[2][1] * deltaY + dmx[2][2] * deltaZ;
                    range = (float)sqrt(rx * rx + ry * ry + rz * rz);

                    // 45 degree limit on the seeker
                    // 2001-04-17 MODIFIED BY S.G. AGAIN, THEY MIXE UP DEGREES AND RADIAN BUT THIS TIME, THE OTHER WAY AROUND...
                    //            PLUS THEY FORGOT THAT atan2 RETURNS A SIGNED VALUE
                    //            END THEN, range-rx * rx is range - rx*rx which is ALWAYS NEGATIVE. CAN'T TAKE A SQUARE ROOT OF A NEGATIVE NUMBER. I'VE CODED WHAT I *THINK* THEY WERE TRYING TO DO...
                    //   if (atan2(sqrt(range-rx * rx),rx) < 45.0F * RTD)
                    //JAM 17Apr04 - This is what they were trying to do buddy :)
                    if (Abs(acosf(rx / range)) <= 45.f * DTR)
                        //   if (fabs(atan2(sqrt(range*range - rx*rx),rx)) < 45.0F * DTR)
                    {
                        desDx = (deltaX) / tFall;
                        desDy = (deltaY) / tFall;
                        // 2001-04-17 ADDED BY S.G. WE'LL KEEP OUR LAST desDx and desDy IN THE UNUSED tgtX and tgtY BombClass VARIABLE (RENAMED desDxPrev AND desDyPrev) TO IN CASE WE LOOSE LOCK LATER
                        desDxPrev = desDx;
                        desDyPrev = desDy;
                        // END OF ADDED SECTION
                        SetDelta(0.8F * XDelta() + 0.2F * desDx, 0.8F * YDelta() + 0.2F * desDy, ZDelta());
                    }
                    // 2002-01-05 ADDED BY S.G. Similarly to below. if the lgb cannot see the laser, it can't guide (forgot to do this).
                    else if (g_bRP5Comp)
                    {
                        // 2001-04-17 ADDED BY S.G. WE'LL KEEP GOING WHERE WE WERE GOING...
                        Falcon4EntityClassType *classPtr = &Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE];
                        WeaponClassDataType *wc = (WeaponClassDataType *)classPtr->dataPtr;

                        // If a 3rg gen LGB, fins are more precised, even when no longer lased...
                        //#define WEAP_LGB_3RD_GEN 0x40 moved to campwp.h and changed to 0x80
                        if (wc->Flags bitand WEAP_LGB_3RD_GEN)
                            SetDelta(0.8F * XDelta() + 0.2f * desDxPrev, 0.8f * YDelta() + 0.2f * desDyPrev, ZDelta());
                        else // 2001-10-19 MODIFIED BY S.G. IT'S * 1.05f AND NOT * 2.0f
                            SetDelta((0.8F * XDelta() + 0.2f * desDxPrev) * 1.05f, (0.8f * YDelta() + 0.2f * desDyPrev) * 1.05f, ZDelta());
                    }

                    if ( not ((SimBaseClass*)(targetPtr->BaseData()))->IsSetFlag(IS_LASED))
                    {
                        targetPtr->Release();
                        targetPtr = NULL;
                    }
                }
                // 2001-04-17 ADDED BY S.G. WE'LL KEEP GOING WHERE WE WERE GOING...
                else if (g_bRP5Comp)
                {
                    // 2001-04-17 ADDED BY S.G. WE'LL KEEP GOING WHERE WE WERE GOING...
                    Falcon4EntityClassType *classPtr = &Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE];
                    WeaponClassDataType *wc = (WeaponClassDataType *)classPtr->dataPtr;

                    // If a 3rg gen LGB, fins are more precised, even when no longer lased...
                    //#define WEAP_LGB_3RD_GEN 0x40 moved to campwp.h and changed to 0x80
                    if (wc->Flags bitand WEAP_LGB_3RD_GEN)
                        SetDelta(0.8F * XDelta() + 0.2f * desDxPrev, 0.8f * YDelta() + 0.2f * desDyPrev, ZDelta());
                    else // 2001-10-19 MODIFIED BY S.G. IT'S * 1.05f AND NOT * 2.0f
                        SetDelta((0.8F * XDelta() + 0.2f * desDxPrev) * 1.05f, (0.8f * YDelta() + 0.2f * desDyPrev) * 1.05f, ZDelta());
                }
            }
        }
        // RV - Biker - Add 2 sec delay for guidance
        else if ((flags bitand IsLGB) and g_bRealisticAvionics and (SimLibElapsedTime - timeOfDeath) > (2.0f * SEC_TO_MSEC))
        {
            AircraftClass *parentAC = parent->IsAirplane() ? static_cast<AircraftClass*>(parent.get()) : NULL;

            //AI's don't need to keep a lock until impact
            // sfr: since someone removed the player check here, im using the parent instead
            //if(parent /* and ((AircraftClass *)parent)->IsPlayer()*/ )
            if (parentAC)
            {
                radical = 0;

                //if (playerAC->FCC->groundDesignateZ <= ZPos())
                if (parentAC->FCC->groundDesignateZ <= ZPos())
                {
                    radical = 0;
                }
                else
                {
                    //radical = (float)sqrt (ZDelta()*ZDelta() + 2.0F *
                    //GRAVITY * (playerAC->FCC->groundDesignateZ - ZPos()));
                    radical = (float)sqrt(ZDelta() * ZDelta() + 2.0F * GRAVITY *
                                          (parentAC->FCC->groundDesignateZ - ZPos()));
                }

                tFall = -ZDelta() - radical;

                if (tFall < 0.0F)
                {
                    tFall = -ZDelta() + radical;
                }

                tFall /= GRAVITY;

                //deltaX = playerAC->FCC->groundDesignateX - XPos();
                //deltaY = playerAC->FCC->groundDesignateY - YPos();
                //deltaZ = playerAC->FCC->groundDesignateZ - ZPos();
                deltaX = parentAC->FCC->groundDesignateX - XPos();
                deltaY = parentAC->FCC->groundDesignateY - YPos();
                deltaZ = parentAC->FCC->groundDesignateZ - ZPos();


                rx    = dmx[0][0] * deltaX + dmx[0][1] * deltaY + dmx[0][2] * deltaZ;
                ry    = dmx[1][0] * deltaX + dmx[1][1] * deltaY + dmx[1][2] * deltaZ;
                rz    = dmx[2][0] * deltaX + dmx[2][1] * deltaY + dmx[2][2] * deltaZ;
                range = (float)sqrt(rx * rx + ry * ry + rz * rz);
                //float range1 = (float)sqrt(deltaX*deltaX + deltaY*deltaY+deltaZ*deltaZ);
                //float rate = (float)sqrt(XDelta()*XDelta()+YDelta()*YDelta()+ZDelta()*ZDelta());

                //we need to find the time until impact
                if (parentAC->IsPlayer())
                {
                    parentAC->FCC->ImpactTime = tFall;
                }

                // 18 degree limit on the seeker
                /* JAM 17Apr04 - (range-rx * rx) yields a negative number, you can't take the square
                 root of a negative number In VC6, the seeker check always passes, but in VC >= 6 + PP,
                 the check always FAILS, due to differences in how the compilers treat sqrt(-). This is
                 why LGB's consistantly missed their targets in anything but vanilla VC6.
                */
                if (
                    Abs(acosf(rx / range)) <= 18.f * DTR and 
                    (parentAC->IsPlayer() and parentAC->FCC->LaserFire) or
 not parentAC->IsPlayer()
                )
                {
                    desDx = (deltaX) / tFall;
                    desDy = (deltaY) / tFall;
                    // 2001-04-17 ADDED BY S.G. WE'LL KEEP OUR LAST desDx and desDy IN THE UNUSED
                    // tgtX and tgtY BombClass VARIABLE (RENAMED desDxPrev AND desDyPrev)
                    // TO IN CASE WE LOOSE LOCK LATER
                    desDxPrev = desDx;
                    desDyPrev = desDy;
                    // END OF ADDED SECTION
                    SetDelta(0.8F * XDelta() + 0.2F * desDx, 0.8F * YDelta() + 0.2F * desDy, ZDelta());
                }
                else
                {
                    if ( not (desDxPrev == 0.0f and desDyPrev == 0.0f and desDzPrev == 0.0f))
                    {
                        // 2001-04-17 ADDED BY S.G. WE'LL KEEP GOING WHERE WE WERE GOING...
                        Falcon4EntityClassType *classPtr = &Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE];
                        WeaponClassDataType *wc = (WeaponClassDataType *)classPtr->dataPtr;

                        // If a 3rg gen LGB, fins are more precised, even when no longer lased...
                        if (wc->Flags bitand WEAP_LGB_3RD_GEN)
                        {
                            SetDelta(0.8F * XDelta() + 0.2f * desDxPrev, 0.8f * YDelta() + 0.2f * desDyPrev, ZDelta());
                        }
                        else
                        {
                            // 2001-10-19 MODIFIED BY S.G. IT'S * 1.05f AND NOT * 2.0f
                            SetDelta(
                                (0.8F * XDelta() + 0.2f * desDxPrev) * 1.05f,
                                (0.8f * YDelta() + 0.2f * desDyPrev) * 1.05f,
                                ZDelta()
                            );
                        }
                    }
                }

                if (targetPtr and targetPtr->BaseData() and parentAC->IsPlayer())
                {
                    if ( not ((SimBaseClass*)(targetPtr->BaseData()))->IsSetFlag(IS_LASED))
                    {
                        targetPtr->Release();
                        targetPtr = NULL;
                    }
                }
            }
        }
        //Wombat778 3-09-04 If this is a GPS weapon, guide to the GPS coordinates. A ripoff from the LGB code above
        // RV - Biker - Add 2 sec delay for guidance
        else if (((flags bitand IsGPS) or (flags bitand IsJSOW)) and (SimLibElapsedTime - timeOfDeath) > (2.0f * SEC_TO_MSEC))
        {
            FalconEntity *target = NULL;
            // SimBaseClass *simTarg;

            // Cobra - Check that we have a valid auxData->JDAMLift for JSOWs
            if (EntityType()->classInfo_[VU_STYPE] == STYPE_BOMB_JSOW and (auxData->JDAMLift <= 5.0f))
                auxData->JDAMLift = g_fJDAMLift;


            // Cobra - Skip target assigning until close to target
            rx = JSOWtgtPos.x - XPos();
            ry = JSOWtgtPos.y - YPos();
            range = (float)sqrt(rx * rx + ry * ry);

            // FRB - JSOW Test monitor
            if (range < 10.0f * NM_TO_FT)
            {
                if ((flags bitand IsJSOW) and targetPtr)
                {
                    // First get the campaign object if it's still a sim entity
                    /*
                    CampBaseClass *campBaseObj;
                    if (targetPtr->BaseData()->IsSim()) // If we're a SIM object, get our campaign object
                     campBaseObj = ((SimBaseClass*)targetPtr->BaseData())->GetCampaignObject();
                    else
                     campBaseObj = (CampBaseClass *)targetPtr->BaseData();
                    // Now find out if our campaign object is aggregated
                    if ((campBaseObj and not campBaseObj->IsAggregate()))
                    {
                    target = targetPtr->BaseData();
                    // Get the sim object associated to this entity number
                    simTarg = campBaseObj->GetComponentEntity(JSOWtgtID);

                    if (simTarg)
                    {
                    gpsx = simTarg->XPos();
                    gpsy = simTarg->YPos();
                    }
                    else
                    {
                    gpsx = targetPtr->BaseData()->XPos();
                    gpsy = targetPtr->BaseData()->YPos();
                    }
                    }
                    else // Use feature cheat coordinates */
                    {
                        int tgg = JSOWtgtID; // debug
                        gpsx = JSOWtgtPos.x;
                        gpsy = JSOWtgtPos.y;
                    }

                    if (fp)
                        fprintf(fp, "JSOWtgtID: %d  X= %f  Y= %f\n", JSOWtgtID, gpsx, gpsy);

                    fflush(fp);
                }
            }

            radical = 0;
            //if(gpsz <= ZPos())
            //  radical = 0;

            //radical = (float)sqrt (ZDelta()*ZDelta() + 2.0F * (GRAVITY - auxData->JDAMLift) * (gpsz - ZPos()));
            radical = (float)sqrt(ZDelta() * ZDelta() + 2.0F * GRAVITY * (gpsz - ZPos()));
            tFall = -ZDelta() - radical;

            if (tFall < 0.0F)
                tFall = -ZDelta() + radical;

            tFall /= GRAVITY;

            deltaX = gpsx - XPos();
            deltaY = gpsy - YPos();
            deltaZ = gpsz - ZPos();

            rx    = dmx[0][0] * deltaX + dmx[0][1] * deltaY + dmx[0][2] * deltaZ;
            ry    = dmx[1][0] * deltaX + dmx[1][1] * deltaY + dmx[1][2] * deltaZ;
            rz    = dmx[2][0] * deltaX + dmx[2][1] * deltaY + dmx[2][2] * deltaZ;
            range = (float)sqrt(rx * rx + ry * ry + rz * rz);
            //float range1 = (float)sqrt(deltaX*deltaX + deltaY*deltaY+deltaZ*deltaZ);
            //float rate = (float)sqrt(XDelta()*XDelta()+YDelta()*YDelta()+ZDelta()*ZDelta());

            desDx = (deltaX) / tFall;
            desDy = (deltaY) / tFall;
            desDxPrev = desDx;
            desDyPrev = desDy;

            //Limit new deltaX and Y to the initial deltaX and Y.  This stops the stupid Zoom issue.
            //Basically this is a VERY basic 2d energy model

            //Wombat778 3-12-04 Changed to a pythagorean theorem method
            static float maxenergy = sqrt((XDelta() * XDelta()) + (YDelta() * YDelta()));

            float newxdelta = 0.8F * XDelta() + 0.2F * desDx;
            float newydelta = 0.8F * YDelta() + 0.2F * desDy;
            //   float newzdelta= ZDelta()-(g_fJDAMLift*SimLibMajorFrameTime*cos(Pitch()));   //take the "lift" of the bomb into account
            float newzdelta = ZDelta() - (auxData->JDAMLift * SimLibMajorFrameTime * cos(Pitch())); // Cobra - Use Bombdata JDAMLift
            float newenergy = sqrt((newxdelta * newxdelta) + (newydelta * newydelta));

            if (newenergy > maxenergy)
            {
                newxdelta *= maxenergy / newenergy;
                newydelta *= maxenergy / newenergy;
            }

            SetDelta(newxdelta, newydelta, newzdelta); //Wombat778 3-12-04 added newzdelta
        }
        else if ((SimLibElapsedTime - timeOfDeath) > (2.0f * SEC_TO_MSEC))
        {
            if (targetPtr)
            {
                radical = (float)sqrt(ZDelta() * ZDelta() + 2.0F * GRAVITY * (targetPtr->BaseData()->ZPos() - ZPos()));
                tFall = -ZDelta() - radical;

                if (tFall < 0.0F)
                    tFall = -ZDelta() + radical;

                tFall /= GRAVITY;
                deltaX = targetPtr->BaseData()->XPos() - XPos();
                deltaY = targetPtr->BaseData()->YPos() - YPos();
                deltaZ = (float)fabs(targetPtr->BaseData()->ZPos() - ZPos());

                rx    = dmx[0][0] * deltaX + dmx[0][1] * deltaY + dmx[0][2] * deltaZ;
                ry    = dmx[1][0] * deltaX + dmx[1][1] * deltaY + dmx[1][2] * deltaZ;
                rz    = dmx[2][0] * deltaX + dmx[2][1] * deltaY + dmx[2][2] * deltaZ;
                range = (float)sqrt(rx * rx + ry * ry + rz * rz);

                desDx = (deltaX) / tFall;
                desDy = (deltaY) / tFall;

                SetDelta(0.8F * XDelta() + 0.2F * desDx, 0.8F * YDelta() + 0.2F * desDy, ZDelta());
            }
        }


        terrainHeight = OTWDriver.GetGroundLevel(x, y);

        // 2 seconds from release until any alt detonation will fuse
        if (SimLibElapsedTime - timeOfDeath > 2 * SEC_TO_MSEC)
        {
            bheight = burstHeight; // CBUs
        }
        else
            bheight = 0.0f;

        // check for feature collision impact

        if (bombType == None and z - terrainHeight > -800.0f)
        {
            hitObj = FeatureCollision(terrainHeight);

            if (hitObj)
            {
                //me123 OWLOOK make your armingdelay switch here.
                //MI
                //if (g_bArmingDelay and (SimLibElapsedTime - timeOfDeath > armingdelay *10  or ((AircraftClass *)parent)->isDigital))
                if (
                    g_bRealisticAvionics and 
                    (SimLibElapsedTime - timeOfDeath > armingdelay * 10  or
                     (parent and ((AircraftClass *)parent.get())->IsDigital()))
                )
                {
                    //me123 addet arming check, for now digi's dont's have arming delay, becourse they will fuck up the delivery
                    SendDamageMessage(hitObj, 0, FalconDamageType::BombDamage);
                    // JB 000816 ApplyProximityDamage( terrainHeight, 0.0f ); // Cause of objects not blowing up on runways
                    ApplyProximityDamage(terrainHeight, detonateHeight);
                    edeltaX = XDelta();
                    edeltaY = YDelta();
                    edeltaZ = ZDelta();
                    SetDelta(0.0f, 0.0f, 0.0f);
                    SetExploding(TRUE);

                    // if we've hit a flat container, NULL it out now so that this is
                    // treated as a ground hit later on
                    if (hitObj->IsSetCampaignFlag(FEAT_FLAT_CONTAINER))
                    {
                        hitObj = NULL;
                    }
                }
                //else if ( not g_bArmingDelay) MI
                else if ( not g_bRealisticAvionics)
                {
                    SendDamageMessage(hitObj, 0, FalconDamageType::BombDamage);
                    // JB 000816 ApplyProximityDamage( terrainHeight, 0.0f ); // Cause of objects not blowing up on runways
                    ApplyProximityDamage(terrainHeight, detonateHeight);
                    edeltaX = XDelta();
                    edeltaY = YDelta();
                    edeltaZ = ZDelta();
                    SetDelta(0.0f, 0.0f, 0.0f);
                    SetExploding(TRUE);

                    // if we've hit a flat container, NULL it out now so that this is
                    // treated as a ground hit later on
                    if (hitObj->IsSetCampaignFlag(FEAT_FLAT_CONTAINER))
                    {
                        hitObj = NULL;
                    }
                }
            }
            else if (z >= terrainHeight)
            {
                if (bombType == None and (SimLibElapsedTime - timeOfDeath > armingdelay * 10.0f or (parent and ((AircraftClass *)parent.get())->IsDigital()))) //me123 addet arming check
                {
                    // Interpolate
                    delta = (z - terrainHeight) / (z - ZPos());

                    x = x - delta * (x - XPos());
                    y = y - delta * (y - YPos());

                    edeltaX = XDelta();
                    edeltaY = YDelta();
                    edeltaZ = ZDelta();
                    SetDelta(0.0f, 0.0f, 0.0f);

                    SetYPR(Yaw() + (float)rand() / (float)RAND_MAX, 0.0F, 0.0F);

                    SetFlag(ON_GROUND);
                    z = terrainHeight;

                    SetExploding(TRUE);

                    ApplyProximityDamage(terrainHeight, detonateHeight);
                }
                else
                {
                    SetDead(TRUE);
                }
            }
        }

        //MI this else is causing our CBU's to not burst with a BA < 900 because of the check above
        //else
        if (bheight > 0 and z >= terrainHeight - bheight and not IsSetFlag(SHOW_EXPLOSION) and bombType == BombClass::None)   //me123 check addet to making flares stop exploding
        {
            // for altitude detonations we start the effect here
            SetFlag(SHOW_EXPLOSION);


            // 2002-02-26 ADDED BY S.G. If our target is an aggregated entity, send a 'SendDamageMessage' message to the target and let the 2D engine sort out what gets destroyed...
            if (targetPtr)
            {
                // First get the campaign object if it's still a sim entity
                CampBaseClass *campBaseObj;

                if (targetPtr->BaseData()->IsSim()) // If we're a SIM object, get our campaign object
                    campBaseObj = ((SimBaseClass*)targetPtr->BaseData())->GetCampaignObject();
                else
                    campBaseObj = (CampBaseClass *)targetPtr->BaseData();

                // Now find out if our campaign object is aggregated
                if (campBaseObj and campBaseObj->IsAggregate())
                {
                    // Yes, send a damage message right away otherwise the other code is not going to deal with it...
                    SendDamageMessage(campBaseObj, 0, FalconDamageType::BombDamage);
                }
            }

            endMessage = new FalconMissileEndMessage(Id(), FalconLocalGame);
            endMessage->RequestReliableTransmit();
            endMessage->RequestOutOfBandTransmit();
            endMessage->dataBlock.fEntityID = parent->Id();
            endMessage->dataBlock.fCampID = parent->GetCampID();
            endMessage->dataBlock.fSide = (uchar)parent->GetCountry();
            endMessage->dataBlock.fPilotID = shooterPilotSlot;
            endMessage->dataBlock.fIndex = parent->Type();
            endMessage->dataBlock.dEntityID = FalconNullId;
            endMessage->dataBlock.dCampID = 0;
            endMessage->dataBlock.dSide = 0;
            endMessage->dataBlock.dPilotID = 0;
            endMessage->dataBlock.dIndex = 0;
            endMessage->dataBlock.fWeaponUID = Id();
            endMessage->dataBlock.wIndex  = Type();
            endMessage->dataBlock.x = XPos() + XDelta() * SimLibMajorFrameTime * 2.0f;
            endMessage->dataBlock.y = YPos() + YDelta() * SimLibMajorFrameTime * 2.0f;
            endMessage->dataBlock.z = ZPos() + ZDelta() * SimLibMajorFrameTime * 2.0f;
            endMessage->dataBlock.xDelta = XDelta();
            endMessage->dataBlock.yDelta = YDelta();
            endMessage->dataBlock.zDelta = ZDelta();
            endMessage->dataBlock.groundType    = -1;
            endMessage->dataBlock.endCode = FalconMissileEndMessage::BombImpact;

            endMessage->SetParticleEffectName(auxData->psBombImpact); // MLR 6/26/2004 -

            FalconSendMessage(endMessage, FALSE);

            // set height at which we detonated for applying
            // proximity damage
            detonateHeight = max(0.0f, terrainHeight - z);
        }

        SetPosition(x, y, z);
    }

    return TRUE;
}

void BombClass::SetTarget(SimObjectType* newTarget)
{
    if (F4IsBadReadPtr(this, sizeof(BombClass))) // JB 010317 CTD
        return;

    if (newTarget == targetPtr)
        return;

    if (targetPtr)
    {
        targetPtr->Release();
        targetPtr = NULL;
    }

    if (newTarget)
    {
        ShiAssert(newTarget->BaseData() not_eq (FalconEntity*)0xDDDDDDDD);

        //#ifdef DEBUG
        // targetPtr = newTarget->Copy(OBJ_TAG, this);
        //#else
        // targetPtr = newTarget->Copy();
        //#endif
        targetPtr->Reference();
    }
}

void BombClass::GetTransform(TransformMatrix vmat)
{
    mlTrig trig;
    float xyDelta = XDelta() * XDelta() + YDelta() * YDelta();
    float vt = (float)sqrt(xyDelta + ZDelta() * ZDelta());
    float costha, sintha;

    xyDelta = (float)sqrt(xyDelta);
    mlSinCos(&trig, Yaw());
    costha = xyDelta / vt;
    sintha = ZDelta() / vt;

    vmat[0][0] = trig.cos * costha;
    vmat[0][1] = trig.sin * costha;
    vmat[0][2] = -sintha;

    vmat[1][0] = -trig.sin;
    vmat[1][1] = trig.cos;
    vmat[1][2] = 0.0F;

    vmat[2][0] = trig.cos * sintha;
    vmat[2][1] = trig.sin * sintha;
    vmat[2][2] = costha;
}

void BombClass::SetVuPosition(void)
{
    SetPosition(x, y, z);
}

/*
 ** Name: ApplyProximityDamage
 ** Description:
 ** Cycles thru objectList check for proximity.
 ** Cycles thru all objectives, and checks vs individual features
 **        if it's within the objective's bounds.
 */
#define RESCALE(in,inmin,inmax,outmin,outmax) ( ((in) - (inmin)) * ((outmax) - (outmin)) / ((inmax) - (inmin)) + (outmin))

void BombClass::ApplyProximityDamage(float groundZ, float detonateHeight)
{
    float tmpX, tmpY, tmpZ;
    float rangeSquare;
    SimBaseClass* testObject;
    CampBaseClass* objective;
    float damageRadiusSqrd;
    float strength, damageMod;
    WeaponClassDataType* wc;
    wc = (WeaponClassDataType *)(Falcon4ClassTable[Type() - VU_LAST_ENTITY_TYPE].dataPtr);
    float modifier = 1.0F;

    if (wc and wc->DamageType == NuclearDam)
        modifier = g_fNukeDamageRadius;

#ifdef VU_GRID_TREE_Y_MAJOR
    VuGridIterator gridIt(ObjProxList, YPos(), XPos(), NM_TO_FT * (3.5F * modifier));
#else
    VuGridIterator gridIt(ObjProxList, XPos(), YPos(), NM_TO_FT * (3.5F * modifier));
#endif

    //MI
    float hat = 0;
    float maxHeight = 3000.0f;


    // for altitude detonations (cluster bomb), the damage radius must
    // be changed depending on the detonation height
    if (burstHeight > 0.0f)
    {
        // COBRA - RED - Fixed in a more RL way
        // Height above Ground
        float HaG = detonateHeight - groundZ;
        //damageRadiusSqrd = max( lethalRadiusSqrd / 20, lethalRadiusSqrd * detonateHeight/3000.0f );
        damageRadiusSqrd = min(lethalRadiusSqrd, lethalRadiusSqrd * HaG / 1000.0f);

        // COBRA _ RED- Scale strenght to have it's Max at 500 Ft, then going down
        if (HaG < 1000) strength = 1.0f * HaG / 1000.0f;
        else strength = RESCALE(HaG, 1000.0f, 2000.0f, 1.0f, 0.1f);

        // Bomblets drops, so, always some strengt
        if (strength < .1f) strength = .1f;

    }
    else
    {
        damageRadiusSqrd = lethalRadiusSqrd;
        strength = 1.0f;
    }

    if (/*parentReferenced and */SimDriver.objectList)
    {
        // Damage multiplier for damage type
        switch (wc->DamageType)
        {
            case PenetrationDam:
            case HeaveDam:
            case KineticDam:
            case IncendairyDam:
            case ChemicalDam:
                damageMod = 1.0F;  // Cobra - no penalties
                //damageMod = 0.25F;
                break;

            case HighExplosiveDam:
            case ProximityDam:
            case HydrostaticDam:
            case OtherDam:
            case NoDamage:
            default:
                damageMod = 1.0F;
                break;
        }

        // Check vs vehicles
        VuListIterator objectWalker(SimDriver.objectList);
        testObject = (SimBaseClass*) objectWalker.GetFirst();

        while (testObject)
        {
            // until digi's are smarter about thier bombing, prevent them
            // from dying in their own blast
            // 2002-04-21 MN check for damage type and only skip if it is not a nuclear
            if (wc->DamageType not_eq NuclearDam and (testObject == parent and 
                                                 parent and parent->IsAirplane() and 
                                                 (((AircraftClass *)parent.get())->IsDigital() or
                                                  ((AircraftClass *)parent.get())->AutopilotType() == AircraftClass::CombatAP)))
            {
                testObject = (SimBaseClass*) objectWalker.GetNext();
                continue;
            }

            if (testObject not_eq this)
            {
                tmpX = testObject->XPos() - XPos();
                tmpY = testObject->YPos() - YPos();
                tmpZ = testObject->ZPos() - groundZ;

                rangeSquare = tmpX * tmpX + tmpY * tmpY + tmpZ * tmpZ;

                // Height Above Terrain
                if (parent)
                {
                    AircraftClass *p = static_cast<AircraftClass*>(parent.get());
                    hat = p->ZPos() - OTWDriver.GetGroundLevel(p->XPos(), p->YPos());
                }

                //MI special case for airplane. Use the "MaxAlt" field to determine if you blow up or not
                if (testObject and testObject->IsAirplane() and wc and wc->DamageType == NuclearDam)
                {
                    //if you're below the entered setting, you're screwed
                    if (fabsf((wc->MaxAlt) * 1000.0f) >= fabs(hat)) //JAM 27Sep03 - Should be fabsf
                        // 2002-03-25 MN removed damageMod, as the higher this value, the less the chance to hit
                        SendDamageMessage(testObject, rangeSquare * strength * /*damageMod*/ g_fNukeStrengthFactor, FalconDamageType::ProximityDamage);
                }
                // 2002-03-25 MN some more fixes for nukes
                else if (wc and wc->DamageType == NuclearDam)
                {
                    if (rangeSquare < damageRadiusSqrd * g_fNukeDamageMod)
                    {
                        SendDamageMessage(testObject, rangeSquare * strength * g_fNukeStrengthFactor, FalconDamageType::ProximityDamage);
                    }
                }
                else if (rangeSquare < damageRadiusSqrd * damageMod)
                    SendDamageMessage(testObject, rangeSquare * strength, FalconDamageType::ProximityDamage);
            }

            testObject = (SimBaseClass*) objectWalker.GetNext();
        }
    }

    // get the 1st objective that contains the bomb
    objective = (CampBaseClass*)gridIt.GetFirst();

    // main loop through objectives
    while (objective)
    {
        if (objective->GetComponents())
        {
            // loop thru each element in the objective
            VuListIterator featureWalker(objective->GetComponents());
            testObject = (SimBaseClass*) featureWalker.GetFirst();

            while (testObject)
            {
                if ( not testObject->IsSetCampaignFlag(FEAT_CONTAINER_TOP))
                {
                    tmpX = testObject->XPos() - XPos();
                    tmpY = testObject->YPos() - YPos();
                    // tmpZ = testObject->ZPos() - ZPos(); // Features are at ground level, and so is this bomb

                    rangeSquare = tmpX * tmpX + tmpY * tmpY;; // + tmpZ*tmpZ;

                    if (wc and wc->DamageType == NuclearDam)
                    {
                        if (rangeSquare < damageRadiusSqrd * g_fNukeDamageMod)
                        {
                            SendDamageMessage(testObject, rangeSquare * strength * g_fNukeStrengthFactor, FalconDamageType::ProximityDamage);
                        }
                    }
                    else if (rangeSquare < damageRadiusSqrd * damageMod) //MI added *damageMod
                    {
                        SendDamageMessage(testObject, rangeSquare * strength, FalconDamageType::ProximityDamage);
                    } // end if within lethal radius
                }

                testObject = (SimBaseClass*) featureWalker.GetNext();
            }
        }

        // get the next objective that contains the bomb
        objective = (CampBaseClass*)gridIt.GetNext();
    } // end objective loop

}

void BombClass::DoExplosion(void)
{
    ACMIStationarySfxRecord acmiStatSfx;
    FalconMissileEndMessage* endMessage;
    float groundZ;

    if ( not IsSetFlag(SHOW_EXPLOSION))
    {
        // edg note: all special effects are now handled in the
        // missile end message process method
        endMessage = new FalconMissileEndMessage(Id(), FalconLocalGame);
        endMessage->RequestReliableTransmit();
        endMessage->RequestOutOfBandTransmit();
        endMessage->dataBlock.fEntityID = parent ? parent->Id() : Id();
        endMessage->dataBlock.fCampID = parent ? parent->GetCampID() : 0;
        endMessage->dataBlock.fSide = parent ? (uchar)parent->GetCountry() : 0;
        endMessage->dataBlock.fPilotID = shooterPilotSlot;
        endMessage->dataBlock.fIndex = parent ? parent->Type() : 0;
        endMessage->dataBlock.dEntityID = FalconNullId;
        endMessage->dataBlock.dCampID = 0;
        endMessage->dataBlock.dSide = 0;
        endMessage->dataBlock.dPilotID = 0;
        endMessage->dataBlock.dIndex = 0;
        endMessage->dataBlock.fWeaponUID = Id();
        endMessage->dataBlock.wIndex  = Type();
        endMessage->dataBlock.x = XPos();
        endMessage->dataBlock.y = YPos();
        endMessage->dataBlock.z = ZPos();
        endMessage->dataBlock.xDelta = edeltaX;
        endMessage->dataBlock.yDelta = edeltaY;
        endMessage->dataBlock.zDelta = edeltaZ;

        // add crater depending on ground type and closeness to ground
        groundZ = OTWDriver.GetGroundLevel(XPos(), YPos());

        if (hitObj)
        {
            endMessage->dataBlock.endCode    = FalconMissileEndMessage::FeatureImpact;
            endMessage->SetParticleEffectName(auxData->psFeatureImpact);
        }
        else
        {
            endMessage->dataBlock.endCode    = FalconMissileEndMessage::BombImpact;
            endMessage->SetParticleEffectName(auxData->psBombImpact);
        }

        endMessage->dataBlock.groundType    =
            (char)OTWDriver.GetGroundType(XPos(), YPos());

        FalconSendMessage(endMessage, FALSE);


        if (hitObj == NULL and 
 not (endMessage->dataBlock.groundType == COVERAGE_WATER or
              endMessage->dataBlock.groundType == COVERAGE_RIVER)
           ) // and ( ZPos() - groundZ ) > -40.0f ) // JB 010710 craters weren't showing up
        {
            //AddToTimedPersistantList(
            // VIS_CRATER2 + PRANDInt3(), Camp_GetCurrentTime() + CRATER_REMOVAL_TIME, XPos(), YPos()
            //);
            AddToTimedPersistantList(
                MapVisId(VIS_CRATER2 + 2), Camp_GetCurrentTime() + CRATER_REMOVAL_TIME, XPos(), YPos()
            );

            // add crater to ACMI as special effect
            if (gACMIRec.IsRecording())
            {

                acmiStatSfx.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
                acmiStatSfx.data.type = SFX_CRATER4;
                acmiStatSfx.data.x = XPos();
                acmiStatSfx.data.y = YPos();
                acmiStatSfx.data.z = ZPos();
                acmiStatSfx.data.timeToLive = 180.0f;
                acmiStatSfx.data.scale = 1.0f;
                gACMIRec.StationarySfxRecord(&acmiStatSfx);
            }
        }

        // make sure we don't do it again...
        SetFlag(SHOW_EXPLOSION);
    }
    else if ( not IsDead())
    {
        // we can now kill it immediately
        SetDead(TRUE);
    }
}

void BombClass::SpecialGraphics(void)
{
    if (SimLibElapsedTime - timeOfDeath > 1 * SEC_TO_MSEC)
    {
        if (((DrawableBSP*)drawPointer)->GetNumSwitches() > 0)
        {
            ((DrawableBSP *)drawPointer)->SetSwitchMask(0, 1);
        }
    }
}

void BombClass::InitTrail(void)
{
}

void BombClass::UpdateTrail(void)
{
}

void BombClass::RemoveTrail(void)
{
}

int BombClass::IsUseable(void)
{
    if (lauWeaponId)
    {
        return ((lauRounds - lauFireCount) > 0); // could be bool'ize
    }
    else
        return 1;
}

float BombClass::GetJDAMLift(void)
{
    if (auxData)
        return auxData->JDAMLift;
    else
        return g_fJDAMLift;
}

float BombClass::GetJSOWmaxRange(void)
{
    if (auxData)
        return auxData->JSOWmaxRange;
    else
        return g_fAIJSOWmaxRange;
}

void BombClass::CreateGfx()
{
    //SimWeaponClass::CreateGfx();
    InitTrail();
    ExtraGraphics();
}

void BombClass::DestroyGfx()
{
    //SimWeaponClass::DestroyGfx();
}
