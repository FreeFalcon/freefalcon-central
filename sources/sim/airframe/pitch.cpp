/******************************************************************************/
/*                                                                            */
/*  Unit Name : pitch.cpp                                                     */
/*                                                                            */
/*  Abstract  : Pitch axis control laws.                                      */
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
#include "simbase.h"
#include "vutypes.h"
#include "limiters.h"
#include "aircrft.h"
#include "dofsnswitches.h"
#include "simdrive.h" // MN check for player aircraft

extern VU_TIME vuxGameTime;
//extern bool g_bEnableAircraftLimits;//me123	MI replaced with g_bRealisticAvionics
extern bool g_bRealisticAvionics;
/********************************************************************/
/*                                                                  */
/* Routine: void AirframeClass::Pitch(void)                        */
/*                                                                  */
/* Description:                                                     */
/*    Calculate new AOA using Prop/Integral feedback loop on        */
/*    user input.                                                   */
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

float gmmaHold = 0.0F;
extern bool g_bNewFm;
void AirframeClass::Pitch(void)
{
	//if(vt == 0.0F)
	//if(IsSet(Planted) || (!IsSet(InAir) && pstick <= 0.0F) )
	if(IsSet(Planted))
		return;

	float error, eprop, eintg1, eintg;
	float cosmu_lim,alphaError;//, tempnzcgs;
	float aoacmd, ptcmd, maxNegGs, maxCmd;
	float minCmd = 0.0f; //Cobra 10/30/04 TJL
	//float limitGs, alpdt1;
	Limiter *limiter = NULL;
	
	alphaError = 0.0F;
	cosmu_lim = max(0.0F,platform->platformAngles.cosmu);
	float maxgcmd = alpha + (maxGs - cl*qsom/GRAVITY - cosmu_lim* platform->platformAngles.cosgam) * (GRAVITY/(qsom*cnalpha));
	//tempnzcgs = (oldnzcgs + nzcgs)*0.5F;

	/*--------------------------------*/
	/* Check for AOA/G command system */
	/*--------------------------------*/
	if (IsSet(AOACmdMode))
	{
		/*------------------*/
		/* AOA command path */
		/*------------------*/
		
		if(IsSet(InAir))
		{
		    alphaError = (float)(fabs(q*0.5F) + fabs(p))*0.1F*RTD;
			minCmd = aoamin - alphaError; // needed for non f16 case
			maxCmd = aoamax + alphaError;
		}

		if(IsSet(MPOverride) && (alpha > 29.0F))
		{
			minCmd = max(gsAvail*(-1)*GRAVITY/(qsom*cnalpha),-9.0F);
		}
		else 
		{
			if(IsSet(CATLimiterIII))
			{
				limiter = gLimiterMgr->GetLimiter(CatIIIAOALimiter,vehicleIndex);
				if(limiter)
					maxCmd = limiter->Limit(alpha - alphaError);

// 2002-03-12 MN use 9.0f G's only for F-16 and player entity - AI planes can use this code, too
			if(SimDriver.GetPlayerEntity() && platform == SimDriver.GetPlayerEntity() && platform->IsAirplane() && platform->IsF16()) // 2002-03-19 MODIFIED BY S.G. Lets make sure it's an airplane first. Seems odd to happen here but it CTD after ejecting (BT 1071)
				maxCmd = min(maxCmd + alphaError, alpha + (9.0f - cl*qsom/GRAVITY - cosmu_lim* platform->platformAngles.cosgam) * (GRAVITY/(qsom*cnalpha)));
			else
				maxCmd = min(maxCmd + alphaError, alpha + (curMaxGs - cl*qsom/GRAVITY - cosmu_lim* platform->platformAngles.cosgam) * (GRAVITY/(qsom*cnalpha)));

			}
			else
			{
				//TJL 05/08/04
				float tempMaxGs = maxGs;
				limiter = gLimiterMgr->GetLimiter(PosGLimiter, vehicleIndex);
				if (limiter)
					tempMaxGs = limiter->Limit(alpha);
				
				if (g_bNewFm)
				//maxCmd = min(alpha + (maxGs - cl*qsom/GRAVITY - cosmu_lim* platform->platformAngles.cosgam) * (GRAVITY/(qsom*cnalpha)), aoamax);
				maxCmd = min(alpha + (tempMaxGs - cl*qsom/GRAVITY - cosmu_lim* platform->platformAngles.cosgam) * (GRAVITY/(qsom*cnalpha)), aoamax);
				else
  				//maxCmd = min(alpha + (maxGs - cl*qsom/GRAVITY - cosmu_lim* platform->platformAngles.cosgam) * (GRAVITY/(qsom*cnalpha)), aoamax + alphaError);
				maxCmd = min(alpha + (tempMaxGs - cl*qsom/GRAVITY - cosmu_lim* platform->platformAngles.cosgam) * (GRAVITY/(qsom*cnalpha)), aoamax + alphaError);
	
				if (maxCmd < 0) maxCmd =0;
				limiter = gLimiterMgr->GetLimiter(AOALimiter,vehicleIndex);

				if (g_bNewFm && limiter)
				maxCmd =  min( limiter->Limit(alpha), maxCmd);
				//maxCmd *= (GRAVITY/(qsom*cnalpha));
				//maxCmd = min(maxCmd, aoamax);
				
				

				//TJL 04/10/04 Adding limiter
				//TJL 05/07/04 Removing the dreaded code as this may have caused
				//major issues in the AI floating point calculations 
				//but I don't know why.  Possibly even missing aircraft!
			/*
				limiter = gLimiterMgr->GetLimiter(PosGLimiter, vehicleIndex);
				if (limiter)
					maxGs = limiter->Limit(alpha);
			*/
					

			}	

			if(platform->IsF16() )
			{
				if(!IsSet(MPOverride) )
				{
					//negative G limiter
					if(gearPos)
					{
						maxNegGs = -1.0F;
					}
					else
					{
						maxNegGs = -3.0F;
						limiter = gLimiterMgr->GetLimiter(NegGLimiter,vehicleIndex);
						if(limiter)
							maxNegGs = limiter->Limit(vcas);						
					}
					minCmd = max(maxNegGs*GRAVITY/(qsom*cnalpha), -10.0F);
				}
				else
				{
					minCmd = max(gsAvail*(-1)*GRAVITY/(qsom*cnalpha),-9.0F);
				}

				if (g_bNewFm )
				{
					float overshoot = alpha - maxCmd;
					float minspeed = 90.0f;
					if(stallMode == None)
					{
						alphaError = 0; 
						float alphadelta = maxCmd-2.0f - alpha;
						if (alphadelta <0 )//aoa has overshot max
						{
							alphaError += alphadelta*0.4f;
			
							if (alpdot >0) // aoa going up 
							{
								float slowspeedfactor =0.0f;
								float slowspeed = 200.0f;
								if (vcas < slowspeed) slowspeedfactor = ((slowspeed-vcas)/slowspeed);
								//vcas = 0 -> ssf = 1
								//vcas = 130 -> ssf = 0
								alphaError += min(10.0f,alpdot)*(0.2f+slowspeedfactor*0.7f); 
							}
							else // we are recovering
							alphaError -= alpdot*0.9f;// slow down the return
						}
						else if(!IsSet(CATLimiterIII)) 
						{// aoa is below max
						alphaError += alphadelta* 0.1f;//aoa elastic  only cat I otw up
						}
						 if (vcas < minspeed)//simulate we loose authority
						{
							alphaError += ((minspeed-vcas)/minspeed)*10*(float)fabs(alpdot);
						}
					}
					if (maxgcmd) maxCmd = min(maxCmd + alphaError,maxgcmd);
					maxCmd = max(minCmd,maxCmd);
				}

				if (alpha - alphaError > maxCmd)
				{
					if (!g_bNewFm)
					pshape = max(-1.0F, (maxCmd - alpha - alphaError)/4.0F);
					else if (stallMode == None)
					pshape = pshape* 1.0f-((alpha - alphaError-maxCmd)/3.0f);


				}
				else if(alpha + alphaError < minCmd)
				{
					pshape = min(1.0F,(minCmd - alpha + alphaError)/4.0F);
				}
			}					
		}
		
		if (pshape >= 0.0)
		{
			ptcmd = pshape * (maxCmd );
		}
		else 
			ptcmd = -pshape * (minCmd);
				
		/*---------------------------*/
		/* Forward path error signal */
		/*---------------------------*/
		error  = (ptcmd - (alpha - aoabias)) * kp05;
		eprop  = kp02 * error;
		eintg1 = kp03 * error;
	}
	//Go To G Command
	else
	{
		/*-----------------*/
		/* NZ command path */
		/*-----------------*/
		ptcmd = pshape*kp01;
		
		maxNegGs = -3.0F;
		//me123 OWLOOK switch needet to enable overg's
		//if (g_bEnableAircraftLimits)	MI

// 2003-03-12 MN changed to check for player and F-16
// guys, this code is also used by ALL AI planes in complex mode (having a TU-16 with 9G limit !!!)
		if(SimDriver.GetPlayerEntity() && platform == SimDriver.GetPlayerEntity() && platform->IsAirplane()  && platform->IsF16()) // 2002-03-19 MODIFIED BY S.G. Lets make sure it's an airplane first. Seems odd to happen here but it CTD after ejecting (BT 1071)
		maxCmd = 9.0f ;	
		else
		maxCmd = curMaxGs ;// me123 status ok. changed from = curMaxGs to 9.0f;
		
		if( platform->IsF16() )
		{
			if(!IsSet(MPOverride))
			{
				limiter = gLimiterMgr->GetLimiter(NegGLimiter,vehicleIndex);
				if(limiter)
					maxNegGs = limiter->Limit(vcas);
				maxNegGs = max(maxNegGs, -10.0F/(GRAVITY/(qsom*cnalpha)) );
			}
			else
				maxNegGs = gsAvail*(-1);
			
			if(alpha < -10.0F && !IsSet(MPOverride))
			{
				pshape = min(1.0F,(aoamin - alpha)/4.0F);
				ptcmd = pshape*kp01;
			}			
		}

		if(IsSet(CATLimiterIII))
		{
			limiter = gLimiterMgr->GetLimiter(CatIIIAOALimiter,vehicleIndex);
			if(limiter)
				maxCmd = limiter->Limit(alpha)/( GRAVITY/(qsom*cnalpha));

			// 2002-03-12 MN Check for player - if not, use curMaxGs (which is the case for all AI planes)
			AircraftClass *playerAC = SimDriver.GetPlayerAircraft();
			if(playerAC && this == playerAC->af && platform->IsF16()){
				// 2002-03-19 MODIFIED BY S.G. Lets make sure it's an airplane first. SimDriver.GetPlayerEntity()->af might be invalid if the player ejects
				maxCmd = min(maxCmd, 9.0F) - platform->platformAngles.cosgam*cosmu_lim;//,me123 status ok. changed curMaxGs to 9.0
			}
			else {
				maxCmd = min(maxCmd, curMaxGs) - platform->platformAngles.cosgam*cosmu_lim;
			}
		}
		
		ptcmd = min ( max (ptcmd, max(maxNegGs,gsAvail*(-1)) ), min(gsAvail,maxCmd));
		
		/*-----------------------------*/
		/* nz load factor loop closure */
		/*-----------------------------*/
		//TJL 03/28/04 Removing the gear pos bias
		//put it back, just because no time to test what the issue here is with pitch errors
		if(IsSet(Simplified))
		{
			error = (ptcmd - (nzcgs - platform->platformAngles.cosmu * platform->platformAngles.cosgam - 0.1F*gearPos*qsom/GRAVITY)) * kp05;
			//error = (ptcmd - (nzcgs - platform->platformAngles.cosmu * platform->platformAngles.cosgam - 0.1F*0.0f*qsom/GRAVITY)) * kp05;
		}
		else
		{
			error = (ptcmd - (nzcgs - cosmu_lim* platform->platformAngles.cosgam - 0.1F*gearPos*qsom/GRAVITY)) * kp05;
			//error = (ptcmd - (nzcgs - cosmu_lim* platform->platformAngles.cosgam - 0.1F*0.0f*qsom/GRAVITY)) * kp05;
		}

		eprop = kp02*error;
		eintg1 = kp03*error;
	}
	
	eintg = Math.FIAdamsBash (eintg1, SimLibMinorFrameTime, oldp02);
	
	/*-------------*/
	/* aoa limiter */
	/*-------------*/
	
		if(qbar > 100.0f)
		{
			if(eintg > aoamax)
			{
				eintg       = aoamax;
				eprop       = 0.0F;
				oldp02[0]   = aoamax;
				oldp02[1]   = aoamax;
				oldp02[2]   = 0.0;
				oldp02[3]   = 0.0;
			}
			else if(eintg < aoamin)
			{
				eintg       = aoamin;
				eprop       = 0.0F;
				oldp02[0]   = aoamin;
				oldp02[1]   = aoamin;
				oldp02[2]   = 0.0;
				oldp02[3]   = 0.0;
			}
		}
		else
		{
			if(eintg > aoamax + alphaError )
			{
				eintg       = aoamax + alphaError;
				eprop       = 0.0f;
				oldp02[0]   = eintg;
				oldp02[1]   = eintg;
				oldp02[2]   = eprop;
				oldp02[3]   = eprop;
			}
			else if(eintg < aoamin - alphaError)
			{
				eintg       = aoamin - alphaError;
				eprop       = 0.0f;
				oldp02[0]   = eintg;
				oldp02[1]   = eintg;
				oldp02[2]   = eprop;
				oldp02[3]   = eprop;
			}
		}
	
	/*---------------------------*/
	/* Update Alpha and alphadot */
	/*---------------------------*/
	aoacmd = eprop + eintg;
	if(IsSet(InAir))
		aoacmd *= plsdamp;

	switch(stallMode)
	{
	case EnteringDeepStall:
		if( alpha > 0.0f)
			aoacmd = 60.0f + 5.0F * pshape;
		else
			aoacmd = -40.0f + 5.0F * pshape;
		break;
		
	case DeepStall:
		if( alpha > 35.0f && qbar * platform->platformAngles.cosalp < 135.0f )
		{
			pshape *= max(1.0F - (float)fabs(r)*RTD/45.0F, 0.0F);
			
			if(oscillationTimer*pshape > 0)
				stallMagnitude += (float)fabs(pshape) * SimLibMinorFrameTime * 10.0F/(loadingFraction * loadingFraction);
			else if(pshape)
				stallMagnitude -= (float)fabs(pshape) * SimLibMinorFrameTime * 10.0F/(loadingFraction * loadingFraction);
			else 
				stallMagnitude += (desiredMagnitude - stallMagnitude)/desiredMagnitude * SimLibMinorFrameTime/3;
			
			aoacmd = 60.0f + pshape * 5.0f + oscillationTimer * stallMagnitude * max(0.0F,(0.3F - (float)fabs(r))*3.3F);
		}
		else if(alpha < -20.0f && qbar * platform->platformAngles.cosalp < 135.0f )
		{
			pshape *= max(1.0F - (float)fabs(r)*RTD/45.0F, 0.0F);
			
			if(oscillationTimer*pshape > 0)
				stallMagnitude += (float)fabs(pshape) * SimLibMinorFrameTime * 10.0F/(loadingFraction * loadingFraction);
			else if(pshape)
				stallMagnitude -= (float)fabs(pshape) * SimLibMinorFrameTime * 10.0F/(loadingFraction * loadingFraction);
			else 
				stallMagnitude += (desiredMagnitude - stallMagnitude)/desiredMagnitude * SimLibMinorFrameTime/3;
			
			aoacmd = -40.0f + pshape * 5.0f + oscillationTimer * stallMagnitude * max(0.0F,(0.3F - (float)fabs(r))*3.3F);
		}
		else
		{
			pitch = q;
			slice = r;
			stallMode = Recovering;
			stallMagnitude = 10.0f;
			oldp02[5] = alpha;
		}
		break;
		
	case Recovering:
		aoacmd = aoacmd + (oldp02[5] - aoacmd)*0.8F;
		oldp02[5] *= 0.9F;
		break;

	case Spinning:
		if(r)
		{
			if(platform->platformAngles.cosphi > 0.0F)
				aoacmd = 60.0F + oscillationTimer * 5.0F/(float)fabs(r);
			else
				aoacmd = -40.0F - oscillationTimer * 5.0F/(float)fabs(r);
		}
		else
		{
			if(platform->platformAngles.cosphi > 0.0F)
				aoacmd = 60.0F + oscillationTimer * 5.0F;
			else
				aoacmd = -40.0F - oscillationTimer * 5.0F;
		}
	}
	
	//if(!IsSet(InAir) && aoacmd < 0.0F)
	//	aoacmd = 0.0F;

	PitchIt(aoacmd, SimLibMinorFrameTime);
	
	switch(stallMode)
	{
		case DeepStall:
			oldp02[5] *= 0.97F;
			alpha += oldp02[5];
		break;
		case FlatSpin:
			oldp02[5] -= (90.0F + oldp02[5])* SimLibMinorFrameTime*0.2F;
			alpha = oldp02[5];
		break;
		
		case Spinning:
			oldp02[5] -= (85.0F + oldp02[5])* SimLibMinorFrameTime*0.2F;
			alpha = oldp02[5] + oscillationTimer * 5.0F;
		break;
		default:
		break;
	}
	
}

void AirframeClass::PitchIt(float aoacmd, float dt)
{
	// JB 010714 mult by the momentum
	float oldalpha = alpha;
	alpha  = Math.F7Tust(aoacmd, tp01 * auxaeroData->pitchMomentum, 
		tp02 * auxaeroData->pitchMomentum, tp03 * auxaeroData->pitchMomentum,
		dt, oldp03, &jp01);
	alpdot = (alpha - oldalpha)/dt;
	ShiAssert(!_isnan(alpha));
		
	if(alpha < -180.0F)
	{
		oldp03[0] += 360.0F;
		oldp03[1] += 360.0F;
		oldp03[2] += 360.0F;
		oldp03[3] += 360.0F;
	}
	else if(alpha > 180.0F)
	{
		oldp03[0] -= 360.0F;
		oldp03[1] -= 360.0F;
		oldp03[2] -= 360.0F;
		oldp03[3] -= 360.0F;
	}
	
}
