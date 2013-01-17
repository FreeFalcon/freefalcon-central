#include "stdhdr.h"
#include "mfd.h"
#include "hud.h"
#include "sms.h"
#include "SmsDraw.h"
#include "airframe.h"
#include "Aircrft.h"
#include "fcc.h"
#include "otwdrive.h"
#include "playerop.h"
#include "Graphics\Include\render2d.h"
#include "Graphics\Include\canvas3d.h"
#include "Graphics\Include\tviewpnt.h"
#include "Graphics\Include\renderir.h"
#include "dispcfg.h"
#include "simdrive.h"
#include "camp2sim.h"
#include "digi.h"
#include "lantirn.h"
#include "FalcLib\include\dispopts.h"

static int flash = FALSE;
static int lantdebug = FALSE;

extern bool g_bLantDebug;

void LantirnDrawable::DisplayInit (ImageBuffer* image)
{
    DisplayExit();
    
    privateDisplay = new RenderIR;
    ((RenderIR*)privateDisplay)->Setup (image, OTWDriver.GetViewpoint());
    
    privateDisplay->SetColor (0xff00ff00);
   	((Render3D*)privateDisplay)->SetFOV(28.0f*DTR);
}

void LantirnDrawable::Display (VirtualDisplay* newDisplay)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    display = newDisplay;
    ShiAssert(theLantirn);
    flash = vuxRealTime & 0x200;

    if (display->type == VirtualDisplay::DISPLAY_GENERAL)
        DrawTerrain ();
    display->SetColor(GetMfdColor(MFD_DEFAULT));
    DrawWarnings ();
    DrawFlightInfo ();
    
    DrawAzimuthScan();
    DrawRangeScale();
    DrawRadarArrows();
    switch(theLantirn -> GetTFRMode()) {
    case LantirnClass::TFR_NORM:
        LabelButton (0, "NORM");
	break;
    case LantirnClass::TFR_LP1:
        LabelButton (0, "LP1");
	break;
    case LantirnClass::TFR_STBY:
        LabelButton (0, "STBY");
	break;
    case LantirnClass::TFR_WX:
        LabelButton (0, "WX");
	break;
    case LantirnClass::TFR_ECCM:
        LabelButton (0, "ECCM");
	break;
    }

    switch (theLantirn->GetTFRRide()) {
    case LantirnClass::TFR_HARD:
	LabelButton (1, "HARD");
	break;
    case LantirnClass::TFR_MED:
	LabelButton (1, "MED");
	break;
    case LantirnClass::TFR_SOFT:
	LabelButton (1, "SOFT");
	break;
    }
    LabelButton(3, playerAC->AutopilotType() == AircraftClass::LantirnAP ? "ON" : "OFF");
    LabelButton(4, "CHN1");

    LabelButton(5, "1000", NULL, theLantirn->GetTFRAlt() == 1000);
    LabelButton(6, "500", NULL, theLantirn->GetTFRAlt() == 500);
    LabelButton(7, "300", NULL, theLantirn->GetTFRAlt() == 300);
    LabelButton(8, "200", NULL, theLantirn->GetTFRAlt() == 200);
    LabelButton(9, "VLC", NULL, theLantirn->GetTFRAlt() == 100);

    BottomRow();

    LabelButton(15, "ECCM", NULL, theLantirn -> GetTFRMode() == LantirnClass::TFR_ECCM);
    LabelButton(16, "WX", NULL, theLantirn -> GetTFRMode() == LantirnClass::TFR_WX);
    LabelButton(17, "STBY", NULL, theLantirn -> GetTFRMode() == LantirnClass::TFR_STBY);
    LabelButton(18, "LP1", NULL, theLantirn -> GetTFRMode() == LantirnClass::TFR_LP1);
    LabelButton(19, "NORM", NULL, theLantirn -> GetTFRMode() == LantirnClass::TFR_NORM);

// JB 010325
	//MI reenabled with config var
	if(lantdebug && g_bLantDebug)
	{
		char buf[200];
		float pos = 9.2f;
		float divs = 11;
		sprintf (buf, "HoldAlt %.0f ft", theLantirn->holdheight);
		display->TextCenter(0, (pos--)/divs, buf);
		sprintf (buf, "GndAlt %.0f ft", theLantirn->gAlt);
		display->TextCenter(0, (pos--)/divs, buf);
		sprintf (buf, "GndDist %.0f ft", theLantirn->gdist);
		display->TextCenter(0, (pos--)/divs, buf);
		sprintf (buf, "Eva %d", theLantirn->evasize);
		display->TextCenter(0, (pos--)/divs, buf);
		sprintf (buf, "TurnR %.0f ft", theLantirn->turnradius);
		display->TextCenter(0, (pos--)/divs, buf);
		sprintf (buf, "minR %.0f ft", theLantirn->min_Radius);
		display->TextCenter(0, (pos--)/divs, buf);
		sprintf (buf, "safeD %.0f ft", theLantirn->min_safe_dist);
		display->TextCenter(0, (pos--)/divs, buf);
		sprintf (buf, "GndDist2 %.0f ft /%.1f", theLantirn->gDist2, theLantirn->lookingAngle);
		display->TextCenter(0, (pos--)/divs, buf);
		sprintf (buf, "PIDMX %.3f", theLantirn->PID_MX);
		display->TextCenter(0, (pos--)/divs, buf);
		sprintf (buf, "PIDer %.3f", theLantirn->PID_error);
		display->TextCenter(0, (pos--)/divs, buf);
		sprintf (buf, "PIDOut %.3f", theLantirn->PID_Output);
		display->TextCenter(0, (pos--)/divs, buf);
		sprintf (buf, "Min/Max G %.1f/%.1f", theLantirn->MinG, theLantirn->MaxG);
		display->TextCenter(0, (pos--)/divs, buf);
		sprintf (buf, "Roll %.5f", theLantirn->roll * RTD);
		display->TextCenter(0, (pos--)/divs, buf);
		sprintf (buf, "gammaCorr %.2f", theLantirn->gammaCorr);
		display->TextCenter(0, (pos--)/divs, buf);
		sprintf (buf, "gamma %.2f", theLantirn->gamma*RTD);
		display->TextCenter(0, (pos--)/divs, buf);
		sprintf (buf, "f1Ht/Dist %.0f / %.0f", theLantirn->featureHeight, theLantirn->featureDistance);
		display->TextCenter(0, (pos--)/divs, buf);
		sprintf (buf, "f2Ht/Dist %.0f / %.0f", theLantirn->featureHeight2, theLantirn->featureDistance2);
		display->TextCenter(0, (pos--)/divs, buf);
		sprintf (buf, "f3Ht/Dist %.0f / %.0f", theLantirn->featureHeight3, theLantirn->featureDistance3);
		display->TextCenter(0, (pos--)/divs, buf);
	//	sprintf (buf, "MinDist %.0f", theLantirn->minavoiddist);
	//	display->TextCenter(0, -0.25, buf);
	//	sprintf (buf, "ADist %.0f", theLantirn->avoiddist);
	//	display->TextCenter(0, -0.375, buf);
	}
	else 
	{
		char tempstr[20] = "";
		if (theLantirn->evasize  == 1 && flash)
			sprintf(tempstr, "FLY UP");
		else if (theLantirn->evasize  == 2 && flash)
			sprintf(tempstr, "OBSTACLE");
		if (theLantirn->SpeedUp && !flash)
			sprintf(tempstr, "SLOW");
		if(playerAC && playerAC->RFState != 2)
			display->TextCenter (0, 0.25, tempstr);
	}
}

void LantirnDrawable::PushButton (int whichButton, int whichMFD)
{
    ShiAssert(theLantirn);
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

    switch (whichButton) {
    case 0:
	theLantirn->StepTFRMode();
	break;
    case 1:
	theLantirn->StepTFRRide();
	break;
    case 2:
	lantdebug = !lantdebug;
	break;
    case 3:
	if (playerAC->AutopilotType() == AircraftClass::LantirnAP)
	{
	    playerAC->SetAutopilot(AircraftClass::APOff);
		//MI
		playerAC->lastapType = AircraftClass::APOff;
	}
	else
	{
		playerAC->SetAutopilot(AircraftClass::LantirnAP);
		playerAC->lastapType = AircraftClass::LantirnAP;
	}
	break;
    case 4:
	break;
    case 5:
	theLantirn->SetTFRAlt(1000);
	break;
    case 6:
	theLantirn->SetTFRAlt(500);
	break;
    case 7:
	theLantirn->SetTFRAlt(300);
	break;
    case 8:
	theLantirn->SetTFRAlt(200);
	break;
    case 9:
	theLantirn->SetTFRAlt(100);
	break;
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
	MfdDrawable::PushButton(whichButton, whichMFD);
	return;
    case 15:
	theLantirn->SetTFRMode (LantirnClass::TFR_ECCM);
	break;
    case 16:
	theLantirn->SetTFRMode (LantirnClass::TFR_WX);
	break;
    case 17:
	theLantirn->SetTFRMode (LantirnClass::TFR_STBY);
	break;
    case 18:
	theLantirn->SetTFRMode (LantirnClass::TFR_LP1);
	break;
    case 19:
	theLantirn->SetTFRMode (LantirnClass::TFR_NORM);
	break;
    }
}

void LantirnDrawable::DrawRadarArrows ()
{
#if 0
    static const float arrowH = 0.0375f;
    static const float arrowW = 0.0433f;

    display->SetColor(GetMfdColor(MFD_LABELS));
    display->TextCenterVertical(RANGE_POSITION, -0.15f, "F1");
    /*----------*/
    /* up arrow */
    /*----------*/
    display->AdjustOriginInViewport( RANGE_POSITION, 0.0F );
    display->Tri (0.0F, arrowH, arrowW, -arrowH, -arrowW, -arrowH);
    /*------------*/
    /* down arrow */
    /*------------*/
    display->AdjustOriginInViewport( 0.0f, -0.3F );
    display->Tri (0.0F, -arrowH, arrowW, arrowH, -arrowW, arrowH);
    display->CenterOriginInViewport();
#endif
}

void LantirnDrawable::DrawAzimuthScan()
{
    static const float azminx = 0.58f, azmaxx = 0.90f;
    static const float azstep = (azmaxx - azminx) / 4.0f;
    static const float azy = 0.75f;
    display->Line (azminx, azy, azmaxx, azy);
    for (int i = 0; i < 5; i++) {
	display->Line (azminx + i * azstep, azy, 
	    azminx + i * azstep, azy + 0.05f);
    }
    float scanline = azminx + (azmaxx - azminx)/2.0f + 
	theLantirn->GetScanLoc() * (azmaxx - azminx)/2.0f;
    display->Line(scanline, azy + 0.06f, scanline, azy + 0.08f);
}

void LantirnDrawable::DrawRangeScale ()
{
    static const float rangeminx = -0.95f, rangemaxx = 0.90f;
    static const float rangexstep = (rangemaxx - rangeminx) / 7.0f;
    static const float rangey = -0.85f;
    display->Line (rangeminx, rangey, rangemaxx, rangey);
    for (int i = 0; i < 8; i++) {
	display->Line (rangeminx + i * rangexstep, rangey,
	    rangeminx + i * rangexstep, rangey + 0.05f);
    }
}
void LantirnDrawable::DrawTerrain ()
{
  Tpoint cameraPos;
  theLantirn->GetCameraPos (&cameraPos);

	Trotation viewRotation;
	float costha,sintha,cospsi,sinpsi, sinphi, cosphi = 0.0;
	costha = (float)cos (MfdDisplay[OnMFD()]->GetOwnShip()->Pitch());
	sintha = (float)sin (MfdDisplay[OnMFD()]->GetOwnShip()->Pitch());
	cospsi = (float)cos (MfdDisplay[OnMFD()]->GetOwnShip()->Yaw());
	sinpsi = (float)sin (MfdDisplay[OnMFD()]->GetOwnShip()->Yaw());
	cosphi = (float)cos (MfdDisplay[OnMFD()]->GetOwnShip()->Roll());
	sinphi = (float)sin (MfdDisplay[OnMFD()]->GetOwnShip()->Roll());

	viewRotation.M11 = cospsi*costha;
	viewRotation.M21 = sinpsi*costha;
	viewRotation.M31 = -sintha;

	viewRotation.M12 = -sinpsi*cosphi + cospsi*sintha*sinphi;
	viewRotation.M22 = cospsi*cosphi + sinpsi*sintha*sinphi;
	viewRotation.M32 = costha*sinphi;

	viewRotation.M13 = sinpsi*sinphi + cospsi*sintha*cosphi;
	viewRotation.M23 = -cospsi*sinphi + sinpsi*sintha*cosphi;
	viewRotation.M33 = costha*cosphi;

	Tpoint p;
	Trotation *r = &viewRotation;
	p.x = cameraPos.x;
	p.y = cameraPos.y;
	p.z = cameraPos.z;
	cameraPos.x = p.x * r->M11 + p.y * r->M12 + p.z * r->M13;
	cameraPos.y = p.x * r->M21 + p.y * r->M22 + p.z * r->M23;
	cameraPos.z = p.x * r->M31 + p.y * r->M32 + p.z * r->M33;

	//JAM 24Nov03
//  display->FinishFrame();
  //((RenderIR*)display)->DrawScene(&cameraPos, &OTWDriver.cameraRot);
	((RenderIR*)display)->DrawScene(&cameraPos, &viewRotation);

	//JAM 12Dec03 - ZBUFFERING OFF
	if(DisplayOptions.bZBuffering)
		((RenderIR*)display)->context.FlushPolyLists();

//	((RenderIR*)display)->PostSceneCloudOcclusion();
//  ((RenderIR*)display)->FinishFrame();
  display->StartDraw();
}

void LantirnDrawable::DrawWarnings ()
{
}

void LantirnDrawable::DrawFlightInfo ()
{
}
