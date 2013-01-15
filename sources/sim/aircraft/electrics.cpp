#include "stdhdr.h"
#include "aircrft.h"
#include "fack.h"
#include "airframe.h"
#include "otwdrive.h"


extern bool g_bRealisticAvionics;

// hierarchy of electriocal systems
const unsigned long AircraftClass::systemStates[PowerMaxState] = {
    NoPower, // PowerNone state
	systemStates[0],	//  PowerFlcs,
	systemStates[1],	//   PowerBattery,
	systemStates[2] | 
	    HUDPower|InteriorLightPower,   // PowerEmergencyBus,
	systemStates[3] |
	    RaltPower|MFDPower|UFCPower|APPower|IFFPower,   // PowerEssentialBus,
	systemStates[4] |
	    SMSPower| 
	    FCCPower|GPSPower|FCRPower|EWSRWRPower|MAPPower|DLPower|TISLPower|
	    LeftHptPower|RightHptPower|RwrPower|
	    InstrumentLightPower|InteriorLightPower|SpotLightPower|
	    EWSJammerPower|EWSChaffPower|EWSFlarePower|ChaffFlareCount|PFDPower,   // PowerNonEssentialBus,
};

BOOL AircraftClass::HasPower(AvionicsPowerFlags fl)
{
    ShiAssert(currentPower >= 0 && currentPower < PowerMaxState);
    return (powerFlags & systemStates[currentPower] & fl) == (unsigned int)fl ? 1 : 0;
#if 0
    return (powerFlags & fl) == (unsigned int)fl ? 1 : 0; 
#endif
}

// run electrical calculations
void AircraftClass::DoElectrics ()
{
    bool elecfault = false;
    // check the power grid.
    if (af->GeneratorRunning(AirframeClass::GenMain))
	{
		currentPower = PowerNonEssentialBus;
		//MI various electrics
		PowerOn(PFDPower);
		PowerOn(ChaffFlareCount);
		PowerOn(APPower);
		PowerOn(RaltPower);
	}
    else if (af->GeneratorRunning(AirframeClass::GenStdby))
	{
		currentPower = PowerEssentialBus;
		//MI various electrics
		PowerOn(RaltPower);
		PowerOn(APPower);
		PowerOff(PFDPower);
		PowerOff(ChaffFlareCount);
	}
    else if (af->GeneratorRunning(AirframeClass::GenEpu))
	{
		currentPower = PowerEmergencyBus;
		//MI various electrics
		PowerOff(RaltPower);
		PowerOff(APPower);
		PowerOff(PFDPower);
		PowerOff(ChaffFlareCount);
	}
    else {
	if (mainPower == MainPowerOff)
	    currentPower = PowerNone;
	else
	    currentPower = PowerBattery;
    }
    if (currentPower != PowerNonEssentialBus)
	elecfault = true;

    ElecClear(ElecToFlcs);
    ElecClear(ElecFlcsRly);
    if(currentPower == PowerBattery) { // no generator
	if (mainPower == MainPowerBatt) {
	    ElecSet(ElecFlcsRly);
	}
	else if(mainPower == MainPowerMain) {
	    ElecSet(ElecToFlcs);
	}
    }

    if (!isDigital) {
	if (elecfault)
	{
	    //MI
	    if(!g_bRealisticAvionics)
		mFaults->SetFault(elec_fault);
	    else
		mFaults->SetCaution(elec_fault);
	}
	else mFaults->ClearFault(elec_fault);
    }
}