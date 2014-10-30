#include "Graphics/Include/TOD.h"
#include "Graphics/Include/RenderOW.h"
#include "Graphics/Include/RenderNVG.h"
#include "Graphics/Include/Canvas3D.h"
#include "Graphics/Include/RViewPnt.h"
#include "Graphics/Include/Drawbsp.h"
#include "Graphics/Include/Drawpnt.h"
#include "stdhdr.h"
#include "otwdrive.h"
#include "hud.h"
#include "mfd.h"
#include "cpmanager.h"
#include "simdrive.h"
#include "camplib.h"
#include "resource.h"
#include "simbase.h"
#include "simWeapn.h"
#include "sfx.h"
#include "playerop.h"
#include "FalcSess.h"
#include "aircrft.h"
#include "camp2sim.h"
#include "fakerand.h"
#include "object.h"
#include "mesg.h"
#include "playerOp.h"
#include "sinput.h"
#include "campbase.h"
// KCK: These are only for detail level shit.
#include "SimFeat.h"
#include "Feature.h"
#include "fsound.h"
#include "vdial.h"

extern HWND mainMenuWnd;
extern int CommandsKeyCombo;
extern int CommandsKeyComboMod;
extern unsigned int chatterCount;
extern char chatterStr[256];
extern SimBaseClass* eyeFlyTgt;
extern int gUseAlpha;
extern int eyeFlyEnabled;
extern int narrowFOV;
extern bool g_bGreyMFD;
extern bool bNVGmode;

extern float g_f3DHeadTilt; // Cobra
extern float g_f3DPitFOV; // Cobra
extern bool g_bSync2D3DPit; //Wombat778 11-17-04
extern bool g_bEnableTrackIR; //Wombat778 11-18-04

#ifdef DAVE_DBG
extern float boomAzTest, boomElTest, boomExtTest;
extern int MoveBoom;
#endif

#include "IVibeData.h"
extern IntellivibeData g_intellivibeData;

#include "Graphics/DXEngine/DXEngine.h"
extern bool g_bUse_DX_Engine;

void OTWDriverClass::ToggleSidebar(void)
{
    if (mDoSidebar)
    {
        mDoSidebar = FALSE;
    }
    else
    {
        mDoSidebar = TRUE;

        if (pPadlockCPManager)
            pPadlockCPManager->SetDefaultPanel(PADLOCK_DEFAULT_PANEL);

    }
}


void OTWDriverClass::ViewStepNext(void)
{
    viewStep = 1;
}

void OTWDriverClass::ViewStepPrev(void)
{
    viewStep = -1;
}

void OTWDriverClass::TargetStepNext(void)
{
    tgtStep = 1;
    // these keys are now used for stepping targets in the views....
    SetOTWDisplayMode(mOTWDisplayMode);
    // need to reset after the above call
    tgtStep = 1;
}

void OTWDriverClass::TargetStepPrev(void)
{
    tgtStep = -1;
    // these keys are now used for stepping targets in the views....
    SetOTWDisplayMode(mOTWDisplayMode);
    // need to reset after the above call
    tgtStep = -1;
}

void OTWDriverClass::NVGToggle(void)
{
    // RenderOTW *newRenderer;
    ShiAssert(renderer);

    TheTimeOfDay.SetNVGmode( not TheTimeOfDay.GetNVGmode());

    // Construct a new renderer of the appropriate type

    //JAM 12Oct03, sfr fixed stuff
    if (TheTimeOfDay.GetNVGmode())
    {
        // newRenderer = new RenderNVG;
        renderer->context.SetNVGmode(TRUE);
        //TheDXEngine.SetState(DX_NVG);
        renderer->SetGreenMode(true); //sfr
        bNVGmode = true;
    }
    else
    {
        // newRenderer = new RenderOTW;
        renderer->context.SetNVGmode(FALSE);
        //TheDXEngine.SetState(DX_OTW);
        renderer->SetGreenMode(false); //sfr
        bNVGmode = false;
    }

    //JAM 12Oct03

    //renderer->context.FinishFrame(NULL);

    /* ShiAssert( newRenderer );

     // Setup the new renderer
     newRenderer->Setup( OTWImage, viewPoint );

     // Copy the previous renderer's settings
     newRenderer->SetFOV( renderer->GetFOV() );
    // newRenderer->SetTerrainTextureLevel( renderer->GetTerrainTextureLevel() );
     newRenderer->SetHazeMode( renderer->GetHazeMode() );
    // newRenderer->SetAlphaMode( TRUE /*renderer->GetAlphaMode()*//* );
// newRenderer->SetRoofMode( renderer->GetRoofMode() );
// newRenderer->SetSmoothShadingMode( TRUE /*renderer->GetSmoothShadingMode()*//* );
// newRenderer->SetDitheringMode( renderer->GetDitheringMode() );
// newRenderer->SetFilteringMode( renderer->GetFilteringMode() );
// Now swap in the new renderer
/* renderer->Cleanup();
delete renderer;
renderer = newRenderer;
renderer->context.StartFrame();
renderer->StartDraw();*/
    //sfr: update 2D cockpit
    pCockpitManager->UpdatePalette();
    DrawableClass::SetGreenMode(renderer->GetGreenMode());
}

void OTWDriverClass::IDTagToggle(void)
{
    if (PlayerOptions.NameTagsOn())
    {
        if (DrawableBSP::drawLabels)
        {
            DrawableBSP::drawLabels = FALSE;
            // DrawablePoint::drawLabels = FALSE;
        }
        else
        {
            DrawableBSP::drawLabels = TRUE;
        }
    }
}

void OTWDriverClass::CampTagToggle(void)
{
    if (PlayerOptions.NameTagsOn())
    {
        if (DrawablePoint::drawLabels)
        {
            DrawablePoint::drawLabels = FALSE;
        }
        else
        {
            DrawablePoint::drawLabels = TRUE;
            // DrawableBSP::drawLabels = TRUE;
        }
    }
}

/////////////


void OTWDriverClass::SelectF3PadlockMode()
{
    if (pPadlockCPManager)
        pPadlockCPManager->SetDefaultPanel(PADLOCK_DEFAULT_PANEL);
}

////////////////////////
////////////////////////
////////////////////////
void OTWDriverClass::Select2DCockpitMode(bool viewreset) //Wombat778 11-18-04  added viewreset argument
{
    if (pCockpitManager)
    {
        //MI
        if (SimDriver.GetPlayerAircraft() and SimDriver.GetPlayerAircraft()->WideView)
            pCockpitManager->SetDefaultPanel(COCKPIT_DEFAULT_PANEL + 90000);
        else
            pCockpitManager->SetDefaultPanel(COCKPIT_DEFAULT_PANEL);

        //Wombat778 11-17-04
        if (g_bSync2D3DPit and not viewreset)
            pCockpitManager->Set2DPanelDirection(eyePan, eyeTilt);
    }
}

void OTWDriverClass::Cleanup2DCockpitMode()
{
    if (pCockpitManager)
        pCockpitManager->SetActivePanel(PANELS_INACTIVE);

    // MfdDisplay[0]->SetImageBuffer(NULL, -1.0F, 1.0F, 1.0F, -1.0F);
    // MfdDisplay[1]->SetImageBuffer(NULL, -1.0F, 1.0F, 1.0F, -1.0F);
    MfdDisplay[0]->SetImageBuffer(OTWImage, 0.0F, 0.0F, 0.0F, 0.0F);
    MfdDisplay[1]->SetImageBuffer(OTWImage, 0.0F, 0.0F, 0.0F, 0.0F);

    //Wombat778 4-12-04 Cleanup 3 and 4 MFDs too
    MfdDisplay[2]->SetImageBuffer(OTWImage, 0.0F, 0.0F, 0.0F, 0.0F);
    MfdDisplay[3]->SetImageBuffer(OTWImage, 0.0F, 0.0F, 0.0F, 0.0F);
}

////////////////////////
////////////////////////
////////////////////////
void OTWDriverClass::Select3DCockpitMode()
{
    if ( not g_bSync2D3DPit)
    {
        ViewReset();
        eyePan = 0.0F;
        eyeTilt = g_f3DHeadTilt * DTR; // Cobra
    }

    SetFOV(g_f3DPitFOV * DTR);   // Cobra
}


////////////////////////
////////////////////////
////////////////////////
void OTWDriverClass::SelectExternal()
{

    float dx, dy, dz;

    // reset our chase ranges etc to default or zero out slew stuff
    ViewReset();

    switch (mOTWDisplayMode)
    {
        case ModeFlyby:
            // end in X seconds
            //flybyTimer = SimLibElapsedTime + 3500;
            flybyTimer = SimLibElapsedTime + 6000;  // MLR 12/3/2003 - Made fly by timer 6 seconds

            if (otwPlatform)
            {
                if (otwPlatform->IsEject())
                {
                    flybyTimer = SimLibElapsedTime + 3000;
                    dx = otwPlatform->XDelta() * 1.0f;
                    dy = otwPlatform->YDelta() * 1.0f;
                    dz = otwPlatform->ZDelta() * 1.0f;
                }
                else if (otwPlatform->OnGround())
                {
                    dx = otwPlatform->dmx[0][0] * 10.0f + otwPlatform->XDelta() * 2.0f + PRANDFloat() * 100.0f;
                    dy = otwPlatform->dmx[0][1] * 10.0f + otwPlatform->YDelta() * 2.0f + PRANDFloat() * 100.0f;
                    dz = otwPlatform->dmx[0][2] * 10.0f + otwPlatform->ZDelta() * 2.0f - PRANDFloatPos() * 200.0f;
                }
                else
                {
                    dx = otwPlatform->dmx[0][0] * 10.0f + otwPlatform->XDelta() * 2.0f + PRANDFloat() * 100.0f;
                    dy = otwPlatform->dmx[0][1] * 10.0f + otwPlatform->YDelta() * 2.0f + PRANDFloat() * 100.0f;
                    dz = otwPlatform->dmx[0][2] * 10.0f + otwPlatform->ZDelta() * 2.0f + PRANDFloat() * 100.0f;
                }

                // edg: I think this may be where we're getting bad values
                // for camera position
                if (fabs(dx) > 15000.0f or fabs(dy) > 15000.0f or fabs(dz) > 15000.0f)
                {
                    endFlightPoint.x = otwPlatform->XPos();
                    endFlightPoint.y = otwPlatform->YPos();
                    endFlightPoint.z = otwPlatform->ZPos();
                }
                else
                {
                    endFlightPoint.x = otwPlatform->XPos() + dx;
                    endFlightPoint.y = otwPlatform->YPos() + dy;
                    endFlightPoint.z = otwPlatform->ZPos() + dz;
                }

                if (rand() bitand 1)
                {
                    endFlightVec.x = PRANDFloat();
                    endFlightVec.y = PRANDFloat();

                    if (otwPlatform->OnGround())
                        endFlightVec.z = PRANDFloat();
                    else
                        endFlightVec.z = -PRANDFloatPos();
                }
                else
                {
                    endFlightVec.x = 0.0f;
                    endFlightVec.y = 0.0f;
                    endFlightVec.z = 0.0f;
                }
            }
            else
            {
                endFlightPoint = focusPoint;
                endFlightVec.x = PRANDFloat();
                endFlightVec.y = PRANDFloat();
                endFlightVec.z = -PRANDFloatPos();
            }

            mOTWDisplayMode = ModeChase;
            break;

        case ModeSatellite:
            if (otwPlatform)
            {
                chaseCamPos.z = otwPlatform->ZPos() + chaseRange * 12.0f;
            }
            else
            {

                chaseCamPos.z = chaseRange * 20.0f;
            }

            chaseCamPos.x = 0.0f;
            chaseCamPos.y = 0.0f;
            chaseCamRoll = 0.0f;
            break;

        case ModeWeapon:

            if (otwPlatform)
            {

                chaseCamRoll = 0.0f;
                chaseCamPos.x = otwPlatform->dmx[0][0] * chaseRange * 0.5f;
                chaseCamPos.y = otwPlatform->dmx[0][1] * chaseRange * 0.5f;
                chaseCamPos.z = otwPlatform->dmx[0][2] * chaseRange * 0.5f - 5.0f;
            }
            else
            {

                chaseCamRoll = 0.0f;
                chaseCamPos.x = 0.0f;
                chaseCamPos.y = 0.0f;
                chaseCamPos.z = -20.0f;
            }

            break;

        default:

            if (otwPlatform)
            {

                chaseCamRoll = otwPlatform->Roll();
                chaseCamPos.x = otwPlatform->dmx[0][0] * chaseRange;
                chaseCamPos.y = otwPlatform->dmx[0][1] * chaseRange;
                chaseCamPos.z = otwPlatform->dmx[0][2] * chaseRange - 5.0f;
            }
            else
            {

                chaseCamRoll = 0.0f;
                chaseCamPos.x = 0.0f;
                chaseCamPos.y = 0.0f;
                chaseCamPos.z = -500.0f;
            }

            break;

    }

    cameraPos = chaseCamPos;

    // mix it up a bit for action camera
    if (actionCameraMode == TRUE)
    {
        cameraPos.x += PRANDFloat() * 700.0f;
        cameraPos.y += PRANDFloat() * 700.0f;
        chaseCamPos.x += PRANDFloat() * 700.0f;
        chaseCamPos.y += PRANDFloat() * 700.0f;

        if (otwPlatform and otwPlatform->drawPointer)
        {
            chaseRange = -1.5F * otwPlatform->drawPointer->Radius();
            chaseRange += -PRANDFloatPos() * 400.0f;

            if (otwPlatform->OnGround())
            {
                cameraPos.z -= PRANDFloatPos() * 400.0f;
                chaseCamPos.z -= PRANDFloatPos() * 400.0f;
                return;
            }
        }

        cameraPos.z += PRANDFloat() * 700.0f;
        chaseCamPos.z += PRANDFloat() * 700.0f;

    }
}


OTWDriverClass::OTWDisplayMode OTWDriverClass::GetOTWDisplayMode(void)
{

    return mOTWDisplayMode;
}

/*
** Turn on/off the automatic action camera.
*/
void OTWDriverClass::ToggleActionCamera(void)
{
    if (actionCameraMode == FALSE)
    {
        // 1st go into chase mode
        SetOTWDisplayMode(ModeChase);

        // turn on
        actionCameraMode = TRUE;

        // set the timer
        actionCameraTimer = vuxRealTime;
    }
    else
    {
        // turn off
        actionCameraMode = FALSE;
    }
}

//Wombat778 11-18-04 Toggle the hybrid pit mode on or off

void OTWDriverClass::ToggleHybridPitMode(void)
{

    if ( not HybridPitModeEnabled)
    {
        if (g_bSync2D3DPit)
            if (g_bEnableTrackIR and PlayerOptions.Get3dTrackIR() == true) //added check for 3d TIR being enabled
                HybridPitModeEnabled = 1;
            else
                HybridPitModeEnabled = 2;
    }
    else
        HybridPitModeEnabled = 0;
}

int OTWDriverClass::GetHybridPitMode()
{
    return HybridPitModeEnabled;
}

//Wombat778 END

void OTWDriverClass::SetOTWDisplayMode(OTWDisplayMode mode)
{
    SimBaseClass *newObject;
    //SimObjectType *targetPtr;

    // if in action camera mode shut it off
    if (actionCameraMode)
    {
        actionCameraMode = FALSE;
    }

    // reset our chase ranges etc to default or zero out slew stuff
    // if( not (mode == mOTWDisplayMode and mode == ModePadlockF3)) {
    // ViewReset();
    // }

    /*
    if((mode not_eq ModePadlockF3) and 
       (mode not_eq ModePadlockEFOV) and 
       (mode not_eq ModeTarget) and 
       (mode not_eq ModeTargetToSelf) )
    {
     if(mpPadlockPriorityObject) {
     VuDeReferenceEntity(mpPadlockPriorityObject);
     mpPadlockPriorityObject = NULL;
     }
     mpPadlockCandidate = NULL;
    }
    */
    if ((mode not_eq ModePadlockF3) and 
        (mode not_eq ModePadlockEFOV))
    {
        /* 2001-01-29 MODIFIED BY S.G. FOR THE NEW mpPadlockPrioritySimObject
         if(mpPadlockPriorityObject) {
         VuDeReferenceEntity(mpPadlockPriorityObject);
         mpPadlockPriorityObject = NULL;
        */ SetmpPadlockPriorityObject(NULL);
        // }
        mpPadlockCandidate = NULL;
        mPadlockCandidateID  = FalconNullId;
    }

    if (mode == mOTWDisplayMode)
    {
        // we're in the same mode, some modes will require processing
        switch (mode)
        {
            case ModeWeapon:

                // try and find a flying weapon for the current otwPlatform's parent
                // since we assume the current otwplatform is a weapon
                ShiAssert(otwPlatform->IsWeapon());
                newObject = FindNextViewObject(((SimWeaponClass*)otwPlatform.get())->Parent(), otwPlatform.get(), NEXT_WEAPON);

                // At the moment, no weapons flying, no mode change
                if (newObject == NULL)
                    return;

                // set graphics focus to the weapon
                SetGraphicsOwnship(newObject);

                break;

            case ModeTargetToWeapon:

                // SCR 12/3/98  Hmm.  How did this happen?  Lets just get on with life (and hope we live)
                if (otwTrackPlatform.get() == NULL)
                {
                    return;
                }

                // try and find a flying weapon for the current otwTrackPlatform's parent
                // since we assume the current otwTrackplatform is a weapon
                ShiAssert(otwTrackPlatform->IsWeapon());
                newObject = FindNextViewObject(
                                ((SimWeaponClass*)otwTrackPlatform.get())->Parent(), otwTrackPlatform.get(), NEXT_WEAPON);

                // At the moment, no weapons flying, no mode change
                if (newObject == NULL)
                    return;

                // set graphics focus to the target, tracking to the weapon
                SetGraphicsOwnship(otwTrackPlatform.get());
                SetTrackPlatform(newObject);

                break;

            case ModeAirFriendly:

                // try and find a flying weapon for the current otwPlatform
                newObject = FindNextViewObject(SimDriver.GetPlayerAircraft(), otwPlatform.get(), NEXT_AIR_FRIEND);

                // At the moment, no weapons flying, no mode change
                if (newObject == NULL)
                    return;

                // set graphics focus to the object
                SetGraphicsOwnship(newObject);

                break;

            case ModeAirEnemy:

                // try and find a flying weapon for the current otwPlatform
                newObject = FindNextViewObject(SimDriver.GetPlayerAircraft(), otwPlatform.get(), NEXT_AIR_ENEMY);

                // At the moment, no weapons flying, no mode change
                if (newObject == NULL)
                    return;

                // set graphics focus to the object
                SetGraphicsOwnship(newObject);

                break;

            case ModeGroundFriendly:

                // try and find a flying weapon for the current otwPlatform
                newObject = FindNextViewObject(SimDriver.GetPlayerAircraft(), otwPlatform.get(), NEXT_GROUND_FRIEND);

                // At the moment, no weapons flying, no mode change
                if (newObject == NULL)
                    return;

                // set graphics focus to the object
                SetGraphicsOwnship(newObject);

                break;

            case ModeGroundEnemy:

                // try and find a flying weapon for the current otwPlatform
                newObject = FindNextViewObject(SimDriver.GetPlayerAircraft(), otwPlatform.get(), NEXT_GROUND_ENEMY);

                // At the moment, no weapons flying, no mode change
                if (newObject == NULL)
                    return;

                // set graphics focus to the object
                SetGraphicsOwnship(newObject);

                break;

                // edg note: this is now actually self to any enemny
            case ModeTarget:

                if ( not SimDriver.GetPlayerAircraft() or otwTrackPlatform.get() == NULL)
                    return;

                // try and find a flying weapon for the current otwPlatform
                newObject = FindNextViewObject(SimDriver.GetPlayerAircraft(), otwTrackPlatform.get(), NEXT_ENEMY);    // 2002-02-16 MODIFIED BY S.G. Uses the new NEXT_ENEMY mode instead of NEXT_GROUND_ENEMY

                // At the moment, no bject, no mode change
                if (newObject == NULL)
                    return;

                // set graphics focus to the object
                SetGraphicsOwnship(SimDriver.GetPlayerAircraft());
                SetTrackPlatform(newObject);

                // we simply swap track with otw platform
                /*

                // ownship is always the focus
                SetGraphicsOwnship( SimDriver.GetPlayerAircraft() );

                // use the enhanced padlock priority to cycle thru appro-
                // priate objects
                Padlock_FindEnhancedPriority( TRUE );

                // if we got a candidate we use that
                if ( mpPadlockCandidate )
                {
                 if ( mpPadlockPriorityObject and 
                  mpPadlockPriorityObject not_eq mpPadlockCandidate )
                 {
                 VuDeReferenceEntity(mpPadlockPriorityObject);
                 mpPadlockPriorityObject = (SimBaseClass*) mpPadlockCandidate;
                 VuReferenceEntity(mpPadlockPriorityObject);
                 }
                 else if ( not mpPadlockPriorityObject )
                 {
                 mpPadlockPriorityObject = (SimBaseClass*) mpPadlockCandidate;
                 VuReferenceEntity(mpPadlockPriorityObject);
                 }
                 mpPadlockCandidate = NULL;
                }

                if ( mpPadlockPriorityObject )
                {
                 SetTrackPlatform( mpPadlockPriorityObject );
                }
                if ( tgtStep == 0 )
                {
                 newObject = otwTrackPlatform;

                 SetTrackPlatform( otwPlatform );
                 SetGraphicsOwnship( newObject );

                 chaseRange *= 2.5f;
                }
                else
                {
                 newObject = FindNextViewObject( otwPlatform, otwTrackPlatform, NEXT_AIR_ENEMY  );
                 if ( newObject )
                 {
                 SetTrackPlatform( newObject );
                 chaseRange *= 2.5f;
                 }
                }
                */


                /*
                // get the target (if any) of the current platform
                targetPtr = (( SimMoverClass * )otwPlatform)->targetPtr;

                // any target?
                if ( targetPtr == NULL or targetPtr->BaseData() == NULL or not targetPtr->BaseData()->IsSim())
                 return;

                // set graphics focus to the target
                SetGraphicsOwnship( (SimBaseClass*)targetPtr->BaseData() );
                */

                break;

            case ModeTargetToSelf:

                if ( not SimDriver.GetPlayerAircraft())
                    return;

                // try and find a flying weapon for the current otwPlatform
                newObject = FindNextViewObject(SimDriver.GetPlayerAircraft(), otwPlatform.get(), NEXT_ENEMY);    // 2002-02-16 MODIFIED BY S.G. Uses the new NEXT_ENEMY mode instead of NEXT_GROUND_ENEMY

                // At the moment, no bject, no mode change
                if (newObject == NULL)
                    return;

                // set graphics focus to the object
                SetGraphicsOwnship(newObject);
                SetTrackPlatform(SimDriver.GetPlayerAircraft());

                /*
                ** old stuff based on padlock

                // we need to have the player be otwPlatform for
                // the Padlock_FindEnhancedPriority
                SetGraphicsOwnship( SimDriver.GetPlayerAircraft() );

                // use the enhanced padlock priority to cycle thru appro-
                // priate objects
                Padlock_FindEnhancedPriority( TRUE );

                // if we got a candidate we use that
                if ( mpPadlockCandidate and mpPadlockCandidate->IsSim() )
                {
                 if ( mpPadlockPriorityObject and 
                  mpPadlockPriorityObject not_eq mpPadlockCandidate )
                 {
                 VuDeReferenceEntity(mpPadlockPriorityObject);
                 mpPadlockPriorityObject = (SimBaseClass*) mpPadlockCandidate;
                 VuReferenceEntity(mpPadlockPriorityObject);
                 }
                 else if ( not mpPadlockPriorityObject )
                 {
                 mpPadlockPriorityObject = (SimBaseClass*) mpPadlockCandidate;
                 VuReferenceEntity(mpPadlockPriorityObject);
                 }
                 mpPadlockCandidate = NULL;
                }

                if ( mpPadlockPriorityObject and mpPadlockPriorityObject->IsSim() )
                {
                 SetGraphicsOwnship( mpPadlockPriorityObject );
                 SetTrackPlatform( SimDriver.GetPlayerAircraft() );
                }
                */

                break;

            case ModeIncoming:

                if ( not otwTrackPlatform)
                    return;

                // try and find a flying weapon for the current otwPlatform
                newObject = FindNextViewObject(otwTrackPlatform.get(), otwPlatform.get(), NEXT_INCOMING);

                // At the moment, no weapons flying, no mode change
                if (newObject == NULL)
                    return;

                // set graphics focus to the object
                SetGraphicsOwnship(newObject);

                break;

            default:
                break;
        }

        return;
    }
    else
    {
        // we've got a new mode, some must be checked prior to switching
        switch (mode)
        {
            case ModeFlyby:
                if (otwPlatform)
                {
                    endFlightPoint.x = otwPlatform->XPos() + otwPlatform->dmx[0][0] * 10.0f + otwPlatform->XDelta() * 2.0f ;
                    endFlightPoint.y = otwPlatform->YPos() + otwPlatform->dmx[0][1] * 10.0f + otwPlatform->YDelta() * 2.0f ;
                    endFlightPoint.z = otwPlatform->ZPos() + otwPlatform->dmx[0][2] * 10.0f + otwPlatform->ZDelta() * 2.0f ;
                    endFlightPoint.z -= 20.0f;
                    endFlightVec.x = 0.0f;
                    endFlightVec.y = 0.0f;
                    endFlightVec.z = 0.0f;
                }
                else
                {
                    // not otwplatform, use last focus point with some randomness...
                    float groundZ;

                    endFlightPoint = focusPoint;
                    endFlightPoint.z += 400.0f * PRANDFloat();
                    endFlightPoint.z += 400.0f * PRANDFloat();
                    endFlightPoint.z -= 100.0f;

                    groundZ = GetGroundLevel(endFlightPoint.x, endFlightPoint.y);

                    if (endFlightPoint.z + 50.0f > groundZ)
                        endFlightPoint.z = groundZ - 50.0f;

                    endFlightVec.x = 0.0f;
                    endFlightVec.y = 0.0f;
                    endFlightVec.z = -1.0f;
                }

                break;

            case ModeWeapon:
                // try and find a flying weapon for the current otwPlatform
                newObject = FindNextViewObject(otwPlatform.get(), NULL, NEXT_WEAPON);

                // At the moment, no weapons flying, no mode change
                if (newObject == NULL)
                    return;

                // set graphics focus to the weapon
                SetGraphicsOwnship(newObject);

                break;

            case ModeTargetToWeapon:
                // try and find a flying weapon for the current otwPlatform
                newObject = FindNextViewObject(otwPlatform.get(), NULL, NEXT_WEAPON);

                // At the moment, no weapons flying, no mode change
                if (newObject == NULL or otwTrackPlatform.get() == NULL)
                    return;

                // set graphics focus to the target, tracking to weapon
                if (otwTrackPlatform->IsSim())
                {
                    SetGraphicsOwnship(otwTrackPlatform.get());
                    SetTrackPlatform(newObject);
                }
                else
                {
                    SetGraphicsOwnship(newObject);
                }

                break;

            case ModeAirFriendly:

                // try and find a flying weapon for the current otwPlatform
                newObject = FindNextViewObject(SimDriver.GetPlayerAircraft(), NULL, NEXT_AIR_FRIEND);

                // At the moment, no weapons flying, no mode change
                if (newObject == NULL)
                    return;

                // set graphics focus to the object
                SetGraphicsOwnship(newObject);

                break;

            case ModeAirEnemy:

                // try and find a flying weapon for the current otwPlatform
                newObject = FindNextViewObject(SimDriver.GetPlayerAircraft(), NULL, NEXT_AIR_ENEMY);

                // At the moment, no weapons flying, no mode change
                if (newObject == NULL)
                    return;

                // set graphics focus to the object
                SetGraphicsOwnship(newObject);

                break;

            case ModeGroundFriendly:

                // try and find a flying weapon for the current otwPlatform
                newObject = FindNextViewObject(SimDriver.GetPlayerAircraft(), NULL, NEXT_GROUND_FRIEND);

                // At the moment, no weapons flying, no mode change
                if (newObject == NULL)
                    return;

                // set graphics focus to the object
                SetGraphicsOwnship(newObject);

                break;

            case ModeGroundEnemy:

                // try and find a flying weapon for the current otwPlatform
                newObject = FindNextViewObject(SimDriver.GetPlayerAircraft(), NULL, NEXT_GROUND_ENEMY);

                // At the moment, no weapons flying, no mode change
                if (newObject == NULL)
                    return;

                // set graphics focus to the object
                SetGraphicsOwnship(newObject);

                break;

            case ModeTarget:

                if ( not SimDriver.GetPlayerAircraft())
                    return;

                // try and find a flying weapon for the current otwPlatform
                newObject = FindNextViewObject(SimDriver.GetPlayerAircraft(), NULL, NEXT_ENEMY);    // 2002-02-16 MODIFIED BY S.G. Uses the new NEXT_ENEMY mode instead of NEXT_GROUND_ENEMY

                // At the moment, no bject, no mode change
                if (newObject == NULL)
                    return;

                // set graphics focus to the object
                SetGraphicsOwnship(SimDriver.GetPlayerAircraft());
                SetTrackPlatform(newObject);

                /*
                if ( not SimDriver.GetPlayerAircraft() )
                 return;

                // ownship is always the focus
                SetGraphicsOwnship( SimDriver.GetPlayerAircraft() );

                // if we got a candidate we use that
                if ( mpPadlockCandidate )
                {
                 if ( mpPadlockPriorityObject and 
                  mpPadlockPriorityObject not_eq mpPadlockCandidate )
                 {
                 VuDeReferenceEntity(mpPadlockPriorityObject);
                 mpPadlockPriorityObject = (SimBaseClass*) mpPadlockCandidate;
                 VuReferenceEntity(mpPadlockPriorityObject);
                 }
                 else if ( not mpPadlockPriorityObject )
                 {
                 mpPadlockPriorityObject = (SimBaseClass*) mpPadlockCandidate;
                 VuReferenceEntity(mpPadlockPriorityObject);
                 }
                 mpPadlockCandidate = NULL;
                }

                if ( mpPadlockPriorityObject )
                {
                 SetTrackPlatform( mpPadlockPriorityObject );
                 chaseRange *= 2.5f;
                }
                else
                {

                 // use the enhanced padlock priority to cycle thru appro-
                 // priate objects
                 Padlock_FindEnhancedPriority( TRUE );

                 if ( mpPadlockCandidate )
                 {
                 if ( mpPadlockPriorityObject and 
                  mpPadlockPriorityObject not_eq mpPadlockCandidate )
                 {
                 VuDeReferenceEntity(mpPadlockPriorityObject);
                 mpPadlockPriorityObject = (SimBaseClass*) mpPadlockCandidate;
                 VuReferenceEntity(mpPadlockPriorityObject);
                 }
                 else if ( not mpPadlockPriorityObject )
                 {
                 mpPadlockPriorityObject = (SimBaseClass*) mpPadlockCandidate;
                 VuReferenceEntity(mpPadlockPriorityObject);
                 }
                 mpPadlockCandidate = NULL;
                 }

                 if ( mpPadlockPriorityObject )
                 {
                 SetTrackPlatform( mpPadlockPriorityObject );
                 chaseRange *= 2.5f;
                 }

                }

                if ( not otwPlatform or otwPlatform->IsSimObjective())
                 return;

                // get the target (if any) of the current platform
                targetPtr = (( SimMoverClass * )otwPlatform)->targetPtr;

                // any target?
                if ( targetPtr == NULL or targetPtr->BaseData() == NULL or not targetPtr->BaseData()->IsSim())
                {
                 // no target, look for air threat
                 newObject = FindNextViewObject( otwPlatform, NULL, NEXT_AIR_ENEMY  );

                 if ( newObject )
                 {
                 // set graphics focus to the target
                 SetGraphicsOwnship( otwPlatform );

                 // set the track platform to view platform
                 SetTrackPlatform( newObject );
                 chaseRange *= 2.5f;
                 }
                }
                else
                {
                 // set graphics focus to the target
                 SetGraphicsOwnship( otwPlatform );

                 // set the track platform to view platform
                 SetTrackPlatform( (SimBaseClass*)targetPtr->BaseData() );
                 chaseRange *= 2.5f;
                }
                */

                break;

            case ModeTargetToSelf:

                if ( not SimDriver.GetPlayerAircraft())
                    return;

                // try and find a flying weapon for the current otwPlatform
                newObject = FindNextViewObject(SimDriver.GetPlayerAircraft(), NULL, NEXT_ENEMY);    // 2002-02-16 MODIFIED BY S.G. Uses the new NEXT_ENEMY mode instead of NEXT_GROUND_ENEMY

                // At the moment, no bject, no mode change
                if (newObject == NULL)
                    return;

                // set graphics focus to the object
                SetGraphicsOwnship(newObject);
                SetTrackPlatform(SimDriver.GetPlayerAircraft());

                /*
                if ( not SimDriver.GetPlayerAircraft() )
                 return;


                // if we got a candidate we use that
                if ( mpPadlockCandidate and mpPadlockCandidate->IsSim() )
                {
                 if ( mpPadlockPriorityObject and 
                  mpPadlockPriorityObject not_eq mpPadlockCandidate )
                 {
                 VuDeReferenceEntity(mpPadlockPriorityObject);
                 mpPadlockPriorityObject = (SimBaseClass*) mpPadlockCandidate;
                 VuReferenceEntity(mpPadlockPriorityObject);
                 }
                 else if ( not mpPadlockPriorityObject )
                 {
                 mpPadlockPriorityObject = (SimBaseClass*) mpPadlockCandidate;
                 VuReferenceEntity(mpPadlockPriorityObject);
                 }
                 mpPadlockCandidate = NULL;
                }

                if ( mpPadlockPriorityObject and mpPadlockPriorityObject->IsSim() )
                {
                 SetGraphicsOwnship( mpPadlockPriorityObject );
                 SetTrackPlatform( SimDriver.GetPlayerAircraft() );
                 chaseRange *= 2.5f;
                }
                else
                {

                 // use the enhanced padlock priority to cycle thru appro-
                 // priate objects

                 // we need to have the player be otwPlatform for
                 // the Padlock_FindEnhancedPriority
                 SetGraphicsOwnship( SimDriver.GetPlayerAircraft() );
                 Padlock_FindEnhancedPriority( TRUE );

                 if ( mpPadlockCandidate and mpPadlockCandidate->IsSim() )
                 {
                 if ( mpPadlockPriorityObject and 
                  mpPadlockPriorityObject not_eq mpPadlockCandidate )
                 {
                 VuDeReferenceEntity(mpPadlockPriorityObject);
                 mpPadlockPriorityObject = (SimBaseClass*) mpPadlockCandidate;
                 VuReferenceEntity(mpPadlockPriorityObject);
                 }
                 else if ( not mpPadlockPriorityObject )
                 {
                 mpPadlockPriorityObject = (SimBaseClass*) mpPadlockCandidate;
                 VuReferenceEntity(mpPadlockPriorityObject);
                 }
                 mpPadlockCandidate = NULL;
                 }

                 if ( mpPadlockPriorityObject and mpPadlockPriorityObject->IsSim() )
                 {
                 SetGraphicsOwnship( mpPadlockPriorityObject );
                 SetTrackPlatform( SimDriver.GetPlayerAircraft() );
                 chaseRange *= 2.5f;
                 }
                 else
                 {
                 return;
                 }

                }
                */


                break;

            case ModeIncoming:

                // try and find a flying weapon for the current otwPlatform
                newObject = FindNextViewObject(otwPlatform.get(), NULL, NEXT_INCOMING);

                // At the moment, no weapons flying, no mode change
                if (newObject == NULL)
                    return;

                // set the track platform to view platform
                SetTrackPlatform(otwPlatform.get());

                // set graphics focus to the object
                SetGraphicsOwnship(newObject);

                break;

                // these modes require a SimDriver Player Entity
            case ModeHud:
            case ModePadlockF3:
            case ModePadlockEFOV:
            case Mode2DCockpit:
            case Mode3DCockpit:
                if (
                    (ejectCam) or
                    ( not SimDriver.GetPlayerAircraft())
                    //or  (FalconLocalSession->GetFlyState() not_eq FLYSTATE_FLYING)
                )
                {
                    return;
                }

                switch (mOTWDisplayMode)
                {
                    case ModeHud:
                    case ModePadlockF3:
                    case ModePadlockEFOV:
                    case Mode2DCockpit:
                    case Mode3DCockpit:
                        break;

                    default:
                        // going from external to internal view
                        //narrowFOV = FALSE;
                        //Wombat778 9-29-2003 Removed to allow FOV to be persistent
                        //SetFOV( 60.0F * DTR );
                        //Wombat778 9-29-2003
                        break;
                }

                break;

            default:
                break;
        }

        CleanupDisplayMode(mOTWDisplayMode);
        OTWDisplayMode lastmode = mOTWDisplayMode; //Wombat778 11-17-04
        mOTWDisplayMode = mode;
        SelectDisplayMode(mode, lastmode); //Wombat778 11-17-04 added lastmode
    }

    CameraChange = true;
}


void OTWDriverClass::CleanupDisplayMode(OTWDisplayMode mode)
{
    if (mode == Mode2DCockpit)
    {
        Cleanup2DCockpitMode();
    }
}


void OTWDriverClass::SelectDisplayMode(OTWDisplayMode mode, OTWDisplayMode lastmode) //Wombat778 11-17-04 added lastmode
{

    // since we're doing a new mode reset any view timers
    flybyTimer = 0;

    //Wombat778 12-03-04 Disable hybrid pit mode on a view change
    if (GetHybridPitMode())
        ToggleHybridPitMode();

    switch (mode)
    {

            //Wombat778 11-18-04 Move view resetting stuff here

        case ModeNone:
            ViewReset(); //Wombat778
            break;

        case ModeHud:
        case ModePadlockEFOV:
            ViewReset(); //Wombat778

            if (PlayerOptions.GetPadlockMode() not_eq PDDisabled)
            {
                // this mode restores to player's f16
                if (otwPlatform.get() not_eq SimDriver.GetPlayerAircraft())
                    SetGraphicsOwnship(SimDriver.GetPlayerAircraft());
            }

            break;

        case ModeIncoming:
        case ModeAirFriendly:
        case ModeTarget:
        case ModeTargetToSelf:
        case ModeTargetToWeapon:
        case ModeGroundFriendly:
        case ModeAirEnemy:
        case ModeGroundEnemy:
        case ModeWeapon:
            ViewReset(); //Wombat778
            SelectExternal();
            chaseRange *= 2.5;
            break;

        case ModePadlockF3:
            if (PlayerOptions.GetPadlockMode() not_eq PDDisabled)
            {
                if (otwPlatform.get() not_eq SimDriver.GetPlayerAircraft())
                    SetGraphicsOwnship(SimDriver.GetPlayerAircraft());

                SelectF3PadlockMode();
            }

            break;

        case Mode2DCockpit:

            // this mode restores to player's f16

            //Wombat778 11-18-04 added view reset code.  Only reset the view when not moving from 2d to 3d cockpit.
            if (otwPlatform.get() not_eq SimDriver.GetPlayerAircraft())
                SetGraphicsOwnship(SimDriver.GetPlayerAircraft());

            if (g_bSync2D3DPit and lastmode == Mode3DCockpit) //consider adding padlock modes
                Select2DCockpitMode(0);
            else
            {
                ViewReset();
                Select2DCockpitMode(1);
            }

            break;

        case Mode3DCockpit:

            // this mode restores to player's f16

            if ( not g_bSync2D3DPit or lastmode not_eq Mode2DCockpit) //Wombat778 reset the view if not from 2d pit. COnsider adding padlock modes here
            {
                ViewReset();
                eyePan = 0.0f;
                eyeTilt = g_f3DHeadTilt * DTR; // Cobra
                SetFOV(g_f3DPitFOV * DTR);   // Cobra
            }
            else if (g_bSync2D3DPit and lastmode == Mode2DCockpit)
            {
                if (eyePan == 0.0f and fabs(eyeTilt) < 0.3f)  // Cobra - Default 3D pit view when 2D pit view is straight forward view (panel 1100)
                {
                    eyePan = 0.0f;
                    eyeTilt = g_f3DHeadTilt * DTR; // Cobra
                    SetFOV(g_f3DPitFOV * DTR);   // Cobra
                }
            }

            if (otwPlatform.get() not_eq SimDriver.GetPlayerAircraft())
                SetGraphicsOwnship(SimDriver.GetPlayerAircraft());

            Select3DCockpitMode();
            break;

        case ModeChase:
            ViewReset(); //Wombat778
            SelectExternal();
            break;

        case ModeOrbit:
            ViewReset(); //Wombat778
            SelectExternal();
            break;

        case ModeSatellite:
            ViewReset(); //Wombat778
            SelectExternal();
            break;

        case ModeFlyby:
            ViewReset(); //Wombat778
            SelectExternal();
            break;

        default:
            ViewReset(); //Wombat778
            ShiWarning("Bad Viewing mode");
    }
}


void OTWDriverClass::StepHeadingScale(void)
{
    if (TheHud->headingPos == HudClass::High)
        TheHud->headingPos = HudClass::Low;
    else if (TheHud->headingPos == HudClass::Low)
        TheHud->headingPos = HudClass::Off;
    else if (TheHud->headingPos == HudClass::Off)
        TheHud->headingPos = HudClass::High;
    else
        TheHud->headingPos = HudClass::Off;
}


void OTWDriverClass::ToggleGLOC(void)
{
    doGLOC = 1 - doGLOC;

    if (doGLOC)
        PlayerOptions.ClearSimFlag(SIM_NO_BLACKOUT);
    else
        PlayerOptions.SetSimFlag(SIM_NO_BLACKOUT);
}


void OTWDriverClass::ToggleBilinearFilter(void)
{

    if (PlayerOptions.FilteringOn())
        PlayerOptions.ClearDispFlag(DISP_BILINEAR);
    else
        PlayerOptions.SetDispFlag(DISP_BILINEAR);

    renderer->SetFilteringMode(1 - renderer->GetFilteringMode());
}

void OTWDriverClass::ToggleShading(void)
{
    if (DisplayOptions.bSpecularLighting)
        DisplayOptions.bSpecularLighting = FALSE;
    else
        DisplayOptions.bSpecularLighting = TRUE;
}

void OTWDriverClass::ToggleHaze(void)
{
    if (PlayerOptions.HazingOn())
        PlayerOptions.ClearDispFlag(DISP_HAZING);
    else
        PlayerOptions.SetDispFlag(DISP_HAZING);

    renderer->SetHazeMode(1 - renderer->GetHazeMode());
}

void OTWDriverClass::ToggleLocationDisplay(void)
{
    showPos = not showPos;
}

void OTWDriverClass::ToggleAeroDisplay(void)
{
    showAero = not showAero;
}

//TJL 11/09/03 On/Off Flaps
void OTWDriverClass::ToggleFlapDisplay(void)
{
    showFlaps = not showFlaps;
}

// Retro 1Feb2004 start
void OTWDriverClass::ToggleEngineDisplay(void)
{
    showEngine = not showEngine;
}
// Retro 1Feb2004 end

void OTWDriverClass::ToggleThrustReverseDisplay(void)
{
    showThrustReverse = not showThrustReverse;
}

void OTWDriverClass::ToggleRoof(void)
{
    renderer->SetRoofMode(1 - renderer->GetRoofMode());
}

void OTWDriverClass::ScaleDown(void)
{
    SetScale(Scale() * 0.8F);
    RescaleAllObjects();
}

void OTWDriverClass::ScaleUp(void)
{
    if (Scale() == FalconLocalGame->rules.ObjectMagnification())
        return;

    SetScale(Scale() * 1.25F);

    if (Scale() > FalconLocalGame->rules.ObjectMagnification())
        SetScale(FalconLocalGame->rules.ObjectMagnification());

    RescaleAllObjects();
}

void OTWDriverClass::SetDetail(int newlevel)
{
    SimFeatureClass* theObject, *parentObject;
    DrawableObject *parentDrawable;

    int oldlevel = PlayerOptions.BuildingDeaggLevel();

    if (oldlevel == newlevel)
        return;

    VuListIterator featureWalker(SimDriver.featureList);
    theObject = (SimFeatureClass*)featureWalker.GetFirst();

    while (theObject)
    {
        // Find any existing parent object
        parentObject = (SimFeatureClass*) theObject->GetCampaignObject()->GetComponentLead();

        // Find any existing parent drawable
        parentDrawable = NULL;

        if (parentObject and parentObject->IsSetCampaignFlag(FEAT_ELEV_CONTAINER))
            parentDrawable = parentObject->baseObject;
        else if (parentObject and parentObject->IsSetCampaignFlag(FEAT_FLAT_CONTAINER))
            parentDrawable = parentObject->drawPointer;

        // KCK: For now, if we've got a parent drawable object, don't fuck with it.
        if ( not parentDrawable)
        {
            if (theObject->drawPointer and theObject->displayPriority <= oldlevel and theObject->displayPriority > newlevel)
            {
                // We don't want to display this anymore
                OTWDriver.RemoveObject(theObject->drawPointer);
                SimDriver.featureList->Remove(theObject);
            }

            if (theObject->drawPointer and theObject->displayPriority > oldlevel and theObject->displayPriority <= newlevel)
            {
                // We want to display this now
                OTWDriver.InsertObject(theObject->drawPointer);
                SimDriver.AddToFeatureList(theObject);
            }
        }

        theObject = (SimFeatureClass*)featureWalker.GetNext();
    }

    PlayerOptions.BldDeaggLevel = newlevel;
    PlayerOptions.ObjDeaggLevel = newlevel * 20;
}

void OTWDriverClass::DetailDown(void)
{
    int oldlevel = PlayerOptions.BuildingDeaggLevel();

    if (oldlevel > 0)
        SetDetail(oldlevel - 1);
}

void OTWDriverClass::DetailUp(void)
{
    int oldlevel = PlayerOptions.BuildingDeaggLevel();

    if (oldlevel < 5)
        SetDetail(oldlevel + 1);
}

/*void OTWDriverClass::TextureUp(void)
{
 if ( renderer->GetObjectTextureState() ) {
 renderer->SetTerrainTextureLevel( renderer->GetTerrainTextureLevel()+1 );
 } else {
 renderer->SetObjectTextureState( TRUE );
 }
}

void OTWDriverClass::TextureDown(void)
{
 if ( renderer->GetTerrainTextureLevel() ) {
 renderer->SetTerrainTextureLevel( renderer->GetTerrainTextureLevel()-1 );
 } else {
 renderer->SetObjectTextureState( FALSE );
 }
}
*/
void OTWDriverClass::ToggleWeather(void)
{
    if (PlayerOptions.WeatherOn())
        PlayerOptions.ClearGenFlag(GEN_NO_WEATHER);
    else
        PlayerOptions.SetGenFlag(GEN_NO_WEATHER);

    weatherCmd = 1;
}


void OTWDriverClass::ToggleEyeFly(void)
{
    mlTrig trigYaw, trigPitch, trigRoll;

    if ( not eyeFlyEnabled)
    {
        return;
    }

    if (eyeFly)
    {
        if (SimDriver.GetPlayerEntity())
        {
            // Make sure we don't leave it hidden
            if (otwPlatform and otwPlatform->drawPointer)
            {
                otwPlatform->drawPointer->SetInhibitFlag(FALSE);
            }

            FalconLocalSession->RemoveCamera(flyingEye);
            otwPlatform = lastotwPlatform;
            lastotwPlatform.reset(NULL);
            FalconLocalSession->AttachCamera(SimDriver.GetPlayerEntity());
        }
        else
        {
            eyeFly = 1 - eyeFly;
        }
    }
    else
    {
        // This will crash if it happens after the player has crashed and chosen "Resume"
        // because at that point, the SimDriver.GetPlayerAircraft() is NULL.  Not sure
        // what the right answer is, but for now we'll allow the bullet to be dodged (maybe).
        if (SimDriver.GetPlayerEntity())
        {
            FalconLocalSession->RemoveCamera(SimDriver.GetPlayerEntity());
        }

        lastotwPlatform = otwPlatform;
        otwPlatform.reset(NULL);
        FalconLocalSession->AttachCamera(flyingEye);

        if (lastotwPlatform)
        {
            mlSinCos(&trigYaw,   lastotwPlatform->Yaw() * 0.5F);
            mlSinCos(&trigPitch, lastotwPlatform->Pitch() * 0.5F);
            mlSinCos(&trigRoll,  lastotwPlatform->Roll() * 0.5F);
        }
        else
        {
            trigYaw.cos = 1.0F;
            trigYaw.sin = 0.0F;
            trigPitch.cos = 1.0F;
            trigPitch.sin = 0.0F;
            trigRoll.cos = 1.0F;
            trigRoll.sin = 0.0F;
        }

        e1 = trigYaw.cos * trigPitch.cos * trigRoll.cos +
             trigYaw.sin * trigPitch.sin * trigRoll.sin;

        e2 = trigYaw.sin * trigPitch.cos * trigRoll.cos -
             trigYaw.cos * trigPitch.sin * trigRoll.sin;

        e3 = trigYaw.cos * trigPitch.sin * trigRoll.cos +
             trigYaw.sin * trigPitch.cos * trigRoll.sin;

        e4 = trigYaw.cos * trigPitch.cos * trigRoll.sin -
             trigYaw.sin * trigPitch.sin * trigRoll.cos;
    }

    eyeFly = 1 - eyeFly;
}

void OTWDriverClass::StartLocationEntry(void)
{
    getNewCameraPos = TRUE;
    insertMode = TRUE;
    memset(chatterStr, 0, 256);
    chatterCount = 0;
    CommandsKeyCombo = -1;
    CommandsKeyComboMod = -1;
}

void OTWDriverClass::ToggleAutoScale(void)
{
    autoScale = 1 - autoScale;
}

void OTWDriverClass::EndFlight(void)
{
    SetExitMenu(TRUE);
}

#include "mouselook.h" // Retro 18Jan2004

void OTWDriverClass::ViewTiltUp(void)
{
    if (SimDriver.GetPlayerAircraft() and SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) and mOTWDisplayMode == Mode2DCockpit)
    {
        SimDriver.POVKludgeFunction(POV_N);
    }
    else
    {
#ifdef DAVE_DBG

        if (MoveBoom)
            boomElTest -= 3.0F * DTR;
        else
            elDir = -1.0F;

#else

        elDir = -1.0F;

        if (PlayerOptions.GetMouseLook()) // Retro 18Jan2004
            theMouseView.BumpViewUp(-1.);

#endif

    }
}

void OTWDriverClass::ViewTiltDown(void)
{
    if (SimDriver.GetPlayerAircraft() and SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) and mOTWDisplayMode == Mode2DCockpit)
    {
        SimDriver.POVKludgeFunction(POV_S);
    }
    else
    {
#ifdef DAVE_DBG

        if (MoveBoom)
            boomElTest += 3.0F * DTR;
        else
            elDir = 1.0F;

#else
        elDir = 1.0F;

        if (PlayerOptions.GetMouseLook()) // Retro 18Jan2004
            theMouseView.BumpViewUp(1.);

#endif
    }
}

void OTWDriverClass::ViewTiltHold(void)
{
    if ( not (SimDriver.GetPlayerAircraft() and SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) and mOTWDisplayMode == Mode2DCockpit))
    {
        {
            elDir = 0.0F;

            if (PlayerOptions.GetMouseLook()) // Retro 18Jan2004
                theMouseView.BumpViewUp(0);
        }
    }
}

void OTWDriverClass::ViewSpinLeft(void)
{
    if (SimDriver.GetPlayerAircraft() and SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) and mOTWDisplayMode == Mode2DCockpit)
    {
        SimDriver.POVKludgeFunction(POV_W);
    }
    else
    {
#ifdef DAVE_DBG

        if (MoveBoom)
            boomAzTest += 3.0F * DTR;
        else
            azDir = 1.0F;

#else
        azDir = 1.0F;

        if (PlayerOptions.GetMouseLook()) // Retro 18Jan2004
            theMouseView.BumpViewLeft(1);

#endif
    }
}

void OTWDriverClass::ViewSpinRight(void)
{
    if (SimDriver.GetPlayerAircraft() and SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) and mOTWDisplayMode == Mode2DCockpit)
    {
        SimDriver.POVKludgeFunction(POV_E);
    }
    else
    {
#ifdef DAVE_DBG

        if (MoveBoom)
            boomAzTest -= 3.0F * DTR;
        else
            azDir = -1.0F;

#else
        azDir = -1.0F;

        if (PlayerOptions.GetMouseLook()) // Retro 18Jan2004
            theMouseView.BumpViewLeft(-1);

#endif
    }
}

void OTWDriverClass::ViewSpinHold(void)
{
    if ( not (SimDriver.GetPlayerAircraft() and SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) and mOTWDisplayMode == Mode2DCockpit))
    {
        {
            azDir = 0.0F;

            if (PlayerOptions.GetMouseLook()) // Retro 18Jan2004
                theMouseView.BumpViewLeft(0);
        }
    }
}

void OTWDriverClass::ViewReset(void)
{
    if ( not otwPlatform)
        return;

    azDir = 0.0F;
    elDir = 0.0F;

    if (PlayerOptions.GetMouseLook()) // Retro 18Jan2004
    {
        theMouseView.BumpViewUp(0);
        theMouseView.BumpViewLeft(0);
    }

    chaseAz = 0.0F;
    chaseEl = 0.0F;
    chaseRange = -75.0F;

    eyeHeadRoll = 0.0F;

    if (GetOTWDisplayMode() == Mode3DCockpit or
        GetOTWDisplayMode() == ModeHud or
        GetOTWDisplayMode() == ModePadlockF3)
    {
        eyePan = 0.0F;
        eyeTilt = 0.35F;
    }
}

static int sfxCtr = 0;
static float sfxDist = 30000.0f;

void OTWDriverClass::ViewZoomIn(void)
{
#ifdef DAVE_DBG

    if (MoveBoom)
        boomExtTest += 1.0F;
    else
        chaseRange = min(-50.0F, chaseRange + 50.0F);

#else
    chaseRange = min(-50.0F, chaseRange + 50.0F);
#endif

    // just testing distance effects....

    /*
    sfxDist += 10000.0f;
    if (sfxDist > 250000.0f )
     sfxDist = 30000.0f;
    */
}

void OTWDriverClass::ViewZoomOut(void)
{
#ifdef DAVE_DBG

    if (MoveBoom)
        boomExtTest -= 1.0F;
    else
        chaseRange = max(-900.0F, chaseRange - 50.0F);

#else
    chaseRange = max(-900.0F, chaseRange - 50.0F);
#endif

    // just testing distance effects....
    /*
    Tpoint sfxpos,vec;
    float dist;
    if ( otwPlatform )
    {
     vec.x = otwPlatform->dmx[0][0];
     vec.y = otwPlatform->dmx[0][1];
     dist = sqrt( vec.x * vec.x + vec.y * vec.y );
     vec.x /= dist;
     vec.y /= dist;
     sfxpos.x = otwPlatform->XPos() + vec.x * sfxDist;
     sfxpos.y = otwPlatform->YPos() + vec.y * sfxDist;
     sfxpos.z = otwPlatform->ZPos();
     if ( (sfxCtr)  == 0 )
     {
     vec.x = otwPlatform->dmx[2][0];
     vec.y = otwPlatform->dmx[2][1];
     vec.z = -fabs(otwPlatform->dmx[2][2]);
     AddSfxRequest( new SfxClass( SFX_DIST_SAMLAUNCHES,
      &sfxpos,
      &vec,
      20,
      1.0f ) );
     }
     else if ( (sfxCtr ) == 1 )
     {
     AddSfxRequest( new SfxClass( SFX_DIST_GROUNDBURSTS,
      &sfxpos,
      20,
      1.0f ) );
     }
     else if ( (sfxCtr ) == 2 )
     {
     vec.x = otwPlatform->dmx[2][0];
     vec.y = otwPlatform->dmx[2][1];
     vec.z = -fabs(otwPlatform->dmx[2][2]);
     AddSfxRequest( new SfxClass(SFX_DIST_ARMOR,
      &sfxpos,
      &vec,
      20,
      1.0f ) );
     }
     else if ( (sfxCtr) == 3 )
     {
     vec.x = otwPlatform->dmx[2][0];
     vec.y = otwPlatform->dmx[2][1];
     vec.z = -fabs(otwPlatform->dmx[2][2]);
     AddSfxRequest( new SfxClass(SFX_DIST_INFANTRY,
      &sfxpos,
      &vec,
      20,
      1.0f ) );
     }
     else if ( (sfxCtr) == 4 )
     {
     vec.x = otwPlatform->dmx[1][0];
     vec.y = otwPlatform->dmx[1][1];
     vec.z = otwPlatform->dmx[1][2];
     AddSfxRequest( new SfxClass(SFX_DIST_AALAUNCHES,
      &sfxpos,
      &vec,
      20,
      1.0f ) );
     }
     else if ( (sfxCtr) == 5 )
     {
     AddSfxRequest( new SfxClass(SFX_DIST_AIRBURSTS,
      &sfxpos,
      20,
      1.0f ) );
     }

     sfxCtr++;
     if ( sfxCtr > 5 ) sfxCtr = 0;
    }
    */
}

void OTWDriverClass::SetPopup(int popNum, int flag)
{
    if (flag)
        popupHas[popNum] = popNum;
    else
        popupHas[popNum] = -1;
}

void OTWDriverClass::GlanceForward(void)
{
    if (padlockGlance == GlanceNose)
        padlockGlance = GlanceNone;
    else
        padlockGlance = GlanceNose;
}

void OTWDriverClass::GlanceAft(void)
{
    if (padlockGlance == GlanceTail)
        padlockGlance = GlanceNone;
    else
        padlockGlance = GlanceTail;
}

void OTWDriverClass::Padlock_SetPriority(PadlockPriority newPriority)
{
    //   if (padlockPriority == newPriority)
    //   {
    //      tgtStep = 1;
    //   }
    //   else
    //   {
    padlockPriority = newPriority;

    if (otwPlatform)
    {
        PadlockOccludedTime = 0.0F;
        Padlock_FindNextPriority(FALSE);
    }

    //   }
}

void OTWDriverClass::EyeFlyStateStep(void)
{
    int curStatus;

    if (eyeFlyTgt)
    {
        curStatus = eyeFlyTgt->Status() bitand VIS_TYPE_MASK;
        curStatus ++;
        curStatus %= 4;
        eyeFlyTgt->ClearStatusBit(VIS_TYPE_MASK);
        eyeFlyTgt->SetStatusBit(curStatus);
    }
}

//Wombat778 10-08-2003  Added to allow a respectable mouselook

int OTWDriverClass::ViewRelativePanTilt(float Pan, float Tilt)
{
    if (SimDriver.GetPlayerAircraft() and SimDriver.GetPlayerAircraft()->IsSetFlag(MOTION_OWNSHIP) and mOTWDisplayMode == Mode3DCockpit)
    {
        eyePan += Pan * DTR;
        eyeTilt += Tilt * DTR;
        return 1;
    }
    else
        return 0;

}
