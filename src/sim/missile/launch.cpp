#include "stdhdr.h"
#include "missile.h"
#include "otwdrive.h"
#include "sfx.h"
#include "acmi\src\include\acmirec.h"
#include "simveh.h"
#include "team.h"
#include "Graphics/Include/drawparticlesys.h"

extern ACMIMissilePositionRecord misPos;

void MissileClass::SetLaunchPosition (float nx, float ny, float nz) /* {initXLoc = nx; initYLoc = ny; initZLoc = nz;}; */
{
    SimBaseClass *ptr = (SimBaseClass*)parent.get();

    x = parent->XPos() + ptr->dmx[0][0]*nx + ptr->dmx[1][0]*ny + ptr->dmx[2][0]*nz;
    y = parent->YPos() + ptr->dmx[0][1]*nx + ptr->dmx[1][1]*ny + ptr->dmx[2][1]*nz;
    z = parent->ZPos() + ptr->dmx[0][2]*nx + ptr->dmx[1][2]*ny + ptr->dmx[2][2]*nz;
	SetPosition(x,y,z);
}


void MissileClass::Launch(void)
{
float ubody, vbody, wbody; /* Body axis forces */
float xbreak;              /* x force needed to begin moving on rail */
float xforce, deltu, deltx;
mlTrig trigAlpha, trigBeta;


   /*--------------------------------*/
   /* pass aircraft state to missile */ 
   /*--------------------------------*/
   SetLaunchData();

	if (!ifd)
		return; // JB 010803

	//MI CTD?
	ShiAssert(FALSE==F4IsBadReadPtr(ifd, sizeof *ifd));

   /*--------------------------------*/
   /* missile velocity during launch */
   /*--------------------------------*/
   mlSinCos (&trigAlpha, alpha*DTR);
   mlSinCos (&trigBeta,  beta*DTR);

   ubody = vt*trigAlpha.cos * trigBeta.cos +
           q*initZLoc - r*initYLoc + (float)ifd->olddu[1];

   vbody = vt*trigBeta.sin + r*initXLoc -
           p*initZLoc;

   wbody = vt*trigAlpha.sin * trigBeta.cos +
           p*initYLoc - q*initXLoc;

   vt    = ubody*ubody + wbody*wbody;

   alpha = (float)atan2(wbody, ubody)*RTD;

   if (vt > 1.0F)
      beta = (float)atan2(vbody,sqrt(vt))*RTD;
   else
      beta = 0.0F;

   vt    = (float)sqrt(vt + vbody*vbody);

   /*----------------------------*/
   /* aero and propulsive forces */
   /*----------------------------*/
 
   Atmosphere();
   Trigenometry();
   Aerodynamics();
   Engine();

   /*-------------------------------*/
   /* force breakout on launch rail */
   /*-------------------------------*/
   xforce = ifd->xaero + ifd->xprop;
   xbreak = 200.0F/(m0 + mp0);
   if(xforce <= xbreak)
   {
      xforce = 0.0F;
   }

   /*--------------------------------------------*/
   /* increment in forward velocity and position */
   /*--------------------------------------------*/
   deltu = Math.FITust(xforce,SimLibMinorFrameTime,ifd->olddu);
   deltx = Math.FITust(deltu ,SimLibMinorFrameTime,ifd->olddx);

   /*-------------------*/
   /* earth coordinates */
   /*-------------------*/
   xdot =  vt*ifd->geomData.cosgam*ifd->geomData.cossig;
   ydot =  vt*ifd->geomData.cosgam*ifd->geomData.sinsig;
   zdot = -vt*ifd->geomData.singam;
   alt  = -z;

   /*---------------------------------------------------*/
   /* applied body axis accels on missile during launch */
   /*---------------------------------------------------*/
   ifd->nxcgb  = ifd->nxcgb + (xforce - initXLoc*(q*q + r*r) +
              initYLoc*(p*q) + initZLoc*(p*r))/GRAVITY;

   ifd->nycgb  = ifd->nycgb + (initXLoc*(p*q) - initYLoc*(r*r +
            p*p) + initZLoc*(q*r))/GRAVITY;

   ifd->nzcgb  = ifd->nzcgb + (initZLoc*(q*q + p*p) - initXLoc*(r*p) -
            initYLoc*(q*r))/GRAVITY;

   /*-------------------------------*/
   /* applied stability axis accels */
   /*-------------------------------*/
   ifd->nxcgs  = ifd->nxcgb*ifd->geomData.cosalp - ifd->nzcgb*ifd->geomData.sinalp;
   ifd->nycgs  = ifd->nycgb;
   ifd->nzcgs  = ifd->nzcgb*ifd->geomData.cosalp + ifd->nxcgb*ifd->geomData.sinalp;

   /*------------------*/
   /* wind axis accels */
   /*------------------*/
   ifd->nxcgw  =  ifd->nxcgs*ifd->geomData.cosbet + ifd->nycgs*ifd->geomData.sinbet;
   ifd->nycgw  = -ifd->nxcgs*ifd->geomData.sinbet + ifd->nycgs*ifd->geomData.cosbet;
   ifd->nzcgw  =  ifd->nzcgs;

   /*-----------------*/
   /* velocity vector */
   /*-----------------*/
   vtdot  = GRAVITY*(ifd->nxcgw - ifd->geomData.singam);
   //Cobra TJL we are div 0 here because of VT being 0.0000
   //I will set VT if 0.000
   if (vt == 0.0f)
	   vt = 0.01f;

   ifd->alpdot = q - (p*ifd->geomData.cosalp + r*ifd->geomData.sinalp)*ifd->geomData.tanbet - GRAVITY*(ifd->nzcgw -
            ifd->geomData.cosgam*ifd->geomData.cosmu)/(vt*ifd->geomData.cosbet);

   ifd->betdot = p*ifd->geomData.sinalp - r*ifd->geomData.cosalp + GRAVITY *
            (ifd->nycgw + ifd->geomData.cosgam*ifd->geomData.sinmu)/vt;

   /*--------------------------------*/
   /* test for free flight condition */ 
   /*--------------------------------*/
   
   //if(deltx > 0.6667*inputData->length)  // MLR 1/5/2005 - Uh, Why?  This causes the missles to lag behind the a/c
   {
	  
      launchState = InFlight;
	  Init2();

	  // missile launch effect
	  Tpoint pos, vec;
	 
	  pos.x = x;
	  pos.y = y;
	  pos.z = z;

	  
	  // test for drawpointer because we may be calling this
	  // via FindRocketGroundImpact
	  // MLR - drawPointer is valid due to a bug somewhere else
	  //       added FindingImpact flag
	  if ( drawPointer && !(flags & FindingImpact))
	  {

		  // MLR - Note - this is causing the smoke trail when rockets are selected.
		  //RV I-Hawk this is a missile launch event so
		  //changed type from LIGHT_CLOUD to MISSILE_LAUNCH
		  /*
		  OTWDriver.AddSfxRequest(
				new SfxClass ( SFX_MISSILE_LAUNCH,		// type
				0,
				&pos,					// world pos
				&vec,					// vel vector
				5.3F,					// time to live
				5.0f ) );				// scale
				*/
		  DrawableParticleSys::PS_AddParticleEx((SFX_MISSILE_LAUNCH + 1),
												&pos,
												&vec);
	
			// ACMI Output
			if (gACMIRec.IsRecording() )
			{
				misPos.hdr.time = SimLibElapsedTime * MSEC_TO_SEC + OTWDriver.todOffset;
				misPos.data.type = Type();
				misPos.data.uniqueID = ACMIIDTable->Add(Id(),NULL,TeamInfo[GetTeam()]->GetColor());//.num_;
				misPos.data.x = x;
				misPos.data.y = y;
				misPos.data.z = z;
				misPos.data.roll = phi;
				misPos.data.pitch = theta;
				misPos.data.yaw = psi;
// remove				misPos.data.teamColor = TeamInfo[GetTeam()]->GetColor();

				gACMIRec.MissilePositionRecord( &misPos );
			}
	  }

   }
}

void MissileClass::SetLaunchData (void)
{
	vt    = parent->GetVt();

	if (parent && parent->IsSim()){
		SimBaseClass *ptr = (SimBaseClass*)parent.get();

		// Transform position from parent's object space into world space
		//      x = parent->XPos() + ptr->dmx[0][0]*initXLoc + ptr->dmx[1][0]*initYLoc + ptr->dmx[2][0]*initZLoc;
		//      y = parent->YPos() + ptr->dmx[0][1]*initXLoc + ptr->dmx[1][1]*initYLoc + ptr->dmx[2][1]*initZLoc;
		//      z = parent->ZPos() + ptr->dmx[0][2]*initXLoc + ptr->dmx[1][2]*initYLoc + ptr->dmx[2][2]*initZLoc;
		x=XPos();
		y=YPos();
		z=ZPos();

		// Cobra - removed - fixed missile launching through the wing
		//	  xdot=XDelta();
		//	  ydot=YDelta();
		//	  zdot=ZDelta();

		float disp_az,disp_el; // MLR 1/17/2004 - Added random dispersion
		if(flags && FindingImpact){   
			// MLR - if we are computing the Impact Point, don't randomize the azimuth & elevation, 
			// MLR - or the HUD's impact predictor jumps around wildly
			disp_az=0;
			disp_el=0;
		}
		else {
			// MLR it's not so much a cone, as it is a 4 sided pyramid :)
			disp_az=((float)rand()/RAND_MAX - 0.5f) * auxData->rocketDispersionConeAngle / 180.0f * 3.14159f;
			disp_el=((float)rand()/RAND_MAX - 0.5f) * auxData->rocketDispersionConeAngle / 180.0f * 3.14159f ;
		}

		SimVehicleClass *pv = static_cast<SimVehicleClass*>(parent.get());
		theta = pv->Pitch() + initEl + disp_el; // MLR 1/18/2004 - added disp_el
		phi   = pv->Roll();
		psi   = pv->Yaw() + initAz + disp_az; // MLR 1/18/2004 - added disp_az
		// M.N. add vt > 200.0F check fixes hovering helicopters firing missiles not going ballistic
		if ( !parent->OnGround() && vt > 200.0F){
			p     = pv->GetP();
			q     = pv->GetQ();
			r     = pv->GetR();
			ifd->nxcgb = pv->GetNx();
			ifd->nycgb = pv->GetNy();
			ifd->nzcgb = pv->GetNz();
			//alpha = 0; //((SimVehicleClass*)parent)->GetAlpha(); // MLR 5/30/2004 - 
			//beta  = 0; //((SimVehicleClass*)parent)->GetBeta(); // MLR 5/30/2004 - 
			alpha = pv->GetAlpha();
			beta  = pv->GetBeta();
	
			if (parent->GetKias() < 250.0F){
				alpha *= parent->GetKias() / 250.0F;
				q *= parent->GetKias() / 250.0F;
			}
		}
		else {
			// edg: give ground launched missiles some extra oomph at
			// launch since this seems to cause problems
			vt = 200.0f;
			p     = 0.0F;
			q     = 0.0F;
			r     = 0.0F;
			ifd->nxcgb = 0.0F;
			ifd->nycgb = 0.0F;
			ifd->nzcgb = 0.0F;
			alpha = 0.0F;
			beta  = 0.0F;
		}
	}
	else {
		// Use parent's world space position corrected for terrain height
		x = parent->XPos();
		y = parent->YPos();
		z = parent->ZPos() + OTWDriver.GetGroundLevel( x, y );

		theta = initEl;
		phi   = 0.0F;
		psi   = initAz;
		p     = 0.0F;
		q     = 0.0F;
		r     = 0.0F;
		ifd->nxcgb = 0.0F;
		ifd->nycgb = 0.0F;
		ifd->nzcgb = 0.0F;
		alpha = 0.0F;
		beta  = 0.0F;
	}
}
