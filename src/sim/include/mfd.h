#ifndef _MFD_H
#define _MFD_H

#ifndef _WINDOWS_
#include <windows.h>
#endif
#include "drawable.h"
#include "Graphics/Include/grtypes.h"

// Forward Class declarations

class AircraftClass;
class ImageBuffer;
class DrawableClass;
class VirtualDisplay;
class RViewPoint;
class Render3D;
class Canvas3D;
class SMSClass;
class LaserPodClass;
class RadarDopplerClass;
class FireControlComputer;
class MaverickDisplayClass;

extern bool g_bRealisticAvionics;

//MI
#define AGMaster	0
#define NAVMaster	1
#define AAMaster	2
#define MRMMaster	3
#define DGFTMaster	4

#define RightMFD AGMaster	//0
#define LeftMFD NAVMaster	//1

#define TRUE_NEXT 1
#define TRUE_ABSOLUTE 2
#define TRUE_MODESWITCH 3

class MFDClass
{
public:
	enum { MAXMM = 5};
	MFDClass (int count, Render3D *r3d);
	~MFDClass (void);

	int	id;
	char	changeMode;

	// RV - I-Hawk - Removed HUD mode, inserted HAD mode instead for HTS targeting
	enum	MfdMode {
		MfdOff = 0, MfdMenu = 1, 
		TFRMode = 2, FLIRMode = 3, 
		TestMode = 4, DTEMode = 5, FLCSMode = 6, WPNMode = 7, 
		TGPMode = 8, MaxPrivMode = 9,
		FCCMode = 9, FCRMode, SMSMode, 
			RWRMode, HADMode, /*HUDMode,*/ InputMode, MAXMODES
	};
	MfdMode	mode;
	MfdMode	newMode;
	MfdMode	restoreMode;
	MfdMode	primarySecondary[MAXMM][3]; // this ones modes
	int cursel[MAXMM]; // current selection
	int curmm;
	int newmm;
    static MfdMode initialMfdL[MAXMM][3];
	static MfdMode initialMfdR[MAXMM][3];

	MfdMode GetSP(int n) { return primarySecondary[curmm][n]; };
	MfdMode CurMode() { return mode == MfdMenu ? restoreMode : mode; };
	void StealMode (MfdMode mode);
	void SetNewMasterMode(int n); // new master mode entered.
	void SetNewModeAndPos (int n, MfdMode m);
	static char *ModeName(int n);
	static void CreateDrawables(void);
	static void DestroyDrawables(void);
	void ButtonPushed(int, int);
	void NextMode(void);
	void SetMode (MfdMode);
	void SetOwnship (AircraftClass *newOwnship);
	AircraftClass *GetOwnShip() { return ownship; };
	void SetPosition (int xPos, int yPos);
	void SetNewRenderer(Render3D *r3d);
	void SetNewMode(MfdMode); //VWF 8/12/97
	void Exec(int,int); //VWF 4/5/97
	void Exit(void);
	void UpdateVirtualPosition( const Tpoint *pos, const Trotation *rot );
	void SetImageBuffer (ImageBuffer*, float, float, float, float);
	DrawableClass *GetDrawable() const { return const_cast<DrawableClass*>(drawable); } 
	void SetDrawable(DrawableClass *newDisplay) { drawable = newDisplay; } 

	// RV - I-Hawk - The TGP warning functions
	int GetTGPWarning() { return TGPWarning; }
	void SetTGPWarning( int warning ) { TGPWarning = warning; }

	ImageBuffer*			image;
	ImageBuffer*			privateImage;
private:
	// sfr: going private (as it should be)
	DrawableClass			*drawable;
	int TGPWarning; // RV - I-Hawk - The TGP warning
public:
	Canvas3D				*virtMFD;
	static DrawableClass	*mfdDraw[MaxPrivMode];
	static DrawableClass	*HadDrawable; // RV - I-Hawk - Added for HAD mode instead of HUDMode
	int Color (void);
	unsigned int GetIntensity (void);
	void IncreaseBrightness (void);
	void DecreaseBrightness(void);

	//MI
	MfdMode EmergStoreMode;
	MfdMode GetCurMode(void)	{return mode;};

private:
	AircraftClass	*ownship;
	RViewPoint	*viewPoint;
	float			vTop, vLeft, vBottom, vRight;
	Tpoint cUL, cUR, cLL;	// ASSO:
	int tLeft, tTop, tRight, tBottom;		// ASSO:
	unsigned char cBlend;		// ASSO:
	float	cAlpha;	// ASSO:
	int color;
	int intensity;
	static char *ModeNames[];

private:
	void			FreeDrawable (void);
};

class MfdDrawable : public DrawableClass
{
   public:
      MfdDrawable () {};
      virtual ~MfdDrawable(void);
      virtual void DisplayInit (ImageBuffer*);
      virtual void Display (VirtualDisplay*vd) { display = vd; };
      virtual void PushButton (int, int);
      void BottomRow (void);
      void DefaultLabel(int button);
      void DrawReference (AircraftClass *self);
	  void DrawRedBreak(VirtualDisplay* display);
	  void TGPAttitudeWarning(VirtualDisplay* display); // RV - I-Hawk
};

class MfdMenuDrawable : public MfdDrawable
{
   public:
       MfdMenuDrawable() { mfdpage = 0; };
       virtual void Display (VirtualDisplay*);
       virtual void PushButton (int, int);
       void SetCurMode (MFDClass::MfdMode cm) { curmode = cm; };
       MFDClass::MfdMode curmode;
   private:
      int mfdpage;
};

class LantirnDrawable : public MfdDrawable
{
   public:
      LantirnDrawable () {};
      ~LantirnDrawable(void) {};
      virtual void DisplayInit (ImageBuffer*);
      virtual void Display (VirtualDisplay*);
      void PushButton (int, int);
   private:
       void DrawRadarArrows();
       void DrawAzimuthScan();
       void DrawRangeScale();
       void DrawTerrain ();
       void DrawWarnings ();
       void DrawFlightInfo ();
};

class TestMfdDrawable : public MfdDrawable
{
   public:
       TestMfdDrawable ();
      ~TestMfdDrawable(void) {};
      virtual void Display (VirtualDisplay*);
      void PushButton (int, int);
   private:
	int bitpage;
	VU_TIME timer;
	int bittest;
};

class BlankMfdDrawable : public MfdDrawable
{
   public:
      BlankMfdDrawable () {};
      ~BlankMfdDrawable(void) {};
      virtual void Display (VirtualDisplay*);
      void PushButton (int, int);
   private:
};

class DteMfdDrawable : public MfdDrawable
{
   public:
      DteMfdDrawable () {};
      ~DteMfdDrawable(void) {};
      virtual void Display (VirtualDisplay*);
      void PushButton (int, int);
   private:
};

class FlcsMfdDrawable : public MfdDrawable
{
   public:
      FlcsMfdDrawable () {};
      ~FlcsMfdDrawable(void) {};
      virtual void Display (VirtualDisplay*);
      void PushButton (int, int);
   private:
};

class FlirMfdDrawable : public MfdDrawable
{
   public:
      FlirMfdDrawable () {};
      ~FlirMfdDrawable(void) {};
      virtual void Display (VirtualDisplay*);
      void PushButton (int, int);
   private:
};

class WpnMfdDrawable : public MfdDrawable
{
   public:
      WpnMfdDrawable();
      ~WpnMfdDrawable(void) {};
      virtual void Display (VirtualDisplay*);
	  virtual void DisplayInit (ImageBuffer*);
	  VirtualDisplay* GetDisplay (void);
	  void HARMWpnMode(void);
   private:
	   AircraftClass *self;
	   FireControlComputer *pFCC;
	   MaverickDisplayClass* mavDisplay;
	   SMSClass *Sms;
	   RadarDopplerClass *theRadar;
	   char Str[50];
	   void OffMode(VirtualDisplay* display);
	   void PushButton (int, int);
	   void OSBLabels(VirtualDisplay* display);
	   void DrawRALT(VirtualDisplay* display);
	   void DrawHDPT(VirtualDisplay* display, SMSClass* Sms);
	   char HdptStationSym(int n, SMSClass* Sms);
	   void DrawDLZ(VirtualDisplay* display);
};

class TgpMfdDrawable : public MfdDrawable
{
   public:
      TgpMfdDrawable();
      virtual ~TgpMfdDrawable(void) {};
	  virtual void DisplayInit (ImageBuffer*);
      virtual void Display (VirtualDisplay*);
	  VirtualDisplay* GetDisplay (void);
   private:
	   AircraftClass *self;
	   LaserPodClass *laserPod;
	   FireControlComputer *pFCC;
	   SMSClass *Sms;
	   RadarDopplerClass *theRadar;
	   bool StbyMode, MenuMode, SP;
	   static int flash;
	   char Str[50];
	   void PushButton (int, int);
	   void OffMode(VirtualDisplay* display);
	   void DrawRALT(VirtualDisplay* display);
	   void DrawHDPT(VirtualDisplay* display, SMSClass* Sms);
	   char HdptStationSym(int n, SMSClass* Sms);
	   void LaserIndicator(VirtualDisplay* display);
	   void ImpactTime(VirtualDisplay* display);
	   void DrawMasterArm(VirtualDisplay* display);
	   void DrawRange(VirtualDisplay* display);
	   void CoolingDisplay(VirtualDisplay* display);
	   void OSBLabels(VirtualDisplay* display);
};

// RV - I-Hawk - Added a display class for HAD MFD mode
class HadMfdDrawable : public MfdDrawable
{
   public:
      HadMfdDrawable();
      ~HadMfdDrawable(void) {};
      virtual void Display (VirtualDisplay*);
	  void HARMWpnMode(void);
   private:
	   AircraftClass *self;
	   FireControlComputer *pFCC;
	   SMSClass *Sms;
	   RadarDopplerClass *theRadar;
	   char Str[50];
	   int flash;
	   void OffMode(VirtualDisplay* display);
	   void PushButton (int, int);
	   void DrawRALT(VirtualDisplay* display);
	   void DrawHDPT(VirtualDisplay* display, SMSClass* Sms);
	   char HdptStationSym(int n, SMSClass* Sms);
	   void DrawDLZ(VirtualDisplay* display);
};

#define NUM_MFDS      4
#define MFD_SIZE      154

extern MFDClass* MfdDisplay[NUM_MFDS];
void MFDSwapDisplays (void);

#endif
