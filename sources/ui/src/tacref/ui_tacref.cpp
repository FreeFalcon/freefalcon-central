#include <windows.h>
#include "Graphics\Include\TimeMgr.h"
#include "Graphics\Include\imagebuf.h"
#include "dxutil\ddutil.h"
#include "dispcfg.h"
#include "Graphics\Include\renderow.h"
#include "Graphics\Include\RViewPnt.h"
#include "Graphics\Include\drawbsp.h"
#include "unit.h"
#include "classtbl.h"
#include "cmpclass.h"
#include "chandler.h"
#include "cbsplist.h"
#include "Graphics\Include\loader.h"
#include "c3dview.h"
#include "soundfx.h"
#include "fsound.h"
#include "tacref.h"
#include "userids.h"
#include "textids.h"
//TJL 12/27/03
#include "sim\include\otwdrive.h"
//#include "simbase.h"

enum
{
	TACREF_BITMAP_ID=999888777,
};

extern C_Handler *gMainHandler;
extern C_Parser *gMainParser;
extern int TACREFLoaded;

// RV - Biker - Theater switching stuff
extern char FalconTacrefThrDirectory[];

TacticalReference *Reference=NULL;
C_3dViewer *TAC_Viewer=NULL;
long CurrentModel=0;
long CurrentWeapon=0;
long CurrentEntity=0;
BOOL Helicopter=FALSE;
float BladeAngle=0.0f;
int LastRWRTone=-1;
int LastLockTone=-1;
//TJL 12/28/03
int prevtext = 0;

OBJECTINFO TACREF_Object;

void UI_Help_Guide_CB(long ID,short hittype,C_Base *control);
void CloseWindowCB(long ID,short hittype,C_Base *control);
void FindCameraDeltas(OBJECTINFO *Info);
void UnloadObject();
void TacRef_Cleanup();
void PositandOrientSetData (float x, float y, float z, float pitch, float roll, float yaw,
                            Tpoint* simView, Trotation* viewRotation);

enum
{
	GROUP_COUNT=3,
	SUBGROUP_COUNT=18,
};

static long GroupButtonID[GROUP_COUNT]=
{
	CAT_AIRCRAFT,
	CAT_VEHICLES,
	CAT_MUNITIONS,
};

static long SubGroupButtonID[SUBGROUP_COUNT]=
{
	SUB_CAT_AIRCRAFT_FIGHTERS,
	SUB_CAT_AIRCRAFT_ATTACK,
	SUB_CAT_AIRCRAFT_BOMBERS,
	SUB_CAT_AIRCRAFT_HELICOPTERS,
	SUB_CAT_AIRCRAFT_SUPPORT,
	SUB_CAT_AIRCRAFT_EW,
	SUB_CAT_VEHICLES_TANKS,
	SUB_CAT_VEHICLES_IFVS,
	SUB_CAT_VEHICLES_ARTILLERY,
	SUB_CAT_VEHICLES_AIRDEFENSE,
	SUB_CAT_VEHICLES_SUPPORT,
	SUB_CAT_VEHICLES_SHIPS,
	SUB_CAT_MUNITIONS_AAM,
	SUB_CAT_MUNITIONS_AGM,
	SUB_CAT_MUNITIONS_ARM,
	SUB_CAT_MUNITIONS_BOMBS,
	SUB_CAT_MUNITIONS_STORES,
	SUB_CAT_MUNITIONS_GROUND,
};

void TACMoveRendererCB(C_Window *win)
{
	if(TAC_Viewer)
		TAC_Viewer->Viewport(win,0);
}	
		
void TACREF_ViewBSPObjectCB(long,short,C_Base *control)
{
	F4CSECTIONHANDLE *Leave;

	if (TAC_Viewer)	
	{
		Leave=UI_Enter(control->Parent_);
		TheLoader.WaitForLoader();
		TAC_Viewer->View3d(CurrentModel);
		UI_Leave(Leave);
	}
}

void StopRWRSounds()
{
	C_Window *win;
	C_Button *btn;

	if(LastLockTone != -1)
	{
		if (SFX_DEF) // JB 010425
			F4StopSound (SFX_DEF[LastLockTone].handle);

		LastLockTone=-1;
	}
	if(LastRWRTone != -1)
	{
		if (SFX_DEF) // JB 010425
			F4StopSound (SFX_DEF[LastRWRTone].handle);

		LastLockTone=-1;
	}

	win=gMainHandler->FindWindow(TAC_REF_WIN);
	if(win)
	{
		btn=(C_Button *)win->FindControl(PLAY_SEARCH_TONE);
		if(btn)
		{
			btn->SetState(0);
			btn->Refresh();
		}
																		
		btn=(C_Button *)win->FindControl(PLAY_LOCK_TONE);
		if(btn)
		{
			btn->SetState(0);
			btn->Refresh();
		}
	}
}

void TACREFCloseWindowCB(long ID,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	StopRWRSounds();
	CloseWindowCB(ID,hittype,control);
	TacRef_Cleanup();
}

void TACREF_PositionCamera(OBJECTINFO *Info,C_Window *win,long client)
{
	if(!TAC_Viewer || !Info || !win)
		return;

	FindCameraDeltas(Info);

	TAC_Viewer->SetCamera(Info->DeltaX,Info->DeltaY,Info->DeltaZ,Info->Heading,-Info->Pitch,0.0f);
	win->RefreshClient(client);
}

static void TACREF_PannerCB(long,short hittype,C_Base *control)
{
	float dx,dy;
	C_Panner *pnr;

	if(hittype != C_TYPE_LMOUSEUP && hittype != C_TYPE_REPEAT)
		return;

	pnr = static_cast<C_Panner *>(control);
	dx  = static_cast<float>(pnr->GetHRange());
	dy  = static_cast<float>(pnr->GetVRange());

	TACREF_Object.Heading+=dx;
	TACREF_Object.Pitch+=dy;
	if(TACREF_Object.Heading < 0) TACREF_Object.Heading+=360;
	if(TACREF_Object.Heading > 360) TACREF_Object.Heading-=360;
	if(TACREF_Object.CheckPitch)
	{
		if(TACREF_Object.Pitch < TACREF_Object.MinPitch) TACREF_Object.Pitch=TACREF_Object.MinPitch;
		if(TACREF_Object.Pitch > TACREF_Object.MaxPitch) TACREF_Object.Pitch=TACREF_Object.MaxPitch;
	}
	else
	{
		if(TACREF_Object.Pitch < 0) TACREF_Object.Pitch+=360;
		if(TACREF_Object.Pitch > 360) TACREF_Object.Pitch-=360;
	}
	TACREF_PositionCamera(&TACREF_Object,control->Parent_,control->GetClient());


}

static void TACREF_ZoomCB(long,short hittype,C_Base *control)
{
	float dy;
	C_Panner *pnr;


	if(hittype != C_TYPE_LMOUSEUP && hittype != C_TYPE_REPEAT)
		return;

	

	pnr=(C_Panner*)control;
	dy=static_cast<float>(pnr->GetVRange());

	TACREF_Object.Distance+=dy;
	if(TACREF_Object.Distance < TACREF_Object.MinDistance) TACREF_Object.Distance=TACREF_Object.MinDistance;
	if(TACREF_Object.Distance > TACREF_Object.MaxDistance) TACREF_Object.Distance=TACREF_Object.MaxDistance;

	TACREF_PositionCamera(&TACREF_Object,control->Parent_,control->GetClient());
}

void TACREF_ViewTimerAnimCB(long,short,C_Base *control)
{
	BSPLIST *obj;
	if(control->GetUserNumber(_UI95_TIMER_COUNTER_) < 1)
	{
		if(Helicopter)
		{
			obj=TAC_Viewer->Find(CurrentModel);
			if(obj)
			{
				BladeAngle += PI * 0.2f;
				if(BladeAngle > (PI*2.0f))
					BladeAngle -= PI*2.0f;

				((DrawableBSP*)obj->object)->SetDOFangle(2,BladeAngle);
				((DrawableBSP*)obj->object)->SetDOFangle(4,BladeAngle);
			}
		}

		if(control->GetFlags() & C_BIT_ABSOLUTE)
		{
			control->Parent_->RefreshWindow();
		}
		else
			control->Parent_->RefreshClient(control->GetClient());
		control->SetUserNumber(_UI95_TIMER_COUNTER_,control->GetUserNumber(_UI95_TIMER_DELAY_));
	}
	control->SetUserNumber(_UI95_TIMER_COUNTER_,control->GetUserNumber(_UI95_TIMER_COUNTER_)-1);
}

void TACREF_ViewTimerCB(long,short,C_Base *control)
{
	control->Parent_->RefreshClient(0);
}

static void BuildCatTree(Category *cat,C_TreeList *tree,TREELIST *parent)
{
	C_Text *txt;
	TREELIST *item;
	CatText *cattext;
	long textval=0;

	cattext=cat->GetFirst(&textval);
	while(cattext)
	{
		txt=new C_Text;
		txt->Setup(C_DONT_CARE,0);
		txt->SetFixedWidth(cattext->length);
		txt->SetText(cattext->String);
		txt->SetFont(tree->GetFont());

		item=tree->CreateItem(C_DONT_CARE,C_TYPE_ITEM,txt);
		if(item)
			tree->AddChildItem(parent,item);

		cattext=cat->GetNext(&textval);
	}
}

static void BuildStatsTree(Statistics *stats,C_TreeList *tree)
{
	C_Text *txt;
	Category *cat;
	TREELIST *par;
	long catval=0; // internal... don't use for anything
	long UniqueID=1;

	if(!stats || !tree)
		return;

	tree->Parent_->ScanClientArea(tree->GetClient());
	cat=stats->GetFirst(&catval);
	while(cat)
	{
		txt=new C_Text;
		txt->Setup(C_DONT_CARE,0);
		txt->SetFixedWidth(40);
		txt->SetFont(tree->GetFont());
		txt->SetText(cat->Name);

		par=tree->CreateItem(UniqueID++,C_TYPE_MENU,txt);
		if(par)
		{
			tree->AddItem(tree->GetRoot(),par);
			BuildCatTree(cat,tree,par);
		}
		cat=stats->GetNext(&catval);
	}
	tree->RecalcSize();
	tree->Parent_->ScanClientArea(tree->GetClient());
	tree->Parent_->RefreshClient(tree->GetClient());
}

static void BuildDescTree(Description *desc,C_TreeList *tree)
{
	C_Text *txt;
	TREELIST *item;
	TextString *desctext;
	long textval=0;
	long UniqueID=1;

	if(!desc || !tree)
		return;

	tree->Parent_->ScanClientArea(tree->GetClient());
	desctext=desc->GetFirst(&textval);
	while(desctext)
	{
		txt=new C_Text;
		txt->Setup(UniqueID,0);
		txt->SetW(tree->Parent_->ClientArea_[tree->GetClient()].right-tree->Parent_->ClientArea_[tree->GetClient()].left-10 - tree->GetX());
		txt->SetFlagBitOn(C_BIT_WORDWRAP);
		txt->SetFont(tree->GetFont());
		txt->SetFixedWidth(desctext->length);
		txt->SetText(desctext->String);

		item=tree->CreateItem(UniqueID++,C_TYPE_ITEM,txt);
		if(item)
			tree->AddItem(tree->GetRoot(),item);

		desctext=desc->GetNext(&textval);
	}
	tree->RecalcSize();
	tree->Parent_->ScanClientArea(tree->GetClient());
	tree->Parent_->RefreshClient(tree->GetClient());
}

void SelectRWR(long ID)
{
	C_Window *win;
	C_ListBox *lbox;
	C_Button *btn;
	LISTBOX *item;

	win=gMainHandler->FindWindow(TAC_REF_WIN);
	if(win)
	{
		lbox=(C_ListBox*)win->FindControl(RWR_LIST);
		if(lbox)
		{
			item=lbox->FindID(ID);
			if(item)
			{
				btn=(C_Button*)win->FindControl(PLAY_LOCK_TONE);
				if(btn)
					btn->SetUserNumber(0,item->Label_->GetUserNumber(3));

				btn=(C_Button*)win->FindControl(LOCK_ICON);
				if(btn)
				{
					btn->Refresh();
					btn->SetState(static_cast<short>(item->Label_->GetUserNumber(1)));
					btn->Refresh();
				}
			}
			else
			{
				btn=(C_Button*)win->FindControl(PLAY_LOCK_TONE);
				if(btn)
					btn->SetUserNumber(0,-1);

				btn=(C_Button*)win->FindControl(LOCK_ICON);
				if(btn)
				{
					btn->Refresh();
					btn->SetState(0);
					btn->Refresh();
				}
			}
		}
	}
}

static void BuildRWRList(RWR *rwr,C_ListBox *listbox)
{
	Radar *radar;
	long rwrval=0;
	long UniqueID=1;

	if(!rwr || !listbox)
		return;

	radar=rwr->GetFirst(&rwrval);
	if(radar)
	{
		while(radar)
		{
			listbox->AddItem(UniqueID,C_TYPE_ITEM,radar->Name);
			listbox->SetItemUserData(UniqueID,0,radar->SearchState);
			listbox->SetItemUserData(UniqueID,1,radar->LockState);
			listbox->SetItemUserData(UniqueID,2,radar->SearchTone);
			listbox->SetItemUserData(UniqueID,3,radar->LockTone);
			UniqueID++;
			radar=rwr->GetNext(&rwrval);
		}
	}
	else
	{
		listbox->AddItem(1,C_TYPE_ITEM,TXT_NO_RADAR);
		listbox->SetItemUserData(1,0,0);
		listbox->SetItemUserData(1,1,0);
		listbox->SetItemUserData(1,2,-1);
		listbox->SetItemUserData(1,3,-1);
	}
	listbox->SetValue(1);
}

static void Unload3dModel()
{
	VehicleClassDataType* vc;
	long i,visFlag;
	BSPLIST *obj,*Weapon;

	if (CurrentWeapon && CurrentModel)
	{
		obj=TAC_Viewer->Find(CurrentModel);
		Weapon=TAC_Viewer->Find(CurrentWeapon);

		vc = GetVehicleClassData(CurrentEntity);
		visFlag = vc->VisibleFlags;

		for (i=0; i<HARDPOINT_MAX; i++)
		{
			if (visFlag & (1 << i))
			{
				// This is a visible weapon, so detach
				((DrawableBSP*)obj->object)->DetachChild(((DrawableBSP*)Weapon->object),i);
			}
		}
		TAC_Viewer->Remove(CurrentWeapon);
		CurrentWeapon=0;
	}
	if(CurrentModel)
	{
		TAC_Viewer->Remove(CurrentModel);
		CurrentModel=0;
	}
}

static void CustomPosStuff(long GroupID,long SubGroupID,long ModelID,BSPLIST *Vehicle)
{
	TACREF_Object.Heading=-200.0f;
	TACREF_Object.Pitch=10.0f;
	switch(GroupID)
	{
		case CAT_AIRCRAFT:
			//switch(SubGroupID)
			//{
				/*case SUB_CAT_AIRCRAFT_FIGHTERS: // Fighters
				case SUB_CAT_AIRCRAFT_ATTACK: // Attack
				case SUB_CAT_AIRCRAFT_HELICOPTERS: // Helicopters
					if(ModelID == MapVisId(VIS_AC130))
						TACREF_Object.Distance=350.0f;
					else
						TACREF_Object.Distance=150.0f;
					break;
				case SUB_CAT_AIRCRAFT_BOMBERS: // Bombers
					TACREF_Object.Distance=440.0f;
					break;
				case SUB_CAT_AIRCRAFT_EW: // EW
				case SUB_CAT_AIRCRAFT_SUPPORT: // Support
					TACREF_Object.Distance=350.0f;
					break;*/ //Cobra test
				//default:
					//TACREF_Object.Distance=((DrawableBSP*)Vehicle->object)->Radius()*4;
					//break;
			//}
		//Cobra Steve asked for all aircraft to use the same value
			TACREF_Object.Distance=((DrawableBSP*)Vehicle->object)->Radius()*4;
			TACREF_Object.Direction=0.0f;

			TACREF_Object.MinPitch=0;
			TACREF_Object.MaxPitch=0;
			TACREF_Object.CheckPitch=FALSE;

			TACREF_Object.PosX=0;
			TACREF_Object.PosY=0;
			TACREF_Object.PosZ=0;
			TACREF_Object.MinDistance=((DrawableBSP*)Vehicle->object)->Radius()+30;
			TACREF_Object.MaxDistance=TACREF_Object.Distance + 200;
			break;
		case CAT_VEHICLES:
			switch(SubGroupID)
			{
				case SUB_CAT_VEHICLES_SHIPS:
					TACREF_Object.Distance=static_cast<float>(max(150,(long)((float)((DrawableBSP*)Vehicle->object)->Radius()*2.7)));
					TACREF_Object.Direction=0.0f;

					TACREF_Object.MinDistance=((DrawableBSP*)Vehicle->object)->Radius()+40;
					TACREF_Object.MaxDistance=((DrawableBSP*)Vehicle->object)->Radius()*20;
					TACREF_Object.MinPitch=5;
					TACREF_Object.MaxPitch=90;
					TACREF_Object.CheckPitch=TRUE;

					TACREF_Object.PosX=0;
					TACREF_Object.PosY=0;
					TACREF_Object.PosZ=((DrawableBSP*)Vehicle->object)->Radius()/20+5;
					break;
				default:
					switch ( ModelID ) {
						case VIS_SA2R:	// Fan Song
						case VIS_SA3R:	// Low Blow
							TACREF_Object.Distance=130.0f;
							TACREF_Object.MinDistance=max(90.0f,((DrawableBSP*)Vehicle->object)->Radius()+10);
							TACREF_Object.PosZ=14;
							break;
						case VIS_SA5R:	// Barlock
						case VIS_SA4R:	// Long Track
							TACREF_Object.Distance=110.0f;
							TACREF_Object.MinDistance=max(80.0f,((DrawableBSP*)Vehicle->object)->Radius()+10);
							TACREF_Object.PosZ=10;
							break;
						case VIS_PATRIOTRAD:
						case VIS_SA8L:
						case VIS_SA13L:
							TACREF_Object.Distance=85.0f;
							TACREF_Object.MinDistance=max(50.0f,((DrawableBSP*)Vehicle->object)->Radius()+20);
							TACREF_Object.PosZ=6;
							break;
						case VIS_SA3L:
							TACREF_Object.Distance=80.0f;
							TACREF_Object.MinDistance=max(50.0f,((DrawableBSP*)Vehicle->object)->Radius()+10);
							TACREF_Object.PosZ=3;
							break;
						case VIS_SA14:
						case VIS_STINGER:
							TACREF_Object.Distance=40.0f;
							TACREF_Object.MinDistance=max(20.0f,((DrawableBSP*)Vehicle->object)->Radius()+10);
							TACREF_Object.PosZ=1;
							break;
						case VIS_SA2L:	// sorting problem, need fixing in 3D models
						case VIS_SA5L:
							TACREF_Object.Distance=110.0f;
							TACREF_Object.MinDistance=max(100.0f,((DrawableBSP*)Vehicle->object)->Radius()+10);
							TACREF_Object.PosZ=5;
							break;
						case VIS_M88:	// M-88/A2 IRV, sorting problem need fixing
							TACREF_Object.Distance=135.0f;
							TACREF_Object.MinDistance=max(50.0f,((DrawableBSP*)Vehicle->object)->Radius()+10);
							TACREF_Object.PosZ=14;
							break;
						case VIS_NIKEL:
							TACREF_Object.Distance=140.0f;
							TACREF_Object.MinDistance=max(80.0f,((DrawableBSP*)Vehicle->object)->Radius()+5);
							TACREF_Object.PosZ=14;
							((DrawableBSP*)Vehicle->object)->SetDOFangle(1,30.0f *PI/180);
							break;
						case VIS_ZSU57_2:
							TACREF_Object.Distance=90.0f;
							TACREF_Object.MinDistance=max(20.0f,((DrawableBSP*)Vehicle->object)->Radius());
							TACREF_Object.PosZ=5;
							break;
						default:
							TACREF_Object.Distance=80.0f;
							TACREF_Object.MinDistance=max(40.0f,((DrawableBSP*)Vehicle->object)->Radius()+5);
							TACREF_Object.PosZ=5;
							break;
					}
					TACREF_Object.Direction=0.0f;

					TACREF_Object.MaxDistance=250.0f;
					TACREF_Object.MinPitch=5;
					TACREF_Object.MaxPitch=90;
					TACREF_Object.CheckPitch=TRUE;

					TACREF_Object.PosX=0;
					TACREF_Object.PosY=0;
					break;
			}
			break;
		case CAT_MUNITIONS:
			TACREF_Object.Direction=0.0f;

			TACREF_Object.MinPitch=0;
			TACREF_Object.MaxPitch=0;
			TACREF_Object.CheckPitch=FALSE;

			TACREF_Object.PosX=0;
			TACREF_Object.PosY=0;
			TACREF_Object.PosZ=0;
#if 0
			TACREF_Object.Distance=((DrawableBSP*)Vehicle->object)->Radius()*3;
			TACREF_Object.MinDistance=((DrawableBSP*)Vehicle->object)->Radius()+5;
			TACREF_Object.MaxDistance=((DrawableBSP*)Vehicle->object)->Radius()*10;
#endif
			TACREF_Object.Distance=30.0f;
			TACREF_Object.MinDistance=20.0f;
			TACREF_Object.MaxDistance=50.f;
			break;
	}
}

// This should handle ANY special case stuff for objects
static void Load3dModel(Entity *ent)
{
	BSPLIST *obj,*Weapon;
	Tpoint objPos;
	Trotation objRot;
	C_Window *win;
	VehicleClassDataType* vc;
	long i,visFlag;

// M.N. read the CT index visType[0] (= Normal model) to get the model ID instead of the tacref hardcoded one
	Falcon4EntityClassType* ct;
	short modelid;

	ct = &Falcon4ClassTable[ent->EntityID];
	modelid = ct->visType[0];

	if (ent->EntityID == 913)
		modelid = 1225; // LANTIRN Pod -> no CT record
	if (ent->EntityID == 531)
		modelid = 875;	// M-2A2/ADATS -> this vehicle doesn't exist in the datafiles (huh ?)
//	if(ent->ModelID)
	if (modelid)
	{
		Helicopter=FALSE;
		// Load Current Model
		if(ent->GroupID == CAT_AIRCRAFT) // Aircraft
		{
//			obj=TAC_Viewer->LoadBSP(ent->ModelID,ent->ModelID,TRUE);
			obj=TAC_Viewer->LoadBSP(modelid,modelid,TRUE);
//			if (ent->ModelID == MapVisId(VIS_F16C))
			if (modelid == MapVisId(VIS_F16C) || ((DrawableBSP*)obj->object)->instance.ParentObject->nSwitches >= 10)
			{
				((DrawableBSP*)obj->object)->SetSwitchMask(10, 1); // Afterburner
				((DrawableBSP*)obj->object)->SetSwitchMask(31, 1); // Afterburner
			}
			if(ent->SubGroupID == SUB_CAT_AIRCRAFT_HELICOPTERS)
			{
				Helicopter=TRUE;
				((DrawableBSP*)obj->object)->SetSwitchMask(0, 1); // Turn on rotors
				//((DrawableBSP*)obj->object)->SetDofAngle(2, 0);
				//((DrawableBSP*)obj->object)->SetDofAngle(5, 0);
			}
		}
		else
//			obj=TAC_Viewer->LoadBSP(ent->ModelID,ent->ModelID,FALSE);
			obj=TAC_Viewer->LoadBSP(modelid,modelid,FALSE);

		if(obj)
		{

			if(ent->MissileFlag > 0)
			{
				vc = GetVehicleClassData(ent->EntityID);
				if(vc)
				{
					visFlag = vc->VisibleFlags;
//					Weapon = TAC_Viewer->LoadBSP(ent->ModelID+1,ent->MissileFlag);
					Weapon = TAC_Viewer->LoadBSP(modelid+1,ent->MissileFlag);
//					CurrentWeapon=ent->ModelID+1;
					CurrentWeapon=modelid+1;

					for (i=0; i<HARDPOINT_MAX; i++)
					{
						if (visFlag & (1 << i))
						{
							// This is a visible weapon, so attach
							((DrawableBSP*)obj->object)->AttachChild(((DrawableBSP*)Weapon->object),i);
						}
					}
				}
			}

//			CustomPosStuff(ent->GroupID,ent->SubGroupID,ent->ModelID,obj);
//			CurrentModel=ent->ModelID;
			CustomPosStuff(ent->GroupID,ent->SubGroupID,modelid,obj);
			CurrentModel=modelid;
			//TJL 12/28/03 This code allows for the texture set to be cycled by reselecting the model
			int newtext;
			newtext = ((DrawableBSP*)obj->object)->GetNTextureSet()-1;
			
			if (newtext >= prevtext)
			{
				prevtext++;
			}

			if (prevtext > newtext)
				prevtext = 0;

			((DrawableBSP*)obj->object)->SetTextureSet(prevtext);
			//end new code

			PositandOrientSetData (TACREF_Object.PosX, TACREF_Object.PosY, TACREF_Object.PosZ, 0.0f, 0.0f, 0.0f, &objPos,&objRot);

			((DrawableBSP*)obj->object)->Update(&objPos,&objRot);

			

			win=gMainHandler->FindWindow(TAC_REF_WIN);
			if(win)
			{
				TACREF_PositionCamera(&TACREF_Object,win,0);
			}
		}
	}
}

static void LoadBitmap(C_Button *btn,char filename[])
{
	char file[MAX_PATH];

	gImageMgr->RemoveImage(TACREF_BITMAP_ID);
	strcpy(file,filename);
	strcat(file,".tga");
	gImageMgr->LoadImage(TACREF_BITMAP_ID,file,0,0);
	btn->Refresh();
	btn->SetImage(0,TACREF_BITMAP_ID);
	btn->Refresh();
}

// Moves individual entity to Window (info,model,RWR etc)
static void EntityToWindow(Entity* ent)
{
	C_Window *win;
	C_Button *btn;
	C_ListBox *lbox;
	C_TreeList *tree;

	win=gMainHandler->FindWindow(TAC_REF_WIN);
	if(win)
	{
		Unload3dModel();

		CurrentEntity=ent->EntityID;

		// Kill any sounds
		StopRWRSounds();

		// Make Stats Tree
		tree=(C_TreeList*)win->FindControl(STAT_TREE);
		if(tree)
		{
			tree->DeleteBranch(tree->GetRoot());
			BuildStatsTree(ent->GetStats(),tree);
		}

		// Make Description Tree
		tree=(C_TreeList*)win->FindControl(DESC_TREE);
		if(tree)
		{
			tree->DeleteBranch(tree->GetRoot());
			BuildDescTree(ent->GetDescription(),tree);
		}

		// Make RWR Listbox;
		lbox=(C_ListBox*)win->FindControl(RWR_LIST);
		if(lbox)
		{
			lbox->RemoveAllItems();
			BuildRWRList(ent->GetRWR(),lbox);

			SelectRWR(lbox->GetTextID());
		}

		Load3dModel(ent);

		btn=(C_Button*)win->FindControl(ENTITY_PIC);
		if(btn)
		{
			LoadBitmap(btn,ent->PhotoFile);
		}
		win->RefreshWindow();
	}
}

// Makes the Listbox full of Entities
static long InfoToWindow(long SubGroupID)
{
	C_Window *win;
	C_ListBox *lbox;
	Entity *ent;
	long entval=0;

	win=gMainHandler->FindWindow(TAC_REF_WIN);
	if(win)
	{
		lbox=(C_ListBox*)win->FindControl(ENTITY_LIST);
		if(lbox)
		{
			lbox->RemoveAllItems();

			ent=Reference->GetFirst(&entval);
			while(ent)
			{
				if(ent->SubGroupID == SubGroupID)
					lbox->AddItem(ent->EntityID,C_TYPE_ITEM,ent->Name);
				ent=Reference->GetNext(&entval);
			}

			if(!lbox->GetRoot())
			{
				lbox->AddItem(1,C_TYPE_ITEM,TXT_NONE);
				lbox->SetValue(1);
			}
			return(lbox->GetTextID());
		}
	}
	return(0);
}

static void SetGroupButton(long GroupID)
{
	C_Window *win;
	C_Button *btn;
	short i;

	win=gMainHandler->FindWindow(TAC_REF_WIN);
	if(win)
	{
		for(i=0;i<GROUP_COUNT;i++)
		{
			btn=(C_Button*)win->FindControl(GroupButtonID[i]);
			if(btn)
			{
				if(GroupButtonID[i] == GroupID)
				{
					btn->SetState(1);
					win->HideCluster(btn->GetUserNumber(1));
					win->HideCluster(btn->GetUserNumber(2));
					win->UnHideCluster(btn->GetUserNumber(0));
				}
				else
					btn->SetState(0);
				btn->Refresh();
			}
		}
	}
}

static void SetSubGroupButton(long SubGroupID)
{
	C_Window *win;
	C_Button *btn;
	short i;

	win=gMainHandler->FindWindow(TAC_REF_WIN);
	if(win)
	{
		for(i=0;i<SUBGROUP_COUNT;i++)
		{
			btn=(C_Button*)win->FindControl(SubGroupButtonID[i]);
			if(btn)
			{
				if(SubGroupButtonID[i] == SubGroupID)
					btn->SetState(1);
				else
					btn->SetState(0);
				btn->Refresh();
			}
		}
	}
}

// Worker function to get the Entity into the window
static void LoadEntity(long EntityID)
{
	Entity *ent;

	ent=Reference->Find(EntityID);
	if(ent)
		EntityToWindow(ent);
}

// Finds first occurence of SubGroup... loads that record
static void LoadSubGroup(long SubGroupID)
{
	long EntID;
	EntID=InfoToWindow(SubGroupID);
	LoadEntity(EntID);
}

// Finds first occurence of Group... loads that record
static void LoadGroup(long GroupID)
{
	long SubGroupID;

	switch(GroupID)
	{
		case CAT_AIRCRAFT:
			SubGroupID=SUB_CAT_AIRCRAFT_FIGHTERS;
			break;
		case CAT_VEHICLES:
			SubGroupID=SUB_CAT_VEHICLES_TANKS;
			break;
		case CAT_MUNITIONS:
			SubGroupID=SUB_CAT_MUNITIONS_AAM;
			break;
		default:
			SubGroupID=0;
			break;
	}
	SetSubGroupButton(SubGroupID);
	LoadSubGroup(SubGroupID);
}

static void SelectGroupCB(long ID,short hittype,C_Base *control)
{
	F4CSECTIONHANDLE *Leave;
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	StopRWRSounds();
	Leave=UI_Enter(control->Parent_);

	// Hide/Unhide SubCat buttons
	if(control->GetUserNumber(1))
		control->Parent_->HideCluster(control->GetUserNumber(1));
	if(control->GetUserNumber(2))
		control->Parent_->HideCluster(control->GetUserNumber(2));
	if(control->GetUserNumber(0))
		control->Parent_->UnHideCluster(control->GetUserNumber(0));

	LoadGroup(ID);

	UI_Leave(Leave);
}

static void SelectSubGroupCB(long ID,short hittype,C_Base *control)
{
	F4CSECTIONHANDLE *Leave;
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	StopRWRSounds();
	Leave=UI_Enter(control->Parent_);
	LoadSubGroup(ID);
	UI_Leave(Leave);
}

static void SelectEntityCB(long ID,short hittype,C_Base *control)
{
	F4CSECTIONHANDLE *Leave;
	C_ListBox *lbox=(C_ListBox*)control;

	if(hittype != C_TYPE_SELECT)
		return;

	StopRWRSounds();
	Leave=UI_Enter(control->Parent_);
	ID=lbox->GetTextID();
	LoadEntity(ID);
	UI_Leave(Leave);
}

static void SelectRWRCB(long ID,short hittype,C_Base *control)
{
	C_ListBox *lbox=(C_ListBox*)control;

	if(hittype != C_TYPE_SELECT)
		return;

	StopRWRSounds();
	ID=lbox->GetTextID();
	SelectRWR(ID);
}

static void PlaySoundCB(long,short hittype,C_Base *control)
{
	if(hittype != C_TYPE_LMOUSEUP)
		return;

	if(control->GetState())
	{
		if (SFX_DEF) // JB 010425
			F4StopSound (SFX_DEF[LastLockTone].handle);

		LastLockTone=control->GetUserNumber(0);
		
		if(SFX_DEF && LastLockTone != -1) // JB 010425
			F4LoopSound (SFX_DEF[LastLockTone].handle);
	}
	else if(LastLockTone != -1)
	{
		if (SFX_DEF) // JB 010425
			F4StopSound (SFX_DEF[LastLockTone].handle);

		LastLockTone=-1;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////
// This is the function which makes your objects draw behind the panner/zoomer
//
// DON'T CHANGE IT
//
void ReplaceDummyControl(C_Window *win)
{
	CONTROLLIST *cur;
	C_TimerHook *objectviewer;
	F4CSECTIONHANDLE *Leave;

	cur=win->GetControlList();
	while(cur)
	{
		if(cur->Control_->GetID() == TACREF_SCAB_CTRL)
		{
			Leave=UI_Enter(win);
			objectviewer = new C_TimerHook;
			objectviewer->Setup( C_DONT_CARE,C_TYPE_NORMAL );
			objectviewer->SetClient(0);
			objectviewer->SetXY(win->ClientArea_[0].left,win->ClientArea_[0].top);
			objectviewer->SetW(win->ClientArea_[0].right - win->ClientArea_[0].left);
			objectviewer->SetH(win->ClientArea_[0].bottom - win->ClientArea_[0].top);
			objectviewer->SetRefreshCallback(TACREF_ViewTimerCB); // new
			objectviewer->SetDrawCallback(TACREF_ViewBSPObjectCB);
			objectviewer->SetFlagBitOff( C_BIT_TIMER );
			objectviewer->SetReady( 1);
			objectviewer->SetParent(win);

			cur->Control_->Cleanup();
			delete cur->Control_;
			cur->Control_=objectviewer;
			UI_Leave(Leave);
			return;
		}
		cur=cur->Next;
	}
}

static void HookupTacticalReferenceControls(long ID)
{
	C_Window *winme;
	C_Button *ctrl;
	C_Panner	*panner;
	C_ListBox *lbox;
	C_TimerHook *drawTimer;

	winme=gMainHandler->FindWindow(ID);

	if(winme == NULL)
		return;

	if(ID == TAC_REF_WIN)
		winme->SetDragCallback(TACMoveRendererCB);
																					
	// Hook up AIRCRAFT SELECT button
	ctrl=(C_Button *)winme->FindControl(CAT_AIRCRAFT);
	if(ctrl != NULL)
		ctrl->SetCallback(SelectGroupCB);

	// Hook up VEHICLES SELECT button
	ctrl=(C_Button *)winme->FindControl(CAT_VEHICLES);
	if(ctrl != NULL)
		ctrl->SetCallback(SelectGroupCB);
												
	// Hook up MUNITIONS SELECT button
	ctrl=(C_Button *)winme->FindControl(CAT_MUNITIONS);
	if(ctrl != NULL)
		ctrl->SetCallback(SelectGroupCB);

	// Hook up SUB AIRCRAFT FIGHTERS button
	ctrl=(C_Button *)winme->FindControl(SUB_CAT_AIRCRAFT_FIGHTERS);
	if(ctrl != NULL)
		ctrl->SetCallback(SelectSubGroupCB);
							
	// Hook up SUB AIRCRAFT ATTACK button
	ctrl=(C_Button *)winme->FindControl(SUB_CAT_AIRCRAFT_ATTACK);
	if(ctrl != NULL)
		ctrl->SetCallback(SelectSubGroupCB);
							
	// Hook up SUB AIRCRAFT BOMBERS button
	ctrl=(C_Button *)winme->FindControl(SUB_CAT_AIRCRAFT_BOMBERS);
	if(ctrl != NULL)
		ctrl->SetCallback(SelectSubGroupCB);
									
	// Hook up SUB AIRCRAFT HELICOPTERS button
	ctrl=(C_Button *)winme->FindControl(SUB_CAT_AIRCRAFT_HELICOPTERS);
	if(ctrl != NULL)
		ctrl->SetCallback(SelectSubGroupCB);
							
	// Hook up SUB AIRCRAFT SUPPORT button
	ctrl=(C_Button *)winme->FindControl(SUB_CAT_AIRCRAFT_SUPPORT);
	if(ctrl != NULL)
		ctrl->SetCallback(SelectSubGroupCB);
							
	// Hook up SUB AIRCRAFT EW button
	ctrl=(C_Button *)winme->FindControl(SUB_CAT_AIRCRAFT_EW);
	if(ctrl != NULL)
		ctrl->SetCallback(SelectSubGroupCB);
					
	// Hook up SUB VEHICLES TANKS button
	ctrl=(C_Button *)winme->FindControl(SUB_CAT_VEHICLES_TANKS);
	if(ctrl != NULL)
		ctrl->SetCallback(SelectSubGroupCB);
							
	// Hook up SUB VEHICLES IFVS button
	ctrl=(C_Button *)winme->FindControl(SUB_CAT_VEHICLES_IFVS);
	if(ctrl != NULL)
		ctrl->SetCallback(SelectSubGroupCB);
								
	// Hook up SUB VEHICLES ARTILLERY button
	ctrl=(C_Button *)winme->FindControl(SUB_CAT_VEHICLES_ARTILLERY);
	if(ctrl != NULL)
		ctrl->SetCallback(SelectSubGroupCB);
							
	// Hook up SUB VEHICLES AIRDEFENSE button
	ctrl=(C_Button *)winme->FindControl(SUB_CAT_VEHICLES_AIRDEFENSE);
	if(ctrl != NULL)
		ctrl->SetCallback(SelectSubGroupCB);
							
	// Hook up SUB VEHICLES SUPPORT button
	ctrl=(C_Button *)winme->FindControl(SUB_CAT_VEHICLES_SUPPORT);
	if(ctrl != NULL)
		ctrl->SetCallback(SelectSubGroupCB);
							
	// Hook up SUB VEHICLES SHIPS button
	ctrl=(C_Button *)winme->FindControl(SUB_CAT_VEHICLES_SHIPS);
	if(ctrl != NULL)									
		ctrl->SetCallback(SelectSubGroupCB);
			
	// Hook up SUB MUNITIONS AAM button
	ctrl=(C_Button *)winme->FindControl(SUB_CAT_MUNITIONS_AAM);
	if(ctrl != NULL)									
		ctrl->SetCallback(SelectSubGroupCB);
								
	// Hook up SUB MUNITIONS AGM button
	ctrl=(C_Button *)winme->FindControl(SUB_CAT_MUNITIONS_AGM);
	if(ctrl != NULL)																
		ctrl->SetCallback(SelectSubGroupCB);
								
	// Hook up SUB MUNITIONS ARM button
	ctrl=(C_Button *)winme->FindControl(SUB_CAT_MUNITIONS_ARM);
	if(ctrl != NULL)																
		ctrl->SetCallback(SelectSubGroupCB);
							
	// Hook up SUB MUNITIONS BOMBS button
	ctrl=(C_Button *)winme->FindControl(SUB_CAT_MUNITIONS_BOMBS);
	if(ctrl != NULL)																
		ctrl->SetCallback(SelectSubGroupCB);
								
	// Hook up SUB MUNITIONS STORES button
	ctrl=(C_Button *)winme->FindControl(SUB_CAT_MUNITIONS_STORES);
	if(ctrl != NULL)																
		ctrl->SetCallback(SelectSubGroupCB);
								
	// Hook up SUB MUNITIONS GROUND button
	ctrl=(C_Button *)winme->FindControl(SUB_CAT_MUNITIONS_GROUND);
	if(ctrl != NULL)																						
		ctrl->SetCallback(SelectSubGroupCB);

	// Hook up Close Button
	ctrl=(C_Button *)winme->FindControl(CLOSE_WINDOW);
	if(ctrl != NULL)
		ctrl->SetCallback(TACREFCloseWindowCB);

	// for ROTATION
	panner = (C_Panner *)winme->FindControl(TACREF_PANNER);
	if(panner != NULL)
		panner->SetCallback( TACREF_PannerCB );

	// for ZOOM
	panner = (C_Panner *)winme->FindControl(TACREF_ZOOMER);
	if(panner != NULL)
		panner->SetCallback( TACREF_ZoomCB);
	
	lbox=(C_ListBox *)winme->FindControl(ENTITY_LIST);
	if(lbox != NULL)
		lbox->SetCallback(SelectEntityCB);


	// RWR LISTBOX
	lbox=(C_ListBox *)winme->FindControl(RWR_LIST);
	if(lbox != NULL)
		lbox->SetCallback(SelectRWRCB);


	// Hook up RWR Search Tone
	ctrl=(C_Button *)winme->FindControl(PLAY_SEARCH_TONE);
	if(ctrl != NULL)
		ctrl->SetCallback(PlaySoundCB);
					
	// Hook up RWR Lock Tone
	ctrl=(C_Button *)winme->FindControl(PLAY_LOCK_TONE);
	if(ctrl != NULL)
		ctrl->SetCallback(PlaySoundCB);

// Help GUIDE thing
	ctrl=(C_Button*)winme->FindControl(UI_HELP_GUIDE);
	if(ctrl)
		ctrl->SetCallback(UI_Help_Guide_CB);

	if(ID == TAC_REF_WIN)
	{
		ReplaceDummyControl(winme);

		drawTimer = new C_TimerHook;
		drawTimer->Setup( C_DONT_CARE,C_TYPE_NORMAL );
		drawTimer->SetClient(0);
		drawTimer->SetXY(winme->ClientArea_[0].left,winme->ClientArea_[0].top);
		drawTimer->SetW(winme->ClientArea_[0].right - winme->ClientArea_[0].left);
		drawTimer->SetH(winme->ClientArea_[0].bottom - winme->ClientArea_[0].top);
		drawTimer->SetUpdateCallback(TACREF_ViewTimerAnimCB );
		drawTimer->SetReady( 1);
		drawTimer->SetUserNumber(_UI95_TIMER_DELAY_,1);
		winme->AddControl( drawTimer );
	}
}

BOOL TacRef_Setup()
{
	C_Window *win;

	// RV - Biker - Add theater switching for tacref
	char tmpPath[_MAX_PATH];
	sprintf(tmpPath, "%s\\%s", FalconTacrefThrDirectory, "tacrefdb.bin");

	win=gMainHandler->FindWindow(TAC_REF_WIN);
	if(win)
	{
		if(TAC_Viewer == NULL)
		{
			if(TAC_Viewer != NULL)
			{
				TAC_Viewer->Cleanup();
				delete TAC_Viewer;
			}
			CurrentModel=0;
			CurrentWeapon=0;
			CurrentEntity=0;
			TAC_Viewer=new C_3dViewer;
			TAC_Viewer->Setup();
			TAC_Viewer->Viewport(win,0);
			TAC_Viewer->Init3d(30.0f);
		}
		if(!Reference)
		{
			Reference=new TacticalReference;
			// RV - Biker - Load theater specific tacref
			//Reference->Load("tacrefdb.bin");
			Reference->Load(tmpPath);
			SetGroupButton(CAT_AIRCRAFT);
			LoadGroup(CAT_AIRCRAFT);
		}
		return(TRUE);
	}
	return(FALSE);
}

void TacRef_Cleanup()
{
	if(TAC_Viewer)
	{
		UnloadObject(); // just in case :)
		TAC_Viewer->Cleanup();
		delete TAC_Viewer;
		TAC_Viewer=NULL;
	}
	if(Reference)
	{
		Reference->Cleanup();
		delete Reference;
		Reference=NULL;
	}
}

void LoadTacticalReferenceWindows()
{
	long ID;

	if(TACREFLoaded) return;

	if( _LOAD_ART_RESOURCES_)
		gMainParser->LoadImageList("ref_res.lst");
	else
		gMainParser->LoadImageList("ref_art.lst");
	gMainParser->LoadSoundList("ref_snd.lst");
	gMainParser->LoadWindowList("ref_scf.lst");

	ID=gMainParser->GetFirstWindowLoaded();
	while(ID)
	{
		HookupTacticalReferenceControls(ID);
		ID=gMainParser->GetNextWindowLoaded();
	}
	TACREFLoaded++;

}
