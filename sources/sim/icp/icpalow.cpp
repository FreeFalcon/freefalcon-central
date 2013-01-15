#include "icp.h"
#include "simdrive.h"
#include "aircrft.h"
#include "hud.h"

//MI for ICP stuff
extern bool g_bRealisticAvionics;

void ICPClass::PNUpdateALOWMode(int button, int) 
{

	if(button == PREV_BUTTON) {
		if(TheHud->lowAltWarning > 1000.0F) {
			TheHud->lowAltWarning -= 1000.0F;
		}
		else {
			TheHud->lowAltWarning -= 100.0F;
		}
	}
	else {
		if(TheHud->lowAltWarning >= 1000.0F) {
			TheHud->lowAltWarning += 1000.0F;
		}
		else {
			TheHud->lowAltWarning += 100.0F;
		}
	}

	TheHud->lowAltWarning = min(max(0.0F, TheHud->lowAltWarning), 99999.0F);

	mUpdateFlags |= ALOW_UPDATE;
}
void ICPClass::ExecALOWMode(void)
{
	if(!g_bRealisticAvionics)
	{
		//MI Original code
		char	tmpstr[5] = "";
		long	alt;
		int	alt1, alt2;

		//	if(mUpdateFlags & ALOW_UPDATE) {

				mUpdateFlags &= !ALOW_UPDATE;

				sprintf(mpLine1, "ALOW LEVEL");
				
				alt = (long)TheHud->lowAltWarning;

				alt1 = (int) alt / 1000;
				alt2 = (int) alt % 1000;

				if(alt1) {
					sprintf(tmpstr, "%-3d", alt2);
					
					if(alt2 < 100) {
						tmpstr[0] = '0';
					}
					if(alt2 < 10) {
						tmpstr[1] = '0';
					}
					if(alt2 < 1) {
						tmpstr[2] = '0';
					}

					sprintf(mpLine2, "%-d,%s FT", alt1, tmpstr);
				}
				else {
					sprintf(mpLine2, "%d FT", alt2);
				}
				*mpLine3 = NULL;
		//	}
	}
	else
	{
		//Line1
		FillDEDMatrix(0,12,"ALOW");
		AddSTPT(0,22);
		//Line2
		FillDEDMatrix(1,4,"CARA ALOW");
		if(!EDITMSLFLOOR && !TFADV)
		{
			PossibleInputs = 5;
			ScratchPad(1,16,24);
		}
		else
		{
			long	alt;
			int	alt1, alt2;
			char	tmpstr[5] = "";
			alt = (long)TheHud->lowAltWarning;

			alt1 = (int) alt / 1000;
			alt2 = (int) alt % 1000;

			if(alt1) 
			{
				sprintf(tmpstr, "%-3d", alt2);
				
				if(alt2 < 100) 
					tmpstr[0] = '0';
				if(alt2 < 10)
					tmpstr[1] = '0';
				if(alt2 < 1)
					tmpstr[2] = '0';

				sprintf(tempstr, "%-d%sFT", alt1, tmpstr);
			}
			else
				sprintf(tempstr, "%dFT", alt2);
			FillDEDMatrix(1,(24-strlen(tempstr)),tempstr);
		}
		//Line3
		FillDEDMatrix(2,4,"MSL FLOOR");
		if(EDITMSLFLOOR)
		{
			PossibleInputs = 5;
			ScratchPad(2,16,24);
		}
		else
		{
			sprintf(tempstr,"%dFT", TheHud->MSLFloor);
			FillDEDMatrix(2,(24-strlen(tempstr)),tempstr);
		}
		//Line4
		FillDEDMatrix(3,1,"TF ADV (MSL)");
		if(TFADV)
		{
			PossibleInputs = 5;
			ScratchPad(3,16,24);
		}
		else
		{
			sprintf(tempstr,"%dFT",TheHud->TFAdv);
			FillDEDMatrix(3,(24-strlen(tempstr)),tempstr);
		}
	}
}
	

