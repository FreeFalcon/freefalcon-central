#include "stdhdr.h"
#include "entity.h"
#include "object.h"
#include "sensors.h"
#include "PilotInputs.h"
#include "simmover.h"
#include "object.h"
#include "fsound.h"
#include "soundfx.h"
#include "radarDoppler.h"
#include "fack.h"
#include "aircrft.h"
#include "simdrive.h"
#include "hud.h"
#include "fcc.h"
#include "fakerand.h"

static int didDesignate = FALSE;
static int didDroptargetCmd = FALSE;
// 2001-02-21 MODIFIED BY S.G. APG68_BAR_WIDTH IS 2.2
//static const float APG68_BAR_WIDTH = (2.0f * DTR); // Here and in State.cpp
static const float APG68_BAR_WIDTH = (2.2f * DTR); // Here and in State.cpp
extern bool g_bMLU;

extern bool g_bAntElevKnobFix;  // MD -- 20031222: antenna elevation knob fixes

void RadarDopplerClass::ExecModes(int newDesignate, int newDrop)
{
    designateCmd = newDesignate and IsSOI();

    if (designateCmd)
        SetFlagBit(Designating);
    else
        ClearFlagBit(Designating);

    dropTrackCmd = newDrop;

    /*----------------------*/
    /* reacquisition marker */
    /*----------------------*/
    if (reacqFlag) reacqFlag--;

    /*---------------------*/
    /* service radar modes */
    /*---------------------*/

    switch (mode)
    {
            /*----------------------------*/
            /* Range While Search         */
            /*----------------------------*/
        case RWS:   // Range While Search
        case LRS:
            RWSMode();
            break;

        case SAM:   // Situational Awareness mode
            SAMMode();
            break;

        case STT:   // Single Target Track
            STTMode();
            break;

            /*-----------------------*/
            /* Auto Aquisition modes */
            /*-----------------------*/
        case ACM_30x20:   // Normal ACM
        case ACM_SLEW:    // Slewable ACM
        case ACM_BORE:    // Boresight ACM
        case ACM_10x60:   // Vertical Search ACM
            ACMMode();
            break;

        case VS:
            VSMode();   // Velocity Search Mode
            break;

        case TWS:
            TWSMode();
            break;

        case GM:
        case GMT:
        case SEA:
            GMMode();
            break;
    }

    didDesignate = designateCmd;

}

void RadarDopplerClass::RWSMode()
{
    SimObjectType* rdrObj;
    SimObjectLocalData* rdrData;

    /*-----------------------*/
    /* Spotlight / Designate */
    /*-----------------------*/
    if (IsSet(Spotlight) and not designateCmd)
        SetFlagBit(Designating);
    else
        ClearFlagBit(Designating);

    if ( not IsSet(Spotlight) and designateCmd)
    {
        lastAzScan = azScan;
        lastBars = bars;
        azScan = 10.0F * DTR;
        bars = 4;
        ClearSensorTarget();
    }

    if (designateCmd)
        SetFlagBit(Spotlight);
    else
        ClearFlagBit(Spotlight);

    /*-------------------*/
    /* check all objects */
    /*-------------------*/
    rdrObj = platform->targetList;

    while (rdrObj)
    {
        rdrData = rdrObj->localData;

        /*-------------------------------*/
        /* check for object in radar FOV */
        /*-------------------------------*/
        if (rdrData->painted)
        {
            /*----------------------*/
            /* detection this frame */
            /*----------------------*/
            if (rdrData->rdrDetect bitand 0x10)
            {
                AddToHistory(rdrObj, Solid);
            }
            /*--------------*/
            /* no detection */
            /*--------------*/
            else
                ExtrapolateHistory(rdrObj);
        }

        /*-------------*/
        /* Track State */
        /*-------------*/
        // 2002-03-25 MN add a check if the target is in our radar cone - if not, we not even have an UnreliableTrack
        // This fixes the AI oscillating target acquisition and losing
        if ((rdrData->rdrDetect bitand 0x1f) and (fabs(rdrData->ata) < radarData->ScanHalfAngle))
            rdrData->sensorState[Radar] = UnreliableTrack;

        /*--------------------------------*/
        /* If designating, check for lock */
        /*--------------------------------*/
        if (IsSet(Designating) and (mode == RWS or mode == LRS) and rdrObj->BaseData()->Id() == targetUnderCursor)
        {
            // Always lock if it is bright green (detected last time around)
            if (rdrData->rdrDetect bitand 0x10)
            {
                rdrData->rdrDetect = 0x1f;
                SetSensorTarget(rdrObj);
                ClearHistory(lockedTarget);
                AddToHistory(lockedTarget, Track);
                ClearFlagBit(Designating);
                ChangeMode(SAM);
                //   seekerAzCenter = rdrData->az;
                //   seekerElCenter = rdrData->el;
                //   beamAz   = 0.0F;
                //   beamEl   = 0.0F;
                SetScan();
                break;
            }
        }

        rdrObj = rdrObj->next;
    }

    /*-----------------------------------*/
    /* No target found, return to search */
    /*-----------------------------------*/
    if (IsSet(Designating))
    {
        azScan = lastAzScan;
        bars = lastBars;
    }
}

void RadarDopplerClass::SAMMode(void)
{
    int totHits, dropSAM = FALSE;
    float  tmpRange, tmpVal;
    SimObjectType* rdrObj;
    static bool justdidSTT = FALSE;

    // Drop Track, Revert to last mode
    if (lockedTarget == NULL)
    {
        //MI better do what the comment tells us
        //ChangeMode (RWS);
        ChangeMode(prevMode);
        return;
    }

    // Drop immediatly on leaving volume
    if (fabs(lockedTargetData->ata) > MAX_ANT_EL)//me123 or
        //me123 fabs(lockedTargetData->el) > MAX_ANT_EL)
    {
        dropSAM = TRUE;
    }

    if ( not g_bAntElevKnobFix)
    {
        if (oldseekerElCenter and subMode not_eq SAM_AUTO_MODE)
        {
            seekerElCenter = min(max(oldseekerElCenter, -MAX_ANT_EL + elScan), MAX_ANT_EL - elScan);
            oldseekerElCenter = 0.0f;
        }
    }
    else
        seekerElCenter = AntElevKnob();  // always center on the elevation commanded by the knob position


    if (IsSet(STTingTarget))
    {
        justdidSTT = TRUE;

        if (dropSAM)
        {
            float elhack;

            if ( not g_bAntElevKnobFix)
                elhack = oldseekerElCenter;

            azScan = lastSAMAzScan;
            bars = lastSAMBars;
            barWidth = APG68_BAR_WIDTH;
            ClearFlagBit(VerticalScan);
            SetFlagBit(HorizontalScan);
            scanDir  = ScanFwd;
            ClearFlagBit(STTingTarget);
            ChangeMode(SAM);

            if ( not g_bAntElevKnobFix)
            {
                oldseekerElCenter = elhack;
                seekerElCenter = elhack;
            }
            else
                seekerElCenter = AntElevKnob();
        }
        else
        {
            STTMode();
        }
    }
    else
    {
        /*-------------------*/
        /* check all objects */
        /*-------------------*/
        rdrObj = platform->targetList;

        while (rdrObj)
        {
            if (rdrObj->BaseData()->Id() == targetUnderCursor and IsSet(Designating) and not didDesignate)
            {
                // Bug a target and go into STT
                if (rdrObj == lockedTarget)
                {
                    SetSensorTarget(rdrObj);
                    ClearFlagBit(Designating);
                    seekerAzCenter = lockedTargetData->az;
                    seekerElCenter = lockedTargetData->el;
                    beamAz   = 0.0F;
                    beamEl   = 0.0F;
                    ClearFlagBit(SpaceStabalized);
                    beamWidth = radarData->BeamHalfAngle;
                    lastSAMAzScan = azScan;
                    lastSAMBars = bars;
                    azScan = 0.0F * DTR;
                    bars = 1;
                    barWidth = APG68_BAR_WIDTH;
                    ClearFlagBit(VerticalScan);
                    SetFlagBit(HorizontalScan);
                    scanDir  = ScanNone;
                    SetScan();
                    SetFlagBit(STTingTarget);
                    patternTime = 1;
                }
                else
                {
                    // Move sam target to something else.
                    // Lock anything detected at last chance
                    if (rdrObj->localData->rdrDetect bitand 0x10)
                    {
                        if (lockedTarget)
                        {
                            ClearHistory(lockedTarget);
                            AddToHistory(lockedTarget, Solid);
                        }

                        rdrObj->localData->rdrDetect = 0x1f;
                        SetSensorTarget(rdrObj);
                        ClearHistory(lockedTarget);
                        AddToHistory(lockedTarget, Track);
                        ClearFlagBit(Designating);
                        //seekerAzCenter = rdrObj->localData->az;
                        //seekerElCenter = rdrObj->localData->el;
                        rdrObj->localData->sensorLoopCount[Radar] = SimLibElapsedTime;
                        /*
                        beamAz   = 0.0F;
                        beamEl   = 0.0F;
                        SetFlagBit (STTingTarget);
                        patternTime = 1;
                        */
                        break;
                    }
                }
            }

            /*-------------------------------*/
            /* check for object in radar FOV */
            /*-------------------------------*/
            if (rdrObj->localData->painted)
            {
                /*----------------------*/
                /* detection this frame */
                /*----------------------*/
                if (rdrObj->localData->rdrDetect bitand 0x10)
                {
                    AddToHistory(rdrObj, Solid);
                }
                /*--------------*/
                /* no detection */
                /*--------------*/
                else
                    ExtrapolateHistory(rdrObj);
            }

            rdrObj = rdrObj->next;
        }

        /*------------*/
        /* SAM Target */
        /*------------*/
        // MD -- 20031222: use a helper function
        totHits = HitsOnTrack(lockedTargetData);

        if (lockedTargetData->painted)
        {
            if (lockedTargetData->rdrDetect bitand 0x10)
            {
                lockedTargetData->sensorState[Radar] = SensorTrack;
                AddToHistory(lockedTarget, Track);
            }

            //         lockedTargetData->rdrSy[1] = None;
        }


        if (subMode == SAM_MANUAL_MODE)
        {
            // Recalculate az limit
            CalcSAMAzLimit();
        }
        else
        {
            oldseekerElCenter = seekerElCenter;//me123
            tmpVal = TargetAz(platform, lockedTarget);
            seekerAzCenter = min(max(tmpVal , -MAX_ANT_EL + azScan), MAX_ANT_EL - azScan);
            tmpVal = TargetEl(platform, lockedTarget);

            if ( not g_bAntElevKnobFix)
            {
                if ( not oldseekerElCenter)
                    seekerElCenter = min(max(tmpVal, -MAX_ANT_EL + elScan), MAX_ANT_EL - elScan);
            }
            else
                seekerElCenter = AntElevKnob();  // always center on the knob in SAM
        }

        /*------------------*/
        /* Auto Range Scale */
        /*------------------*/
        //me123 don't autorange in SAM MODE
        //    if (lockedTargetData->range > 0.9F * tdisplayRange and 
        //     curRangeIdx < NUM_RANGES - 1)
        //    rangeChangeCmd = 1;
        //    else if (lockedTargetData->range < 0.4F * tdisplayRange and 
        //     curRangeIdx > 0)
        //    rangeChangeCmd = -1;
        if ( not dropTrackCmd)
            justdidSTT = FALSE;

        if (totHits < HITS_FOR_TRACK or dropSAM or (dropTrackCmd and not justdidSTT))
        {
            if (platform == SimDriver.GetPlayerAircraft() and ((AircraftClass*)platform)->AutopilotType() == AircraftClass::CombatAP)
            {
                // nothing....this causes lock and brakelocs stream in mp
            }
            else
            {
                rangeChangeCmd = 0;
                reacqEl = lockedTargetData->el;
                reacqFlag = (int)(ReacqusitionCount / SEC_TO_MSEC * SimLibMajorFrameRate);
                ClearHistory(lockedTarget);
                tmpRange = displayRange;
                //MI
                //ChangeMode (RWS);
                ChangeMode(prevMode);
                azScan = rwsAzs[rwsAzIdx];
                displayAzScan = rwsAzs[rwsAzIdx];
                bars = rwsBars[rwsBarIdx];
                displayRange = tmpRange;
                tdisplayRange = displayRange * NM_TO_FT;
                SetScan();
                ClearSensorTarget();
            }
        }
    }
}

#if 0

// MD -- 20040125
// The old TWS mode code -- here for posterity or at least until testing on the new version is complete
// enough to warrant deleting thins lot.
void RadarDopplerClass::TWSMode(void)
{
    int totHits;
    float  tmpVal;
    SimObjectType* rdrObj;
    SimObjectLocalData* rdrData;
    static bool tgtenteredcursor = FALSE;
    static bool justdidSTT = FALSE;

    // No TWS if sngl failure
    if (platform == SimDriver.GetPlayerAircraft())
    {
        if (((AircraftClass*)platform)->mFaults->GetFault(FaultClass::fcc_fault) == FaultClass::sngl)
        {
            ClearSensorTarget();
            return;
        }
    }

    if (oldseekerElCenter and not targetUnderCursor) //me123
    {
        seekerElCenter = min(max(oldseekerElCenter, -MAX_ANT_EL + elScan), MAX_ANT_EL - elScan);
        oldseekerElCenter = 0.0f;
    }

    if (IsSet(STTingTarget))
    {
        justdidSTT = TRUE;
        STTMode();
        //return;
    }

    //me123 addet spotlight to tws
    /*-----------------------*/
    /* Spotlight / Designate */
    /*-----------------------*/
    if (IsSet(Spotlight) and not designateCmd)
        SetFlagBit(Designating);
    else
        ClearFlagBit(Designating);

    if ( not IsSet(Spotlight) and designateCmd)
    {
        if ( not tgtenteredcursor)
        {
            lastAzScan = azScan;
            lastBars = bars;
            azScan = 10.0F * DTR;
            bars = 4;
            ClearSensorTarget();
            ClearFlagBit(STTingTarget);
        }
        else if (lockedTarget and lockedTarget->BaseData()->Id() == targetUnderCursor)
        {
            ClearFlagBit(Designating);
            SetSensorTarget(lockedTarget);
            seekerAzCenter = lockedTargetData->az;
            seekerElCenter = lockedTargetData->el;
            beamAz   = 0.0F;
            beamEl   = 0.0F;
            //       ClearFlagBit (SpaceStabalized);
            beamWidth = radarData->BeamHalfAngle;
            lastSAMAzScan = azScan;
            lastSAMBars = bars;
            azScan = 0.0F; // * DTR;  // MD -- 20031222: superfluous *
            bars = 1;
            barWidth = APG68_BAR_WIDTH;
            ClearFlagBit(VerticalScan);
            SetFlagBit(HorizontalScan);
            scanDir  = ScanNone;
            SetScan();
            SetFlagBit(STTingTarget);
            patternTime = 1;
        }
    }

    if (designateCmd)
        SetFlagBit(Spotlight);
    else
        ClearFlagBit(Spotlight);

    if (IsSet(Designating))
    {
        azScan = lastAzScan;
        bars = lastBars;
    }

    // max 30 az when tgt under cursor or "locked" target
    if ( not IsSet(Spotlight) and not IsSet(Designating))
    {

        if (targetUnderCursor or (lockedTargetData and not g_bMLU))
        {

            if (azScan > 30.0F * DTR)
            {
                lastAzScan = azScan;
                lastBars = bars;
                azScan = 30.0F * DTR;
                tgtenteredcursor = TRUE;
            }
        }
        else if (tgtenteredcursor and lastAzScan > 30.0F * DTR)
        {
            azScan = lastAzScan;
            tgtenteredcursor = FALSE;
        }
    }

    if (lasttargetUnderCursor)
    {

        int attach = FALSE;
        //check if the attached target is still in the targetlist
        rdrObj = platform->targetList;

        while (rdrObj and not attach)
        {
            //TJL 11/16/03 Adding init of rdrData
            rdrData = rdrObj->localData;

            //TJL 11/16/03 Added checks for painted and locked target.
            //Only in those conditions should the cursor track.
            //Cursor track now stops at gimbals or when lock is lost.

            // MD -- 20031222: slightly different method of determining the sticky conditions
            // will make sticky work more effectively.  For now the sticky falls off if the target
            // exceeds max antenna AZ/EL but it may be more correct to extrapolate the track even out
            // past the end of the field of view.

            //if ((rdrObj == lasttargetUnderCursor) and (rdrData->painted or lockedTarget) )
            if (rdrObj == lasttargetUnderCursor)
                if ((HitsOnTrack(rdrObj->localData) > HITS_FOR_TRACK) or lockedTarget)
                    if (fabs(TargetAz(platform, lasttargetUnderCursor)) < MAX_ANT_EL)
                        if (fabs(TargetEl(platform, lasttargetUnderCursor)) < MAX_ANT_EL)
                            attach = TRUE;

            rdrObj = rdrObj->next;
        }

        if (attach and SimDriver.GetPlayerAircraft() and SimDriver.GetPlayerAircraft()->FCC
           and SimDriver.GetPlayerAircraft()->FCC->cursorXCmd == 0 and SimDriver.GetPlayerAircraft()->FCC->cursorYCmd == 0)

        {
            //me123 attach the cursor

            TargetToXY(lasttargetUnderCursor->localData, 0, tdisplayRange, &cursorX, &cursorY);

            if ( not oldseekerElCenter)
                oldseekerElCenter = seekerElCenter;//me123

            tmpVal = TargetAz(platform, lasttargetUnderCursor);
            seekerAzCenter = min(max(tmpVal , -MAX_ANT_EL + azScan), MAX_ANT_EL - azScan);
            tmpVal = TargetEl(platform, lasttargetUnderCursor);

            if ( not g_bAntElevKnobFix)    // MD -- 20031222: EL should only follow target if we're locked on
            {
                seekerElCenter = min(max(tmpVal, -MAX_ANT_EL + elScan), MAX_ANT_EL - elScan);
            }
            else if (lockedTarget)
            {
                seekerElCenter = min(max(tmpVal, -MAX_ANT_EL + elScan), MAX_ANT_EL - elScan);
            }
            else
                seekerElCenter = AntElevKnob();  // track the knob position
        }
        else
        {
            lasttargetUnderCursor = NULL;
            attach = FALSE;
        }
    }


    /*-------------------*/
    /* check all objects */
    /*-------------------*/
    rdrObj = platform->targetList;

    while (rdrObj)
    {
        rdrData = rdrObj->localData;

        /*-------------------------------*/
        /* check for target under cursor */
        /*-------------------------------*/
        if (rdrObj->BaseData()->Id() == targetUnderCursor)
        {
            if ( not lasttargetUnderCursor)
                lasttargetUnderCursor = rdrObj;

            // MD -- 20031222: use a helper function
            totHits = HitsOnTrack(rdrData);

            if (IsSet(Designating) and totHits > HITS_FOR_TRACK)
            {
                SetSensorTarget(rdrObj);
                ClearFlagBit(Designating);
            }
        }

        /*-------------------------------*/
        /* check for object in radar FOV */
        /*-------------------------------*/
        if (rdrData->painted)
        {
            // count Hits
            // MD -- 20031222: use a helper function
            totHits = HitsOnTrack(rdrData);

            /*----------------------*/
            /* detection this frame */
            /*----------------------*/
            if (rdrData->rdrDetect bitand 0x10)
            {
                if (totHits > HITS_FOR_TRACK)
                {
                    AddToHistory(rdrObj, Track);
                }
                else
                {
                    AddToHistory(rdrObj, Solid);
                }
            }
        }

        rdrObj = rdrObj->next;
    }

    /*------------*/
    /* SAM Target */
    /*------------*/
    if (lockedTargetData)
    {
        if (lockedTargetData->painted)
        {
            if (lockedTargetData->rdrDetect bitand 0x10)
            {
                lockedTargetData->sensorState[Radar] = SensorTrack;
                AddToHistory(lockedTarget, Bug);
            }
        }

        static int test = 10;
        test = 20;
        //if ( not oldseekerElCenter) oldseekerElCenter = seekerElCenter;//me123
        tmpVal = TargetAz(platform, lockedTarget);
        seekerAzCenter = min(max(tmpVal , -MAX_ANT_EL + azScan), MAX_ANT_EL - azScan);
        tmpVal = TargetEl(platform, lockedTarget);

        if ( not g_bMLU and not g_bAntElevKnobFix)   // MD -- 20031223: antenna elevation fixes
            seekerElCenter = min(max(tmpVal, -MAX_ANT_EL + elScan), MAX_ANT_EL - elScan);

        /*------------------*/
        /* Auto Range Scale */
        /*------------------*/
        if (lockedTargetData->range > 0.9F * tdisplayRange and 
            curRangeIdx < NUM_RANGES - 1)
            rangeChangeCmd = 1;

        // MD -- 20031222: use a helper function
        totHits = HitsOnTrack(lockedTargetData);

        if (lockedTarget)
        {
            //me123 gimbal check addet
            // Drop lock if the guy is outside our radar cone
            if ((fabs(lockedTarget->localData->az) > radarData->ScanHalfAngle) or
                (fabs(lockedTarget->localData->el) > radarData->ScanHalfAngle))
            {
                ClearSensorTarget();
            }
        }

        if ( not dropTrackCmd)
            justdidSTT = FALSE;

        if ( not IsSet(STTingTarget) and ((dropTrackCmd and not justdidSTT) or totHits < HITS_FOR_TRACK))
        {
            rangeChangeCmd = 0;

            if (lockedTargetData) reacqEl = lockedTargetData->el;

            reacqFlag = (int)(ReacqusitionCount / SEC_TO_MSEC * SimLibMajorFrameRate);
            ClearSensorTarget();
            //me123 dont do this
            //         seekerAzCenter = 0.0F;
            //         seekerElCenter = 0.0F;
        }
    }
}

#endif

// MD -- 20040125: completed first pass at extensive re-write for TWS mode.  This one follows the
// open source information about the Block 50/52 APG-68 TWS mode rather better than the old
// implementation.

void RadarDopplerClass::TWSMode(void)
{
    float  tmpVal;
    SimObjectType* rdrObj;
    SimObjectLocalData* rdrData;
    static bool tgtenteredcursor = FALSE;
    static bool justdidSTT = FALSE;

    // No TWS if sngl failure
    if (platform == SimDriver.GetPlayerAircraft())
    {
        if (((AircraftClass*)platform)->mFaults->GetFault(FaultClass::fcc_fault) == FaultClass::sngl)
        {
            ClearSensorTarget();

            // MD -- 20041017: and clear any TWS directory entries.
            if (TWSTrackDirectory)
                TWSTrackDirectory = TWSTrackDirectory->Purge();

            return;
        }
    }

    // update the TWS Track Directory
    TWSTrackDirectory = UpdateTWSDirectory(platform->targetList, TWSTrackDirectory);

    if (IsSet(STTingTarget))
    {
        justdidSTT = TRUE;
        STTMode();
    }

    //me123 addet spotlight to tws
    /*-----------------------*/
    /* Spotlight / Designate */
    /*-----------------------*/
    if (IsSet(Spotlight) and not designateCmd)
        SetFlagBit(Designating);
    else
        ClearFlagBit(Designating);

    if ( not IsSet(Spotlight) and designateCmd)
    {
        if ( not tgtenteredcursor)
        {
            lastTwsAzIdx = curAzIdx;
            lastTwsBarIdx = curBarIdx;
            // spotlighting pattern is +/-10 degrees, 4 bars
            curAzIdx = twsAzIdx = 0;
            azScan = twsAzs[curAzIdx];
            curBarIdx = twsBarIdx = 0;
            bars = twsBars[curBarIdx];
            SetScan();
            ClearSensorTarget();
            ClearFlagBit(STTingTarget);
        }
    }

    if (designateCmd)
        SetFlagBit(Spotlight);
    else
        ClearFlagBit(Spotlight);

    if ( not IsSet(Spotlight) and not IsSet(Designating) and not IsSet(STTingTarget))
    {

        if (targetUnderCursor or (lockedTargetData and not g_bMLU))
        {
            lastTwsAzIdx = curAzIdx;
            lastTwsBarIdx = curBarIdx;
            curAzIdx = twsAzIdx = 2;
            curBarIdx = twsBarIdx = 2;
            azScan = twsAzs[curAzIdx];
            bars = twsBars[curBarIdx];
            SetScan();
            tgtenteredcursor = TRUE;
        }
        else if (tgtenteredcursor)
        {
            curBarIdx = twsBarIdx = lastTwsBarIdx;
            curAzIdx = twsAzIdx = lastTwsAzIdx;
            azScan = twsAzs[curAzIdx];
            bars = twsBars[curBarIdx];
            SetScan();
            tgtenteredcursor = FALSE;
        }
    }

    if (lasttargetUnderCursor)
    {

        int attach = FALSE;
        //check if the attached target is still in the targetlist
        rdrObj = platform->targetList;

        while (rdrObj and not attach)
        {
            rdrData = rdrObj->localData;

            if (rdrObj == lasttargetUnderCursor)
                if (rdrData->TWSTrackFileOpen or lockedTarget)
                    attach = TRUE;

            rdrObj = rdrObj->next;
        }

        if (attach and SimDriver.GetPlayerAircraft() and SimDriver.GetPlayerAircraft()->FCC
           and SimDriver.GetPlayerAircraft()->FCC->cursorXCmd == 0 and SimDriver.GetPlayerAircraft()->FCC->cursorYCmd == 0)

        {
            //me123 attach the cursor

            TargetToXY(lasttargetUnderCursor->localData, 0, tdisplayRange, &cursorX, &cursorY);
            tmpVal = TargetAz(platform, lasttargetUnderCursor);
            seekerAzCenter = min(max(tmpVal , -MAX_ANT_EL + azScan), MAX_ANT_EL - azScan);
            tmpVal = TargetEl(platform, lasttargetUnderCursor);

            if ( not g_bAntElevKnobFix)    // MD -- 20031222: EL should only follow target if we're locked on
            {
                seekerElCenter = min(max(tmpVal, -MAX_ANT_EL + elScan), MAX_ANT_EL - elScan);
            }
            else if (lockedTarget)
            {
                seekerElCenter = min(max(tmpVal, -MAX_ANT_EL + elScan), MAX_ANT_EL - elScan);
            }
            else
                seekerElCenter = AntElevKnob();  // track the knob position
        }
        else
        {
            lasttargetUnderCursor = NULL;
            attach = FALSE;
        }
    }


    /*-------------------*/
    /* check all objects */
    /*-------------------*/
    rdrObj = platform->targetList;

    while (rdrObj)
    {
        rdrData = rdrObj->localData;

        /*-------------------------------*/
        /* check for target under cursor */
        /*-------------------------------*/
        if (rdrObj->BaseData()->Id() == targetUnderCursor)
        {
            if ( not lasttargetUnderCursor)
                lasttargetUnderCursor = rdrObj;

            if (IsSet(Designating))
            {
                if (lockedTarget and (lockedTarget->BaseData()->Id() == targetUnderCursor))
                {
                    ClearFlagBit(Designating);
                    SetSensorTarget(lockedTarget);
                    seekerAzCenter = lockedTargetData->az;
                    seekerElCenter = lockedTargetData->el;
                    beamAz   = 0.0F;
                    beamEl   = 0.0F;
                    beamWidth = radarData->BeamHalfAngle;
                    lastTwsAzIdx = curAzIdx;
                    lastTwsBarIdx = curBarIdx;
                    azScan = 0.0F;
                    bars = 1;
                    barWidth = APG68_BAR_WIDTH;
                    ClearFlagBit(VerticalScan);
                    SetFlagBit(HorizontalScan);
                    scanDir  = ScanNone;
                    SetScan();
                    SetFlagBit(STTingTarget);
                    patternTime = 1;
                }
                else

                    // can't bug on a target being extrapolated
                    if (rdrData->TWSTrackFileOpen and (rdrData->extrapolateStart == 0))
                    {
                        SetSensorTarget(rdrObj);
                        AddToHistory(rdrObj, Bug);  // promote to bug
                        ClearFlagBit(Designating);
                    }
                    else
                    {
                        // force addition to the directory of any search target the pilot is interested in if the
                        // directory already has content otherwise start a new directory.
                        if (TWSTrackDirectory)
                            TWSTrackDirectory = TWSTrackDirectory->ForceInsert(rdrObj);
                        else
                            TWSTrackDirectory = new TWSTrackList(rdrObj);

                        AddToHistory(rdrObj, Track);  // promote to track
                        ClearFlagBit(Designating);
                    }

            }
        }

        /*-------------------------------*/
        /* check for object in radar FOV */
        /*-------------------------------*/
        if (rdrData->painted)
        {
            /*----------------------*/
            /* detection this frame */
            /*----------------------*/
            if (rdrData->rdrDetect bitand 0x10)
            {
                if ( not rdrData->TWSTrackFileOpen)
                {
                    AddToHistory(rdrObj, Solid);
                }
                else
                {
                    if (rdrObj == lockedTarget)
                    {
                        lockedTargetData->sensorState[Radar] = SensorTrack;
                        AddToHistory(lockedTarget, Bug);
                    }
                    else
                    {
                        // Update track status for anything on the list that isn't bugged
                        AddToHistory(rdrObj, Track);
                    }
                }
            }
        }

        rdrObj = rdrObj->next;
    }

    if (lockedTarget and lockedTargetData)
    {
        /*------------------*/
        /* Auto Range Scale */
        /*------------------*/
        if (lockedTargetData->range > 0.9F * tdisplayRange and 
            curRangeIdx < NUM_RANGES - 1)
            rangeChangeCmd = 1;

        if (lockedTarget)
        {
            // Drop lock if the guy is outside our radar cone and remove the track file
            if ((fabs(lockedTarget->localData->az) > radarData->ScanHalfAngle) or
                (fabs(lockedTarget->localData->el) > radarData->ScanHalfAngle))
            {
                ClearHistory(lockedTarget);

                if (TWSTrackDirectory)
                    TWSTrackDirectory = TWSTrackDirectory->Remove(lockedTarget);

                ClearSensorTarget();
            }
            else

                // when there is a bugged target, elevation is centered on it not the antenna knob position
                if ( not IsSet(STTingTarget))
                    seekerElCenter = lockedTarget->localData->el;
        }

        if ( not dropTrackCmd)
            justdidSTT = FALSE;

        if ( not IsSet(STTingTarget) and ((dropTrackCmd and not justdidSTT) and lockedTargetData->TWSTrackFileOpen))
        {
            rangeChangeCmd = 0;

            if (lockedTargetData)
                reacqEl = lockedTargetData->el;

            reacqFlag = (int)(ReacqusitionCount / SEC_TO_MSEC * SimLibMajorFrameRate);

            if (lockedTarget)
                AddToHistory(lockedTarget, Track);  // demote from bug to track

            ClearSensorTarget();
            SimDriver.GetPlayerAircraft()->FCC->dropTrackCmd = FALSE;  // shouldn't need this but timing is everything
        }
    }
    else
    {
        if (dropTrackCmd and not lockedTarget and TWSTrackDirectory)
        {
            // on udesignate with no bugged target, clear track directory and history and rebuild
            // starting next frame.
            ClearAllHistory();

            if (TWSTrackDirectory)
                TWSTrackDirectory = TWSTrackDirectory->Purge();
        }
    }

    if (IsSet(Designating) and not IsSet(STTingTarget))
    {
        curBarIdx = (twsBarIdx = lastTwsBarIdx);
        curAzIdx = (twsAzIdx = lastTwsAzIdx);
        azScan = twsAzs[curAzIdx];
        bars = twsBars[curBarIdx];
        SetScan();
    }
}

void RadarDopplerClass::STTMode(void)//me123 status test. multible changes
{
    int totHits;
    static bool diddroptrack = FALSE;

    if (oldseekerElCenter and not lockedTargetData) //me123
    {
        seekerElCenter = min(max(oldseekerElCenter, -MAX_ANT_EL + elScan), MAX_ANT_EL - elScan);
        oldseekerElCenter = 0.0f;
    }

    if ( not lockedTarget)
    {
        ClearFlagBit(STTingTarget);

        if (mode not_eq STT)
            ChangeMode(mode);
        else
            ChangeMode(prevMode);

        return;
    }
    else
    {
        //me123 gimbal check addet //Cobra 10/31/04 TJL

        // Drop lock if the guy is outside our radar cone
        if ((fabs(lockedTarget->localData->az) > radarData->ScanHalfAngle) or
            (fabs(lockedTarget->localData->el) > radarData->ScanHalfAngle))
        {
            // MD -- 20040911: according to Mirv, ACM with a lock losing track at the
            // gimbals should not cause the radar to go to NO RAD but dropTrackCmd
            // is also used in ACM mode for the TMS down and in that case it should
            // go to NO RAD.  Since the ACMMode() code can't tell the difference,
            // let's just drop the lock here for ACM modes and let the other modes
            // with STTingTarget use dropTrackCmd as before.  May not be pretty but
            // it seems to work how Mirv wants it ;)

            if (mode == RadarClass::ACM_30x20 or mode == RadarClass::ACM_SLEW or
                mode == RadarClass::ACM_BORE or mode == RadarClass::ACM_10x60)
            {
                ClearSensorTarget();
                ChangeMode(mode);
                SetScan();
                lockedTarget = NULL;
                return;
            }
            else
                dropTrackCmd = TRUE;
        }
    }

    if ( not lockedTargetData)
        return;

    if ( not g_bMLU and (dropTrackCmd and not didDesignate and mode == SAM) or
        g_bMLU and dropTrackCmd and not diddroptrack)//me123
    {
        reacqFlag = (int)(ReacqusitionCount / SEC_TO_MSEC * SimLibMajorFrameRate);
        reacqEl = lockedTargetData->el;
        //ClearHistory(lockedTarget);
        azScan = rwsAzs[rwsAzIdx];
        displayAzScan = rwsAzs[rwsAzIdx];
        bars = rwsBars[rwsBarIdx];
        barWidth = APG68_BAR_WIDTH;
        ClearFlagBit(VerticalScan);
        SetFlagBit(HorizontalScan);
        scanDir  = ScanFwd;
        SetScan();
        ClearFlagBit(STTingTarget);
        diddroptrack = TRUE;
        return;
    }

    diddroptrack = FALSE;

    if ( not oldseekerElCenter)
        oldseekerElCenter = seekerElCenter;//me123

    seekerAzCenter = max(min(lockedTargetData->az, MAX_ANT_EL), -MAX_ANT_EL);
    seekerElCenter = max(min(lockedTargetData->el, MAX_ANT_EL), -MAX_ANT_EL);
    bars = 1;
    azScan = 0.0F;

    /*--------------------------*/
    /* Hits to maintain a Track */
    /*--------------------------*/

    // MD -- 20031222: use a helper function
    totHits = HitsOnTrack(lockedTargetData);

    if (lockedTargetData->painted and totHits > HITS_FOR_TRACK)
    {
        AddToHistory(lockedTarget, Track);
        lockedTargetData->sensorState[Radar] = SensorTrack;
    }

    /*------------------*/
    /* Auto Range Scale */
    /*------------------*/
    if (mode not_eq VS)
    {
        if (lockedTargetData->range > 0.9F * tdisplayRange and curRangeIdx < NUM_RANGES - 1)
            rangeChangeCmd = 1;
        else if (lockedTargetData->range < 0.4F * tdisplayRange and curRangeIdx > 0)
            rangeChangeCmd = -1;
    }

    if (totHits < HITS_FOR_TRACK or dropTrackCmd)
    {
        ExtrapolateHistory(lockedTarget);

        if (((SimLibElapsedTime - lockedTarget->localData->rdrLastHit) > radarData->CoastTime) or dropTrackCmd)
        {
            //me123 rangeChangeCmd = 0;
            reacqFlag = (int)(ReacqusitionCount / SEC_TO_MSEC * SimLibMajorFrameRate);
            reacqEl = lockedTargetData->el;

            // MD -- 20040125: add new TWS mode processing: still show a bug after STT
            if (mode not_eq TWS)
                ClearHistory(lockedTarget);

            if (mode == ACM_30x20 or mode == ACM_SLEW
                or mode == ACM_BORE or mode == ACM_10x60 or mode == VS)
            {
                lockedTarget->localData->rdrDetect = 0;
                ClearSensorTarget();
                ChangeMode(mode);
            }
            else if (mode == SAM)
            {
                azScan = rwsAzs[rwsAzIdx];
                displayAzScan = rwsAzs[rwsAzIdx];
                bars = rwsBars[rwsBarIdx];
                barWidth = APG68_BAR_WIDTH;
                ClearFlagBit(VerticalScan);
                SetFlagBit(HorizontalScan);
                scanDir  = ScanFwd;
                //MI
                ChangeMode(prevMode);
            }
            else
            {
                float elhack = oldseekerElCenter;
                ChangeMode(prevMode);
                oldseekerElCenter = elhack;

                // MD -- 20031223: antenna should go back to where the knob was set for all but TWS with a bug.
                if ( not g_bAntElevKnobFix)
                    seekerElCenter = elhack;
            }

            SetScan();
            ClearFlagBit(STTingTarget);
        }
    }
}

void RadarDopplerClass::ACMMode(void)
{
    int totHits;
    SimObjectType* rdrObj = platform->targetList;

    /*----------*/
    /* Locked ? */
    /*----------*/
    //Cobra
    static int islck = FALSE;

    if (lockedTarget)
    {
        // RED - CTD Fix - Hud was already flushed on exit
        if (TheHud)
        {
            TheHud->HudData.Clear(HudDataType::RadarSlew);
            TheHud->HudData.Clear(HudDataType::RadarBoresight);
            TheHud->HudData.Clear(HudDataType::RadarVertical);
        }

        SetFlagBit(STTingTarget);
        STTMode();
        islck = TRUE;//Cobra
    } // ASSOCIATOR 03/12/03: Added IsEmitting() check so that OVRD to turn off radar doesn't relock in ACM
    else if (IsEmitting())
    {
        if (IsSet(STTingTarget))
        {
            ClearFlagBit(STTingTarget);
            ChangeMode(mode);
        }

        if (scanDir == ScanNone)
        {
            scanDir = ScanFwd;
        }

        if (mode == ACM_BORE)
        {
            seekerAzCenter = 0.0F;
            seekerElCenter = -3.0F * DTR;;
        }

        /*-------------------*/
        /* check all objects */
        /*-------------------*/
        while (rdrObj)
        {
            /*--------------------------*/
            /* Hits to maintain a Track */
            /*--------------------------*/

            // MD -- 20031222: use a helper function
            totHits = HitsOnTrack(rdrObj->localData);

            if (totHits > HITS_FOR_LOCK / 2 and 
                rdrObj->localData->range < tdisplayRange and IsEmitting() and 
                rdrObj->localData->painted
               )
            {
                // Play the lock message
                F4SoundFXSetDist(SFX_BB_LOCK, 0, 0.0f, 1.0f);
                MonoPrint("Lock-----------------------------");
                SetSensorTarget(rdrObj);
                ClearFlagBit(Designating);
                seekerAzCenter = lockedTargetData->az;
                seekerElCenter = lockedTargetData->el;
                beamAz   = 0.0F;
                beamEl   = 0.0F;
                ClearFlagBit(SpaceStabalized);
                beamWidth = radarData->BeamHalfAngle;
                azScan = 0.0F * DTR;
                bars = 1;
                barWidth = APG68_BAR_WIDTH;
                ClearFlagBit(VerticalScan);
                SetFlagBit(HorizontalScan);
                scanDir  = ScanNone;
                SetScan();
                patternTime = 1;
                SetFlagBit(STTingTarget);
                SimDriver.GetPlayerAircraft()->FCC->dropTrackCmd = FALSE;//Cobra 1/29/05 Needs this to reset lock
                break;
            }
            else
            {
                rdrObj->localData->sensorState[Radar] = NoTrack;
            }

            rdrObj = rdrObj->next;
        }

        // RED - CTD Fix - Hud was already flushed on exit
        if (TheHud)
        {
            if (mode == ACM_BORE)
            {
                TheHud->HudData.Set(HudDataType::RadarBoresight);
            }
            else if (mode == ACM_SLEW)
            {
                TheHud->HudData.Set(HudDataType::RadarSlew);
            }
            else if (mode == ACM_10x60)
            {
                TheHud->HudData.Set(HudDataType::RadarVertical);
            }
        }
    }


    //Cobra
    if ( not lockedTarget and islck)
    {
        ClearSensorTarget();
        ChangeMode(mode);
        SetScan();
        lockedTarget = NULL;
        islck = FALSE;
    }

    /*-------------------------*/// me123 so designate doesn't drop target if locked

    /* Select correct ACM Mode *///

    // and drop target command returns to search if we have a lock, if we dont have a lock  it changes
    /*-------------------------*/// to 20/30 from all acm modes exept in 20/30 it goes to 10/60

    if (designateCmd and not lockedTarget)
    {
        // don't drop the targer if it's locked me123
        ChangeMode(ACM_BORE);
        SetScan();
    }
    else if (
 not lockedTarget and mode not_eq ACM_SLEW and 
        SimDriver.GetPlayerAircraft() and // JB 010113 CTD fix
        (SimDriver.GetPlayerAircraft()->FCC->cursorYCmd not_eq 0 or
         SimDriver.GetPlayerAircraft()->FCC->cursorXCmd not_eq 0)
    )
    {
        ChangeMode(ACM_SLEW);
        SetScan();
    }
    // MD -- 20040111: Adding this condition here to remove it from the cursor key commands
    // which ended up being necessary to make the analog cursor support work the same way
    // as the previous implementation of the key commands which did this job.  Looks like
    // if should have been here all along really.
    else if (
        lockedTarget and mode not_eq ACM_SLEW and 
        SimDriver.GetPlayerAircraft() and 
        (SimDriver.GetPlayerAircraft()->FCC->cursorYCmd not_eq 0 or
         SimDriver.GetPlayerAircraft()->FCC->cursorXCmd not_eq 0)
    )
    {
        //ClearSensorTarget();  //JPG 28 - This no longer applies w/ one-switch TMS aft to ACM NO RAD condition
        ChangeMode(ACM_SLEW);
        SetScan();
        //lockedTarget = NULL;  //JPG This too
    }
    else if (dropTrackCmd)
    {
        if (lockedTarget)
        {
            ClearSensorTarget();  // me123 brake lock if locked
            ChangeMode(mode);
            SetScan();
            lockedTarget = NULL;//me123
            SetEmitting(FALSE);
        }
        else
        {
            SetEmitting(TRUE);
        }
    }

    if (mode == ACM_10x60)
    {
        SetEmitting(TRUE);
    }

    /*else if (didDroptargetCmd)
    {
     SetEmitting(FALSE);
    }
    else {
     didDroptargetCmd = FALSE;
    } */
}

void RadarDopplerClass::VSMode(void)
{
    int totHits;
    float tmpRange;
    SimObjectType* rdrObj;
    SimObjectLocalData* rdrData;

    if (lockedTarget)
    {
        STTMode();
    }
    else
    {
        /*-----------------------*/
        /* Spotlight / Designate */
        /*-----------------------*/
        if (IsSet(Spotlight) and not designateCmd)
            SetFlagBit(Designating);
        else
            ClearFlagBit(Designating);

        if ( not IsSet(Spotlight) and designateCmd)
        {
            lastAzScan = azScan;
            lastBars = bars;
            azScan = 10.0F * DTR;
            bars = 4;
            ClearSensorTarget();
        }

        if (designateCmd)
            SetFlagBit(Spotlight);
        else
            ClearFlagBit(Spotlight);

        /*-------------------*/
        /* check all objects */
        /*-------------------*/
        rdrObj = platform->targetList;

        while (rdrObj)
        {
            rdrData = rdrObj->localData;

            /*-------------------------------*/
            /* check for object in radar FOV */
            /*-------------------------------*/
            if (rdrData->painted)
            {
                /*----------------------*/
                /* detection this frame */
                /*----------------------*/
                if (rdrData->rdrDetect bitand 0x10 and rdrData->rangedot < 0.0F)
                {
                    tmpRange = rdrData->range;
                    rdrData->range = -rdrData->rangedot * FTPSEC_TO_KNOTS;
                    SetHistory(rdrObj, Det);
                    rdrData->range = tmpRange;
                }
                /*--------------*/
                /* no detection */
                /*--------------*/
                else
                    AddToHistory(rdrObj, None);
            }

            /*--------------------------------*/
            /* If designating, check for lock */
            /*--------------------------------*/
            if (IsSet(Designating))
            {
                // MD -- 20031222: use a helper function
                totHits = HitsOnTrack(rdrData);

                if (totHits >= HITS_FOR_LOCK and 
                    IsUnderVSCursor(rdrObj, platform->Yaw()))
                {
                    SetSensorTarget(rdrObj);
                    ClearFlagBit(Designating);
                    seekerAzCenter = lockedTargetData->az;
                    seekerElCenter = lockedTargetData->el;
                    beamAz   = 0.0F;
                    beamEl   = 0.0F;
                    //       ClearFlagBit (SpaceStabalized);
                    beamWidth = radarData->BeamHalfAngle;
                    azScan = 0.0F * DTR;
                    bars = 1;
                    barWidth = APG68_BAR_WIDTH;
                    ClearFlagBit(VerticalScan);
                    SetFlagBit(HorizontalScan);
                    scanDir  = ScanNone;
                    SetScan();
                    patternTime = 1;
                    SetFlagBit(STTingTarget);
                    break;
                }
            }

            rdrObj = rdrObj->next;
        }

        /*-----------------------------------*/
        /* No target found, return to search */
        /*-----------------------------------*/
        if (IsSet(Designating))
        {
            azScan = lastAzScan;
            bars = lastBars;
        }
    }
}

void RadarDopplerClass::AddToHistory(SimObjectType* ptr, int sy)
{
    int i;
    SimObjectLocalData* rdrData;

    F4Assert(ptr);
    rdrData = ptr->localData;

    rdrData->aspect = 180.0F * DTR - rdrData->ataFrom;      /* target aspect  */

    if (rdrData->aspect > 180.0F * DTR)
        rdrData->aspect -= 360.0F * DTR;

    for (i = NUM_RADAR_HISTORY - 1; i > 0; i--)
    {
        rdrData->rdrX[i]  = rdrData->rdrX[i - 1];
        rdrData->rdrY[i]  = rdrData->rdrY[i - 1];
        rdrData->rdrHd[i] = rdrData->rdrHd[i - 1];
        rdrData->rdrSy[i] = rdrData->rdrSy[i - 1];
    }

    rdrData->rdrX[0]  = TargetAz(platform, ptr);
    rdrData->rdrY[0]  = rdrData->range;

    // if its jamming and we can't burn through - its a guess where it is.
    if (ptr->BaseData()->IsSPJamming() and ReturnStrength(ptr) < 1.0f)
    {
        float delta = rdrData->range / 10.0f; // range may be out by up to 1/10th
        rdrData->rdrY[0] += delta * PRANDFloat(); // +/- the delta
        rdrData->rdrSy[0] = 1;//Cobra let's make you only Detected no burn through
    }
    else //added else for sy
        rdrData->rdrSy[0] = sy;//Cobra added this here

    rdrData->rdrHd[0] = platform->Yaw();
    //rdrData->rdrSy[0] = sy; Cobra removed from here and moved up

    if (sy not_eq None)
    {
        rdrData->rdrLastHit  = SimLibElapsedTime;
        //      UpdateObjectData(ptr);
    }
}

void RadarDopplerClass::ClearHistory(SimObjectType* ptr, BOOL clrDetect)
{
    int i;
    SimObjectLocalData* rdrData = ptr->localData;

    rdrData->aspect = 0.0F;

    if (clrDetect)
        rdrData->rdrDetect = 0;

    for (i = 0; i < NUM_RADAR_HISTORY; i++)
    {
        rdrData->rdrX[i]  = 0.0F;
        rdrData->rdrY[i]  = 0.0F;
        rdrData->rdrHd[i] = 0.0F;
        rdrData->rdrSy[i] = 0;
    }
}

void RadarDopplerClass::SlipHistory(SimObjectType* ptr)
{
    int i;
    SimObjectLocalData* rdrData = ptr->localData;

    for (i = NUM_RADAR_HISTORY - 1; i > 0; i--)
    {
        rdrData->rdrX[i]  = rdrData->rdrX[i - 1];
        rdrData->rdrY[i]  = rdrData->rdrY[i - 1];
        rdrData->rdrHd[i] = rdrData->rdrHd[i - 1];
        rdrData->rdrSy[i] = rdrData->rdrSy[i - 1];
    }

    rdrData->rdrX[0]  = 0.0F;
    rdrData->rdrY[0]  = 0.0F;
    rdrData->rdrHd[0] = 0.0F;
    rdrData->rdrSy[0] = 0;
}

void RadarDopplerClass::ExtrapolateHistory(SimObjectType* ptr)
{
    int i;
    SimObjectLocalData* rdrData = ptr->localData;

    for (i = NUM_RADAR_HISTORY - 1; i > 0; i--)
    {
        rdrData->rdrX[i]  = rdrData->rdrX[i - 1];
        rdrData->rdrY[i]  = rdrData->rdrY[i - 1];
        rdrData->rdrHd[i] = rdrData->rdrHd[i - 1];
        rdrData->rdrSy[i] = rdrData->rdrSy[i - 1];
    }

    // Change range
    rdrData->rdrY[0]  += (rdrData->rdrY[1] - rdrData->rdrY[2]);

    if (SimLibElapsedTime < (rdrData->extrapolateStart + TwsExtrapolateTime))  // MD -- 20040121: use extrapolation timer
    {
        if (rdrData->rdrSy[0] == Bug)
            rdrData->rdrSy[0] = FlashBug;
        else if (rdrData->rdrSy[0] == Track)
            rdrData->rdrSy[0] = FlashTrack;
    }
}

void RadarDopplerClass::ClearAllHistory(void)
{
    SimObjectType* rdrObj = platform->targetList;

    while (rdrObj)
    {
        ClearHistory(rdrObj, TRUE);
        rdrObj = rdrObj->next;
    }
}

void RadarDopplerClass::SetHistory(SimObjectType* ptr, int sy)
{
    int i;
    SimObjectLocalData* rdrData = ptr->localData;

    rdrData->aspect = 180.0F * DTR - rdrData->ataFrom;      /* target aspect  */

    if (rdrData->aspect > 180.0F * DTR)
        rdrData->aspect -= 360.0F * DTR;

    for (i = NUM_RADAR_HISTORY - 1; i > 0; i--)
    {
        rdrData->rdrX[i]  = 0.0F;
        rdrData->rdrY[i]  = 0.0F;
        rdrData->rdrHd[i] = 0.0F;
        rdrData->rdrSy[i] = 0;
    }

    rdrData->rdrX[0]  = TargetAz(platform, ptr);
    rdrData->rdrY[0]  = rdrData->range;
    rdrData->rdrHd[0] = platform->Yaw();
    rdrData->rdrSy[0] = sy;

    if (sy not_eq None)
    {
        //   UpdateObjectData(ptr);
    }
}

// MD -- 20031222: turned this into a helper function since the same fragment appears
// in most of the AA mode functions.

int RadarDopplerClass::HitsOnTrack(SimObjectLocalData* rdrData)
{
    int totHits = 0, i = 0;
    unsigned long detect = rdrData->rdrDetect;

    if ( not rdrData)
        return 0;

    for (i = 0; i < 5; i++)
    {
        totHits += detect bitand 0x0001;
        detect = detect >> 1;
    }

    return totHits;
}

// MD -- 20040116: adding class implementation for the TWS track file directory
// This is a range ordered list of the track files that are doppler correlated
// and that TWS mode will focus on.  The length of the list is automatically
// limited to the max number of tracks that the TWS can handle.

RadarDopplerClass::TWSTrackList::TWSTrackList(SimObjectType* tgt)
{
    F4Assert(tgt)
    track = tgt;
    count = 1;
    nextTrack = NULL;
    track->Reference();
    track->localData->TWSTrackFileOpen = TRUE;
}

RadarDopplerClass::TWSTrackList* RadarDopplerClass::TWSTrackList::Insert(SimObjectType* tgt, int depth)
{

    if ( not (depth < MAX_TWS_TRACKS))  // keep the list from growing needlessly
        return NULL;

    if (tgt not_eq track)
    {
        if (tgt->localData->range < track->localData->range)
        {
            TWSTrackList* tmp = new TWSTrackList(tgt);
            tmp->SetNext(this);
            Clip(depth + 1); // make sure the directory is limited to 10 entries when inserting in the middle
            return tmp;
        }
        else if (nextTrack == NULL)
            nextTrack = new TWSTrackList(tgt);
        else
            nextTrack = nextTrack->Insert(tgt, depth + 1);
    }

    return this;
}

RadarDopplerClass::TWSTrackList* RadarDopplerClass::TWSTrackList::ForceInsert(SimObjectType* tgt, int depth)
{
    // This function does the same as an insert but first it checks to see if there's room in
    // the directory and if not, it will remove the longest ranged track file.

    if (this and ((CountTracks() - depth) >= MAX_TWS_TRACKS))
    {
        TWSTrackList* tmp = this, *last = (TWSTrackList *) NULL;

        for (int i = 1; ((i < (MAX_TWS_TRACKS - depth)) and tmp); i++)
        {
            last = tmp;
            tmp = tmp->Next();
        }

        if (tmp)
        {
            tmp->Purge();

            if (last)
                last->SetNext((TWSTrackList *) NULL);
        }
    }

    return Insert(tgt, depth);
}

RadarDopplerClass::TWSTrackList* RadarDopplerClass::TWSTrackList::Remove(SimObjectType* tgt)
{
    if (tgt == track)
    {
        TWSTrackList* tmp = nextTrack;
        Release();
        return tmp;
    }
    else if (nextTrack)
        nextTrack = nextTrack->Remove(tgt);

    return this;
}

RadarDopplerClass::TWSTrackList* RadarDopplerClass::TWSTrackList::OnList(SimObjectType* tgt)
{
    TWSTrackList* tmp = this;

    while (tmp)
    {
        if (tmp->TrackFile() == tgt)
            return tmp;

        tmp = tmp->Next();
    }

    return NULL;
}

int RadarDopplerClass::TWSTrackList::CountTracks(void)
{
    int i = 1;

    TWSTrackList* tmp = this;

    while (tmp)
    {
        if (tmp->Next())
        {
            tmp = tmp->Next();
            i++;
        }
        else
            break;
    }

    return i;
}

RadarDopplerClass::TWSTrackList* RadarDopplerClass::TWSTrackList::Purge()
{
    if (nextTrack)
        nextTrack->Purge();

    Release();

    return (TWSTrackList*) NULL;
}

void RadarDopplerClass::TWSTrackList::Clip(int depth)
{
    TWSTrackList* tmp = this;

    while (tmp)
    {
        depth++;

        if ((depth == MAX_TWS_TRACKS) and tmp->Next())
        {
            tmp->Next()->Purge();
            tmp->SetNext((TWSTrackList *)NULL);
        }

        tmp = tmp->Next();
    }
}

void RadarDopplerClass::TWSTrackList::Release(void)
{
    F4Assert(track);
    track->localData->TWSTrackFileOpen = FALSE;
    track->Release();
    count--;

    if (count == 0)
        delete this;
}

RadarDopplerClass::TWSTrackList* RadarDopplerClass::UpdateTWSDirectory(SimObjectType* tgtList, RadarDopplerClass::TWSTrackList* directory)
{
    SimObjectType* rdrObj = tgtList;

    if (tgtList)
    {
        // Don't waste the time to check if the directory is already full
        if ( not directory or (directory and (directory->CountTracks() < MAX_TWS_TRACKS)))
        {
            while (rdrObj)
            {
                SimObjectLocalData* rdrData = rdrObj->localData;

                // pick up directory entries for targets with multiple hits that are in the scan cone
                if ((HitsOnTrack(rdrData) > HITS_FOR_LOCK) and 
                    ((fabs(rdrData->az) < radarData->ScanHalfAngle) and 
                     (fabs(rdrData->el) < radarData->ScanHalfAngle)))
                {
                    if ( not rdrData->TWSTrackFileOpen) // insert new tracks only
                        if (directory)
                            directory = directory->Insert(rdrObj);
                        else
                            directory = new TWSTrackList(rdrObj);
                }

                rdrObj = rdrObj->next;
            }
        }
    }

    return directory;
}
