/*

  UI Planner stuff Here

*/

#include <windows.h>
#include "Graphics\Include\RViewPnt.h"
#include "Graphics\Include\render3d.h"
#include "Graphics\Include\drawBSP.h"
#include "Graphics\Include\matrix.h"
#include "Graphics\Include\loader.h"
#include "entity.h"
#include "squadron.h"
#include "cmpclass.h"
#include "flight.h"
#include "find.h"
#include "cbsplist.h"
#include "cstores.h"
#include "cbsplist.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "c3dview.h"
#include "userids.h"
#include "textids.h"
#include "MissEval.h"
#include "falcsess.h"
#include "f4find.h"
#include "playerop.h"
#include "railinfo.h"

extern int PlannerLoaded;
extern C_Handler *gMainHandler;
extern C_Parser *gMainParser;
extern VU_ID gSelectedFlightID;
extern VU_ID gLoadoutFlightID;
extern VU_ID gPlayerFlightID;
extern VU_ID gActiveFlightID;
extern short gActiveWPNum;
extern Render3D *UIrend3d;
extern long PlaneEditList[];
extern long FirstPlane;
extern LoadoutStruct gCurStores[];
extern long HardPoints;
extern long PlaneCount;
extern BOOL ReadyToPlayMovie;
extern RailList gCurRails[4];
extern VehicleClassDataType *gVCPtr;
extern int gVehID;

extern OBJECTINFO Object,Recon;

void UI_Help_Guide_CB(long ID,short hittype,C_Base *ctrl);
void SetBullsEye(C_Window *);
void SetSlantRange(C_Window *);
void SetHeading(C_Window *);
extern RViewPoint *UIviewPoint;
extern C_TreeList *gATOAll,*gATOPackage,*gOOBTree;
void RedrawTreeWindowCB(long ID,short hittype,C_Base *control);
void refresh_waypoint (WayPointClass * wp);
BOOL ReconListSortCB(TREELIST *list,TREELIST *newitem);
void ReconArea(float x,float y,float range);
void BuildTargetList(float x,float y,float range);
void AssignVCCB(long ID,short hittype,C_Base *control);
void CloseReconWindowCB(long ID,short hittype,C_Base *control);
void UpdateRemoteCompression();

// 3d Object loading stuff
void PositionCamera(OBJECTINFO *Info,C_Window *win,long client);
void LoadFlight(VU_ID flightID);
void SetPlaneToArm(long Plane,BOOL ArmIt);
void SetupLoadoutDisplay();
//BOOL GetRackAndWeapon(VehicleClassDataType* vc,short VehID,short WeaponID,short count,short hardpoint,short center,RailInfo *rail);
BOOL GetJRackAndWeapon(VehicleClassDataType* vc, Falcon4EntityClassType *classPtr, 
		       short WeaponIndex, short count, short hardpoint, 
		       RailInfo *rail);
void LoadHardPoint(long plane,long hp,long center);
void ClearHardPoint(long plane,long hardpoint,long center,RailInfo *rail);
void LoadHardPoint(long plane,long hardpoint,long center,RailInfo *rail);
void ApplyWaypointChangesCB(long ID,short hittype,C_Base *control);
void LoadAFile(long TitleID,_TCHAR *filespec,_TCHAR *excludelist[],void (*YesCB)(long,short,C_Base*),void (*NoCB)(long,short,C_Base*));
void SaveAFile(long TitleID,_TCHAR *filespec,_TCHAR *excludelist[],void (*YesCB)(long,short,C_Base*),void (*NoCB)(long,short,C_Base*), _TCHAR *filename);
void ViewTimerCB(long ID,short hittype,C_Base *control);
void ViewObjectCB(long ID,short hittype,C_Base *control);
void MoveViewTimerCB(long ID,short hittype,C_Base *control);
void AssignTargetCB(long ID,short hittype,C_Base *control);
void DisplayView(long ID,short hittype,C_Base *control);
void ViewTimerAnimCB(long ID,short hittype,C_Base *control);
void CancelCampaignCompression (void);

void GetObjectivesNear(float x,float y,float range);
void SetupMunitionsWindow(VU_ID FlightID);
void SetCurrentLoadout();
void RestoreStores(C_Window *win);
void ClearStores(C_Window *win);
void UseStores();
void MakeStoresList(C_Window *win,long client);
void CloseWindowCB(long ID,short hittype,C_Base *control);
void ViewObjectCB(long ID,short hittype,C_Base *control);
void HookupPlannerControls(long ID);
void ToggleATOInfoCB(long ID,short hittype,C_Base *control);
void DelSTRFileCB(long ID,short hittype,C_Base *control);
void DelDFSFileCB(long ID,short hittype,C_Base *control);
void DelLSTFileCB(long ID,short hittype,C_Base *control);
void DelCamFileCB(long ID,short hittype,C_Base *control);
void DelTacFileCB(long ID,short hittype,C_Base *control);
void DelTGAFileCB(long ID,short hittype,C_Base *control);
void DelVHSFileCB(long ID,short hittype,C_Base *control);
void DelKeyFileCB(long ID,short hittype,C_Base *control);
void SetDeleteCallback(void (*cb)(long,short,C_Base*));
void GotoFlightCB(long ID,short hittype,C_Base *control);
void GotoPrevWaypointCB(long ID,short hittype,C_Base *control);
void GotoNextWaypointCB(long ID,short hittype,C_Base *control);
void set_waypoint_climb_mode(long ID,short hittype,C_Base *control);
void set_waypoint_enroute_action(long ID,short hittype,C_Base *control);
void set_waypoint_action(long ID,short hittype,C_Base *control);
void set_waypoint_formation(long ID,short hittype,C_Base *control);
void MinMaxWindowCB(long ID,short hittype,C_Base *control);
void MoveRendererCB(C_Window *win);
void PlaceLoadedWeapons(LoadoutStruct *loadout);
void MoveRendererClient0CB(C_Window *win);
void DeleteGroupList(long ID);
void ChangeTOSCB(long ID,short hittype,C_Base *control);
void ChangeTOSLockCB(long ID,short hittype,C_Base *control);
void ChangeAirspeedCB(long ID,short hittype,C_Base *control);
void ChangeAirspeedLockCB(long ID,short hittype,C_Base *control);
void ChangeAltCB(long ID,short hittype,C_Base *control);
void ChangePatrolCB(long ID,short hittype,C_Base *control);
void ToggleOOBTeamCB(long ID,short hittype,C_Base *control);
void ToggleOOBFilterCB(long ID,short hittype,C_Base *control);
void RemoveGPSItemCB(TREELIST *item);
void AreYouSure(long TitleID,long MessageID,void (*OkCB)(long,short,C_Base*),void (*CancelCB)(long,short,C_Base*));
void AreYouSure(long TitleID,_TCHAR *text,void (*OkCB)(long,short,C_Base*),void (*CancelCB)(long,short,C_Base*));
void SquadronAirUnitCB(long ID,short hittype,C_Base *control);
void PilotAirUnitCB(long ID,short hittype,C_Base *control);
void OpenPriorityCB(long ID,short hittype,C_Base *control);
void GetGroundUnitsNear(float x,float y,float range);
void OOBFindCB(long ID,short hittype,C_Base *control);
void OOBInfoCB(long ID,short hittype,C_Base *control);
void SquadronFindCB(long ID,short hittype,C_Base *control);
void CloseAllRenderers(long winID);
void UpdateInventoryCount();
//TJL 01/02/04 Change Skin function
void ChangeSkin();
extern int set3DTexture = -1;// this tells the skin change code nothing has been changed...yet.


// This stuff used for targetting
void (*OldReconCWCB)(long,short,C_Base*)=NULL;
VU_ID FeatureID=FalconNullId;
long FeatureNo=0;
extern C_TreeList *TargetTree;
extern C_TreeList *gVCTree;

extern long gFlightOverloaded;
extern bool g_bAllowOverload; // 2002-04-18 MN
extern bool g_bEmptyFilenameFix; // 2002-04-18 MN




void LoadPlannerWindows()
{
	long ID;
	C_Window *win;
	C_TimerHook *tmr;
	if(PlannerLoaded) return;

	if( _LOAD_ART_RESOURCES_)
		gMainParser->LoadImageList("pln_res.lst");
	else
		gMainParser->LoadImageList("pln_art.lst");
	gMainParser->LoadSoundList("pln_snd.lst");
	gMainParser->LoadWindowList("pln_scf.lst");		// Modified by M.N. - add art/art1024 by LoadWindowList

	ID=gMainParser->GetFirstWindowLoaded();
	while(ID)
	{
		HookupPlannerControls(ID);
		ID=gMainParser->GetNextWindowLoaded();
	}
	PlannerLoaded++;
	win=gMainHandler->FindWindow(RECON_WIN);
	if(win)
	{
		tmr=new C_TimerHook;
		tmr->Setup(C_DONT_CARE,C_TYPE_NORMAL);
		tmr->SetClient(0);
		tmr->SetXY(win->ClientArea_[0].left,win->ClientArea_[0].top);
		tmr->SetW(win->ClientArea_[0].right - win->ClientArea_[0].left);
		tmr->SetH(win->ClientArea_[0].bottom - win->ClientArea_[0].top);
		tmr->SetRefreshCallback(ViewTimerCB);
		tmr->SetDrawCallback(DisplayView);
		tmr->SetFlagBitOff(C_BIT_TIMER);
		tmr->SetReady(1);
		win->AddControl(tmr);

		tmr=new C_TimerHook;
		tmr->Setup(C_DONT_CARE,C_TYPE_TIMER);
		tmr->SetUpdateCallback(MoveViewTimerCB);
		tmr->SetUserNumber(_UI95_TIMER_DELAY_,2);
		tmr->SetFlagBitOn(C_BIT_ABSOLUTE);
		win->AddControl(tmr);

		// Add Drag CB

	}
	win=gMainHandler->FindWindow(MUNITIONS_WIN);
	if(win)
	{
		tmr=new C_TimerHook;
		tmr->Setup(C_DONT_CARE,C_TYPE_NORMAL);
		tmr->SetClient(2);
		tmr->SetXY(win->ClientArea_[2].left,win->ClientArea_[2].top);
		tmr->SetW(win->ClientArea_[2].right - win->ClientArea_[2].left);
		tmr->SetH(win->ClientArea_[2].bottom - win->ClientArea_[2].top);
		tmr->SetRefreshCallback(ViewTimerCB);
		tmr->SetDrawCallback(ViewObjectCB);
		tmr->SetFlagBitOff(C_BIT_TIMER);
		tmr->SetReady(1);
		win->AddControl(tmr);

		tmr=new C_TimerHook;
		tmr->Setup(C_DONT_CARE,C_TYPE_NORMAL);
		tmr->SetClient(2);
		tmr->SetXY(win->ClientArea_[2].left,win->ClientArea_[2].top);
		tmr->SetW(win->ClientArea_[2].right - win->ClientArea_[2].left);
		tmr->SetH(win->ClientArea_[2].bottom - win->ClientArea_[2].top);
		tmr->SetUpdateCallback(ViewTimerAnimCB);
		tmr->SetUserNumber(_UI95_TIMER_DELAY_,1);
		tmr->SetReady(1);
		win->AddControl(tmr);
	}
}


static void SpinViewCB(long ID,short hittype,C_Base *control)
{
	C_Button *btn;
	if(hittype != C_TYPE_LMOUSEUP) return;
	switch(ID)
	{
		case LEFT_SPIN:
			if(!((C_Button*)control)->GetState())
			{
				Recon.Direction=2;
				((C_Button*)control)->SetState(1);
				((C_Button*)control)->Refresh();
			}
			else
			{
				Recon.Direction=0;
				((C_Button*)control)->SetState(0);
				((C_Button*)control)->Refresh();
			}
			btn=(C_Button *)control->Parent_->FindControl(RIGHT_SPIN);
			if(btn)
			{
				btn->SetState(0);
				btn->Refresh();
			}
			break;
		case RIGHT_SPIN:
			if(!((C_Button*)control)->GetState())
			{
				Recon.Direction=-2;
				((C_Button*)control)->SetState(1);
				((C_Button*)control)->Refresh();
			}
			else
			{
				Recon.Direction=0;
				((C_Button*)control)->SetState(0);
				((C_Button*)control)->Refresh();
			}
			btn=(C_Button *)control->Parent_->FindControl(LEFT_SPIN);
			if(btn)
			{
				btn->SetState(0);
				btn->Refresh();
			}
			break;
	}
}

static void OverHeadCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP) return;
	Recon.Pitch=90;
	PositionCamera(&Recon,control->Parent_,0);
}

static void ViewPannerCB(long,short hittype,C_Base *control)
{
	float dx,dy;
	C_Panner *pnr;
	if(hittype != C_TYPE_LMOUSEUP && hittype != C_TYPE_REPEAT)
		return;

	pnr=(C_Panner*)control;
	dx=static_cast<float>(pnr->GetHRange());
	dy=static_cast<float>(pnr->GetVRange());

	if(dx < 0) dx=-2.5f;
	if(dx > 0) dx=2.5f;
	if(dy < 0) dy=-2.5f;
	if(dy > 0) dy=2.5f;

	if(dx)
	{
		Recon.Heading -= dx;
		if(Recon.Heading > 360.0f) Recon.Heading-=360.0f;
		if(Recon.Heading < 0.0f) Recon.Heading+=360.0f;
	}
	if(dy)
	{
		Recon.Pitch -= dy;
		if(Recon.Pitch > 90.0f) Recon.Pitch=90.0f;
		if(Recon.Pitch < 10.0f) Recon.Pitch=10.0f;
	}

	PositionCamera(&Recon,control->Parent_,0);
}

static void ZoomPannerCB(long,short hittype,C_Base *control)
{
	C_Panner *pnr=(C_Panner*)control;
	C_Text *txt;
	_TCHAR buffer[20];
	int dx;
	if(hittype != C_TYPE_LMOUSEUP && hittype != C_TYPE_REPEAT)
		return;

	dx=pnr->GetVRange();

	Recon.Distance += dx*10;

	if(Recon.Distance < Recon.MinDistance)
		Recon.Distance=Recon.MinDistance;
	if(Recon.Distance > Recon.MaxDistance)
		Recon.Distance=Recon.MaxDistance;

	txt=(C_Text*)control->Parent_->FindControl(SLANT_RANGE);
	if(txt)
	{
		txt->Refresh();
		_stprintf(buffer,"%1ld %s",(long)Recon.Distance,gStringMgr->GetString(TXT_FT));
		txt->SetText(buffer);
		txt->Refresh();
	}

	PositionCamera(&Recon,control->Parent_,0);
}

void AssignTargetCB(long ID,short hittype,C_Base *control)
{
	Flight flight;
	WayPoint wp;
	long i;

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	CloseReconWindowCB(ID,hittype,control);

	if(FeatureID != FalconNullId && FeatureNo >= 0)
	{
		// Set Target
		flight=(Flight)vuDatabase->Find(gActiveFlightID);
		if(flight)
		{
			i=1;
			wp=flight->GetFirstUnitWP();
			while(i < gActiveWPNum && wp)
			{
				wp=wp->GetNextWP();
				i++;
			}
			if(wp)
			{
				wp->SetWPTarget(FeatureID);
				wp->SetWPTargetBuilding(static_cast<uchar>(FeatureNo));
				refresh_waypoint(wp);
			}
		}
	}
}

void CloseReconWindowCB(long,short hittype,C_Base *control)
{
	long Flags1,Flags2;
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	gMainHandler->HideWindow(control->Parent_);

	Flags1=gMainHandler->GetWindowFlags(RECON_WIN);
	Flags2=gMainHandler->GetWindowFlags(RECON_LIST_WIN);

	if(!(Flags1 & C_BIT_ENABLED) && !(Flags2 & C_BIT_ENABLED))
	{
		if(gUIViewer)
		{
			gUIViewer->Cleanup();
			delete gUIViewer;
			gUIViewer=NULL;
		}
	}
}

void TgtAssignCWCB(long ID,short hittype,C_Base *control)
{
	C_Base *btn;

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	btn=control->Parent_->FindControl(SET_TARGET);
	if(btn)
		btn->SetFlagBitOn(C_BIT_INVISIBLE);

	btn=control->Parent_->FindControl(SET_VC);
	if(btn)
		btn->SetFlagBitOn(C_BIT_INVISIBLE);

	control->SetCallback(OldReconCWCB);
	OldReconCWCB=NULL;

	CloseReconWindowCB(ID,hittype,control);
}

static void OpenReconWindowCB(long,short hittype,C_Base *)
{
	C_Window *win;
	C_Base *btn;
	Flight flight;
	WayPoint wp;
	int i;
	float x,y,z;

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	SetCursor(gCursors[CRSR_WAIT]);

	win=gMainHandler->FindWindow(RECON_LIST_WIN);
	if(win)
	{
		CloseAllRenderers(RECON_WIN);
		if(TargetTree)
			TargetTree->DeleteBranch(TargetTree->GetRoot());

		if(!OldReconCWCB)
		{
			btn=win->FindControl(CLOSE_WINDOW);
			if(btn)
			{
				OldReconCWCB=btn->GetCallback();
				btn->SetCallback(TgtAssignCWCB);
			}
		}
		btn=win->FindControl(SET_TARGET);
		if(btn)
			btn->SetFlagBitOff(C_BIT_INVISIBLE);

		flight=(Flight)vuDatabase->Find(gActiveFlightID);
		if(flight == NULL) return;
		i=1;
		wp=flight->GetFirstUnitWP();
		while(i < gActiveWPNum && wp)
		{
			wp=wp->GetNextWP();
			i++;
		}
		if(wp)
		{
			wp->GetLocation(&x,&y,&z);
			BuildTargetList(x,y,18000.0f);
		}
	}
}

void OpenReconForVCCB(long,short hittype,C_Base *)
{
	C_Window *win;
	C_Base *btn;

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	SetCursor(gCursors[CRSR_WAIT]);

	win=gMainHandler->FindWindow(RECON_LIST_WIN);
	if(win)
	{
		CloseAllRenderers(RECON_WIN);
		if(TargetTree)
			TargetTree->DeleteBranch(TargetTree->GetRoot());

		if(!OldReconCWCB)
		{
			btn=win->FindControl(CLOSE_WINDOW);
			if(btn)
			{
				OldReconCWCB=btn->GetCallback();
				btn->SetCallback(TgtAssignCWCB);
			}
		}
		btn=win->FindControl(SET_VC);
		if(btn)
			btn->SetFlagBitOff(C_BIT_INVISIBLE);

		// Find where user clicked... and do recon, OR pick as target
		// Set Cur VC's Target
	}
}

void ClosePlannerWindowCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	if(control->GetGroup())
		gMainHandler->DisableWindowGroup(control->GetGroup());
}

static void ObjectPannerCB(long,short hittype,C_Base *control)
{
	float dx,dy;
	C_Panner *pnr;

	if(hittype != C_TYPE_LMOUSEUP && hittype != C_TYPE_REPEAT)
		return;

	pnr=(C_Panner*)control;
	dx=static_cast<float>(pnr->GetHRange());
	dy=static_cast<float>(pnr->GetVRange());

	Object.Heading+=dx;
	Object.Pitch+=dy;
	if(Object.Heading < 0) Object.Heading+=360;
	if(Object.Heading > 360) Object.Heading-=360;
	if(Object.CheckPitch)
	{
		if(Object.Pitch < Object.MinPitch) Object.Pitch=Object.MinPitch;
		if(Object.Pitch < Object.MaxPitch) Object.Pitch=Object.MaxPitch;
	}
	else
	{
		if(Object.Pitch < 0) Object.Pitch+=360;
		if(Object.Pitch > 360) Object.Pitch-=360;
	}
	PositionCamera(&Object,control->Parent_,control->GetClient());
}

static void ObjectZoomCB(long,short hittype,C_Base *control)
{
	float dy;
	C_Panner *pnr;

	if(hittype != C_TYPE_LMOUSEUP && hittype != C_TYPE_REPEAT)
		return;

	pnr=(C_Panner*)control;
	dy=static_cast<float>(pnr->GetVRange());

	Object.Distance+=dy;
	if(Object.Distance < Object.MinDistance) Object.Distance=Object.MinDistance;
	if(Object.Distance > Object.MaxDistance) Object.Distance=Object.MaxDistance;

	PositionCamera(&Object,control->Parent_,control->GetClient());
}

void OpenMunitionsWindowCB(long,short hittype,C_Base *control)
{
	F4CSECTIONHANDLE *Leave;
	Flight flight;
	C_Window *win;

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	if(gSelectedFlightID == FalconNullId)
		return;

	SetCursor(gCursors[CRSR_WAIT]);

	// TODO: Check to see if the flight has already taken off
	// if so... open Error window... and don't do anything
	flight=(Flight)vuDatabase->Find(gSelectedFlightID);
	if(!flight) return;
	if(!flight->IsFlight()) return;

	if(control->GetGroup())
	{
		if(UIrend3d == NULL)
		{
			win=gMainHandler->FindWindow(MUNITIONS_WIN);
			if(win)
			{
				CloseAllRenderers(MUNITIONS_WIN);
				Leave=UI_Enter(win);
				if(gUIViewer)
				{
					gUIViewer->Cleanup();
					delete gUIViewer;
				}
				gUIViewer=new C_3dViewer;
				gUIViewer->Setup();
				gUIViewer->Viewport(win,2);
				gUIViewer->Init3d(30.0f);

				gLoadoutFlightID=gSelectedFlightID;
				SetupMunitionsWindow(gLoadoutFlightID);
				LoadFlight(gLoadoutFlightID);
				SetupLoadoutDisplay();
				UpdateInventoryCount();
				MakeStoresList(win,1);
				win->RefreshClient(1);
				TheLoader.WaitLoader();

				PositionCamera(&Object,win,2);
				UI_Leave(Leave);
			}
		}
		gMainHandler->EnableWindowGroup(control->GetGroup());
	}
	SetCursor(gCursors[CRSR_F16]);
}

void ClearAllHardPointBSPs(void); 

void CloseMunitionsWindowCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	if(control->GetGroup())
		gMainHandler->DisableWindowGroup(control->GetGroup());

	if(gUIViewer)
	{
		ClearAllHardPointBSPs(); // MLR 2/29/2004 - else CTD when reentering the loadout
		gUIViewer->Cleanup();
		delete gUIViewer;
		gUIViewer=NULL;
	}
}

static void SelectPlaneToArmCB(long ID,short hittype,C_Base *control)
{
	int i,idx,count;

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	switch(ID)
	{
		case AIR_1:
			idx=0;
			break;
		case AIR_2:
			idx=1;
			break;
		case AIR_3:
			idx=2;
			break;
		case AIR_4:
			idx=3;
			break;
		default:
			return;
	}

	count=0;
	for(i=0;i<4;i++)
		if(PlaneEditList[i])
			count++;

	if(((C_Button *)control)->GetState())
	{
		if(count < 2)
			return;

		((C_Button *)control)->SetState(0);
		SetPlaneToArm(idx,FALSE);
	}
	else
	{
		((C_Button *)control)->SetState(1);
		SetPlaneToArm(idx,TRUE);
	}
	SetCurrentLoadout();
	control->Refresh();
	control->Parent_->RefreshClient(2);
}

static void ChooseWeaponListCB(long,short hittype,C_Base *)
{
	C_Window *win;
	F4CSECTIONHANDLE *Leave;
	if(hittype != C_TYPE_SELECT)
		return;

	win=gMainHandler->FindWindow(MUNITIONS_WIN);
	if(win)
	{
		Leave=UI_Enter(win);
		MakeStoresList(win,1);
		win->RefreshWindow();
		//win->RefreshClient(1);
		UI_Leave(Leave);
	}
}

static void LoadTheStoresCB(long,short hittype,C_Base *control)
{
	FILE *ifp;
	LoadoutStruct hp;
	long i,j;
	_TCHAR fname[MAX_PATH];
	C_EditBox *ebox;
	C_Window *win;
	F4CSECTIONHANDLE *Leave;

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	gMainHandler->HideWindow(control->Parent_);

	ebox=(C_EditBox*)control->Parent_->FindControl(FILE_NAME);
	if(ebox)
	{
		_stprintf(fname,"%s\\%s.str",FalconCampUserSaveDirectory,ebox->GetText());
		ifp=fopen(fname,"rb");
		if(ifp == NULL)
			return;

		fread(&hp,sizeof(LoadoutStruct),1,ifp);
		fclose(ifp);

		PlaceLoadedWeapons(&hp);

		win=gMainHandler->FindWindow(MUNITIONS_WIN);
		if(win)
		{
			Leave=UI_Enter(win);
			for(i=0;i<4;i++)
				if(PlaneEditList[i])
					for(j=1;j<HardPoints;j++)
					{
						ClearHardPoint(i,j,static_cast<short>(HardPoints/2),&gCurRails[i].rail[j]);
						//if(GetRackAndWeapon(gVCPtr,static_cast<short>(gVehID),gCurStores[i].WeaponID[j],gCurStores[i].WeaponCount[j],static_cast<short>(j),static_cast<short>(HardPoints / 2),&gCurRails[i].rail[j]))
						if(GetJRackAndWeapon(gVCPtr,&Falcon4ClassTable[gVehID],gCurStores[i].WeaponID[j],gCurStores[i].WeaponCount[j],static_cast<short>(j),&gCurRails[i].rail[j])) // MLR 2/29/2004 - 
							LoadHardPoint(i,j,static_cast<short>(HardPoints/2),&gCurRails[i].rail[j]);
					}

			MakeStoresList(win,1);
			win->RefreshWindow();
			UI_Leave(Leave);
		}
	}
}

static void LoadStoresCB(long,short hittype,C_Base *)
{
	_TCHAR fname[MAX_PATH];

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	SetDeleteCallback(DelSTRFileCB);
	_stprintf(fname,"%s\\*.str",FalconCampUserSaveDirectory);
	LoadAFile(TXT_LOAD_STORES,fname,NULL,LoadTheStoresCB,CloseWindowCB);
}

static void SaveTheStoresCB(long,short hittype,C_Base *control)
{
	_TCHAR fname[MAX_PATH];
	FILE *ofp;
	C_EditBox *ebox;
	C_Window *win;

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	win=gMainHandler->FindWindow(SAVE_WIN);
	if(!win)
		return;

	gMainHandler->HideWindow(win);
	gMainHandler->HideWindow(control->Parent_);

	ebox=(C_EditBox*)win->FindControl(FILE_NAME);
	if(ebox)
	{
		_stprintf(fname,"%s\\%s.str",FalconCampUserSaveDirectory,ebox->GetText());
		ofp=fopen(fname,"wb");
		if(ofp == NULL)
			return;

		fwrite(&gCurStores[FirstPlane],sizeof(LoadoutStruct),1,ofp);
		fclose(ofp);
	}
}

static void VerifySaveTheStoresCB(long ID,short hittype,C_Base *control)
{
	_TCHAR fname[MAX_PATH];
	FILE *ofp;
	C_EditBox *ebox;

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	ebox=(C_EditBox*)control->Parent_->FindControl(FILE_NAME);
	if(ebox)
	{
		//dpc EmptyFilenameSaveFix, modified by MN - added a warning to enter a filename
		if (g_bEmptyFilenameFix)
		{
			if (_tcslen(ebox->GetText()) == 0)
			{
				AreYouSure(TXT_WARNING, TXT_ENTER_FILENAME,CloseWindowCB,CloseWindowCB);
				return;
			}
		}
		//end EmptyFilenameSaveFix
		_stprintf(fname,"%s\\%s.str",FalconCampUserSaveDirectory,ebox->GetText());
		ofp=fopen(fname,"r");
		if(ofp)
		{
			fclose(ofp);
			AreYouSure(TXT_SAVE_STORES,TXT_FILE_EXISTS,SaveTheStoresCB,CloseWindowCB);
		}
		else
			SaveTheStoresCB(ID,hittype,control);
	}
}

static void SaveStoresCB(long,short hittype,C_Base *)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	SetDeleteCallback(DelSTRFileCB);
//dpc SaveStoresListFix
	if (g_bEmptyFilenameFix)
	{
		_TCHAR fname[MAX_PATH];
		_stprintf(fname,"%s\\*.str",FalconCampUserSaveDirectory);
		SaveAFile(TXT_SAVE_STORES,fname,NULL,VerifySaveTheStoresCB,CloseWindowCB,"");
	}
	else
//end SaveStoresListFix
	SaveAFile(TXT_SAVE_STORES,"*.str",NULL,VerifySaveTheStoresCB,CloseWindowCB,"");
}

static void RestoreStoresCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	RestoreStores(control->Parent_);
}

static void ClearStoresCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	ClearStores(control->Parent_);
}

//TJL 01/02/04 Change_Skin stuff
static void ChangeSkinCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	//add code;
	
	ChangeSkin();

	return;
}



static void UseStoresCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	if(gFlightOverloaded)
	{
		if (g_bAllowOverload)
		{
			AreYouSure(TXT_WARNING,TXT_OVERLOADED,CloseWindowCB,CloseWindowCB);
		}
		else
		{
			AreYouSure(TXT_ERROR,TXT_OVERLOADED,NULL,CloseWindowCB);
			return;
		}
	}

	UseStores();
	if(control->GetGroup())
		gMainHandler->DisableWindowGroup(control->GetGroup());

	if(gUIViewer)
	{
		ClearAllHardPointBSPs(); // MLR 2/29/2004 - clear hardpoint drawables
		gUIViewer->Cleanup();
		delete gUIViewer;
		gUIViewer=NULL;
	}

	// KCK: We need to update the mission evaluation stuff if this is the selected flight
	if (gLoadoutFlightID == gPlayerFlightID)
	{
		Flight	flight = (Flight) vuDatabase->Find(gLoadoutFlightID);
		int		pilotSlot = 255;
		if (!flight)
			return;
		pilotSlot = FalconLocalSession->GetPilotSlot();
		TheCampaign.MissionEvaluator->PreMissionEval(flight, static_cast<uchar>(pilotSlot));
	}
}

static void OpenReconWinCB(long,short hittype,C_Base *)
{
	C_Window *win;

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	win=gMainHandler->FindWindow(RECON_WIN);
	if(win)
	{
		CloseAllRenderers(RECON_WIN);
		SetCursor(gCursors[CRSR_WAIT]);
		TheLoader.WaitLoader();
		PositionCamera(&Recon,win,0);
		gMainHandler->ShowWindow(win);
		gMainHandler->WindowToFront(win);
		SetCursor(gCursors[CRSR_F16]);
	}
}

static void OpenTargetWinCB(long,short hittype,C_Base *)
{
	C_Window *win;

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	win=gMainHandler->FindWindow(RECON_LIST_WIN);
	if(win)
	{
		CloseAllRenderers(RECON_WIN);
		gMainHandler->ShowWindow(win);
		gMainHandler->WindowToFront(win);
	}
}

static void CampaignAbortTakeoffCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	CancelCampaignCompression();
	UpdateRemoteCompression();
	gMainHandler->HideWindow(control->Parent_);
	ReadyToPlayMovie=TRUE;
}

static void CampaignTaxiCB(long ID,short hittype,C_Base *)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;
	Flight fl;
	fl = FalconLocalSession->GetPlayerFlight();

	// RV - Biker - Disable ramp and taxi for carrier take off
	CampEntity ent = NULL;

	if (fl)
		ent = fl->GetUnitAirbase();

	if (ent && ent->IsTaskForce())
		ID = WAIT_TAKEOFF;

	switch (ID) {
	case WAIT_TAXI:
		PlayerOptions.SetStartFlag(PlayerOptionsClass::START_TAXI);
		if(fl) fl->ClearEvalFlag(FEVAL_START_COLD);
		break;
	case WAIT_TAKEOFF:
		PlayerOptions.SetStartFlag(PlayerOptionsClass::START_RUNWAY);
		if(fl) fl->ClearEvalFlag(FEVAL_START_COLD);
		break;
	case WAIT_RAMP:
		PlayerOptions.SetStartFlag(PlayerOptionsClass::START_RAMP);
		if(fl) fl->SetEvalFlag(FEVAL_START_COLD);
		break;
	default:
	    ShiWarning("Unexpected taxi ID");
	}
}

static BOOL CampaignCountDownCB(C_Base *me)
{
	Flight fl;
	WayPoint w;
	_TCHAR buffer[30];
	C_Text *txt;
	long time,hour,minute,second,day;
	int deltaTime = 0;

	txt=(C_Text*)me;
	if(!txt)
		return(FALSE);

	fl = FalconLocalSession->GetPlayerFlight();
	if(fl)
	{
		w = fl->GetCurrentUnitWP();
		if (w && w->GetWPAction() == WP_TAKEOFF)
			deltaTime = (w->GetWPArrivalTime() - vuxGameTime) / VU_TICS_PER_SECOND;
	}

	if (deltaTime < 0)
		deltaTime = 0;

	if(deltaTime != me->GetUserNumber(0))
	{
		txt->SetUserNumber(0,deltaTime);
		day=(long)(deltaTime/(24*60*60));
		time=(long)((deltaTime)-(day*24*60*60));
		hour=time/(60*60);
		minute=(time/60)%60;
		second=time%60;
// 2002-04-18 MN HACK HACK HACK - sometimes the clock goes to 17 hours, 47 minutes - 
// god knows why...let's just set the timer to 00:00:00 when that happens...
		if (hour >= 17)
		{
			hour = 0;
			minute = 0;
			second = 0;
		}
		_stprintf(buffer,"%02d:%02d:%02d",hour,minute,second);
		txt->SetText(buffer);
		txt->Refresh();
		return(TRUE);
	}
	return(FALSE);
}

void HookupPlannerControls(long ID)
{
	C_Window *winme;
	C_Button *ctrl;
	C_ListBox *lbox;
	C_Panner *pnr;
	C_Text *txt;
	C_TreeList *tree;
	winme=gMainHandler->FindWindow(ID);

	if(winme == NULL)
		return;

	// Hook up IDs here

	ctrl=(C_Button*)winme->FindControl(SET_PRIORITIES);
	if(ctrl)
		ctrl->SetCallback(OpenPriorityCB);

	// Time/Date CB
	txt=(C_Text *)winme->FindControl(COUNTDOWN_ID);
	if(txt)
	{
		txt->SetTimerCallback(CampaignCountDownCB);
		txt->SetFlagBitOn(C_BIT_TIMER);
		txt->Refresh();
	}

	// Hook up Close Button
	ctrl=(C_Button *)winme->FindControl(CLOSE_WINDOW);
	if(ctrl)
	{
		if(ID == RECON_WIN)
		{
			ctrl->SetCallback(CloseReconWindowCB);
			winme->SetDragCallback(MoveRendererClient0CB);
		}
		else if(ID == RECON_LIST_WIN)
		{
			ctrl->SetCallback(CloseReconWindowCB);
		}
		else if(ID == MUNITIONS_WIN)
		{
			ctrl->SetCallback(CloseMunitionsWindowCB);
			winme->SetDragCallback(MoveRendererCB);
		}
		else
			ctrl->SetCallback(ClosePlannerWindowCB);
	}

	lbox=(C_ListBox *)winme->FindControl(LID_FLIGHT_CALLSIGN);
	if(lbox)
		lbox->SetCallback(GotoFlightCB);

	ctrl=(C_Button*)winme->FindControl(WAIT_TAXI);
	if(ctrl)
		ctrl->SetCallback(CampaignTaxiCB);

	ctrl=(C_Button*)winme->FindControl(WAIT_TAKEOFF);
	if(ctrl)
		ctrl->SetCallback(CampaignTaxiCB);

	ctrl=(C_Button*)winme->FindControl(WAIT_RAMP);
	if(ctrl)
		ctrl->SetCallback(CampaignTaxiCB);

	ctrl=(C_Button*)winme->FindControl(WAIT_BACK);
	if(ctrl)
		ctrl->SetCallback(CampaignAbortTakeoffCB);

	ctrl=(C_Button *)winme->FindControl(ALL_CTRL);
	if(ctrl)
		ctrl->SetCallback(ToggleATOInfoCB);

	ctrl=(C_Button *)winme->FindControl(PREV_STPT);
	if(ctrl)
		ctrl->SetCallback(GotoPrevWaypointCB);

	ctrl=(C_Button *)winme->FindControl(NEXT_STPT);
	if(ctrl)
		ctrl->SetCallback(GotoNextWaypointCB);

	lbox=(C_ListBox *)winme->FindControl(ENR_ACTION_LISTBOX);
	if (lbox)
		lbox->SetCallback (set_waypoint_enroute_action);

	lbox=(C_ListBox *)winme->FindControl(ACTION_LISTBOX);
	if (lbox)
		lbox->SetCallback (set_waypoint_action);

	lbox=(C_ListBox *)winme->FindControl(FORM_LISTBOX);
	if (lbox)
		lbox->SetCallback (set_waypoint_formation);

	lbox=(C_ListBox *)winme->FindControl(CLIMB_LISTBOX);
	if (lbox)
		lbox->SetCallback (set_waypoint_climb_mode);

	ctrl=(C_Button *)winme->FindControl(MINIMIZE_WINDOW);
	if(ctrl)
		ctrl->SetCallback(MinMaxWindowCB);

	ctrl=(C_Button *)winme->FindControl(TGT_ASSIGN);
	if(ctrl)
		ctrl->SetCallback(OpenReconWindowCB);

	ctrl=(C_Button *)winme->FindControl(SET_TARGET);
	if(ctrl)
		ctrl->SetCallback(AssignTargetCB);

	ctrl=(C_Button *)winme->FindControl(SET_VC);
	if(ctrl)
		ctrl->SetCallback(AssignVCCB);

	ctrl=(C_Button *)winme->FindControl(LEFT_SPIN);
	if(ctrl)
		ctrl->SetCallback(SpinViewCB);

	ctrl=(C_Button *)winme->FindControl(RIGHT_SPIN);
	if(ctrl)
		ctrl->SetCallback(SpinViewCB);

	ctrl=(C_Button *)winme->FindControl(RECON_OVERHEAD);
	if(ctrl)
		ctrl->SetCallback(OverHeadCB);

	pnr=(C_Panner*)winme->FindControl(RECON_PANNER);
	if(pnr)
		pnr->SetCallback(ViewPannerCB);

	pnr=(C_Panner*)winme->FindControl(ZOOMER);
	if(pnr)
		pnr->SetCallback(ZoomPannerCB);

	pnr=(C_Panner*)winme->FindControl(PLANE_PANNER);
	if(pnr)
		pnr->SetCallback(ObjectPannerCB);

	pnr=(C_Panner*)winme->FindControl(PLANE_ZOOMER);
	if(pnr)
		pnr->SetCallback(ObjectZoomCB);

	ctrl=(C_Button *)winme->FindControl(AIR_1);
	if(ctrl)
		ctrl->SetCallback(SelectPlaneToArmCB);

	ctrl=(C_Button *)winme->FindControl(AIR_2);
	if(ctrl)
		ctrl->SetCallback(SelectPlaneToArmCB);

	ctrl=(C_Button *)winme->FindControl(AIR_3);
	if(ctrl)
		ctrl->SetCallback(SelectPlaneToArmCB);

	ctrl=(C_Button *)winme->FindControl(AIR_4);
	if(ctrl)
		ctrl->SetCallback(SelectPlaneToArmCB);

	lbox=(C_ListBox *)winme->FindControl(WEAPON_LIST_CTRL);
	if(lbox)
		lbox->SetCallback(ChooseWeaponListCB);

	ctrl=(C_Button *)winme->FindControl(SAVE_AS);
	if(ctrl)
		ctrl->SetCallback(SaveStoresCB);

	ctrl=(C_Button *)winme->FindControl(LOAD);
	if(ctrl)
		ctrl->SetCallback(LoadStoresCB);

	ctrl=(C_Button *)winme->FindControl(RESTORE);
	if(ctrl)
		ctrl->SetCallback(RestoreStoresCB);

	ctrl=(C_Button *)winme->FindControl(CLEAR);
	if(ctrl)
		ctrl->SetCallback(ClearStoresCB);

	ctrl=(C_Button *)winme->FindControl(CANCEL);
	if(ctrl)
		ctrl->SetCallback(CloseMunitionsWindowCB);

	ctrl=(C_Button *)winme->FindControl(OK);
	if(ctrl)
		ctrl->SetCallback(UseStoresCB);

	//TJL 01/02/04 Adding Change_Skin Button

	ctrl=(C_Button *)winme->FindControl(CHANGE_SKIN);
	if(ctrl)
		ctrl->SetCallback(ChangeSkinCB);
	//End

	ctrl=(C_Button *)winme->FindControl(TOS_EARLIER);
	if(ctrl)
		ctrl->SetCallback(ChangeTOSCB);

	ctrl=(C_Button *)winme->FindControl(TOS_LATER);
	if(ctrl)
		ctrl->SetCallback(ChangeTOSCB);

	ctrl=(C_Button *)winme->FindControl(PATROL_LESS);
	if(ctrl)
		ctrl->SetCallback(ChangePatrolCB);

	ctrl=(C_Button *)winme->FindControl(PATROL_LONGER);
	if(ctrl)
		ctrl->SetCallback(ChangePatrolCB);

	ctrl=(C_Button *)winme->FindControl(AIRSPEED_DECR);
	if(ctrl)
		ctrl->SetCallback(ChangeAirspeedCB);

	ctrl=(C_Button *)winme->FindControl(AIRSPEED_INC);
	if(ctrl)
		ctrl->SetCallback(ChangeAirspeedCB);

	ctrl=(C_Button *)winme->FindControl(ALT_DECR);
	if(ctrl)
		ctrl->SetCallback(ChangeAltCB);

	ctrl=(C_Button *)winme->FindControl(ALT_INC);
	if(ctrl)
		ctrl->SetCallback(ChangeAltCB);

	ctrl=(C_Button *)winme->FindControl(TOS_LOCK);
	if(ctrl)
		ctrl->SetCallback(ChangeTOSLockCB);

	ctrl=(C_Button *)winme->FindControl(AIRSPEED_LOCK);
	if(ctrl)
		ctrl->SetCallback(ChangeAirspeedLockCB);

	ctrl=(C_Button *)winme->FindControl(GROUP1_FLAG);
	if(ctrl)
		ctrl->SetCallback(ToggleOOBTeamCB);

	ctrl=(C_Button *)winme->FindControl(GROUP2_FLAG);
	if(ctrl)
		ctrl->SetCallback(ToggleOOBTeamCB);

	ctrl=(C_Button *)winme->FindControl(GROUP3_FLAG);
	if(ctrl)
		ctrl->SetCallback(ToggleOOBTeamCB);

	ctrl=(C_Button *)winme->FindControl(GROUP4_FLAG);
	if(ctrl)
		ctrl->SetCallback(ToggleOOBTeamCB);

	ctrl=(C_Button *)winme->FindControl(GROUP5_FLAG);
	if(ctrl)
		ctrl->SetCallback(ToggleOOBTeamCB);

	ctrl=(C_Button *)winme->FindControl(GROUP6_FLAG);
	if(ctrl)
		ctrl->SetCallback(ToggleOOBTeamCB);

	ctrl=(C_Button *)winme->FindControl(GROUP7_FLAG);
	if(ctrl)
		ctrl->SetCallback(ToggleOOBTeamCB);

	ctrl=(C_Button *)winme->FindControl(GROUP8_FLAG);
	if(ctrl)
		ctrl->SetCallback(ToggleOOBTeamCB);

	ctrl=(C_Button *)winme->FindControl(AF_FILTER);
	if(ctrl)
		ctrl->SetCallback(ToggleOOBFilterCB);

	ctrl=(C_Button *)winme->FindControl(ARMY_FILTER);
	if(ctrl)
		ctrl->SetCallback(ToggleOOBFilterCB);

	ctrl=(C_Button *)winme->FindControl(NAVY_FILTER);
	if(ctrl)
		ctrl->SetCallback(ToggleOOBFilterCB);

	ctrl=(C_Button *)winme->FindControl(OBJECTIVE_FILTER);
	if(ctrl)
		ctrl->SetCallback(ToggleOOBFilterCB);

	ctrl=(C_Button *)winme->FindControl(UNIT_SQUADRON_TAB);
	if(ctrl)
		ctrl->SetCallback(SquadronAirUnitCB);

	ctrl=(C_Button *)winme->FindControl(UNIT_PILOT_TAB);
	if(ctrl)
		ctrl->SetCallback(PilotAirUnitCB);

	ctrl=(C_Button *)winme->FindControl(OOB_UNIT_FIND);
	if(ctrl)
		ctrl->SetCallback(OOBFindCB);

	ctrl=(C_Button *)winme->FindControl(OOB_UNIT_INFO);
	if(ctrl)
		ctrl->SetCallback(OOBInfoCB);

	ctrl=(C_Button *)winme->FindControl(FIND_UNIT);
	if(ctrl)
		ctrl->SetCallback(SquadronFindCB);

	ctrl=(C_Button *)winme->FindControl(OPEN_RECON_TARGETS);
	if(ctrl)
		ctrl->SetCallback(OpenTargetWinCB);

	ctrl=(C_Button *)winme->FindControl(OPEN_RECON);
	if(ctrl)
		ctrl->SetCallback(OpenReconWinCB);

	tree=(C_TreeList *)winme->FindControl(ATO_ALL_TREE);
	if(tree)
	{
		tree->SetCallback(RedrawTreeWindowCB);
		gATOAll=tree;
	}
	tree=(C_TreeList *)winme->FindControl(ATO_PACKAGE_TREE);
	if(tree)
	{
		tree->SetCallback(RedrawTreeWindowCB);
		gATOPackage=tree;
	}
	tree=(C_TreeList *)winme->FindControl(OOB_TREE);
	if(tree)
	{
		tree->SetCallback(RedrawTreeWindowCB);
		gOOBTree=tree;
	}
	tree=(C_TreeList *)winme->FindControl(RECON_TREE);
	if(tree)
	{
		TargetTree=tree;
		tree->SetSortType(TREE_SORT_CALLBACK);
		tree->SetSortCallback(ReconListSortCB);
	}
// Help GUIDE thing
	ctrl=(C_Button*)winme->FindControl(UI_HELP_GUIDE);
	if(ctrl)
		ctrl->SetCallback(UI_Help_Guide_CB);

}
