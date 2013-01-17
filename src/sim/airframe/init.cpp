/******************************************************************************/
/*                                                                            */
/*  Unit Name : init.cpp                                                      */
/*                                                                            */
/*  Abstract  : Initialize global variables to the appropriate values.        */
/*                                                                            */
/*  Naming Conventions :                                                      */
/*                                                                            */
/*      Public Functions    : Mixed case with no underscores                  */
/*      Private Functions   : Mixed case with no underscores                  */
/*      Public Functions    : Mixed case with no underscores                  */
/*      Global Variables    : Mixed case with no underscores                  */
/*      Classless Functions : Mixed case with no underscores                  */
/*      Classes             : All upper case seperated by an underscore       */
/*      Defined Constants   : All upper case seperated by an underscore       */
/*      Macros              : All upper case seperated by an underscore       */
/*      Structs/Types       : All upper case seperated by an underscore       */
/*      Private Variables   : All lower case seperated by an underscore       */
/*      Public Variables    : All lower case seperated by an underscore       */
/*      Local Variables     : All lower case seperated by an underscore       */
/*                                                                            */
/*  Development History :                                                     */
/*  Date      Programer           Description                                 */
/*----------------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                               */
/*                                                                            */
/******************************************************************************/
#include "stdhdr.h"
#include "airframe.h"
#include "aircrft.h"
#include "Graphics/Include/tmap.h"
#include "Graphics/Include/rviewpnt.h"  // to get ground type
#include "otwdrive.h"


/********************************************************************/
/*                                                                  */
/* Routine: void AirframeClass::Initialize(void)                   */
/*                                                                  */
/* Description:                                                     */
/*    Initialize global variables.                                  */
/*                                                                  */
/* Inputs:                                                          */
/*    None                                                          */
/*                                                                  */
/* Outputs:                                                         */
/*    None                                                          */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
void AirframeClass::Initialize()
{
	float e10, e20, e30, e40 ,qptchc;
	mlTrig trigMu, trigSig, trigGam;

	Accelerometers();

	/*--------------------------------------------------*/
	/* initial angles for 1-g straight and level flight */
	/*--------------------------------------------------*/
	beta  = 0.0f;
	alpha = 0.0f;
	Trigenometry();

	xdot  =  vt*platform->platformAngles.cosgam *
		platform->platformAngles.cossig;
	ydot  =  vt*platform->platformAngles.cosgam *
		platform->platformAngles.sinsig;
	zdot  = -vt*platform->platformAngles.singam ;

	ShiAssert(!_isnan(xdot));
	ShiAssert(!_isnan(ydot));
	ShiAssert(!_isnan(zdot));
	/*------------------------*/
	/* initialize quaternions */
	/*------------------------*/
	  
	mlSinCos (&trigGam, gmma	* 0.5F);
	mlSinCos (&trigSig, sigma * 0.5F);
	mlSinCos (&trigMu,  mu	* 0.5F);
	e10 = trigSig.cos*trigGam.cos*trigMu.cos +
			trigSig.sin*trigGam.sin*trigMu.sin;

	e20 = trigSig.sin*trigGam.cos*trigMu.cos -
			trigSig.cos*trigGam.sin*trigMu.sin;

	e30 = trigSig.cos*trigGam.sin*trigMu.cos +
			trigSig.sin*trigGam.cos*trigMu.sin;

	e40 = trigSig.cos*trigGam.cos*trigMu.sin -
			trigSig.sin*trigGam.sin*trigMu.cos;

	e1 = e10;
	e2 = e20;
	e3 = e30;
	e4 = e40;

	/*------------------------------------*/
	/* initial earth coordinate positions */
	/*------------------------------------*/
	if(!IsSet(InAir))
	{
		Tpoint normal;
		float groundZ = OTWDriver.GetGroundLevel(x, y, &normal);
		if (z > groundZ - CheckHeight() + 0.01F)
			z = groundZ - CheckHeight();
	}
	  
	/*-------------------------*/
	/* pitch command path lags */
	/*-------------------------*/
	oldp01[0] = 0.0;
	oldp01[1] = 0.0;
	oldp01[2] = 0.0;
	oldp01[3] = 0.0;

	/*------------------------*/
	/* pitch integral pathway */
	/*------------------------*/
	oldp02[0] = alpha;
	oldp02[1] = alpha;
	oldp02[2] = 0.0;
	oldp02[3] = 0.0;

	/*------------------*/
	/* aoa filter model */
	/*------------------*/
	oldp03[0] = alpha;
	oldp03[1] = alpha;
	oldp03[2] = alpha;
	oldp03[3] = alpha;
	oldp03[4] = alpha;
	oldp03[5] = alpha;

	/*-----------------*/
	/* alpha dot model */
	/*-----------------*/
	oldp04[0] = 0.0;
	oldp04[1] = 0.0;
	oldp04[2] = 0.0;
	oldp04[3] = alpha;
	oldp04[4] = alpha;
	oldp04[5] = alpha;

	/*------------------------------*/
	/* pitch rate correction filter */
	/*------------------------------*/
	if (vt)
	{
		//qptchc = GRAVITY*nzcgs/(vt*platform->platformAngles.cosbet);
		qptchc = GRAVITY*nzcgs/vt;
		oldp05[0] = qptchc;
		oldp05[1] = qptchc;
		oldp05[2] = qptchc;
		oldp05[3] = qptchc;
	}
	else
	{
		oldp05[0] = 0.0;
		oldp05[1] = 0.0;
		oldp05[2] = 0.0;
		oldp05[3] = 0.0;
	}

	/*--------------------------------------------*/
	/* initialize roll axis flight control system */
	/*--------------------------------------------*/
	/* roll rate response model */
	/*--------------------------*/
	oldr01[0] = 0.0;
	oldr01[1] = 0.0;
	oldr01[2] = 0.0;
	oldr01[3] = 0.0;

	/*------------------------------------------*/
	/* intialize yaw axis flight control system */
	/*------------------------------------------*/
	/* yaw integral pathway */
	/*----------------------*/
	oldy01[0] = 0.0;
	oldy01[1] = 0.0;
	oldy01[2] = 0.0;
	oldy01[3] = 0.0;

	/*-------------------*/
	/* beta filter model */
	/*-------------------*/
	//   oldy02[0] = 0.0;
	//oldy02[1] = 0.0;
	//oldy02[2] = 0.0;
	//oldy02[3] = 0.0;

	/*----------------*/
	/* beta dot model */
	/*----------------*/
	oldy03[0] = 0.0;
	oldy03[1] = 0.0;
	oldy03[2] = 0.0;
	oldy03[3] = 0.0;

	//oldy04[0] = 0.0;
	//oldy04[1] = 0.0;
	//oldy04[2] = 0.0;
	//oldy04[3] = 0.0;

	/*--------------------------------*/
	/* intialize axial control system */
	/*--------------------------------*/
	olda01[0] = thrtab;
	olda01[1] = thrtab;
	olda01[2] = thrtab;
	olda01[3] = thrtab;
	athrev = 0.0F;

	//TJL 01/14/03 multi-engine
	olda012[0] = thrtab2;
	olda012[1] = thrtab2;
	olda012[2] = thrtab2;
	olda012[3] = thrtab2;
	athrev2 = 0.0F;

	//TJL 03/14/04 Turb
	oldTurb1[0] = 0.0;
	oldTurb1[1] = 0.0;
	oldTurb1[2] = 0.0;
	oldTurb1[3] = 0.0;

	//TJL 03/23/04 Turb
	oldRoll1[0] = 0.0;
	oldRoll1[1] = 0.0;
	oldRoll1[2] = 0.0;
	oldRoll1[3] = 0.0;

	if(IsSet(InAir)) { // JPO - if in the air only.
	       
		oldRpm[0] = 0.75F;
		oldRpm[1] = 0.75F;
		oldRpm[2] = 0.75F;
		oldRpm[3] = 0.75F;
		rpm = 0.75;

		//TJL 01/14/03 Multi-engine
		oldRpm2[0] = 0.75F;
		oldRpm2[1] = 0.75F;
		oldRpm2[2] = 0.75F;
		oldRpm2[3] = 0.75F;
		rpm2 = 0.75;



	}
	else
	{
		rpm = 0.0f;
		rpm2 = 0.0f;//TJL 01/14/03 multi-engine
	}


	nzcgb = 1.0F;
}

void AirframeClass::InitializeEOM(void)
{
	float mag;

   // Initialize EOM
   Atmosphere();
   FlightControlSystem();
   Aerodynamics();
   EngineModel(SimLibMinorFrameTime);
   Accelerometers();

	groundZ = OTWDriver.GetGroundLevel(x, y, &gndNormal);
	mag = (float)sqrt(gndNormal.x*gndNormal.x + gndNormal.y*gndNormal.y + gndNormal.z*gndNormal.z);
	gndNormal.x /= mag;
	gndNormal.y /= mag;
	gndNormal.z /= mag;

   EquationsOfMotion(SimLibMinorFrameTime);
}
   
void AirframeClass::ReInitialize()
{
	// float e10, e20, e30, e40;
	float qptchc;
	//mlTrig trigMu, trigSig, trigGam;

	/*--------------------------------------------------*/
	/* initial angles for 1-g straight and level flight */
	/*--------------------------------------------------*/
	beta  = 0.0f;
	ResetOrientation();
	Trigenometry();

	xdot  =  vt*platform->platformAngles.cosgam *
		platform->platformAngles.cossig;
	ydot  =  vt*platform->platformAngles.cosgam *
		platform->platformAngles.sinsig;
	zdot  = -vt*platform->platformAngles.singam ;

	ShiAssert(!_isnan(xdot));
	ShiAssert(!_isnan(ydot));
	ShiAssert(!_isnan(zdot));
	   
	/*------------------------------------*/
	/* initial earth coordinate positions */
	/*------------------------------------*/
	if(!IsSet(InAir))
	{
		Tpoint normal;
		float groundZ = OTWDriver.GetGroundLevel(x, y, &normal);
		if (z > groundZ - CheckHeight() + 0.01F)
			z = groundZ - CheckHeight();
	}
  
	/*-------------------------*/
	/* pitch command path lags */
	/*-------------------------*/
	oldp01[0] = 0.0;
	oldp01[1] = 0.0;
	oldp01[2] = 0.0;
	oldp01[3] = 0.0;

	/*------------------------*/
	/* pitch integral pathway */
	/*------------------------*/
	oldp02[0] = alpha;
	oldp02[1] = alpha;
	oldp02[2] = 0.0;
	oldp02[3] = 0.0;

	/*------------------*/
	/* aoa filter model */
	/*------------------*/
	oldp03[0] = alpha;
	oldp03[1] = alpha;
	oldp03[2] = alpha;
	oldp03[3] = alpha;
	oldp03[4] = alpha;
	oldp03[5] = alpha;

	/*-----------------*/
	/* alpha dot model */
	/*-----------------*/
	oldp04[0] = 0.0;
	oldp04[1] = 0.0;
	oldp04[2] = 0.0;
	oldp04[3] = alpha;
	oldp04[4] = alpha;
	oldp04[5] = alpha;

	/*------------------------------*/
	/* pitch rate correction filter */
	/*------------------------------*/
	if (vt)
	{
		//qptchc = GRAVITY*nzcgs/(vt*platform->platformAngles.cosbet);
		qptchc = GRAVITY*nzcgs/vt;
		oldp05[0] = qptchc;
		oldp05[1] = qptchc;
		oldp05[2] = qptchc;
		oldp05[3] = qptchc;
	}
	else
	{
		oldp05[0] = 0.0;
		oldp05[1] = 0.0;
		oldp05[2] = 0.0;
		oldp05[3] = 0.0;
	}

	/*--------------------------------------------*/
	/* initialize roll axis flight control system */
	/*--------------------------------------------*/
	/* roll rate response model */
	/*--------------------------*/
	oldr01[0] = 0.0;
	oldr01[1] = 0.0;
	oldr01[2] = 0.0;
	oldr01[3] = 0.0;

	/*------------------------------------------*/
	/* intialize yaw axis flight control system */
	/*------------------------------------------*/
	/* yaw integral pathway */
	/*----------------------*/
	oldy01[0] = 0.0;
	oldy01[1] = 0.0;
	oldy01[2] = 0.0;
	oldy01[3] = 0.0;

	/*-------------------*/
	/* beta filter model */
	/*-------------------*/
	//oldy02[0] = 0.0;
	//oldy02[1] = 0.0;
	//oldy02[2] = 0.0;
	//oldy02[3] = 0.0;

	/*----------------*/
	/* beta dot model */
	/*----------------*/
	oldy03[0] = 0.0;
	oldy03[1] = 0.0;
	oldy03[2] = 0.0;
	oldy03[3] = 0.0;

	//oldy04[0] = 0.0;
	//oldy04[1] = 0.0;
	//oldy04[2] = 0.0;
	//oldy04[3] = 0.0;

	Aerodynamics();
	Gains();
}
