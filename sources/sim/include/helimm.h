/*
** Name: HELIMM.H
** Description:
**		Important structs, typedefs and defines for the helicopter
**		Math Model.
*/

#ifndef _HELIMM_H_
#define _HELIMM_H_

#include "simmath.h"
#include "geometry.h"

/*
** DEFINES
*/
#define 	STD_DENSITY_0	.0023769	// air density at sea level (STD)

#ifndef		PI
#define 	PI 				3.14159265358979323846
#endif

// for SIMPLE model
#define MAX_HELI_PITCH	( DTR * 15.0f )
#define MAX_HELI_ROLL	( DTR * 80.0f )
#define MAX_HELI_YAWRATE	( DTR * 120.0f )
#define MAX_HELI_ROLLRATE	( DTR * 70.0f )
#define MAX_HELI_CLIMBRATE	( 120.0f )
#define MAX_HELI_FPS	( 180.0f * KNOTS_TO_FTPSEC )

/*
** STRUCTURES
*/

// 3d floating point vector
typedef struct _DVECT3D
{
	union
	{
		float x;
		float X;
	};
	union
	{
		float y;
		float Y;
	};
	union
	{
		float z;
		float Z;
	};
} DVECT3D;

typedef struct _DVECT2D
{
	union
	{
		float x;
		float X;
	};
	union
	{
		float y;
		float Y;
	};
} DVECT2D;

// 6 degrees of freedom struct where in general:
//		x,y,z represent linear forces, velocities, etc acting along the
//		x (roll), y(pitch) and z(yaw) axi.
//
//		aX, aY and aZ are angular
//
//		a1 = lateral (pitch) axis tip plane path angular (rads)
//		b1 = longitude(roll) axis tip plane path angular
typedef struct _D6DOF
{
	union
	{
		float x;
		float X;
	};
	union
	{
		float y;
		float Y;
	};
	union
	{
		float z;
		float Z;
	};
	union
	{
		float ax;
		float aX;
	};
	union
	{
		float ay;
		float aY;
	};
	union
	{
		float az;
		float aZ;
	};

	float a1;
	float b1;

} D6DOF;

// Fuselage Data
typedef struct _FUSELAGE_DATA
{
	float fs_cg;		// center of grav - inches from fuselage station 0
	float wl_cg;		// center of grav - inches from water line 0
	float weight;		// weight of copter in lbs
	DVECT3D mi;			// moments of inertia around x,y,z axis
	float mi_xz;		// combined moment of intertia for xz (?)
	float fs_cp;		// center of pressure - inches from fuselage station 0
	float wl_cp;		// center of pressure - inches from water line 0
	DVECT3D fe;			// effective frontal areas in sq ft used for drag
} FUSELAGE_DATA;

// Main Rotor Data
typedef struct _MAINROTOR_DATA
{
	float fs_hub;		// hub location - inches from fs 0
	float wl_hub;		// hub location - inches from wl 0
	float hub_is;		// forward tilt of shaft wrt fuselage in rads
	float h_offset;	// flapping hinge offset in ft
	float b_mi;		// moment of inertia of single flapping blade
	float radius;		// radius in ft
	float lslope;		// lift slope cL per unit radian aoa (usually ~ 6.0)
	float rpm;			// revspeed freq
	float cD;			// profile drag coeef of blade (usually ~ 0.010)
	float nb;			// number of blades
	float chord;		// blade chord
	float twist;		// effective blade root->tip twist in rads
	float k1;			// coeff -- pitch-flap coupling (?)
} MAINROTOR_DATA;

// Tail Rotor Data
typedef struct _TAILROTOR_DATA
{
	float fs_hub;		// hub location - inches from fs 0
	float wl_hub;		// hub location - inches from wl 0
	float radius;		// radius in ft
	float lslope;		// lift slope cL per unit radian aoa (usually ~ 6.0)
	float rpm;			// revspeed freq
	float twist;		// effective blade root->tip twist in rads
	float solidity;	// solidity of tail rotor
	float nb;			// number of blades
	float chord;		// blade chord
} TAILROTOR_DATA;

// Wing and or horizontal stabilizer Data
typedef struct _HWING_DATA
{
	float fs;		// inches from fuselage station 0
	float wl;		// inches from water line 0
	float zuu;		// effective lift per unit pressure in ft sq
	float zuw;		// effective variation in circulation lift in ft sq
	float zmax;	// maximum lift value in ft sq at stall
	float span;	// for wing only -- the span in ft
} HWING_DATA;

// Verticle (wing) stabilizer data
typedef struct _VWING_DATA
{
	float fs;		// inches from fuselage station 0
	float wl;		// inches from water line 0
	float yuu;		// effective lift per unit pressure in ft sq
	float yuv;		// effective variation in circulation lift in ft sq
	float ymax;	// maximum lift value in ft sq at stall
} VWING_DATA;

// trim data for centered positions of controls and max throw values
typedef struct _TRIM_DATA
{
	float cyc_roll_center;
	float cyc_roll_max;
	float cyc_pitch_center;
	float cyc_pitch_max;
	float coll_pitch_center;
	float coll_pitch_max;
	float tr_pitch_center;
	float tr_pitch_max;
} TRIM_DATA;

// This is the structure which holds a
// helicopters input model data comprised of the
// above structures
enum HelicopterTypeEnum {A109 = 0, COBRA, MD500, STABLE, SIMPLE, NUM_MODELS};
typedef struct _HELI_MODEL_DATA
{
	int				type;
	int				has_wing;	// y/n if has a wing or not
	float			roll_damp;
	FUSELAGE_DATA	fus;
	MAINROTOR_DATA	mr;
	TAILROTOR_DATA	tr;
	HWING_DATA		wn;
	HWING_DATA		ht;
	VWING_DATA		vt;
	TRIM_DATA		td;
} HELI_MODEL_DATA;


// these are the flight state vars grouped into 1 structure for
// the helicopter math model
class HeliMMClass
{
	public:

	// variables
	HELI_MODEL_DATA *md;	// helicopter data for model basis
	float		dT;			// delta time
	float		mass;		// mass = weight/grav
	float		omega_mr;	// main rotor angular vel
	float		omega_tr;	// tail rotor angular vel
	float		vtip;		// mr tip speed
	float		fr_mr;		// effective main rotor frontal area
	float		fr_tr;		// effective tail rotor frontal area
	float		hp_loss;	// horsepower loss
	float		vtrans;		// translational velocity for wake effect
	float		rh0;		// air density
	float		rh02;		// air density over 2
	float		gam_om_16;	// ????
	float		kC;			// ???? flapping aero couple
	float		itb2_om;	// ???? flapping x-couple coeff
	float		itb;		// ???? flapping primary response
	float		dl_db1;		// ????	primary flapping stiffness
	float		dl_da1;		// ????	cross flapping stiffness
	float		cT;			// thrust coeff
	float		a_sigma;	// a * sigma ????
	float		db1dv;		// ???? tpp dihedral effect
	float		da1du;		// ???? tpp pitchup with speed
	float		vta;		// total relative airspeed
	DVECT2D		ma_hub;		// moment arm
	DVECT2D		ma_fus;		// moment arm
	DVECT2D		ma_wn;		// moment arm
	DVECT2D		ma_ht;		// moment arm
	DVECT2D		ma_vt;		// moment arm
	DVECT2D		ma_tr;		// moment arm
	DVECT3D		eucos;		// Euler cosines
	DVECT3D		eusin;		// Euler sines
	D6DOF		AB;			// body accelerations
	D6DOF		ABprev;		// previous values of above
	D6DOF		F;			// forces and moments
	D6DOF		VA;			// velocity rel to airmass
	D6DOF		VB;			// velocity inertial
	D6DOF		VE;			// velocity relative to earth
	D6DOF		VEprev;		// velocity relative to earth -- previous value
	D6DOF		VG;			// velocity gust
	D6DOF		XE;			// positions and euler angles of craft
	DVECT2D		GR;			// tpp angular rates
	DVECT2D		GV;			// tpp angular rates
	float		coll_pitch;	// collective setting in rads (at root)
	float		cyc_pitch;	// cyclic pitch in rads
	float		cyc_roll;	// cyclic roll in rads
	float		tr_pitch;	// tail rotor pitch
	float		a_sum;		// tpp temp?
	float		b_sum;		// tpp temp?
	float		wr;			// Z axis air velocity relative to rotor plane
	float		wb;			// Z axis air velocity relative to rotor blade
	float		thrust_mr;	// main rotor thrust
	float		vi_mr;		// main rotor induced velocity;
	float		wa_fus;		// downwash on fuselage
	float		wa_fus_pos;	// position of downwash on fuselage
	float		va_x_sq;	// square of VA.x
	float		va_y_sq;	// square of VA.y
	D6DOF		fus6d;		// fuselage forces and moments
	D6DOF		mr6d;		// main rotor forces and moments
	D6DOF		tr6d;		// main rotor forces and moments
	D6DOF		ht6d;		// main rotor forces and moments
	D6DOF		vt6d;		// main rotor forces and moments
	D6DOF		wn6d;		// main rotor forces and moments
	float		p_ind;		// induced power
	float		p_climb;	// climb power
	float		p_par;		// parasite power
	float		p_prof;		// profile power
	float		p_tot;		// total power
	float		p_mr;		// main rotor power
	float		p_tr;		// tail rotor power
	float		torque_mr;	// main rotor torque
	float		vr_tr	;	// air vel relative to tail rotor disk
	float		vb_tr	;	// air vel relative to tail rotor blade
	float		thrust_tr;	// thrust tail rotor
	float		vi_tr;		// induced air tail rotor
	float		dw_ht_pos;	// downwash on horizontal tail pos
	float		wa_ht;		// airflow on ht
	float		eps_ht;		// downwash factor on ht
	float		vta_ht;		// total airspeed at ht
	float		va_vt;		// airflow on vt
	float		vta_vt;		// total airspeed at vt
	DVECT3D		grav;		// gravity forces on xyz copter axi
	float		alpha;		// aoa in degs
	float 		beta;		// slip angle in degs
	SimBaseClass *platform; // pointer to owning craft
	float		GetKias;		// knots, indicated air speed
	float		gmma;
	float		mu;
	float		sigma;
    SAVE_ARRAY  olde1, olde2, olde3, olde4;
    SAVE_ARRAY  oldGRx, oldGRy, oldABax, oldABay, oldABaz;
    float 		e1, e2, e3, e4;
	float 		ctlroll;
	float 		ctlpitch;
	float 		ctlcpitch;
	float 		ctltpitch;
	float 		mr_tmp1;
	float 		mr_tmp2;
	BOOL		isDigital;

	// member functions
	HeliMMClass( SimBaseClass *self, int helitype );
	~HeliMMClass( void );
	void	Setup( void );
	void	TipPlanePath( void );
	void	MainRotor( void );
	void	Fuselage( void );
	void	TailRotor( void );
	void	HorizTail( void );
	void	VertTail( void );
	void	Wing( void );
	void	ForceCalc( void );
	void	ResetForceVars( void );
	void	PreCalc( void );
	void	Exec( void );
	void	Init( float x, float y, float z );
	void	SetPlatformData( void );
	void	InitQuat( void );
	void	SimpleModel( void );
	void	CalcBodyOrientation( void );
	void	SetControls( float pstick, float rstick, float throttle, float pedals );
};

#endif _HELI_MM_
