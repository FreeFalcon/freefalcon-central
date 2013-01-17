#include "falclib.h"
#include "chandler.h"
#include "userids.h"
#include "PlayerOp.h"
#include <mmsystem.h>
#include "sim\include\stdhdr.h"
#include "sim\include\simio.h"
#include "Graphics\Include\render3d.h"
#include "Graphics\Include\renderow.h"
#include "Graphics\Include\drawBSP.h"
#include "Graphics\Include\matrix.h"
#include "Graphics\Include\loader.h"
#include "objectiv.h"
#include "cbsplist.h"
#include "c3dview.h"
#include "ui_setup.h"
#include "Graphics\Include\RViewPnt.h"
#include "dispcfg.h"
#include "f4find.h"
#include "Graphics\Include\draw2d.h"
#include "Graphics\Include\devmgr.h"
#include "dispopts.h"
#include "sim\include\sinput.h"
#include "classtbl.h"
#include "Campaign\include\Cmpclass.h"
#include "TimeMgr.h"

//JAM 18Nov03
#include "Weather.h"

#pragma warning(disable : 4706)		// assignment within conditional expression
extern C_Handler *gMainHandler;
extern int GraphicSettingMult;

extern int HighResolutionHackFlag;		// Used in WinMain.CPP

// M.N.
//extern int skycolortime;
//extern SkyColorDataType* skycolor;

extern int MainLastGroup;
extern bool g_bAlwaysAnisotropic;	// to always turn on Anisotropic label (workaround for the GF 3)
extern bool g_bForceDXMultiThreadedCoopLevel;
extern bool g_bEnableNonPersistentTextures;
extern bool g_bEnableStaticTerrainTextures;
extern bool g_bCheckBltStatusBeforeFlip;
//extern bool g_bForceSoftwareGUI;	// not switchable inside the game
extern bool g_bVoodoo12Compatible;

#ifdef _DEBUG
bool g_bEnumSoftwareDevices = true;
#else
bool g_bEnumSoftwareDevices = false;
#endif


C_3dViewer	*SetupViewer = NULL;
RViewPoint	*tmpVpoint = NULL;
ObjectPos	*Objects = NULL;
FeaturePos	*Features = NULL;
short		NumObjects = 0, NumFeatures = 0;

extern const Trotation	IMatrix;

//defined in this file
void ChangeViewpointCB(long ID,short hittype,C_Base *control);


//defined in another file
void PositandOrientSetData (float x, float y, float z, float pitch, float roll, float yaw,
                            Tpoint* simView, Trotation* viewRotation);
void STPSetupControls(void);

const int	SMOKE = 1000;
Drawable2D *Smoke = NULL;


////////////////
/////GraphicsTab
////////////////
void STPEnterCrit()
{
	F4EnterCriticalSection(SetupCritSection);
}

void STPLeaveCrit()
{
	F4LeaveCriticalSection(SetupCritSection);
}

void STPMoveRendererCB(C_Window *win)
{
	if(SetupViewer)
		SetupViewer->Viewport(win,2);
}//MoveRendererCB

void STPViewTimerCB(long,short,C_Base *control)
{
	control->SetReady(1);
	if(control->GetFlags() & C_BIT_ABSOLUTE)
	{
		control->Parent_->RefreshWindow();
	}
	else
		control->Parent_->RefreshClient(control->GetClient());
}//ViewTimerCB


void STPDisplayCB(long,short,C_Base *)
{
	F4EnterCriticalSection(SetupCritSection);
	if(SetupViewer)
		SetupViewer->ViewOTW();
	F4LeaveCriticalSection(SetupCritSection);
}//DisplayCB

void InitializeViewer(C_Window *win, RenderOTW	*renderer )
{
	C_Slider	*slider;
	C_Button	*button;
	
	slider=(C_Slider *)win->FindControl(OBJECT_DETAIL);
	if(slider != NULL)
	{
		renderer->SetObjectDetail((float)slider->GetSliderPos()/(slider->GetSliderMax()-slider->GetSliderMin())*1.5f + 0.5f);
	}

/*	slider=(C_Slider *)win->FindControl(TEXTURE_DISTANCE);
	if(slider != NULL)
	{
		float sliderPos;
		sliderPos = (float)slider->GetSliderPos()/(slider->GetSliderMax()-slider->GetSliderMin());
		renderer->SetTerrainTextureLevel( (int)floor(sliderPos * 4.0f) );
	}
*/	
/*	button=(C_Button *)win->FindControl(OBJECT_TEXTURES);
	if(button != NULL)
	{
		renderer->SetObjectTextureState( button->GetState() );
	}
*/	

	//JAM 16Jan04
	button=(C_Button *)win->FindControl(SETUP_SPECULAR_LIGHTING);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1)
			DisplayOptions.bSpecularLighting = true;
		else
			DisplayOptions.bSpecularLighting = false;
	}

	//JAM 07Dec03
	button=(C_Button *)win->FindControl(SETUP_REALWEATHER_SHADOWS);
	if(button != NULL)
	{
		if(button->GetState() == C_STATE_1)
			PlayerOptions.SetDispFlag(DISP_SHADOWS);
		else
			PlayerOptions.ClearDispFlag(DISP_SHADOWS);
	}
/*
	button=(C_Button *)win->FindControl(BILINEAR_FILTERING);
	if(button != NULL)
	{
		renderer->SetFilteringMode( button->GetState() );
	}
*/	
	button=(C_Button *)win->FindControl(HAZING);
	if(button != NULL)
	{
		renderer->SetHazeMode(button->GetState());
	}

/*	button=(C_Button *)win->FindControl(GOUROUD);
	if(button != NULL)
	{
		renderer->SetSmoothShadingMode(button->GetState());
	}

	button=(C_Button *)win->FindControl(ALPHA_BLENDING);
	if(button != NULL)
	{
		renderer->SetAlphaMode( button->GetState() );
	}*/
}

void InsertSmokeCloud()
{
	FILE		*fp;
	char		filename[MAX_PATH];

	sprintf(filename,"%s\\config\\viewer.dat",FalconDataDirectory);
	fp = fopen(filename,"rb");
	if(fp)
	{
		ViewPos View;
		Trotation	rot = IMatrix;
		Tpoint		pos = {0.0f};
		float		scale = 100.0F;
		RViewPoint *VP;

		fread(&View,sizeof(ViewPos),1,fp);
		fclose(fp);
		fp = NULL;

		VP = SetupViewer->GetVP();
		
		PositandOrientSetData(View.Xpos * FEET_PER_KM+ 300,View.Ypos * FEET_PER_KM - 100,
						View.CamZ + View.Zpos + 100,0,0,0,&pos,&rot);

		Smoke = new Drawable2D(DRAW2D_SMOKECLOUD1,scale,&pos);
		
		VP->InsertObject(Smoke);
	}
}

void LoadObjects( ViewPos &View ,C_Window *win)
{
	FILE		*fp;
	BSPLIST		*list;
	float		Temp;
	Trotation	rot = IMatrix;
	Tpoint		pos = {0.0f};
	char		filename[MAX_PATH];
	float		scale = 1.0F;
	C_Slider	*slider;

	sprintf(filename,"%s\\config\\viewer.dat",FalconDataDirectory);
	fp = fopen(filename,"rb");
	if(fp)
	{
		fread(&View,sizeof(ViewPos),1,fp);
		
		fread(&Temp,sizeof(float),1,fp);
		
		if(Objects)
			delete [] Objects;
		
		NumObjects = static_cast<short>(FloatToInt32(Temp));
		
		Objects = new ObjectPos[NumObjects];
		
		fread(Objects,sizeof(ObjectPos),NumObjects,fp);
		
		fread(&Temp,sizeof(float),1,fp);
		
		if(Features)
			delete [] Features;

		NumFeatures = static_cast<short>(FloatToInt32(Temp));

		Features = new FeaturePos[NumFeatures];
		
		fread(Features,sizeof(FeaturePos),NumFeatures,fp);
		
		fclose(fp);
		
		fp = NULL;
		
		slider=(C_Slider *)win->FindControl(VEHICLE_SIZE);
		if(slider != NULL)
		{
			scale = ((float)slider->GetSliderPos()/(float)(slider->GetSliderMax()-slider->GetSliderMin()) * 4.0F + 1.0F);
		}

		if(NumObjects)
			{
				for(int i=0;i<NumObjects;i++)
				{
					PositandOrientSetData(View.Xpos * FEET_PER_KM + Objects[i].Xpos ,View.Ypos * FEET_PER_KM + Objects[i].Ypos,
						Objects[i].Zpos,Objects[i].Pitch,Objects[i].Roll,Objects[i].Yaw,&pos,&rot);
					list = SetupViewer->LoadBSP(i,FloatToInt32(Objects[i].VisID),TRUE);
					((DrawableBSP*)list->object)->Update(&pos,&rot);
					if(Objects[i].VisID == MapVisId(VIS_F16C))
						((DrawableBSP*)list->object)->SetSwitchMask(10, TRUE);
					list->object->SetScale(scale); 
				}
			}
			
		if(NumFeatures)
		{
			for(int i=0;i<NumFeatures;i++)
			{
				PositandOrientSetData(View.Xpos * FEET_PER_KM + Features[i].Xpos ,View.Ypos * FEET_PER_KM + Features[i].Ypos,
					Features[i].Zpos,0.0f,0.0f,Features[i].Facing,&pos,&rot);
				list = SetupViewer->LoadBuilding(i + 100,FloatToInt32(Features[i].VisID),&pos,0.0f);
				((DrawableBSP*)list->object)->Update(&pos,&rot);
			}
		}

	}
}


void STPRender(C_Base *control)
{
	C_Text		*text=NULL;
	
	if(SetupViewer == NULL)
	{
		SetCursor(gCursors[CRSR_WAIT]);

		RenderOTW	*renderer=NULL;
		RViewPoint	*viewpt=NULL;
		ViewPos		View = {0.0f};
		C_Slider	*slider=NULL;
		Tpoint		pos={0.0F};
		int			objdetail=0;
		
		F4EnterCriticalSection(SetupCritSection);

		SetupViewer=new C_3dViewer;
		SetupViewer->Setup();
		SetupViewer->Viewport(control->Parent_,2); // use client 2 for this window

		short terrlvl=0;
		float terrdist=40.0F;

		slider=(C_Slider *)control->Parent_->FindControl(TERRAIN_DETAIL);
		if(slider != NULL)
		{
			int mid;
			mid = (slider->GetSliderMax()-slider->GetSliderMin())/2;
		
			if(slider->GetSliderPos() > mid)
			{
				terrdist = (40.0f + ((float)slider->GetSliderPos() - mid)/mid * 40.0f);
				terrlvl = 0;
			}
			else 
			{
				terrdist = 40.0f;
				terrlvl = static_cast<short>(FloatToInt32(2 - ((float)slider->GetSliderPos()/mid * 2)));
			}
		}
		SetupViewer->SetTextureLevels(terrlvl,4);
		SetupViewer->SetViewDistance(terrdist * FEET_PER_KM);
		
		
		LoadObjects( View, control->Parent_ );
		SetupViewer->SetPosition(View.Xpos * FEET_PER_KM, View.Ypos * FEET_PER_KM, View.Zpos);
		SetupViewer->SetCamera(View.CamX, View.CamY, View.CamZ, View.CamYaw, View.CamPitch, View.CamRoll );
		SetupViewer->InitOTW(30.0f,FALSE);

		viewpt = SetupViewer->GetVP();		
		viewpt->GetPos(&pos);
		
		renderer = SetupViewer->GetRendOTW();
		InitializeViewer((C_Window *)control->Parent_, renderer);
		
		slider=(C_Slider *)control->Parent_->FindControl(DISAGG_LEVEL);
		if(slider != NULL)
		{
			objdetail = FloatToInt32((float)slider->GetSliderPos()/(slider->GetSliderMax()-slider->GetSliderMin()) * 5 + 0.5F);
		}
		
		if(NumObjects)
		{
			for(int i=0;i<NumObjects;i++)
			{	
				SetupViewer->AddToView(i);
			}
		}
		
		if(NumFeatures)
		{
			for(int i=0;i<NumFeatures;i++)
			{
				if(Features[i].Priority <= objdetail)
					SetupViewer->AddToView(i + 100);
			}
		}

		InsertSmokeCloud();
		
		TheLoader.WaitForLoader();

		F4LeaveCriticalSection(SetupCritSection);

		

		text = (C_Text *)control->Parent_->FindControl(LOADING);
		if(text)
		{
			text->SetFlagBitOn(C_BIT_INVISIBLE);
			text->Refresh();
		}
		
		if(!tmpVpoint)
		{
			tmpVpoint = new RViewPoint;
			tmpVpoint->Setup(80.0f*FEET_PER_KM,0,4,DisplayOptions.bZBuffering);
			tmpVpoint->Update(&pos);
		}
		SetCursor(gCursors[CRSR_F16]);
		control->Parent_->RefreshClient(2);
		ready = TRUE;
	}	
}//STPRender

void RenderViewCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;
	C_Text		*text;

	if(((C_Button *)control)->GetState())
	{
		text = (C_Text *)control->Parent_->FindControl(LOADING);
		if(text)
		{
			text->SetFlagBitOff(C_BIT_INVISIBLE);
			text->Refresh();
		}
		//Sleep(100);
		//STPRender(control);
		PostMessage(gMainHandler->GetAppWnd(),FM_STP_START_RENDER,0,(LPARAM)control);
	}
	else
	{
		if(SetupViewer)
		{
			if(!ready)
			{
				((C_Button *)control)->SetState(C_STATE_1);
				((C_Button *)control)->Refresh();
				return;
			}

			ready = FALSE;
			F4EnterCriticalSection(SetupCritSection);

			RViewPoint	*viewpt;
			viewpt = SetupViewer->GetVP();
			viewpt->RemoveObject(Smoke);
			delete Smoke;
			Smoke = NULL;
			SetupViewer->Cleanup();
			delete SetupViewer;
			SetupViewer = NULL;
			
			F4LeaveCriticalSection(SetupCritSection);
			//control->Parent_->RefreshClient(2);
			control->Parent_->RefreshWindow();
		}
	}
}



void ChangeViewpointCB(long,short,C_Base *)
{
	static int count = 0;
	
	C_Window	*win;
	
	if(ready)
	{
		// Update the Joystick control
		GetJoystickInput();

		if ( IO.digital[0] || ((count %= 3) == 0) )
		{
			win=gMainHandler->FindWindow(SETUP_WIN);
			if(win != NULL)
			{
				//Leave=UI_Enter(control->Parent_);
				F4EnterCriticalSection(SetupCritSection);
				if(SetupViewer == NULL)
				{
					F4LeaveCriticalSection(SetupCritSection);
					//UI_Leave(Leave);
					return;
				}
			
					
			
				float pitchChg = 0.0f,yawChg = 0.0f;
				float altChg = 0.0f;
				float newPitch=0.0f, newYaw=0.0f, newAlt;
			
				//gMainHandler->EnterCriticalSection();
// Retro 31Dec2003
extern AxisMapping AxisMap;
				if (AxisMap.FlightControlDevice != -1)
					if(IO.digital[(AxisMap.FlightControlDevice-SIM_JOYSTICK1)*SIMLIB_MAX_DIGITAL])
				{
					pitchChg = IO.analog[AXIS_PITCH].engrValue * PITCH_CHG;
					yawChg = IO.analog[AXIS_ROLL].engrValue * YAW_CHG;
					altChg = 1.0f - IO.analog[AXIS_THROTTLE].engrValue;
					
					if(IO.digital[((AxisMap.FlightControlDevice-SIM_JOYSTICK1)*SIMLIB_MAX_DIGITAL)+1])
					{
						altChg = altChg * altChg * ALT_CHG;
					}
					else
					{
						altChg = altChg * altChg * ALT_CHG * -1;
					}
				}
				
				if(fabs(pitchChg)<0.5)
					pitchChg = 0.0f;
				newPitch = SetupViewer->GetCameraPitch() + pitchChg;
				
				if(newPitch > 90.0f)
					newPitch = 90.0f;
				else if(newPitch < -90.0f)
					newPitch = -90.0f;
				
				if(fabs(yawChg) < 0.5)
					yawChg = 0.0f;
				newYaw = SetupViewer->GetCameraHeading() + yawChg;
				
				if(fabs(altChg) < .5f)
					altChg = 0.0f;
				
				newAlt = altChg + SetupViewer->GetCameraZ();
				
				if(newAlt > MIN_ALT)
					newAlt = MIN_ALT;
				else if(newAlt < MAX_ALT)
					newAlt = MAX_ALT;
				
				
				SetupViewer->SetCamera(SetupViewer->GetCameraX(),SetupViewer->GetCameraY(),
					newAlt,newYaw,newPitch,0.0f);
				
				//win->RefreshClient(2);
				F4LeaveCriticalSection(SetupCritSection);
				//UI_Leave(Leave);
				win->RefreshWindow();
			}
			
		}
		count++;
	}
}//ChangeViewpointCB

void ScalingCB(long,short hittype,C_Base *)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;
}

void GouraudCB(long,short hittype,C_Base *control)
{
/*	if(hittype != C_TYPE_LMOUSEUP)
		return;
	
	if(SetupViewer == NULL)
		return;
	
	RenderOTW	*renderer;
	
	renderer = SetupViewer->GetRendOTW();
	
	if(((C_Button *)control)->GetState())
		renderer->SetSmoothShadingMode( 1 );
	else
		renderer->SetSmoothShadingMode( 0 );
	
	control->Parent_->RefreshClient(2);
	
	
	control->Parent_->RefreshClient(2);
	//have the rendered view update with new settings
*/
}//GouraudCB

void HazingCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;
	
	if(SetupViewer == NULL)
		return;
	
	RenderOTW	*renderer;
	
	renderer = SetupViewer->GetRendOTW();
	
	if(((C_Button *)control)->GetState())
		renderer->SetHazeMode( 1 );
	else
		renderer->SetHazeMode( 0 );
	
	control->Parent_->RefreshClient(2);
	
	//have the rendered view update with new settings
	
}//HazingCB

//JAM 07Dec03
void RealWeatherShadowsCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;
	
	if(SetupViewer == NULL)
		return;
	
	RenderOTW	*renderer;
	
	renderer = SetupViewer->GetRendOTW();

	if(((C_Button *)control)->GetState())
			PlayerOptions.SetDispFlag(DISP_SHADOWS);
	else
			PlayerOptions.ClearDispFlag(DISP_SHADOWS);
	
	control->Parent_->RefreshClient(2);
}

void BilinearFilterCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;
	
	if(SetupViewer == NULL)
		return;
	
	RenderOTW	*renderer;
	
	renderer = SetupViewer->GetRendOTW();
	
	if(((C_Button *)control)->GetState())
		renderer->SetFilteringMode( TRUE );
	else
		renderer->SetFilteringMode( FALSE );
	
	control->Parent_->RefreshClient(2);
	
	//have the rendered view update with new settings
	
}//BilinearFilterCB


/*void ObjectTextureCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;
	
	if(SetupViewer == NULL)
		return;
	
	RenderOTW	*renderer;
	
	renderer = SetupViewer->GetRendOTW();
	
	if(((C_Button *)control)->GetState())
		renderer->SetObjectTextureState( TRUE );
	else
		renderer->SetObjectTextureState( FALSE );
	
	control->Parent_->RefreshClient(2);
	
	//have the rendered view update with new settings
	
}//ObjectTextureCB
*/
void RemoveObjFromView(int objID)
{
	C_BSPList *bsplist;
	BSPLIST	  *list;

	F4EnterCriticalSection(SetupCritSection);

	bsplist = SetupViewer->GetBSPList();
	// I don't think this will happen, but time is short so lets be safe...
	if (!bsplist)	return;

	list = bsplist->Root_;
	
	while (list)
	{
		if(list->ID == objID)
			break;
		list = list->Next;
	}

	if (list)
	{
		RViewPoint *tVP;

		tVP = SetupViewer->GetVP();

		tVP->RemoveObject(list->object);
	}

	F4LeaveCriticalSection(SetupCritSection);
	
}
void BuildingDetailCB(long,short hittype,C_Base *control)
{
	C_Slider *slider;
	int		objdetail;
	static int prevdetail = -1;
	
	if(hittype != C_TYPE_MOUSEMOVE)
		return;

	slider = (C_Slider *)control;
	objdetail = FloatToInt32((float)slider->GetSliderPos()/(slider->GetSliderMax()-slider->GetSliderMin()) * 5 + 0.5F);
		
	C_EditBox *ebox;
	ebox = (C_EditBox *)control->Parent_->FindControl(slider->GetUserNumber(0));
	if(ebox)
	{
		ebox->SetInteger(objdetail + 1);
		ebox->Refresh();
	}

	F4EnterCriticalSection(SetupCritSection);

	if(SetupViewer == NULL)
	{
		F4LeaveCriticalSection(SetupCritSection);
		return;
	}
	
	if(NumFeatures)
	{
		
		if(prevdetail < objdetail)
		{
			if(NumFeatures)
			{
				for(int i=0;i<NumFeatures;i++)
				{
					if((Features[i].Priority <= objdetail) && (Features[i].Priority > prevdetail))
						SetupViewer->AddToView(i + 100);
				}
			}
		}
		else if(prevdetail > objdetail)
		{
			for(int i=0;i<NumFeatures;i++)
			{
				if((Features[i].Priority > objdetail) && (Features[i].Priority <= prevdetail))
					RemoveObjFromView(i+100);	
			}
		}
		prevdetail = objdetail;
	}
	
	control->Parent_->RefreshWindow();
	F4LeaveCriticalSection(SetupCritSection);
	
}//BuildingDetailCB

void PlayerBubbleCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_MOUSEMOVE)
		return;

	C_Slider    *slider;
	slider=(C_Slider *)control;

	C_EditBox *ebox;
	ebox = (C_EditBox *)control->Parent_->FindControl(slider->GetUserNumber(0));
	if(ebox)
	{
		ebox->SetInteger(FloatToInt32((float)slider->GetSliderPos()/(slider->GetSliderMax()-slider->GetSliderMin())*6*GraphicSettingMult + 1.5f));
		ebox->Refresh();
	}
}


void ObjectDetailCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_MOUSEMOVE)
		return;

	float detail;
	C_Slider    *slider;
	slider=(C_Slider *)control;

	C_EditBox *ebox;
	ebox = (C_EditBox *)control->Parent_->FindControl(slider->GetUserNumber(0));
	if(ebox)
	{
		ebox->SetInteger(FloatToInt32((float)slider->GetSliderPos()/(slider->GetSliderMax()-slider->GetSliderMin())*6*GraphicSettingMult + 1.5f));
		ebox->Refresh();
	}

	if(SetupViewer == NULL)
		return;

	RenderOTW	*renderer;	
	renderer = SetupViewer->GetRendOTW();
	detail = ((float)slider->GetSliderPos()/(slider->GetSliderMax()-slider->GetSliderMin())*1.5f*GraphicSettingMult);
	renderer->SetObjectDetail(detail);
	
	//have the rendered view update with new settings
	control->Parent_->RefreshWindow();
	
}//VehicleDetailCB

void VehicleSizeCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_MOUSEMOVE)
		return;

	C_Slider	*slider;
	BSPLIST		*list;
	int			scale;

	slider=(C_Slider *)control;
	scale = FloatToInt32((float)slider->GetSliderPos()/(slider->GetSliderMax()-slider->GetSliderMin()) * 4 + 1);

	C_EditBox *ebox;
	ebox = (C_EditBox *)control->Parent_->FindControl(slider->GetUserNumber(0));
	if(ebox)
	{
		ebox->SetInteger(scale);
		ebox->Refresh();
	}

	if(SetupViewer == NULL)
		return;

	
	static int	prevscale;

	
	if(prevscale != scale)
	{
		list = ((C_BSPList	*)SetupViewer->GetBSPList())->Root_;
		
		while(list)
		{
			if(list->ID < 100)
				list->object->SetScale((float)scale);

			list = list->Next;
		}
		
		prevscale = scale;
	}
	
	//have the rendered view update with new settings
	control->Parent_->RefreshWindow();
	
	
	
}//VehicleSizeCB

void TerrainDetailCB(long,short hittype,C_Base *control)
{
	C_Slider *slider;
	static int prevpos = -1;

	if(hittype != C_TYPE_MOUSEMOVE)
		return;

	slider=(C_Slider *)control;

	C_EditBox *ebox;
	ebox = (C_EditBox *)control->Parent_->FindControl(slider->GetUserNumber(0));
	if(ebox)
	{
		ebox->SetInteger(FloatToInt32((float)slider->GetSliderPos()/(slider->GetSliderMax()-slider->GetSliderMin())*6.0F*GraphicSettingMult  + 1.5F));
		ebox->Refresh();
	}

	if(SetupViewer == NULL)
		return;
	
	int step;
	step = (slider->GetSliderMax()-slider->GetSliderMin())/(6*GraphicSettingMult);

	if(abs(slider->GetSliderPos() - prevpos) > step - 1)
	{
		SetCursor(gCursors[CRSR_WAIT]);

		int disagglvl;
		short terrlvl;
		float terrdist;
		RenderOTW	*renderer;
		RViewPoint	*viewpt;
		C_BSPList	*bsplist;
		BSPLIST		*cur;
		Tpoint		pos;
		
		prevpos = slider->GetSliderPos();

		if(slider->GetSliderPos() > 2*step){
			terrdist = 40.0f + ( (float)slider->GetSliderPos()/step - 2) * 10.0f;
			terrlvl = 0;
		}
		else {
			terrdist = 40.0f;
			terrlvl = static_cast<short>(FloatToInt32(2.0F - ((float)slider->GetSliderPos()/step)));
		}
					
		F4EnterCriticalSection(SetupCritSection);
		if(SetupViewer)
		{
			C_Slider *tslider;
			C_Button *button;

			renderer = SetupViewer->GetRendOTW();
			viewpt = SetupViewer->GetVP();
			
			viewpt->GetPos(&pos);
			
			//SetupViewer->Cleanup();
			bsplist = SetupViewer->GetBSPList();
			cur=bsplist->GetFirst();
			while(cur)
			{
				if(((DrawableBSP*)cur->object)->InDisplayList())
					viewpt->RemoveObject(cur->object);
				cur=cur->Next;
			}

			viewpt->RemoveObject(Smoke);
			delete Smoke;
			Smoke = NULL;
			viewpt->Cleanup();
			//viewpt = new RViewPoint;
			viewpt->Setup(terrdist*FEET_PER_KM,terrlvl,4,DisplayOptions.bZBuffering);
			viewpt->Update(&pos);
			
			renderer->Cleanup();

			renderer->Setup(gMainHandler->GetFront(),viewpt);

			//reset all values for new renderer
			tslider = (C_Slider *)control->Parent_->FindControl(OBJECT_DETAIL);
			if(tslider)
			{
				renderer->SetObjectDetail((float)tslider->GetSliderPos()/(tslider->GetSliderMax()-tslider->GetSliderMin())*1.5f + 0.5f);
			}

/*			tslider = (C_Slider *)control->Parent_->FindControl(TEXTURE_DISTANCE);
			if(tslider)
			{
				renderer->SetTerrainTextureLevel(FloatToInt32((float)tslider->GetSliderPos()/(tslider->GetSliderMax()-tslider->GetSliderMin())* 4.0f));
			}

			button = (C_Button *)control->Parent_->FindControl(OBJECT_TEXTURES);
			if(button)
			{
				if(button->GetState())
					renderer->SetObjectTextureState( TRUE );
				else
					renderer->SetObjectTextureState( FALSE );
			}
			button = (C_Button *)control->Parent_->FindControl(BILINEAR_FILTERING);

			if(button)
			{
				if(button->GetState())
					renderer->SetFilteringMode( TRUE );
				else
					renderer->SetFilteringMode( FALSE );
			}
*/			

/*			button = (C_Button *)control->Parent_->FindControl(ALPHA_BLENDING);
			if(button)
			{
				if(button->GetState())
					renderer->SetAlphaMode( TRUE );
				else
					renderer->SetAlphaMode( FALSE );
			}
*/
			button = (C_Button *)control->Parent_->FindControl(HAZING);
			if(button)
			{
				if(button->GetState())
					renderer->SetHazeMode( TRUE );
				else
					renderer->SetHazeMode( FALSE );
			}

/*			button = (C_Button *)control->Parent_->FindControl(GOUROUD);
			if(button)
			{
				if(button->GetState())
					renderer->SetSmoothShadingMode( TRUE );
				else
					renderer->SetSmoothShadingMode( FALSE );
			}
*/

			InitializeViewer((C_Window *)control->Parent_, renderer);

			SetupViewer->Viewport((C_Window *)control->Parent_,2);

			slider=(C_Slider *)control->Parent_->FindControl(DISAGG_LEVEL);
			if(slider != NULL)
			{
				disagglvl = FloatToInt32((float)slider->GetSliderPos()/(slider->GetSliderMax()-slider->GetSliderMin()) * 5 + 0.5F);
			
				if(NumFeatures)
				{
					for(int i=0;i<NumFeatures;i++)
					{
						if(Features[i].Priority <= disagglvl)
							SetupViewer->AddToView(i + 100);
					}
				}
			}
			
			if(NumObjects)
			{
				for(int i=0;i<NumObjects;i++)
				{	
					SetupViewer->AddToView(i);
				}
			}

			InsertSmokeCloud();
			
			SetCursor(gCursors[CRSR_F16]);
		}
		F4LeaveCriticalSection(SetupCritSection);
		
			
		
		control->Parent_->RefreshWindow();
	}
}

/*void TextureDistanceCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_MOUSEMOVE)
		return;
	
	C_Slider	*slider;
	slider = (C_Slider *)control;
	int pos;
	pos = FloatToInt32((float)slider->GetSliderPos()/(slider->GetSliderMax()-slider->GetSliderMin()) * 4.0f);
		
	C_EditBox *ebox;
	ebox = (C_EditBox *)control->Parent_->FindControl(slider->GetUserNumber(0));
	if(ebox)
	{
		ebox->SetInteger(pos + 1);
		ebox->Refresh();
	}

	if(SetupViewer)
	{
		RenderOTW	*renderer;
		int			TexLev;
		
		renderer = SetupViewer->GetRendOTW();
		
		TexLev = renderer->GetTerrainTextureLevel();
		
		//have the rendered view update with new settings
		if(pos != TexLev)
		{
			renderer->SetTerrainTextureLevel(pos);
			//control->Parent_->RefreshClient(2);
		}
		control->Parent_->RefreshWindow();
		
	}
}//TextureDistanceCB
*/
void SfxLevelCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_MOUSEMOVE)
		return;
	
	C_Slider	*slider;
	slider = (C_Slider *)control;
	int pos;
	pos = FloatToInt32((float)slider->GetSliderPos()/(slider->GetSliderMax()-slider->GetSliderMin()) * 4.0f);
		
	C_EditBox *ebox;
	ebox = (C_EditBox *)control->Parent_->FindControl(slider->GetUserNumber(0));
	if(ebox)
	{
		ebox->SetInteger(pos + 1);
		ebox->Refresh();
	}

	
}//SfxLevelCB


void BuildVideoCardList(C_ListBox *lbox)
{
	const char *buf;
	int i = 0;
	int Driver;
	char buf2[256];
	long value;

	C_ListBox *VidCardList = (C_ListBox *)lbox->Parent_->FindControl(SET_VIDEO_DRIVER);
	Driver = VidCardList->GetTextID() - 1;

	DeviceManager::DDDriverInfo *pDI = FalconDisplay.devmgr.GetDriver(Driver);
	if(!pDI) return;

	value = lbox->GetTextID();
	lbox->RemoveAllItems();

	while(buf = FalconDisplay.devmgr.GetDeviceName(Driver, i))
	{
		if(!g_bEnumSoftwareDevices)
		{
			// check for software device
			DeviceManager::DDDriverInfo::D3DDeviceInfo *pD3DDI = pDI->GetDevice(i);
			if(pD3DDI && !pD3DDI->IsHardware())
			{
				i++;
				continue;
			}
		}

		strcpy(buf2,buf);
		i++;

		lbox->AddItem(i,C_TYPE_ITEM,buf2);
	}

	ShiAssert(i>0);
	lbox->SetValue(value);
	lbox->Refresh();
}


void BuildVideoDriverList(C_ListBox *lbox)
{
	char		buf2[256];
	const char	*buf;
	int			i = 0;

	lbox->RemoveAllItems();

	while(buf = FalconDisplay.devmgr.GetDriverName(i))
	{
		if(FalconDisplay.devmgr.GetDeviceName(i,0))
		{
			strcpy(buf2,buf);
			lbox->AddItem(i + 1,C_TYPE_ITEM,buf2);
		}

		i++;
	}
	ShiAssert(i>0);
	lbox->Refresh();
}


void BuildResolutionList(C_ListBox *lbox)
{
	int i =0,Card,Driver;
	char buf2[256];
	long value;
	UINT width,height,depth;
	int isel = -1;
	int nNumItems = 0;

	C_ListBox *VidDriverList = (C_ListBox *)lbox->Parent_->FindControl(SET_VIDEO_DRIVER);
	Driver = VidDriverList->GetTextID() - 1;

	C_ListBox *VidCardList = (C_ListBox *)lbox->Parent_->FindControl(SET_VIDEO_CARD);
	Card = VidCardList->GetTextID() - 1;

	value = lbox->GetTextID();
	lbox->RemoveAllItems();

	DeviceManager::DDDriverInfo *pDI = FalconDisplay.devmgr.GetDriver(Driver);
	if(!pDI) return;
	DeviceManager::DDDriverInfo::D3DDeviceInfo *pD3DDI = pDI->GetDevice(Card);
	if(!pD3DDI) return;

// OW
#if 1
	while(FalconDisplay.devmgr.GetMode(Driver,Card, i++, &width, &height, &depth))
	{
		// For now we only allow 640x480, 800x600, 1280x960, 1600x1200
		// (MPR already does the 4:3 aspect ratio check for us)
		if(height > 400 && ((width == 640 || width == 800 || width == 1024 ||
			(width == 1280 && height == 960) || width == 1600 || HighResolutionHackFlag)))
		{
			if(depth == 8 || depth == 24)
				continue;

//			if(depth == 16 && !(pD3DDI->m_devDesc.dwDeviceRenderBitDepth & DDBD_16))
			if(depth == 16)
				continue;
			else if(depth == 32 && !(pD3DDI->m_devDesc.dwDeviceRenderBitDepth & DDBD_32))
				continue;

			sprintf(buf2, "%0dx%0d - %d Bit", width, height, depth);
			lbox->AddItem(i - 1, C_TYPE_ITEM, buf2);

			// remember index for current mode
			if(width == DisplayOptions.DispWidth && height == DisplayOptions.DispHeight && depth == DisplayOptions.DispDepth)
				isel = i - 1;

			nNumItems++;
		}
	}

	ShiAssert(i>0);
	if(isel != -1) lbox->SetValue(isel);
	else lbox->SetValue(value);
	lbox->Refresh();
#else
	while(buf = FalconDisplay.devmgr.GetModeName(Driver,Card,i))
	{
		strcpy(buf2,buf);
		i++;
		sscanf(buf2,"%d x %d",&width,&height);
		//sscanf(buf2,"%d",&width);

		lbox->AddItem(width,C_TYPE_ITEM,buf2);
	}

	ShiAssert(i>0);
	lbox->SetValue(value);
	lbox->Refresh();
#endif
}

void DisableEnableDrivers(C_ListBox *)
{
	
}

void DisableEnableResolutions(C_ListBox* )
{
}

void SetAdvanced()
{
	C_Window	*win;
	C_Button	*button;
	C_ListBox	*lbox;

	win=gMainHandler->FindWindow(SETUP_WIN);
	if(win == NULL) return;

	lbox=(C_ListBox *)win->FindControl(SET_VIDEO_DRIVER);
	if(!lbox) return;
	int nDriver = lbox->GetTextID() - 1;
	lbox=(C_ListBox *)win->FindControl(SET_VIDEO_CARD);
	if(!lbox) return;
	int nDevice = lbox->GetTextID() - 1;
	DeviceManager::DDDriverInfo *pDI = FalconDisplay.devmgr.GetDriver(nDriver);
	if(!pDI) return;
	DeviceManager::DDDriverInfo::D3DDeviceInfo *pD3DDI = pDI->GetDevice(nDevice);
	if(!pD3DDI) return;

	win=gMainHandler->FindWindow(SETUP_ADVANCED_WIN);
	if(!win) return;

	//========================================
	// FRB - Force Z-Buffering
     DisplayOptions.bZBuffering = TRUE;
	// FRB - Force Specular Lighting
//     DisplayOptions.bSpecularLighting = TRUE;
  // DDS textures only
//		 DisplayOptions.m_texMode = TEX_MODE_DDS;
	//========================================

	//JAM 12Oct03
	button = (C_Button *) win->FindControl(SETUP_ADVANCED_ANISOTROPIC_FILTERING);
	if(button) button->SetState(DisplayOptions.bAnisotropicFiltering ? C_STATE_1 : C_STATE_0);

	button = (C_Button *) win->FindControl(SETUP_ADVANCED_LINEAR_MIPMAP_FILTERING);
	if(button) button->SetState(DisplayOptions.bLinearMipFiltering ? C_STATE_1 : C_STATE_0);

	button = (C_Button *) win->FindControl(SETUP_ADVANCED_RENDER_2DCOCKPIT);
	if(button) button->SetState(DisplayOptions.bRender2DCockpit ? C_STATE_1 : C_STATE_0);

	button = (C_Button *) win->FindControl(SETUP_ADVANCED_SCREEN_COORD_BIAS_FIX);
	if(button) button->SetState(DisplayOptions.bScreenCoordinateBiasFix ? C_STATE_1 : C_STATE_0);		//Wombat778 4-01-04

	button = (C_Button *) win->FindControl(SETUP_ADVANCED_MIPMAPPING);
	if(button) button->SetState(DisplayOptions.bMipmapping ? C_STATE_1 : C_STATE_0);

	button = (C_Button *) win->FindControl(SETUP_ADVANCED_RENDER_TO_TEXTURE);
	if(button)
	{
		if(pDI->SupportsSRT()) button->SetFlagBitOn(C_BIT_ENABLED);
		else
		{
			button->SetFlagBitOff(C_BIT_ENABLED);
			DisplayOptions.bRender2Texture = false;
		}

		button->SetState(DisplayOptions.bRender2Texture ? C_STATE_1 : C_STATE_0);
	}

// 	lbox = (C_ListBox *)win->FindControl(SETUP_ADVANCED_TEXTURE_MODE);
// 	if(lbox) lbox->SetValue(DisplayOptions.m_texMode);
	//JAM
}

static void LoadBitmap(long ID, C_Button *btn,char filename[])
{
	char file[MAX_PATH];

	btn->ClearImage(0, ID);
	gImageMgr->RemoveImage(ID);
	strcpy(file,filename);
	strcat(file,".tga");
	gImageMgr->LoadImage(ID,file,0,0);
	btn->Refresh();
	btn->SetImage(0,ID);
	btn->Refresh();

}

//M.N.
/*void SetSkyColor()
{
	C_Window	*win;
	C_Button	*btn;
	C_ListBox	*lbox;

	win=gMainHandler->FindWindow(SETUP_SKY_WIN);
	if(!win) return;
	
	lbox=(C_ListBox *)win->FindControl(SETUP_SKY_COLOR);
	if (lbox)
	{
		lbox->SetValue(PlayerOptions.skycol);
		lbox->Refresh();
	}
	btn=(C_Button*)win->FindControl(SKY_COLOR_PIC);
	if(btn)
	{
		switch(skycolortime)
		{
		case 0:
			LoadBitmap(SKYCOLOR_BITMAP_ID,btn,skycolor[PlayerOptions.skycol-1].image1);
			break;
		case 1:
			LoadBitmap(SKYCOLOR_BITMAP_ID,btn,skycolor[PlayerOptions.skycol-1].image2);
			break;
		case 2:
			LoadBitmap(SKYCOLOR_BITMAP_ID,btn,skycolor[PlayerOptions.skycol-1].image3);
			break;
		case 3:
			LoadBitmap(SKYCOLOR_BITMAP_ID,btn,skycolor[PlayerOptions.skycol-1].image4);
			break;
		default:
			MonoPrint("Not allowed skycolortime found !!");
			break;
		}
	}
	win->RefreshWindow();
}

void SelectSkyColorCB(long ID,short hittype,C_Base *control)
{
	C_ListBox *lbox=(C_ListBox*)control;

	if(hittype != C_TYPE_SELECT)
		return;

	ID=lbox->GetTextID();
	PlayerOptions.skycol = lbox->GetTextID();
	SetSkyColor();
}

void SkyColTimeCB(long ID,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	if(!control)
		return;

	switch(control->GetID())
	{
		case SKY_COLOR_TIME_1: // 7:00
			skycolortime = 0;
			break;
		case SKY_COLOR_TIME_2: // 12:00
			skycolortime = 1;
			break;
		case SKY_COLOR_TIME_3: // 19:00
			skycolortime = 2;
			break;
		case SKY_COLOR_TIME_4: // 0:00
			skycolortime = 3;
			break;
		default:
			MonoPrint("Not allowed button type");
	}
	SetSkyColor();
}
*/

void VideoCardCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_SELECT)
		return;
	
	C_ListBox *lbox;
	

	lbox = (C_ListBox *)control->Parent_->FindControl(SET_RESOLUTION);
	if(lbox)
		BuildResolutionList(lbox);
	//DisableEnableDrivers //SET_VIDEO_DRIVER
	//DisableEnableResolutions //SET_RESOLUTION

	SetAdvanced();
}

void VideoDriverCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_SELECT)
		return;
	
	C_ListBox *lbox = (C_ListBox *)control->Parent_->FindControl(SET_VIDEO_CARD);
	if(lbox)
		BuildVideoCardList(lbox);

	lbox = (C_ListBox *)control->Parent_->FindControl(SET_RESOLUTION);
	if(lbox)
		BuildResolutionList(lbox);

	//DisableEnableResolutions //SET_RESOLUTION

// JB 011124 No relevance for the DX7 engine
/*
	C_Button *button;
	if( ((C_ListBox*)control)->GetTextID() > 1)
	{
		button=(C_Button *)control->Parent_->FindControl(ALPHA_BLENDING);
		if(button != NULL)
		{
			button->SetState(C_STATE_1);
			button->Refresh();
		}
		
		button=(C_Button *)control->Parent_->FindControl(BILINEAR_FILTERING);
		if(button != NULL)
		{
			button->SetState(C_STATE_1);
			button->Refresh();
		}
	}
	else
	{
		button=(C_Button *)control->Parent_->FindControl(ALPHA_BLENDING);
		if(button != NULL)
		{
			button->SetState(C_STATE_0);
			button->Refresh();
		}
		
		button=(C_Button *)control->Parent_->FindControl(BILINEAR_FILTERING);
		if(button != NULL)
		{
			button->SetState(C_STATE_0);
			button->Refresh();
		}
	}
*/
}

void ResolutionCB(long,short hittype,C_Base *)
{
	if(hittype != C_TYPE_SELECT)
		return;
}

//JAM 21Nov03
void RealWeatherCB(long, short hittype, C_Base *control)
{
	if(hittype != C_TYPE_SELECT) return;

	C_ListBox *lbox=(C_ListBox*)control;

	if( TheCampaign.InMainUI || !((WeatherClass *)realWeather)->lockedCondition )
	{
		PlayerOptions.weatherCondition = lbox->GetTextID()-70207;
		((WeatherClass *)realWeather)->UpdateCondition(PlayerOptions.weatherCondition, true);
		((WeatherClass *)realWeather)->Init(true);
	}
	else if( ((WeatherClass *)realWeather)->unlockableCondition == 0 )
	{
		if( lbox->GetTextID() == 70213 )
		{
			lbox->RemoveAllItems();

			lbox->AddItem(70208,C_TYPE_ITEM,"Sunny");
			lbox->AddItem(70209,C_TYPE_ITEM,"Fair");
			lbox->AddItem(70210,C_TYPE_ITEM,"Poor");
			lbox->AddItem(70211,C_TYPE_ITEM,"Inclement");

			lbox->SetValue(realWeather->weatherCondition+70207);
			lbox->Refresh();

			((WeatherClass *)realWeather)->lockedCondition = FALSE;
		}
	}
	else if( lbox->GetTextID() == 70213 )
	{
		lbox->RemoveAllItems();

		lbox->AddItem(70208,C_TYPE_ITEM,"Sunny");
		lbox->AddItem(70209,C_TYPE_ITEM,"Fair");
		lbox->AddItem(70210,C_TYPE_ITEM,"Poor");
		lbox->AddItem(70211,C_TYPE_ITEM,"Inclement");

		lbox->SetValue(realWeather->weatherCondition+70207);
		lbox->Refresh();

		((WeatherClass *)realWeather)->lockedCondition = FALSE;
	}
}
//JAM

//THW 2004-01-17
void SeasonCB(long, short hittype, C_Base *control)
{
	if(hittype != C_TYPE_SELECT) return;

	C_ListBox *lbox=(C_ListBox*)control;
	PlayerOptions.Season = lbox->GetTextID()-70313;
	//lbox->AddItem(70313,C_TYPE_ITEM,"Summer");
	//lbox->AddItem(70314,C_TYPE_ITEM,"Fall");
	//lbox->AddItem(70315,C_TYPE_ITEM,"Winter");
	//lbox->AddItem(70316,C_TYPE_ITEM,"Spring");
	//lbox->SetValue(PlayerOptions.Season+70313);
	//lbox->Refresh();
}
//THW

void SetupGraphicsControls(void)
{
	C_Window	*win;
	C_Button	*button;
	C_ListBox	*lbox;
	C_Slider	*slider;
	C_EditBox	*ebox;
	
	win=gMainHandler->FindWindow(SETUP_WIN);
	
	if(win == NULL)
		return;
		lbox=(C_ListBox *)win->FindControl(SET_VIDEO_CARD);
	if(lbox != NULL)
	{
		BuildVideoCardList(lbox);

		lbox->SetValue(DisplayOptions.DispVideoCard + 1);
		lbox->Refresh();
	}
	
	lbox=(C_ListBox *)win->FindControl(SET_VIDEO_DRIVER);
	if(lbox != NULL)
	{
		BuildVideoDriverList(lbox);

		DisableEnableDrivers(lbox);
		lbox->SetValue(DisplayOptions.DispVideoDriver + 1);
		lbox->Refresh();
	}

	lbox=(C_ListBox *)win->FindControl(SET_RESOLUTION);
	if(lbox != NULL)
	{
		DisableEnableResolutions(lbox);
		lbox->SetValueText( DisplayOptions.DispWidth ); 
		lbox->Refresh();
	}

	button=(C_Button *)win->FindControl(70136);//GOUROUD
	if(button != NULL)
	{
		if(PlayerOptions.GouraudOn())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}
	
	button=(C_Button *)win->FindControl(HAZING);
	if(button != NULL)
	{
		if(PlayerOptions.HazingOn())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}

	//JAM 16Jan04
	button=(C_Button *)win->FindControl(SETUP_SPECULAR_LIGHTING);
	if(button != NULL)
	{
		if(DisplayOptions.bSpecularLighting)
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}

	//JAM 07Dec03
	button=(C_Button *)win->FindControl(SETUP_REALWEATHER_SHADOWS);
	if(button != NULL)
	{
		if(PlayerOptions.ShadowsOn())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}

/*	
	button=(C_Button *)win->FindControl(BILINEAR_FILTERING);
	if(button != NULL)
	{
		if(PlayerOptions.FilteringOn())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}
*/	
/*	button=(C_Button *)win->FindControl(OBJECT_TEXTURES);
	if(button != NULL)
	{
		if(PlayerOptions.ObjectTexturesOn())
			button->SetState(C_STATE_1);
		else
			button->SetState(C_STATE_0);
		button->Refresh();
	}
*/
	slider=(C_Slider *)win->FindControl(OBJECT_DETAIL);
	if(slider != NULL)
	{
		slider->Refresh();
		ebox = (C_EditBox *)win->FindControl(OBJECT_DETAIL_READOUT);
		if(ebox)
		{
			ebox->SetInteger(FloatToInt32(PlayerOptions.ObjDetailLevel*4.0F - 1.0F));
			ebox->Refresh();
			slider->SetUserNumber(0,OBJECT_DETAIL_READOUT);
		}
		slider->SetSliderPos(FloatToInt32((slider->GetSliderMax()-slider->GetSliderMin())*(PlayerOptions.ObjDetailLevel-0.5f)/1.5f));
		slider->Refresh();
	}

	slider=(C_Slider *)win->FindControl(DISAGG_LEVEL);
	if(slider != NULL)
	{
		slider->Refresh();
		ebox = (C_EditBox *)win->FindControl(DISAGG_LEVEL_READOUT);
		if(ebox)
		{
			// 2001-11-10 M.N. Added "+ 1" -> BldDeaggLevel = 0-5, display = 1-6
			ebox->SetInteger(PlayerOptions.BldDeaggLevel + 1);
			ebox->Refresh();
			slider->SetUserNumber(0,DISAGG_LEVEL_READOUT);
		}
		slider->SetSliderPos((slider->GetSliderMax()-slider->GetSliderMin())*PlayerOptions.ObjDeaggLevel/100);
		slider->Refresh();
	}
	
		
	slider=(C_Slider *)win->FindControl(VEHICLE_SIZE);
	if(slider != NULL)
	{
		slider->Refresh();
		slider->SetSliderPos(FloatToInt32((slider->GetSliderMax()-slider->GetSliderMin())*(PlayerOptions.ObjMagnification - 1.0F)/4.0F));
		ebox = (C_EditBox *)win->FindControl(VEHICLE_SIZE_READOUT);
		if(ebox)
		{
			ebox->SetInteger(FloatToInt32(PlayerOptions.ObjMagnification));
			ebox->Refresh();
			slider->SetUserNumber(0,VEHICLE_SIZE_READOUT);
		}
		slider->Refresh();
	}
	
/*	slider=(C_Slider *)win->FindControl(TEXTURE_DISTANCE);
	if(slider != NULL)
	{
		slider->Refresh();
		ebox = (C_EditBox *)win->FindControl(TEX_DISTANCE_READOUT);
		if(ebox)
		{
			ebox->SetInteger(PlayerOptions.DispTextureLevel + 1);
			ebox->Refresh();
			slider->SetUserNumber(0,TEX_DISTANCE_READOUT);
		}
		slider->SetSliderPos((slider->GetSliderMax()-slider->GetSliderMin())*(PlayerOptions.DispTextureLevel)/4);
		slider->Refresh();
	}
*/
	slider=(C_Slider *)win->FindControl(TERRAIN_DETAIL);
	if(slider != NULL)
	{
		int step;
		step = (slider->GetSliderMax()-slider->GetSliderMin())/6;
	
		slider->Refresh();
		if(PlayerOptions.DispTerrainDist > 40)
			slider->SetSliderPos(FloatToInt32(step*(2+(PlayerOptions.DispTerrainDist - 40.0F)/10.0F)));
		else 
			slider->SetSliderPos((2 - PlayerOptions.DispMaxTerrainLevel)*step);
		slider->Refresh();

		ebox = (C_EditBox *)win->FindControl(TEX_DETAIL_READOUT);
		if(ebox)
		{
			ebox->SetInteger( FloatToInt32(((float)slider->GetSliderPos()/(slider->GetSliderMax()-slider->GetSliderMin()))*6.0F + 1.5F) );
			ebox->Refresh();
			slider->SetUserNumber(0,TEX_DETAIL_READOUT);
		}
	}

	win=gMainHandler->FindWindow(SETUP_ADVANCED_WIN);
	if(!win) return;

	// M.N. SkyColor stuff
//	win = gMainHandler->FindWindow(SETUP_SKY_WIN);
//	if (win) { // JPO conditional
//	    lbox = (C_ListBox *) win->FindControl(SETUP_SKY_COLOR);
//	    if (lbox) lbox->SetValue(PlayerOptions.skycol);
//	}
	// M.N. end SkyColor stuff
}

void GraphicsDefaultsCB(long,short hittype,C_Base *)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	PlayerOptionsClass Player;
	DisplayOptionsClass Display;

	//if(Display.LoadOptions("default"))
	if(Display.LoadOptions("display")) // JB 011124 Why load the wrong file?
	{
		DisplayOptions.DispWidth = Display.DispWidth;
		DisplayOptions.DispHeight = Display.DispHeight;
		DisplayOptions.DispDepth = Display.DispDepth;	// OW
		DisplayOptions.DispVideoCard = Display.DispVideoCard;
		DisplayOptions.DispVideoDriver = Display.DispVideoDriver;
		DisplayOptions.DispDepth = 32;	// Cobra - Always use 32-bit
	}
	else
	{
		DisplayOptions.DispWidth = 1024; // JB 011124
		DisplayOptions.DispHeight = 768; // JB 011124
		DisplayOptions.DispDepth = 32;	// Cobra - Always use 32-bit
		DisplayOptions.DispVideoCard = 0;
		DisplayOptions.DispVideoDriver = 0;
	}

	if(Player.LoadOptions("default"))
	{
/*		PlayerOptions.DispFlags = DISP_HAZING|DISP_GOURAUD|DISP_SHADOWS;
//		PlayerOptions.DispTextureLevel = 4;
		//PlayerOptions.DispTerrainDist = 64.0;
		PlayerOptions.DispTerrainDist = 80.0; // JB 011124
		PlayerOptions.DispMaxTerrainLevel = 0;
		PlayerOptions.ObjFlags = DISP_OBJ_TEXTURES;
		//PlayerOptions.SfxLevel = 4.0F;
		PlayerOptions.SfxLevel = 5.0F; // JB 011124
		//PlayerOptions.ObjDetailLevel = 1;
		PlayerOptions.ObjDetailLevel = 2; // JB 011124
		PlayerOptions.ObjMagnification = 1;
		PlayerOptions.ObjDeaggLevel = 100;	// 2001-11-09 M.N. from 60
		PlayerOptions.BldDeaggLevel = 5;	// 2001-11-09 M.N. from 3, Realism Patch default
		PlayerOptions.PlayerBubble = 1.0F;

		DisplayOptions.bRender2Texture = false;
		DisplayOptions.bAnisotropicFiltering = false;
		DisplayOptions.bLinearMipFiltering = false;
		DisplayOptions.bMipmapping = false;
		DisplayOptions.bZBuffering = false;
		DisplayOptions.bRender2DCockpit = false;
		DisplayOptions.bFontTexelAlignment = false;
		DisplayOptions.m_texMode = DisplayOptionsClass::TEX_MODE_DDS;*/
	}

	SetupGraphicsControls();
}

// OW
void AdvancedCB(long ID,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	C_Window *win;

	win=gMainHandler->FindWindow(SETUP_ADVANCED_WIN);
	if(!win) return;

	gMainHandler->ShowWindow(win);
	gMainHandler->WindowToFront(win);
}

// JPO - select advanced features
void AdvancedGameCB(long ID,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	C_Window *win;

	win=gMainHandler->FindWindow(ADVANCED_GAME_OPTIONS_WIN);	// JPOLOOK - not finished yet
	if(!win) return;

	gMainHandler->ShowWindow(win);
	gMainHandler->WindowToFront(win);
}

// M.N.	Skyfix
void SkyColorCB(long ID,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;
	C_Window *win;

	win=gMainHandler->FindWindow(SETUP_SKY_WIN);
	if(!win) return;

	gMainHandler->ShowWindow(win);
	gMainHandler->WindowToFront(win);
}
