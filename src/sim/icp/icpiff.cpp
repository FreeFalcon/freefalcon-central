#include "icp.h"
#include "simdrive.h"
#include "aircrft.h"
#include "navsystem.h"

extern bool g_bIFF;

//This handles the userinput
void ICPClass::ExecIFFMode(void)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if(gNavigationSys->GetControlSrc() == NavigationSystem::AUXCOMM)
		IFFBackup();
	else
	{
		if(!g_bIFF)
		{
			//Line1
			FillDEDMatrix(0,2,"IFF  ON");
			FillDEDMatrix(0,12, "MAN",2);
			//Line3
			FillDEDMatrix(2,2,"M1 22");
			FillDEDMatrix(2,10,"MC",2);
			FillDEDMatrix(2,14,"(5)");
			FillDEDMatrix(2,18,"\x02",2);
			FillDEDMatrix(2,24,"\x02",2);
			//Line4
			FillDEDMatrix(3,2,"M2 3412");
			FillDEDMatrix(3,10,"M4");
			FillDEDMatrix(3,13,"B(6)");
			//Line5
			FillDEDMatrix(4,2,"M3 1234");
			FillDEDMatrix(4,10,"OUT");
			FillDEDMatrix(4,14,"(7)");
			FillDEDMatrix(4,19,"MS (8)");
			return;
		}
		//Line1
		if(playerAC && playerAC->HasPower(AircraftClass::IFFPower))
			FillDEDMatrix(0,2,"IFF  ON");
		else
			FillDEDMatrix(0,2,"IFF  OFF");
		FillDEDMatrix(0,12, "MAN",2);
		//Line3
		if(IsIFFSet(ICPClass::MODE_1))
			FillDEDMatrix(2,2,"M1",2);
		else
			FillDEDMatrix(2,2,"M1"); 
		sprintf(tempstr, "%d", Mode1Code);
		FillDEDMatrix(2,5,tempstr);

		if(IsIFFSet(ICPClass::MODE_C))
			FillDEDMatrix(2,10,"MC",2);
		else
			FillDEDMatrix(2,10,"MC");
		FillDEDMatrix(2,14,"(5)");

		ScratchPad(2,18,24);
		PossibleInputs = 5;

		//Line4
		if(IsIFFSet(ICPClass::MODE_2))
			FillDEDMatrix(3,2,"M2", 2);
		else
			FillDEDMatrix(3,2,"M2");
		sprintf(tempstr, "%d", Mode2Code);
		FillDEDMatrix(3,5, tempstr);

		if(IsIFFSet(ICPClass::MODE_4))
			FillDEDMatrix(3,10,"M4", 2);
		else
			FillDEDMatrix(3,10,"M4");
		if(IsIFFSet(ICPClass::MODE_4B))
			FillDEDMatrix(3,13,"B(6)");
		else
			FillDEDMatrix(3,13,"A(6)");
		//Line5
		if(IsIFFSet(ICPClass::MODE_3))
			FillDEDMatrix(4,2,"M3",2);
		else
			FillDEDMatrix(4,2,"M3");
		sprintf(tempstr,"%d", Mode3Code);
		FillDEDMatrix(4,5, tempstr);

		if(IsIFFSet(ICPClass::MODE_4OUT))
			FillDEDMatrix(4,10,"OUT");
		else if(IsIFFSet(ICPClass::MODE_4LIT))
			FillDEDMatrix(4,10,"LIT");
		else
			FillDEDMatrix(4,10,"AUD");
		FillDEDMatrix(4,14,"(7)");
		FillDEDMatrix(4,19,"MS (8)");
	}
}


