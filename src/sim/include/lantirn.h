#ifndef LANTIRN_H
#define LANTIRN_H

#include "drawable.h"

struct Tpoint;
class AircraftClass;

class LantirnClass : public DrawableClass
{
public:
    enum { FLIR_ON = 0x1, AVAILABLE = 0x2, CONH = 0x4, };
private:
    VirtualDisplay*      display;	// The renderer we are to draw upon
public:
	void GetCameraPos (Tpoint *pos);
    bool IsEnabled() { return (m_flags & AVAILABLE) ? TRUE : FALSE; };
    bool IsFLIR () { return (m_flags & FLIR_ON) ? TRUE : FALSE; };
    bool IsHonC () { return (m_flags & CONH) == 0? TRUE : FALSE; };
    void ToggleHonC () { m_flags ^= CONH; };
    void ToggleFLIR();
    void SetFovScale(float fscale) { m_fscale = fscale; };
    float GetFovScale(void) { return m_fscale; };
    float GetDPitch() { return m_dpitch; };
    void SetDPitch(float dp) { m_dpitch = dp; };
    void SetFOV(float fov);
    float GetFov();
    LantirnClass(); // constructor
    ~LantirnClass(); // desctructor
    
    VirtualDisplay*	GetDisplay (void) {return privateDisplay;};
    void			Display (VirtualDisplay*);
    void			DisplayInit (ImageBuffer*);
    void    SetTFRAlt (int n) { m_tfr_alt = n; };
    int	    GetTFRAlt (void) { return m_tfr_alt; };
    enum TfrRide { TFR_SOFT, TFR_MED, TFR_HARD };
    TfrRide GetTFRRide(void) { return m_tfr_ride; };
    void    SetTFRRide (TfrRide ride) { m_tfr_ride = ride; };
    void    StepTFRRide ();

    enum TfrMode { TFR_ECCM, TFR_LP1, TFR_WX, TFR_STBY, TFR_NORM};
    TfrMode GetTFRMode(void) { return m_tfrmode; };
    void    SetTFRMode (TfrMode mode) { m_tfrmode = mode; };
    void    StepTFRMode();
    float   GetScanLoc () { return scanpos; };
    // debug stuff
    float gdist;    // distance to ground intersection
    int evasize;    // are we being evasize
    float holdheight;	// what height we want to be at
    float turnradius;	// what turn radius we can do
    float pitch; // desired pitch angle
    float GetHoldHeight() { return holdheight; };
    float GetGLimit ();
    void Exec (AircraftClass* self);	// do whats necessary

	//new variables
	float PID_MX, PID_lastErr, PID_error, PID_Output;
	float MaxG, MinG;
	float gAlt;
	float lookingAngle;
	float gDist2;
	float gammaCorr;
	float gamma;
	float min_Radius;
	float min_safe_dist;
	bool SpeedUp;
	float roll;
	
	float GetGroundDistance(AircraftClass* self, float zOffset, float yaw, float pitch);
	float FeatureCollisionPrediction (AircraftClass* self, float zOffset, BOOL MeasureHorizontally, 
		BOOL GreatestAspect, float Clearance, float GridSizeNM, float boxScale, float *featureHeight);
	float GetEVAFactor(AircraftClass* self, int eva);
	
	float featureDistance, featureDistance2, featureDistance3;
	float featureHeight, featureHeight2, featureHeight3;
	float featureAngle, featureAngle2, featureAngle3;
private:
    float m_fscale; // delta fov
    float m_dpitch; // delta pitch
    unsigned int m_flags;
    int m_tfr_alt;
    TfrRide m_tfr_ride;
    TfrMode m_tfrmode;
    void DrawTerrain ();
    float scanpos; // -1 to 1
    float scandir;
    float scanrate;
    void MoveBeam ();
    float GetGroundIntersection(AircraftClass* self, float yaw, float pitch, float galt, int &type);
};

extern LantirnClass *theLantirn;
#endif
