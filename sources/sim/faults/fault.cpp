#ifndef USE_SH_POOLS
// OW
#define NO_MALLOC_MACRO
#endif

#include "stdhdr.h"
#include "fault.h"
#include "fsound.h"
#include "soundfx.h"
#include "debuggr.h"


#ifdef USE_SH_POOLS
MEM_POOL gFaultMemPool;
#endif

// JPO
// Idea is to try and gather all fault data together. 
// pity about the probabilities...
static  float parray1[1] = { 1.0f};
static  float cmds_array[] = {0.2f, 0.6f, 1.0f};
static  float eng_array[] = {0.2f, 0.4f, 0.6f, 0.7f, 0.8f, 1.0f};
static  float eng_array2[] = {0.2f, 0.4f, 0.6f, 0.7f, 0.8f, 1.0f};//TJL 01/16/04 Multi-engine
static  float fcr_array[] = {0.3f, 0.65f, 1.0f};
static  float flcs_array[] = {0.2f, 0.4f, 0.8f, 1.0f};
static  float rudr_array[] = {0.8f, 1.0f};
static  float mfds_array[] = {0.5f, 1.0f};
static  float sms_array[] = {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.9f, 1.0f};


#define FDATA(s, type, sprob, aprob) { s, (type_FFunction)(type), (float)sprob, aprob, sizeof(aprob)/sizeof(aprob[0])}
//TJL 01/16/04 multi-engine added ENG2
const struct FaultClass::InitFaultData FaultClass::mpFaultData[FaultClass::TotalFaultStrings] = {
    FDATA("AMUX", bus,	0.1f, parray1),
	FDATA("BLKR", bus,	0.2f, parray1),
	FDATA("BMUX", bus,	0.1f, parray1),
	FDATA("CADC", bus,	0.1f, parray1),
	FDATA("CMDS", chaf|flar|bus, 0.15f, cmds_array),
	FDATA("DLNK", bus,	0.5f, parray1),
	FDATA("DMUX", bus,	0.05, parray1),
	FDATA("DTE",  bus,	0.2f, parray1),
	FDATA("ENG",  a_i|a_b|pfl|efire|hydr|fl_out, 0.4f, eng_array),
	FDATA("ENG2",  a_i|a_b|pfl|efire|hydr|fl_out, 0.4f, eng_array2),
	FDATA("EPOD", slnt,	0.2f, parray1),
	FDATA("FCC",  bus,	0.2f, parray1),
	FDATA("FCR",  bus|sngl|xmtr, 0.2f, fcr_array),
	FDATA("FLCS", dmux|dual|sngl|a_p, 0.3f, flcs_array),
	FDATA("FMS",  bus,	0.1f, parray1),
	FDATA("GEAR", ldgr,	0.5f, parray1),
	FDATA("GPS",  bus,	0.5f, parray1),
	FDATA("HARM", bus,	0.5f, parray1),
	FDATA("HUD",  bus,	0.4f, parray1),
	FDATA("IFF",  bus,	0.2f, parray1),
	FDATA("INS",  bus,	0.2f, parray1),
	FDATA("ISA",  all|rudr,	0.2f, rudr_array),
	FDATA("MFDS", lfwd|rfwd,0.3f, mfds_array),
	FDATA("MSL",  bus,	0.0f, parray1),
	FDATA("RALT", xmtr,	0.3f, parray1),
	FDATA("RWR",  bus,	0.2f, parray1),
	FDATA("SMS",  bus | sta1 | sta2 | sta3 | sta4 | sta5 | sta6 | sta7 | sta8 | sta9,
				0.1f, sms_array),
	FDATA("TCN",  bus,	0.2f, parray1),
	FDATA("UFC",  bus,	0.2f, parray1),
	FDATA("???",  bus, 0, parray1), // bogus entries
	FDATA("LAND",  bus, 0, parray1), // bogus entries
	FDATA("TOF", bus, 0, parray1), // bogus entries
};
#undef FDATA

const char* FaultClass::mpFFunctionNames[NumFaultFunctions] = {
               "",
               "BUS",
               "SLNT",
               "CHAF",
               "FLAR",
               "DMUX",
               "DUAL",
               "SNGL",
               "A/P",
               "RUDR",
               "ALL",
               "XMTR",
               "A/I",
               "A/B",
               "PFL",
               "FIRE",
               "HYDR",
               "M 3",
               "M C",
               "SLV",
               "LFWD",
               "RFWD",
               "STA1",
               "STA2",
               "STA3",
               "STA4",
               "STA5",
               "STA6",
               "STA7",
               "STA8",
               "STA9",
               "LDGR",
	       "FLOUT",
};

const char* FaultClass::mpFSeverityNames[NumFaultSeverity] = {
					"CNTL",	"DEGR",
					"FAIL",	"LOW",
					"RST",	"TEMP",
					"WARN",  ""
};

//-------------------------------------------------
// FaultClass::FaultClass
//-------------------------------------------------

FaultClass::FaultClass(void) {
	
	int	i;

	for (i = 0; i < NumFaultListSubSystems; i++) {

		mpFaultList[i].elFunction	= nofault;
		mpFaultList[i].elSeverity	= no_fail;
	}
	mFaultCount	= 0;
	ZeroMemory(mMflList, sizeof mMflList);
	mLastMfl = 0;
	mStartTime = 0;
}

FaultClass::~FaultClass(void)
{

}

//-------------------------------------------------
// FaultClass::IsFlagSet
//-------------------------------------------------

BOOL FaultClass::IsFlagSet() {

MonoPrint("remove call\n");
return FALSE;
}

//-------------------------------------------------
// FaultClass::ClearFlag
//-------------------------------------------------

void FaultClass::ClearFlag() {

}

FaultClass::type_FSubSystem FaultClass::PickSubSystem(int subsystemBits)
{
    type_FSubSystem retval = amux_fault;
    int failedThings[NumFaultListSubSystems];
    int failedThing;
    int i, j = 0;
    
    for(i = 0; i < FaultClass::NumFaultListSubSystems; i++)
    {
	if(subsystemBits & (1 << i))
	{
	    failedThings[j] = i;
	    j++;
	}
    }
    
    // Choose one of the available
    failedThing = rand() % j;
    failedThing = failedThings[failedThing];
    
    // Did it fail?
   if (mpFaultData[failedThing].mSProb <= (float)rand() / (float)RAND_MAX)
      retval = (type_FSubSystem)failedThing;

   return retval;
}

FaultClass::type_FFunction FaultClass::PickFunction(FaultClass::type_FSubSystem system)
{
    type_FFunction retval = nofault;
    float pFail = (float)rand() / (float)RAND_MAX;
    int i, counter;
    int breakable = mpFaultData[system].mBreakable;
    
    for (i=0; i<mpFaultData[system].mCount; i++)
    {
	if (mpFaultData[system].mFProb[i] >= pFail)
	{
	    break;
	}
    }

   // Find the i'th bit in the failure
   i++;
   counter = -1;
   while (i)
   {
      counter ++;
      if (breakable & (1 << counter))
         i --;
   }

   retval = (type_FFunction)(1 << counter);

   return retval;
}

//-------------------------------------------------
// FaultClass::SetFault
//-------------------------------------------------

void FaultClass::SetFault(type_FSubSystem	subsystem,
			  type_FFunction	function,
			  type_FSeverity	severity,
			  BOOL			doWarningMsg) {
    
    if(mpFaultList[subsystem].elFunction == nofault) {
	
	mFaultCount++;
	
	if(doWarningMsg) {
	    // sound effect warning goes here?
	    F4SoundFXSetDist( SFX_BB_WARNING, FALSE, 0.0f, 1.0f );	}
    }
    
    mpFaultList[subsystem].elFunction	|= function;
    mpFaultList[subsystem].elSeverity	= severity;
    AddMflList(SimLibElapsedTime, subsystem, (int)severity);
}

//-------------------------------------------------	
// FaultClass::ClearFault
//-------------------------------------------------

void FaultClass::ClearFault(type_FSubSystem subsystem) {

	if(mpFaultList[subsystem].elFunction != nofault) {
		mFaultCount--;
		//MI small fixup
		if(mFaultCount < 0)
			mFaultCount = 0;
//		mpFaultList[subsystem].elFunction	= nofault;
	}
}

// JPO clear individual fault bit
void FaultClass::ClearFault(type_FSubSystem subsystem, type_FFunction	function) {
	mpFaultList[subsystem].elFunction &= ~ function;

	if(mpFaultList[subsystem].elFunction == nofault) {
		mFaultCount--;
		//MI small fixup
		if(mFaultCount < 0)
			mFaultCount = 0;
	}
}

//-------------------------------------------------
// FaultClass::GetFault
//-------------------------------------------------

void FaultClass::GetFault(type_FSubSystem subsystem, str_FEntry* entry) {
	
	entry->elFunction = mpFaultList[subsystem].elFunction;
	entry->elSeverity = mpFaultList[subsystem].elSeverity;
}

//-------------------------------------------------
// FaultClass::GetFault
//-------------------------------------------------

int FaultClass::GetFault(type_FSubSystem subsystem) {

	return mpFaultList[subsystem].elFunction;
}

//-------------------------------------------------	
// FaultClass::GetFaultNames
//-------------------------------------------------

void FaultClass::GetFaultNames(type_FSubSystem	subsystem,
                               int funcNum,
				str_FNames*		names) {

	ShiAssert(FALSE == F4IsBadReadPtr(names, sizeof *names));
	ShiAssert(FALSE == F4IsBadReadPtr(mpFaultData, sizeof *mpFaultData));
	ShiAssert(FALSE == F4IsBadReadPtr(mpFFunctionNames, sizeof *mpFFunctionNames));
	ShiAssert(FALSE == F4IsBadReadPtr(mpFSeverityNames, sizeof *mpFSeverityNames));

	names->elpFSubSystemNames	= mpFaultData[subsystem].mpFSSName;
	names->elpFFunctionNames	= mpFFunctionNames[funcNum];
	names->elpFSeverityNames	= mpFSeverityNames[mpFaultList[subsystem].elSeverity];
}

// fills in the values 
BOOL FaultClass::GetFirstFault(type_FSubSystem *subsystemp, int *functionp)
{
    for(int i = 0; i < FaultClass::NumFaultListSubSystems; i++) {
	if (mpFaultList[i].elFunction != nofault) {
	    *subsystemp = (type_FSubSystem)i;
	    return FindFirstFunction((type_FSubSystem)i, functionp); // this should be true
	}
    }
    return FALSE;
}

// takes the values, and finds the next one
BOOL FaultClass::GetNextFault(type_FSubSystem *subsystemp, int *functionp)
{
    if (FindNextFunction(*subsystemp, functionp) == TRUE)
	return TRUE;

    for(int i = (*subsystemp) + 1; i < FaultClass::NumFaultListSubSystems; i++) {
	if (mpFaultList[i].elFunction != nofault) {
	    *subsystemp = (type_FSubSystem)i;
	    return FindFirstFunction((type_FSubSystem)i, functionp);
	}
    }
    return FALSE;
}

BOOL FaultClass::FindFirstFunction(type_FSubSystem sys, int *functionp)
{
    for (int i = 0; i < NumFaultFunctions-1; i++) {
	if (mpFaultList[sys].elFunction & (1U << i)) {
	    *functionp = i+1;
	    return TRUE;
	}
    }
    return FALSE;
}

BOOL FaultClass::FindNextFunction(type_FSubSystem sys, int *functionp)
{
    for (int i = *functionp; i < NumFaultFunctions-1; i++) {
	if (mpFaultList[sys].elFunction & (1U << i)) {
	    *functionp = i+1;
	    return TRUE;
	}
    }
    return FALSE;
}

void FaultClass::TotalPowerFailure() // JPO
{
    int i, j;
    // set practically every fault known...
    for (i = 0; i < NumFaultListSubSystems; i++) {
	//if (i == ufc_fault) continue; // for debugging - so we can see whats happening
	for (j = 0; j < NumFaultFunctions; j++) {
	    switch (1<<j) {
	    case efire: // skip engine fire
	    case ldgr: // skip landing gear
		break;
	    default:
		if (mpFaultData[i].mBreakable & (1<<j)) {
		    SetFault((type_FSubSystem)i, (type_FFunction)(1<<j), fail, TRUE);
		}
	    }
	}
    }
}

void FaultClass::RandomFailure() // THW 2003-11-20 Make up some random failures, copied from Codec's code above and slightly altered
{
    int i, j;
    // Loop through every fault known...
    for (i = 0; i < NumFaultListSubSystems; i++) {
	//if (i == ufc_fault) continue; // for debugging - so we can see whats happening
		if (rand() % 100 < 5) { // 5% failure chance for each system
		{
			for (j = 0; j < NumFaultFunctions; j++) {
				//switch (1<<j) {
				//case efire: // skip engine fire
				//case ldgr: // skip landing gear
				//break;
				//default:
				if (mpFaultData[i].mBreakable & (1<<j)) {
					SetFault((type_FSubSystem)i, (type_FFunction)(1<<j), fail, TRUE);
				}
				}
			}
		}
	}
}

void FaultClass::AddMflList(VU_TIME thetime, FaultClass::type_FSubSystem type, int subtype)
{
    for (int i = 0; i < mLastMfl; i++) {
	if (mMflList[i].type == type &&
	    mMflList[i].subtype == subtype) {
	    mMflList[i].no ++;
	    return;
	}
    }
    if (mLastMfl >= MAX_MFL) return;
    mMflList[mLastMfl].time = (int)((thetime - mStartTime) * MSEC_TO_SEC); // delta from start
    mMflList[mLastMfl].type = type;
    mMflList[mLastMfl].no = 1;
    mMflList[mLastMfl].subtype = subtype;
    mLastMfl ++;
}


bool FaultClass::GetMflEntry (int n, const char **name, int *subsys, int *count, char timestr[])
{
    ShiAssert(n >=0 && n < mLastMfl);
    if (n < 0 || n >= mLastMfl)
	return false;
    ShiAssert(mMflList[n].type >= 0 && mMflList[n].type < TotalFaultStrings);
    *name = mpFaultData[mMflList[n].type].mpFSSName;
    *subsys = mMflList[n].subtype;
    *count = mMflList[n].no;
    sprintf (timestr, "%3d:%02d", mMflList[n].time / 60, mMflList[n].time % 60);
    return true;
}
