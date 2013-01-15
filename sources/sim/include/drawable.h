#ifndef _DRAWABLE_H
#define _DRAWABLE_H

class VirtualDisplay;
class RViewPoint;
class ImageBuffer;

enum MfdColor {
    MFD_WHITE = 1,
	MFD_RED = 2,
	MFD_GREEN = 0,
	MFD_CYAN = 4,
	MFD_YELLOW = 3,
	MFD_MAGENTA = 5,
	MFD_BLUE = 6,
	MFD_GREY = 7,
	MFD_BRIGHT_GREEN = 8, // I-Hawk
	MFD_WHITY_GRAY = 9, // I-Hawk
	MFD_DEFAULT= MFD_GREEN,
	MFD_CURSOR = MFD_WHITE, 
	MFD_BUGGED = MFD_WHITE,
	MFD_LINES = MFD_WHITE, 
	MFD_ROUTES = MFD_WHITE,
	MFD_HOSTILE = MFD_RED,
	MFD_FRIENDLY = MFD_GREEN,
	MFD_TEAMDATA = MFD_CYAN,
	MFD_UNKNOWN = MFD_YELLOW,
	MFD_DATALINK = MFD_MAGENTA,
	MFD_SWITCHFLASH = MFD_GREY,
	MFD_SEADBOX = MFD_WHITE,
	MFD_RWRDIAMOND = MFD_GREEN,
	MFD_CARAPACETHREAT = MFD_RED,
	MFD_TFRLIMIT = MFD_YELLOW,
	MFD_IFFJAMAZIMUTH = MFD_RED,
	MFD_DLCASBOX = MFD_WHITE,
	MFD_IFFFREIENDLY = MFD_GREEN,
	MFD_IFFUNKNOWN = MFD_YELLOW,
	MFD_BREAKX = MFD_RED,
	MFD_DL_SEADTGT = MFD_MAGENTA,
	MFD_DL_TGTOUTFOV = MFD_MAGENTA,
	MFD_DL_TEAM14 = MFD_CYAN,
	MFD_DL_TEAMOUTFOV = MFD_CYAN,
	MFD_DL_TEAM58 = MFD_GREEN,
	MFD_DL_TEAMUNKTOI = MFD_YELLOW,
	MFD_DL_TEAMUNKTOIFOV = MFD_YELLOW,
	MFD_DL_AGCURSOR = MFD_MAGENTA,
	MFD_DL_MARKPOINT = MFD_MAGENTA,
	MFD_DL_PENGUIN = MFD_MAGENTA,
	MFD_DL_CARAPACE = MFD_MAGENTA,
	MFD_DL_CASIP = MFD_MAGENTA,
	MFD_DL_CASTGT = MFD_MAGENTA,
	MFD_DL_CASLINE = MFD_MAGENTA,
	MFD_STEERINGBARS = MFD_CYAN,
	MFD_DLZ = MFD_WHITE,
	MFD_TGT_RANGECUE = MFD_WHITE,
	MFD_ACTIVE_SEEKER_RANGE_CUE = MFD_WHITE,
	MFD_ATTACK_STEERING_CUE = MFD_WHITE,
	MFD_STEER_ERROR_CUE = MFD_WHITE,
	MFD_AA_MODE_RANGE_MARK = MFD_GREEN,
	MFD_FCR_UNK_TRACK = MFD_YELLOW,
	MFD_FCR_UNK_TRACK_TAIL = MFD_MAGENTA,
	MFD_FCR_UNK_TRACK_FLASH = MFD_RED,
	MFD_FCR_BUGGED = MFD_YELLOW,
	MFD_FCR_BUGGED_TAIL = MFD_MAGENTA,
	MFD_FCR_BUGGED_FLASH_TAIL = MFD_RED,
	MFD_KILL_X = MFD_RED,
	MFD_FCR_TOI_CIRCLE = MFD_YELLOW,
	MFD_FCR_TRACK_FILE_TGT = MFD_YELLOW,
	MFD_TGT_CLOSURE_RATE = MFD_WHITE,
	MFD_OFF = MFD_WHITE,
	MFD_TEXT = MFD_GREY,
	MFD_NAV_ROUTE_INACTIVE = MFD_GREY,
	MFD_NAV_ROUTE_ACTIVE = MFD_WHITE,
	MFD_STPT_INACTICE = MFD_GREY,
	MFD_STPT_ACTIVE = MFD_WHITE,
	MFD_PENGUIN_TGT = MFD_YELLOW,
	MFD_PREPLAN = MFD_YELLOW,
	MFD_PREPLAN_INRANGE= MFD_RED,
	MFD_MARKPOINT = MFD_CYAN,
	MFD_BULLSEYE_LOS = MFD_CYAN,
	MFD_LINE1 = MFD_GREY,
	MFD_LINE2 = MFD_GREY,
	MFD_LINE3 = MFD_GREY,
	MFD_LINE4 = MFD_GREY,
	MFD_FCR_REAQ_IND = MFD_CYAN,
	MFD_ANTENNA_AZEL = MFD_CYAN,
	MFD_CUR_ALT = MFD_CYAN,
	MFD_MINMAX_ALT = MFD_CYAN,
	MFD_SET_CLEARANCE_IND = MFD_GREEN,
	MFD_ANTENNA_AZEL_SCALE = MFD_CYAN,
	MFD_ALOW_SETTING = MFD_GREEN,
	MFD_ANALOG_ALT = MFD_GREEN,
	MFD_AIRSPEED = MFD_GREEN,
	MFD_AIRSPEED_BOX = MFD_GREEN,
	MFD_AIRCRAFT_HDG_BOX = MFD_GREEN,
	MFD_RADAR_ALT_BOX = MFD_GREEN,
	MFD_CUR_STPT = MFD_WHITE,
	MFD_IFF_JAM = MFD_RED,
	MFD_AIRCRAFT_REF = MFD_CYAN,
	MFD_NORTH_PTR_RINGS = MFD_BLUE,
	MFD_SOI = MFD_WHITE,
	MFD_FCR_AZIMUTH_SCAN_LIM = MFD_CYAN,
	MFD_EXPAND_BOX = MFD_CYAN,
	MFD_HORIZON_LINE = MFD_CYAN,
	MFD_TEST_PATTERN = MFD_GREEN,
	MFD_DETECTED_THREAT_BOX = MFD_GREEN,
	MFD_LAUNCH_STATUS_DIV = MFD_GREEN,
	MFD_DLNK = MFD_YELLOW,
	MFD_SWEEP = MFD_CYAN,
	MFD_BULLSEYE = MFD_CYAN,
	MFD_OWNSHIP = MFD_CYAN,
	MFD_BREAK = MFD_RED, // red
	MFD_LABELS = MFD_GREEN,
	MFD_GMSCOPE_CURSOR = MFD_WHITY_GRAY, // I-Hawk
	MFD_GMSCOPE_ARCS = MFD_WHITY_GRAY, // I-Hawk
	MFD_RADAR_WATERLINE = MFD_BLUE, // I-Hawk
};

// this needs to go somewhere FCC and MFD can share.
enum MASTERMODES { MM_AG = 0, MM_AA = 1, MM_NAV = 2, MM_MSL = 3, MM_DGFT = 4, MM_MAXMM};

class DrawableClass
{
   protected:
      enum DrawableFlags {SOI = 0x1};
      DrawableClass(void) {privateDisplay = display = NULL; viewPoint = NULL; drawFlags = 0;};
      int MFDOn;
      int drawFlags;
      unsigned int intensity;

   public:
      virtual ~DrawableClass (void)				{};

      enum DisplayTypes {ThreeDIR, ThreeDVis, ThreeDColor, MonoChrome, NumDisplayTypes};

      virtual void Display(VirtualDisplay*)		{ ShiWarning( "No Display!" ); };
      virtual void DisplayInit (ImageBuffer*)	{};
      virtual void DisplayExit (void);
      virtual VirtualDisplay* GetDisplay (void)	{return privateDisplay;};

      int IsSOI (void) {return (drawFlags & SOI ? TRUE : FALSE);};
      void SetSOI (int newVal) {if (newVal) drawFlags |= SOI; else drawFlags &= ~SOI; };

      void	SetMFD (int newMFD)		{MFDOn = newMFD;};
      int	OnMFD (void)			{return MFDOn;};

      void  SetIntensity (unsigned int val) { intensity = val; };
      unsigned int GetIntensity(void) { return intensity; };
      void LabelButton (int idx, char* str1, char* str2 = NULL, int inverse = 0);	// Last argument tells if its an INVERSE label or not...
      void DrawBorder ();
      void GetButtonPos (int bno, float *xposp, float *yposp);
      virtual void PushButton (int, int)	{};								// Override to get button messages in subclasses

      RViewPoint		*viewPoint;
      VirtualDisplay	*display;
      VirtualDisplay	*privateDisplay;
      static unsigned int MFDColors[], AltMFDColors[];
   unsigned int GetMfdColor(MfdColor);
   unsigned int GetAgedMfdColor(MfdColor col, int age);

	static BOOL greenMode;
	static void SetGreenMode(BOOL state);
	//static float lighting[3];
   static void SetLighting(float red, float green, float blue);

};
#endif
