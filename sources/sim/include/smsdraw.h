#ifndef _SMSDRAW_H
#define _SMSDRAW_H

#include "sms.h"
#include "mfd.h"

#define MAX_DIGITS 7
#define STR_LEN 12

// Forward declaration of class pointers
class SMSClass;
class MissileClass; // 2002-01-28 S,G,

class SmsDrawable : public MfdDrawable
{
public:
	enum SmsDisplayMode {Off, Inv, Wpn, SelJet, EmergJet, InputMode};
	SmsDrawable (SMSClass* self);
	~SmsDrawable(void);
	virtual void DisplayInit (ImageBuffer*);
	virtual void Display (VirtualDisplay*);
	virtual void DisplayExit(void);
	VirtualDisplay* GetDisplay (void);
	void ToggleInventory ();
	void SetDisplayMode (SmsDisplayMode newDisplayMode);
	SmsDisplayMode DisplayMode(void) {return displayMode;}
	void StepDisplayMode (void);
	void PushButton (int whichButton, int whichMFD);
	int frameCount;
	int hardPointSelected;
	//int sjHardPointSelected;
	JettisonMode sjSelected[32]; // MLR 3/9/2004 - Selective Jett data
	unsigned int flags;
	enum SmsDrawFlags { MENUMODE = 0x1, };
	int IsSet(SmsDrawFlags fl) { return (flags & fl) ? TRUE : FALSE; };
	void SetFlag(SmsDrawFlags fl) { flags |= fl; };
	void UnsetFlag(SmsDrawFlags fl) { flags &= ~fl; };
	void ToggleFlag(SmsDrawFlags fl) { flags ^= fl; };
	int IsDisplayed (void) {return isDisplayed;};
	void SetGroundSpotPos (float x, float y, float z);
	void UpdateGroundSpot(void);
	char HdptStationSym(int n);

	char sjWeaponRack[32];

	//MI
	static int flash;
	SmsDisplayMode EmergStoreMode;
	void EmergJetDisplay(void);

	//MI new input stuff
	enum InputModes {NONE, RELEASE_PULSE, RELEASE_SPACE, CONTROL_PAGE, ARMING_DELAY, BURST_ALT,
	REL_ANG, C1, C2, C3, C4, LADD_MODE}; 
	SmsDisplayMode DataInputMode;
	SmsDisplayMode lastInputMode;
	void InputDisplay(void);
	void InputRP(void);
	void InputRS(void);
	void InputPushButton(int, int);
	void FillInputString(void);
	void CheckInput(void);
	void CheckDigits(void);
	void CorrectInput(void);
	void WrongInput(void);
	void CDisplay(void);
	void ClearDigits(void);
	void ChangeProf(void);
	void RelAngDisplay(void);
	void ChangeToInput(int button);
	void CNTLPage(void);
	void LabelOSB(void);
	void ADPage(void);
	void AddInput(int whichButton);
	void InputBA(void);
	void LADDDisplay(void);
	void SetWeapParams(void);
	int AddUp(void);

	InputModes InputModus;
	int PossibleInputs, InputsMade;
	int CheckButton(int whichButton);
	int Input_Digits[MAX_DIGITS];
	int InputLine, MaxInputLines;
	bool C1Weap, C2Weap, C3Weap, C4Weap;
	static int InputFlash;
	bool Manual_Input, wrong;
	char inputstr[STR_LEN];

	void MavSMSDisplay(void);

private:
	//MissileClass* thePrevMissile;
	//int thePrevMissileIsRef;
	VuBin<MissileClass> thePrevMissile;
	int hadCamera, needCamera;
	int isDisplayed;
	SMSClass* Sms;
	VuEntity* groundSpot;
	SmsDisplayMode displayMode;
	SmsDisplayMode lastMode;
	void GunDisplay (void);
	void MissileDisplay (void);
	void MaverickDisplay (void);
	void AAMDisplay (void);
	void HarmDisplay (void);
	void GBUDisplay (void);
	void CameraDisplay (void);
	void BombDisplay (void);
	void AGMenuDisplay(void);
	void InventoryDisplay(int);
	void DogfightDisplay(void);
	void SelectiveJettison(void);
	void ShowMissiles (int buttonNum);
	void MissileOverrideDisplay (void);
	void BottomRow ();
	void TopRow (int isinv);
	void SJPushButton (int, int);
	void InvPushButton (int, int);
	void WpnPushButton (int, int);
	void InvDrawHp (int hp, int jettonly);
	void WpnAGPushButton (int, int);
	void WpnAAPushButton (int, int);
	void WpnNavPushButton (int, int);
	void WpnOtherPushButton (int, int);
	void WpnAAMissileButton(int whichButton, int whichMfd);
	void WpnAGMenu(int whichButton, int whichMfd);
	void StepSelectiveJettisonMode(int hp);
	void JDAMDisplay(void);//Cobra


	friend SMSClass;
};

#endif
