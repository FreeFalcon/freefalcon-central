#define	PICKLE_RELEASE_TIME		1000		// Time in milliSeconds for Pickle to release 



class PilotInputs
{
   public:
      enum TwoWaySwitch {Off, On};
      enum FourWaySwitch {Center, Up, Down, Left, Right};
      TwoWaySwitch pickleButton;
      TwoWaySwitch pitchOverride;
      TwoWaySwitch missileStep;
      TwoWaySwitch missileUncage;
      FourWaySwitch speedBrakeCmd;
      FourWaySwitch missileOverride;
      FourWaySwitch trigger;
      FourWaySwitch tmsPos;
      FourWaySwitch trimPos;
      FourWaySwitch dmsPos;
      FourWaySwitch cmmsPos;
      FourWaySwitch cursorControl;
      FourWaySwitch micPos;
      float manRange;
      float antennaEl;
      float pstick;
      float rstick;
      float throttle;
	  float engineThrottle[2];			// Retro 12Jan2004 - I didn´t want to call it 'throttle' as we´d certainly get errors that way
      float rudder;
      float ptrim;
      float rtrim;
      float ytrim;
	  DWORD	PickleTime;
      PilotInputs (void);
      ~PilotInputs (void);
      void Update (void);
	  void Reset(void);

	  typedef enum			// Retro 12Jan2004
	  {
		Left_Engine = 0,	// do NOT change this, it´s an array index !!!
		Right_Engine = 1,	// do NOT change this, it´s an array index !!!
		Both_Engines		// do with that whatever you want :p
	  } Engine_t;

	  Engine_t currentlyActiveEngine;	// Retro 12Jan2004

	  // Retro 12Jan2004 - for explanations, see the .cpp
	  Engine_t getCurrentEngine() { return currentlyActiveEngine; }
	  void cycleCurrentEngine();
	  void selectLeftEngine() { currentlyActiveEngine = Left_Engine; }
	  void selectRightEngine() { currentlyActiveEngine = Right_Engine; };
	  void selectBothEngines() { currentlyActiveEngine = Both_Engines; };
	  // Retro 12Jan2004 end
};

extern PilotInputs UserStickInputs;

