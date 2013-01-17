#ifndef _FACK_H
#define _FACK_H

#include "fault.h"
#include "caution.h"

class FackClass {
    
    int	         mMasterCaution;
    int			 NeedsWarnReset;	//MI for Warn Reset switch
	int			 DidManWarnReset;	//MI for Warn Reset switch

    
    FaultClass		mFaults;
    CautionClass	mCautions;
    
public:
	int			 NeedAckAvioncFault;    
    BOOL	IsFlagSet();
    void	ClearFlag();
    
    void	SetFault(FaultClass::type_FSubSystem, FaultClass::type_FFunction, FaultClass::type_FSeverity, BOOL);
    void	SetFault(int, BOOL); // Choose sub-system and function
    void	SetFault(type_CSubSystem);
    
    void	ClearFault(FaultClass::type_FSubSystem);
    void	ClearFault(type_CSubSystem);
    void	ClearFault(FaultClass::type_FSubSystem ss, FaultClass::type_FFunction type) { mFaults.ClearFault(ss, type); };

    void	ClearAvioncFault (void) {NeedAckAvioncFault = FALSE;};
    
    void	GetFault(FaultClass::type_FSubSystem, FaultClass::str_FEntry*);
    BOOL	GetFault(FaultClass::type_FSubSystem);
    BOOL	GetFault(type_CSubSystem);
    int		MasterCaution(void) {return mMasterCaution;};
    int         Breakable (FaultClass::type_FSubSystem id) {return mFaults.Breakable(id);};
    void        ClearMasterCaution(void) {mMasterCaution = FALSE;};
    void        SetMasterCaution (void) {mMasterCaution = TRUE; }
    void	TotalPowerFailure(); // JPO
    void	RandomFailure(); // THW 2003-11-20
    int		WarnReset(void)	{return NeedsWarnReset;};	//MI
	int		DidManWarn(void)	{return DidManWarnReset;};	//MI
	void	SetManWarnReset(void)	{DidManWarnReset = TRUE;};	//MI
	void	ClearManWarnReset(void)	{DidManWarnReset = FALSE;};	//MI
    void	ClearWarnReset(void) {NeedsWarnReset = FALSE;};
    void	SetWarnReset(void) {NeedsWarnReset = TRUE;};
    int		GetFFaultCount(void) {return mFaults.GetFaultCount();}
	//MI
	void	SetWarning(type_CSubSystem);
	void	SetCaution(type_CSubSystem);
    
    void	GetFaultNames(FaultClass::type_FSubSystem, int funcNum, FaultClass::str_FNames*);
    void	AddTakeOff(VU_TIME thetime) { mFaults.AddMflList(thetime, FaultClass::takeoff, 0); };
    void	AddLanding(VU_TIME thetime) { mFaults.AddMflList(thetime, FaultClass::landing, 0); };
    void	SetStartTime(VU_TIME thetime) { mFaults.SetStartTime(thetime); };
    bool	GetMflEntry (int n, const char **name, int *subsys, int *count, char timestr[]) {
	return mFaults.GetMflEntry(n, name, subsys, count, timestr);
    };
    int	GetMflListCount() { return mFaults.GetMflListCount(); };
    void ClearMfl() { mFaults.ClearMfl(); };
    BOOL FindFirstFunction(FaultClass::type_FSubSystem sys, int *functionp) 
    { return mFaults.FindFirstFunction(sys, functionp); };
    BOOL FindNextFunction(FaultClass::type_FSubSystem sys, int *functionp)
    { return mFaults.FindNextFunction(sys, functionp); };
    BOOL GetFirstFault(FaultClass::type_FSubSystem *subsystemp, int *functionp) 
    { return mFaults.GetFirstFault(subsystemp, functionp); };
    BOOL GetNextFault(FaultClass::type_FSubSystem *subsystemp, int *functionp)
    { return mFaults.GetNextFault(subsystemp, functionp); };

    FackClass();
    ~FackClass();
};


#endif
