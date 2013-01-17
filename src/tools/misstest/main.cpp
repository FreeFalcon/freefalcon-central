#include "stdhdr.h"
#include "misslist.h"
#include "missile.h"
#include "acmi\src\include\acmirec.h"
#include "otwdrive.h"
#include "simdrive.h"
#include "sfx.h"
#include "terrain\tviewpnt.h"
#include "mvrdef.h"
#include "f4find.h"
extern "C" {
#include "codelib\resources\reslib\src\resmgr.h"
}
#include "classtbl.h"
#include "entity.h"
#include "object.h"
#include "playerop.h"

MissileClass *theMissile = NULL;
FILE* OpenCampFile (char *filename, char *ext, char *mode);
CalcPressureRatio(float alt, float* ttheta, float* rsigma);
void CalcTransformMatrix (SimBaseClass* theObject);
extern VuAntiDatabase *vuAntiDB;

void RunMissile (void);
void LaunchMissile (void);
void UpdateTarget (void);
void ReadAllMissileData(void);
SimObjectType* targetPtr;
SimVehicleClass* parent;
float startRange;

typedef struct
{
	char *filename;
	int missileType;
	BOOL fireFromSurface;
	BOOL targOnGround;
} MISSILE_TYPE_FILE_MAP;

// For missileType  See [Weapon Data].[Weapon Index] in the class table
MISSILE_TYPE_FILE_MAP gMissFiles[] =
{
	"aim9l.dat", 	2, 			FALSE,		FALSE,
	"aim120.dat", 	56, 		FALSE,		FALSE,
	"agm65b.dat", 	18, 		FALSE,		TRUE,
	"agm65d.dat", 	19, 		FALSE,		TRUE,
	"agm45.dat", 	17, 		FALSE,		TRUE,
	"agm88.dat", 	23, 		FALSE,		TRUE,
	"sa6.dat", 		39, 		TRUE,		FALSE,
	"2_75In.dat", 	71, 		FALSE,		TRUE,
	"agmXXX.dat", 	18, 		FALSE,		TRUE,
	"aim9p.dat", 	2, 			FALSE,		FALSE,
	"aim54.dat", 	9,	 		FALSE,		FALSE,
	"aa7.dat", 		53, 		FALSE,		FALSE,
	"aa7r.dat", 	11, 		FALSE,		FALSE,
	"aa9.dat", 		55, 		FALSE,		FALSE,
	"aa10.dat", 	7, 			FALSE,		FALSE,
	"aa10c.dat", 	8, 			FALSE,		FALSE,
	"aa11.dat", 	10, 		FALSE,		FALSE,
	"aim7.dat", 	1, 			FALSE,		FALSE,
	"sa7.dat", 		40, 		TRUE,		FALSE,
	"sa13.dat", 	36, 		TRUE,		FALSE,
	"sa9.dat", 		122, 		TRUE,		FALSE,
	"sa14.dat", 	134, 		TRUE,		FALSE,
	"sa2.dat", 		37, 		TRUE,		FALSE,
	"sa3.dat", 		116, 		TRUE,		FALSE,
	"sa4.dat", 		38, 		TRUE,		FALSE,
	"sa5.dat", 		117, 		TRUE,		FALSE,
	"sa8.dat", 		41, 		TRUE,		FALSE,
	"sa15.dat", 	123, 		TRUE,		FALSE,
};
int gNumMissiles = sizeof(gMissFiles)/sizeof(MISSILE_TYPE_FILE_MAP);

int main (void)
{
char tmpStr[1024];
int missileType;
int targetType;
int count;
int i, j, k, m;
float l;
float pdelta, ttheta, sound, rsigma;
FILE *fp = NULL;
int velMax, aspMax, aspStep, altStep;

   InitDebug (DEBUGGER_TEXT_MODE);
   F4GetRegistryString ("baseDir", FalconDataDirectory, sizeof (FalconDataDirectory));
	SetCurrentDirectory(FalconDataDirectory);
   ResInit(NULL);
   ResCreatePath (FalconDataDirectory, FALSE);
   ResAddPath (FalconCampaignSaveDirectory, FALSE);
   //Prep Object Data
   SimMoverDefinition::ReadSimMoverDefinitionData();
   ReadAllMissileData();
   LoadClassTable("Falcon4");
	LoadWeaponData("Falcon4");	
	LoadVehicleData("Falcon4");	
	if (!LoadRadarData("Falcon4"))				ShiError( "Failed to load radar data" );
//	if (!LoadIRSTData("Falcon4"))				ShiError( "Failed to load IRST data" );
	if (!LoadSimWeaponData("Falcon4"))			ShiError( "Failed to load SimWeapon data" );


   // Data fixup
	for (i=0; i<NumEntities; i++)
	{
      if (Falcon4ClassTable[i].dataPtr != NULL)
      {
	      if (Falcon4ClassTable[i].dataType == DTYPE_WEAPON)
	      {
		      WeaponDataTable[(int)Falcon4ClassTable[i].dataPtr].Index = i;
		      Falcon4ClassTable[i].dataPtr = (void*) &WeaponDataTable[(int)Falcon4ClassTable[i].dataPtr];
	      }
			else if (Falcon4ClassTable[i].dataType == DTYPE_VEHICLE)
			{
				VehicleDataTable[(int)Falcon4ClassTable[i].dataPtr].Index = i;
				Falcon4ClassTable[i].dataPtr = (void*) &VehicleDataTable[(int)Falcon4ClassTable[i].dataPtr];
			}
      }
   }

   sprintf (tmpStr, "MISSILETEST\0");
   vuxWorldName = new char[strlen(tmpStr) + 1];
   strcpy (vuxWorldName, tmpStr);
   vuCritical = F4CreateCriticalSection();
   gMainThread = new VuMainThread (5000,NULL,1024,NULL);
	PlayerOptions.SimWeaponEffect = WEEnhanced;

   // creates global
   if (!vuxDriverSettings)
   {
      vuxDriverSettings = new VuDriverSettings(
         (SM_SCALAR)200.0, (SM_SCALAR)200.0, (SM_SCALAR)200.0, // gx, gy, gz tolerance
          (SM_SCALAR)0.0,  (SM_SCALAR)0.0,  (SM_SCALAR)0.0, // x, y, z tolerance
          (SM_SCALAR)0.0,  (SM_SCALAR)0.0,  (SM_SCALAR)0.0, // y, p, r tolerance
          (SM_SCALAR)1.0,  (SM_SCALAR)0.1,   // maxJumpDist, maxJumpAngle
		  2000);
//          1000);         // lookahead time (ms)
   }

   

   targetType = GetClassID (DOMAIN_AIR, CLASS_VEHICLE, TYPE_AIRPLANE, STYPE_AIR_FIGHTER_BOMBER,
			SPTYPE_F16C,   VU_ANY, VU_ANY, VU_ANY) + VU_LAST_ENTITY_TYPE;

   parent = new SimVehicleClass(targetType);
   VuReferenceEntity (parent);
   targetPtr = new SimObjectType (OBJ_TAG, parent, new SimBaseClass(targetType));
   targetPtr->Reference( SIM_OBJ_REF_ARGS );

   // Run a whole slew of cases. extend the range at each case until you miss
   // Initial Conditions
   parent->SetYPR (0.0F, 0.0F, 0.0F);
   parent->SetYPRDelta (0.0F, 0.0F, 0.0F);


   // main loop thru all the different missile types
   for ( m = 0; m < gNumMissiles; m++ )
   {
	   /*
	   ** for testing
	   if ( !gMissFiles[m].fireFromSurface )
	   		continue;
	   */


	   missileType = gMissFiles[m].missileType;
	   fprintf( stdout, "##################################### \n" );
	   fprintf( stdout, "GENERATING MISSILE TYPE %d\n", missileType );
	   fprintf( stdout, "##################################### \n\n\n" );


	   // open file for writing
	   fp = fopen( gMissFiles[m].filename, "w" );
	   if ( fp == NULL )
	   {
		   exit( -1 );
	   }

	   // write the header
	   fprintf( fp, "# \n" );
	   fprintf( fp, "# \n" );
	   fprintf( fp, "# Range Data (generated by Missile Test Program)\n" );
	   fprintf( fp, "# \n" );
	   fprintf( fp, "# \n" );
	   fprintf( fp, "\n" );
	   fprintf( fp, "# Table Multiplier\n" );
	   fprintf( fp, "1.0\n" );
	   fprintf( fp, "\n" );

	   fprintf( stdout, "# \n" );
	   fprintf( stdout, "# \n" );
	   fprintf( stdout, "# Range Data (generated by Missile Test Program)\n" );
	   fprintf( stdout, "# \n" );
	   fprintf( stdout, "# \n" );
	   fprintf( stdout, "\n" );
	   fprintf( stdout, "# Table Multiplier\n" );
	   fprintf( stdout, "1.0\n" );
	   fprintf( stdout, "\n" );


	   // velocities
	   if ( gMissFiles[m].fireFromSurface )
	   {
		   // altitudes
		   fprintf( fp, "# Number of altitude breakpoints:\n" );
		   fprintf( fp, "11 \n" );
		   fprintf( fp, "\n" );
		   fprintf( stdout, "# Number of altitude breakpoints:\n" );
		   fprintf( stdout, "11 \n" );
		   fprintf( stdout, "\n" );
	
		   fprintf( fp, "# Altitude breakpoints:\n" );
		   fprintf( fp, "0.0 5000.0 10000.0 15000.0 20000.0 25000.0 30000.0 35000.0 40000.0 45000.0 50000.0 \n" );
		   fprintf( fp, "\n" );
		   fprintf( stdout, "# Altitude breakpoints:\n" );
		   fprintf( stdout, "0.0 5000.0 10000.0 15000.0 20000.0 25000.0 30000.0 35000.0 40000.0 45000.0 50000.0 \n" );
		   fprintf( stdout, "\n" );

		   fprintf( fp, "# Number of Velocity breakpoints:\n" );
		   fprintf( fp, "1 \n" );
		   fprintf( fp, "\n" );
		   fprintf( stdout, "# Number of Velocity breakpoints:\n" );
		   fprintf( stdout, "1 \n" );
		   fprintf( stdout, "\n" );
	
		   fprintf( fp, "# Velocity breakpoints:\n" );
		   fprintf( fp, "%5.2f \n", 0.0f );
		   fprintf( fp, "\n" );
		   fprintf( stdout, "# Velocity breakpoints:\n" );
		   fprintf( stdout, "%5.2f  \n", 0.0f );
		   fprintf( stdout, "\n" );

	
		   // aspects
		   fprintf( fp, "# Number of Aspect breakpoints:\n" );
		   fprintf( fp, "1 \n" );
		   fprintf( fp, "\n" );
		   fprintf( stdout, "# Number of Aspect breakpoints:\n" );
		   fprintf( stdout, "1 \n" );
		   fprintf( stdout, "\n" );
	
		   fprintf( fp, "# Aspect breakpoints:\n" );
		   fprintf( fp, "%2.4f \n", 0.0f );
		   fprintf( fp, "\n" );
		   fprintf( stdout, "# Aspect breakpoints:\n" );
		   fprintf( stdout, "%2.4f \n", 0.0f );
		   fprintf( stdout, "\n\n" );

		   velMax = 0;
		   aspStep = 1;
		   aspMax = 0;
		   altStep = 5000;
	   }
	   else
	   {
		   fprintf( fp, "# Number of altitude breakpoints:\n" );
		   fprintf( fp, "6 \n" );
		   fprintf( fp, "\n" );
		   fprintf( stdout, "# Number of altitude breakpoints:\n" );
		   fprintf( stdout, "6 \n" );
		   fprintf( stdout, "\n" );
	
		   fprintf( fp, "# Altitude breakpoints:\n" );
		   fprintf( fp, "0.0 10000.0 20000.0 30000.0 40000.0 50000.0 \n" );
		   fprintf( fp, "\n" );
		   fprintf( stdout, "# Altitude breakpoints:\n" );
		   fprintf( stdout, "0.0 10000.0 20000.0 30000.0 40000.0 50000.0 \n" );
		   fprintf( stdout, "\n" );

		   fprintf( fp, "# Number of Velocity breakpoints:\n" );
		   fprintf( fp, "2 \n" );
		   fprintf( fp, "\n" );
		   fprintf( stdout, "# Number of Velocity breakpoints:\n" );
		   fprintf( stdout, "2 \n" );
		   fprintf( stdout, "\n" );
	
		   fprintf( fp, "# Velocity breakpoints:\n" );
		   fprintf( fp, "%5.2f %5.2f \n", 0.0f, 700.0f * KNOTS_TO_FTPSEC );
		   fprintf( fp, "\n" );
		   fprintf( stdout, "# Velocity breakpoints:\n" );
		   fprintf( stdout, "%5.2f %5.2f \n", 0.0f, 700.0f * KNOTS_TO_FTPSEC );
		   fprintf( stdout, "\n" );
	
		   // aspects
		   fprintf( fp, "# Number of Aspect breakpoints:\n" );
		   fprintf( fp, "3 \n" );
		   fprintf( fp, "\n" );
		   fprintf( stdout, "# Number of Aspect breakpoints:\n" );
		   fprintf( stdout, "3 \n" );
		   fprintf( stdout, "\n" );
	
		   fprintf( fp, "# Aspect breakpoints:\n" );
		   fprintf( fp, "%2.4f %2.4f %2.4f\n", 0.0f, 90.0f * DTR, 180.0f * DTR );
		   fprintf( fp, "\n" );
		   fprintf( stdout, "# Aspect breakpoints:\n" );
		   fprintf( stdout, "%2.4f %2.4f %2.4f\n", 0.0f, 90.0f * DTR, 180.0f * DTR );
		   fprintf( stdout, "\n\n" );

		   velMax = 700;
		   aspStep = 90;
		   aspMax = 180;
		   altStep = 10000;
	   }

	
	   // alt loop
	   for (i=0; i >= -50000; i -= altStep)
	   {
		  // speed loop
		  for ( j = 0; j <= velMax; j += 700 )
		  {
			  l = (float)j * KNOTS_TO_FTPSEC;

	   		  fprintf( fp, "# Alt = %d\n", -i );
	   		  fprintf( fp, "# Vel = %5.2f\n", l );
	   		  fprintf( stdout, "# Alt = %d\n", -i );
	   		  fprintf( stdout, "# Vel = %5.2f\n", l );

			  // aspect loop
			  for (k=0; k<=aspMax; k+=aspStep)
			  {
				count = 0;
	
				// Lock speed at 0.7 Mach
				// pdelta = CalcPressureRatio(-i, &ttheta, &rsigma);
				// sound	= (float)sqrt(ttheta) * AASL;
				// l = 0.7F * sound;
	
				// printf ("# Launch alt: %5d   TAS: %4d   Az: %3d   MaxRng: ", -i, l, k);
				{
				   startRange = 30.0F * NM_TO_FT;
				   // Check for min range
				   do
				   {
					  SimLibElapsedTime = 0.0F;
	   				  if ( gMissFiles[m].fireFromSurface )
					  {
					  		parent->SetPosition (0.0F, 0.0F, -100.0f);
					  		targetPtr->BaseData()->SetPosition (startRange, 0.0f , i-100.0f);
   							parent->SetYPR (0.0F, atan2( -(float)i, startRange) , 0.0F);
					  }
					  else
					  {
					  		parent->SetPosition (0.0F, 0.0F, i - 100);
   							parent->SetYPR (0.0F, 0.0F, 0.0F);
	   				  		if ( gMissFiles[m].targOnGround )
					  			targetPtr->BaseData()->SetPosition (startRange*cos(k*DTR), startRange*sin(k*DTR), -100.0f);
							else
					  			targetPtr->BaseData()->SetPosition (startRange*cos(k*DTR), startRange*sin(k*DTR), i-100.0f);
					  }
					  parent->SetDelta (l, 0.0F, 0.0f);
					  targetPtr->BaseData()->SetYPR (0.0F, 0.0F, 0.0F);
					  targetPtr->BaseData()->SetDelta (0.0f, 0.0f, 0.0f);
					  targetPtr->BaseData()->SetYPRDelta (0.0F, 0.0F, 0.0F);
	
					  if (theMissile)
					  {
						 // Remove the missile
//						 vuAntiDB->Remove(theMissile);
						 theMissile = NULL;
					  }
	
					  theMissile = (MissileClass*)InitAMissile(parent, missileType, 0);
	
					  LaunchMissile();
	
					  while (theMissile->done == FalconMissileEndMessage::NotDone)
					  {
						 UpdateTarget();
						 RunMissile();
						 // printf( "Missile x,y,z:  %7.0f, %7.0f, %6.0f\n", theMissile->XPos(), theMissile->YPos(), theMissile->ZPos() );
						 SimLibElapsedTime += 33;	// 33 ms = 30 Htz frame rate
					  }
	
					  startRange -= 1.0F * NM_TO_FT;
				   } while (!(theMissile->Flags() & MissileClass::EndGame) && startRange >= 0.0F);

				   if ( startRange < 0.0f )
				   		startRange = 0.0f;

				   fprintf (fp, "%8.2f ", startRange);
				   fprintf (stdout, "%8.2f ", startRange);
	
//				   vuAntiDB->Remove(theMissile);
				   theMissile = NULL;
				}
	

			 } // aspect loop
			 fprintf (fp, "\n\n");
			 fprintf (stdout, "\n\n");

		  } // speed loop

	   } // end alt loop

	   fclose( fp );

   } // end main loop

//   printf("Done.  Press ENTER to quit.\n");
//   getchar();

   return 0;
}

void LaunchMissile (void)
{
float el = atan2 (-targetPtr->BaseData()->ZPos() - 1000.0F, startRange);

   theMissile->SetLaunchRotation (0.0F, el);
   theMissile->launchState = MissileClass::Launching;
   theMissile->Start (targetPtr);
}

void RunMissile (void)
{
   if (theMissile->launchState == MissileClass::InFlight)
	{
		theMissile->Exec();
   }
	else
	{
		theMissile->launchState = MissileClass::Launching;
		theMissile->Exec();
	}
}

void UpdateTarget (void)
{
SimBaseClass* base = (SimBaseClass*)targetPtr->BaseData();

   targetPtr->BaseData()->SetPosition(
      base->XPos() + base->XDelta() * SimLibMajorFrameTime,
      base->YPos() + base->YDelta() * SimLibMajorFrameTime,
      base->ZPos() + base->ZDelta() * SimLibMajorFrameTime);

   parent->SetPosition(
      parent->XPos() + parent->XDelta() * SimLibMajorFrameTime,
      parent->YPos() + parent->YDelta() * SimLibMajorFrameTime,
      parent->ZPos() + parent->ZDelta() * SimLibMajorFrameTime);

   CalcTransformMatrix(parent);
}


int LoadWeaponData(char *filename)
	{
	FILE*			fp;
	short			entries;

	if ((fp = OpenCampFile(filename, "WCD", "rb")) == NULL)
		return 0;
	if (fread(&entries,sizeof(short),1,fp) < 1)
		return 0;
	WeaponDataTable = new WeaponClassDataType[entries];
	fread(WeaponDataTable,sizeof(WeaponClassDataType),entries,fp);
	fclose(fp);
	return 1;
	}
int LoadVehicleData(char *filename)
	{
	FILE*			fp;
	short			entries;

	if ((fp = OpenCampFile(filename, "VCD", "rb")) == NULL)
		return 0;
	if (fread(&entries,sizeof(short),1,fp) < 1)
		return 0;
	VehicleDataTable = new VehicleClassDataType[entries];
	fread(VehicleDataTable,sizeof(VehicleClassDataType),entries,fp);
	fclose(fp);

   return 1;
}

int LoadRadarData(char *filename)
{
	FILE*		fp;
	short		entries;

	if ((fp = OpenCampFile(filename, "RCD", "rb")) == NULL)
		return 0;
	if (fread(&entries,sizeof(short),1,fp) < 1)
		return 0;
	RadarDataTable = new RadarDataType[entries];
	ShiAssert( RadarDataTable );
	int ret = fread(RadarDataTable,sizeof(RadarDataType),entries,fp);
	fclose(fp);
	return 1;
}


int LoadSimWeaponData(char *filename)
{
	FILE*		fp;
	short		entries;

	if ((fp = OpenCampFile(filename, "SWD", "rb")) == NULL)
		return 0;
	if (fread(&entries,sizeof(short),1,fp) < 1)
		return 0;
	SimWeaponDataTable = new SimWeaponDataType[entries];
	ShiAssert( SimWeaponDataTable );
	int ret = fread(SimWeaponDataTable,sizeof(SimWeaponDataType),entries,fp);
	fclose(fp);
	return 1;
}

CalcPressureRatio(float alt, float* ttheta, float* rsigma)
{
	
	/*-----------------------------------------------*/
	/* calculate temperature ratio and density ratio */
	/*-----------------------------------------------*/
	if (alt <= 36089.0F)
	{
		*ttheta = 1.0F - 0.000006875F * alt;
		*rsigma = (float)pow (*ttheta, 4.256F);
	}
	else
	{
		*ttheta = 0.7519F;
		*rsigma = 0.2971F * (float)pow(2.718, 0.00004806 * (36089.0 - alt));
	}
	
	return (*ttheta) * (*rsigma);
}
