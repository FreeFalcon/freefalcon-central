// F4Compress.cpp
// Miro "Jammer" Torrielli - 12May04

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "F4Compress.h"
#include "F4Thread.h"
#include "setup.h"
#include "f4find.h"
#include "dxtlib.h"
#include "nvdxt.h"
#include "fartex.h"
#include "texbank.h"
#include "terrtex.h"
#include "dispcfg.h"
#include "playerop.h"
#include "dispopts.h"
#include "otwdrive.h"
#include "simfile.h"
#include "theaterdef.h"
#include "realweather.h"
#include "weather.h"
#include "resmgr.h"
#include "fsound.h"

#define FALCON_REGISTRY_KEY "Software\\MicroProse\\Falcon\\4.0"

RealWeather *realWeather = NULL;
//WeatherClass *TheWeather;

extern void LoadTheaterList();
#define ResFOpen  fopen
#define ResFClose  fclose
int gLangIDNum = 1;
bool bWHOT;
DWORD p3DpitHilite; // Cobra - 3D pit high night lighting color
DWORD p3DpitLolite; // Cobra - 3D pit low night lighting color
//JUNK
#ifdef DEBUG
int f4AssertsOn=TRUE,f4HardCrashOn=FALSE,shiAssertsOn=TRUE,shiWarningsOn=TRUE,shiHardCrashOn=FALSE;
#endif

int NumHats = -1;
int SimLibErrno = 0;
char g_CardDetails[1024];
bool g_bDrawBoundingBox=false;
int targetCompressionRatio = 0;
class OTWDriverClass OTWDriver;
unsigned long vuxGameTime = 0;
short NumObjectiveTypes = 0;

void ResetLatLong(void) {}
void SetLatLong(float,float) {}
void GetLatLong (float *,float *) {}
OTWDriverClass::OTWDriverClass(void) {}
OTWDriverClass::~OTWDriverClass(void) {}

void ConstructOrderedSentence (short maxlen,_TCHAR *buffer, _TCHAR *format, ... ) { };

void ReadDTXnFile(unsigned long count, void * buffer)
{
}

void WriteDTXnFile(unsigned long count, void *buffer)
{
    _write(fileout,buffer,count);
}
extern "C"
{
	int movieInit(int,LPVOID) {	return 0; }
	void movieUnInit(void) {}
}

extern "C" void
F4SoundFXSetPos( int sfxId, int override, 
				 float x,  float y,  float z, 
				 float pscale, float volume , 
				 int   uid)
{
}


LRESULT CALLBACK FalconMessageHandler (HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam){ return 0; }

int FindBestResolution(void)
{
	if (DisplayOptions.DispWidth>1280)
		return 1600;
	if (DisplayOptions.DispWidth>1024)
		return 1280;
	if (DisplayOptions.DispWidth>800)
		return 1024;
	if (DisplayOptions.DispWidth>640)
		return 800;
	return 640;
}

FILE* OpenCampFile (char *filename, char *ext, char *mode)
{
	char	fullname[MAX_PATH],path[MAX_PATH];
	
	char
		buffer[MAX_PATH];

	FILE
		*fp;

	sprintf (buffer, "OpenCampFile %s.%s %s\n", filename, ext, mode);
	// MonoPrint (buffer);
	// OutputDebugString (buffer);
	
	if (strcmp(ext,"cmp") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"obj") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"obd") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"uni") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"tea") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"wth") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"plt") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"mil") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"tri") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"evl") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"smd") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"sqd") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"pol") == 0)
		sprintf(path,FalconCampUserSaveDirectory);
	else if (stricmp(ext,"ct") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"ini") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"ucd") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"ocd") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"fcd") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"vcd") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"wcd") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"rcd") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"icd") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"rwd") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"vsd") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"swd") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"acd") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"wld") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"phd") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"pd") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"fed") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"ssd") == 0)
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"rkt") == 0)		// 2001-11-05 Added by M.N.
		sprintf(path,FalconObjectDataDir);
	else if (stricmp(ext,"ddp") == 0)		// 2002-04-20 Added by M.N.
		sprintf(path,FalconObjectDataDir);
	else
		sprintf(path,FalconCampaignSaveDirectory);
	
	//	Outdated by resmgr:
	//	if (!ResExistFile(filename))
	//		ResAddPath(path, FALSE);

	sprintf(fullname,"%s\\%s.%s",path,filename,ext);
	fp = fopen(fullname,mode);
	
	return fp;
}

void CloseCampFile (FILE *fp)
{
	if (fp)
	{
		fclose (fp);
	}
}


int main(int argc, char* argv[])
{
	HKEY theKey;
	TheaterDef *td;
	DWORD type,size;
	int retval, season;
 	BOOL bAuto = FALSE;
	BOOL bForce = TRUE;
	char tmpPath[_MAX_PATH];

	HRESULT hr = CoInitialize(NULL);
	_controlfp(_RC_CHOP,MCW_RC);
	_controlfp(_PC_24,MCW_PC);

	SetCurrentDirectory(FalconDataDirectory);
	size = sizeof(FalconDataDirectory);
	RegOpenKeyEx(HKEY_LOCAL_MACHINE,FALCON_REGISTRY_KEY,0,KEY_ALL_ACCESS,&theKey);
	RegQueryValueEx(theKey,"baseDir",0,&type,(LPBYTE)&FalconDataDirectory,&size);
	size = sizeof(FalconTerrainDataDir);
	RegQueryValueEx(theKey,"theaterDir",0,&type,(LPBYTE)FalconTerrainDataDir,&size);
	size = sizeof (FalconObjectDataDir);
	RegQueryValueEx(theKey,"objectDir",0,&type,(LPBYTE)FalconObjectDataDir,&size);
	strcpy(Falcon3DDataDir,FalconObjectDataDir);
	size = sizeof (FalconMiscTexDataDir);
	RegQueryValueEx(theKey,"misctexDir",0,&type,(LPBYTE)FalconMiscTexDataDir,&size);

	//realWeather = new WeatherClass();
//TheWeather = new WeatherClass();
	ResInit(NULL);
	ResCreatePath(FalconDataDirectory,FALSE);
	ResAddPath(FalconCampaignSaveDirectory,FALSE);
	sprintf(tmpPath,"%s\\Zips",FalconDataDirectory);
	ResAddPath(tmpPath,FALSE);
	sprintf(tmpPath,"%s\\Config",FalconDataDirectory);
	ResAddPath(tmpPath,FALSE);
	sprintf(tmpPath,"%s\\Art",FalconDataDirectory);
	ResAddPath(tmpPath,TRUE);
	sprintf(tmpPath,"%s\\sim",FalconDataDirectory);
	ResAddPath(tmpPath,TRUE);
	ResSetDirectory(FalconDataDirectory);

	LoadTheaterList();

	if(td = g_theaters.GetCurrentTheater())
		g_theaters.SetNewTheater(td);

	if(argc > 1)
 		if(!stricmp(argv[1],"-auto")) bAuto = TRUE;

#ifndef F4SEASONSWITCH
	if(!bAuto)
	{
		retval = MessageBox(
		NULL,
		"Do you wish to recompress all textures? Selecting yes will overwrite all existing dds textures.",
		"Recompress?",
		MB_YESNO | MB_ICONEXCLAMATION);
	
		if(retval == IDNO) bForce = FALSE;
	}
#endif

	DeviceIndependentGraphicsSetup(FalconTerrainDataDir,Falcon3DDataDir,FalconMiscTexDataDir);
	//F4CompressGraphicsSetup(&FalconDisplay.theDisplayDevice,bForce);

#ifdef F4SEASONSWITCH
	printf("One of the following seasons will be applied to terrain during compression:\n");
	printf("0)Summer, 1)Autumn, 2)Winter, 3)Spring\n");
	printf("Please select one of the above: ");
	char ch = getchar();
	season = min(max(atoi(&ch),0),3);

	TheTerrTextures.SyncDDSTextures(TRUE);
	TheTerrTextures.Cleanup();
	TheFarTextures.SyncDDSTextures(TRUE);
	TheFarTextures.Cleanup();
#else
	if(bAuto)
	{
		TheTerrTextures.SyncDDSTextures(TRUE);
		TheTerrTextures.Cleanup();
		TheFarTextures.SyncDDSTextures(TRUE);
		TheFarTextures.Cleanup();
	}

	TheTextureBank.SyncDDSTextures(bForce);
	TheTextureBank.Cleanup();
#endif

	if(!bAuto)
		MessageBox(
			NULL,
			"All operations completed successfully.",
			"F4Compress",
			MB_OK);

	//SAFE_DELETE(TheWeather);
	//if (realWeather != NULL)
		//delete ((WeatherClass*)realWeather);

	return 1;
}
