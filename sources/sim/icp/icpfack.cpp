#include "stdhdr.h"
#include "aircrft.h"
#include "fack.h"
#include "icp.h"
#include "simdrive.h"

//MI for ICP stuff
extern bool g_bRealisticAvionics;

void ICPClass::ExecFACKMode(void) {
	
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if(!g_bRealisticAvionics)
	{
		//MI original Code

	   FaultClass::str_FNames	faultNames;
		int			faultCount;

		if(!playerAC) {
			return;
		}

		faultCount	= playerAC->mFaults->GetFFaultCount();

		if(mUpdateFlags & FACK_UPDATE || (!(mUpdateFlags & FACK_UPDATE) && faultCount)) {

			mUpdateFlags &= !FACK_UPDATE;

			if(!faultCount) {

				sprintf(mpLine1, "      NO FAULTS");
				sprintf(mpLine2, "");
				sprintf(mpLine3, "      ALL SYS OK");
			}
			else {

				playerAC->mFaults->GetFaultNames((FaultClass::type_FSubSystem)mFaultNum, mFaultFunc, &faultNames);

				sprintf(mpLine1, "        FAULT");
				sprintf(mpLine2, "");
				sprintf(mpLine3, "   %4s %4s %4s", faultNames.elpFSubSystemNames,
															faultNames.elpFFunctionNames,
															faultNames.elpFSeverityNames);
			}
		}
	}
	else
	{
		if(!playerAC) 
		{
			return;
		}

		if(playerAC->mFaults->GetFFaultCount() <= 0)
		{
		    PflNoFaults();
		}
		else 
		{
		    PflFault((FaultClass::type_FSubSystem)mFaultNum, (FaultClass::type_FFunction)mFaultFunc);
		}
	}
}

void ICPClass::ExecPfl()
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	if ((mUpdateFlags & FACK_UPDATE) == 0) // nothing to update;
    ClearPFLLines(); // reset the display
    mUpdateFlags &= ~FACK_UPDATE; // we'll have updated.
    if (m_FaultDisplay == false || !playerAC || !playerAC->mFaults)
	return; // nothing to show

    if (playerAC->mFaults->GetFFaultCount() <= 0) {
	PflNoFaults();
    }
    else {
		if (m_function == FaultClass::nofault)					//Wombat778 10-20-2003 removed change because it seemed to break PFL in realistic modes.  Changed code in ICPclass instead.
			playerAC->mFaults->GetFirstFault(&m_subsystem, &m_function);
		PflFault(m_subsystem, m_function);

	} 
}

void ICPClass::PflNoFaults()
{
    //Line1
    FillPFLMatrix(0,10, "NO FAULTS");
    //Line3
    FillPFLMatrix(2,9, "ALL SYS OK");
}

void ICPClass::PflFault(FaultClass::type_FSubSystem sys, int func)
{
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
	char tempstr[40];
    FaultClass::str_FNames	faultNames;

    playerAC->mFaults->GetFaultNames(sys, func, &faultNames);
    //Line1
    FillPFLMatrix(0,12,"FAULT");
    //Line3
    //fix (better hack) to prevent strange stuff beeing written onto the PFL
    sprintf(tempstr, "%4s %4s %4s", 
	faultNames.elpFSubSystemNames,
	faultNames.elpFFunctionNames,
	faultNames.elpFSeverityNames);

    FillPFLMatrix(2,((25-strlen(tempstr))/2), tempstr);
}

void ICPClass::PNUpdateFACKMode(int button, int) {

	int faultIdx;
   int failedFuncs;
   int funcIdx;
   int testFunc;
	AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

	if(!playerAC) {
		return;
	}

	if (button == PREV_BUTTON && playerAC->mFaults->GetFFaultCount() >= 1) {
		
		faultIdx = mFaultNum;
      failedFuncs = playerAC->mFaults->GetFault((FaultClass::type_FSubSystem) faultIdx);

      // previous failures on the System?
      testFunc = mFaultFunc - 1;
      if (mFaultFunc && (failedFuncs & ((1 << testFunc)- 1)) > 0)
      {
         mFaultFunc -= 2;
         funcIdx = (1 << mFaultFunc);
         while ((failedFuncs & funcIdx) == 0)
         {
            mFaultFunc --;
            funcIdx = funcIdx >> 1;

         }
      }
      else
      {
         // Find the previous sub-subsystem
		   do {
			   faultIdx--;
			   if(faultIdx < 0) {
				   faultIdx = FaultClass::NumFaultListSubSystems - 1;
			   }
			   failedFuncs = playerAC->mFaults->GetFault((FaultClass::type_FSubSystem) faultIdx);
		   }	   
		   while(!failedFuncs && faultIdx != mFaultNum);
		   

         // Find highest failed sub-system
         funcIdx = (1 << 31);
         mFaultFunc = 31;
         while ((failedFuncs & funcIdx) == 0)
         {
            mFaultFunc --;
            funcIdx = funcIdx >> 1;
         }
      }

		mFaultNum = faultIdx;
      mFaultFunc ++;
	}
	else if(button == NEXT_BUTTON && playerAC->mFaults->GetFFaultCount() >= 1) {

		faultIdx = mFaultNum;
      failedFuncs = playerAC->mFaults->GetFault((FaultClass::type_FSubSystem) faultIdx);

      // next failures on the System?
      if ((failedFuncs & ~((1 << mFaultFunc)- 1)) > 0)
      {
         funcIdx = (1 << mFaultFunc);
         while ((failedFuncs & funcIdx) == 0)
         {
            mFaultFunc ++;
            funcIdx = funcIdx << 1;
         }
      }
      else
      {
		  do
		  {
			  faultIdx++;
			  if(faultIdx >= FaultClass::NumFaultListSubSystems)
			  {
				  faultIdx = 0;
			  }
			  failedFuncs = playerAC->mFaults->GetFault((FaultClass::type_FSubSystem) faultIdx);
		  }		  
		  while(!failedFuncs && faultIdx != mFaultNum);
			  
		  // Find lowest failed sub-system
		  funcIdx = 1;
		  mFaultFunc = 0;
		  while ((failedFuncs & funcIdx) == 0)
		  {
			  mFaultFunc ++;
			  funcIdx = funcIdx << 1;
		  }
      }

		mFaultNum = faultIdx;
      mFaultFunc ++;
	}

	mUpdateFlags |= FACK_UPDATE;
}
