/***************************************************************************\
	Statistics.cpp
    Scott Randolph
    June 18, 1998

	This module encapsulates some simple statistics gathering which can
	be called from TheSimLoop while the graphics are running.
\***************************************************************************/
#include "stdhdr.h"
#include "Battalion.H"
#include "Brigade.H"
#include "Navunit.H"
#include "Objectiv.H"
#include "Package.H"
#include "Squadron.H"
#include "Flight.H"
#include "Graphics\Include\DRAWBLDG.H"
#include "Graphics\Include\DRAWBRDG.H"
#include "Graphics\Include\DRAWGRND.H"
#include "Graphics\Include\DRAWOVC.H"
#include "Graphics\Include\DRAWPLAT.H"
#include "Graphics\Include\DRAWRDBD.H"
#include "Graphics\Include\DRAWBSP.H"
#include "Graphics\Include\DRAWSGMT.H"
#include "Graphics\Include\DRAWTRCR.H"
#include "Graphics\Include\DRAW2D.H"
#include "Graphics\Include\DRAWPUFF.H"
#include "Graphics\Include\DRAWSHDW.H"
#include "Graphics\Include\TBLKLIST.H"
#include "Graphics\Include\TBLOCK.H"
#include "BOMB.H"
#include "chaff.H"
#include "flare.H"
#include "debris.H"
#include "GROUND.H"
#include "GUNS.H"
#include "MISSILE.H"
#include "OBJECT.H"
#include "AIRCRFT.H"
#include "GNDAI.H"
#include "SIMFEAT.H"
#include "SFX.H"
#include "airframe.h"
#include "fsound.h"
#include "falcsnd\LHSP.h"
#include "falcsnd\FalcVoice.h"
#include "falcsnd\voicemanager.h"
#include "Statistics.h"
#include "persist.h"


BOOL	log_frame_rate = FALSE;


// Gather interesting statistics.  Not required for release versions
static char
	buffer[100];

static int
	frame_count = 0,
	frame_time[100],
	last_time,
	num_frames = 0,
	cur_time,
	avg_time,
	min_time,
	max_time,
	total_time;

static FILE
	*fp,
	*fp1;


extern int gTotSfx;
extern int numObjsProcessed;
extern int numObjsInDrawList;


void InitializeStatistics(void)
{
	for (int loop = 0; loop < 100; loop ++)
	{
		frame_time[loop] = 0;
	}

	if (log_frame_rate)
	{
		fp = fopen ("framerate.csv", "w");
		fp1 = fopen ("fr_summary.csv", "w");

		sprintf (buffer, "Rate, Max, Min, Avg, Sfx, Proc, Draw\n\n");

		fwrite (buffer, strlen (buffer), 1, fp);
		fwrite (buffer, strlen (buffer), 1, fp1);

		fflush (fp);
		fflush (fp1);

		last_time = timeGetTime ();
	}

}


void CloseStatistics(void)
{
	if (log_frame_rate)
	{
		fclose (fp);
		fclose (fp1);
	}
}


void WriteStatistics(void)
{
	if (log_frame_rate)
	{
		cur_time = timeGetTime ();
		frame_time[frame_count] = cur_time - last_time;
		last_time = cur_time;

		if (num_frames < 100)
		{
			num_frames ++;
		}

		min_time = 100000;
		max_time = 0;
		total_time = 0;

		for (int loop = 0; loop < num_frames; loop ++)
		{
			if (frame_time[loop] < min_time)
			{
				min_time = frame_time[loop];
			}

			if (frame_time[loop] > max_time)
			{
				max_time = frame_time[loop];
			}

			total_time += frame_time[loop];
		}

		avg_time = total_time / num_frames;

		sprintf(
			buffer,
			"%f, %f, %f, %f, %d, %d, %d\n",
			1000.0 / frame_time[frame_count],
			1000.0 / min_time,
			1000.0 / max_time,
			1000.0 / avg_time,
			gTotSfx,
			numObjsProcessed,
			numObjsInDrawList
		);

		fwrite (buffer, strlen (buffer), 1, fp);

		frame_count ++;

		if (frame_count >= 100)
		{
			MonoPrint (buffer);

			fwrite (buffer, strlen (buffer), 1, fp1);

			frame_count = 0;

			fflush (fp);
			fflush (fp1);
		}
	}
}

void PrintMemUsage (void)
{

#ifdef USE_SH_POOLS
int simTotal, campTotal, graphTotal,soundTotal;
   MonoLocate (0,0);
   MonoCls();
   MonoPrint ("Objectives %5d:%5d    Squadrons   %5d:%5d\n",
      MemPoolCount(ObjectiveClass::pool), MemPoolSize(ObjectiveClass::pool) / 1024,
      MemPoolCount(SquadronClass::pool), MemPoolSize(SquadronClass::pool) / 1024);
   MonoPrint ("Battalion  %5d:%5d    Flight      %5d:%5d\n",
      MemPoolCount(BattalionClass::pool), MemPoolSize(BattalionClass::pool) / 1024,
      MemPoolCount(FlightClass::pool), MemPoolSize(FlightClass::pool) / 1024);
   MonoPrint ("Brigade    %5d:%5d    TaskForce   %5d:%5d\n",
      MemPoolCount(BrigadeClass::pool), MemPoolSize(BrigadeClass::pool) / 1024,
      MemPoolCount(TaskForceClass::pool), MemPoolSize(TaskForceClass::pool) / 1024);
   MonoPrint ("Package    %5d:%5d    Tracer      %5d:%5d\n",
      MemPoolCount(PackageClass::pool), MemPoolSize(PackageClass::pool) / 1024,
      MemPoolCount(DrawableTracer::pool), MemPoolSize(DrawableTracer::pool) / 1024);
   MonoPrint ("Building   %5d:%5d    Bridge      %5d:%5d\n",
      MemPoolCount(DrawableBuilding::pool), MemPoolSize(DrawableBuilding::pool) / 1024,
      MemPoolCount(DrawableBridge::pool), MemPoolSize(DrawableBridge::pool) / 1024);
   MonoPrint ("GroundVeh  %5d:%5d    Platform    %5d:%5d\n",
      MemPoolCount(DrawableGroundVehicle::pool), MemPoolSize(DrawableGroundVehicle::pool) / 1024,
      MemPoolCount(DrawablePlatform::pool), MemPoolSize(DrawablePlatform::pool) / 1024);
   MonoPrint ("Roadbed    %5d:%5d    BSP         %5d:%5d\n",
      MemPoolCount(DrawableRoadbed::pool), MemPoolSize(DrawableRoadbed::pool) / 1024,
      MemPoolCount(DrawableBSP::pool), MemPoolSize(DrawableBSP::pool) / 1024);
   MonoPrint ("Trail      %5d:%5d    Overcast    %5d:%5d\n",
      MemPoolCount(DrawableTrail::pool), MemPoolSize(DrawableTrail::pool) / 1024,
      MemPoolCount(DrawableOvercast::pool), MemPoolSize(DrawableOvercast::pool) / 1024);
   MonoPrint ("Puff       %5d:%5d    Shadowed    %5d:%5d\n",
      MemPoolCount(DrawablePuff::pool), MemPoolSize(DrawablePuff::pool) / 1024,
      MemPoolCount(DrawableShadowed::pool), MemPoolSize(DrawableShadowed::pool) / 1024);
   MonoPrint ("2D         %5d:%5d\n",
      MemPoolCount(Drawable2D::pool), MemPoolSize(Drawable2D::pool) / 1024);
   MonoPrint ("Trail Elmt %5d:%5d    TListEntry  %5d:%5d\n",
      MemPoolCount(TrailElement::pool), MemPoolSize(TrailElement::pool) / 1024,
      MemPoolCount(TListEntry::pool), MemPoolSize(TListEntry::pool) / 1024);
   MonoPrint ("T Block    %5d:%5d    TBlock List %5d:%5d\n",
      MemPoolCount(TBlock::pool), MemPoolSize(TBlock::pool) / 1024,
      MemPoolCount(TBlockList::pool), MemPoolSize(TBlockList::pool) / 1024);
   MonoPrint ("Bomb       %5d:%5d    Chaff %5d:%5d\n",
      MemPoolCount(BombClass::pool), MemPoolSize(BombClass::pool) / 1024,
      MemPoolCount(ChaffClass::pool), MemPoolSize(ChaffClass::pool) / 1024);
   MonoPrint ("Flare      %5d:%5d    Debris %5d:%5d\n",
      MemPoolCount(FlareClass::pool), MemPoolSize(FlareClass::pool) / 1024,
      MemPoolCount(DebrisClass::pool), MemPoolSize(DebrisClass::pool) / 1024);
   MonoPrint ("                :         Sim Feature %5d:%5d\n",
      MemPoolCount(SimFeatureClass::pool), MemPoolSize(SimFeatureClass::pool) / 1024);
   MonoPrint ("Sim Grnd   %5d:%5d    Grnd AI     %5d:%5d\n",
      MemPoolCount(GroundClass::pool), MemPoolSize(GroundClass::pool) / 1024,
      MemPoolCount(GNDAIClass::pool), MemPoolSize(GNDAIClass::pool) / 1024);
   MonoPrint ("Sim Gun    %5d:%5d    Sim Missile %5d:%5d\n",
      MemPoolCount(GunClass::pool), MemPoolSize(GunClass::pool) / 1024,
      MemPoolCount(MissileClass::pool), MemPoolSize(MissileClass::pool) / 1024);
   MonoPrint ("SFX        %5d:%5d    Aircraft    %5d:%5d\n",
      MemPoolCount(SfxClass::pool), MemPoolSize(SfxClass::pool) / 1024,
      MemPoolCount(AircraftClass::pool), MemPoolSize(AircraftClass::pool) / 1024);
   MonoPrint ("Airframe   %5d:%5d    Aero Data   %5d:%5d\n",
      MemPoolCount(AirframeClass::pool), MemPoolSize(AirframeClass::pool) / 1024,
      MemPoolCount(AirframeDataPool), MemPoolSize(AirframeDataPool) / 1024);
   MonoPrint ("SimObject  %5d:%5d    Sim Local   %5d:%5d\n",
      MemPoolCount(SimObjectType::pool), MemPoolSize(SimObjectType::pool) / 1024,
      MemPoolCount(SimObjectLocalData::pool), MemPoolSize(SimObjectLocalData::pool) / 1024);
   MonoPrint ("Default    %5d:%5d    ConvList   %5d:%5d\n",
      MemPoolCount(MemDefaultPool), MemPoolSize(MemDefaultPool) / 1024,
	  MemPoolCount(VM_CONVLIST::pool),MemPoolSize(VM_CONVLIST::pool)/1024);
   MonoPrint ("Conversations    %5d:%5d   VoiceBuffers    %5d:%5d \n",
      MemPoolCount(CONVERSATION::pool), MemPoolSize(CONVERSATION::pool)/1024,
	  MemPoolCount(VM_BUFFLIST::pool), MemPoolSize(VM_BUFFLIST::pool) / 1024);
   MonoPrint ("Persistant %5d:%5d\n",
      MemPoolCount(SimPersistantClass::pool), MemPoolSize(SimPersistantClass::pool) / 1024);

   simTotal = MemPoolSize(BombClass::pool) +
      MemPoolSize(ChaffClass::pool) +
      MemPoolSize(FlareClass::pool) +
      MemPoolSize(DebrisClass::pool) +
      MemPoolSize(SimFeatureClass::pool) +
      MemPoolSize(GroundClass::pool) +
      MemPoolSize(GNDAIClass::pool) +
      MemPoolSize(GunClass::pool) +
      MemPoolSize(MissileClass::pool) +
      MemPoolSize(AircraftClass::pool) +
      MemPoolSize(AirframeClass::pool) +
      MemPoolSize(SimObjectType::pool) +
      MemPoolSize(SimObjectLocalData::pool) +
      MemPoolSize(AirframeDataPool);
   simTotal /= 1024;

   campTotal = MemPoolSize(ObjectiveClass::pool) +
      MemPoolSize(SquadronClass::pool) +
      MemPoolSize(BattalionClass::pool) +
      MemPoolSize(FlightClass::pool) +
      MemPoolSize(BrigadeClass::pool) +
      MemPoolSize(TaskForceClass::pool) +
      MemPoolSize(PackageClass::pool) +
	  MemPoolSize(SimPersistantClass::pool);
   campTotal /= 1024;

   graphTotal = MemPoolSize(DrawableTracer::pool) +
      MemPoolSize(DrawableBuilding::pool) +
      MemPoolSize(DrawableBridge::pool) +
      MemPoolSize(DrawableGroundVehicle::pool) +
      MemPoolSize(DrawablePlatform::pool) +
      MemPoolSize(DrawableRoadbed::pool) +
      MemPoolSize(DrawableBSP::pool) +
      MemPoolSize(DrawableTrail::pool) +
      MemPoolSize(DrawableOvercast::pool) +
      MemPoolSize(DrawablePuff::pool) +
      MemPoolSize(DrawableShadowed::pool) +
      MemPoolSize(Drawable2D::pool) +
      MemPoolSize(TrailElement::pool) +
      MemPoolSize(TListEntry::pool) +
      MemPoolSize(TBlock::pool) +
      MemPoolSize(TBlockList::pool) +
      MemPoolSize(SfxClass::pool);
   graphTotal /= 1024;


   soundTotal = MemPoolSize(CONVERSATION::pool) +
				MemPoolSize(VM_BUFFLIST::pool) +
   				MemPoolSize(VM_CONVLIST::pool);
   soundTotal/=1024;


   MonoPrint ("Sim Pools = %7d Campaign Pools = %7d Graphics Pools = %7d\n SoundPools = %7d",
      simTotal, campTotal, graphTotal,soundTotal);
#endif
}

void WriteMemUsage (void)
{
#ifdef USE_SH_POOLS
	int		simTotal, campTotal, graphTotal,soundTotal;
	FILE	*fp;

	fp = fopen ("memusage.log", "w");

   fprintf (fp, "Objectives %5d:%5d    Squadrons   %5d:%5d\n",
      MemPoolCount(ObjectiveClass::pool), MemPoolSize(ObjectiveClass::pool) / 1024,
      MemPoolCount(SquadronClass::pool), MemPoolSize(SquadronClass::pool) / 1024);
   fprintf (fp, "Battalion  %5d:%5d    Flight      %5d:%5d\n",
      MemPoolCount(BattalionClass::pool), MemPoolSize(BattalionClass::pool) / 1024,
      MemPoolCount(FlightClass::pool), MemPoolSize(FlightClass::pool) / 1024);
   fprintf (fp, "Brigade    %5d:%5d    TaskForce   %5d:%5d\n",
      MemPoolCount(BrigadeClass::pool), MemPoolSize(BrigadeClass::pool) / 1024,
      MemPoolCount(TaskForceClass::pool), MemPoolSize(TaskForceClass::pool) / 1024);
   fprintf (fp, "Package    %5d:%5d    Tracer      %5d:%5d\n",
      MemPoolCount(PackageClass::pool), MemPoolSize(PackageClass::pool) / 1024,
      MemPoolCount(DrawableTracer::pool), MemPoolSize(DrawableTracer::pool) / 1024);
   fprintf (fp, "Building   %5d:%5d    Bridge      %5d:%5d\n",
      MemPoolCount(DrawableBuilding::pool), MemPoolSize(DrawableBuilding::pool) / 1024,
      MemPoolCount(DrawableBridge::pool), MemPoolSize(DrawableBridge::pool) / 1024);
   fprintf (fp, "GroundVeh  %5d:%5d    Platform    %5d:%5d\n",
      MemPoolCount(DrawableGroundVehicle::pool), MemPoolSize(DrawableGroundVehicle::pool) / 1024,
      MemPoolCount(DrawablePlatform::pool), MemPoolSize(DrawablePlatform::pool) / 1024);
   fprintf (fp, "Roadbed    %5d:%5d    BSP         %5d:%5d\n",
      MemPoolCount(DrawableRoadbed::pool), MemPoolSize(DrawableRoadbed::pool) / 1024,
      MemPoolCount(DrawableBSP::pool), MemPoolSize(DrawableBSP::pool) / 1024);
   fprintf (fp, "Trail      %5d:%5d    Overcast    %5d:%5d\n",
      MemPoolCount(DrawableTrail::pool), MemPoolSize(DrawableTrail::pool) / 1024,
      MemPoolCount(DrawableOvercast::pool), MemPoolSize(DrawableOvercast::pool) / 1024);
   fprintf (fp, "Puff       %5d:%5d    Shadowed    %5d:%5d\n",
      MemPoolCount(DrawablePuff::pool), MemPoolSize(DrawablePuff::pool) / 1024,
      MemPoolCount(DrawableShadowed::pool), MemPoolSize(DrawableShadowed::pool) / 1024);
   fprintf (fp, "2D         %5d:%5d\n",
      MemPoolCount(Drawable2D::pool), MemPoolSize(Drawable2D::pool) / 1024);
   fprintf (fp, "Trail Elmt %5d:%5d    TListEntry  %5d:%5d\n",
      MemPoolCount(TrailElement::pool), MemPoolSize(TrailElement::pool) / 1024,
      MemPoolCount(TListEntry::pool), MemPoolSize(TListEntry::pool) / 1024);
   fprintf (fp, "T Block    %5d:%5d    TBlock List %5d:%5d\n",
      MemPoolCount(TBlock::pool), MemPoolSize(TBlock::pool) / 1024,
      MemPoolCount(TBlockList::pool), MemPoolSize(TBlockList::pool) / 1024);
   fprintf (fp, "Bomb       %5d:%5d    Sim Feature %5d:%5d\n",
      MemPoolCount(BombClass::pool), MemPoolSize(BombClass::pool) / 1024,
      MemPoolCount(SimFeatureClass::pool), MemPoolSize(SimFeatureClass::pool) / 1024);
   fprintf (fp, "Sim Grnd   %5d:%5d    Grnd AI     %5d:%5d\n",
      MemPoolCount(GroundClass::pool), MemPoolSize(GroundClass::pool) / 1024,
      MemPoolCount(GNDAIClass::pool), MemPoolSize(GNDAIClass::pool) / 1024);
   fprintf (fp, "Sim Gun    %5d:%5d    Sim Missile %5d:%5d\n",
      MemPoolCount(GunClass::pool), MemPoolSize(GunClass::pool) / 1024,
      MemPoolCount(MissileClass::pool), MemPoolSize(MissileClass::pool) / 1024);
   fprintf (fp, "SFX        %5d:%5d    Aircraft    %5d:%5d\n",
      MemPoolCount(SfxClass::pool), MemPoolSize(SfxClass::pool) / 1024,
      MemPoolCount(AircraftClass::pool), MemPoolSize(AircraftClass::pool) / 1024);
   fprintf (fp, "Airframe   %5d:%5d    Aero Data   %5d:%5d\n",
      MemPoolCount(AirframeClass::pool), MemPoolSize(AirframeClass::pool) / 1024,
      MemPoolCount(AirframeDataPool), MemPoolSize(AirframeDataPool) / 1024);
   fprintf (fp, "SimObject  %5d:%5d    Sim Local   %5d:%5d\n",
      MemPoolCount(SimObjectType::pool), MemPoolSize(SimObjectType::pool) / 1024,
      MemPoolCount(SimObjectLocalData::pool), MemPoolSize(SimObjectLocalData::pool) / 1024);
   fprintf (fp, "Default    %5d:%5d    ConvList   %5d:%5d\n",
      MemPoolCount(MemDefaultPool), MemPoolSize(MemDefaultPool) / 1024,
	  MemPoolCount(VM_CONVLIST::pool),MemPoolSize(VM_CONVLIST::pool)/1024);
   fprintf (fp, "Conversations    %5d:%5d   VoiceBuffers    %5d:%5d \n",
      MemPoolCount(CONVERSATION::pool), MemPoolSize(CONVERSATION::pool)/1024,
	  MemPoolCount(VM_BUFFLIST::pool), MemPoolSize(VM_BUFFLIST::pool) / 1024);
   fprintf (fp, "Persistant %5d:%5d\n",
      MemPoolCount(SimPersistantClass::pool), MemPoolSize(SimPersistantClass::pool) / 1024);

   simTotal = MemPoolSize(BombClass::pool) +
      MemPoolSize(SimFeatureClass::pool) +
      MemPoolSize(GroundClass::pool) +
      MemPoolSize(GNDAIClass::pool) +
      MemPoolSize(GunClass::pool) +
      MemPoolSize(MissileClass::pool) +
      MemPoolSize(AircraftClass::pool) +
      MemPoolSize(AirframeClass::pool) +
      MemPoolSize(SimObjectType::pool) +
      MemPoolSize(SimObjectLocalData::pool) +
      MemPoolSize(AirframeDataPool);
   simTotal /= 1024;

   campTotal = MemPoolSize(ObjectiveClass::pool) +
      MemPoolSize(SquadronClass::pool) +
      MemPoolSize(BattalionClass::pool) +
      MemPoolSize(FlightClass::pool) +
      MemPoolSize(BrigadeClass::pool) +
      MemPoolSize(TaskForceClass::pool) +
      MemPoolSize(PackageClass::pool) +
	  MemPoolSize(SimPersistantClass::pool);
   campTotal /= 1024;

   graphTotal = MemPoolSize(DrawableTracer::pool) +
      MemPoolSize(DrawableBuilding::pool) +
      MemPoolSize(DrawableBridge::pool) +
      MemPoolSize(DrawableGroundVehicle::pool) +
      MemPoolSize(DrawablePlatform::pool) +
      MemPoolSize(DrawableRoadbed::pool) +
      MemPoolSize(DrawableBSP::pool) +
      MemPoolSize(DrawableTrail::pool) +
      MemPoolSize(DrawableOvercast::pool) +
      MemPoolSize(DrawablePuff::pool) +
      MemPoolSize(DrawableShadowed::pool) +
      MemPoolSize(Drawable2D::pool) +
      MemPoolSize(TrailElement::pool) +
      MemPoolSize(TListEntry::pool) +
      MemPoolSize(TBlock::pool) +
      MemPoolSize(TBlockList::pool) +
      MemPoolSize(SfxClass::pool);
   graphTotal /= 1024;


   soundTotal = MemPoolSize(CONVERSATION::pool) +
				MemPoolSize(VM_BUFFLIST::pool) +
   				MemPoolSize(VM_CONVLIST::pool);
   soundTotal/=1024;


   fprintf (fp, "Sim Pools = %7d Campaign Pools = %7d Graphics Pools = %7d\n SoundPools = %7d",
      simTotal, campTotal, graphTotal,soundTotal);
   fclose (fp);

#endif
}

