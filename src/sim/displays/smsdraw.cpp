#include "stdhdr.h"
#include "Graphics/Include/Render2D.h"
#include "Sim/Include/SimVuDrv.h"
#include "radar.h"
#include "mfd.h"
#include "missile.h"
#include "misldisp.h"
#include "mavdisp.h"
#include "laserpod.h"
#include "harmpod.h"
#include "fcc.h"
#include "hardpnt.h"
#include "simveh.h"
#include "falcsess.h"
#include "playerop.h"
#include "commands.h"
#include "SmsDraw.h"
#include "sms.h"
#include "otwdrive.h"
#include "vu2.h"
/* ADDED BY S.G. FOR SELECTIVE JETISSON */ #include "aircrft.h"
/* ADDED BY S.G. FOR SELECTIVE JETISSON */ #include "airframe.h"
#include "campbase.h" // Marco for AIM9P
#include "classtbl.h"
#include "otwdrive.h" //MI
#include "cpmanager.h" //MI
#include "icp.h" //MI
#include "aircrft.h" //MI
#include "fcc.h" //MI
#include "radardoppler.h" //MI
#include "simdrive.h" //MI
#include "hud.h" //MI
#include "fault.h" //MI
#include "radardoppler.h" //MI
#include "rdrackdata.h"
#include "bomb.h"
#include "navsystem.h"

bool isJSOW = FALSE;
bool isJDAM = FALSE;
//MI
void DrawBullseyeCircle(VirtualDisplay* display, float cursorX, float cursorY);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

SensorClass* FindLaserPod(SimMoverClass* theObject);
extern int F4FlyingEyeType;
//extern bool g_bArmingDelay;//me123 MI replaced with g_bRealisticAvionics
extern bool g_bRealisticAvionics;
extern bool g_bMLU;
//extern bool g_bHardCoreReal; //me123 MI replaced with g_bRealisticAvionics


//MI
int SmsDrawable::flash = FALSE;
int SmsDrawable::InputFlash = FALSE;
//M.N.
int maxripple;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

SmsDrawable::SmsDrawable(SMSClass* self)
{
    lastMode = displayMode = Inv;
    Sms = self;
    groundSpot = new SpotEntity(F4FlyingEyeType + VU_LAST_ENTITY_TYPE);
    vuDatabase->/*Quick*/Insert(groundSpot);
    frameCount = 0;
    isDisplayed = FALSE;
    /*sjHardPointSelected = */
    hardPointSelected = 0; // MLR 3/9/2004
    groundSpot->SetDriver(new SpotDriver(groundSpot));
    hadCamera = FALSE;
    needCamera = FALSE;
    flags = 0;
    //MI new stuff
    Sms->BHOT = TRUE;
    EmergStoreMode = Inv;
    DataInputMode = Inv;
    lastInputMode = Inv;
    PossibleInputs = 0;
    InputModus = NONE;

    for (int i = 0; i < MAX_DIGITS; i++)
        Input_Digits[i] = 25;

    Manual_Input = FALSE;

    for (int i = 0; i < STR_LEN; i++)
        inputstr[i] = ' ';

    inputstr[STR_LEN - 1] = '\0';
    wrong = FALSE;
    InputsMade = 0;
    C1Weap = FALSE;
    C2Weap = FALSE;
    C3Weap = FALSE;
    C4Weap = FALSE;
    InputLine = 0;
    MaxInputLines = 0;

    //Init some stuff
    //Sms->Prof1 = not Sms->Prof1;  // MLR 4/5/2004 - Why???

    FireControlComputer *pFCC = Sms->ownship->GetFCC();

    // MLR 4/3/2004 -
    //Sms->rippleCount    = Sms->GetAGBRippleCount();
    //Sms->rippleInterval = Sms->GetAGBRippleInterval();
    //Sms->SetPair( Sms->GetAGBPair() );
    //Sms->angle          = Sms->GetAGBReleaseAngle();
    //Sms->armingdelay    = Sms->agbProfile[Sms->curProfile].armingDelay;


    if (pFCC)
        pFCC->SetSubMode(Sms->GetAGBSubMode());

    /*
    if(Sms->Prof1)
    {
     Sms->rippleCount = Sms->Prof1RP;
     Sms->rippleInterval = Sms->Prof1RS;
     Sms->SetPair(Sms->Prof1Pair);
     if(pFCC)
     pFCC->SetSubMode(Sms->Prof1SubMode);
    }
    else
    {
     Sms->rippleCount = Sms->Prof2RP;
     Sms->rippleInterval = Sms->Prof2RS;
     Sms->SetPair(Sms->Prof2Pair);
     if(pFCC)
     pFCC->SetSubMode(Sms->Prof2SubMode);
    }*/

    // 2002-01-28 ADDED BY S.G. Init our stuff to keep the target deaggregated until last missile impact
    //thePrevMissile = NULL;
    // Stupid shit because theMissile isn't inserted into VU (therefore not referenced) until launched
    //thePrevMissileIsRef = FALSE;
    thePrevMissile.reset();

    int l;

    for (l = 0; l < 32; l++)
    {
        sjSelected[l] = JettisonNone;
    }

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

SmsDrawable::~SmsDrawable(void)
{
    vuDatabase->Remove(groundSpot);

    // sfr: this seems quite odd...
    // 2002-01-28 ADDED BY S.G. Clean up our act once we get destroyed
    //if (thePrevMissile and thePrevMissileIsRef){
    // VuDeReferenceEntity((VuEntity *)thePrevMissile);
    //}
    //thePrevMissileIsRef = FALSE;
    //thePrevMissile = NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Find out what to draw

VirtualDisplay* SmsDrawable::GetDisplay(void)
{
    VirtualDisplay* retval = privateDisplay;
    MissileClass* theMissile = (MissileClass*)(Sms->GetCurrentWeapon());
    MaverickDisplayClass* theDisplay;
    float rx, ry, rz;
    Tpoint pos;

    if (Sms->curHardpoint < 0 or displayMode == Off)
    {
        return retval;
    }

    switch (Sms->hardPoint[Sms->curHardpoint]->GetWeaponClass())
    {
        case wcAgmWpn:
            if (Sms->curWeaponType == wtAgm65)
            {
                if (theMissile and theMissile->IsMissile())
                {
                    theDisplay = (MaverickDisplayClass*)theMissile->display;

                    if (theDisplay)
                    {
                        if ( not theDisplay->GetDisplay())
                        {
                            if (privateDisplay)
                            {
                                theDisplay->DisplayInit(((Render2D*)(privateDisplay))->GetImageBuffer());
                            }

                            theDisplay->viewPoint = viewPoint;

                            // Set missile initial position
                            Sms->hardPoint[Sms->curHardpoint]->GetSubPosition(Sms->curWpnNum, &rx, &ry, &rz);
                            rx += 5.0F;
                            pos.x = Sms->ownship->XPos() + Sms->ownship->dmx[0][0] * rx + Sms->ownship->dmx[1][0] * ry +
                                    Sms->ownship->dmx[2][0] * rz;
                            pos.y = Sms->ownship->YPos() + Sms->ownship->dmx[0][1] * rx + Sms->ownship->dmx[1][1] * ry +
                                    Sms->ownship->dmx[2][1] * rz;
                            pos.z = Sms->ownship->ZPos() + Sms->ownship->dmx[0][2] * rx + Sms->ownship->dmx[1][2] * ry +
                                    Sms->ownship->dmx[2][2] * rz;
                            theDisplay->SetXYZ(pos.x, pos.y, pos.z);
                        }

                        retval = theDisplay->GetDisplay();
                    }
                }
            }

            break;

        case wcGbuWpn:
        {
            // Cobra - causes TGP MFD display to disappear in 3D pit
            /*
             SensorClass* laserPod = FindLaserPod (Sms->ownship);

             if (laserPod)
             {
             if ( not laserPod->GetDisplay())
             {
             if (privateDisplay)
             {
              laserPod->DisplayInit(((Render2D*)(privateDisplay))->GetImageBuffer());
             }
             laserPod->viewPoint = viewPoint;
             }
             retval = laserPod->GetDisplay();
             }
            */
        }
        break;

        default:
            if (hadCamera)
                needCamera = TRUE;

            hadCamera = FALSE;
            break;
    }

    return (retval);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::UpdateGroundSpot(void)
{
    int i;
    float rx, ry, rz;
    MissileClass* theMissile = static_cast<MissileClass*>(Sms->GetCurrentWeapon());

    thePrevMissile.reset(theMissile);

    // 2002-01-28 ADDED BY S.G. Keep a note on the previous missile if we currently have no missile selected...
    /*if (thePrevMissile) {
     // If it's a new missile, switch to it
     if (theMissile and (thePrevMissile not_eq theMissile)){
     if (thePrevMissileIsRef){
     VuDeReferenceEntity((VuEntity *)thePrevMissile);
     }
     thePrevMissile = theMissile;
     if (thePrevMissile->RefCount() > 0) {
     VuReferenceEntity((VuEntity *)thePrevMissile);
     thePrevMissileIsRef = TRUE;
     }
     else {
     thePrevMissileIsRef = FALSE;
     }
     }
    }
    // If we have a missile now, keep note of it for later
    else if (theMissile) {
     thePrevMissile = theMissile;
     if (theMissile->RefCount() > 0) {
     VuReferenceEntity((VuEntity *)thePrevMissile);
     thePrevMissileIsRef = TRUE;
     }
    }

    // If our previous missile is now dead
    if (thePrevMissile and (thePrevMissile not_eq theMissile) and thePrevMissile->IsDead()) {
     if (thePrevMissileIsRef)
     VuDeReferenceEntity((VuEntity *)thePrevMissile);
     thePrevMissileIsRef = FALSE;
     thePrevMissile = NULL;
    }

    // Now check if it's time to reference our thePrevMissile
    if (thePrevMissile and thePrevMissileIsRef == FALSE and thePrevMissile->RefCount() > 0) {
     VuReferenceEntity((VuEntity *)thePrevMissile);
     thePrevMissileIsRef = TRUE;
    }

    // If we don't currently have a missile but had one, use it.
    if (thePrevMissile and not theMissile){
     theMissile = thePrevMissile.get();
    }*/
    // END OF ADDED SECTION

    // If a weapon has the spot, use it
    if (Sms->curHardpoint > 0)
    {
        switch (Sms->hardPoint[Sms->curHardpoint]->GetWeaponClass())
        {
            case wcAgmWpn:
                if (Sms->curWeaponType == wtAgm65)
                {
                    if (theMissile and theMissile->IsMissile())
                    {
                        theMissile->GetTargetPosition(&rx, &ry, &rz);

                        if (rx < 0.0F or ry < 0.0F)
                            SetGroundSpotPos(Sms->ownship->XPos(), Sms->ownship->YPos(), Sms->ownship->ZPos());
                        else
                            SetGroundSpotPos(rx, ry, rz);

                        hadCamera = TRUE;
                    }
                }

                break;

            case wcHARMWpn:
                if (theMissile and theMissile->IsMissile())
                {
                    theMissile->GetTargetPosition(&rx, &ry, &rz);

                    if (rx < 0.0F)
                        SetGroundSpotPos(Sms->ownship->XPos(), Sms->ownship->YPos(), Sms->ownship->ZPos());
                    else
                        SetGroundSpotPos(rx, ry, rz);

                    hadCamera = TRUE;
                }

                break;

            case wcGbuWpn:
            {
                SensorClass* laserPod = FindLaserPod(Sms->ownship);

                if (laserPod)
                {
                    ((LaserPodClass*)laserPod)->GetTargetPosition(&rx, &ry, &rz);

                    if (rx < 0.0F or ry < 0.0F)
                        SetGroundSpotPos(Sms->ownship->XPos(), Sms->ownship->YPos(), Sms->ownship->ZPos());
                    else
                        SetGroundSpotPos(rx, ry, rz);

                    hadCamera = TRUE;
                }
            }
            break;

            default:
                if (hadCamera)
                    needCamera = TRUE;

                hadCamera = FALSE;
                break;
        }
    }

    // Add the camera if needed
    // edg: only do it if the display is in the cockpit
    if (needCamera and OTWDriver.DisplayInCockpit())
    {
        needCamera = FALSE;
        groundSpot->EntityDriver()->Exec(vuxGameTime);

        for (i = 0; i < FalconLocalSession->CameraCount(); i++)
        {
            if (FalconLocalSession->GetCameraEntity(i) == groundSpot)
            {
                break;
            }
        }

        if (i == FalconLocalSession->CameraCount())
        {
            FalconLocalSession->AttachCamera(groundSpot);
        }
    }
    else
    {
        // sfr: cleanup camera mess
        FalconLocalSession->RemoveCamera(groundSpot);
        //for (i=0; i<FalconLocalSession->CameraCount(); i++)
        //{
        // if (FalconLocalSession->GetCameraEntity(i) == groundSpot)
        // {
        // FalconLocalSession->RemoveCamera(groundSpot);
        // break;
        // }
        //}
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::SetGroundSpotPos(float x, float y, float z)
{
    F4Assert(groundSpot);

    groundSpot->SetPosition(x, y, z);

    needCamera = TRUE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::DisplayInit(ImageBuffer* image)
{
    DisplayExit();

    privateDisplay = new Render2D;
    ((Render2D*)privateDisplay)->Setup(image);

    privateDisplay->SetColor(0xff00ff00);
    isDisplayed = TRUE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::DisplayExit(void)
{
    isDisplayed = FALSE;

    DrawableClass::DisplayExit();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::Display(VirtualDisplay* newDisplay)
{
    isDisplayed = TRUE; // MLR 3/22/2004 - Because sometimes DisplayInit is NOT called when the SMS is displayed.
    // appears to happen when cycling AG stores

    float cX, cY = 0;
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(playerAC, SensorClass::Radar);

    // sfr: @todo remove this or check why it happens
    if ( not theRadar)
    {
        ShiWarning("Oh Oh shouldn't be here without a radar");
        return;
    }

    theRadar->GetCursorPosition(&cX, &cY);
    display = newDisplay;
    FireControlComputer *FCC = Sms->ownship->GetFCC();
    isJSOW = FALSE;
    isJDAM = FALSE;
    // sfr: no waypoint selected, NULL bc. Had CTDs here.
    BombClass *bc = Sms->CurHardpoint() == -1 ?
                    NULL :
                    static_cast<BombClass*>(Sms->hardPoint[Sms->CurHardpoint()]->weaponPointer.get())
                    ;

    if ((Sms->GetCurrentWeapon() not_eq NULL) and (Sms->GetCurrentHardpoint() > 0) and 
        bc and bc->IsSetBombFlag(BombClass::IsJSOW))
    {
        isJSOW = TRUE;
    }
    else
    {
        isJSOW = FALSE;
    }

    if ((Sms->GetCurrentWeapon() not_eq NULL) and (Sms->GetCurrentHardpoint() > 0) and 
        bc and bc->IsSetBombFlag(BombClass::IsGPS))
    {
        isJDAM = TRUE;
    }
    else
    {
        isJDAM = FALSE;
    }

    // MN get aircrafts maximum ripple count
    maxripple = ((AircraftClass*)(Sms->ownship))->af->GetMaxRippleCount();

    if ( not ((AircraftClass*)(Sms->ownship))->HasPower(AircraftClass::SMSPower))
    {
        if (displayMode not_eq Off)
        {
            lastMode = displayMode;
            displayMode = Off;
        }
    }
    else if (displayMode == Off)
    {
        displayMode = lastMode;
    }

    switch (displayMode)
    {
        case Off:
        {
            display->TextCenter(0.0f, 0.2f, "SMS");  // JPG 14 Dec 03 - Fixed these
            int ofont = display->CurFont();
            display->SetFont(2);
            display->TextCenter(0.0f, 0.0f, "OFF");
            display->SetFont(ofont);
            theRadar->GetCursorPosition(&cX, &cY);

            // JPG 14 Dec 03 - Added BE/ownship info
            if (
                OTWDriver.pCockpitManager and OTWDriver.pCockpitManager->mpIcp and 
                OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo
            )
            {
                DrawBullseyeCircle(display, cX, cY);
            }
            else
            {
                DrawReference(MfdDisplay[OnMFD()]->GetOwnShip());
            }

            BottomRow();
        }
        break;

        case Inv:
            InventoryDisplay(FALSE);
            break;

        case SelJet:
            SelectiveJettison();
            break;

            //MI
        case EmergJet:
            EmergJetDisplay();
            break;

            //MI
        case InputMode:
            InputDisplay();
            break;

        case Wpn:

            // common stuff.
            if (FCC->IsNavMasterMode())  // shouldn't happened really
            {
                SetDisplayMode(Inv);
                InventoryDisplay(FALSE);
            }
            else if (FCC->GetMasterMode() == FireControlComputer::Dogfight)
            {
                DogfightDisplay();
            }
            else if (FCC->GetMasterMode() == FireControlComputer::MissileOverride)
            {
                MissileOverrideDisplay();
            }
            else
            {
                if (Sms->curHardpoint < 0)
                {
                    TopRow(0);
                    LabelButton(5, "AAM");
                    LabelButton(6, "AGM");
                    LabelButton(7, "A-G");
                    LabelButton(8, "GUN");
                    BottomRow();
                }
                else
                {
                    //cobra test
                    if (isJDAM or isJSOW)
                    {
                        JDAMDisplay();
                        break;
                    }

                    switch (Sms->hardPoint[Sms->GetCurrentHardpoint()]->GetWeaponClass())
                    {
                        case wcAimWpn:
                        case wcAgmWpn:
                        case wcSamWpn:
                        case wcHARMWpn:
                            MissileDisplay();
                            break;

                        case wcGunWpn:
                            GunDisplay();
                            break;

                        case wcBombWpn:
                        case wcRocketWpn:
                            BombDisplay();
                            break;

                        case wcGbuWpn:
                            if ( not g_bRealisticAvionics)
                                GBUDisplay();
                            else
                                BombDisplay();

                            break;

                        case wcCamera:
                            CameraDisplay();
                            break;

                        default:
                            TopRow(0);
                            LabelButton(5, "AAM");
                            LabelButton(6, "AGM");
                            LabelButton(7, "A-G");
                            LabelButton(8, "GUN");
                            BottomRow();
                            break;
                    }
                }
            }

            break;
    }

    //Booster 15/09/2004 - Draw Pull Up cross on MFD-SMS if ground Collision
    if (playerAC->mFaults->GetFault(alt_low))
    {
        DrawRedBreak(display);
    }

    // RV - I-Hawk
    // Check, if 1 of the MFDs is showing TGP and the attitude warning is set, show it...
    // (works on all MFDs and not only here)
    for (int i = 0; i < 4; i++)
    {
        if ((MfdDisplay[i])->GetTGPWarning() and (MfdDisplay[i])->CurMode() == MFDClass::TGPMode)
        {
            TGPAttitudeWarning(display);
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::SetDisplayMode(SmsDisplayMode newDisplayMode)
{
    // 2000-08-26 MODIFIED BY S.G. SO WE SAVE THE JETTISON SELECTION
    // hardPointSelected = 0; MOVED WITHIN THE CASE
    // EXCEPT FOR SelJet WERE WE READ oldp01[5] INSTEAD
    switch (newDisplayMode)
    {
        case Inv:
            hardPointSelected = 0;
            break;

        case Wpn:
            hardPointSelected = 0;
            break;

        case SelJet:
            lastMode = displayMode;
            // hardPointSelected = ((AircraftClass *)Sms->ownship)->af->oldp01[5];
            //hardPointSelected = sjHardPointSelected; // Until I fixe oldp01 being private // MLR 3/9/2004 -
            break;
    }

    // hardPointSelected = 0;
    displayMode = newDisplayMode;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::StepDisplayMode(void)
{
    // 2000-08-26 MODIFIED BY S.G. SO WE SAVE THE JETTISON SELECTION
    // hardPointSelected = 0; MOVED WITHIN THE CASE
    // case SelJet ADDED AS WELL
    // EXCEPT FOR SelJet WERE WE READ oldp01[5] INSTEAD
    switch (displayMode)
    {
        case Off:
            break;

        case Wpn:
            displayMode = Inv;
            hardPointSelected = 0;
            break;

        case SelJet:
            // hardPointSelected = ((AircraftClass *)Sms->ownship)->af->oldp01[5];
            // hardPointSelected = sjHardPointSelected; // Until I fixe oldp01 being private // MLR 3/9/2004 -
            break;

        case Inv:
        default:
            displayMode = Wpn;
            hardPointSelected = 0;
            break;
    }

    // hardPointSelected = 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::PushButton(int whichButton, int whichMFD)
{
    switch (displayMode)
    {
        case Off:
            MfdDrawable::PushButton(whichButton, whichMFD);
            break;

        case Wpn:
            WpnPushButton(whichButton, whichMFD);
            break;

        case Inv:
            InvPushButton(whichButton, whichMFD);
            break;

        case SelJet:
            SJPushButton(whichButton, whichMFD);
            break;

            //MI
        case InputMode:
            InputPushButton(whichButton, whichMFD);
            break;
    }
}



// Weapon main handling - handles common cases and then hands off
void SmsDrawable::WpnPushButton(int whichButton, int whichMFD)
{

    FireControlComputer* pFCC = Sms->ownship->GetFCC();

    switch (whichButton)  // common controls.
    {
            //MI
        case 1:
            if (g_bRealisticAvionics)
            {
                if (Sms->curWeaponType == wtAgm65 and Sms->curWeapon)
                {
                    Sms->StepMavSubMode();
                }
                else if (pFCC->GetMasterMode() == FireControlComputer::Dogfight or pFCC->GetMasterMode() ==
                         FireControlComputer::MissileOverride)
                {
                    WpnOtherPushButton(whichButton, whichMFD);
                }
                else if (pFCC->GetMainMasterMode() == MM_AA)
                    WpnAAPushButton(whichButton, whichMFD);
                else if (pFCC->GetMainMasterMode() == MM_AG)
                {
                    if (IsSet(MENUMODE))
                        WpnAGMenu(whichButton, whichMFD);
                    else
                        WpnAGPushButton(whichButton, whichMFD);
                }
            }
            else
            {
                switch (pFCC->GetMainMasterMode())
                {
                    case MM_AG:
                        if (IsSet(MENUMODE))
                            WpnAGMenu(whichButton, whichMFD);
                        else
                            WpnAGPushButton(whichButton, whichMFD);

                        break;

                    case MM_AA:
                        WpnAAPushButton(whichButton, whichMFD);
                        break;

                    case MM_NAV:
                        WpnNavPushButton(whichButton, whichMFD);
                        break;

                    default:
                        WpnOtherPushButton(whichButton, whichMFD);
                        break;
                }
            }

            break;

        case 3:
            UnsetFlag(MENUMODE); // force out of menu mode JPO
            SetDisplayMode(Inv);
            break;

        case 10:
            UnsetFlag(MENUMODE); // force out of menu mode JPO
            SetDisplayMode(SelJet);
            break;

        case 11:
            if (g_bRealisticAvionics)
            {
                MfdDrawable::PushButton(whichButton, whichMFD);
            }

            break;

        case 12:
            if (g_bRealisticAvionics)
            {
                MfdDrawable::PushButton(whichButton, whichMFD);
            }
            else
            {
                MfdDisplay[whichMFD]->SetNewMode(MFDClass::FCRMode);
            }

            break;

        case 13:
            if (g_bRealisticAvionics)
            {
                MfdDrawable::PushButton(whichButton, whichMFD);
            }
            else if (pFCC->GetMasterMode() == FireControlComputer::ILS or
                     pFCC->GetMasterMode() == FireControlComputer::Nav)
            {
                MfdDisplay[whichMFD]->SetNewMode(MFDClass::MfdMenu);
            }
            else
            {
                SetDisplayMode(Wpn);
                pFCC->SetMasterMode(FireControlComputer::Nav);
            }

            break;

        case 14:
            MfdDrawable::PushButton(whichButton, whichMFD);
            break;

            //MI
        case 19:
            if (g_bRealisticAvionics)
            {
                if (Sms->FEDS)
                    Sms->FEDS = FALSE;
                else
                    Sms->FEDS = TRUE;
            }

        default:
            switch (pFCC->GetMainMasterMode())
            {
                case MM_AG:
                    if (IsSet(MENUMODE))
                        WpnAGMenu(whichButton, whichMFD);
                    else
                        WpnAGPushButton(whichButton, whichMFD);

                    break;

                case MM_AA:
                    WpnAAPushButton(whichButton, whichMFD);
                    break;

                case MM_NAV:
                    WpnNavPushButton(whichButton, whichMFD);
                    break;

                default:
                    WpnOtherPushButton(whichButton, whichMFD);
                    break;
            }
    }
}

// Air to Ground buttons
void SmsDrawable::WpnAGPushButton(int whichButton, int whichMFD)
{
    FireControlComputer* pFCC = Sms->ownship->GetFCC();
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    AircraftClass *self = ((AircraftClass*)playerAC);

    switch (whichButton)
    {
        case 0:   // in AG mode, toggle between guns and bombs
            pFCC->ToggleAGGunMode();
            break;

        case 1:
            if (g_bRealisticAvionics)
            {
                //MI temporary until we can get the LGB's going
                if (pFCC->GetMasterMode() == FireControlComputer::AirGroundLaser  or
                    pFCC->GetMasterMode() == FireControlComputer::AirGroundRocket or // MLR 4/3/2004 -
                    pFCC->GetMasterMode() == FireControlComputer::AGGun)//           or // MLR 4/3/2004 -
                    //pFCC->GetSubMode()    == FireControlComputer::OBSOLETERCKT) // MLR 4/3/2004 -
                    break;


                ToggleFlag(MENUMODE);
            }
            else
                pFCC->NextSubMode();

            break;

        case 2:
            if (Sms->curWeaponType == wtAgm65 and Sms->curWeapon)
            {
                ShiAssert(Sms->curWeapon->IsMissile());
                MaverickDisplayClass* mavDisplay =
                    (MaverickDisplayClass*)((MissileClass*)Sms->GetCurrentWeapon())->display;

                if (mavDisplay)
                {
                    mavDisplay->ToggleFOV();
                }
            }
            else if (Sms->curWeaponClass == wcGbuWpn)
            {
                LaserPodClass* laserPod = (LaserPodClass*)FindLaserPod(Sms->ownship);

                if (laserPod)
                {
                    laserPod->ToggleFOV();
                }
            }

            break;

            //MI
        case 4:
            if (g_bRealisticAvionics)
            {
                if ((pFCC->GetMasterMode() == FireControlComputer::AirGroundBomb) or //me123 status test. addet four lines
                    (pFCC->GetMasterMode() == FireControlComputer::AirGroundRocket) or // MLR 4/3/2004 -
                    (pFCC->GetMasterMode() == FireControlComputer::AirGroundLaser))
                {
                    ChangeToInput(whichButton);
                }
            }

            break;

        case 5:
            Sms->StepAGWeapon(); // MLR 2/8/2004 - changed to use the new step code
            //Sms->GetNextWeapon(wdGround);

            //MI
            if (g_bRealisticAvionics)
            {
                SetWeapParams();
            }

            break;

            //MI
        case 6:
            if (isJDAM or isJSOW)
            {
                if ( not Sms->JDAMPowered and Sms->curWeapon)
                {
                    Sms->JDAMPowered = TRUE;
                    break;
                }
                else if (Sms->JDAMPowered and not Sms->curWeapon)
                {
                    Sms->JDAMPowered = FALSE;
                    break;
                }
                else
                {
                    Sms->JDAMPowered = FALSE;
                    break;
                }
            }

            if ( not g_bRealisticAvionics or Sms->CurHardpoint() < 0)
                break;

            if ((pFCC->GetMasterMode() == FireControlComputer::AirGroundBomb) or //me123 status test. addet four lines
                (pFCC->GetMasterMode() == FireControlComputer::AirGroundLaser))
            {
                ChangeProf();
                SetWeapParams();
            }
            else if (pFCC->GetMasterMode() == FireControlComputer::AirGroundMissile and 
                     Sms->curWeaponType == wtAgm65)
            {
                Sms->ToggleMavPower();
            }

            // RV - I-Hawk - HARM power
            else if (pFCC->GetMasterMode() == FireControlComputer::AirGroundHARM and 
                     Sms->curWeaponType == wtAgm88)
            {
                Sms->ToggleHARMPower();
            }

            break;

        case 7:
            if (g_bRealisticAvionics and Sms->CurHardpoint() >= 0) //MI
            {
                if ((pFCC->GetMasterMode() == FireControlComputer::AirGroundBomb) or  //me123 status test. addet four lines
                    (pFCC->GetMasterMode() == FireControlComputer::AirGroundLaser) or
                    pFCC->GetMasterMode() == FireControlComputer::AirGroundRocket)
                {
                    Sms->SetAGBPair( not Sms->GetAGBPair());
                }
            }
            else
            {
                if (pFCC->GetMasterMode() == FireControlComputer::AirGroundBomb) //me123 status test. addet four lines
                {
                    Sms->SetAGBPair( not Sms->GetAGBPair());
                    /*
                    if (Sms->pair)
                     Sms->SetPair(FALSE);
                    else
                     Sms->SetPair(TRUE);
                    */
                }
            }

            break;

        case 8:
            if (g_bRealisticAvionics and Sms->CurHardpoint() >= 0) //MI
            {
                if ((pFCC->GetMasterMode() == FireControlComputer::AirGroundBomb) or //me123 status test. addet four lines
                    (pFCC->GetMasterMode() == FireControlComputer::AirGroundLaser))
                {
                    ChangeToInput(whichButton);
                }
            }
            else
            {
                if (pFCC->GetMasterMode() == FireControlComputer::AirGroundBomb) //me123 status test. addet four lines
                    Sms->IncrementRippleInterval();
            }

            break;

        case 9:
            if (g_bRealisticAvionics and Sms->CurHardpoint() >= 0) //MI
            {
                if (pFCC->GetMasterMode() == FireControlComputer::AirGroundMissile)
                    pFCC->WeaponStep();
                else if ((pFCC->GetMasterMode() == FireControlComputer::AirGroundBomb) or //me123 status test. addet four lines
                         (pFCC->GetMasterMode() == FireControlComputer::AirGroundLaser))
                {
                    ChangeToInput(whichButton);
                }
            }
            else
            {
                if (pFCC->GetMasterMode() == FireControlComputer::AirGroundMissile)
                    pFCC->WeaponStep();
                else if (pFCC->GetMasterMode() == FireControlComputer::AirGroundBomb)
                    Sms->IncrementRippleCount();
            }

            break;

        case 15:
            if (g_bRealisticAvionics and Sms->CurHardpoint() >= 0) //MI
            {
                if (pFCC->GetMasterMode() == FireControlComputer::AirGroundMissile)
                    pFCC->WeaponStep();
                else if ((pFCC->GetMasterMode() == FireControlComputer::AirGroundBomb) or //me123 status test. addet four lines
                         (pFCC->GetMasterMode() == FireControlComputer::AirGroundLaser)) //MI LGB's
                {
                    if (g_bMLU)
                        ChangeToInput(whichButton);
                    else
                    {
                        self->JDAMStep = -1;
                    }
                }
            }
            else
            {
                if (pFCC->GetMasterMode() == FireControlComputer::AirGroundMissile)
                    pFCC->WeaponStep();
                else if (pFCC->GetMasterMode() == FireControlComputer::AirGroundBomb) //me123 status test. addet four lines
                    Sms->Incrementarmingdelay();
            }

            break;

        case 16:

            //JDAM
            if (isJDAM or isJSOW)
            {
                self->JDAMStep = 1;
            }

            break;

            //MI
        case 17:
            if (g_bRealisticAvionics and Sms->CurHardpoint() >= 0)
            {
                if (pFCC->GetMasterMode() == FireControlComputer::AirGroundBomb)
                {
                    int i; // MLR 4/3/2004 -

                    i = Sms->GetAGBFuze() + 1;

                    if (i > 2)
                        i = 0;

                    Sms->SetAGBFuze(i);

                    /*
                    if(Sms->Prof1)
                    {
                     Sms->Prof1NSTL++;
                     if(Sms->Prof1NSTL > 2)
                     Sms->Prof1NSTL = 0;
                    }
                    else
                    {
                     Sms->Prof2NSTL++;
                     if(Sms->Prof2NSTL > 2)
                     Sms->Prof2NSTL = 0;
                    }*/
                    SetWeapParams();
                }
            }

            break;

        case 18:
            if (isJDAM or isJSOW)
            {
                if (Sms->JDAMtargeting == SMSBaseClass::PB)
                {
                    Sms->JDAMtargeting = SMSBaseClass::TOO;
                    // RV - Biker - If we have a locked target on laser pod go to TOO
                    LaserPodClass* laserPod = (LaserPodClass*)FindLaserPod(Sms->ownship);

                    if (laserPod and laserPod->IsLocked())
                        laserPod->SetDesiredTarget(NULL);

                    // RV - Biker - Only allow auto step when in PB
                    self->JDAMAllowAutoStep = false;
                    break;
                }
                else
                {
                    Sms->JDAMtargeting = SMSBaseClass::PB;
                    self->JDAMAllowAutoStep = true;
                    break;
                }
            }

            if (Sms->curHardpoint >= 0 and Sms->hardPoint[Sms->curHardpoint]->GetWeaponData()->flags bitand SMSClass::HasBurstHeight)
            {
                //MI not here in real
                if (g_bRealisticAvionics)
                    break;

                Sms->IncrementBurstHeight();
            }
            //I-Hawk - Moved this to WpnMfd
            //else if (Sms->curWeaponType == wtAgm88) {
            // // HTS used HSD display range for now, so...
            // //SimHSDRangeStepDown (0, KEY_DOWN, NULL);
            //}

            else if (g_bRealisticAvionics and pFCC->GetMasterMode() == FireControlComputer::AirGroundMissile and Sms->curWeaponType == wtAgm65)
                pFCC->WeaponStep();

            break;

        case 19:

            //MI
            if (g_bRealisticAvionics)
            {
                if (pFCC->GetMasterMode() == FireControlComputer::AirGroundMissile)
                {
                    //Doc says it's been deleted from the real thing
                    //pFCC->NextSubMode();
                }
                //I-Hawk - Moved this to WpnMfd
                //else if (Sms->curWeaponType == wtAgm88)
                //{
                // // HTS used HSD display range for now, so...
                // //SimHSDRangeStepUp (0, KEY_DOWN, NULL);
                //}

                else if (isJDAM or isJSOW)
                {
                    //RV - I-Hawk - toggle status of the JDAMAllowAutoStep flag
                    if (Sms->JDAMtargeting == SMSBaseClass::PB)
                    {
                        if (self->JDAMAllowAutoStep)
                        {
                            self->JDAMAllowAutoStep = false;
                        }
                        else
                        {
                            self->JDAMAllowAutoStep = true;
                        }
                    }
                    // RV - Biker - Only allow auto step when in PB
                    else
                    {
                        self->JDAMAllowAutoStep = false;
                    }
                }

                break;
            }

            if (pFCC->GetMasterMode() == FireControlComputer::AirGroundMissile or
                (pFCC->GetMasterMode() == FireControlComputer::AirGroundLaser and 
                 Sms->curWeaponClass == wcGbuWpn))
            {
                pFCC->NextSubMode();
            }

            //I-Hawk - Moved this to WpnMfd
            //else if (Sms->curWeaponType == wtAgm88)
            //{
            // // HTS used HSD display range for now, so...
            // //SimHSDRangeStepUp (0, KEY_DOWN, NULL);
            //}
            break;
    }
}

// Handle Menu mode
void SmsDrawable::WpnAGMenu(int whichButton, int whichMFD)
{
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    RadarDopplerClass* pradar = (RadarDopplerClass*) FindSensor(playerAC, SensorClass::Radar);
    FireControlComputer* pFCC = Sms->ownship->GetFCC();

    if (pradar) //MI fix
        pradar->SetScanDir(1.0F);

    switch (whichButton)
    {
        case 19:
            pFCC->SetSubMode(FireControlComputer::CCIP); // MLR 4/4/2004 - SetSubMode calls Sms->SetAGBSubMode()
            break;

        case 18:
            pFCC->SetSubMode(FireControlComputer::CCRP);
            pradar->SelectLastAGMode();
            break;

        case 17:
            pFCC->SetSubMode(FireControlComputer::DTOSS);
            break;

        case 16:
            pFCC->SetSubMode(FireControlComputer::LADD);
            break;

        case 15:
            pFCC->SetSubMode(FireControlComputer::MAN);
            break;

        default:
            return; // nothing interesting
    }

    UnsetFlag(MENUMODE);
}

// Handle other cases - basically missile override and dogfight.
void SmsDrawable::WpnOtherPushButton(int whichButton, int whichMFD)
{
    FireControlComputer* pFCC = Sms->ownship->GetFCC();

    switch (whichButton)
    {
        case 0:
            break;

        case 1:
            if (pFCC->GetMasterMode() == FireControlComputer::Dogfight and g_bRealisticAvionics)
            {
                pFCC->NextSubMode(); // MLR 4/2/2004 -
                break;
            }

            break;

        case 5: // MLR 2/8/2004 - simplified
            if (pFCC->GetMasterMode() == FireControlComputer::Dogfight)
                break;

            Sms->StepAAWeapon();
            break;

        case 6: // MLR 2/8/2004 - simplified
            if (pFCC->GetMasterMode() not_eq FireControlComputer::Dogfight)
                break;

            Sms->StepAAWeapon();
            break;

        case 9:
        case 15:
            pFCC->WeaponStep();
            break;

        default:
            WpnAAMissileButton(whichButton, whichMFD);
            break;
    }
}

// Navigation mode - should never happen
void SmsDrawable::WpnNavPushButton(int whichButton, int whichMFD)
{
    MonoPrint("Sms Weapon in Nav mode\n");
    // shouldn't happen.
}

// AA mode.
void SmsDrawable::WpnAAPushButton(int whichButton, int whichMFD)
{
    FireControlComputer* pFCC = Sms->ownship->GetFCC();

    switch (whichButton)
    {
        case 0: // in AA between guns and missiles.
            pFCC->ToggleAAGunMode();

            break;

        case 1:
            if (pFCC->GetMasterMode() == FireControlComputer::AAGun)
            {
                pFCC->NextSubMode();
            }

            break;

        case 5: // MLR 2/8/2004 - simplified
            if (pFCC->GetMasterMode() == FireControlComputer::Dogfight)
                break;

            Sms->StepAAWeapon();
            break;

            break;

        case 9:
        case 15:
            if (pFCC->GetMasterMode() == FireControlComputer::Missile)
                pFCC->WeaponStep();

            break;

        default:
            if (pFCC->GetMasterMode() == FireControlComputer::Missile)
                WpnAAMissileButton(whichButton, whichMFD);

            break;
    }
}

void SmsDrawable::SJPushButton(int whichButton, int whichMFD)
{

    FireControlComputer* pFCC = Sms->ownship->GetFCC();

    switch (whichButton)
    {
        case 2:
            if (Sms->hardPoint[5]->GetWeaponClass() not_eq wcECM and 
                Sms->hardPoint[5]->GetWeaponClass() not_eq wcCamera)
            {
                StepSelectiveJettisonMode(5);
                //          hardPointSelected xor_eq (1 << 5);
                // sjHardPointSelected = hardPointSelected;
            }

            break;

        case 6:
            StepSelectiveJettisonMode(6);
            // hardPointSelected xor_eq (1 << 6);
            // sjHardPointSelected = hardPointSelected;
            break;

        case 7:
            StepSelectiveJettisonMode(7);
            // hardPointSelected xor_eq (1 << 7);
            // sjHardPointSelected = hardPointSelected;
            break;

        case 8:
            StepSelectiveJettisonMode(8);
            // hardPointSelected xor_eq (1 << 8);
            // sjHardPointSelected = hardPointSelected;
            break;

        case 10:
        {
            SetDisplayMode(lastMode);
        }
        break;

        case 11:
            if (g_bRealisticAvionics)
            {
                MfdDrawable::PushButton(whichButton, whichMFD);
            }

            break;

        case 12:
            if (g_bRealisticAvionics)
            {
                MfdDrawable::PushButton(whichButton, whichMFD);
            }
            else
            {
                SetDisplayMode(Wpn);
            }

            break;

        case 13:
            if (g_bRealisticAvionics)
            {
                MfdDrawable::PushButton(whichButton, whichMFD);
            }
            else if (pFCC->GetMasterMode() == FireControlComputer::ILS or
                     pFCC->GetMasterMode() == FireControlComputer::Nav)
            {
                SetDisplayMode(Wpn);
                pFCC->SetMasterMode(FireControlComputer::Nav);
            }
            else
            {
                SetDisplayMode(Wpn);
                pFCC->SetMasterMode(FireControlComputer::Nav);
            }

            break;

        case 14:
            MfdDrawable::PushButton(whichButton, whichMFD);
            break;

        case 15:
            break;

        case 16:
            StepSelectiveJettisonMode(2);
            //     hardPointSelected xor_eq (1 << 2);
            // sjHardPointSelected = hardPointSelected;
            break;

        case 17:
            StepSelectiveJettisonMode(3);
            //     hardPointSelected xor_eq (1 << 3);
            // sjHardPointSelected = hardPointSelected;
            break;

        case 18:
            StepSelectiveJettisonMode(4);
            //     hardPointSelected xor_eq (1 << 4);
            // sjHardPointSelected = hardPointSelected;
            break;

        case 19:
            break;
    }

    //MI commented out to fix SJ remembering stuff
    //sjHardPointSelected = hardPointSelected;
    // 2000-08-26 ADDED BY S.G. SO WE SAVE THE JETTISON SELECTION
    // if (displayMode == SelJet) // Until I fixe oldp01 being private
    // ((AircraftClass *)Sms->ownship)->af->oldp01[5] = hardPointSelected;
}

// MLR 3/9/2004 -
void SmsDrawable::StepSelectiveJettisonMode(int hp)
{
    int bit = 1 << hp;

    //if(Sms->hardPoint[hp]->GetRackDataFlags() bitand RDF_BMSDEFINITION)
    {
        switch (sjSelected[hp])
        {
            case JettisonNone:
                sjSelected[hp] = SelectiveWeapon;

                if ((Sms->hardPoint[hp]->GetRackDataFlags() bitand RDF_SELECTIVE_JETT_WEAPON))
                    break;

            case SelectiveWeapon:
                sjSelected[hp] = SelectiveRack;

                if ((Sms->hardPoint[hp]->GetRackDataFlags() bitand RDF_SELECTIVE_JETT_RACK))
                    break;

            case SelectiveRack:
            default:
                sjSelected[hp] = JettisonNone;
                break;
        }
    }
    /*
    else
    {   // SP3 data compatible
     switch(sjSelected[hp])
     {
     case JettisonNone:
     sjSelected[hp] = SelectiveRack;
     break;
     case SelectiveRack:
     default:
     sjSelected[hp] = JettisonNone;
     break;
     }
    }
    */
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::DogfightDisplay(void)
{
    GunDisplay();
    ShowMissiles(6);
    // Marco Edit - SLAVE/BORE Mode
    SimWeaponClass *wpn = Sms->GetCurrentWeapon();

    if ( not wpn) return; // prevent CTD

    //MI 29/01/02 changed to be more accurate
    if (g_bRealisticAvionics)
    {
        ShiAssert(Sms->curWeapon and Sms->curWeapon->IsMissile());

        if (Sms->curWeaponType == wtAim9 and ((CampBaseClass*)wpn)->GetSPType() not_eq SPTYPE_AIM9P)
        {
            // JPO new Aim9 code
            switch (Sms->GetCoolState())
            {
                case SMSClass::WARM:
                    LabelButton(7, "WARM");  // needs cooling
                    break;

                case SMSClass::COOLING:
                    LabelButton(7, "WARM", NULL, 1);  // is cooling
                    break;

                case SMSClass::COOL:
                    LabelButton(7, "COOL");
                    break;

                case SMSClass::WARMING:
                    LabelButton(7, "COOL", NULL, 1);  // Is warming back up
                    break;
            }

            if (((MissileClass*)Sms->GetCurrentWeapon())->isSlave)
            {
                LabelButton(18, "SLAV");
            }
            else
            {
                LabelButton(18, "BORE");
            }

            // Marco Edit - TD/BP Mode
            if (((MissileClass*)Sms->GetCurrentWeapon())->isTD and ((CampBaseClass*)wpn)->GetSPType() not_eq SPTYPE_AIM9P)
            {
                LabelButton(17, "TD");
            }
            else if (((CampBaseClass*)wpn)->GetSPType() not_eq SPTYPE_AIM9P)
            {
                LabelButton(17, "BP");
            }

            // Marco Edit - SPOT/SCAN Mode
            if (((MissileClass*)Sms->GetCurrentWeapon())->isSpot and 
                ((CampBaseClass*)wpn)->GetSPType() not_eq SPTYPE_AIM9P
               )
            {
                LabelButton(2, "SPOT");
            }
            else if (((CampBaseClass*)wpn)->GetSPType() not_eq SPTYPE_AIM9P)
            {
                LabelButton(2, "SCAN");
            }
        }
        else if (Sms->curWeaponType == wtAim120)
        {
            if (Sms->curWeaponType == wtAim120 and 
                Sms->MasterArm() == SMSBaseClass::Safe or
                Sms->MasterArm() == SMSBaseClass::Sim)
            {
                // JPO new AIM120 code
                LabelButton(7, "BIT");
                LabelButton(8, "ALBIT");
            }

            char idnum[10];
            sprintf(idnum, "%d", Sms->AimId());
            LabelButton(16, "ID", idnum);
            LabelButton(17, "TM", "OFF");   // JPG 12 Jan 03 - ON is for telemetry missile

            if (Sms and Sms->GetCurrentWeapon() and ((MissileClass*)Sms->GetCurrentWeapon())->isSlave)
            {
                LabelButton(18, "SLAV");
            }
            else
            {
                LabelButton(18, "BORE");
            }
        }
    }
    // AIM9Ps only have Bore/Slave to worry about
    else if (Sms->curWeapon and ((CampBaseClass*)wpn)->GetSPType() == SPTYPE_AIM9P)
    {
        ShiAssert(Sms->curWeapon->IsMissile());

        if (((MissileClass*)Sms->GetCurrentWeapon())->isSlave)
        {
            LabelButton(18, "SLAV");
        }
        else
        {
            LabelButton(18, "BORE");
        }
    }
    else if (Sms->curWeapon and ((CampBaseClass*)wpn)->GetSPType() not_eq SPTYPE_AIM9P)
    {
        // Keep non-full avionics mode as is display wise
        ShiAssert(Sms->curWeapon->IsMissile());

        if (((MissileClass*)Sms->GetCurrentWeapon())->isCaged)
        {
            LabelButton(18, "SLAV");
        }
        else
        {
            LabelButton(18, "BORE");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::MissileOverrideDisplay(void)
{
    TopRow(0);
    BottomRow();
    //MI 19/01/02
    // JPO ID and Telemetry readouts.
    // Marco Edit - SLAVE/BORE Mode
    SimWeaponClass* wpn = NULL;
    ShiAssert(Sms);
    wpn = Sms->GetCurrentWeapon();

    //MI changed 29/01/02 to be more accurate
    if (g_bRealisticAvionics)
    {
        if (Sms->curWeaponType == wtAim120)
        {
            char idnum[10];
            sprintf(idnum, "%d", Sms->AimId());
            LabelButton(16, "ID", idnum);
            LabelButton(17, "TM", "OFF");  // 20 Jan 04 ON is for telemetry missile

            if (Sms->curWeapon)
            {
                ShiAssert(Sms->curWeapon->IsMissile());

                if (((MissileClass*)Sms->GetCurrentWeapon())->isSlave) // and Sms->curWeaponType == wtAim9)
                {
                    LabelButton(18, "SLAV");
                }
                else // if (((MissileClass*)Sms->curWeapon)->isCaged)
                {
                    LabelButton(18, "BORE");
                }
            }
        }
        else if (Sms->curWeaponType == wtAim9 and wpn and ((CampBaseClass*)wpn)->GetSPType() not_eq SPTYPE_AIM9P)
        {
            ShiAssert(Sms->curWeapon and Sms->curWeapon->IsMissile());

            // JPO new Aim9 code
            switch (Sms->GetCoolState())
            {
                case SMSClass::WARM:
                    LabelButton(7, "WARM");  // needs cooling
                    break;

                case SMSClass::COOLING:
                    LabelButton(7, "WARM", NULL, 1);  // is cooling
                    break;

                case SMSClass::COOL:
                    LabelButton(7, "COOL");
                    break;

                case SMSClass::WARMING:
                    LabelButton(7, "COOL", NULL, 1);  // Is warming back up
                    break;
            }

            if (Sms and Sms->GetCurrentWeapon() and ((MissileClass*)Sms->GetCurrentWeapon())->isSlave)
            {
                LabelButton(18, "SLAV");
            }
            else
            {
                LabelButton(18, "BORE");
            }

            // Marco Edit - TD/BP Mode
            if (Sms and Sms->curWeapon and ((MissileClass*)Sms->GetCurrentWeapon())->isTD and wpn and ((CampBaseClass*)wpn)->GetSPType() not_eq SPTYPE_AIM9P)
            {
                LabelButton(17, "TD");
            }
            else if (wpn and ((CampBaseClass*)wpn)->GetSPType() not_eq SPTYPE_AIM9P)
            {
                LabelButton(17, "BP");
            }

            // Marco Edit - SPOT/SCAN Mode
            if (Sms and Sms->curWeapon and ((MissileClass*)Sms->GetCurrentWeapon())->isSpot and wpn and ((CampBaseClass*)wpn)->GetSPType() not_eq SPTYPE_AIM9P)
            {
                LabelButton(2, "SPOT");
            }
            else if (wpn and ((CampBaseClass*)wpn)->GetSPType() not_eq SPTYPE_AIM9P)
            {
                LabelButton(2, "SCAN");
            }
        }
        else if (Sms->curWeaponType == wtGuns)
        {
            GunDisplay();
        }
    }

    //MI
    if (g_bRealisticAvionics)
    {
        if (Sms->curWeaponType not_eq wtGuns)
            ShowMissiles(5);
    }
    else
        ShowMissiles(5);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::MissileDisplay(void)
{
    display->SetColor(GetMfdColor(MFD_LABELS));

    switch (Sms->curWeaponType)
    {
        case wtAgm65:

            //MI
            if ( not g_bRealisticAvionics)
                MaverickDisplay();
            else
                MavSMSDisplay();

            break;

        case wtAgm88:
            HarmDisplay();
            break;

        case wtAim9:
        case wtAim120:
            TopRow(0);
            BottomRow();
            AAMDisplay();
            break;
    }

    ShowMissiles(5);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
char SmsDrawable::HdptStationSym(int n) // JPO new routine
{
    if (Sms->hardPoint[n] == NULL) return ' '; // empty hp

    if (Sms->hardPoint[n]->weaponCount <= 0) return ' '; //MI don't bother drawing empty hardpoints

    if (Sms->StationOK(n) == FALSE) return 'F'; // malfunction on  HP

    if (Sms->hardPoint[n]->GetWeaponType() == Sms->curWeaponType)
        return '0' + n; // exact match for weapon

    if (Sms->hardPoint[n]->GetWeaponClass() == Sms->curWeaponClass)
        return 'M'; // weapon of similar class

    return ' ';
}

void SmsDrawable::ShowMissiles(int buttonNum)
{
    char tmpStr[12];
    float leftEdge;
    int i;
    float width = display->TextWidth("M ");

    if (Sms->curHardpoint < 0)
        return;

    sprintf(tmpStr, "%d%s", Sms->NumCurrentWpn(), Sms->hardPoint[Sms->curHardpoint]->GetWeaponData()->mnemonic);
    ShiAssert(strlen(tmpStr) < sizeof(tmpStr));
    LabelButton(buttonNum, tmpStr);

    if (Sms->CurStationOK())
    {
        switch (Sms->MasterArm())
        {
            case SMSBaseClass::Safe:

                //MI not here in real
                if ( not g_bRealisticAvionics)
                    sprintf(tmpStr, "SAF");
                else
                    sprintf(tmpStr, "");

                break;

            case SMSBaseClass::Sim:
                sprintf(tmpStr, "SIM");
                break;

            case SMSBaseClass::Arm:
                sprintf(tmpStr, "RDY");
                break;
        }
    }
    else
    {
        sprintf(tmpStr, "MAL");
    }

    float x, y, y1;
    GetButtonPos(buttonNum, &x, &y);

    //MI
    if (g_bRealisticAvionics and Sms->curWeaponType == wtAgm65 and Sms->curWeapon)
    {
        if ( not Sms->Powered or Sms->MavCoolTimer > 0.0F)
            sprintf(tmpStr, "");
    }

    display->TextCenter(0.3F, y, tmpStr);
    GetButtonPos(9, &x, &y); // use button 9 as a reference (lower rhs)
    GetButtonPos(8, &x, &y1); // get button 8 y loc
    y = y + fabs(y - y1) * 0.25f; // move up

    // List stations with Current Missile
    for (i = 0; i < Sms->NumHardpoints(); i++)
    {
        // JPO redone to use this routine. Returns the appropriate character
        char c = HdptStationSym(i);

        if (c == ' ') continue; // Don't bother drawing blanks.

        tmpStr[0] = c;
        tmpStr[1] = '\0';

        if (i < 6)
        {
            leftEdge = -x + width * (i - 1);
            display->TextLeft(leftEdge, y, tmpStr, (Sms->curHardpoint == i ? 2 : 0));
        }
        else
        {
            leftEdge = x - width * (Sms->NumHardpoints() - i - 1);
            // Box the current station
            display->TextRight(leftEdge, y, tmpStr, (Sms->curHardpoint == i ? 2 : 0));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::MaverickDisplay(void)
{
    FireControlComputer* pFCC = Sms->ownship->GetFCC();
    MaverickDisplayClass* mavDisplay = NULL;
    //MI
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    AircraftClass *self = ((AircraftClass*)playerAC);

    if ( not self)
        return;

    float yOffset;
    float percentRange;
    float rMax, rMin;
    float range;
    float dx, dy, dz;

    if (Sms->curWeapon)
    {
        ShiAssert(Sms->curWeapon->IsMissile());
        mavDisplay = (MaverickDisplayClass*)((MissileClass*)Sms->GetCurrentWeapon())->display;

        if (mavDisplay)
        {
            mavDisplay->SetIntensity(GetIntensity());
            mavDisplay->viewPoint = viewPoint;
            mavDisplay->Display(display);
        }

        if (mavDisplay->IsSOI())
            LabelButton(1, "VIS", "");
        else
            LabelButton(1, "PRE", "");
    }

    // OSS Button Labels
    LabelButton(0, "OPER");
    //MI
    //if (mavDisplay and mavDisplay->CurFOV() > (3.0F * DTR))

    // RV - Biker - Make FOV switching this dynamic
    //if (mavDisplay and mavDisplay->CurFOV() > (3.5F * DTR))
    float ZoomMin;
    float ZoomMax;

    if ((MissileClass*)Sms->GetCurrentWeapon() and ((MissileClass*)Sms->GetCurrentWeapon())->GetEXPLevel() > 0 and ((MissileClass*)Sms->GetCurrentWeapon())->GetFOVLevel() > 0)
    {
        ZoomMin = ((MissileClass*)Sms->GetCurrentWeapon())->GetFOVLevel();
        ZoomMax = ((MissileClass*)Sms->GetCurrentWeapon())->GetEXPLevel();
    }
    else
    {
        // This should work for old values
        ZoomMin = 3.0f;
        ZoomMax = 6.0f;
    }

    if (mavDisplay and mavDisplay->CurFOV() > 12.0f / (ZoomMax - (ZoomMax - ZoomMin) / 2.0f) * DTR)
        LabelButton(2, "FOV");
    else
        LabelButton(2, "EXP", NULL, 1);

    LabelButton(4, "HOC");
    BottomRow();
    LabelButton(19, pFCC->subModeString);

    // Mav DLZ
    if (pFCC->missileTarget)
    {
        // Range Carat / Closure
        rMax   = pFCC->missileRMax;
        rMin   = pFCC->missileRMin;

        // get range to ground designaate point
        dx = Sms->ownship->XPos() - pFCC->groundDesignateX;
        dy = Sms->ownship->YPos() - pFCC->groundDesignateY;
        dz = Sms->ownship->ZPos() - pFCC->groundDesignateZ;
        range = (float)sqrt(dx * dx + dy * dy + dz * dz);

        // Normailze the ranges for DLZ display
        percentRange = range / pFCC->missileWEZDisplayRange;
        rMin /= pFCC->missileWEZDisplayRange;
        rMax /= pFCC->missileWEZDisplayRange;

        // Clamp in place
        rMin = min(rMin, 1.0F);
        rMax = min(rMax, 1.0F);

        // Draw the symbol

        // Rmin/Rmax
        display->Line(0.9F, -0.8F + rMin * 1.6F, 0.95F, -0.8F + rMin * 1.6F);
        display->Line(0.9F, -0.8F + rMin * 1.6F, 0.9F,  -0.8F + rMax * 1.6F);
        display->Line(0.9F, -0.8F + rMax * 1.6F, 0.95F, -0.8F + rMax * 1.6F);

        // Range Caret
        yOffset = min(-0.8F + percentRange * 1.6F, 1.0F);

        display->Line(0.9F, yOffset, 0.9F - 0.03F, yOffset + 0.03F);
        display->Line(0.9F, yOffset, 0.9F - 0.03F, yOffset - 0.03F);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::GBUDisplay(void)
{
    flash = (vuxRealTime bitand 0x200);

    LaserPodClass* laserPod = (LaserPodClass*)FindLaserPod(Sms->ownship);
    FireControlComputer* pFCC = Sms->ownship->GetFCC();

    if (laserPod)
    {
        laserPod->SetIntensity(GetIntensity());
        laserPod->Display(display);
    }

    // OSS Button Labels
    LabelButton(0, "OPER");

    //MI
    //if (laserPod and laserPod->CurFOV() < (3.0F * DTR))
    if ( not g_bRealisticAvionics)
    {
        if (laserPod and laserPod->CurFOV() < (3.5F * DTR))
            LabelButton(2, "EXP", NULL, 1);
        else
            LabelButton(2, "FOV");
    }
    else
    {
        if (laserPod and laserPod->CurFOV() < (3.5F * DTR))
            LabelButton(2, "NARO", NULL);
        else
            LabelButton(2, "WIDE", NULL);
    }

    BottomRow();
    LabelButton(19, pFCC->subModeString, NULL);

    // Show Available wpns
    ShowMissiles(5);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::AAMDisplay(void)
{
    // Marco Edit - SLAVE/BORE Mode
    SimWeaponClass* wpn = NULL;
    wpn = Sms->GetCurrentWeapon();

    ShiAssert(wpn not_eq NULL);

    // MN return for now if no weapon found...CTD "fix"
    if ( not wpn)
        return;

    if (Sms->curWeaponType == wtAim120 and 
        Sms->MasterArm() == SMSBaseClass::Safe or
        Sms->MasterArm() == SMSBaseClass::Sim)   // JPO new AIM120 code
    {
        LabelButton(7, "BIT");
        LabelButton(8, "ALBIT");
    }
    else if (Sms->curWeaponType == wtAim9 and ((CampBaseClass*)wpn)->GetSPType() not_eq SPTYPE_AIM9P)
    {
        // JPO new Aim9 code
        switch (Sms->GetCoolState())
        {
            case SMSClass::WARM:
                LabelButton(7, "WARM");  // needs cooling
                break;

            case SMSClass::COOLING:
                LabelButton(7, "WARM", NULL, 1);  // is cooling
                break;

            case SMSClass::COOL:
                LabelButton(7, "COOL");
                break;

            case SMSClass::WARMING:
                LabelButton(7, "COOL", NULL, 1);  // Is warming back up
                break;
        }
    }

    // JPO ID and Telemetry readouts.
    if (Sms->curWeaponType == wtAim120)
    {
        char idnum[10];
        sprintf(idnum, "%d", Sms->AimId());
        LabelButton(16, "ID", idnum);
        LabelButton(17, "TM", "OFF");  // JPG 20 Jan 04 - telemetry OFF
    }

    if (g_bRealisticAvionics and Sms->curWeapon)
    {
        ShiAssert(Sms->curWeapon->IsMissile());

        if (((MissileClass*)Sms->GetCurrentWeapon())->isSlave and ((CampBaseClass*)wpn)->GetSPType() not_eq SPTYPE_AIM9P) // and Sms->curWeaponType == wtAim9)
        {
            LabelButton(18, "SLAV");
        }
        else if (Sms->curWeaponType not_eq wtAim9 or ((CampBaseClass*)wpn)->GetSPType() not_eq SPTYPE_AIM9P)
        {
            LabelButton(18, "BORE");
        }

        // Marco Edit - TD/BP Mode
        if (((MissileClass*)Sms->GetCurrentWeapon())->isTD and Sms->curWeaponType == wtAim9 and ((CampBaseClass*)wpn)->GetSPType() not_eq SPTYPE_AIM9P)
        {
            LabelButton(17, "TD");
        }
        else if (Sms->curWeaponType == wtAim9 and ((CampBaseClass*)wpn)->GetSPType() not_eq SPTYPE_AIM9P)
        {
            LabelButton(17, "BP");
        }

        // Marco Edit - SPOT/SCAN Mode
        if (
            ((MissileClass*)Sms->GetCurrentWeapon())->isSpot and 
            Sms->curWeaponType == wtAim9 and 
            ((CampBaseClass*)wpn)->GetSPType() not_eq SPTYPE_AIM9P
        )
        {
            LabelButton(2, "SPOT");
        }
        else if (Sms->curWeaponType == wtAim9 and ((CampBaseClass*)wpn)->GetSPType() not_eq SPTYPE_AIM9P)
        {
            LabelButton(2, "SCAN");
        }
    }
}

// generic missile handling
void SmsDrawable::WpnAAMissileButton(int whichButton, int whichMfd)
{
    SimWeaponClass *cw = Sms->GetCurrentWeapon();

    switch (whichButton)
    {
            // Marco Edit - SPOT/SCAN Mode
        case 2:
            if (g_bRealisticAvionics and cw and Sms->curWeaponType == wtAim9)
            {
                //MI fixup... only toggle for the 9M
                if (cw->GetSPType() not_eq SPTYPE_AIM9P)
                {
                    ((MissileClass*)cw)->isSpot = not ((MissileClass*)cw)->isSpot;
                }
            }

            break;

        case 7:
            if (Sms->curWeaponType == wtAim120)
            {
                // code for AIM120 BIT test
            }
            //MI only with M's
            else if (
                Sms->curWeaponType == wtAim9 and 
                cw->GetSPType() not_eq SPTYPE_AIM9P
            )
            {
                if (Sms->GetCoolState() == SMSClass::WARM or Sms->GetCoolState() == SMSClass::WARMING)
                {
                    /*if (Sms->aim9warmtime not_eq 0.0)
                    {
                     Sms->aim9cooltime = (SimLibElapsedTime + 3 * CampaignSeconds) - ((Sms->aim9warmtime - SimLibElapsedTime) / 20); // 20 = 60/3
                     Sms->aim9warmtime = 0.0;
                    }
                    else
                    {
                     Sms->aim9cooltime = SimLibElapsedTime + 3 * CampaignSeconds; // in 3 seconds
                    }*/
                    Sms->SetCoolState(SMSClass::COOLING);
                }

                if (Sms->GetCoolState() == SMSClass::COOL)
                {
                    //Sms->aim9warmtime = SimLibElapsedTime + 60 * CampaignSeconds; // in 60 seconds
                    Sms->SetCoolState(SMSClass::WARMING);
                }
            }

            break;

        case 8:
            if (Sms->curWeaponType == wtAim120)
            {
                // code for AIM120 ALBIT test
            }

            break;

        case 16:
            if (Sms->curWeaponType == wtAim120)
            {
                Sms->NextAimId(); // JPO bump the AIM 120 id.
            }

            break;

        case 17:
            if (Sms->curWeaponType == wtAim120)
            {
                // code for AIM120 Telemetry toggle
            }

            if (Sms->curWeaponType == wtAim9)
            {
                // Marco Edit - Auto Uncage TD/BP Mode
                if (g_bRealisticAvionics and cw and Sms->curWeaponType == wtAim9)
                {
                    //MI fixup... only toggle for pM
                    if (cw->GetSPType() not_eq SPTYPE_AIM9P)
                    {
                        ((MissileClass*)cw)->isTD = not ((MissileClass*)cw)->isTD;
                    }
                }
            }

            break;

        case 18:

            //MI fixup... only for the M
            if (g_bRealisticAvionics and cw and cw->GetSPType() not_eq SPTYPE_AIM9P)
            {
                ((MissileClass*)cw)->isSlave = not ((MissileClass*)cw)->isSlave;
            }

            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::HarmDisplay(void)
{
    HarmTargetingPod* harmPod = (HarmTargetingPod*)FindSensor(Sms->ownship, SensorClass::HTS);
    FireControlComputer *FCC = Sms->ownship->GetFCC();

    LabelButton(0,  "A-G");
    LabelButton(1,  "TER", "TBL1");

    LabelButton(3,  "INV");

    // RV - I-Hawk - HARM power up timing
    if ( not Sms->GetHARMPowerState())
    {
        LabelButton(6,  "PWR", "OFF");
    }

    else if (Sms->GetHARMPowerState() and Sms->GetHARMInitTimer() > 1.75f)
    {
        LabelButton(6,  "ALIGN");
    }

    else if (Sms->GetHARMPowerState() and Sms->GetHARMInitTimer() > 1.0f)
    {
        LabelButton(6,  "INIT");
    }

    else
        LabelButton(6,  "PWR", "ON");

    LabelButton(7,  "BIT");

    LabelButton(19,  "CD", "OFF");

    BottomRow();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::BombDisplay(void)
{
    char tmpStr[12];
    FireControlComputer* pFCC = Sms->ownship->GetFCC();
    TopRow(0);
    BottomRow();

    if (IsSet(MENUMODE))
    {
        AGMenuDisplay();
        return;
    }

    // Count number of current weapons
    sprintf(tmpStr, "%d%s", Sms->NumCurrentWpn(), Sms->hardPoint[Sms->curHardpoint]->GetWeaponData()->mnemonic);
    ShiAssert(strlen(tmpStr) < 12);
    LabelButton(5,  tmpStr, "");

    if (g_bRealisticAvionics)
        SetWeapParams();

    // OSS Button Labels
    //MI

    if (Sms->curWeaponClass == wcRocketWpn)
    {
        // since pair/single is the only thing modelled for rockets
        if (Sms->GetAGBPair())
            LabelButton(7,  "PAIR");
        else
            LabelButton(7,  "SGL");
    }
    else
    {
        LabelButton(4,  "CNTL");

        if (Sms->curProfile == 0) // MLR 4/3/2004 -
            LabelButton(6,  "PROF 1");
        else
            LabelButton(6,  "PROF 2");

        if ( not g_bRealisticAvionics)
        {
            if (Sms->GetAGBPair())
                LabelButton(7,  "PAIR");
            else
                LabelButton(7,  "SGL");
        }
        else
        {
            if (Sms->GetAGBPair())
                sprintf(tmpStr, "%d PAIR", Sms->GetAGBRippleCount() + 1);
            else
                sprintf(tmpStr, "%d SGL", Sms->GetAGBRippleCount() + 1);

            LabelButton(7, tmpStr);
        }

        sprintf(tmpStr, "%dFT", Sms->GetAGBRippleInterval());
        LabelButton(8,  tmpStr);

        sprintf(tmpStr, "%d", Sms->GetAGBRippleCount() + 1);
        LabelButton(9, "RP", tmpStr);

        if (g_bRealisticAvionics and not g_bMLU)
        {
            char tempstr[12]; //JAM 27Sep03 - Changed from 10, stack over run
            sprintf(tempstr, "AD %.2fSEC", Sms->armingdelay / 100);

            display->TextLeft(-0.3F, 0.2F, tempstr);

            if (pFCC and (pFCC->GetSubMode() == FireControlComputer::LADD or
                         pFCC->GetSubMode() == FireControlComputer::CCRP or
                         pFCC->GetSubMode() == FireControlComputer::DTOSS))
            {
                sprintf(tempstr, "REL ANG %d", Sms->GetAGBReleaseAngle());
                display->TextLeft(-0.3F, 0.0F, tempstr);
            }

            if (Sms->curHardpoint >= 0 and 
                Sms->hardPoint[Sms->curHardpoint]->GetWeaponData()->flags bitand SMSClass::HasBurstHeight)
            {
                sprintf(tempstr, "BA %.0fFT", Sms->burstHeight);
                display->TextLeft(-0.3F, 0.1F, tempstr);
            }

            if (pFCC and pFCC->GetSubMode() == FireControlComputer::LADD)
            {
                display->TextCenter(-0.3F, -0.1F, "PR 25000");
                display->TextCenter(-0.3F, -0.2F, "TOF 28.00");
                display->TextCenter(-0.3F, -0.3F, "MRA 1105");
            }
        }

        //OWLOOK we need a switch here for arming delay
        //if (g_bArmingDelay) MI
        if (g_bRealisticAvionics and g_bMLU)
        {
            sprintf(tmpStr, "AD %.0f", Sms->armingdelay); //me123
            LabelButton(15,  tmpStr); //me123
        }

        //MI
        if (g_bRealisticAvionics)
        {
            switch (Sms->GetAGBFuze())
            {
                case 0:
                    LabelButton(17, "NSTL");
                    break;

                case 1:
                    LabelButton(17, "NOSE");
                    break;

                case 2:
                    LabelButton(17, "TAIL");
                    break;
            }
        }
        else
        {
            LabelButton(17, "NSTL");
        }

        //MI not here in real
        if ( not g_bRealisticAvionics)
        {
            if (Sms->curHardpoint >= 0 and 
                Sms->hardPoint[Sms->curHardpoint]->GetWeaponData()->flags bitand SMSClass::HasBurstHeight)
            {
                sprintf(tmpStr, "BA %.0f", Sms->burstHeight);
                LabelButton(18,  tmpStr);
            }
        }
    }
}

// JPO - daw the menu options
void SmsDrawable::AGMenuDisplay(void)
{
    FireControlComputer* pFCC = Sms->ownship->GetFCC();
    ShiAssert(FALSE == F4IsBadReadPtr(pFCC, sizeof * pFCC));
    LabelButton(19, "CCIP", NULL, pFCC->GetSubMode() == FireControlComputer::CCIP);
    LabelButton(18, "CCRP", NULL, pFCC->GetSubMode() == FireControlComputer::CCRP);
    LabelButton(17, "DTOS", NULL, pFCC->GetSubMode() == FireControlComputer::DTOSS);
    LabelButton(16, "LADD", NULL, pFCC->GetSubMode() == FireControlComputer::LADD);
    LabelButton(15, "MAN", NULL, pFCC->GetSubMode() == FireControlComputer::MAN);
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::CameraDisplay(void)
{
    char tmpStr[12];
    TopRow(0);

    if ( not Sms->CurStationOK())
    {
        LabelButton(2, "MAL");
    }
    else if (Sms->ownship->OnGround())
    {
        LabelButton(2, "RDY");
    }
    else
    {
        LabelButton(2, "RUN");
    }

    BottomRow();

    sprintf(tmpStr, "IDX %d", frameCount);
    LabelButton(19, tmpStr);

    LabelButton(5, "RPOD");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SmsDrawable::SelectiveJettison(void)
{
    LabelButton(1, "S-J", NULL, 1);

    BottomRow();

    InventoryDisplay(TRUE);
}

void SmsDrawable::TopRow(int isinv)
{
    FireControlComputer *FCC = Sms->ownship->GetFCC();

    if (isinv)
    {
        if ( not FCC->IsNavMasterMode())
            LabelButton(3, "INV", NULL, 1);
    }
    else
        LabelButton(3, "INV");

    switch (FCC->GetMasterMode())
    {
        case FireControlComputer::ILS:
        case FireControlComputer::Nav:
            LabelButton(0, "STBY");
            break;

        case FireControlComputer::MissileOverride:
            LabelButton(0, "MSL");

            // ASSOCIATOR 03/12/03: Added the combined SnapShot LCOS Gunmode SSLC
            if (g_bRealisticAvionics and Sms->curWeaponType == wtGuns)
            {
                switch (FCC->GetSubMode())
                {
                    case FireControlComputer::EEGS:
                        LabelButton(1, "EEGS");
                        break;

                    case FireControlComputer::SSLC:
                        LabelButton(1, "SSLC");
                        break;

                    case FireControlComputer::LCOS:
                        LabelButton(1, "LCOS");
                        break;

                    case FireControlComputer::Snapshot:
                        LabelButton(1, "SNAP");
                        break;
                }
            }

            break;

            // ASSOCIATOR 03/12/03: Added the combined SnapShot LCOS Gunmode SSLC
        case FireControlComputer::Dogfight:
            LabelButton(0, "DGFT");

            if ( not g_bRealisticAvionics)
            {
                switch (FCC->GetSubMode())
                {
                    case FireControlComputer::EEGS:
                        LabelButton(1, "EEGS");
                        break;

                    case FireControlComputer::SSLC:
                        LabelButton(1, "SSLC");
                        break;

                    case FireControlComputer::LCOS:
                        LabelButton(1, "LCOS");
                        break;

                    case FireControlComputer::Snapshot:
                        LabelButton(1, "SNAP");
                        break;
                }
            }
            else
            {
                switch (FCC->GetDgftGunSubMode())
                {
                    case FireControlComputer::EEGS:
                        LabelButton(1, "EEGS");
                        break;

                    case FireControlComputer::SSLC:
                        LabelButton(1, "SSLC");
                        break;

                    case FireControlComputer::LCOS:
                        LabelButton(1, "LCOS");
                        break;

                    case FireControlComputer::Snapshot:
                        LabelButton(1, "SNAP");
                        break;
                }
            }

            break;

        case FireControlComputer::AAGun:
        case FireControlComputer::AGGun:
            LabelButton(0, "GUN");

            LabelButton(1, FCC->subModeString);
            break;

        default:
            if (FCC->IsAGMasterMode())
            {
                LabelButton(0, "A-G");

                //MI temporary till we get LGB's going
                if (g_bRealisticAvionics)
                {
                    if (FCC->GetMasterMode() == FireControlComputer::AirGroundLaser or Sms and Sms->curWeaponDomain not_eq wdGround)
                        LabelButton(1, "");
                    else
                        LabelButton(1, FCC->subModeString);
                }
                else
                    LabelButton(1, FCC->subModeString);
            }
            else
                LabelButton(0, "AAM");

            break;
    }
}

void SmsDrawable::BottomRow(void)
{
    //MI
    float cX, cY = 0;
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    RadarDopplerClass* theRadar = (RadarDopplerClass*)FindSensor(playerAC, SensorClass::Radar);

    if ( not theRadar)
    {
        ShiWarning("Oh Oh shouldn't be here without a radar");
        return;
    }
    else
        theRadar->GetCursorPosition(&cX, &cY);

    char *mode = "";

    switch (Sms->MasterArm())
    {
        case SMSBaseClass::Safe:

            //MI not here in real
            if ( not g_bRealisticAvionics)
                mode = "SAF";
            else
                mode = "";

            break;

        case SMSBaseClass::Sim:
            mode = "SIM";
            break;

        case SMSBaseClass::Arm:
            mode = "RDY";
            break;
    }

    if (g_bRealisticAvionics)
    {
        float x, y;
        GetButtonPos(12, &x, &y);
        // JPO - do this ourselves, so we can pass the rest off to the superclass.
        display->TextCenter(x, y + display->TextHeight(), mode);
        MfdDrawable::BottomRow();

        //MI changed
        if (g_bRealisticAvionics)
        {
            if (OTWDriver.pCockpitManager and OTWDriver.pCockpitManager->mpIcp and 
                OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
            {
                DrawBullseyeCircle(display, cX, cY);
            }
            else
                MfdDrawable::DrawReference((AircraftClass*)Sms->Ownship());
        }
        else
            MfdDrawable::DrawReference((AircraftClass*)Sms->Ownship());
    }
    else
    {
        LabelButton(10, "S-J");
        LabelButton(12, "FCR", mode);
        LabelButton(13, "MENU", NULL, 1); //me123
        LabelButton(14, "SWAP");
    }
}
//MI
void SmsDrawable::EmergJetDisplay(void)
{
    int curStation = 0;
    LabelButton(0, "E-J", NULL);
    LabelButton(4, "CLR", NULL);
    InventoryDisplay(2);

    for (curStation = Sms->numHardpoints - 1; curStation > 0; curStation--)
    {
        // OW Jettison fix
#if 0
        if ( not (((AircraftClass *)Sms->ownship)->IsF16() and 
              (curStation == 1 or curStation == 9 or hardPoint[curStation]->GetWeaponClass() == wcECM)) and 
            hardPoint[curStation]->GetRack())
#else
        if ( not (((AircraftClass *)Sms->ownship)->IsF16() and 
              (curStation == 1 or curStation == 9 or Sms->hardPoint[curStation]->GetWeaponClass() == wcECM or Sms->hardPoint[curStation]->GetWeaponClass() == wcAimWpn)) and 
            (Sms->hardPoint[curStation]->GetRack() or curStation == 5 and Sms->hardPoint[curStation]->GetWeaponClass() == wcTank))//me123 in the line above addet a check so we don't emergency jettison a-a missiles
#endif
        {
            hardPointSelected or_eq (1 << curStation);
        }
    }

    BottomRow();
}
//MI
void SmsDrawable::ChangeToInput(int button)
{
    ClearDigits();
    lastInputMode = displayMode;
    SetDisplayMode(InputMode);
    InputLine = 0;
    MaxInputLines = 1;
    InputsMade = 0;

    switch (button)
    {
        case 4:
            PossibleInputs = 4;
            InputModus = CONTROL_PAGE;

            if (Sms->curHardpoint >= 0 and 
                Sms->hardPoint[Sms->curHardpoint]->GetWeaponData()->flags bitand SMSClass::HasBurstHeight)
            {
                C1Weap = FALSE;
                C2Weap = TRUE;
                C3Weap = FALSE;
                C4Weap = FALSE;
            }
            else
            {
                C1Weap = TRUE;
                C2Weap = FALSE;
                C3Weap = FALSE;
                C4Weap = FALSE;
            }

            break;

        case 8:
            PossibleInputs = 3;
            InputModus = RELEASE_SPACE;
            break;

        case 9:
            PossibleInputs = 2;
            InputModus = RELEASE_PULSE;
            break;

        case 15:
            PossibleInputs = 4;
            InputModus = ARMING_DELAY;
            break;

        case 18:
            PossibleInputs = 4;
            InputModus = BURST_ALT;
            break;

        default:
            break;
    }
}
void SmsDrawable::ChangeProf(void)
{
    FireControlComputer *pFCC = Sms->ownship->GetFCC();

    //if(pFCC->GetSubMode() == FireControlComputer::OBSOLETERCKT)
    // MLR 5/30/2004 - going to allow rockets
    //if(pFCC->GetMasterMode() == FireControlComputer::AirGroundRocket)
    // return; // MLR 4/3/2004 - Rockets have no profiles
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    RadarDopplerClass* pradar = (RadarDopplerClass*) FindSensor(playerAC, SensorClass::Radar);

    if (pradar) //MI fix
    {
        pradar->SetScanDir(1.0F);
        pradar->SelectLastAGMode();
    }

    //change profiles

    Sms->NextAGBProfile(); // MLR 4/3/2004 -

    //Sms->rippleCount    = Sms->GetAGBRippleCount();
    //Sms->rippleInterval = Sms->GetAGBRippleInterval();
    //Sms->SetPair(Sms->GetAGBPair());
    if (pFCC)
    {
        if (pFCC->GetMasterMode() == FireControlComputer::AirGroundLaser)
            pFCC->SetSubMode(FireControlComputer::SLAVE);
        else
        {
            pFCC->SetSubMode(Sms->GetAGBSubMode());
        }
    }

    /*
    Sms->Prof1 = not Sms->Prof1;

    if(Sms->Prof1)
    {
     Sms->rippleCount = Sms->Prof1RP;
     Sms->rippleInterval = Sms->Prof1RS;
     Sms->SetPair(Sms->Prof1Pair);
     if(pFCC)
     {
     if(pFCC->GetMasterMode() == FireControlComputer::AirGroundLaser)
     pFCC->SetSubMode(FireControlComputer::SLAVE);
     else
     {
     pFCC->SetSubMode(Sms->Prof1SubMode);
     }
     }
    }
    else
    {
     Sms->rippleCount = Sms->Prof2RP;
     Sms->rippleInterval = Sms->Prof2RS;
     Sms->SetPair(Sms->Prof2Pair);
     if(pFCC)
     {
     if(pFCC->GetMasterMode() == FireControlComputer::AirGroundLaser)
     pFCC->SetSubMode(FireControlComputer::SLAVE);
     else
     {
     pFCC->SetSubMode(Sms->Prof2SubMode);
     }
     }
    }
    */
}
void SmsDrawable::SetWeapParams(void)
{
    if (Sms->curHardpoint >= 0)
    {
        if (Sms->hardPoint[Sms->curHardpoint]->GetWeaponData()->flags bitand SMSClass::HasBurstHeight)
        {
            //Sms->armingdelay = Sms->C2AD;
            Sms->armingdelay = Sms->GetAGBC2ArmDelay();
            //Sms->burstHeight = Sms->C2BA;
            Sms->burstHeight = (float)Sms->GetAGBBurstAlt();
        }
        else
        {
            // MLR 4/3/2004 -
            //Sms->rippleCount    = Sms->GetAGBRippleCount();
            //Sms->rippleInterval = Sms->GetAGBRippleInterval();
            //Sms->SetPair        ( Sms->GetAGBPair() );

            if (Sms->GetAGBFuze() == 0 or Sms->GetAGBFuze() == 2)
                Sms->armingdelay = Sms->GetAGBC1ArmDelay2();
            else
                Sms->armingdelay = Sms->GetAGBC1ArmDelay1();



            /*
            if(Sms->Prof1)
            {
             Sms->rippleCount = Sms->Prof1RP;
             Sms->rippleInterval = Sms->Prof1RS;
             Sms->SetPair(Sms->Prof1Pair);
             if(Sms->Prof1NSTL == 0 or Sms->Prof1NSTL == 2)
             Sms->armingdelay = Sms->C1AD2;
             else
             Sms->armingdelay = Sms->C1AD1;
            }
            else
            {
             Sms->rippleCount = Sms->Prof2RP;
             Sms->rippleInterval = Sms->Prof2RS;
             Sms->SetPair(Sms->Prof2Pair);
             if(Sms->Prof2NSTL == 0 or Sms->Prof2NSTL == 2)
             Sms->armingdelay = Sms->C1AD2;
             else
             Sms->armingdelay = Sms->C1AD1;
            } */
        }
    }
}
void SmsDrawable::MavSMSDisplay(void)
{
    AircraftClass *self = ((AircraftClass*)SimDriver.GetPlayerAircraft());
    int CAPplayer = 0;

    if (self->autopilotType == AircraftClass::CombatAP)
        CAPplayer = 1;

    FireControlComputer *pFCC = Sms->ownship->GetFCC();

    if ( not pFCC)
        return;

    char tempstr[10] = "";

    if (CAPplayer)
    {
        Sms->Powered = true;
        Sms->MavCoolTimer = 0;
        Sms->MavSubMode = SMSBaseClass::VIS;
        LabelButton(6, "PWR", "ON");
    }

    LabelButton(0, "A-G");

    if (Sms->MavSubMode == SMSBaseClass::PRE)
        sprintf(tempstr, "PRE");
    else if (Sms->MavSubMode == SMSBaseClass::VIS)
        sprintf(tempstr, "VIS");
    else
        sprintf(tempstr, "BORE");

    LabelButton(1, tempstr);
    LabelButton(3, "INV");

    if ((Sms->Powered) and ( not CAPplayer))
        LabelButton(6, "PWR", "ON");
    else
        LabelButton(6, "PWR", "OFF");

    LabelButton(7, "RP", "1");
    LabelButton(18, "STEP");
    //not here in the real deal
    //LabelButton(19, pFCC->subModeString);
    BottomRow();
}

//Cobra
void SmsDrawable::JDAMDisplay(void)
{
    char tmpStr[12];
    FireControlComputer* pFCC = Sms->ownship->GetFCC();
    AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
    AircraftClass *self = ((AircraftClass*)playerAC);
    int CAPplayer = 0;

    if (self->autopilotType == AircraftClass::CombatAP)
        CAPplayer = 1;

    TopRow(0);
    BottomRow();

    if (IsSet(MENUMODE))
    {
        AGMenuDisplay();
        return;
    }

    // Count number of current weapons
    sprintf(tmpStr, "%d%s", Sms->NumCurrentWpn(), Sms->hardPoint[Sms->curHardpoint]->GetWeaponData()->mnemonic);
    ShiAssert(strlen(tmpStr) < 12);
    LabelButton(5,  tmpStr, "");

    if (g_bRealisticAvionics)
        SetWeapParams();

    // OSS Button Labels
    if (Sms->curWeaponClass == wcRocketWpn)
    {
        // since pair/single is the only thing modelled for rockets
        if (Sms->GetAGBPair())
            LabelButton(7,  "PAIR");
        else
            LabelButton(7,  "SGL");
    }
    else
    {
        LabelButton(4,  "CNTL");

        if (CAPplayer)
        {
            Sms->JDAMPowered = true;
            Sms->JDAMInitTimer = 1;
            LabelButton(6,  "RDY");
            Sms->JDAMtargeting = SMSBaseClass::PB;
        }
        else
        {
            if ( not Sms->JDAMPowered) // MLR 4/3/2004 -
                LabelButton(6,  "PWR", "OFF");
            else if (Sms->JDAMPowered and not Sms->curWeapon)
                LabelButton(6,  "PWR", "OFF");
            else if (Sms->JDAMPowered and Sms->JDAMInitTimer > 7)
                LabelButton(6,  "ALIGN");
            else if (Sms->JDAMPowered and Sms->JDAMInitTimer > 4)
                LabelButton(6,  "INIT");
            else
                LabelButton(6,  "RDY");
        }


        if ( not g_bRealisticAvionics)
        {
            if (Sms->GetAGBPair())
                LabelButton(7,  "PAIR");
            else
                LabelButton(7,  "SGL");
        }
        else
        {
            if (Sms->GetAGBPair())
                sprintf(tmpStr, "%d PAIR", Sms->GetAGBRippleCount() + 1);
            else
                sprintf(tmpStr, "%d SGL", Sms->GetAGBRippleCount() + 1);

            LabelButton(7, tmpStr);
        }

        sprintf(tmpStr, "%dFT", Sms->GetAGBRippleInterval());
        LabelButton(8,  tmpStr);

        sprintf(tmpStr, "%d", Sms->GetAGBRippleCount() + 1);
        LabelButton(9, "RP", tmpStr);

        // RV - Biker - Only display target name if we're in PB
        //if(g_bRealisticAvionics and not g_bMLU)
        if (g_bRealisticAvionics and not g_bMLU)
        {
            if ((Sms->JDAMtargeting == SMSBaseClass::PB and self->JDAMtargetRange > 0) or (CAPplayer))
            {
                char tempstr[80]; //JAM 27Sep03 - Changed from 10, stack over run
                //sprintf(tempstr, "AD %.2fSEC", Sms->armingdelay / 100);
                sprintf(tempstr, "TGT: %s", self->JDAMtargetName);
                display->TextLeft(-0.6F, 0.2F, tempstr);
                sprintf(tempstr, "OBJ: %s", self->JDAMtargetName1);
                display->TextLeft(-0.6F, 0.1F, tempstr);

                sprintf(tempstr, "RNG: %.2f", self->JDAMtargetRange);
                display->TextLeft(-0.6F, 0.0F, tempstr);
            }
            else
            {
                char tempstr[80];
                float xCurr = pFCC->groundDesignateX;
                float yCurr = pFCC->groundDesignateY;

                float dX = xCurr - self->XPos();
                float dY = yCurr - self->YPos();

                float latitude = (FALCON_ORIGIN_LAT * FT_PER_DEGREE + xCurr) / EARTH_RADIUS_FT;
                float cosLatitude = (float)cos(latitude);
                float longitude = ((FALCON_ORIGIN_LONG * DTR * EARTH_RADIUS_FT * cosLatitude) + yCurr) / (EARTH_RADIUS_FT * cosLatitude);

                latitude *= RTD;
                longitude *= RTD;

                int   longDeg = FloatToInt32(longitude);
                float longMin = (float)fabs(longitude - longDeg) * DEG_TO_MIN;

                int   latDeg = FloatToInt32(latitude);
                float latMin = (float)fabs(latitude - latDeg) * DEG_TO_MIN;

                // format lat/long here
                if (latMin < 10.0F)
                    sprintf(tempstr, "LAT: %3d*0%2.2f\'", latDeg, latMin);
                else
                    sprintf(tempstr, "LAT: %3d*%2.2f\'", latDeg, latMin);

                display->TextLeft(-0.6F, 0.2F, tempstr);

                if (longMin < 10.0F)
                    sprintf(tempstr, "LNG: %3d*0%2.2f\'", longDeg, longMin);
                else
                    sprintf(tempstr, "LNG: %3d*%2.2f\'", longDeg, longMin);

                display->TextLeft(-0.6F, 0.1F, tempstr);

                sprintf(tempstr, "RNG: %.2f", SqrtF(dX * dX + dY * dY) * FT_TO_NM);
                display->TextLeft(-0.6F, 0.0F, tempstr);
            }
        }


        //OWLOOK we need a switch here for arming delay
        //if (g_bArmingDelay) MI
        if (g_bRealisticAvionics and g_bMLU)
        {
            sprintf(tmpStr, "AD %.0f", Sms->armingdelay); //me123
            LabelButton(15,  tmpStr); //me123
        }
        else if ( not g_bMLU)
        {
            //Target Step
            LabelButton(15, "TGT", "STEP ^");
            LabelButton(16, "TGT", "STEP v");
        }
        else
            //Target Step
            LabelButton(16, "TGT", "STEP");

        //MI
        if (g_bRealisticAvionics)
        {
            switch (Sms->GetAGBFuze())
            {
                case 0:
                    LabelButton(17, "NSTL");
                    break;

                case 1:
                    LabelButton(17, "NOSE");
                    break;

                case 2:
                    LabelButton(17, "TAIL");
                    break;
            }
        }
        else
            LabelButton(17, "NSTL");

        //MI not here in real

        //PB or TOO
        if (Sms->JDAMtargeting == SMSBaseClass::PB)
        {
            LabelButton(18, "PB");

            // RV - Biker - Automaically designate JDAMs???  FRB - Yes...Targets are Pre-Programmed
            if (self->JDAMAllowAutoStep and Sms->JDAMPowered and Sms->JDAMInitTimer <= 4.0f and pFCC->designateCmd == FALSE)
            {
                pFCC->designateCmd = TRUE;
            }
        }
        else
            LabelButton(18, "TOO");

        //RV - I-Hawk - GPS targets STEP/NO-STEP display
        if (g_bRealisticAvionics)
        {
            if (self->JDAMAllowAutoStep)
            {
                LabelButton(19, "AUTO STEP");
            }
            else
            {
                LabelButton(19, "MAN" , "STEP");
            }
        }

        ShowMissiles(5);
    }
}


