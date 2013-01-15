/******************************************************************************/
/*                                                                            */
/*  Unit Name : axial.cpp                                                     */
/*                                                                            */
/*  Abstract  : Calculates addition axial forces                              */
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
#include "Graphics/Include/drawparticlesys.h"
#include "airframe.h"
#include "fack.h"
#include "aircrft.h"
#include "fsound.h"
#include "soundfx.h"
#include "otwdrive.h"
#include "fakerand.h"
#include "sfx.h"
#include "playerop.h"
#include "dofsnswitches.h"

/********************************************************************/
/*                                                                  */
/* Routine: void AirframeClass::Axial (void)                       */
/*                                                                  */
/* Description:                                                     */
/*    Calculate forces due to speed brake                           */
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
extern bool g_bRealisticAvionics;
void AirframeClass::Axial(float dt)
{
float probability;
float maxQbar, chance;
float dBrakeMax;

   /*---------------------*/
   /* speed brake command */
   /*---------------------*/

	//MI retracts our SBR if it's > 43° and gear down and locked
    //TJL 11/21/03 added isF16 so non-F16 aircraft will not retract brakes with gear down
	if(platform->IsF16() && gearPos == 1.0F && HydraulicA() != 0 && speedBrake == 0)
	{
		//if you hold the switch, they go to 60°
		if(speedBrake == 1.0F)
		{
			dbrake += 0.3F * dt * speedBrake;
		}
		//always stay where you are when on ground
		else if(platform->OnGround() && platform->Pitch() * RTD <= 0);
		else if(dbrake > (1.0F - (60.0F - 43.0F)/60.0F))
		{
			//Move the brake
			dbrake += 0.3F * dt * -1.0F;
			//Make some music
			platform->SoundPos.Sfx(auxaeroData->sndSpdBrakeLoop);
		}
	}

   if (speedBrake != 0.0)
   {
      dbrake += 0.3F * dt * speedBrake;

      // Limit max speed brake to 43 degrees when gear is fully extended
	  //MI make it realistic
#if 0
        dBrakeMax = 1.0F - (60.0F - 43.0F)/60.0F * gearPos;
		dbrake = min ( max ( dbrake, 0.0F), dBrakeMax);
#else
		dBrakeMax = 1.0F;
		dbrake = min ( max ( dbrake, 0.0F), dBrakeMax);
#endif

		// do sound effect.  This has begin, end and loop pieces
		if ( speedBrake < 0.0f )
		{
			// closing brake
		 	if ( dbrake > 0.90f * dBrakeMax && dbrake < dBrakeMax &&
			  	 !platform->SoundPos.IsPlaying( auxaeroData->sndSpdBrakeStart) 
				 
				 ) // JB 010425
			{
				platform->SoundPos.Sfx( auxaeroData->sndSpdBrakeStart );
			}

		 	if ( dbrake < 0.10f && dbrake > 0.0f &&
				 !platform->SoundPos.IsPlaying( auxaeroData->sndSpdBrakeEnd) 
				 
				 ) // JB 010425
			{
				platform->SoundPos.Sfx( auxaeroData->sndSpdBrakeEnd );
			}

		}
		else
		{
			// opening brake
		 	if ( dbrake < 0.10f && dbrake > 0.0f &&
				 !platform->SoundPos.IsPlaying( auxaeroData->sndSpdBrakeStart) 
				 ) // JB 010425
			{
				platform->SoundPos.Sfx( auxaeroData->sndSpdBrakeStart);
			}

		 	if ( dbrake > 0.90f * dBrakeMax && dbrake < dBrakeMax  &&
				!platform->SoundPos.IsPlaying( auxaeroData->sndSpdBrakeEnd) 
				 ) // JB 010425
			{
				platform->SoundPos.Sfx( auxaeroData->sndSpdBrakeEnd);
			}
		}

	 	if ( dbrake > 0.05f && dbrake < 0.95F * dBrakeMax )
			platform->SoundPos.Sfx( auxaeroData->sndSpdBrakeLoop);

		//MI fix for "b" key
		if((dbrake == 1.0F || dbrake == 0) && speedBrake != 0 && BrakesToggle)
		{
			speedBrake = 0.0F;
			BrakesToggle = FALSE;
		}
   }

   if (gearHandle != 0 && !IsSet(GearBroken) && IsSet(InAir))
   {
      	gearPos += 0.3F * dt * gearHandle;
		gearPos = min ( max ( gearPos, 0.0F), 1.0F);

		// do sound effect.  This has begin, end and loop pieces
		if ( gearHandle < 0.0f )
		{
			// closing brake
		 	if ( gearPos > 0.90f && gearPos < 1.0f && 
				!platform->SoundPos.IsPlaying(auxaeroData->sndGearCloseStart)
				)
			{
				platform->SoundPos.Sfx( auxaeroData->sndGearCloseStart );
			}

		 	if ( gearPos < 0.10f && gearPos > 0.0f && 
				!platform->SoundPos.IsPlaying(auxaeroData->sndGearCloseEnd)
				)
			{
				platform->SoundPos.Sfx( auxaeroData->sndGearCloseEnd );
			}

		}
		else
		{
			// opening brake
		 	if ( gearPos < 0.10f && gearPos > 0.0f && 
				!platform->SoundPos.IsPlaying(auxaeroData->sndGearOpenStart)
				)
			{
				platform->SoundPos.Sfx( auxaeroData->sndGearOpenStart );
			}

		 	if ( gearPos > 0.90f && gearPos < 1.0f && 
				!platform->SoundPos.IsPlaying(auxaeroData->sndGearOpenEnd)
				)
			{
				platform->SoundPos.Sfx( auxaeroData->sndGearOpenEnd );
			}
		}

	 	if ( gearPos > 0.05f && gearPos < 0.95f )
			platform->SoundPos.Sfx( auxaeroData->sndGearLoop);
   }

	// JB carrier start
	if (hookHandle != 0)
	{
		hookPos += 0.3F * dt * hookHandle;
		hookPos = min ( max ( hookPos, 0.0F), 1.0F);

		// do sound effect.  This has begin, end and loop pieces
		if ( hookHandle < 0.0f )
		{
			// closing hook
			if ( hookPos > 0.90f && hookPos < 1.0f && 
				!platform->SoundPos.IsPlaying(auxaeroData->sndHookEnd)
				)
			{
				platform->SoundPos.Sfx( auxaeroData->sndHookEnd );
			}

			if ( hookPos < 0.10f && hookPos > 0.0f && 
				!platform->SoundPos.IsPlaying(auxaeroData->sndHookStart)
				)
			{
				platform->SoundPos.Sfx( auxaeroData->sndHookStart );
			}
		}
		else
		{
			// opening hook
			if ( hookPos < 0.10f && hookPos > 0.0f && 
				!platform->SoundPos.IsPlaying(auxaeroData->sndHookStart)
				)
			{
				platform->SoundPos.Sfx( auxaeroData->sndHookStart);
			}

			if ( hookPos > 0.90f && hookPos < 1.0f && 
				!platform->SoundPos.IsPlaying(auxaeroData->sndHookEnd)
				)
			{
				platform->SoundPos.Sfx( auxaeroData->sndHookEnd );
			}
		}

		if ( hookPos > 0.05f && hookPos < 0.95f )
			platform->SoundPos.Sfx( auxaeroData->sndHookLoop);
	}
	// JB carrier end

   //DSP hack until we can get the digi's to stay slow until the gear come up
   if (!platform->IsSetFalcFlag(FEC_INVULNERABLE) && gearPos > 0.1F &&  !IsSet( GearBroken ) && !IsSet(IsDigital))
   {
			if (gearPos > 0.9F)
				maxQbar = 350.0F;
			else
				maxQbar = 300.0F;

			maxQbar += (IsSet(Simplified)*100.0F);

			if (qbar > maxQbar && ((AircraftClass*)platform)->mFaults )
			{
					probability = (qbar - maxQbar)/75.0F*dt;
					chance = (float)rand()/(float)RAND_MAX;

					if ( chance < probability)
					{
						int i, numDebris=0;
						if ( probability > 3.0F*dt)
						{	   	   
							((AircraftClass*)platform)->mFaults->SetFault(FaultClass::gear_fault,
								FaultClass::ldgr, FaultClass::fail, TRUE);
						
							numDebris = rand()%4 + 5;
							
							gearPos = 0.1F;
							for(int i = 0; i < NumGear(); i++){
								gear[i].flags |= GearData::GearProblem;
								gear[i].flags |= GearData::DoorBroken;
								if(platform->IsComplex()){
										platform->SetDOF(ComplexGearDOF[i] /*COMP_NOS_GEAR + i */, 0.0F);
								}
							}
				
						SetFlag (GearBroken);
						ClearFlag (WheelBrakes);
						// gear breaks sound
						platform->SoundPos.Sfx( auxaeroData->sndWheelBrakes );

						// gear breaks sound
						platform->SoundPos.Sfx( SFX_GRNDHIT2 );
					} 
			   else if(NumGear() > 1)
					{
					// MLR 2/22/2004 - BEGIN MY CHANGES!
						float dmg = (qbar - maxQbar)/3.0F + rand()%26;

						int l, 
							g = NumGear();
					   
						int what = rand()%2;
						int which = rand()%g;
						float newpos;
					   
						numDebris = rand()%2;
					   
						switch(what)
						{
						case 0: // gear
							if (platform->IsComplex())
								newpos = platform->GetDOFValue(ComplexGearDOF[which]) - (float)rand()/(float)RAND_MAX * 5.0F*DTR;
						   
							gear[which].strength -= dmg;
							if(gear[which].strength <= 0.0F)
							{
								gear[which].flags |= GearData::GearBroken;
								if(platform->IsComplex())
									platform->SetDOF(ComplexGearDOF[which], 0.0F);
							   
								platform->SoundPos.Sfx( auxaeroData->sndWheelBrakes );
							   
								SetFlag (GearDamaged);
								((AircraftClass*)platform)->mFaults->SetFault(FaultClass::gear_fault,
									FaultClass::ldgr, FaultClass::fail, TRUE);
							}
							else 
							{
								if(gear[which].strength < 50.0F)
								{
									gear[which].flags |= GearData::GearStuck;
									platform->mFaults->SetFault(FaultClass::gear_fault,
										FaultClass::ldgr, FaultClass::fail, FALSE);
								}
							}
						   
							if(newpos > 20.0F*DTR && platform->IsComplex())
								platform->SetDOF(ComplexGearDOF[which], newpos);

							break;

						case 1: // door
							newpos = ((float)rand()/(float)RAND_MAX * 50.0F + 40.0F)*DTR;
						   
							if(dmg > 25.0F + rand()%5)
								gear[which].flags |= GearData::DoorBroken;
							else if(dmg > 15.0F + rand()%5)
								gear[which].flags |= GearData::DoorStuck;

							if(platform->IsComplex() && newpos > platform->GetDOFValue(ComplexGearDOF[which]))
								platform->SetDOF(ComplexGearDoorDOF[which], newpos);
							break;
						}
					   
					   
						for(l = 0; l < g; l++)
						{
							if(gear[l].flags & GearData::GearBroken)
								{
								((AircraftClass*)platform)->mFaults->SetFault(FaultClass::gear_fault,
									FaultClass::ldgr, 
									FaultClass::fail, 
									TRUE);
									SetFlag (GearBroken);
								}
							
								if(	gear[l].flags & GearData::GearStuck)
								{
									((AircraftClass*)platform)->mFaults->SetFault(	FaultClass::gear_fault,
																					FaultClass::ldgr, 
																					FaultClass::fail, 
																					TRUE);					   
								}
						}

						// MLR 2/22/2004 - REMOVED OBSOLETE CODE


						// gear breaks sound
						switch(rand()%3)
						{
						case 0:
							platform->SoundPos.Sfx( SFX_GROUND_CRUNCH );
							break;
						case 1:
							platform->SoundPos.Sfx( SFX_HIT_1 );
							break;
						case 2:
							platform->SoundPos.Sfx( SFX_HIT_5 );
							break;
						}
					}

						if(numDebris)
						{
							Tpoint pos, vec;
		   
							// for now, we'll scatter some debris -- need BSP for wheels?
							pos.x = x;
							pos.y = y;
							pos.z = z + 3.0f;
							vec.z = zdot;
							for ( i = 0; i < numDebris; i++ )
							{
								vec.x = xdot + PRANDFloat() * 30.0f;
								vec.y = ydot + PRANDFloat() * 30.0f;
								/*
								OTWDriver.AddSfxRequest(
									new SfxClass( SFX_AC_DEBRIS,		// type
									SFX_MOVES | SFX_USES_GRAVITY | SFX_BOUNCES,
									&pos,					// world pos
									&vec,					// vel vector
									3.0f,					// time to live
									1.0f ) );				// scale
									*/
								DrawableParticleSys::PS_AddParticleEx((SFX_AC_DEBRIS + 1),
														&pos,
														&vec);
							}
						}
          }
      }
   }

   /*-------------------------*/
   /* compute thrust fraction */
   /*-------------------------*/
   anozl  = 1.0F - athrev;
   ethrst = anozl - athrev * 0.8666F;

   //01/14/03 TJL Multi-engine
   anoz2 = 1.0F - athrev2;
   ethrst2 = anoz2 - athrev2 * 0.8666F;
}

 // MLR 2/22/2004 - OBSOLETE CODE
#if 0
				   //we'll just bend or stick something
				   int which = rand()%6;
				   float newpos;
				
				   numDebris = rand()%2;			
				   
				   float dmg = (qbar - maxQbar)/3.0F + rand()%26;

				   switch(which)
				   {
				   case 0:
					//nose gear
				       if (platform->IsComplex())
					   newpos = platform->GetDOFValue(COMP_NOS_GEAR) - (float)rand()/(float)RAND_MAX * 5.0F*DTR;

					   gear[0].strength -= dmg;
					   if(gear[0].strength <= 0.0F)
					   {
							gear[0].flags |= GearData::GearBroken;
							if(platform->IsComplex())
							    platform->SetDOF(COMP_NOS_GEAR, 0.0F);
							platform->SoundPos.Sfx( auxaeroData->sndWheelBrakes );
							SetFlag (GearDamaged);
							((AircraftClass*)platform)->mFaults->SetFault(FaultClass::gear_fault,
								FaultClass::ldgr, FaultClass::fail, TRUE);
					   }
					   else if(gear[0].strength < 50.0F)
					   {
							gear[0].flags |= GearData::GearStuck;
							platform->mFaults->SetFault(FaultClass::gear_fault,
								FaultClass::ldgr, FaultClass::fail, FALSE);
					   }

					   if(newpos > 20.0F*DTR && platform->IsComplex())
							platform->SetDOF(COMP_NOS_GEAR, newpos);
					   break;
				   case 1:
					//left gear
				       if(platform->IsComplex()) {
					       newpos = platform->GetDOFValue(COMP_LT_GEAR) + (float)rand()/(float)RAND_MAX * 5.0F*DTR;


					   if( newpos < (GetAeroData(AeroDataSet::LtGearRng) + 15.0F)*DTR )
							platform->SetDOF(COMP_LT_GEAR, newpos);
				       }

					   gear[1].strength -= dmg;
					   if(gear[1].strength <= 0.0F)
					   {
					       if(platform->IsComplex())
						   platform->SetDOF(COMP_LT_GEAR, 0.0F);
							gear[1].flags |= GearData::GearBroken;
							platform->SoundPos.Sfx( auxaeroData->sndWheelBrakes );
							SetFlag (GearDamaged);
							((AircraftClass*)platform)->mFaults->SetFault(FaultClass::gear_fault,
								FaultClass::ldgr, FaultClass::fail, TRUE);
					   }
					   else if(gear[1].strength < 50.0F)
					   {
							gear[1].flags |= GearData::GearStuck;
							platform->mFaults->SetFault(FaultClass::gear_fault,
								FaultClass::ldgr, FaultClass::fail, FALSE);
					   }

					   if(platform->IsComplex() && platform->GetDOFValue(COMP_LT_GEAR) < platform->GetDOFValue(COMP_LT_GEAR_DR))
					   {
							platform->SetDOF(COMP_LT_GEAR_DR, max(platform->GetDOFValue(COMP_LT_GEAR) + (float)rand()/(float)RAND_MAX * 10.0F*DTR, 0.0F));
					   }
					   break;
				   case 2:
					//right gear
				       if (platform->IsComplex()) {
					   newpos = platform->GetDOFValue(COMP_RT_GEAR) + (float)rand()/(float)RAND_MAX * 5.0F*DTR;
					   
					   if( newpos < (GetAeroData(AeroDataSet::RtGearRng) + 15.0F)*DTR )
					       platform->SetDOF(COMP_RT_GEAR, newpos);
				       }
					   gear[2].strength -= dmg;
					   if(gear[2].strength <= 0.0F)
					   {
					       if(platform->IsComplex())
						   platform->SetDOF(COMP_RT_GEAR, 0.0F);
					       gear[2].flags |= GearData::GearBroken;
						   platform->SoundPos.Sfx( auxaeroData->sndWheelBrakes);
					       SetFlag (GearDamaged);
					       ((AircraftClass*)platform)->mFaults->SetFault(FaultClass::gear_fault,
						   FaultClass::ldgr, FaultClass::fail, TRUE);
					   }
					   else if(gear[2].strength < 50.0F)
					   {
							gear[2].flags |= GearData::GearStuck;
							platform->mFaults->SetFault(FaultClass::gear_fault,
								FaultClass::ldgr, FaultClass::fail, FALSE);
					   }

					   if(platform->IsComplex() && platform->GetDOFValue(COMP_RT_GEAR_DR) < platform->GetDOFValue(COMP_RT_GEAR))
					   {
							platform->SetDOF(COMP_RT_GEAR_DR, max(platform->GetDOFValue(COMP_RT_GEAR) + (float)rand()/(float)RAND_MAX * 10.0F*DTR, 0.0F));
					   }
					   break;
				   case 3:
					//nose door
					   newpos = ((float)rand()/(float)RAND_MAX * 50.0F + 40.0F)*DTR;
					   
					   if(dmg > 25.0F + rand()%5)
						   gear[0].flags |= GearData::DoorBroken;
					   else if(dmg > 15.0F + rand()%5)
						   gear[0].flags |= GearData::DoorStuck;

					   if(platform->IsComplex() && newpos > platform->GetDOFValue(COMP_NOS_GEAR))
							platform->SetDOF(COMP_NOS_GEAR_DR, newpos);
					   break;
				   case 4:
					//left door
					   newpos = ((float)rand()/(float)RAND_MAX * 50.0F + 25.0F)*DTR;

					   if(dmg > 25.0F + rand()%5)
						   gear[1].flags |= GearData::DoorBroken;
					   else if(dmg > 15.0F + rand()%5)
						   gear[1].flags |= GearData::DoorStuck;

					   if(platform->IsComplex() && newpos > platform->GetDOFValue(COMP_NOS_GEAR))
							platform->SetDOF(COMP_LT_GEAR_DR, newpos);
					   break;
				   case 5:
					//right door
					   newpos = ((float)rand()/(float)RAND_MAX * 50.0F + 25.0F)*DTR;

					   if(dmg > 25.0F + rand()%5)
						   gear[2].flags |= GearData::DoorBroken;
					   else if(dmg > 15.0F + rand()%5)
						   gear[2].flags |= GearData::DoorStuck;

					   if(platform->IsComplex() && newpos > platform->GetDOFValue(COMP_RT_GEAR))
							platform->SetDOF(COMP_RT_GEAR_DR, newpos);
					   break;
				   default:
					   break;
				   }

				   if(	gear[0].flags & GearData::GearBroken && 
						gear[1].flags & GearData::GearBroken &&
						gear[2].flags & GearData::GearBroken)
				   {
					   ((AircraftClass*)platform)->mFaults->SetFault(FaultClass::gear_fault,
							FaultClass::ldgr, FaultClass::fail, TRUE);
					   SetFlag (GearBroken);
				   }

				   if(	gear[0].flags & GearData::GearStuck && 
						gear[1].flags & GearData::GearStuck &&
						gear[2].flags & GearData::GearStuck)
				   {
					   ((AircraftClass*)platform)->mFaults->SetFault(FaultClass::gear_fault,
							FaultClass::ldgr, FaultClass::fail, TRUE);					   
				   }
#endif
