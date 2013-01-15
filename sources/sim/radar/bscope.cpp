#include "stdhdr.h"
#include "geometry.h"
#include "debuggr.h"
#include "object.h"
#include "radarDoppler.h"
#include "simmover.h"
#include "Graphics/Include/display.h"
#include "Graphics/Include/gmComposit.h"
#include "otwdrive.h"
#include "campwp.h"
#include "simveh.h"
#include "cmpclass.h"
#include "fcc.h"
#include "campbase.h"
#include "aircrft.h"
#include "simdrive.h"	//MI
#include "fack.h"		//MI
#include "sms.h"		//MI
#include "icp.h"		//MI
#include "cpmanager.h"	//MI
#include "hud.h"	//MI
#include "team.h"	//MI
//#include "fault.h"	//MI
//#include "campbase.h" // 2002-02-25 S.G.

#define SCH_ANG_INC  11.5F      /* velocity pointer angle increment - JPG 24 Mar 04 - was 22.5, now 11.5    */
#define SCH_FACT   1600.0F      /* velocity pointer length is the ratio */
/* of vt/SCH_FACT                       */
#define DD_LENGTH   0.2F        /* donkey dick length                   */
#define NCTR_BAR_WIDTH        0.2F
#define SECOND_LINE_Y         0.8F
#define TICK_POS              0.75F

static const float DisplayAreaViewTop    =  0.75F;
static const float DisplayAreaViewBottom = -0.68F;
static float DisplayAreaViewLeft   = -0.80F;
static const float DisplayAreaViewRight  =  0.72F;

static float disDeg;
static float steerpoint[12][2] = {
	-0.05F, -0.03F,
	-0.05F, -0.01F,
	-0.03F, -0.01F,
	-0.03F,  0.03F,
	-0.01F,  0.03F,
	-0.01F,  0.05F,
	0.01F,  0.05F,
	0.01F,  0.03F,
	0.03F,  0.03F,
	0.03F, -0.01F,
	0.05F, -0.01F,
	0.05F, -0.03F};

static float elReacqMark[3][2];
static float elMark[4][2] = {0.0F, 0.02F, 0.0F, -0.02F, 0.0F, 0.0F, 0.04F, 0.0F};
static float cursor[12][2];
static int fpass = TRUE;

extern bool g_bEPAFRadarCues, g_bRadarJamChevrons;
extern bool g_bEnableGRCursorBullseye;	// ASSOCIATOR 3/12/03: Enables Cursor Bullseye in Ground Radar Modes
extern float g_fRadarScale;
extern bool g_bnewAMRAAMdlz; //JPG 7 Apr 04

//MI
extern bool g_bMLU;
extern bool g_bIFF;
extern bool g_bINS;
void DrawBullseyeData (VirtualDisplay* display, float cursorX, float cursorY);

DWORD	tmpColor = MFD_GREEN; // RV - I-Hawk

//MI
void DrawCursorBullseyeData(VirtualDisplay* display, float cursorX, float cursorY);
void DrawBullseyeCircle(VirtualDisplay* display, float cursorX, float cursorY);

// JPG 16 Jan 04
void DrawSteerPointCursorData (VirtualDisplay* display, FalconEntity* platform, float cursorX, float cursorY);

void RadarDopplerClass::Display(VirtualDisplay* newDisplay){
	float cX, cY;
	float vpLeft, vpTop, vpRight, vpBottom;

	GetCursorPosition (&cX, &cY);
	display = newDisplay;

	if (fpass){
		fpass = FALSE;

		/*----------------------------------------------------------*/
		/* Set the scale factor to convert degrees to display scale */
		/*----------------------------------------------------------*/
		disDeg = AZL / MAX_ANT_EL;

		elReacqMark[0][0] =  0.03F;
		elReacqMark[0][1] =  0.03F;
		elReacqMark[1][0] =  0.0F;
		elReacqMark[1][1] =  0.0F;
		elReacqMark[2][0] =  0.03F;
		elReacqMark[2][1] = -0.03F;

		cursor[0][0] = -0.04F;
		cursor[0][1] =  0.06F;
		cursor[1][0] =  0.04F;
		cursor[1][1] =  0.06F;
		cursor[2][0] =  0.02F;
		cursor[2][1] =  0.02F;
		cursor[3][0] =  0.06F;
		cursor[3][1] =  0.04F;
		cursor[4][0] =  0.06F;
		cursor[4][1] = -0.04F;
		cursor[5][0] =  0.02F;
		cursor[5][1] = -0.02F;
		cursor[6][0] =  0.04F;
		cursor[6][1] = -0.06F;
		cursor[7][0] = -0.04F;
		cursor[7][1] = -0.06F;
		cursor[8][0] = -0.02F;
		cursor[8][1] = -0.02F;
		cursor[9][0] = -0.06F;
		cursor[9][1] = -0.04F;
		cursor[10][0] = -0.06F;
		cursor[10][1] =  0.04F;
		cursor[11][0] = -0.02F;
		cursor[11][1] =  0.02F;

	}


	int tmpColor = display->Color();
	float x18, y18;
	GetButtonPos(18, &x18, &y18);

	DisplayAreaViewLeft = x18 + display->TextWidth("40");

	/*---------------------*/
	/* common to a/a modes */
	/*---------------------*/
	// ASSOCIATOR 3/12/03: Moved DrawBars before DrawAzElTicks and DrawScanMarkers so that they draw peoperly 
	// in transparent MFD view and also with better drawing order.  
	if (mode == RWS || mode == SAM || mode == TWS || mode == LRS || mode == VS || mode == STT ){    	
		DrawBars();
	}

	if (mode != GM && mode != GMT && mode != SEA){
		DrawAzElTicks();	// ASSOCIATOR 3/12/03: Reversed drawing order
		DrawScanMarkers();
	}

	display->GetViewport (&vpLeft, &vpTop, &vpRight, &vpBottom);

	/*----------------------*/
	/* Mode dependent stuff */
	/*----------------------*/
	switch (mode){
			case OFF: // JPO - new modes..
			case STBY:
					STBYDisplay();
					break;
			case TWS:
					DrawRangeTicks();
					if (IsAADclt(Arrows) == FALSE) DrawRangeArrows();
					if (IsAADclt(Rng) == FALSE) DrawRange();
					// DrawBars(); // ASSOCIATOR 3/12/03: Redundant
					DrawWaterline();
					display->SetColor(GetMfdColor(MFD_BULLSEYE)); // JPO
					//MI we only get the bullseye readout in the corner if we selected it so
					//the info on the cursor get's drawn nontheless
					if(!g_bRealisticAvionics)
						DrawBullseyeData(display, cX, cY);
					else
					{
						if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
										OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
						{
							DrawBullseyeCircle(display, cX, cY);
							DrawCursorBullseyeData(display, cX, cY);
						}
						else
						{
							DrawReference(display);

							DrawSteerPointCursorData(display, platform, cX, cY);
						}
					}
					display->SetColor(tmpColor);
					TWSDisplay();
					DrawACQCursor();  
					display->SetColor(GetMfdColor(MFD_LINES)); // JPO
					DrawSteerpoint();
					display->SetColor(tmpColor);
					DrawAzLimitMarkers();
					//MI
					if(g_bRealisticAvionics && DrawRCR)
						DrawRCRCount();
					break;

			case SAM:
					DrawRangeTicks();
					if (IsAADclt(Arrows) == FALSE) DrawRangeArrows();
					if (IsAADclt(Rng) == FALSE) DrawRange();
					// DrawBars(); // ASSOCIATOR 3/12/03: Redundant
					DrawWaterline();
					display->SetColor(GetMfdColor(MFD_BULLSEYE)); // JPO
					//MI we only get the bullseye readout in the corner if we selected it so
					//the info on the cursor get's drawn nontheless
					if(!g_bRealisticAvionics)
						DrawBullseyeData(display, cX, cY);
					else
					{
						if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
										OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
						{
							DrawBullseyeCircle(display, cX, cY);
							DrawCursorBullseyeData(display, cX, cY);
						}
						else
						{
							DrawReference(display);

							DrawSteerPointCursorData(display, platform, cX, cY);
						}
					}
					display->SetColor(tmpColor);
					SAMDisplay();
					DrawACQCursor();  
					display->SetColor(GetMfdColor(MFD_LINES)); // JPO
					DrawSteerpoint();
					display->SetColor(tmpColor);
					//MI
					if(g_bRealisticAvionics && DrawRCR)
						DrawRCRCount();
					break;

			case RWS:
			case LRS:
					DrawRangeTicks();
					if (IsAADclt(Arrows) == FALSE) DrawRangeArrows();
					if (IsAADclt(Rng) == FALSE) DrawRange();
					// DrawBars(); // ASSOCIATOR 3/12/03: Redundant
					DrawWaterline();
					display->SetColor(GetMfdColor(MFD_BULLSEYE)); // JPO
					//MI we only get the bullseye readout in the corner if we selected it so
					//the info on the cursor get's drawn nontheless
					if(!g_bRealisticAvionics)
						DrawBullseyeData(display, cX, cY);
					else
					{
						if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
										OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
						{
							DrawBullseyeCircle(display, cX, cY);
							DrawCursorBullseyeData(display, cX, cY);
						}
						else
						{
							DrawReference(display);

							DrawSteerPointCursorData(display, platform, cX, cY);
						}
					}
					display->SetColor(tmpColor);
					RWSDisplay();
					DrawACQCursor();
					display->SetColor(GetMfdColor(MFD_LINES)); // JPO
					DrawSteerpoint();
					display->SetColor(tmpColor);
					DrawAzLimitMarkers();
					break;

			case STT:
					DrawRangeTicks();
					if (IsAADclt(Rng) == FALSE) DrawRange();
					// DrawBars(); // ASSOCIATOR 3/12/03: Redundant
					DrawWaterline();
					display->SetColor(GetMfdColor(MFD_BULLSEYE)); // JPO
					//MI we only get the bullseye readout in the corner if we selected it so
					//the info on the cursor get's drawn nontheless
					if(!g_bRealisticAvionics)
						DrawBullseyeData(display, cX, cY);
					else
					{
						if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
										OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
						{
							DrawBullseyeCircle(display, cX, cY);
							DrawCursorBullseyeData(display, cX, cY);
						}
						else
						{
							DrawReference(display);

							DrawSteerPointCursorData(display, platform, cX, cY);
						}
					}
					display->SetColor(tmpColor);
					STTDisplay();
					display->SetColor(GetMfdColor(MFD_LINES)); // JPO
					DrawSteerpoint();
					display->SetColor(tmpColor);
					//MI
					if(g_bRealisticAvionics && DrawRCR)
						DrawRCRCount();
					break;

			case ACM_SLEW:
			case ACM_30x20:
			case ACM_10x60:
			case ACM_BORE:
					if (IsAADclt(Rng) == FALSE) DrawRange();
					DrawRangeTicks();
					DrawWaterline();
					DrawAzElTicks();	// ASSOCIATOR Added here so that it draws properly in transparent MFD view 
					DrawScanMarkers();	// ASSOCIATOR Added here so that it draws properly in transparent MFD view 
					display->SetColor(GetMfdColor(MFD_BULLSEYE)); // JPO
					//MI we only get the bullseye readout in the corner if we selected it so
					//the info on the cursor get's drawn nontheless
					if(!g_bRealisticAvionics)
						DrawBullseyeData(display, cX, cY);
					else
					{
						if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
										OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
						{
							DrawBullseyeCircle(display, cX, cY);
							DrawCursorBullseyeData(display, cX, cY);
						}
						else
						{
							DrawReference(display);

							DrawSteerPointCursorData(display, platform, cX, cY);
						}
					}
					display->SetColor(tmpColor);
					ACMDisplay();
					display->SetColor(GetMfdColor(MFD_LINES)); // JPO
					DrawSteerpoint();
					display->SetColor(tmpColor);
					break;

			case VS:
					DrawRangeTicks();
					if (IsAADclt(Arrows) == FALSE) DrawRangeArrows();
					if (IsAADclt(Rng) == FALSE) DrawRange();
					// DrawBars(); // ASSOCIATOR 3/12/03: Redundant
					DrawWaterline();
					display->SetColor(GetMfdColor(MFD_BULLSEYE)); // JPO
					//MI we only get the bullseye readout in the corner if we selected it so
					//the info on the cursor get's drawn nontheless
					if(!g_bRealisticAvionics)
						DrawBullseyeData(display, cX, cY);
					else
					{
						if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
										OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
						{
							DrawBullseyeCircle(display, cX, cY);
							DrawCursorBullseyeData(display, cX, cY);
						}
						else
						{
							DrawReference(display);

							DrawSteerPointCursorData(display, platform, cX, cY);
						}
					}
					display->SetColor(tmpColor);
					VSDisplay();
					DrawACQCursor();
					display->SetColor(GetMfdColor(MFD_LINES)); // JPO
					DrawSteerpoint();
					display->SetColor(tmpColor);
					break;
					//MI changed
			case GM:
			case GMT:
			case SEA:
					if(g_bRealisticAvionics)
					{
						// ASSOCIATOR moved this check to RadarDopplerClass::DefaultAGModehere so that it only
						// defaults to AGR when first selected and than be changed manually to any other radar mode
						/*FireControlComputer* pFCC = ((SimVehicleClass*)platform)->GetFCC();//me123 addet for ccip/DTOSS ground ranging check
						  if(pFCC->GetSubMode() == FireControlComputer::CCIP || pFCC->GetSubMode() == FireControlComputer::DTOSS ||
						  pFCC->GetSubMode() == FireControlComputer::***STRAF || pFCC->GetSubMode() == FireControlComputer::RCKT)
						  {
						  if(mode != AGR) {
						  mode = AGR;
						  }
						  return;
						  }
						  else
						 */
						{
							GMDisplay();
							DrawWaterline();
							display->SetColor(GetMfdColor(MFD_BULLSEYE)); // JPO
							//MI we only get the bullseye readout in the corner if we selected it so
							//the info on the cursor get's drawn nontheless
							if(!g_bRealisticAvionics)
								DrawBullseyeData(display, cX, cY);
							else
							{
								if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
												OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
								{
									DrawBullseyeCircle(display, cX, cY);
									DrawCursorBullseyeData(display, cX, cY);
								}
								else
								{
									DrawReference(display);

									// ASSOCIATOR 3/12/03: check to enable Cursor Bullseye in Ground Radar Modes
									if( g_bEnableGRCursorBullseye )
										DrawSteerPointCursorData(display, platform, cX, cY);
								}
							}
							display->SetColor(tmpColor);
						}
					}
					else
					{
						GMDisplay();
						DrawWaterline();
						display->SetColor(GetMfdColor(MFD_BULLSEYE)); // JPO
						//MI we only get the bullseye readout in the corner if we selected it so
						//the info on the cursor get's drawn nontheless
						if(!g_bRealisticAvionics)
							DrawBullseyeData(display, cX, cY);
						else
						{
							if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
											OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
							{
								DrawBullseyeCircle(display, cX, cY);
								DrawCursorBullseyeData(display, cX, cY);
							}
							else
							{
								DrawReference(display);

								DrawSteerPointCursorData(display, platform, cX, cY);
							}
						}
						display->SetColor(tmpColor);
					}
					break;

					//MI
			case AGR:
					AGRangingDisplay();
					break;	
			default:
					DrawTargets();
					break;
	}

	display->SetViewport (vpLeft, vpTop, vpRight, vpBottom);
	// SOI/Radiate status
	if (!IsSOI())
	{
		display->SetColor(GetMfdColor(MFD_TEXT));
		display->TextCenter(0.0F, 0.4F, "NOT SOI");
	}
	else {
		display->SetColor(GetMfdColor(MFD_GREEN));
		DrawBorder(); // JPO SOI
	}

	if (!isEmitting && mode != OFF) // JPO 
	{
		//MI not here in real
		if(!g_bRealisticAvionics)
		{
			display->SetColor(GetMfdColor(MFD_TEXT));
			display->TextCenter (0.0F, 0.0F, "NO RAD");
		}
	}
	//Booster 12/09/2004 - Draw Pull Up cross on MFD if ground Collision
   	if (SimDriver.GetPlayerAircraft()->mFaults->GetFault(alt_low))
		DrawRedBreak(display);

	// RV - I-Hawk
	// Check, if 1 of the MFDs is showing TGP and the attitude warning is set, show it... 
	// (works on all MFDs and not only here)
	for ( int i = 0; i < 4; i++ )
	{
		if ( (MfdDisplay[i])->GetTGPWarning() && (MfdDisplay[i])->CurMode() == MFDClass::TGPMode )
			{ 
				TGPAttitudeWarning(display);
				break;
			}
	}

	display = NULL;
}


// JPO - do stdby/off display
void RadarDopplerClass::STBYDisplay(void) 
{
	float cX, cY;
	display->SetColor(GetMfdColor(MFD_LABELS));
	if (g_bRealisticAvionics) 
	{
		LabelButton(0, mode == OFF ? "OFF" : "STBY");
		if (mode != OFF)
		{
			//LabelButton(3, "OVRD", NULL, !isEmitting);
			LabelButton(3, "OVRD");
			LabelButton(4, "CNTL", NULL, IsSet(CtlMode));
		}
		if (IsSet(MenuMode|CtlMode)) 
		{
			MENUDisplay();
			return;
		}
		LabelButton(5, "BARO");
		LabelButton(8, "CZ");
		if (mode != OFF) 
		{
			LabelButton(7, "SP");
			LabelButton(9, "STP");
		}
		DrawBars();
		DrawRange();
		DrawRangeArrows();
		GetCursorPosition (&cX, &cY);
		if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
						OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
			DrawBullseyeCircle(display, cX, cY);
		else
			DrawReference(display);

	}
	else 
	{
		LabelButton (5,  "GM ");
		LabelButton (6,  "GMT");
		LabelButton (7,  "SEA");
		LabelButton (16, "ACM");
		LabelButton (17, "VS ");
		LabelButton (18, "RWS");
		LabelButton (19, "TWS");
	}
	AABottomRow ();
}


void RadarDopplerClass::DrawTargets (void)
{
	float az;
	SimObjectType* theTarget = platform->targetList;

	while (theTarget)
	{
		if (theTarget->localData->sensorState[Radar] == SensorTrack)
		{
			az = TargetAz (platform, theTarget->BaseData()->XPos(),
							theTarget->BaseData()->YPos());

			display->AdjustOriginInViewport (az * disDeg,
							-1.0F + 2.0F * theTarget->localData->range / tdisplayRange);
			display->Line (-0.025F, -0.025F, -0.025F,  0.025F);
			display->Line ( 0.025F, -0.025F,  0.025F,  0.025F);
			display->Line (-0.025F, -0.025F,  0.025F, -0.025F);
			display->Line ( 0.025F,  0.025F, -0.025F,  0.025F);
			display->CenterOriginInViewport();
		}

		theTarget = theTarget->next;
	}
}
//MI
int RadarDopplerClass::GetInterogate(SimObjectType *rdrObj, SimObjectType *lockedTarget)
{
	int retval = -1;
	return retval;
}

void RadarDopplerClass::DrawAzElTicks(void)
{
	int i;
	float posStep;
	float curPos;

	if (IsAADclt(AzBar)) return;
	/*------------------------------------------*/
	/* elevation ticks -30 deg to +30 by 10 deg */
	/*------------------------------------------*/
	display->SetColor(GetMfdColor(MFD_LABELS));
	//MI
	if(!g_bRealisticAvionics)
	{
		posStep = (DisplayAreaViewTop - DisplayAreaViewBottom) / 6.0F;
		curPos = DisplayAreaViewBottom;
		for (i=0; i<7; i++)
		{
			if (1 != i)
				display->Line (DisplayAreaViewLeft - 0.01F, curPos,
								DisplayAreaViewLeft - 0.04F, curPos);
			curPos += posStep;
		}
	}
	else
	{
		posStep = (DisplayAreaViewTop - DisplayAreaViewBottom) / 12.0F;
		curPos = 0;
		display->Line(DisplayAreaViewLeft + 0.05F, curPos, DisplayAreaViewLeft - 0.04F, curPos);
		for (i = 0; i < 4; i++)
		{
			display->Line(DisplayAreaViewLeft + 0.01F, curPos,
							DisplayAreaViewLeft - 0.04F, curPos);
			display->Line(DisplayAreaViewLeft + 0.01F, -curPos,
							DisplayAreaViewLeft - 0.04F, -curPos);
			curPos += posStep;
		}
	}

	/*----------------------------------------*/
	/* Azimuth ticks -30 deg to +30 by 10 deg */
	/*----------------------------------------*/
	if(!g_bRealisticAvionics)	//MI this is not correct
	{
		posStep = (DisplayAreaViewRight - DisplayAreaViewLeft) / 6.0F;
		curPos = DisplayAreaViewLeft;
		for (i=0; i<7; i++)
		{ 
			display->Line (curPos, DisplayAreaViewBottom - 0.01F, 
							curPos, DisplayAreaViewBottom - 0.04F);
			curPos += posStep;	
		}
	}
	else
	{
		//our scope has 120°, not only 60°
		posStep = (DisplayAreaViewRight - DisplayAreaViewLeft) / 12.0F;
		curPos = 0;
		display->Line(curPos, DisplayAreaViewBottom - 0.15F,
						curPos, DisplayAreaViewBottom - 0.06F);
		for(i = 0; i < 4; i++)
		{
			display->Line(curPos, DisplayAreaViewBottom - 0.15F,
							curPos, DisplayAreaViewBottom - 0.10F);
			display->Line(-curPos, DisplayAreaViewBottom - 0.15F,
							-curPos, DisplayAreaViewBottom - 0.10F);
			curPos += posStep;
		}
	}

}

void RadarDopplerClass::DrawBars(void)
{
	char str[32];
	if (IsSet(MenuMode|CtlMode)) return; // JPO special modes
	if (IsAADclt(AzBar)) return;
	display->SetColor(GetMfdColor(MFD_LABELS));
	/*----------------*/
	/* number of bars */
	/*----------------*/
	sprintf(str,"%d",(bars > 0 ? bars : -bars));
	ShiAssert (strlen(str) < sizeof(str));
	LabelButton (16, str, "B"); // JPG 6 Dec 03 LabelButton (16, "B", str);

	/*--------------*/
	/* Azimuth Scan */
	/*--------------*/
	sprintf(str,"%.0f", displayAzScan * 0.1F * RTD);
	ShiAssert (strlen(str) < sizeof(str));
	LabelButton (17, "A", str);
}

void RadarDopplerClass::DrawWaterline(void)
{
	float yPos, theta;

	static const float	InsideEdge	= 0.08f;
	static const float	OutsideEdge	= 0.40f;
	static const float	Height		= 0.04f;

	display->SetColor(GetMfdColor( MFD_RADAR_WATERLINE )); // RV - I-Hawk - Draw in blue

	theta  = platform->Pitch();
	if(theta > 45.0F * DTR)
		theta = 45.0F * DTR;
	else if(theta < -45.0F * DTR)
		theta = -45.0F * DTR;

	yPos = -disDeg * theta;							  

	display->AdjustOriginInViewport (0.0F, yPos);
	display->AdjustRotationAboutOrigin (-platform->Roll());

	display->Line(	OutsideEdge,	-Height,	OutsideEdge,	0.0f);
	display->Line(	OutsideEdge,	0.0f,		InsideEdge,		0.0f);
	display->Line(	-OutsideEdge,	-Height,	-OutsideEdge,	0.0f);
	display->Line(	-OutsideEdge,	0.0f,		-InsideEdge,	0.0f);

	display->ZeroRotationAboutOrigin();
	display->CenterOriginInViewport();
	display->SetColor( tmpColor ); // RV - I-Hawk - Return to green
}

void RadarDopplerClass::DrawScanMarkers(void)
{
	float yPos;
	float curPos;
	if (IsAADclt(AzBar)) return;
	display->SetColor(GetMfdColor (MFD_ANTENNA_AZEL));
	/*----------------*/
	/* Az Scan Marker */
	/*----------------*/
	curPos = (DisplayAreaViewRight - DisplayAreaViewLeft) / (2.0F * MAX_ANT_EL) * (beamAz + seekerAzCenter);
	curPos += (DisplayAreaViewRight + DisplayAreaViewLeft) * 0.5F;
	//MI this is the other way around in reality
	if(!g_bRealisticAvionics)
	{
		display->Line (curPos, DisplayAreaViewBottom - 0.04F, curPos, DisplayAreaViewBottom - 0.07F);
		display->Line (curPos - 0.015F, DisplayAreaViewBottom - 0.07F, curPos + 0.015F, DisplayAreaViewBottom - 0.07F);
	}
	else
	{
		const static float width = 0.02F;
		//vertical line
		display->Tri(curPos - width/2, DisplayAreaViewBottom - 0.15F, curPos + width/2, DisplayAreaViewBottom - 0.15F, curPos + width/2, DisplayAreaViewBottom - 0.22F);
		display->Tri(curPos + width/2, DisplayAreaViewBottom - 0.22F, curPos - width/2, DisplayAreaViewBottom - 0.22F, curPos - width/2, DisplayAreaViewBottom - 0.15F);
		//"T" Line
		display->Tri(curPos - width*2, DisplayAreaViewBottom - 0.15F + width/2, curPos + width*2, DisplayAreaViewBottom - 0.15F + width/2, curPos + width*2, DisplayAreaViewBottom - 0.15F - width/2);
		display->Tri(curPos + width*2, DisplayAreaViewBottom - 0.15F - width/2, curPos - width*2, DisplayAreaViewBottom - 0.15F - width/2, curPos - width*2, DisplayAreaViewBottom - 0.15F + width/2);
	}

	/*----------------*/
	/* El Scan Marker */
	/*----------------*/
	//MI this is the other way around in reality
	curPos = (DisplayAreaViewTop - DisplayAreaViewBottom) / (2.0F * MAX_ANT_EL) * (beamEl + seekerElCenter);
	curPos += (DisplayAreaViewTop + DisplayAreaViewBottom) * 0.5F;
	if(!g_bRealisticAvionics)
	{
		display->Line (DisplayAreaViewLeft - 0.04F, curPos, DisplayAreaViewLeft - 0.07F, curPos);
		display->Line (DisplayAreaViewLeft - 0.07F, curPos + 0.015F, DisplayAreaViewLeft - 0.07F, curPos - 0.015F);
	}
	else
	{
		const static float width = 0.02F;
		//horizontal line
		display->Tri(DisplayAreaViewLeft - 0.04F, curPos + width/2, DisplayAreaViewLeft - 0.11F, curPos + width/2, DisplayAreaViewLeft - 0.04F, curPos - width/2);
		display->Tri(DisplayAreaViewLeft - 0.04F, curPos - width/2, DisplayAreaViewLeft - 0.11F, curPos - width/2, DisplayAreaViewLeft - 0.11F, curPos + width/2);
		//vertical line
		display->Tri(DisplayAreaViewLeft - 0.04F + width/2, curPos + width*2, DisplayAreaViewLeft - 0.04F - width/2, curPos + width*2, DisplayAreaViewLeft - 0.04F + width/2, curPos - width*2);
		display->Tri(DisplayAreaViewLeft - 0.04F + width/2, curPos - width*2, DisplayAreaViewLeft - 0.04F - width/2, curPos - width*2, DisplayAreaViewLeft - 0.04F - width/2, curPos + width*2);
	}
	/*--------------------------------------------------*/
	/* elevation reaquisition marker, remains on screen */
	/* for 10 sec after loss of target Track            */
	/*--------------------------------------------------*/
	if (reacqFlag)
	{
		display->SetColor(GetMfdColor (MFD_FCR_REAQ_IND));
		yPos = (DisplayAreaViewTop - DisplayAreaViewBottom) / (2.0F * MAX_ANT_EL) * reacqEl;
		yPos += (DisplayAreaViewTop + DisplayAreaViewBottom) * 0.5F;
		display->AdjustOriginInViewport(DisplayAreaViewLeft, yPos);
		display->Line (elReacqMark[0][0], elReacqMark[0][1],
						elReacqMark[1][0], elReacqMark[1][1]);
		display->Line (elReacqMark[2][0], elReacqMark[2][1],
						elReacqMark[1][0], elReacqMark[1][1]);
		display->CenterOriginInViewport();
	}
}

void RadarDopplerClass::DrawRangeTicks(void)
{
	static const float Hstart	= 0.90f;
	static const float Hstop	= 0.98f;

	display->SetColor(GetMfdColor(MFD_LABELS));
	display->Line (	Hstart,	 0.0f,	Hstop,  0.0f );
	display->Line (	Hstart,	 0.5f,	Hstop,  0.5f );
	display->Line (	Hstart,	-0.5f,	Hstop, -0.5f );
}

void RadarDopplerClass::DrawRange(void)
{
	char str[8];
	if (IsSet(MenuMode|CtlMode)) return;

	display->SetColor( GetMfdColor( MFD_LABELS ) );
	float x18, y18;
	float x19, y19;
	GetButtonPos(18, &x18, &y18);
	GetButtonPos(19, &x19, &y19);
	sprintf(str,"%.0f",displayRange);
	ShiAssert (strlen(str) < sizeof(str));
	display->TextLeftVertical(x18, y18 + (y19-y18)/2, str);
}

void RadarDopplerClass::DrawRangeArrows(void)
{
	static const float arrowH = 0.0375f;
	static const float arrowW = 0.0433f;
	if (IsSet(MenuMode|CtlMode)) return; // JPO special modes

	float x18, y18;
	float x19, y19;
	GetButtonPos(18, &x18, &y18); // work out button positions.
	GetButtonPos(19, &x19, &y19);
	/*----------*/
	/* up arrow */
	/*----------*/
	display->SetColor(GetMfdColor(MFD_LABELS));
	display->AdjustOriginInViewport(x19 + arrowW, y19 + arrowH/2);
	//MI 
	if(g_bRealisticAvionics && (mode == GM || mode == GMT || mode == SEA))
	{
		if(mode == GMT || mode == SEA)
		{
			if(curRangeIdx < NUM_RANGES - 3)
				display->Tri (0.0F, arrowH, arrowW, -arrowH, -arrowW, -arrowH);
		}
		else if(mode == GM)
		{
			if(curRangeIdx < NUM_RANGES - 2)
				display->Tri (0.0F, arrowH, arrowW, -arrowH, -arrowW, -arrowH);
		}
	}
	else if (curRangeIdx < NUM_RANGES - 1) 
	{
		display->Tri (0.0F, arrowH, arrowW, -arrowH, -arrowW, -arrowH);
	}
	/*------------*/
	/* down arrow */
	/*------------*/
	if (curRangeIdx > 0) {
		display->CenterOriginInViewport();
		display->AdjustOriginInViewport( x18 + arrowW, y18 - arrowH/2);
		display->Tri (0.0F, -arrowH, arrowW, arrowH, -arrowW, arrowH);
	}
	display->CenterOriginInViewport();
}
//MI
void RadarDopplerClass::DrawIFFStatus(void)
{
}
int RadarDopplerClass::GetCurScanMode(int i)
{
	return 0;
}
void RadarDopplerClass::RWSDisplay(void)
{
	int   i;
	float xPos, yPos, alt;
	char  str[12];
	float ang, vt;
	SimObjectType* rdrObj = platform->targetList;
	SimObjectLocalData* rdrData;
	int tmpColor = display->Color();
	FireControlComputer* pFCC = ((SimVehicleClass*)platform)->GetFCC();
	AircraftClass* self = SimDriver.GetPlayerAircraft();//Cobra 11/20/04
	int iff = self->GetiffEnabled();//Cobra 11/20/04
	bool isCpl = TRUE;
	if (IsIFFFlags(Dcpl))
		isCpl = FALSE;

	//MI
	Pointer = 0;

	//MI added EXP FOV
	if (fovStepCmd)
	{
		fovStepCmd = 0;
		ToggleFlag(EXP);
	}
	//MI to disable it
	//if(IsSet(EXP)&& !g_bMLU)
	//	ToggleFlag(EXP);

	display->SetColor(GetMfdColor(MFD_LABELS));
	// OSS Button Labels
	if (IsAADclt(MajorMode) == FALSE) 
		LabelButton (0, "CRM");
	if (IsAADclt(SubMode) == FALSE)
	{
		//MI
		if(lockedTarget)
			LabelButton (1, prevMode == LRS ? "LRS" : "RWS");
		else
			LabelButton (1, mode == LRS ? "LRS" : "RWS");
	}
	//MI added EXP mode to RWS
	if (IsAADclt(Fov) == FALSE)
		LabelButton (2, IsSet(EXP) ? "EXP" : "NORM", NULL, IsSet(EXP) ? (vuxRealTime & 0x080) : 0);
	if (IsAADclt(Ovrd) == FALSE) LabelButton (3, "OVRD", NULL, isEmitting == 0);
	if (IsAADclt(Cntl) == FALSE) LabelButton (4, "CTNL", NULL, IsSet(CtlMode));
	if (IsSet(MenuMode|CtlMode)) {
		MENUDisplay ();
	}
	else {
		AABottomRow ();
	}
	//Cobra 11/20/04
	if (iff && (self->iffModeChallenge != 99))
		LabelButton (16,"SCAN");

	// Set the viewport
	display->SetViewportRelative (DisplayAreaViewLeft, DisplayAreaViewTop, DisplayAreaViewRight, DisplayAreaViewBottom);

	//MI
	float tgtx, tgty;
	float brange;
	if (IsSet(EXP)) 
	{
		if (lockedTargetData) 
		{
			// work out where on the display the target is
			TargetToXY(lockedTargetData, 0, tdisplayRange, &tgtx, &tgty);
		}
		else 
		{
			tgtx = cursorX;
			tgty = cursorY;
		}
		// draw the box
		brange = 2.0f * 2 * NM_TO_FT / tdisplayRange;
		display->Line(tgtx - brange, tgty - brange, tgtx + brange, tgty - brange);
		display->Line(tgtx + brange, tgty - brange, tgtx + brange, tgty + brange);
		display->Line(tgtx + brange, tgty + brange, tgtx - brange, tgty + brange);
		display->Line(tgtx - brange, tgty + brange, tgtx - brange, tgty - brange);
	}
	else 
	{
		brange = 0;
		tgtx = 0; tgty = 0;
	}

	/*-----------------*/
	/* for all objects */
	/*-----------------*/
	while (rdrObj)
	{
		rdrData = rdrObj->localData;

		if (F4IsBadCodePtr((FARPROC) rdrObj->BaseData())) // JB 010317 CTD
		{
			rdrObj = rdrObj->next;
			continue;
		}

		/*-------------------------*/
		/* show target and history */
		/*-------------------------*/
		for (i=histno-1; i>=0; i--) // JPO - history is now dynamic
		{
			if (rdrData->rdrY[i] > 1.0)
			{
				TargetToXY(rdrData, i, tdisplayRange, &xPos, &yPos);

				//Mi
				Pointer++;
				if(Pointer > 19)
					Pointer = 0;
				/*----------------------------*/
				/* keep objects on the screen */
				/*----------------------------*/
				if (fabs(xPos) < AZL && fabs(yPos) < AZL)
				{
					if(IsSet(EXP))
					{
						float dx = xPos - tgtx;
						float dy = yPos - tgty;
						dx *= 4; // zoom factor
						dy *= 4;
						xPos = tgtx + dx;
						yPos = tgty + dy;
						if(fabs(xPos) > AZL || fabs(yPos) > AZL) 
						{
							continue;
						}
					}

					display->AdjustOriginInViewport (xPos, yPos);
					if (rdrData->rdrSy[i] >= Track)
					{
						// This _should_ be right -- based on 2D angle between target velocity
						// and our line of sight to target  SCR 10/27/97
						ang = rdrObj->BaseData()->Yaw() - rdrData->rdrX[0] - platform->Yaw();
						if (ang >= 0.0)
							ang = SCH_ANG_INC*(float)floor(ang/(SCH_ANG_INC * DTR));
						else
							ang = SCH_ANG_INC*(float)ceil(ang/(SCH_ANG_INC * DTR));
						display->AdjustRotationAboutOrigin (ang * DTR);
					}
					vt = rdrObj->BaseData()->GetVt();
					//MI we get all the AIM120 goodies in RWS too


					//Cobra 11/21/04 Are they interrogated?
					bool isCpl = TRUE;
					if (IsIFFFlags(Dcpl))
						isCpl = FALSE;

					if (g_bRealisticAvionics)
					{
						if (rdrData->rdrSy[i] >= Det && self->interrogating && isCpl)
						{
							rdrData->interrogated = TRUE;
						}
						//else if (!rdrData->rdrSy[i] >= Det && isCpl) Cobra fix warning C4804
						else if ((!(rdrData->rdrSy[i] >= Det)) && isCpl)
						{
							rdrData->interrogated = FALSE;
						}
						else if (self->interrogating && !rdrObj->BaseData()->OnGround())
						{
							rdrData->interrogated = TRUE;
							wipeIFF = TRUE;
							iffTimer = SimLibElapsedTime + 5000.0f;
						}
							
						//if (wipeIFF == TRUE && (!rdrData->rdrSy[i] >= Det) && isCpl) Cobra fix warning c4804
						if (wipeIFF == TRUE && ((!(rdrData->rdrSy[i] >= Det)) && isCpl))
							{
							rdrData->interrogated = FALSE;
							wipeIFF = FALSE;
							}
						//else if (wipeIFF == TRUE && (!rdrData->rdrSy[i] >= Det) Cobra fix warning c4804
						else if (wipeIFF == TRUE && (!(rdrData->rdrSy[i] >= Det))
							&& (iffTimer < SimLibElapsedTime))
							{
							rdrData->interrogated = FALSE;
							iffTimer = static_cast<float>(SimLibElapsedTime);
							}
						}


					if(g_bRealisticAvionics)
					{
						if (rdrObj == lockedTarget && 
										pFCC->lastMissileImpactTime > 0.0F) 
						{ // Aim Target
							if (pFCC->lastMissileImpactTime > pFCC->lastmissileActiveTime)
								DrawSymbol(AimRel, vt/SCH_FACT, i);
							else
								DrawSymbol(AimFlash, vt/SCH_FACT, i);
						}
						else
							//Cobra 11/20 Test of IFF
							//DrawSymbol(rdrData->rdrSy[i], vt/SCH_FACT, i);
							if ( rdrData->rdrSy[i] >= Det && rdrData->interrogated && TeamInfo[platform->GetTeam()]->TStance(rdrObj->BaseData()->GetTeam()) == Allied && iff && isCpl)
							{
								//
								DrawSymbol(InterogateFriend, vt/SCH_FACT, i);
							}
							else if ( rdrData->interrogated && TeamInfo[platform->GetTeam()]->TStance(rdrObj->BaseData()->GetTeam()) == Allied && iff)
							{
								//
								DrawSymbol(InterogateFriend, vt/SCH_FACT, i);
							}
						/*else
						  {
						//
						DrawSymbol(rdrData->rdrSy[i], vt/SCH_FACT, i);
						}*/ //Cobra original above but let's test
							else if (rdrData->rdrSy[i] == Det && rdrObj->BaseData()->IsSPJamming())
							{
								DrawSymbol( Jam, vt/SCH_FACT, i );
							}
							else if (rdrData->rdrSy[i] == Solid && rdrObj->BaseData()->IsSPJamming())
							{
								DrawSymbol( Jam, vt/SCH_FACT, i );
								DrawSymbol(rdrData->rdrSy[i], vt/SCH_FACT, i);
							}
							else
							{
								//
								DrawSymbol(rdrData->rdrSy[i], vt/SCH_FACT, i);
							}//Cobra 01/30/05 End new

					}
					else
						DrawSymbol(rdrData->rdrSy[i], vt/SCH_FACT, i);

					display->ZeroRotationAboutOrigin();

					//MI
					if(g_bRealisticAvionics)
					{
						if(i == 0)
						{
							alt  = -rdrObj->BaseData()->ZPos();
							if(rdrObj == lockedTarget &&
											pFCC->LastMissileWillMiss(lockedTargetData->range) &&
											(vuxRealTime & 0x180))
							{
								sprintf (str, "LOSE");
								ShiAssert (strlen(str) < sizeof(str));
								display->TextCenter(0.0F, -0.05F, str);
							}
							else if(rdrObj == lockedTarget)
							{
								alt  = -rdrObj->BaseData()->ZPos();
								sprintf(str,"%02d",(int)((alt + 500.0F)*0.001));
								ShiAssert (strlen(str) < sizeof(str));
								display->TextCenter(0.0F, -0.05F, str);
							}
							else
							{
								if ((rdrData->rdrSy[i] >= Track) || (rdrObj->BaseData()->Id() == targetUnderCursor))
								{ 
									alt  = -rdrObj->BaseData()->ZPos();
									sprintf(str,"%02d",(int)((alt + 500.0F)*0.001));
									ShiAssert (strlen(str) < sizeof(str));
									display->TextCenter(0.0F, -0.05F, str);
								}
							}
						}


						// JPO - draw hit ind.
						if (rdrObj == lockedTarget &&
										pFCC->MissileImpactTimeFlash > SimLibElapsedTime) 
						{ // Draw X
							if (pFCC->MissileImpactTimeFlash-SimLibElapsedTime  > 5.0f * CampaignSeconds ||
											(vuxRealTime & 0x200)) 
							{
								DrawSymbol(HitInd, 0, 0);
							} 
						}
					}

					if (i == 0)
					{
						//MI done above in realistic
						if(!g_bRealisticAvionics)
						{
							/*---------------------------------------------*/
							/* target under cursor or locked show altitude */
							/*---------------------------------------------*/
							if ((rdrData->rdrSy[i] >= Track) || (rdrObj->BaseData()->Id() == targetUnderCursor))
							{
								alt  = -rdrObj->BaseData()->ZPos();
								sprintf(str,"%02d",(int)((alt + 500.0F)*0.001));
								ShiAssert (strlen(str) < sizeof(str));
								display->TextCenter(0.0F, -0.15F, str);
							}
						}

						display->SetColor(GetMfdColor(MFD_ATTACK_STEERING_CUE)); // JPO draw in yellow
						// Show collision steering if this is a locked target
						if (rdrObj == lockedTarget)
							DrawCollisionSteering(rdrObj, xPos);

						// SCR 9/9/98  If target is jamming, indicate such
						/*if (rdrObj->BaseData()->IsSPJamming())
						  {
						  DrawSymbol( Jam, 0.0f, 0 );
						  }*/ //Cobra removed for testing

						display->SetColor(tmpColor);
					}
					display->CenterOriginInViewport();

				} 
			}
		}
		rdrObj = rdrObj->next;
	}
}

void RadarDopplerClass::SAMDisplay(void)
{
	float ang;
	char str[20];
	FireControlComputer* pFCC = ((SimVehicleClass*)platform)->GetFCC();


	if (!lockedTarget)
		return;
	else if (IsSet(STTingTarget))
	{
		display->SetColor(GetMfdColor(MFD_LABELS));
		if(IsAADclt(MajorMode) == FALSE) LabelButton (0, "CRM");
		if (IsAADclt(SubMode) == FALSE)
		{
			//MI
			if(lockedTarget)
				LabelButton (1, prevMode == LRS ? "LRS" : "RWS");
			else
				LabelButton (1, mode == LRS ? "LRS" : "RWS");
		}
		//LabelButton (2, "NORM");
		if(IsAADclt(Ovrd) == FALSE) LabelButton (3, "OVRD", NULL, !isEmitting); // JPO 
		if(IsAADclt(Cntl) == FALSE) LabelButton (4, "CTNL", NULL, IsSet(CtlMode));
		if (IsSet(MenuMode|CtlMode)) {
			MENUDisplay ();
		} else {
			// OSS Button Labels
			AABottomRow ();
		}
		STTDisplay();
	}
   else
   {
      // DLZ?
      if (pFCC->GetSubMode() == FireControlComputer::Aim120 && pFCC->missileTarget)
         DrawDLZSymbol();

	   /*---------------------*/
	   /* Sam Mode Track data */
	   /*---------------------*/
	   // Aspect
       ang = max ( min (lockedTargetData->aspect * RTD * 0.1F, 18.0F), -18.0F);
	   sprintf (str, "%02.0f%c", ang, (lockedTargetData->azFrom > 0.0F ? 'R' : 'L'));
	   ShiAssert( strlen(str) < sizeof(str) );
      display->TextLeft(-0.875F, SECOND_LINE_Y, str);

	   // Heading
	   ang = int(((lockedTarget->BaseData()->Yaw()*RTD*.1f)+.5f))*10.0F;
      if(ang < 0.0F)
         ang += 360.0F;
	   sprintf (str, "%03.0f", ang);
	   ShiAssert( strlen(str) < sizeof(str) );
	   display->TextLeft(-0.5F, SECOND_LINE_Y, str);


	   // Closure
	   display->SetColor(GetMfdColor (MFD_TGT_CLOSURE_RATE));
      ang = max ( min (-lockedTargetData->rangedot * FTPSEC_TO_KNOTS, 1500.0F), -1500.0F);
	  if (ang > 0)//Cobra at the + to closure
		  {
		  sprintf (str, "+%03.0f", ang);
		  }
	  else
		  {
		sprintf (str, "%03.0f", ang);
		  }
	   ShiAssert( strlen(str) < sizeof(str) );
      display->TextRight (0.875F, SECOND_LINE_Y, str);

	   // TAS
       ang = max ( min (lockedTarget->BaseData()->GetKias(), 1000.0F), -1000.0F);
	   sprintf (str, "%03.0f", ang);
	   ShiAssert( strlen(str) < sizeof(str) );
      display->TextRight (0.45F, SECOND_LINE_Y, str);

	   // SAM Target Elevation
	   display->AdjustOriginInViewport (-0.83F, TargetEl(platform,
		   lockedTarget) * disDeg);
	   display->Line (elMark[0][0], elMark[0][1], elMark[1][0], elMark[1][1]);
	   display->Line (elMark[2][0], elMark[2][1], elMark[3][0], elMark[3][1]);
	   display->CenterOriginInViewport();

	   // SAM Target Azimuth
	   display->AdjustOriginInViewport (TargetAz(platform,
		   lockedTarget) * disDeg, -0.77F);
	   display->AdjustRotationAboutOrigin(270.0F * DTR);
	   display->Line (elMark[0][0], elMark[0][1], elMark[1][0], elMark[1][1]);
	   display->Line (elMark[2][0], elMark[2][1], elMark[3][0], elMark[3][1]);
	   display->ZeroRotationAboutOrigin();
	   display->CenterOriginInViewport();

		// Add NCTR data for any bugged target
		if(lockedTargetData->rdrSy[0] == Track || lockedTargetData->rdrSy[0] ==FlashTrack)
		{
			DrawNCTR(true);
		}

		if (subMode == SAM_AUTO_MODE && lockedTargetData->range)
		{
			TargetToXY(lockedTargetData, 0, tdisplayRange, &cursorX, &cursorY);
			//cursorY += 0.09F;
		}

		// Add the rest of the RWS blips
		RWSDisplay();

		// Add Azimuth limit markers
		DrawAzLimitMarkers();
	}
}

void RadarDopplerClass::ACMDisplay (void)
{
	display->SetColor(GetMfdColor (MFD_LABELS));

	// OSS Button Labels
	if(IsAADclt(MajorMode) == FALSE) LabelButton (0, "ACM");

	if(IsAADclt(SubMode) == FALSE) {
		switch (mode)
		{
				case ACM_30x20:
						LabelButton(1, "20");
						break;

				case ACM_SLEW:
						LabelButton(1, "SLEW");
						break;

				case ACM_BORE:
						LabelButton(1, "BORE");
						break;

				case ACM_10x60:
						LabelButton(1, "60");
						break;
		}
	}
	if(IsAADclt(Ovrd) == FALSE) LabelButton (3, "OVRD", NULL, !isEmitting);
	if(IsAADclt(Cntl) == FALSE) LabelButton (4, "CTNL", NULL, IsSet(CtlMode));
	if (IsSet(MenuMode|CtlMode)) {
		MENUDisplay ();
	} else 
		AABottomRow ();


	if (IsSet(STTingTarget))
		STTDisplay();
	else
	{
		if (mode != ACM_BORE)
		{
			// Set the viewport
			display->SetViewportRelative (DisplayAreaViewLeft, DisplayAreaViewTop, DisplayAreaViewRight, DisplayAreaViewBottom);
			DrawAzLimitMarkers();
		}

		if (mode == ACM_SLEW)
			DrawSlewCursor();
	}
}

void RadarDopplerClass::TWSDisplay (void)
{
	float xPos, yPos, alt;
	char  str[12];
	float ang, vt;
	SimObjectType* rdrObj = platform->targetList;
	SimObjectLocalData* rdrData;
	FireControlComputer* pFCC = ((SimVehicleClass*)platform)->GetFCC();
	int tmpColor = display->Color(); 
	AircraftClass* self = SimDriver.GetPlayerAircraft();//Cobra 11/20/04
	int iff = self->GetiffEnabled();//Cobra 11/20/04

	// MD -- 20040120: move the OSB labeling up a bit so that we can do STT and then bail out of 
	// this display mode (else go on to normal TWS display).

	display->SetColor(GetMfdColor (MFD_LABELS));

	// OSS Button Labels
	if(IsAADclt(MajorMode) == FALSE) { LabelButton (0, "CRM"); }
	if(IsAADclt(SubMode) == FALSE)   { LabelButton (1, "TWS"); }
	if (IsAADclt(Fov) == FALSE){
		// flash faster
		LabelButton (2, IsSet(EXP) ? "EXP" : "NORM", NULL, IsSet(EXP) ? (vuxRealTime & 0x080) : 0);
	}
	if(IsAADclt(Ovrd) == FALSE)      { LabelButton (3, "OVRD", NULL, !isEmitting); }
	if(IsAADclt(Cntl) == FALSE)      { LabelButton (4, "CTNL", NULL, IsSet(CtlMode)); }
	if (IsSet(MenuMode|CtlMode)){
		MENUDisplay();
	} 
	else {
		AABottomRow ();
	}

	// MD -- 20040120: use the STTmode() display if we are locked (versus merely bugged)
	if (lockedTarget && IsSet(STTingTarget))
	{
		STTDisplay();
		return;
	}

	//Cobra 11/20/04
	if (iff && (self->iffModeChallenge != 99)){
		LabelButton (16,"SCAN");
	}

	//MI
	Pointer = 0;

	if (fovStepCmd){
		fovStepCmd = 0;
		ToggleFlag(EXP);
	}

	// Labels / Text
	if (lockedTargetData){
		display->SetColor(GetMfdColor (MFD_TGT_CLOSURE_RATE));

		// Aspect
		ang = max ( min (lockedTargetData->aspect * RTD * 0.1F, 18.0F), -18.0F);
		sprintf (str, "%02.0f%c", ang, (lockedTargetData->azFrom > 0.0F ? 'R' : 'L'));
		ShiAssert (strlen(str) < sizeof(str));
		display->TextLeft(-0.875F, SECOND_LINE_Y, str);

		// Heading
		ang = ((((lockedTarget->BaseData()->Yaw()*RTD*.1f)+.5f))*10.0F);
		if (ang < 0.0){ ang += 360.0F; }
		sprintf (str, "%03.0f", ang);
		ShiAssert (strlen(str) < sizeof(str));
		display->TextLeft(-0.5F, SECOND_LINE_Y, str);

		// Closure
		ang = max( min( -lockedTargetData->rangedot * FTPSEC_TO_KNOTS, 1500.0F), -1500.0F);
		sprintf (str, "%03.0f", ang);
		ShiAssert (strlen(str) < sizeof(str));
		display->TextRight (0.875F, SECOND_LINE_Y, str);

		// TAS
		ang = max( min( lockedTarget->BaseData()->GetKias(), 1500.0F), -1500.0F);
		sprintf (str, "%03.0f", ang);
		ShiAssert (strlen(str) < sizeof(str));
		display->TextRight (0.45F, SECOND_LINE_Y, str);

		// Add NCTR data for any bugged target
		if (lockedTargetData->rdrSy[0] == Bug || lockedTargetData->rdrSy[0] == FlashBug)
		{
			DrawNCTR(true);
			// JPO - also do AIM120 DLZ
			if (pFCC->GetSubMode() == FireControlComputer::Aim120 && pFCC->missileTarget)
				DrawDLZSymbol();
		}
	}

	// Draw in the drawing area only
	display->SetViewportRelative (
		DisplayAreaViewLeft, DisplayAreaViewTop, DisplayAreaViewRight, DisplayAreaViewBottom
	);

	float tgtx, tgty;
	float brange;
	if (IsSet(EXP)) {
		if (lockedTargetData) {
			// work out where on the display the target is
			TargetToXY(lockedTargetData, 0, tdisplayRange, &tgtx, &tgty);
		}
		else {
			tgtx = cursorX;
			tgty = cursorY;
		}
		// draw the box
		brange = 2.0f * 2 * NM_TO_FT / tdisplayRange;
		display->Line(tgtx - brange, tgty - brange, tgtx + brange, tgty - brange);
		display->Line(tgtx + brange, tgty - brange, tgtx + brange, tgty + brange);
		display->Line(tgtx + brange, tgty + brange, tgtx - brange, tgty + brange);
		display->Line(tgtx - brange, tgty + brange, tgtx - brange, tgty - brange);
	}
	else {
		brange = 0;
		tgtx = 0; tgty = 0;
	}

	/*-----------------*/
	/* for all objects */
	/*-----------------*/
	while (rdrObj)
	{
		rdrData = rdrObj->localData;
		//Cobra We put this here becuase > 1.0 below won't let us in the loop at times.
		 //if (!rdrData->rdrSy[0] >= Det) Cobra fix warning C4804
		if (!(rdrData->rdrSy[0] >= Det))
			{
				rdrData->interrogated = FALSE;
			}
		//End


		/*-------------------------*/
		/* show target and history */
		/*-------------------------*/
		if (rdrData->rdrY[0] > 1.0)
		{
			TargetToXY(rdrData, 0, tdisplayRange, &xPos, &yPos);

			/*----------------------------*/
			/* keep objects on the screen */
			/*----------------------------*/
			if (fabs(xPos) < AZL && fabs(yPos) < AZL)
			{
				if (IsSet(EXP)) 
#if 0	// check if its in the box
					&& 
							xPos > tgtx - brange && xPos < tgtx + brange &&
							yPos > tgty - brange && yPos < tgty + brange
#endif
							{ 
								float dx = xPos - tgtx;
								float dy = yPos - tgty;
								dx *= 4; // zoom factor
								dy *= 4;
								xPos = tgtx + dx;
								yPos = tgty + dy;
								if (fabs(xPos) > AZL || fabs(yPos) > AZL) 
								{
									rdrObj = rdrObj->next;
									continue;
								}
							}

				//Mi
				Pointer++;
				if(Pointer > 19)
					Pointer = 0;

				display->AdjustOriginInViewport (xPos, yPos);
				if (rdrData->rdrSy[0] >= Track)
				{
					// This _should_ be right -- based on 2D angle between target velocity
					// and our line of sight to target  SCR 10/27/97
					ang = rdrObj->BaseData()->Yaw() - rdrData->rdrX[0] - platform->Yaw();
					if (ang >= 0.0)
						ang = SCH_ANG_INC*(float)floor(ang/(SCH_ANG_INC * DTR));
					else
						ang = SCH_ANG_INC*(float)ceil(ang/(SCH_ANG_INC * DTR));
					display->AdjustRotationAboutOrigin (ang * DTR);
				}
				//Cobra
				if (g_bRealisticAvionics)
				{
					if (rdrData->rdrSy[0] >= Track && self->interrogating)
					{
						rdrData->interrogated = TRUE;
					}
				}

				vt = rdrObj->BaseData()->GetVt();
				if (rdrObj == lockedTarget && 
								pFCC->lastMissileImpactTime > 0.0F) 
				{ // Aim Target
					if (pFCC->lastMissileImpactTime > pFCC->lastmissileActiveTime)
						DrawSymbol(AimRel, vt/SCH_FACT, 0);
					else
						DrawSymbol(AimFlash, vt/SCH_FACT, 0);
				}
				else
				{
					/*if (rdrData->TWSTrackFileOpen)
					  {
					  if (rdrData->extrapolateStart && (SimLibElapsedTime > (rdrData->extrapolateStart + TwsFlashTime)))
					// MD -- 20040121: provide data for flash check
					DrawSymbol(rdrData->rdrSy[0], vt/SCH_FACT, 0, 1);  
					else
					DrawSymbol(rdrData->rdrSy[0], vt/SCH_FACT, 0);
					}
					else
					{
					// search targets are drawn with history
					for (int i=histno-1; i>=0; i--)
					{
					DrawSymbol(rdrData->rdrSy[i], vt/SCH_FACT, 0);
					}
					}*/
					//Cobra 11/20/04 Rewrite for IFF

					//Add more here
					//Cobra 11/21/04 Are they interrogated?
					bool isCpl = TRUE;
					if (IsIFFFlags(Dcpl))
						isCpl = FALSE;

					if (g_bRealisticAvionics)
					{
						if (rdrData->rdrSy[0] >= Det && self->interrogating && isCpl)
						{
							rdrData->interrogated = TRUE;
						}
						//else if (!rdrData->rdrSy[0] >= Det && isCpl) Cobra fix warning C4804
						else if ((!(rdrData->rdrSy[0] >= Det)) && isCpl)
						{
							rdrData->interrogated = FALSE;
						}
						else if (self->interrogating && !rdrObj->BaseData()->OnGround())
						{
							rdrData->interrogated = TRUE;
							wipeIFF = TRUE;
							iffTimer = SimLibElapsedTime + 5000.0f;
						}
							
						//if (wipeIFF == TRUE && (!rdrData->rdrSy[0] >= Det) && isCpl) Cobra fix warning C4804
						if (wipeIFF == TRUE && ((!(rdrData->rdrSy[0] >= Det)) && isCpl))
						{
							rdrData->interrogated = FALSE;
							wipeIFF = FALSE;
						}
						//else if (wipeIFF == TRUE && (!rdrData->rdrSy[0] >= Det) Cobra fix warning C4804
						else if (wipeIFF == TRUE && (!(rdrData->rdrSy[0] >= Det))
							&& (iffTimer < SimLibElapsedTime))
						{
							rdrData->interrogated = FALSE;
							iffTimer = (float)SimLibElapsedTime;
						}
					}
					//End

					if (rdrData->TWSTrackFileOpen)
					{
						if (rdrData->extrapolateStart && (SimLibElapsedTime > (rdrData->extrapolateStart + TwsFlashTime)))
						{
							if (
								rdrData->interrogated && 
								TeamInfo[platform->GetTeam()]->TStance(rdrObj->BaseData()->GetTeam()) == Allied &&
								iff
							){
								DrawSymbol(InterogateFriend, vt/SCH_FACT, 0, 1);
							}
							//Cobra added this
							else if (
								rdrData->rdrSy[0] == Det && 
								rdrObj->BaseData()->IsSPJamming()
							){								
								DrawSymbol( Jam, 0.0f, 0);
							}//end
							else
							{
								// sfr: speed here, triplicated below... =(
								DrawSymbol(rdrData->rdrSy[0], vt/SCH_FACT, 0, 1);  
							}
						}
						else
						{
							if ( 
								rdrData->interrogated && 
								TeamInfo[platform->GetTeam()]->TStance(rdrObj->BaseData()->GetTeam()) == Allied && 
								iff)
							{
								DrawSymbol(InterogateFriend, vt/SCH_FACT, 0);
							}
							else if (rdrData->rdrSy[0] == Det && rdrObj->BaseData()->IsSPJamming())//Cobra added this
							{
								DrawSymbol( Jam, 0.0f, 0);
							}//end
							else
							{
								DrawSymbol(rdrData->rdrSy[0], vt/SCH_FACT, 0);
							}

						}
					}
					else
						/*{
						// search targets are drawn with history
						for (int i=histno-1; i>=0; i--)
						{
						DrawSymbol(rdrData->rdrSy[i], vt/SCH_FACT, 0);
						}
						}*/
						//Cobra 11/20/04 IFF
					{
						for (int i=histno-1; i>=0; i--)
						{

							//This works, but we need more
							/*if (g_bRealisticAvionics)
							  {
							  if (rdrData->rdrSy[i] >= Det && self->interrogating)
							  {
							  rdrData->interrogated = TRUE;
							  }
							  }*/

							if ( rdrData->rdrSy[i] >= Det && rdrData->interrogated && TeamInfo[platform->GetTeam()]->TStance(rdrObj->BaseData()->GetTeam()) == Allied  && iff)
							{
								DrawSymbol(InterogateFriend, vt/SCH_FACT, 0);
							}
							else if (rdrData->rdrSy[i] == Det && rdrObj->BaseData()->IsSPJamming())//Cobra added this
							{
								DrawSymbol( Jam, 0.0f, 0);
							}//end
							else if (rdrData->rdrSy[i] == Solid && rdrObj->BaseData()->IsSPJamming())
							{
								DrawSymbol( Jam, 0.0f, 0);//Cobra fixed for jamming aircraft
								DrawSymbol(rdrData->rdrSy[i], vt/SCH_FACT, 0);
							}
							else
							{
								DrawSymbol(rdrData->rdrSy[i], vt/SCH_FACT, 0);
							}
						}
					}
				}

				display->ZeroRotationAboutOrigin();

				if (rdrData->rdrSy[0] >= Track)
				{
					if (rdrData->rdrSy[0] >= Bug)
					{
						DrawCollisionSteering(rdrObj, xPos);
					}
					alt  = -rdrObj->BaseData()->ZPos();
					if (rdrObj == lockedTarget &&
									pFCC->LastMissileWillMiss(lockedTargetData->range) &&
									(vuxRealTime & 0x180))
						sprintf (str, "LOSE");
					else 
						sprintf(str,"%02d",(int)((alt + 500.0F)*0.001));
					ShiAssert (strlen(str) < sizeof(str));
					display->TextCenter(0.0F, -0.05F, str);

					// JPO - draw hit ind.
					if (rdrObj == lockedTarget &&
									pFCC->MissileImpactTimeFlash > SimLibElapsedTime) { // Draw X
						if (pFCC->MissileImpactTimeFlash-SimLibElapsedTime  > 5.0f * CampaignSeconds ||
										(vuxRealTime & 0x200)) 
						{
							DrawSymbol(HitInd, 0, 0);
						}
					}
				}

				display->SetColor(GetMfdColor(MFD_UNKNOWN));
				/*------------------------------------*/
				/* target under cursor, show altitude */
				/*------------------------------------*/
				if (rdrObj->BaseData()->Id() == targetUnderCursor)
				{
					alt  = -rdrObj->BaseData()->ZPos();
					sprintf(str,"%02d",(int)((alt + 500.0F)*0.001));
					ShiAssert (strlen(str) < sizeof(str));
					display->TextCenter(0.0F, -0.05F, str);
				}

				// SCR 9/9/98  If target is jamming, indicate such (radar hit or no)
				/*if (rdrObj->BaseData()->IsSPJamming())
				  {
				  DrawSymbol( Jam, 0.0f, 0);
				  }*/ //Cobra removed and moved above
				display->CenterOriginInViewport();
				display->SetColor(tmpColor);		
			}
		}
		rdrObj = rdrObj->next;
	}
}

void RadarDopplerClass::VSModeDisplay (void)
{
	display->SetColor(GetMfdColor (MFD_LABELS));
	// OSS Button Labels
	if(IsAADclt(MajorMode) == FALSE) LabelButton (0, "CRM");
	//MI this is labeled VSR, not VS
	if(!g_bRealisticAvionics)
	{
		if(IsAADclt(SubMode) == FALSE) LabelButton (1, "VS");
	}
	else
	{
		if(IsAADclt(SubMode) == FALSE) LabelButton (1, "VSR");
	}

	if(IsAADclt(Ovrd) == FALSE) LabelButton (3, "OVRD", NULL, !isEmitting);
	if(IsAADclt(Cntl) == FALSE) LabelButton (4, "CTNL", NULL, IsSet(CtlMode));
	if (IsSet(MenuMode|CtlMode))
		MENUDisplay();
	else
		AABottomRow ();

	if (IsSet(STTingTarget))
		STTDisplay();
}

void RadarDopplerClass::STTDisplay(void)
{
	float xPos, yPos, alt;
	char  str[12];
	float ang, vt;
	FireControlComputer* pFCC = ((SimVehicleClass*)platform)->GetFCC();
	int tmpColor = display->Color();
	//MI
	Pointer = 0;

	if (!lockedTarget)
		return;

	if (pFCC->GetSubMode() == FireControlComputer::Aim120 && pFCC->missileTarget)
		DrawDLZSymbol();

	display->SetColor(GetMfdColor (MFD_TGT_CLOSURE_RATE));

	// Aspect 
	sprintf (str, "%02.0f%c", (lockedTargetData->aspect * RTD) * 0.1F,
					(lockedTargetData->azFrom > 0.0F ? 'R' : 'L'));
	display->TextLeft(-0.875F, SECOND_LINE_Y, str);
	ShiAssert (strlen(str) < sizeof(str));

	// Heading 
	ang = static_cast<int>(((lockedTarget->BaseData()->Yaw()*RTD*.1f)+.5f))*10.0F;
	if (ang < 0.0){
		ang += 360.0F;
	}
	sprintf (str, "%03.0f", ang);
	ShiAssert (strlen(str) < sizeof(str));
	display->TextLeft(-0.5F, SECOND_LINE_Y, str);

	// Closure 
	/*sprintf (str, "%03.0f", -lockedTargetData->rangedot * FTPSEC_TO_KNOTS);
	ShiAssert (strlen(str) < sizeof(str));
	display->TextRight (0.875F, SECOND_LINE_Y, str);*/
	ang = max ( min (-lockedTargetData->rangedot * FTPSEC_TO_KNOTS, 1500.0F), -1500.0F);
	if (ang > 0){
		//Cobra at the + to closure	
		sprintf (str, "+%03.0f", ang);
	}
	else {
		sprintf (str, "%03.0f", ang);
	}
	ShiAssert( strlen(str) < sizeof(str) );
    display->TextRight (0.875F, SECOND_LINE_Y, str);

	// TAS 
	sprintf (str, "%03.0f", lockedTarget->BaseData()->GetKias());
	ShiAssert (strlen(str) < sizeof(str));
	display->TextRight (0.45F, SECOND_LINE_Y, str);

	// Add NCTR data for any bugged target //me123 addet check on ground & jamming
	if (!lockedTarget->BaseData()->OnGround() || !lockedTarget->BaseData()->IsSPJamming()){
		DrawNCTR(false);
	}
	// Set the viewport
	display->SetViewportRelative(
		DisplayAreaViewLeft, DisplayAreaViewTop, DisplayAreaViewRight, DisplayAreaViewBottom
	);

	/*-------------------------*/
	/* show target and history */
	/*-------------------------*/
	if (lockedTargetData->rdrY[0] > 1.0)
	{
		TargetToXY(lockedTargetData, 0, tdisplayRange, &xPos, &yPos);

		/*----------------------------*/
		/* keep objects on the screen */
		/*----------------------------*/
		if (fabs(xPos) < AZL && fabs(yPos) < AZL)
		{
			display->AdjustOriginInViewport (xPos, yPos);
			// This _should_ be right -- based on 2D angle between target velocity
			// and our line of sight to target  SCR 10/27/97
			ang = lockedTarget->BaseData()->Yaw() - lockedTargetData->rdrX[0] - platform->Yaw();
			if (ang >= 0.0){
				ang = SCH_ANG_INC*(float)floor(ang/(SCH_ANG_INC * DTR));
			}
			else{
				ang = SCH_ANG_INC*(float)ceil(ang/(SCH_ANG_INC * DTR));
			}
			display->AdjustRotationAboutOrigin (ang * DTR);

			vt = lockedTarget->BaseData()->GetVt();
			display->SetColor(GetMfdColor(MFD_UNKNOWN));
			if (pFCC->lastMissileImpactTime > 0.0F) 
			{ // Aim Target
				if (pFCC->lastMissileImpactTime > pFCC->lastmissileActiveTime){
					DrawSymbol(AimRel, vt/SCH_FACT, 0);
				}
				else {
					DrawSymbol(AimFlash, vt/SCH_FACT, 0);
				}
			}
			else {	     
				DrawSymbol(Bug, vt/SCH_FACT, 0);
			}

			int bcol = display->Color();
			// Marco Edit - Rotation is rotating the CATA symbol
			display->ZeroRotationAboutOrigin();
			DrawCollisionSteering(lockedTarget, xPos);
			//display->ZeroRotationAboutOrigin();

			alt  = -lockedTarget->BaseData()->ZPos();
			// draw lose indication
			if (pFCC->LastMissileWillMiss(lockedTargetData->range) && (vuxRealTime & 0x180)){
				sprintf (str, "LOSE");
			}
			else {
				sprintf(str,"%02d",(int)((alt + 500.0F)*0.001));
			}
			ShiAssert (strlen(str) < sizeof(str));
			display->SetColor(bcol);
			display->TextCenter(0.0F, -0.15F, str);
			// draw hit indication
			if (pFCC->MissileImpactTimeFlash > SimLibElapsedTime) { // Draw X
				if (pFCC->MissileImpactTimeFlash-SimLibElapsedTime > 5.0f * CampaignSeconds ||
								(vuxRealTime & 0x200)) {
					DrawSymbol(HitInd, 0, 0);
				}
			}

			// SCR 9/9/98  If target is jamming, indicate such
			if (lockedTarget->BaseData()->IsSPJamming())
			{
				DrawSymbol( Jam, 0.0f, 0);
			}
			display->CenterOriginInViewport();
			display->SetColor(tmpColor);
		}
	}
	//MI
	Pointer++;
	if(Pointer > 19){ Pointer = 0; }
}

void RadarDopplerClass::VSDisplay(void)
{
	int   i;
	float xPos, yPos;
	SimObjectType* rdrObj = platform->targetList;
	SimObjectLocalData* rdrData;
	float alt;
	char str[8];
	int tmpColor = display->Color();
	FireControlComputer* pFCC = ((SimVehicleClass*)platform)->GetFCC();
	//MI
	Pointer = 0;

	display->SetColor(GetMfdColor (MFD_LABELS));

	// OSS Button Labels

	if(IsAADclt(MajorMode) == FALSE) LabelButton (0, "CRM");
	//MI this is VSR, not VS
	if(!g_bRealisticAvionics)
	{
		if(IsAADclt(SubMode) == FALSE) LabelButton (1, "VS");
	}
	else
	{
		if(IsAADclt(SubMode) == FALSE) LabelButton (1, "VSR");
	}

	if(IsAADclt(Ovrd) == FALSE) LabelButton (3, "OVRD", NULL, !isEmitting);
	if(IsAADclt(Cntl) == FALSE) LabelButton (4, "CTNL", NULL, IsSet(CtlMode));
	if (IsSet(MenuMode|CtlMode)) 
		MENUDisplay();
	else
		AABottomRow ();

	if (IsSet(STTingTarget))
	{
		STTDisplay();
	}
	else
	{
		// Set the viewport
		display->SetViewportRelative (DisplayAreaViewLeft, DisplayAreaViewTop, DisplayAreaViewRight, DisplayAreaViewBottom);
		DrawAzLimitMarkers();

		/*-----------------*/
		/* for all objects */
		/*-----------------*/
		while (rdrObj)
		{
			rdrData = rdrObj->localData;

			/*-------------------------*/
			/* show target and history */
			/*-------------------------*/
			for (i=histno-1; i>=0; i--)// dynamic history
			{
				if (rdrData->rdrY[i] > 1.0)
				{
					//MI
					Pointer++;
					if(Pointer > 19)
						Pointer = 0;
					TargetToXY(rdrData, i, displayRange, &xPos, &yPos);
					/*----------------------------*/
					/* keep objects on the screen */
					/*----------------------------*/
					if (fabs(xPos) < AZL && fabs(yPos) < AZL)
					{
						display->AdjustOriginInViewport (xPos, yPos);
						// JPO work out the track color by reducing each component in turn
						DrawSymbol(rdrData->rdrSy[i], 0.0F, i);
						display->SetColor(GetMfdColor(MFD_UNKNOWN)); // JPO draw in yellow

						if (rdrObj->BaseData()->Id() == targetUnderCursor)
						{
							alt  = -rdrObj->BaseData()->ZPos();
							sprintf(str,"%02d",(int)((alt + 500.0F)*0.001));
							ShiAssert (strlen(str) < sizeof(str));
							display->TextCenter(0.0F, -0.05F, str);
						}

						// SCR 9/9/98  If target is jamming, indicate such
						if (rdrObj->BaseData()->IsSPJamming())
						{
							DrawSymbol( Jam, 0.0f, 0 );
						}
						display->SetColor(tmpColor);
						display->CenterOriginInViewport();
					}
				}
			}
			rdrObj = rdrObj->next;
		}
	}
}

void RadarDopplerClass::DrawACQCursor(void)
{
	if (IsSet(STTingTarget))  // MD -- 20031222: don't draw this in STT!
		return;

	//MI
	//static const float CursorSize = 0.03f;
	static float CursorSize;
	if(!g_bRealisticAvionics)
		CursorSize = 0.03F;
	else
		CursorSize = 0.06F;

	static float TextLeftPos;
	if(!g_bRealisticAvionics)
		TextLeftPos = 0.03F;
	else
		TextLeftPos = 0.11F;

	float  up,lw,z,ang,height,theta;
	char   str[8];
	FireControlComputer* pFCC = ((SimVehicleClass*)platform)->GetFCC();//me123 addet for ccip/DTOSS ground ranging check
	int tmpColor = display->Color();

	up = lw = 0.0F;

	/* angle of radar scan center to the horizon */
	ang = seekerElCenter;
	theta  = platform->Pitch();

	if (!IsSet(SpaceStabalized))
	{
		//MI this was messed up when our platform was rolling
		//ang += theta;
		mlTrig trig;
		mlSinCos(&trig, platform->Roll());
		ang = theta + seekerElCenter * trig.cos - seekerAzCenter * trig.sin;
	}
	else if (mode == TWS && lockedTarget)
	{
		theta = 0.0F;
	}
	else
	{
		if (theta > MAX_ANT_EL)
			ang += (theta - MAX_ANT_EL);
		else if (theta < -MAX_ANT_EL)
			ang += (theta + MAX_ANT_EL);
	}

	/* cursor range */
	cursRange = tdisplayRange * (cursorY+AZL) / (2.0F*AZL);

	/* altitude of the scan center above ground */
	scanCenterAlt = -platform->ZPos() +
			cursRange * (float)sin(ang);

	/*---------------------*/
	/* find height of scan */
	/*---------------------*/
	/* height of the scan volume in degrees */
	height = ((bars > 0 ? bars : -bars) - 1) * tbarWidth + beamWidth * 2.0F;

	/* vertical dimension of the scan volume in feet */
	z = cursRange * (float)sin(height*0.5);

	/* upper scan limit in thousands */
	up = (scanCenterAlt + z) * 0.001F;
	/* lower scan limit in thousands */
	lw = (scanCenterAlt - z) * 0.001F;

	/*---------------------------------*/
	/* altitudes must be 00 <= x <= 99 */
	/*---------------------------------*/
	up = min ( max (up, -99.0F), 99.0F);
	lw = min ( max (lw, -99.0F), 99.0F);

	/*-------------*/
	/* draw cursor */
	/*-------------*/
	display->AdjustOriginInViewport (cursorX, cursorY);
	display->SetColor(GetMfdColor(MFD_CURSOR));
	display->Line (-CursorSize, CursorSize, -CursorSize, -CursorSize);
	display->Line ( CursorSize, CursorSize,  CursorSize, -CursorSize);

	/*----------------------*/
	/* add elevation limits */
	/*----------------------*/
	if (mode != VS)
	{
		//if(g_bMLU)               // JPG 9 Mar 04 - Hardcoded the negative numbers to show and made em red when they're -
		//{
		if (up > -1.0f)
		{
			sprintf(str,"%02d",(int)up);
			ShiAssert (strlen(str) < sizeof(str));
			display->TextLeftVertical(TextLeftPos+display->TextWidth("-"), 2.0F*CursorSize, str);
		}

		else
		{
			sprintf(str,"%03d",(int)up);
			ShiAssert (strlen(str) < sizeof(str));
			DWORD tempcolor=display->Color();
			display->SetColor( GetMfdColor(MFD_RED) );
			display->TextLeftVertical(TextLeftPos, 2.0F*CursorSize, str);
			display->SetColor(tempcolor);
		}

		if (lw > -1.0f)
		{
			DWORD tempcolor=display->Color();
			display->SetColor( GetMfdColor(MFD_CYAN) ); // RV - I-Hawk - Bottom cursor in cyan
			sprintf(str,"%02d",(int)lw);
			ShiAssert (strlen(str) < sizeof(str));
			display->TextLeftVertical(TextLeftPos+display->TextWidth("-"),-2.0F*CursorSize, str);
			display->SetColor(tempcolor);
		}

		else
		{
			sprintf(str,"%03d",(int)lw);
			ShiAssert (strlen(str) < sizeof(str));
			DWORD tempcolor=display->Color();
			display->SetColor( GetMfdColor(MFD_RED) );
			display->TextLeftVertical(TextLeftPos,-2.0F*CursorSize, str);
			display->SetColor(tempcolor);
		}
		//	}
		//else
		//{
		//	up = min ( max (up, 0.0F), 99.0F);
		//	lw = min ( max (lw, 0.0F), 99.0F);

		//	sprintf(str,"%02d",(int)up);
		//	ShiAssert (strlen(str) < sizeof(str));
		//	display->TextLeftVertical(TextLeftPos, 2.0F*CursorSize, str);

		//	sprintf(str,"%02d",(int)lw);
		//	ShiAssert (strlen(str) < sizeof(str));
		//	display->TextLeftVertical(TextLeftPos,-2.0F*CursorSize, str);
		//	}
		//Cobra 11/21/04 IFF count down hack test
		AircraftClass* self = SimDriver.GetPlayerAircraft();
		int mode = 0;
		mode = self->iffModeChallenge;
		if (self->runIFFInt)
		{
			sprintf(str,"%01d",(int)mode);
			ShiAssert (strlen(str) < sizeof(str));
			DWORD tempcolor=display->Color();
			display->SetColor( GetMfdColor(MFD_GREEN) );
			display->TextLeftVertical(-0.21f, 2.0F*CursorSize, str);
			display->SetColor(tempcolor);
		}
	}
	display->SetColor(tmpColor);
	display->CenterOriginInViewport();
}

void RadarDopplerClass::DrawSlewCursor(void)
{
	float  up,lw,z,ang,height,theta;
	char   str[8];
	FireControlComputer* pFCC = ((SimVehicleClass*)platform)->GetFCC();//me123 addet for ccip/DTOSS ground ranging check
	int tmpColor = display->Color();

	up = lw = 0.0F;

	ang = seekerElCenter;
	theta  = platform->Pitch();
	ang += theta;

	/* altitude of the scan center above ground */
	scanCenterAlt = -platform->ZPos() + 5.0F * NM_TO_FT * (float)sin(ang);

	/*---------------------*/
	/* find height of scan */
	/*---------------------*/
	/* height of the scan volume in degrees */
	height = ((bars > 0 ? bars : -bars) - 1) * tbarWidth + beamWidth * 2.0F;

	/* vertical dimension of the scan volume in feet */
	z = cursRange * (float)sin(height*0.5);

	/* upper scan limit in thousands */
	up = (scanCenterAlt + z) * 0.001F;
	/* lower scan limit in thousands */
	lw = (scanCenterAlt - z) * 0.001F;

	/*---------------------------------*/
	/* altitudes must be 00 <= x <= 99 */  //JPG 25 Mar 04 - Show negatives like above
	/*---------------------------------*/
	up = min ( max (up, -99.0F), 99.0F);
	lw = min ( max (lw, -99.0F), 99.0F);

	/*-------------*/
	/* draw cursor */
	/*-------------*/
	display->AdjustOriginInViewport (cursorX, cursorY);
	display->SetColor(GetMfdColor(MFD_CURSOR));
	display->Line (cursor[0][0],  cursor[0][1],  cursor[1][0],  cursor[1][1]);
	display->Line (cursor[1][0],  cursor[1][1],  cursor[2][0],  cursor[2][1]);
	display->Line (cursor[2][0],  cursor[2][1],  cursor[3][0],  cursor[3][1]);
	display->Line (cursor[3][0],  cursor[3][1],  cursor[4][0],  cursor[4][1]);
	display->Line (cursor[4][0],  cursor[4][1],  cursor[5][0],  cursor[5][1]);
	display->Line (cursor[5][0],  cursor[5][1],  cursor[6][0],  cursor[6][1]);
	display->Line (cursor[6][0],  cursor[6][1],  cursor[7][0],  cursor[7][1]);
	display->Line (cursor[7][0],  cursor[7][1],  cursor[8][0],  cursor[8][1]);
	display->Line (cursor[8][0],  cursor[8][1],  cursor[9][0],  cursor[9][1]);
	display->Line (cursor[9][0],  cursor[9][1],  cursor[10][0], cursor[10][1]);
	display->Line (cursor[10][0], cursor[10][1], cursor[11][0], cursor[11][1]);
	display->Line (cursor[11][0], cursor[11][1], cursor[0][0],  cursor[0][1]);
	display->CenterOriginInViewport();

	/*----------------------*/
	/* add elevation limits */
	/*----------------------*/
	if (up > -1.0f)
	{
		sprintf(str,"%02d",(int)up);
		ShiAssert (strlen(str) < sizeof(str));
		display->TextLeft(0.04F+display->TextWidth("-"), 0.065F, str);
	}
	else
	{
		sprintf(str,"%03d",(int)up);
		ShiAssert (strlen(str) < sizeof(str));
		DWORD tempcolor=display->Color();
		display->SetColor( GetMfdColor(MFD_RED) );
		display->TextLeft(0.04F, 0.065F, str);
		display->SetColor(tempcolor);
	}
	if (lw > -1.0f)
	{
		sprintf(str,"%02d",(int)lw);
		ShiAssert (strlen(str) < sizeof(str));
		display->TextLeft(0.04F+display->TextWidth("-"),-0.055F, str);
	}
	else
	{
		sprintf(str,"%03d",(int)lw);
		ShiAssert (strlen(str) < sizeof(str));
		DWORD tempcolor=display->Color();
		display->SetColor( GetMfdColor(MFD_RED) );
		display->TextLeft(0.04F,-0.055F, str);
		display->SetColor(tempcolor);
	}
}

void RadarDopplerClass::DrawAzLimitMarkers(void)
{
	if (IsAADclt(AzBar)) return;
	if (IsSet(STTingTarget)) return;
	float x;
	display->SetColor(GetMfdColor(MFD_FCR_AZIMUTH_SCAN_LIM));
	// az limit marker
	if (azScan < MAX_ANT_EL - beamWidth)
	{
		x = min (disDeg * (seekerAzCenter + (azScan+beamWidth)), 0.99F);
		display->Line (x , 1.0F, x, -1.0F);

		x = max (disDeg * (seekerAzCenter - (azScan+beamWidth)), -0.99F);
		display->Line (x , 1.0F, x, -1.0F);
	}
}

// JPo - redone for new symbols and new colours.
// MD -- 20040121: cleaned up the parameter list because we have something useful to do
// with something beyond the age one now.
void RadarDopplerClass::DrawSymbol(int type, float schweemLen, int age, int flash)
{
	static const float tgtSize = 0.04f;
	static const float jamSizeW = 0.12f;
	static const float jamSizeH = 0.16f;
	static const float jamNewSizeH = 0.06f;
	static const float jamNewSizeV = 0.04f;
	static const float jamNewDelta = 0.06f;
	static const float hitSizeW = 0.08f;
	static const float hitSizeH = 0.1f;
	static const float trackScale = 0.1f;
	static const float trackTriH = trackScale * (float)cos( DTR * 30.0f );
	static const float trackTriV = trackScale * (float)sin( DTR * 30.0f );
	// MD -- 20040121: use flash parameter to pass flash enable
	int flashOff = flash * (vuxRealTime & 0x200);  
	/*-------------*/
	/* draw symbol */
	/*-------------*/
	if ((type == FlashBug || type == FlashTrack) && flashOff)
		return;

	// RV - RED - WARNING...!!!!				
	// THIS IS KINDA A HACK - The 2D MFDs may be stretched by pit scaling
	// Rings would became ovals ( as it should be ) but later they r turned by AC Yaw...
	// This make the Oval to physically turn...
	// To avoid this, we remove ANY ASPECT RATION SCALING in the NAV MODE MFD making Scale X = Scale Y
	// saving it and restoring after gfx Draw
	// This is the fastest and more harmless way to fix till new code
	// and works fine, as the draw is then stretched by Texture drawing...
	float	OldScaleX=display->scaleX, OldScaleY=display->scaleY;
	display->scaleY=display->scaleX;

	switch(type)
	{
			case FlashBug:
			case AimFlash:
			case AimRel:
					display->SetColor(GetAgedMfdColor(MFD_FCR_BUGGED_FLASH_TAIL, age));
					display->Circle(0.0F, 0.0F, g_fRadarScale * trackScale);  // MD -- 20040202: fix the oversize circles
					break;
			case Bug:
					display->SetColor(GetAgedMfdColor(MFD_FCR_BUGGED, age));
					display->Circle(0.0F, 0.0F, g_fRadarScale * trackScale);  // MD -- 20040202: fix the oversize circles
					break;
			case HitInd:
					display->SetColor(GetAgedMfdColor(MFD_KILL_X, age));
					break;
			case FlashTrack:
					display->SetColor(GetAgedMfdColor(MFD_FCR_UNK_TRACK_FLASH, age));
					break;
			case Track:
					display->SetColor(GetAgedMfdColor(MFD_FCR_UNK_TRACK, age));
					break;
			case  Solid:
			case Jam:
			case  Det:
					display->SetColor(GetAgedMfdColor(MFD_FCR_UNK_TRACK, age));
					break;
					//Cobra 11/20/04
			case InterogateFriend:
					display->SetColor(GetAgedMfdColor(MFD_IFFFREIENDLY, age));
					break;

	} /*switch*/
	switch(type)
	{
			case AimFlash: // jpo draw filled square
			case AimRel:
					if (g_bRealisticAvionics && (type == AimRel || vuxRealTime & 0x080)) {	//MI changed from || flash
						display->Tri(g_fRadarScale * trackTriH/2.0f, g_fRadarScale * -trackTriV, //tail flashes faster then flash
										g_fRadarScale * trackTriH/2.0f, g_fRadarScale * -trackTriV-0.035f, 
										g_fRadarScale * -trackTriH/2.0f, g_fRadarScale * -trackTriV);
						display->Tri(g_fRadarScale * -trackTriH/2.0f, g_fRadarScale * -trackTriV, 
										g_fRadarScale * -trackTriH/2.0f, g_fRadarScale * -trackTriV-0.035f, 
										g_fRadarScale * trackTriH/2.0f, g_fRadarScale * -trackTriV-0.035f);
					}
					// fall
			case FlashBug:
			case Bug:
			case FlashTrack:
			case Track:
					if (g_bRealisticAvionics) {
						if (g_bEPAFRadarCues) { // draw a Square.
							display->Line(g_fRadarScale * tgtSize, g_fRadarScale * tgtSize, g_fRadarScale * -tgtSize, g_fRadarScale * tgtSize);
							display->Line(g_fRadarScale *  -tgtSize, g_fRadarScale * tgtSize, g_fRadarScale * -tgtSize, g_fRadarScale * -tgtSize),
									display->Line(g_fRadarScale *  -tgtSize, g_fRadarScale * -tgtSize, g_fRadarScale * tgtSize, g_fRadarScale * -tgtSize),
									display->Line(g_fRadarScale *  tgtSize, g_fRadarScale * -tgtSize, g_fRadarScale * tgtSize, g_fRadarScale * tgtSize);
							display->Line (0.0f, g_fRadarScale * tgtSize, 0.0f, g_fRadarScale * (tgtSize + DD_LENGTH*schweemLen));
						}
						else { // draw a hollow triange
							display->Line(0.0f, g_fRadarScale * trackScale, g_fRadarScale * trackTriH, g_fRadarScale * -trackTriV);
							display->Line(g_fRadarScale * trackTriH, g_fRadarScale * -trackTriV, g_fRadarScale * -trackTriH, g_fRadarScale * -trackTriV);
							display->Line(g_fRadarScale * -trackTriH, g_fRadarScale * -trackTriV, 0.0f, g_fRadarScale * trackScale);
							display->Line (0.0f, g_fRadarScale * trackScale, 0.0f, g_fRadarScale * (trackScale + DD_LENGTH*schweemLen));
						}
					}
					else {
						display->Tri (0.0f, g_fRadarScale * trackScale, g_fRadarScale * trackTriH, g_fRadarScale * -trackTriV, g_fRadarScale * -trackTriH, g_fRadarScale * -trackTriV);
						display->Line (0.0f, g_fRadarScale * trackScale, 0.0f, g_fRadarScale * (trackScale + DD_LENGTH*schweemLen));
					}
					break;
			case  Solid:
					display->Tri (g_fRadarScale * tgtSize, g_fRadarScale * tgtSize, g_fRadarScale * -tgtSize, g_fRadarScale * tgtSize, g_fRadarScale * -tgtSize, g_fRadarScale * -tgtSize);
					display->Tri (g_fRadarScale * tgtSize, g_fRadarScale * tgtSize, g_fRadarScale * tgtSize, g_fRadarScale * -tgtSize, g_fRadarScale * -tgtSize, g_fRadarScale * -tgtSize);
					break;

			case HitInd: // new symbol
					display->Line( g_fRadarScale * -hitSizeW,  g_fRadarScale * hitSizeH, g_fRadarScale * hitSizeW, g_fRadarScale * -hitSizeH );
					display->Line( g_fRadarScale *  hitSizeW,  g_fRadarScale * hitSizeH, g_fRadarScale * -hitSizeW, g_fRadarScale * -hitSizeH );
					break;

			case Jam:
					if (g_bRadarJamChevrons) { // JPO two chevrons pointing up.
						display->Line(g_fRadarScale * -jamNewSizeH, g_fRadarScale * -jamNewSizeV, 0, 0);
						display->Line(0, 0, g_fRadarScale * jamNewSizeH, g_fRadarScale * -jamNewSizeV);
						display->Line(g_fRadarScale * -jamNewSizeH, g_fRadarScale * (-jamNewSizeV+jamNewDelta), 0, g_fRadarScale * jamNewDelta);
						display->Line(0, g_fRadarScale * jamNewDelta, g_fRadarScale * jamNewSizeH, g_fRadarScale * -jamNewSizeV+jamNewDelta);
					}
					else {
						display->Line( g_fRadarScale * -jamSizeW,  g_fRadarScale * jamSizeH,  g_fRadarScale * jamSizeW, g_fRadarScale * -jamSizeH );
						display->Line( g_fRadarScale *  jamSizeW,  g_fRadarScale * jamSizeH, g_fRadarScale * -jamSizeW, g_fRadarScale * -jamSizeH );
					}
					break;//Cobra This was missing!!!

			case  Det:
					if(!g_bRealisticAvionics)
					{
						display->Line (g_fRadarScale * -tgtSize, 0.0f, g_fRadarScale * tgtSize, 0.0f);
					}
					else
					{
						//MI we get squares here too
						display->Tri (g_fRadarScale * tgtSize, g_fRadarScale * tgtSize, g_fRadarScale * -tgtSize, g_fRadarScale * tgtSize, g_fRadarScale * -tgtSize, g_fRadarScale * -tgtSize);
						display->Tri (g_fRadarScale * tgtSize, g_fRadarScale * tgtSize, g_fRadarScale * tgtSize, g_fRadarScale * -tgtSize, g_fRadarScale * -tgtSize, g_fRadarScale * -tgtSize);
					}

					break;
			case InterogateUnk:
			case InterogateFoe:
					break;
			case InterogateFriend:
					//Cobra Adding stuff here 11/20/04
					{
						display->Circle(0.0F, 0.0F, 0.03F);
					}
					break;
	} /*switch*/

	display->scaleY=OldScaleY, display->scaleX=OldScaleX;

}

int RadarDopplerClass::IsUnderCursor (SimObjectType* rdrObj, float heading)
{
	float az, xPos, yPos;
	int retval = FALSE;

	if (rdrObj->localData->rdrDetect)
	{
		/*-----------------------------------------------*/
		/* azimuth corrected for ownship heading changes */
		/*-----------------------------------------------*/
		az = rdrObj->localData->rdrX[0] + (rdrObj->localData->rdrHd[0] - heading);
		az = RES180(az);

		/*---------------------------------*/
		/* find x and y location on bscope */
		/*---------------------------------*/
		xPos = disDeg*az;
		yPos = -AZL + 2.0F*AZL*(rdrObj->localData->rdrY[0] / tdisplayRange);

		// MD -- 20040714: This function needs to worry about EXP modes for use with RWS and TWS
		// when there is a locked target (SAM or TWS with a bug respectively) so now we adjust
		// for EXP if need be.
		if(IsSet(EXP) && lockedTargetData)
		{
			float tgtx, tgty;

			// work out where on the display the target is
			TargetToXY(lockedTargetData, 0, tdisplayRange, &tgtx, &tgty);

			float dx = xPos - tgtx;
			float dy = yPos - tgty;
			dx *= 4; // zoom factor
			dy *= 4;
			xPos = tgtx + dx;
			yPos = tgty + dy;
		}

		if (xPos > cursorX - 0.02F  && xPos < cursorX + 0.02F  &&
						yPos > cursorY - 0.04F  && yPos < cursorY + 0.04F )
			retval = TRUE;
	}

	return (retval);
}

int RadarDopplerClass::IsUnderVSCursor (SimObjectType* rdrObj, float heading)
{
	float az, xPos, yPos;
	int retval = FALSE;

	if (rdrObj->localData->rdrDetect)
	{
		/*-----------------------------------------------*/
		/* azimuth corrected for ownship heading changes */
		/*-----------------------------------------------*/
		az = rdrObj->localData->rdrX[0] + (rdrObj->localData->rdrHd[0] - heading);
		az = RES180(az);

		/*---------------------------------*/
		/* find x and y location on bscope */
		/*---------------------------------*/
		xPos = disDeg*az;
		yPos = -AZL + 2.0F*AZL*(rdrObj->localData->rdrY[0] / displayRange);
		if (fabs (xPos - cursorX) < 0.04 && fabs (yPos - cursorY) < 0.04)
			retval = TRUE;
	}

	return (retval);
}

void RadarDopplerClass::DrawCollisionSteering (SimObjectType* buggedTarget, float curX)
{
	float   dx, dy, xPos = 0.0F;
	vector  collPoint;

	if (!buggedTarget || !buggedTarget->BaseData()->IsSim())
		return;
	if (IsAADclt(AttackStr)) return;

	if (FindCollisionPoint ((SimBaseClass*)buggedTarget->BaseData(), platform, &collPoint))
	{// me123 status ok. Looks like collision point is returned in World Coords.  We need to
		// make it relative to ownship so subtract out ownship pos 1st....

		//me123 addet next three lines
		collPoint.x -= platform->XPos();
		collPoint.y -= platform->YPos();
		collPoint.z -= platform->ZPos();

		dx = collPoint.x;
		dy = collPoint.y;



		xPos = (float)atan2 (dy,dx) - platform->Yaw();

		if (xPos > (180.0F * DTR))
			xPos -= 360.0F * DTR;
		else if (xPos < (-180.0F * DTR))
			xPos += 360.0F * DTR;
	}

	/*---------------------------*/
	/* steering point screen x,y */
	/*---------------------------*/
	if (fabs(xPos) < (60.0F * DTR))
	{
		xPos = xPos / (60.0F * DTR);
		xPos -= curX;

		/*-------------*/
		/* draw symbol */
		/*-------------*/
		display->SetColor(GetMfdColor (MFD_ATTACK_STEERING_CUE));
		display->AdjustOriginInViewport(xPos, 0.0F);
		if (g_bRealisticAvionics) { // draw maltese cross
			static const float MaxPos = 0.045f;
			static const float MinPos = 0.015f;
			static const float OverLap = 0.0075f;
			display->Tri(-MinPos, -MaxPos,  MinPos, -MaxPos, 0, OverLap); // triangle one down
			display->Tri(-MinPos,  MaxPos,  MinPos,  MaxPos, 0, -OverLap); // triangle two up
			display->Tri(-MaxPos, -MinPos, -MaxPos,  MinPos, OverLap, 0); // triangle three left
			display->Tri( MaxPos, -MinPos,  MaxPos,  MinPos, -OverLap, 0); // triangle four right
		} else
		{
			display->Circle(0.0F, 0.0F, 0.03F);
		}
		display->AdjustOriginInViewport(-xPos, 0.0F);
	}
}

void RadarDopplerClass::DrawSteerpoint (void)
{
	float az, range, xPos, yPos;
	float curSteerpointX, curSteerpointY, curSteerpointZ;

	if (((SimVehicleClass*)platform)->curWaypoint)
	{
		((SimVehicleClass*)platform)->curWaypoint->GetLocation (&curSteerpointX, &curSteerpointY, &curSteerpointZ);
		az = TargetAz (platform, curSteerpointX, curSteerpointY);
		range = (float)sqrt(
						(curSteerpointX - platform->XPos()) * (curSteerpointX - platform->XPos()) +
						(curSteerpointY - platform->YPos()) * (curSteerpointY - platform->YPos()));

		/*---------------------------------*/
		/* find x and y location on bscope */
		/*---------------------------------*/
		xPos = disDeg*az;
		yPos = -AZL + 2.0F*AZL*(range / tdisplayRange);

		/*----------------------------*/
		/* keep objects on the screen */
		/*----------------------------*/
		if (fabs(xPos) < AZL && fabs(yPos) < AZL)
		{
			display->SetColor(GetMfdColor (MFD_CUR_STPT));

			display->AdjustOriginInViewport (xPos, yPos);
			display->Tri(steerpoint[0][0],  steerpoint[0][1],
							steerpoint[1][0],  steerpoint[1][1],
							steerpoint[10][0],  steerpoint[10][1]);
			display->Tri(steerpoint[0][0],  steerpoint[0][1],
							steerpoint[11][0],  steerpoint[11][1],
							steerpoint[10][0],  steerpoint[10][1]);
			display->Tri(steerpoint[2][0],  steerpoint[2][1],
							steerpoint[3][0],  steerpoint[3][1],
							steerpoint[8][0],  steerpoint[8][1]);
			display->Tri(steerpoint[2][0],  steerpoint[2][1],
							steerpoint[9][0],  steerpoint[9][1],
							steerpoint[8][0],  steerpoint[8][1]);
			display->Tri(steerpoint[4][0],  steerpoint[4][1],
							steerpoint[5][0],  steerpoint[5][1],
							steerpoint[6][0],  steerpoint[6][1]);
			display->Tri(steerpoint[4][0],  steerpoint[4][1],
							steerpoint[7][0],  steerpoint[7][1],
							steerpoint[6][0],  steerpoint[6][1]);
			display->AdjustOriginInViewport (-xPos, -yPos);
		}
	}
	display->SetColor (GetMfdColor(MFD_BULLSEYE));
	// Add the bullseye
	TheCampaign.GetBullseyeSimLocation (&xPos, &yPos);
	az = TargetAz (platform, xPos, yPos);
	range = (float)sqrt(
					(xPos - platform->XPos()) * (xPos - platform->XPos()) +
					(yPos - platform->YPos()) * (yPos - platform->YPos()));

	/*---------------------------------*/
	/* find x and y location on bscope */
	/*---------------------------------*/
	xPos = disDeg*az;
	yPos = -AZL + 2.0F*AZL*(range / tdisplayRange);

	/*----------------------------*/
	/* keep objects on the screen */
	/*----------------------------*/
	if (fabs(xPos) < AZL && fabs(yPos) < AZL)
	{
		// Possible CATA Circle
		display->Circle (xPos, yPos, 0.07F);
		display->Circle (xPos, yPos, 0.04F);
		display->Circle (xPos, yPos, 0.01F);
	}
}

void RadarDopplerClass::DrawDLZSymbol(void)
{
	if (IsAADclt(Dlz)) return;
	FireControlComputer* pFCC = ((SimVehicleClass*)platform)->GetFCC();
	char  tmpStr[8];
	float rMin;
	float rMax;
	float rNeMin;
	float rNeMax;
	float yOffset;
	static float leftEdge   = 0.88F;  // .9F
	static float rightEdge  = 0.90F;
	static float bottomEdge = -0.5F;
	static float width      = 0.10F;  //.05
	static float height     = 1.4F;
	static float rangeInv = 1.0F / tdisplayRange;
	float textbottom;
	int color = display->Color();
	rMax   = pFCC->missileRMax * rangeInv;
	rMin   = pFCC->missileRMin * rangeInv;
	rNeMax = pFCC->missileRneMax * rangeInv; // Marco Edit * 0.70f;//me123 addet *0.70 ;
	rNeMin = pFCC->missileRneMin * rangeInv;

	ShiAssert(lockedTargetData != NULL);
	// Clamp in place
	rMin = min (rMin, 1.0F);
	rMax = min (rMax, 1.0F);
	rNeMin = min (rNeMin, 1.0F);
	rNeMax = min (rNeMax, 1.0F);
	display -> SetColor(GetMfdColor(MFD_DLZ));
	// Rmin/Rmax
	textbottom = bottomEdge + rMin * height;
	if ((SimDriver.GetPlayerAircraft()->Sms->MasterArm() == SMSBaseClass::Arm) || (SimDriver.GetPlayerAircraft()->Sms->MasterArm() == SMSBaseClass::Sim))
	{
		if(g_bnewAMRAAMdlz)
		{
			display->Line (leftEdge, textbottom, leftEdge + width, bottomEdge + rMin * height);
			display->Line (leftEdge, textbottom, leftEdge, bottomEdge + (rMax * .75F) * height);
			display->Line (leftEdge, bottomEdge + (rMax * .75F) * height, leftEdge + width, bottomEdge + (rMax * .75F) * height);
		}
		else   // Old stuff
		{
			display->Line (leftEdge, textbottom, leftEdge + width, bottomEdge + rMin * height);
			display->Line (leftEdge, textbottom, leftEdge, bottomEdge + rMax * height);
			display->Line (leftEdge, bottomEdge + rMax * height, leftEdge + width, bottomEdge + rMax * height);
		}

		// Range Caret
		yOffset = bottomEdge + lockedTargetData->range * rangeInv * height;
		if (g_bRealisticAvionics) { // draw a >
			display->Line (leftEdge, yOffset, leftEdge - 0.05F, yOffset - 0.05F);
			display->Line (leftEdge, yOffset, leftEdge - 0.05F, yOffset + 0.05F);
		}
		else { // draw a |-
			display->Line (leftEdge, yOffset, leftEdge - 0.05F, yOffset);
			display->Line (leftEdge - 0.05F, yOffset + 0.05F, leftEdge - 0.05F, yOffset - 0.05F);
		}

		if(g_bnewAMRAAMdlz)
		{
			sprintf (tmpStr, "%.0f ", max ( min (-lockedTargetData->rangedot * FTPSEC_TO_KNOTS, 1500.0F), -1500.0F));
			display->TextRightVertical (leftEdge - 0.015F, yOffset, tmpStr);

			// Draw "A"/"F"-pole range for missile on the rail below closure (which is done above on line 2339)
			// "A" is not used to avoid confusion w/ AMRAAM active indications
			if (pFCC->nextMissileImpactTime > 0.0F)
			{
				if (pFCC->nextMissileImpactTime > pFCC->lastmissileActiveTime)
				{
					sprintf (tmpStr, "%.0fM", (lockedTargetData->range / 6076) - (( max ( min (-lockedTargetData->rangedot * FTPSEC_TO_KNOTS, 1500.0F), -1500.0F)/6076 * (pFCC->nextMissileImpactTime - pFCC->lastmissileActiveTime) )));
				}
				else
				{
					sprintf (tmpStr, "%.0fF", (lockedTargetData->range / 6076) - (( max ( min (-lockedTargetData->rangedot * FTPSEC_TO_KNOTS, 1500.0F), -1500.0F)/6076 * pFCC->nextMissileImpactTime )));
				}
				ShiAssert (strlen(tmpStr) < sizeof(tmpStr));
				display->TextRightVertical (leftEdge - 0.06F, yOffset - 0.09F, tmpStr);
			}
		}
		else
		{
			sprintf (tmpStr, "%.0f ", max ( min (-lockedTargetData->rangedot * FTPSEC_TO_KNOTS, 1500.0F), -1500.0F));
			display->TextRightVertical (leftEdge - 0.05F, yOffset, tmpStr);
		}


		if(g_bnewAMRAAMdlz)
		{
			// Raero - Max Missile Kinematic Range
			yOffset = rMax;
			yOffset = min ( max ( 0.0F, yOffset), 1.0F);
			yOffset = bottomEdge + yOffset * height;
			display->Line (rightEdge, yOffset, rightEdge + 0.05F, yOffset + 0.05F);
			display->Line (rightEdge, yOffset, rightEdge + 0.05F, yOffset - 0.05F);
			display->Line (rightEdge + 0.05F, yOffset - 0.05F, rightEdge + 0.05F, yOffset + 0.05F);


			// No Escape Zone
			display->Line (leftEdge, bottomEdge + rMin * height, leftEdge + width - .06f, bottomEdge + rMin * height);
			display->Line (leftEdge, bottomEdge + rNeMax * height, leftEdge + width - .06f, bottomEdge + rNeMax * height);
			display->Line (leftEdge + width - .06f, bottomEdge + rMin * height, leftEdge + width - .06f, bottomEdge + rNeMax * height);

			// Used to be AMRAAM active seeker range - kludge never worked right to begin with
			// Now it's Ropt - Max Launch Range. NOTE: KLUDGE, sorta :( Assumes optimum a/c steering and high quality termination criteria
			yOffset = rMax * .85F;
			yOffset = min ( max ( 0.0F, yOffset), 1.0F);
			yOffset = bottomEdge + yOffset * height;
			display -> SetColor(GetMfdColor(MFD_STEER_ERROR_CUE));
			display->Circle (leftEdge, yOffset, 0.04F);
		}
		else
		{
			display->Line (leftEdge, bottomEdge + rNeMin * height, leftEdge + width - .06f, bottomEdge + rNeMin * height);
			display->Line (leftEdge, bottomEdge + rNeMax * height, leftEdge + width - .06f, bottomEdge + rNeMax * height);
			display->Line (leftEdge + width - .06f, bottomEdge + rNeMin * height, leftEdge + width - .06f, bottomEdge + rNeMax * height);
			// Range for immediate Active
			//LRKLUDGE
			yOffset = pFCC->missileActiveRange * rangeInv;
			yOffset = min ( max ( 0.0F, yOffset), 1.0F);
			yOffset = bottomEdge + yOffset * height;
			display -> SetColor(GetMfdColor(MFD_STEER_ERROR_CUE));
			display->Circle (leftEdge, yOffset, 0.02F);
		}

		// Draw the ASEC symbol. if its flashing, or outside the NE zone JPO
		if (g_bRealisticAvionics && ((vuxRealTime & 0x200) || lockedTargetData->range > pFCC->missileRneMax*0.7f ||
								lockedTargetData->range < pFCC->missileRneMin)) 
		{
			display -> SetColor(GetMfdColor(MFD_STEER_ERROR_CUE));
			display->Circle(0.0f, 0.0f, pFCC->Aim120ASECRadius(lockedTargetData->range));
		}

		display -> SetColor(GetMfdColor(MFD_DLZ));


		if(g_bnewAMRAAMdlz)
		{
			// Draw "A"/"F"-pole range for missile in flight -- "A" is not used to avoid confusion w/ AMRAAM active indications
			if (pFCC->lastMissileImpactTime > 0.0F)
			{
				if (pFCC->lastMissileImpactTime > pFCC->lastmissileActiveTime)
				{
					sprintf (tmpStr, "%.0fM", (lockedTargetData->range / 6076) - (( max ( min (-lockedTargetData->rangedot * FTPSEC_TO_KNOTS, 1500.0F), -1500.0F)/6076 * pFCC->lastmissileActiveTime )));
				}
				else
				{
					sprintf (tmpStr, "%.0fF", (lockedTargetData->range / 6076) - (( max ( min (-lockedTargetData->rangedot * FTPSEC_TO_KNOTS, 1500.0F), -1500.0F)/6076 * pFCC->lastMissileImpactTime )));
				}
				ShiAssert (strlen(tmpStr) < sizeof(tmpStr));
				display->TextRight (leftEdge + width, bottomEdge + 0.25F * display->TextHeight(), tmpStr);
			}
		}
		else
		{
			// Draw Time to die strings
			if (pFCC->nextMissileImpactTime > 0.0F)
			{
				if (pFCC->nextMissileImpactTime > pFCC->lastmissileActiveTime)
				{
					sprintf (tmpStr, "A%.0f", pFCC->nextMissileImpactTime - pFCC->lastmissileActiveTime);
				}
				else
				{
					sprintf (tmpStr, "T%.0f", pFCC->nextMissileImpactTime);
				}
				ShiAssert (strlen(tmpStr) < sizeof(tmpStr));
				display->TextRight (leftEdge + width, bottomEdge + 0.25F * display->TextHeight(), tmpStr);
			}
		}

		if (pFCC->lastMissileImpactTime > 0.0F)
		{
			// lose indications
			if (pFCC->LastMissileWillMiss(lockedTargetData->range)) { // JPO lose indication
				sprintf (tmpStr, "L%.0f", pFCC->lastMissileImpactTime);
			}
			else if (pFCC->lastMissileImpactTime > pFCC->lastmissileActiveTime)
			{
				sprintf (tmpStr, "A%.0f", pFCC->lastMissileImpactTime - pFCC->lastmissileActiveTime);
			}
			else
			{
				sprintf (tmpStr, "T%.0f", pFCC->lastMissileImpactTime);
			}
			ShiAssert (strlen(tmpStr) < sizeof(tmpStr));
			//       display->TextRight (leftEdge + width, textbottom - 2.1 * display->TextHeight(), tmpStr);
			display->TextRight (leftEdge + width, bottomEdge - 0.1F, tmpStr);
		}
		display->SetColor(color);
	}
}

// JPO fetch bugged target
int RadarDopplerClass::GetBuggedData (float *x, float *y, float *dir, float *speed)
{
	if (lockedTarget == NULL)
		return FALSE;
	//MI I haven't seen a rdrSy[0] state of Bug.. thus we only get it in STT.
	//if (lockedTargetData->rdrSy[0] >= Bug ||
	/*if(lockedTarget ||
	  (IsSet(STTingTarget) && !lockedTarget->BaseData()->OnGround())) */
	if (lockedTarget)//Cobra allow ground bug to show TODO work on symbology
	{
		*x = lockedTarget->BaseData()->XPos();
		*y = lockedTarget->BaseData()->YPos();
		*dir = lockedTarget->BaseData()->Yaw() - lockedTargetData->rdrX[0];
		*speed = lockedTarget->BaseData()->GetVt();
		return TRUE;
	}
	else 
		return FALSE;
}

// JPO - bottom row of MFD buttons.
void RadarDopplerClass::AABottomRow ()
{
	if (g_bRealisticAvionics) 
	{
		if (IsAADclt(Dclt) == FALSE) LabelButton(10, "DCLT", NULL, IsSet(AADecluttered));
		if (IsAADclt(Fmt1) == FALSE) DefaultLabel(11);
		if (IsAADclt(Fmt2) == FALSE) DefaultLabel(12);
		if (IsAADclt(Fmt3) == FALSE) DefaultLabel(13);
		if (IsAADclt(Swap) == FALSE) DefaultLabel(14);

		FackClass* mFaults = ((AircraftClass*)(SimDriver.GetPlayerAircraft()))->mFaults;
		if(mFaults && !(mFaults->GetFault(FaultClass::fcc_fault) == FaultClass::xmtr))
		{
			float x, y;
			char *mode = "";
			GetButtonPos (12, &x, &y);
			display->TextCenter(x, y + display->TextHeight(), mode);
			//MI add in MasterArm state and missile status
			if(SimDriver.GetPlayerAircraft()->Sms->MasterArm() == SMSBaseClass::Sim)
				mode = "SIM";
			else if(SimDriver.GetPlayerAircraft()->Sms->MasterArm() == SMSBaseClass::Arm)
			{
				if(SimDriver.GetPlayerAircraft()->Sms->curWeapon && SimDriver.GetPlayerAircraft()->Sms->CurStationOK())
					mode = "RDY";
				else if(SimDriver.GetPlayerAircraft()->Sms->curWeapon && !SimDriver.GetPlayerAircraft()->Sms->CurStationOK())
					mode = "MAL";
			}
			if(SimDriver.GetPlayerAircraft()->RFState == 1)
				mode = "QUIET";
			else if(SimDriver.GetPlayerAircraft()->RFState == 2)
				mode = "SILENT";

			display->TextCenter(x, y + display->TextHeight(), mode);
		}
		else
			if (IsAADclt(Fmt2) == FALSE) DefaultLabel(12);
	}
	else {
		LabelButton (10, "DCLT", NULL, IsSet (AADecluttered));
		LabelButton (11, "SMS");
		LabelButton (13, "MENU", NULL, 1);//me123
		LabelButton (14, "SWAP");
	}
}

void RadarDopplerClass::TargetToXY(SimObjectLocalData *localData, int hist, 
				float drange, float *x, float *y)
{
	// JB 010730 cursor position gets messed up.
	if (localData->rdrX[hist] == 0 && localData->rdrY[hist] == 0)
		return;

	float az;
	/*-----------------------------------------------*/
	/* azimuth corrected for ownship heading changes */
	/*-----------------------------------------------*/
	az = localData->rdrX[hist] + (localData->rdrHd[hist] - platform->Yaw());
	az = RES180(az);

	/*---------------------------------*/
	/* find x and y location on bscope */
	/*---------------------------------*/
	*x = disDeg*az;
	*y = -AZL + 2.0F*AZL*(localData->rdrY[hist] / drange);
}


void RadarDopplerClass::DrawNCTR(bool TWS)
{
	// 2002-02-25 ADDED BY S.G. If not capable of handling NCTR, then don't do it!
	if (!(radarData->flag & RAD_NCTR))
		return;
	// END OF ADDED SECTION 2002-02-25

	display->SetColor(GetMfdColor (MFD_DEFAULT));
#if 0 // old code
	display->Tri(0.0F, 0.8F, 0.0F, 0.75F, nctrData*NCTR_BAR_WIDTH, 0.8F);
	display->Tri(0.0F, 0.75F, nctrData*NCTR_BAR_WIDTH, 0.75F, nctrData*NCTR_BAR_WIDTH, 0.8F);
#else
	// JPO start with some checks.
	ShiAssert(FALSE == F4IsBadReadPtr(lockedTarget, sizeof *lockedTarget));
	ShiAssert(lockedTarget->BaseData() != NULL);

	// Marco Edit - here we display our NCTR data INSTEAD of the bar
	// This was grabbed from Radar360.cpp (Easy Avionics radar/*	  
	Falcon4EntityClassType *classPtr;
	char string[24];
	classPtr = (Falcon4EntityClassType*)lockedTarget->BaseData()->EntityType();
	ShiAssert(FALSE == F4IsBadReadPtr(classPtr, sizeof *classPtr));

	// NCTR strength > 2.5 for TWS, 1.9 for NCTR
	if (lockedTarget->BaseData()->IsSim() && 
					!((SimBaseClass*)lockedTarget->BaseData())->IsExploding() && 
					((!TWS && ReturnStrength(lockedTarget) > 1.9f)
					 || (TWS && ReturnStrength(lockedTarget) > 2.5f)))
	{
		if(
#if 1 // original marco
						(lockedTargetData->ataFrom * RTD > -25.0 && 
						 lockedTargetData->ataFrom * RTD < 25.0) &&
						(lockedTargetData->elFrom * RTD > -25.0 && 
						 lockedTargetData->elFrom * RTD < 25.0) )
#else // me123 suggestion
			lockedTargetData->ataFrom*RTD < 45.0f)
#endif
			{
				// 5 = DTYPE_VEHICLE
				if (classPtr->dataType == 5) 
				{
					ShiAssert(FALSE == F4IsBadReadPtr(classPtr->dataPtr, sizeof(VehicleClassDataType)));
					sprintf (string, "%.4s", &((VehicleClassDataType*)(classPtr->dataPtr))->Name[15]);
					// 2002-02-25 ADDED BY S.G. If we get the type of the vehicle, then we've identified it (this code is not CPU intensive, even if ran on every frame)
					CampBaseClass *campBase;
					if (lockedTarget->BaseData()->IsSim())
						campBase = ((SimBaseClass *)lockedTarget->BaseData())->GetCampaignObject();
					else
						campBase = ((CampBaseClass *)lockedTarget->BaseData());
					campBase->SetSpotted(platform->GetTeam(), TheCampaign.CurrentTime, 1);
					// END OF ADDED SECTION 2002-02-25
				}
				else
					sprintf(string, "%s", "UNKN");
			}
		else
			sprintf(string, "%s", "UNKN");
	}
	else 
	{
		sprintf (string, "%s", "WAIT");
	}
	ShiAssert( strlen(string) < sizeof(string) );
	display->TextCenter(0.0F, 0.75F, string);
	// End Marco Edit
#endif
}
//MI
void RadarDopplerClass::AGRangingDisplay(void)
{
	//if we have a lock, we loose it here
	DropGMTrack();
	float cX, cY;

	GetCursorPosition (&cX, &cY);

	FireControlComputer* FCC = ((SimVehicleClass*)platform)->GetFCC();
	if(!FCC)
		return;
	display->SetColor(GetMfdColor(MFD_LABELS));
	LabelButton(0, "AGR");
	if(!IsSOI())
		display->TextCenter(0.0F, 0.4F, "NOT SOI");
	else
		DrawBorder();
	DrawWaterline();
	display->SetColor(GetMfdColor (MFD_ATTACK_STEERING_CUE));
	display->AdjustOriginInViewport(FCC->groundPipperAz, FCC->groundPipperEl);
	// draw maltese cross
	static const float MaxPos = 0.045f;
	static const float MinPos = 0.015f;
	static const float OverLap = 0.0075f;
	display->Tri(-MinPos, -MaxPos,  MinPos, -MaxPos, 0, OverLap); // triangle one down
	display->Tri(-MinPos,  MaxPos,  MinPos,  MaxPos, 0, -OverLap); // triangle two up
	display->Tri(-MaxPos, -MinPos, -MaxPos,  MinPos, OverLap, 0); // triangle three left
	display->Tri( MaxPos, -MinPos,  MaxPos,  MinPos, -OverLap, 0); // triangle four right
	display->AdjustOriginInViewport(-FCC->groundPipperAz, -FCC->groundPipperEl);

	display->SetColor(GetMfdColor(MFD_LABELS));
	LabelButton(3, "OVRD", NULL, !isEmitting);

	LabelButton(4, "CNTL", NULL, IsSet(CtlMode));

	if (IsSet(MenuMode|CtlMode)) 
	{
		MENUDisplay();
		return;
	}
	LabelButton(5, "BARO");
	LabelButton(8, "CZ");
	LabelButton(19,"","10");
	LabelButton(17,"A","1");
	if (mode != OFF) 
	{
		LabelButton(7, "SP");
		LabelButton(9, "STP");
	}

	DrawAzElTicks();
	DrawScanMarkers();
	display->SetColor(GetMfdColor(MFD_LABELS));
	AABottomRow ();
	scanDir = ScanNone;
	beamAz = FCC->groundPipperAz;
	beamEl = FCC->groundPipperEl;

	display->SetColor(GetMfdColor(MFD_BULLSEYE));
	if(OTWDriver.pCockpitManager && OTWDriver.pCockpitManager->mpIcp &&
					OTWDriver.pCockpitManager->mpIcp->ShowBullseyeInfo)
		DrawBullseyeCircle(display, cX, cY);
	else
		DrawReference(display);
}
//MI
void RadarDopplerClass::DrawReference(VirtualDisplay* display)
{
	AircraftClass* self = SimDriver.GetPlayerAircraft();
	if(!self)
		return;

	const float yref = -0.8f;
	const float xref = -0.9f;
	const float deltax[] = { 0.05f,  0.02f, 0.02f, 0.005f,  0.02f, 0.02f, 0.05f };
	const float deltay[] = { 0.00f, -0.05f, 0.05f, 0.00f, -0.05f, 0.05f, 0.00f };
	const float RefAngle = 45.0f * DTR;
	float x = xref, y = yref;

	float offset = 0.0f;

	ShiAssert(self != NULL && TheHud != NULL);
	if (TheHud->waypointValid) 
	{
		offset = TheHud->waypointBearing / RefAngle;
	}
	FireControlComputer *FCC = self->GetFCC();
	ShiAssert(FCC != NULL);
	switch (FCC->GetMasterMode() )
	{
			case FireControlComputer::AirGroundBomb:
			case FireControlComputer::AirGroundRocket: // MLR 4/3/2004 - 
			case FireControlComputer::AirGroundLaser:
			case FireControlComputer::AirGroundCamera:
					if (FCC->inRange) 
					{
						offset = FCC->airGroundBearing / RefAngle;
					}
					break;
			case FireControlComputer::Dogfight:
			case FireControlComputer::MissileOverride:
			case FireControlComputer::AAGun:
					//case (FireControlComputer::Gun && FCC->GetSubMode() != FireControlComputer::STRAF):
					{
						if(lockedTarget && lockedTarget->BaseData() && !FCC->IsAGMasterMode())
						{ 
							float   dx = 0.0F, dy = 0.0F, xPos = 0.0F, tgtx = 0.0F, yPos = 0.0F;
							vector  collPoint;
							TargetToXY(lockedTargetData, 0, tdisplayRange, &tgtx, &yPos);
							if (FindCollisionPoint ((SimBaseClass*)lockedTarget->BaseData(), platform, &collPoint))
							{// me123 status ok. Looks like collision point is returned in World Coords.  We need to
								// make it relative to ownship so subtract out ownship pos 1st....

								//me123 addet next three lines
								collPoint.x -= platform->XPos();
								collPoint.y -= platform->YPos();
								collPoint.z -= platform->ZPos();

								dx = collPoint.x;
								dy = collPoint.y;



								xPos = (float)atan2 (dy,dx) - platform->Yaw();

								if (xPos > (180.0F * DTR))
									xPos -= 360.0F * DTR;
								else if (xPos < (-180.0F * DTR))
									xPos += 360.0F * DTR;
							}

							/*---------------------------*/
							/* steering point screen x,y */
							/*---------------------------*/
							if (fabs(xPos) < (60.0F * DTR))
							{
								xPos = xPos / (60.0F * DTR);
								xPos += tgtx;

								offset = xPos;
								offset -= TargetAz (platform, lockedTarget->BaseData()->XPos(),
												lockedTarget->BaseData()->YPos());
							}
							else if(xPos > (60 * DTR))
								offset = 1;
							else
								offset = -1;
						}
					}
					break;
			case FireControlComputer::Nav:
			case FireControlComputer::ILS:
			default:	//Catch all the other stuff
					if (TheHud->waypointValid) 
					{ 
						offset = TheHud->waypointBearing / RefAngle;
					} 
					break;
	}

	offset = min ( max (offset, -1.0F), 1.0F);
	for (int i = 0; i < sizeof(deltax)/sizeof(deltax[0]); i++) 
	{
		display->Line(x, y, x+deltax[i], y+deltay[i]);
		x += deltax[i]; y += deltay[i];
	}
	float xlen = (x - xref);
	x = xref + xlen/2 + offset * xlen/2.0f;
	if(g_bINS)
	{
		if(SimDriver.GetPlayerAircraft() && !SimDriver.GetPlayerAircraft()->INSState(AircraftClass::INS_HSD_STUFF))
			return;
	}
	display->Line(x, yref + 0.086f, x, yref - 0.13f);
}
//MI
void RadarDopplerClass::SetInterogateTimer(int Dir)
{
}
//MI
void RadarDopplerClass::UpdateLOSScan(void)
{
}
//MI
void RadarDopplerClass::GetBuggedIFF(float *x, float *y, int *type)
{
}
