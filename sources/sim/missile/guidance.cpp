#include "stdhdr.h"
#include "object.h"
#include "otwdrive.h"
#include "missile.h"
#include "radar.h"
#include "simveh.h"
#include "fcc.h"
#include "BeamRider.h"

#define MANEUVER_DEBUG
#ifdef MANEUVER_DEBUG
#include "Graphics\include\drawbsp.h"
extern int g_nShowDebugLabels;
#endif
    
extern int g_nMissileFix;
extern float g_fTgtDZFactor;
extern bool g_bNewSensorPrecision;
extern bool g_bActivateDebugStuff;
extern bool g_bActivateMissileDebug;
    
void MissileClass::CheckGuidePhase(void) {
 
	//if (!targetPtr) return; //Cobra 10/31/04 TJL
	//Get Laser Position and store in targetX etc.

	ShiAssert(inputData);
   	if (!inputData)
   		return;
    
   	float dxi=0.0F, dyi=0.0F, dzi=0.0F;
    	
 	// MLR 4/9/2004 - Amazingly, most of the missile code is safe to call without
 	// a targetPtr.  For laser guided missiles, we can have a NULL targetPtr if we
 	// supply targetXYZ.
 
 	// laser targetting code
 	if(GetWCD()->GuidanceFlags & WEAP_LASER) {
		
		// only do this for the player
 		if( g_bRealisticAvionics && parent && ((SimVehicleClass *)parent.get())->IsPlayer()) {   
 			FireControlComputer* theFCC = ((SimVehicleClass*)parent.get())->GetFCC();

 			if(theFCC && theFCC->LaserArm && theFCC->LaserFire) {
 				SimObjectType *tgt;

 				tgt=theFCC->TargetPtr();
 
 				// update the targetPtr
 				// set the targetPtr so the missile can track moving
 				// targets and lead as needed.
 				if(tgt != targetPtr)
 					SetTarget(tgt);
 
				// no FCC target, track laser point
				if(!tgt) {	
 					// need FOV code
 					targetX = theFCC->groundDesignateX;
 					targetY = theFCC->groundDesignateY;
 					targetZ = theFCC->groundDesignateZ;
 
 					dxi = targetX - x;
 					dyi = targetY - y;
 					dzi = targetZ - z;
 				}	
 			}
			// no target
 			else {
 				return;
 			}
 		}
 		/// maintain what ever we had at launch
		//else {
		//// easy avionics, AI
		//SimObjectType *tgt;
		//tgt=theFCC->TargetPtr();
		//
		//if(tgt != targetPtr)
		//	SetTarget(tgt);
		//
		//if(!tgt)
		//	return;
		//}
 	}
 	
	if(targetPtr) {
    	dxi = targetPtr->BaseData()->XPos() - x;
    	dyi = targetPtr->BaseData()->YPos() - y;
    	dzi = targetPtr->BaseData()->ZPos() - z;
 	}

	range = (float)sqrt (dxi*dxi + dyi*dyi + dzi*dzi);
	
    //boost
	if (!guidencephase) {
		if (runTime > inputData->boostguidesec)
			guidencephase = 1;
	}
	//sustain
	else if (guidencephase == 1) {
		if (range < inputData->terminalguiderange)
			guidencephase = 2;
		else if (inputData->terminalguiderange <0 && inputData->mslActiveTtg >0 && sensorArray[0]->Type() == SensorClass::Radar)
			guidencephase = 2;
	}
}

// RV - Biker - This is debug stuff
extern float g_nboostguidesec;//me123 how many sec we are in boostguide mode
extern float g_nterminalguiderange;//me123 what range we transfere to terminal guidence
extern float g_nboostguideSensorPrecision;//me123
extern float g_nsustainguideSensorPrecision ;//me123
extern float g_nterminalguideSensorPrecision ; //me123
extern float g_nboostguideLead ;//me123
extern float g_nsustainguideLead ;//me123
extern float g_nterminalguideLead ;//me123
extern float g_nboostguideGnav ;//me123
extern float g_nsustainguideGnav ;//me123
extern float g_nterminalguideGnav ;//me123
extern float g_nboostguideBwap;//me123
extern float g_nsustainguideBwap;//me123
extern float g_nterminalguideBwap ;//me123

void MissileClass::CommandGuide (void) {
	float invRngSq = 0.0F, wic = 0.0F, wjc = 0.0F, wkc = 0.0F, loftBias = 0.0F;
	float dxi = 0.0F, dyi = 0.0F, dzi = 0.0F;
	float dxdoti = 0.0F, dydoti = 0.0F, dzdoti = 0.0F;
	float omegayReq = 0.0F, omegazReq = 0.0F;
	int hasTarget = FALSE;
	float SensorPrecision;
	float LeadA;
	float LeadB;
	float Gnav;
	float Bwap;

	ShiAssert(inputData);
	if (!inputData)
		return;

	CheckGuidePhase();

	// RV - Biker - This is debug stuff don't do the checks all the time
	if (g_bActivateDebugStuff && g_bActivateMissileDebug)
	{
		if (g_nboostguidesec)
			inputData->boostguidesec = g_nboostguidesec;//me123 how many sec we are in boostguide mode
		
		if (g_nterminalguiderange)
			inputData->terminalguiderange = g_nterminalguiderange;//me123 what range we transfere to terminal guidence
		
		if (g_nboostguideSensorPrecision)
			inputData->boostguideSensorPrecision = g_nboostguideSensorPrecision;//me123
		
		if (g_nsustainguideSensorPrecision)
			inputData->sustainguideSensorPrecision = g_nsustainguideSensorPrecision ;//me123
		
		if (g_nterminalguideSensorPrecision)
			inputData->terminalguideSensorPrecision = g_nterminalguideSensorPrecision ; //me123
		
		if (g_nboostguideLead)
			inputData->boostguideLead = g_nboostguideLead ;//me123
		
		if (g_nsustainguideLead)
			inputData->sustainguideLead = g_nsustainguideLead ;//me123
		
		if (g_nterminalguideLead)
			inputData->terminalguideLead = g_nterminalguideLead ;//me123
		
		if (g_nboostguideGnav)
			inputData->boostguideGnav = g_nboostguideGnav ;//me123
		
		if (g_nsustainguideGnav)
			inputData->sustainguideGnav = g_nsustainguideGnav ;//me123
		
		if (g_nterminalguideGnav)
			inputData->terminalguideGnav = g_nterminalguideGnav ;//me123
		
		if (g_nboostguideBwap)
			inputData->boostguideBwap = g_nboostguideBwap;//me123
		
		if (g_nsustainguideBwap)
			inputData->sustainguideBwap =g_nsustainguideBwap;//me123
		
		if (g_nterminalguideBwap)
			inputData->terminalguideBwap = g_nterminalguideBwap ;//me123
	}

	//boost
	if (!guidencephase) {
		SensorPrecision = inputData->boostguideSensorPrecision;
		LeadA = inputData->boostguideLead;
		LeadB = 1.0f/inputData->boostguideLead;
		Gnav = inputData->boostguideGnav;
		Bwap = inputData->boostguideBwap;
	}

	//sustain
	else if (guidencephase == 1) {
		SensorPrecision = inputData->sustainguideSensorPrecision;
		LeadA = inputData->sustainguideLead;
		LeadB = 1.0f/inputData->sustainguideLead;
		Gnav = inputData->sustainguideGnav;
		Bwap = inputData->sustainguideBwap;
	}

	//terminal
	else if (guidencephase == 2) {
		SensorPrecision = inputData->terminalguideSensorPrecision;
		LeadA = inputData->terminalguideLead;
		LeadB = 1.0f/inputData->terminalguideLead;
		Gnav = inputData->terminalguideGnav;
		Bwap = inputData->terminalguideBwap;
	}

	// No Target
 	if (runTime > inputData->guidanceDelay) {
		if (g_bNewSensorPrecision && flags & SensorLostLock &&  sensorArray && sensorArray[0] && sensorArray[0]->Type() == SensorClass::RadarHoming) {
			ifd->augCommand.yaw = 0.0f;
			ifd->augCommand.pitch = 0.0f;
			return;
		}
		if (targetPtr) {
			hasTarget = TRUE;
         
			// Inertial Line of Sight Vector
			float rangeplatform =0;
			if (auxData && auxData->errorfromparrent && sensorArray && sensorArray[0] && sensorArray[0]->Type() == SensorClass::RadarHoming) {
				FalconEntity* RadarPlt = ((BeamRiderClass*)this->sensorArray[0])->Getplatform();
				RadarClass* theRadar;
				
				if (RadarPlt)
					theRadar = (RadarClass*)FindSensor( (SimMoverClass*)(  RadarPlt ), SensorClass::Radar);
				
				if (theRadar)
					rangeplatform = ((SensorClass*)theRadar)->CurrentTarget()->localData->range;
			}

			// the system get an update
			if(runTime > GuidenceTime) {
				GuidenceTime =  runTime + Bwap;//me123
				dxi = targetPtr->BaseData()->XPos() - x;
				dyi = targetPtr->BaseData()->YPos() - y;
				dzi = targetPtr->BaseData()->ZPos() - z;
				dxdoti = (targetPtr->BaseData()->XDelta()*LeadB) - (xdot*LeadA);
				dydoti = (targetPtr->BaseData()->YDelta()*LeadB) - (ydot*LeadA);
				dzdoti = (targetPtr->BaseData()->ZDelta()*LeadB) - (zdot*LeadA);
				targetX	= targetPtr->BaseData()->XPos();
				targetY	= targetPtr->BaseData()->YPos();
				targetZ	= targetPtr->BaseData()->ZPos();
				targetDX = targetPtr->BaseData()->XDelta();
				targetDY = targetPtr->BaseData()->YDelta();
				targetDZ = targetPtr->BaseData()->ZDelta();

				//me123 add some randomeness to the guidence
		 		range = (float)sqrt (dxi*dxi + dyi*dyi + dzi*dzi);

				if (rangeplatform) range = rangeplatform;
					float error = (float)sin (SensorPrecision*DTR)*range;
				
				if (!g_bNewSensorPrecision) {
					float var =((-error/2)+(error*((float)rand()/(float)RAND_MAX)));
					targetDX += var;
					targetDY -= var;	
					targetDZ += var;
				}
				else {
					float var =((-error/2)+(error*((float)rand()/(float)RAND_MAX)));
					targetX	+= var;
					var =((-error/2)+(error*((float)rand()/(float)RAND_MAX)));
					targetY	+= var;	
					var =((-error/2)+(error*((float)rand()/(float)RAND_MAX)));
					targetZ   += var;
				}
			}
			//"coast on the last info we 
			else {
				dxi = targetX - x;
				dyi = targetY - y;
				dzi = targetZ - z;
				dxdoti = (targetDX*LeadB) - (xdot*LeadA);
				dydoti = (targetDY*LeadB) - (ydot*LeadA);
				dzdoti = (targetDZ*LeadB) - (zdot*LeadA);
				targetX += targetDX * SimLibMajorFrameTime;
				targetY += targetDY * SimLibMajorFrameTime;
				targetZ += targetDZ * SimLibMajorFrameTime;
			}

			if (targetPtr->BaseData()->IsCampaign()) {
				// To save time, we're using the appoximate ground level at the target's location.
				// This will potentially introduce some error in the ultimate impact point, but should be acceptable.
				// We had to make this corection since all campaign entities were storing
				// their z position as "AGL" instead of world space height above sea level.
				dzi += OTWDriver.GetApproxGroundLevel( targetPtr->BaseData()->XPos(), targetPtr->BaseData()->YPos() );
			}

			range = (float)sqrt (dxi*dxi + dyi*dyi + dzi*dzi);
			timpct = range/(vt + targetPtr->BaseData()->GetVt() * (float)cos(targetPtr->localData->ataFrom));
		
			// The target is getting away
			if (timpct < 0.0f) {
				timpct = 100.0f;
			}

			invRngSq  = 1.0F / (range*range);
		}
		// we've lost our target, but want to continue our flight to the last known location
		else if (targetX != -1.0F && targetY != -1.0F && targetZ != -1.0F) {
			hasTarget = TRUE;
         
			// Inertial Line of Sight Vector
			dxi = targetX - x;
			dyi = targetY - y;
			dzi = targetZ - z;

			dxdoti = targetDX - xdot;
			dydoti = targetDY - ydot;
			dzdoti = targetDZ - zdot;

			// 2002-04-07 MN diminish targetDZ so missile won't go too ballistic...
			if (g_nMissileFix & 0x40) {
				if (targetDZ > 0.0F) {
					targetDZ -= g_fTgtDZFactor * SimLibMajorFrameTime;
					if (targetDZ < 0.0F)
						targetDZ = 0.0F;
				}
				else {
					targetDZ += g_fTgtDZFactor * SimLibMajorFrameTime;
					if (targetDZ > 0.0F)
						targetDZ = 0.0F;
				}
			}

			range = (float)sqrt (dxi*dxi + dyi*dyi + dzi*dzi);
			timpct = range / (float)sqrt (dxdoti*dxdoti + dydoti*dydoti + dzdoti*dzdoti);
			invRngSq  = 1.0F / (range*range);

			// Accumulate the velocity into the target point -- a little sloppy to do it here
			// since it _should_ happen every frame once the data is set, but this will only happen
			// when we're actually relying on the data.  This should be good enough, though.
			targetX += targetDX * SimLibMajorFrameTime;
			targetY += targetDY * SimLibMajorFrameTime;
			targetZ += targetDZ * SimLibMajorFrameTime;
		}

	}

	if (hasTarget) {
		wic = (dyi*dzdoti - dzi*dydoti) * invRngSq;
		wjc = (dzi*dxdoti - dxi*dzdoti) * invRngSq;
		wkc = (dxi*dydoti - dyi*dxdoti) * invRngSq;
	
		// Lofting Bias
		if (runTime > inputData->guidanceDelay && runTime < inputData->guidanceDelay + inputData->mslLoftTime) {
			loftBias = -inputData->mslBiasn - 1.0F;	// In units of G
		}
		else {
			loftBias = -1.0F;							// In units of G
		}

		// Desired wind axis rates
		omegayReq = Gnav * (dmx[1][0]*wic + dmx[1][1]*wjc + dmx[1][2]*wkc);
		omegazReq = Gnav * (dmx[2][0]*wic + dmx[2][1]*wjc + dmx[2][2]*wkc);

		// Inertial (body relative) load
		// factor commands in units of G
		// Includes gravity component
		ShiAssert (ifd);
	  
		if (ifd) {
			ifd->augCommand.yaw   = vt * omegazReq / GRAVITY + ifd->geomData.costhe*ifd->geomData.sinphi * (loftBias);
			ifd->augCommand.pitch = -(vt * omegayReq / GRAVITY) + ifd->geomData.costhe*ifd->geomData.cosphi * (loftBias);
		}
	}
	else {
		//MI 03/02/02 uncommented to get BORE shots to not get set as missiled immediately after launch
		range = 100000.0F;
		//timpct   = 100.0F;

		// Set control commands (in units of Gs)
		ShiAssert (ifd);
		if (ifd) {
			ifd->augCommand.yaw   = -2.0F * ifd->nycgb + ifd->geomData.costhe*ifd->geomData.sinphi;
			ifd->augCommand.pitch =  2.0F * ifd->nzcgb - ifd->geomData.costhe*ifd->geomData.cosphi;
		}
	}

#ifdef MANEUVER_DEBUG
	char tmpStr[40];
	char label[20];

	if (g_nShowDebugLabels & 0x02) {
		if (drawPointer) {
			if (targetPtr)
				sprintf(tmpStr,"Lock ");
			else
				sprintf(tmpStr,"NoLock ");
				
			if (flags & SensorLostLock)
				strcat(tmpStr,"OTgtL ");
			
			if (!guidencephase)
				sprintf(label, "B %4.1f %5.0f", vcas, -z);

			if (guidencephase == 1)
				sprintf(label, "G %4.1f %5.0f", vcas, -z);

			if (guidencephase == 2)
				sprintf(label, "T %4.1f %5.0f", vcas, -z);

			strcat (tmpStr, label);

			if (wentActive)
				strcat(tmpStr, " Activ");
			((DrawableBSP*)drawPointer)->SetLabel (tmpStr, ((DrawableBSP*)drawPointer)->LabelColor());
		}
	}
#endif

//#define MISSILEDEBUG 1
#ifdef MISSILEDEBUG
	if (sensorArray && sensorArray[0] && sensorArray[0]->Type() == SensorClass::RWR && launchState == InFlight) {
		static int			file = -1;
		static int			binfile = -1;
		static char			buffer[256];
		static unsigned		now;
		static MissileClass	*theOne = NULL;
	   
		now = SimLibElapsedTime;
	   
		if (file < 0) {
			theOne = this;
			binfile = open( "C:\\temp\\MissileTrack.bin", _O_CREAT | _O_TRUNC | _O_WRONLY | _O_BINARY, 0000666 );
			file = open( "C:\\temp\\MissileTrack.txt", _O_CREAT | _O_TRUNC | _O_WRONLY, 0000666 );
			sprintf( buffer, "Missile Guidance Dump started %2d:%2d.%2d for (0x%X)\n", now/60000, now%60000/1000, now%1000/10, this );
			write( file, buffer, strlen(buffer) );
			sprintf( buffer, "Time     Addr           T   rng   ttg   gndZ   xFromTgt yFromTgt zFromTgt xdot  ydot  zdot  yawCmd ptchCmd\n" );
			write( file, buffer, strlen(buffer) );
		}
		else {
			if (this != theOne) {
			// Get out now so we only have data for one missile to sort through
			return;
			}
		}
	
		sprintf( buffer, "%2d:%2d.%2d \t(0x%X)  %c \t%6.0f \t%3.2f \t%4.0f \t%6.0f \t%6.0f \t%6.0f \t%3.0f \t%3.0f \t%3.0f \t%1.1f \t%1.1f\n", 
			now/60000, now%60000/1000, now%1000/10, this,
			hasTarget ? 'Y' : 'N', range, timpct, groundZ,
			dxi, dyi, dzi, 
			xdot, ydot, zdot,
			ifd->augCommand.yaw, ifd->augCommand.pitch );
		write( file, buffer, strlen(buffer) );
		_commit( file );
	
		typedef struct DataPoint {
			unsigned	time;
			float		x, y, z;
			float		dx, dy, dz;
			float		yawCmd, pitchCmd;
			int			targetState;
			float		range;
			float		timeToImpact;
			float		groundZ;
			float		targetX, targetY, targetZ;
		} DataPoint;
		
		DataPoint	data;
		data.time			= now;
		data.x				= x;
		data.y				= y;
		data.z				= z;
		data.dx				= xdot;
		data.dy				= ydot;
		data.dz				= zdot;
		data.yawCmd			= ifd->augCommand.yaw;
		data.pitchCmd		= ifd->augCommand.pitch;
		data.range			= range;
		data.timeToImpact	= timpct;
		data.groundZ		= groundZ;
	
		if (hasTarget) {
			if (targetPtr) {
				data.targetState	= 2;
			} else {
				data.targetState	= 1;
			}
		} else {
			data.targetState		= 0;
		}
	
		if (targetPtr) {
			data.targetX		= targetPtr->BaseData()->XPos();
			data.targetY		= targetPtr->BaseData()->YPos();
			data.targetZ		= targetPtr->BaseData()->ZPos();
			
			if (targetPtr->BaseData()->IsCampaign()) {
				// To save time, we're using the appoximate ground level at the target's location.
				// This will potential introduce some error in the ultimate impact point, but should be acceptable.
				// We have to make this corection since all campaign entities store
				// their z position as "AGL" instead of world space height above sea level.
				data.targetZ += OTWDriver.GetApproxGroundLevel( targetPtr->BaseData()->XPos(), targetPtr->BaseData()->YPos() );
			}
		} else {
			data.targetX		= targetX;
			data.targetY		= targetY;
			data.targetZ		= targetZ;
		}
	   write( binfile, &data, sizeof(data) );
		_commit( binfile );
	}
#endif
}
