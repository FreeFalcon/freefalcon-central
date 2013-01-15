/***************************************************************************\
    Dispopts.h

	JAM 06Oct03 - Begin Major Rewrite
\***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "dispopts.h"
#include "f4find.h"
#include "graphics\include\devmgr.h"
#include "dispcfg.h"
#include "Debuggr.h"

DisplayOptionsClass DisplayOptions;

DisplayOptionsClass::DisplayOptionsClass(void)
{
	Initialize();
}

void DisplayOptionsClass::Initialize(void)
{
	DispWidth = 1024;
	DispHeight = 768;
	DispDepth = 16;
	DispVideoCard = 0;
	DispVideoDriver = 0;
	bRender2Texture = true;
	bAnisotropicFiltering = false; 
	bLinearMipFiltering = false;
	bMipmapping = false;
	bRender2DCockpit = true;
	bScreenCoordinateBiasFix = false;		//Wombat778 4-01-04
	bSpecularLighting = false;

	FalconDisplay.SetSimMode(DispWidth,DispHeight,DispDepth);
}

int DisplayOptionsClass::LoadOptions(char *filename)
{
	DWORD size;
	FILE *fp;
	size_t		success = 0;
	char		path[_MAX_PATH];

	sprintf(path,"%s\\config\\%s.dsp",FalconDataDirectory,filename);
	fp = fopen(path,"rb");

	if(!fp)
	{
		MonoPrint("Couldn't open display options\n");
		Initialize();
		fp = fopen(path,"wb");
		fclose(fp);
		return TRUE;
	}

	fseek(fp,0,SEEK_END);
	size = ftell(fp);
	fseek(fp,0,SEEK_SET);

	if( size != sizeof(class DisplayOptionsClass) )
	{
		MonoPrint("Old display options format detected\n");
		Initialize();
		fclose(fp);
		return TRUE;
	}

	success = fread(this,1,size,fp);
	fclose(fp);

	if(success != size)
	{
		MonoPrint("Failed to read display options\n",filename);
		Initialize();
		return TRUE;
	}

	const char	*buf;
	int		i = 0;

	// Make sure the chosen sim video driver is still legal
	buf = FalconDisplay.devmgr.GetDriverName(i);

	while(buf)
	{
		i++;
		buf = FalconDisplay.devmgr.GetDriverName(i);
	}

	if(i<=DispVideoDriver)
	{
		DispVideoDriver = 0;
		DispVideoCard = 0;
	}

	// Make sure the chosen sim video device is still legal
	buf = FalconDisplay.devmgr.GetDeviceName(DispVideoDriver, i);

	while(buf)
	{
		i++;
		buf = FalconDisplay.devmgr.GetDeviceName(DispVideoDriver, i);
	}

	if(i<=DispVideoCard)
	{
		DispVideoDriver = 0;
		DispVideoCard = 0;
	}

	FalconDisplay.SetSimMode(DispWidth, DispHeight, DispDepth);

	return TRUE;
}

int DisplayOptionsClass::SaveOptions(void)
{
	FILE *fp;
	size_t		success = 0;
	char		path[_MAX_PATH];

	sprintf(path,"%s\\config\\display.dsp",FalconDataDirectory);
		
	if((fp = fopen(path,"wb")) == NULL)
	{
		MonoPrint("Couldn't save display options");
		return FALSE;
	}

	success = fwrite(this, sizeof(class DisplayOptionsClass), 1, fp);
	fclose(fp);

	if(success == 1)
		return TRUE;

	return FALSE;
}