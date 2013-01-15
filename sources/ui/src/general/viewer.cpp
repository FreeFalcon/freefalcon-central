#include "graphics\include\TimeMgr.h"
#include "graphics\include\imagebuf.h"
#include "graphics\include\renderow.h"
#include "graphics\include\RViewPnt.h"
#include "graphics\include\drawbsp.h"
#include "vu2.h"
#include "F4vu.h"
#include "team.h"
//#include "simbase.h"
//#include "simlib.h"
//#include "initdata.h"
//#include "simfiltr.h"
//#include "simdrive.h"
//#include "simfeat.h"
#include "vehicle.h"
#include "objectiv.h"
#include "feature.h"
#include "find.h"
#include "playerop.h"
#include "dispcfg.h"
#include "graphics\include\setup.h"
#include "graphics\include\loader.h"
#include "chandler.h"
#include "ui95_ext.h"
#include "cbsplist.h"
#include "c3dview.h"
#include "userids.h"
#include "textids.h"
#include "teamdata.h"
#include "classtbl.h"
// 3D stuff in the UI? No Way!!!


// Ground Altitude Function = UIrenderer->GetGroundLevel(x,y);

OBJECTINFO Recon;

extern long FirstPlane;

extern C_Handler *gMainHandler;
void DeleteGroupList(long ID);
//SimBaseClass* AddFeatureToSim (SimInitDataClass *initData);
//void CalcTransformMatrix (SimBaseClass* theObject);
void CloseAllRenderers(long openID);
DrawableBSP *FindAircraft(long ID);
C_Entity *BuildObjectiveInfo(Objective obj);
C_Entity *BuildObjective(Objective obj);
C_Feature *BuildFeature(Objective obj,long featureID,Tpoint *objPos);
// Used for targeting
extern VU_ID FeatureID;
extern long FeatureNo;
extern C_TreeList *TargetTree;

C_BSPList *gBSPList=NULL;

Render3D   *UIrend3d=NULL;
static void AddToDisplayList();
RViewPoint *UIviewPoint=NULL;
static RenderOTW  *UIrenderer=NULL;
static Tpoint zeroPos = {0.0F, 0.0F, 0.0F};
static Tpoint viewPos = {0.0F, 0.0F, 0.0F};
static Trotation viewRot;
static VuOrderedList* UIfeatureList;
static long currentObj=-1;

static long CountryNames[]=
{
	TXT_NEUTRAL,
	TXT_USA,
	TXT_ROK,
	TXT_JAPAN,
	TXT_CIS,
	TXT_CHINA,
	TXT_DPRK,
	TXT_NEUTRAL,
};

void CalculateViewport(C_Window *win,long client,float *l,float *t,float *r,float *b)
{
	float sw,sh;

	sw=(float)gMainHandler->GetFront()->targetXres();
	sh=(float)gMainHandler->GetFront()->targetYres();

	*l=static_cast<float>(-1.0f+((float)(win->GetX()+win->ClientArea_[client].left+6) / (sw * .5)));
	*t=static_cast<float>( 1.0f-((float)(win->GetY()+win->ClientArea_[client].top+6) / (sh * .5)));
	*r=static_cast<float>( 1.0f-((float)(sw-(win->GetX()+win->ClientArea_[client].right)-6) / (sw * .5)));
	*b=static_cast<float>(-1.0f+((float)(sh-(win->GetY()+win->ClientArea_[client].bottom)-6) / (sh * .5)));
}

void CenterOnFeatureCB(long,short hittype,C_Base *control)
{
	BSPLIST *bsp;
	Tpoint pos;
	C_Feature *feat;
	C_Window *win;

	if(hittype != C_TYPE_LMOUSEUP)
		return;

	if(gUIViewer)
	{
		feat=(C_Feature *)control;
		bsp=gUIViewer->Find(feat->GetID());
		if(bsp)
		{
			bsp->object->GetPosition(&pos);
			Recon.PosX=pos.x;
			Recon.PosY=pos.y;
			Recon.PosZ=pos.z;
			gUIViewer->SetPosition(Recon.PosX,Recon.PosY,Recon.PosZ);
			win=gMainHandler->FindWindow(RECON_WIN);
			if(win)
				win->RefreshWindow();
		}
		FeatureID=feat->GetVUID();
		FeatureNo=feat->GetFeatureID();
		if(TargetTree)
			TargetTree->SetAllControlStates(0,TargetTree->GetRoot());
		feat->SetState(1);
	}
}

void SetHeading(C_Window *win)
{
	C_Text *txt;
	_TCHAR buffer[5];

	if(!win)
		return;

	txt=(C_Text*)win->FindControl(RECON_HEADING);
	if(txt)
	{
		txt->Refresh();
		_stprintf(buffer,"%03ld",(long)Recon.Heading);
		txt->SetText(buffer);
		txt->Refresh();
	}
}

void FindCameraDeltas(OBJECTINFO *Info)
{
	Info->DeltaX = static_cast<float>(-Info->Distance * cos(Info->Heading * DTR) * cos(Info->Pitch * DTR));
	Info->DeltaY = static_cast<float>(-Info->Distance * sin(Info->Heading * DTR) * cos(Info->Pitch * DTR));
	Info->DeltaZ = static_cast<float>(-Info->Distance * sin(Info->Pitch * DTR));
}

void PositionCamera(OBJECTINFO *Info,C_Window *win,long client)
{
	if(!gUIViewer || !Info || !win)
		return;

	FindCameraDeltas(Info);
	SetHeading(win);
	gUIViewer->SetCamera(Info->DeltaX,Info->DeltaY,Info->DeltaZ,Info->Heading,-Info->Pitch,0.0f);
	win->RefreshClient(client);
}

void SetSlantRange(C_Window *win)
{
	C_Text *txt;
	_TCHAR buffer[15];

	if(!win)
		return;

	txt=(C_Text*)win->FindControl(SLANT_RANGE);
	if(txt)
	{
		txt->Refresh();
		_stprintf(buffer,"%1ld ft",(long)Recon.Distance);
		txt->SetText(buffer);
		txt->Refresh();
	}
}

void SetBullsEye(C_Window *win)
{
	C_Text *txt;
	long brg,dist;
	_TCHAR buffer[40];

	if(!win)
		return;

	txt=(C_Text*)win->FindControl(BULLSEYE);
	if(txt)
	{
		txt->Refresh();
		brg=TheCampaign.BearingToBullseyeDeg(Recon.PosX,Recon.PosY);

	//MI Bullseye bearing fix
#if 0
		while(brg < 0)
			brg+=360;
		while(brg > 360)
			brg-=360;
#else
		brg += 180;
#endif
		dist=FloatToInt32(TheCampaign.RangeToBullseyeFt(Recon.PosX,Recon.PosY)*FT_TO_NM);

		_stprintf(buffer,"%03d  %1ld %s",brg,dist,gStringMgr->GetString(TXT_NM));

		txt->SetText(buffer);
		txt->Refresh();
	}
}

BOOL ReconListSortCB(TREELIST *list,TREELIST *newitem)
{
	C_Feature *feat1,*feat2;
	C_Entity *ent1,*ent2;
	if(!list || !newitem)
		return(FALSE);

	if(list->Type_ == C_TYPE_ROOT)
	{ // Sort by ID (ID = Team)
		if(newitem->ID_ < list->ID_)
			return(TRUE);
	}
	if(list->Type_ == C_TYPE_MENU)
	{ // Sort for the Headers (Alphabetical)
		ent1=(C_Entity*)list->Item_;
		ent2=(C_Entity*)newitem->Item_;
		if(!ent1 || !ent2)
			return(FALSE);
		if(_tcscmp(ent2->GetName(),ent1->GetName()) < 0)
			return(TRUE);
	}
	else if(list->Type_ == C_TYPE_ITEM)
	{ // Sort for the Individual Items (Based on Value)
		feat1=(C_Feature*)list->Item_;
		feat2=(C_Feature*)newitem->Item_;
		if(!feat1 || !feat2)
			return(FALSE);
		if(feat2->GetFeatureValue() > feat1->GetFeatureValue())
			return(TRUE);
	}
	return(FALSE);
}

BSPLIST *LoadFeature(long ID,int visID,Tpoint *pos,float facing)
{
	return(gUIViewer->LoadBuilding(ID,visID,pos,facing*DEG_TO_RADIANS));
}

int UI_Deaggregate (ObjectiveClass* objective)
{
	int						f,fid;
	VehicleID				classID;
	Falcon4EntityClassType* classPtr;
	float					x,y,z;
	FeatureClassDataType*	fc;
	ObjClassDataType*		oc;
	BSPLIST					*drawptr;
	Tpoint					objPos;
	C_Window				*win;
	C_Text					*txt;
	C_TreeList				*tree;
	TREELIST				*item,*parent,*root;
	C_Entity				*recon_ent;
	C_Feature				*feat;

	if(gUIViewer == NULL)
		return(0);

	CloseAllRenderers(RECON_WIN);
	win=gMainHandler->FindWindow(RECON_WIN);
	if(win == NULL)
		return(0);

	tree=(C_TreeList*)win->FindControl(RECON_TREE);
	if(!tree)
		return(0);

	root=tree->Find(objective->GetTeam());
	if(!root)
	{
		txt=new C_Text;
		txt->Setup(C_DONT_CARE,0);
		txt->SetFixedWidth(100);
		txt->SetFont(tree->GetFont());
		txt->SetText(TeamInfo[objective->GetTeam()]->GetName());
		txt->SetFGColor(0xc0c0c0);

		root=tree->CreateItem(objective->GetTeam(),C_TYPE_ROOT,txt);
		if(root)
			tree->AddItem(tree->GetRoot(),root);

	}
	if(!root)
		return(0);

	recon_ent=BuildObjective(objective);

	parent=tree->CreateItem(objective->GetCampID(),C_TYPE_MENU,recon_ent);
	if(parent)
		tree->AddChildItem(root,parent);

	// Set up the init data structure.
	oc = objective->GetObjectiveClassData();
	fid = oc->FirstFeature;
	for (f=0; f<oc->Features; f++,fid++)
	{
		classID = static_cast<short>(objective->GetFeatureID(f));
		if (classID)
		{
			fc = GetFeatureClassData (classID);
			if (!fc || fc->Flags & FEAT_VIRTUAL)
				continue;
			objective->GetFeatureOffset(f, &y, &x, &z);
			objPos.x=x+objective->XPos();
			objPos.y=y+objective->YPos();
			objPos.z=z;
			classPtr = &Falcon4ClassTable[fc->Index];
			if(classPtr != NULL)
			{
				drawptr=LoadFeature(objective->GetCampID() << 16 | f,classPtr->visType[objective->GetFeatureStatus(f)],&objPos,(float)FeatureEntryDataTable[fid].Facing);
				if(drawptr != NULL)
				{
//					if(objective->GetFeatureValue(f))
//					{
						((DrawableObject*)drawptr)->GetPosition(&objPos);

						feat=BuildFeature(objective,f,&objPos);
						if(feat)
						{
							item=tree->CreateItem(objective->GetCampID() << 16 | f,C_TYPE_ITEM,feat);
							if(item)
								tree->AddChildItem(parent,item);
						}
//					}
				}
			}
		}
	}
	tree->RecalcSize();
	if(tree->Parent_)
		tree->Parent_->RefreshClient(tree->GetClient());
	return(1);
}

void DisplayView(long,short,C_Base *)
{
	if(gUIViewer)
		gUIViewer->ViewGreyOTW();
}

void MoveViewTimerCB(long,short,C_Base *control)
{
	if(gUIViewer == NULL)
		return;

	if(control->GetUserNumber(_UI95_TIMER_COUNTER_) < 1)
	{
		if(Recon.Direction == 0)// && TheLoader.LoaderQueueEmpty())
			return;

		Recon.Heading += Recon.Direction;
		if(Recon.Heading >= 360.0f)
			Recon.Heading-=360.0f;
		if(Recon.Heading < 0)
			Recon.Heading+=360.0f;

		PositionCamera(&Recon,control->Parent_,0);
		control->SetUserNumber(_UI95_TIMER_COUNTER_,control->GetUserNumber(_UI95_TIMER_DELAY_));
	}
	control->SetUserNumber(_UI95_TIMER_COUNTER_,control->GetUserNumber(_UI95_TIMER_COUNTER_)-1);
}

void InitObjectViewer(C_Window *win,long client)
{
	float l,t,r,b;

	TheTimeManager.SetTime( ( unsigned long )( 12 * 60 * 60 * 1000 ) );

	viewRot = IMatrix;
	viewPos.x =0.0f;
	viewPos.y =0.0f;
	viewPos.z =0.0f;

	UIrend3d=new Render3D;
	UIrend3d->Setup(gMainHandler->GetFront());
	UIrend3d->SetFOV( 30.0f * PI/180.f );

	CalculateViewport(win,client,&l,&t,&r,&b);

	UIrend3d->SetViewport( l, t, r, b);
	UIrend3d->SetCamera( &viewPos, &viewRot );

	if(gBSPList == NULL)
	{
		gBSPList=new C_BSPList;
		gBSPList->Setup();
	}
	else
		gBSPList->RemoveAll();
}

void LoadObject(long objID)
{
	if(UIrend3d)
	{
		if(currentObj == objID)
			return;

		if(currentObj != -1)
			DrawableBSP::Unlock(currentObj);

		DrawableBSP::LockAndLoad(objID);
		currentObj=objID;
	}
}

void UnloadObject()
{
	if(currentObj != -1)
	{
		DrawableBSP::Unlock(currentObj);
		currentObj=-1;
	}
}

void CleanupObjectViewer()
{
	if(UIrend3d)
	{
		UnloadObject(); // just in case :)
		UIrend3d->Cleanup();
		delete UIrend3d;
		UIrend3d=NULL;
	}
	if(gBSPList != NULL)
	{
		gBSPList->RemoveAll();
		delete gBSPList;
		gBSPList=NULL;
	}
}

void ViewBSPObjectCB(long,short,C_Base *)
{
	BSPLIST *obj;

	if(gBSPList == NULL) return;

	obj=gBSPList->Find(FirstPlane << 24);
	if(obj != NULL)
	{
		UIrend3d->StartDraw();
		((DrawableBSP*)obj->object)->Draw(UIrend3d);
		UIrend3d->EndDraw();
	}
}

void ViewObjectCB(long,short,C_Base *)
{
	if(gUIViewer)
		gUIViewer->View3d(FirstPlane << 24);
}

void MoveRendererCB(C_Window *win)
{
	if(gUIViewer)
		gUIViewer->Viewport(win,2);
}

void MoveRendererClient0CB(C_Window *win)
{
	if(gUIViewer)
		gUIViewer->Viewport(win,0);
}
