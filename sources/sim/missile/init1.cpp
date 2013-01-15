#include "stdhdr.h"
#include "missile.h"

void MissileClass::Init1(void)
{
	float cl1, cl2;            /* used to calculate lift curve slope */
	float ubody, vbody, wbody; /* Initial velocities in Body Axes */
	//float cx, cz;
	float cx = 0, cz = 0; // JB 010318
	int i=0, j=0;
	mlTrig trigAlpha, trigBeta;
	float tmpAlpha;

	/*---------------------------------------*/
	/* location of missile in inertial space */
	/*---------------------------------------*/
	SetLaunchData();
	tmpAlpha = alpha;
	alt = -z; 

	/*-------------------------------------*/
	/* initial missile and propellant mass */
	/*-------------------------------------*/
	mass  = weight/GRAVITY;
	if (!F4IsBadReadPtr(inputData, sizeof(MissileInputData))){ // JB 010304 CTD
		m0    = inputData->wm0/GRAVITY;
		mp0   = inputData->wp0/GRAVITY;
	}
	else {
		m0 = 0; mp0 = 0; 
	} // JB 010304 CTD

	mprop = wprop/GRAVITY;

	/*------------------------------*/
	/* lift curve slope calculation */
	/*------------------------------*/
	Atmosphere();
	alpha = 0.0F;
	Trigenometry();

	if (aeroData && !F4IsBadReadPtr(aeroData, sizeof(MissileAeroData))){ // JB 010318 CTD
		cx = Math.TwodInterp (mach, alphat, aeroData->mach, 
											aeroData->alpha, aeroData->cx, 
											aeroData->numMach, aeroData->numAlpha, &i, &j);

		cz = Math.TwodInterp (mach, alphat, aeroData->mach, 
											aeroData->alpha, aeroData->cz, 
											aeroData->numMach, aeroData->numAlpha, &i, &j);
	}

	if (!ifd){
		return; // JB 010803
	}

	cl1 = -cz*ifd->geomData.cosalp + cx*ifd->geomData.sinalp;

	alpha = 1.0F;
	Trigenometry();

	if (aeroData && !F4IsBadReadPtr(aeroData, sizeof(MissileAeroData))) // JB 010318 CTD
	{
		cx = Math.TwodInterp (mach, alphat, aeroData->mach, 
											aeroData->alpha, aeroData->cx, 
											aeroData->numMach, aeroData->numAlpha, &i, &j);

		cz = Math.TwodInterp (mach, alphat, aeroData->mach, 
											aeroData->alpha, aeroData->cz, 
											aeroData->numMach, aeroData->numAlpha, &i, &j);
	}

	cl2 = -cz*ifd->geomData.cosalp + cx*ifd->geomData.sinalp;

	/*-------------*/
	/* derivatives */
	/*-------------*/
	ifd->clalph = cl2 - cl1;
	ifd->cybeta = -ifd->clalph;
	        
	/*----------------------------*/
	/* missile velocity at launch */
	/*----------------------------*/
	alpha = tmpAlpha;
	mlSinCos (&trigAlpha, alpha*DTR);
	mlSinCos (&trigBeta,  beta*DTR);
	ubody = vt*trigAlpha.cos* trigBeta.cos + q*initZLoc -
				r*initYLoc;

	vbody = vt*trigBeta.sin + r*initXLoc - p*initZLoc;

	wbody = vt*trigAlpha.sin*trigBeta.cos + p*initYLoc -
				q*initXLoc;

	vt    = (ubody*ubody  + wbody*wbody);
	alpha = (float)atan2(wbody, ubody)   * RTD;
	beta  = (float)atan2(vbody, sqrt(vt))* RTD;
	vt    = (float)sqrt(vbody*vbody + vt);

	Trigenometry();

	/*-----------------*/
	/* rotation matrix */
	/*-----------------*/
	dmx[0][0] =  ifd->geomData.cospsi*ifd->geomData.costhe;
	dmx[0][1] =  ifd->geomData.sinpsi*ifd->geomData.costhe;
	dmx[0][2] = -ifd->geomData.sinthe;

	dmx[1][0] = -ifd->geomData.sinpsi*ifd->geomData.cosphi + ifd->geomData.cospsi*ifd->geomData.sinthe*ifd->geomData.sinphi;
	dmx[1][1] =  ifd->geomData.cospsi*ifd->geomData.cosphi + ifd->geomData.sinpsi*ifd->geomData.sinthe*ifd->geomData.sinphi;
	dmx[1][2] =  ifd->geomData.costhe*ifd->geomData.sinphi;

	dmx[2][0] =  ifd->geomData.sinpsi*ifd->geomData.sinphi + ifd->geomData.cospsi*ifd->geomData.sinthe*ifd->geomData.cosphi;
	dmx[2][1] = -ifd->geomData.cospsi*ifd->geomData.sinphi + ifd->geomData.sinpsi*ifd->geomData.sinthe*ifd->geomData.cosphi;
	dmx[2][2] =  ifd->geomData.costhe*ifd->geomData.cosphi;

	/*-------------------------*/
	/* missile velocity vector */
	/*-------------------------*/
	xdot =  vt*ifd->geomData.cosgam*ifd->geomData.cossig;
	ydot =  vt*ifd->geomData.cosgam*ifd->geomData.sinsig;
	zdot = -vt*ifd->geomData.singam;

	ifd->oldx[0] = x;
	ifd->oldx[1] = x;
	ifd->oldx[2] = xdot; //0.0; // MLR 1/5/2005 - 
	ifd->oldx[3] = xdot; //0.0; // MLR 1/5/2005 - 

	ifd->oldy[0] = y;
	ifd->oldy[1] = y;
	ifd->oldy[2] = ydot; //0.0; // MLR 1/5/2005 - 
	ifd->oldy[3] = ydot; //0.0; // MLR 1/5/2005 - 

	ifd->oldz[0] = z;
	ifd->oldz[1] = z;
	ifd->oldz[2] = zdot; //0.0; // MLR 1/5/2005 - 
	ifd->oldz[3] = zdot; //0.0; // MLR 1/5/2005 - 

	ifd->burnIndex = 0;
	launchState = PreLaunch;
	runTime = 0.0F;
	guidencephase =0;
	GuidenceTime = 0.0f;
	ifd->oldalp = 0;
	ifd->oldalpdt = 0;
	ifd->oldbet = 0;
	ifd->oldbetdt = 0;
	ifd->lastmach = 0;
	ifd->lastalpha = 0;
	/*-------------------*/
	/* aero coefficients */
	/*-------------------*/
	Aerodynamics();

	/*---------------------------*/
	/* initialize engine impulse */
	/*---------------------------*/
	ifd->oldimp[0] = 0.0;
	ifd->oldimp[1] = 0.0;
	ifd->oldimp[2] = 0.0;
	ifd->oldimp[3] = 0.0;

	/*---------------------------------------------------*/
	/* delta velocity and displacement along launch rail */
	/*---------------------------------------------------*/
	ifd->olddx[0] = 0.0;
	ifd->olddx[1] = 0.0;
	ifd->olddx[2] = 0.0;
	ifd->olddx[3] = 0.0;

	ifd->olddu[0] = 0.0;
	ifd->olddu[1] = 0.0;
	ifd->olddu[2] = 0.0;
	ifd->olddu[3] = 0.0;

	SetDelta(xdot, ydot, zdot);
	SetYPR(psi, theta, phi);
}
