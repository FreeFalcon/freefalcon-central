#include "stdafx.h"
#include <math.h>
#include "sim\include\phyconst.h"

C_Waypoint::C_Waypoint() : C_Control()
{
	Root_=NULL;
	_SetCType_(_CNTL_WAYPOINT_);
	SetReady(0);
	DefaultFlags_=C_BIT_ENABLED|C_BIT_SELECTABLE|C_BIT_MOUSEOVER;
	WPScaleType_=0;
	MinWorldX_=0;
	MinWorldY_=0;
	MaxWorldX_=0;
	MaxWorldY_=0;
	scale_=1.0f;
	Dragging_=0;
	LastWP_=NULL;
	memset(&last_,0,sizeof(UI95_RECT));
}

C_Waypoint::C_Waypoint(char **stream) : C_Control(stream)
{
}

C_Waypoint::C_Waypoint(FILE *fp) : C_Control(fp)
{
}

C_Waypoint::~C_Waypoint()
{
	if(Root_)
		Cleanup();
}

long C_Waypoint::Size()
{
	return(0);
}

void C_Waypoint::Setup(long ID,short Type)
{
	SetID(ID);
	SetType(Type);
	SetDefaultFlags();
	SetGroup(0);
	SetReady(1);
}

void C_Waypoint::Cleanup()
{
	if(Root_)
		EraseWaypointList();
	Root_=NULL;
	LastWP_=NULL;
}

BOOL C_Waypoint::ShowByType(long typemask)
{
	WAYPOINTLIST *cur;
	BOOL retval=FALSE;

	cur=Root_;

	while(cur)
	{
		if(cur->Type & typemask)
		{
			cur->Flags &= (0xffffffff ^ C_BIT_INVISIBLE);
			retval=TRUE;
		}
		cur=cur->Next;
	}
	return(retval);
}

BOOL C_Waypoint::HideByType(long typemask)
{
	WAYPOINTLIST *cur;
	BOOL retval=FALSE;

	cur=Root_;

	while(cur)
	{
		if(cur->Type & typemask)
		{
			cur->Flags |= C_BIT_INVISIBLE;
			retval=TRUE;
		}
		cur=cur->Next;
	}
	return(retval);
}

BOOL C_Waypoint::Process(long ID,short HitType)
{
	gSoundMgr->PlaySound(GetSound(HitType));
	if(Callback_)
		(*Callback_)(ID,HitType,this);
	if(LastWP_)
		return(LastWP_->Icon->Process(ID,HitType));
	return(FALSE);
}

WAYPOINTLIST *C_Waypoint::AddWaypointToList(long CampID,short type,long NormID,long SelID,long OthrID,float x,float y,short Dragable)
{
	WAYPOINTLIST *newitem,*cur;

	newitem=new WAYPOINTLIST;
	
	if(newitem == NULL)
		return(NULL);

	newitem->Icon=new C_Button;

	newitem->ID=CampID;
	newitem->Group = 0; // JPO initialise
	newitem->Type=type;
	newitem->Icon->Setup(CampID,C_TYPE_CUSTOM,0,0);
	newitem->Icon->SetImage(C_STATE_0,NormID);
	newitem->Icon->SetImage(C_STATE_1,SelID);
	newitem->Icon->SetImage(C_STATE_2,OthrID);
	if(Dragable)
		newitem->Icon->SetFlags((GetFlags() & ~(C_BIT_DRAGABLE)) | C_BIT_DRAGABLE);
	else
		newitem->Icon->SetFlags((GetFlags() & ~(C_BIT_DRAGABLE)));
	newitem->Icon->SetClient(GetClient());
	newitem->Icon->SetParent(Parent_);
	newitem->Flags=C_BIT_ENABLED;
	if(Dragable)
		newitem->Flags|=C_BIT_DRAGABLE;
	newitem->Dragable=Dragable;
	newitem->state=0;
	newitem->LineColor_[0]=0;
	newitem->LineColor_[1]=0;
	newitem->LineColor_[2]=0;
	newitem->worldx=x;
	newitem->worldy=y;
	newitem->x=(short)(scale_*x);
	if(!WPScaleType_)
		newitem->y=(short)(scale_*y);
	else if(WPScaleType_ == 1)
		newitem->y=(short)(MaxWorldY_ - (28.853f * (log(-y * 0.0001f + 1.0f))));
	else if(WPScaleType_ == 2)
		newitem->y=(short)(MaxWorldY_ + y*0.001f);

	newitem->Icon->SetXY(newitem->x-newitem->Icon->GetW()/2,newitem->y-newitem->Icon->GetH()/2);
	newitem->Next=NULL;

	if(Root_ == NULL)
		Root_=newitem;
	else
	{
		cur=Root_;
		while(cur->Next)
			cur=cur->Next;
		cur->Next=newitem;
	}
	return(newitem);
}

void C_Waypoint::EraseWaypointList()
{
	WAYPOINTLIST *cur,*last;

	cur=Root_;

	if(Root_ == NULL || Parent_ == NULL)
		return;

	Root_=NULL;
	LastWP_=NULL;
	while(cur)
	{
		last=cur;
		cur=cur->Next;
		if(last->Icon)
		{
			last->Icon->Cleanup();
			delete last->Icon;
		}
		delete last;
	}
}

void C_Waypoint::EraseWaypointGroup(long groupid)
{
	WAYPOINTLIST *cur,*last,*prev;

	if(Root_ == NULL || Parent_ == NULL)
		return;

	while(Root_ && Root_->Group == groupid)
	{
		last=Root_;
		Root_=Root_->Next;
		if(last->Icon)
		{
			last->Icon->Cleanup();
			delete last->Icon;
		}
		delete last;
	}

	cur=Root_;
	while(cur)
	{
		prev=cur;
		cur=cur->Next;
		if(cur)
		{
			if(cur->Group == groupid)
			{
				last=cur;
				prev->Next=cur->Next;
				if(last->Icon)
				{
					last->Icon->Cleanup();
					delete last->Icon;
				}
				delete last;
				cur=prev;
			}
		}
	}
}

void C_Waypoint::SetLabel(long ID,_TCHAR *txt)
{
	WAYPOINTLIST *cur;

	cur=FindID(ID);
	if(cur)
		cur->Icon->SetAllLabel(gStringMgr->GetText(gStringMgr->AddText(txt)));
}

void C_Waypoint::SetLabelColor(long ID,COLORREF norm,COLORREF sel,COLORREF othr)
{
	WAYPOINTLIST *cur;

	cur=FindID(ID);
	if(cur)
	{
		cur->Icon->SetLabelColor(C_STATE_0,norm);
		cur->Icon->SetLabelColor(C_STATE_1,sel);
		cur->Icon->SetLabelColor(C_STATE_2,othr);
	}
}

void C_Waypoint::SetLineColor(long ID,COLORREF norm,COLORREF sel,COLORREF othr)
{
	WAYPOINTLIST *cur;

	cur=FindID(ID);
	if(cur)
	{
		cur->LineColor_[0]=norm;
		cur->LineColor_[1]=sel;
		cur->LineColor_[2]=othr;
	}
}

_TCHAR *C_Waypoint::GetLabel(long ID)
{
	WAYPOINTLIST *cur;

	cur=FindID(ID);
	if(cur)
		return(cur->Icon->GetLabel(C_STATE_0));
	return(NULL);
}

void C_Waypoint::SetTextOffset(long ID,short x,short y)
{
	WAYPOINTLIST *cur;

	cur=FindID(ID);
	if(cur)
	{
		cur->Icon->SetLabelOffset(C_STATE_0,x,y);
		cur->Icon->SetLabelOffset(C_STATE_1,x,y);
		cur->Icon->SetLabelOffset(C_STATE_2,x,y);
		cur->Icon->SetLabelInfo();
	}
}

void C_Waypoint::SetWPGroup(long ID,long group)
{
	WAYPOINTLIST *cur;

	cur=FindID(ID);
	if(cur)
		cur->Group=group;
}

void C_Waypoint::SetState(long ID,short state)
{
	WAYPOINTLIST *cur;

	cur=FindID(ID);
	if(cur)
	{
		cur->state=state;
		cur->Icon->SetState(state);
	}
}

void C_Waypoint::SetGroupState(long group,short state)
{
	WAYPOINTLIST *cur;

	cur=Root_;
	while(cur)
	{
		if(cur->Group == group)
		{
			cur->state=state;
			cur->Icon->SetState(state);
		}
		cur=cur->Next;
	}
}

void C_Waypoint::SetScaleFactor(float scale)
{
	WAYPOINTLIST *cur;

	if(scale <= 0.0f || scale == scale_ || WPScaleType_)
		return;

	scale_=scale;

	cur=Root_;
	while(cur)
	{
		cur->x=(short)(cur->worldx*scale_);
		cur->y=(short)(cur->worldy*scale_);
		cur->Icon->SetXY(cur->x-cur->Icon->GetW()/2,cur->y-cur->Icon->GetH()/2);
		cur=cur->Next;
	}
}

void C_Waypoint::SetScaleType(short scaletype)
{
	WAYPOINTLIST *cur;

	WPScaleType_=scaletype;


	if(scaletype == 1)
	{
		cur=Root_;
		while(cur)
		{
			cur->y=(short)(MaxWorldY_ - (28.853f * (log(-cur->worldy * 0.0001f + 1.0f)))); // Only care about the Z
			cur->Icon->SetXY(cur->x-cur->Icon->GetW()/2,cur->y-cur->Icon->GetH()/2);
			cur=cur->Next;
		}
	}
	else if(scaletype == 2)
	{
		cur=Root_;
		while(cur)
		{
			cur->y=(short)(MaxWorldY_ + cur->worldy*0.01f);
			cur->Icon->SetXY(cur->x-cur->Icon->GetW()/2,cur->y-cur->Icon->GetH()/2);
			cur=cur->Next;
		}
	}
}

void C_Waypoint::Refresh()
{
	WAYPOINTLIST *cur;
	UI95_RECT rect;
	if(!Ready() || GetFlags() & C_BIT_INVISIBLE || Parent_ == NULL)
		return;

	rect.left=5000;
	rect.top=5000;
	rect.bottom=0;
	rect.right=0;

	cur=Root_;
	while(cur)
	{
		if(!(cur->Flags & C_BIT_INVISIBLE))
		{
			if(cur->Icon->GetX() < rect.left)
				rect.left=cur->Icon->GetX();
			if(cur->Icon->GetY() < rect.top)
				rect.top=cur->Icon->GetY();
			if((cur->Icon->GetX()+cur->Icon->GetW()) > rect.right)
				rect.right=cur->Icon->GetX()+cur->Icon->GetW();
			if((cur->Icon->GetY()+cur->Icon->GetH()) > rect.bottom)
				rect.bottom=cur->Icon->GetY()+cur->Icon->GetH();
		}
		cur=cur->Next;
	}
	Parent_->SetUpdateRect(last_.left,last_.top,last_.right,last_.bottom,GetFlags(),GetClient());
	Parent_->SetUpdateRect(rect.left,rect.top,rect.right,rect.bottom,GetFlags(),GetClient());
	last_=rect;
}

void C_Waypoint::Draw(SCREEN *surface,UI95_RECT *cliprect)
{
	WAYPOINTLIST *cur,*prev;

	if(!Ready()) return;

	cur=Root_;
	prev=cur;
	while(cur)
	{
		if(cur->ID)
		{
			if(!(cur->Flags & C_BIT_INVISIBLE))
			{
				if(cur->Flags & C_BIT_USELINE)
					if(prev != cur)
						if(prev->Group == cur->Group)
							Parent_->DrawLine(surface,cur->LineColor_[cur->state],prev->x,prev->y,cur->x,cur->y,GetFlags(),GetClient(),cliprect);

				if(cur->Icon)
					cur->Icon->Draw(surface,cliprect);
			}
			prev=cur;
		}
		cur=cur->Next;
	}
}

WAYPOINTLIST *C_Waypoint::FindID(long iID)
{
	WAYPOINTLIST *cur;

	cur=Root_;
	while(cur)
	{
		if(cur->ID == iID)
			return(cur);
		cur=cur->Next;
	}
	return(NULL);
}

BOOL C_Waypoint::UpdateInfo(long ID,float x,float y)
{
	WAYPOINTLIST *cur,*wk1,*wk2;
	short ox,oy;
	float dx,dy;
	_TCHAR buf[15];

	cur=FindID(ID);
	if(cur == NULL)
		return(FALSE);

	if(cur->worldx != x || cur->worldy != y)
	{
		cur->worldx=x;
		cur->worldy=y;
		ox=cur->x;
		oy=cur->y;
		cur->x=(short)(cur->worldx*scale_);
		cur->y=(short)(cur->worldy*scale_);
		cur->Icon->SetXY(cur->x - cur->Icon->GetW()/2,cur->y - cur->Icon->GetH()/2);

		if(!(ID & 0x60000000))
		{
			wk1=FindID(ID-1);
			wk2=FindID(0x40000000+ID);
			if(wk1 && wk2)
			{
				dx=(cur->worldx - wk1->worldx);
				dy=(cur->worldy - wk1->worldy);

				wk2->worldx = wk1->worldx + dx * .5F;
				wk2->worldy = wk1->worldy + dy * .5F;
				wk2->x=(short)(wk2->worldx*scale_);
				wk2->y=(short)(wk2->worldy*scale_);
				_stprintf(buf,"%6.1f",sqrt(dx * dx + dy * dy)*FT_TO_NM);
				wk2->Icon->SetXY(wk2->x - wk2->Icon->GetW()/2,wk2->y - wk2->Icon->GetH()/2);
				wk2->Icon->SetAllLabel(gStringMgr->GetText(gStringMgr->AddText(buf)));
			}
			wk1=FindID(ID+1);
			wk2=FindID(0x60000000+ID+1);
			if(wk1 && wk2)
			{
				dx=(wk1->worldx - cur->worldx);
				dy=(wk1->worldy - cur->worldy);

				wk2->worldx = cur->worldx + dx * .5F;
				wk2->worldy = cur->worldy + dy * .5F;
				wk2->x=(short)(wk2->worldx*scale_);
				wk2->y=(short)(wk2->worldy*scale_);
				_stprintf(buf,"%6.1f",sqrt(dx * dx + dy * dy)*FT_TO_NM);
				wk2->Icon->SetXY(wk2->x - wk2->Icon->GetW()/2,wk2->y - wk2->Icon->GetH()/2);
				wk2->Icon->SetAllLabel(gStringMgr->GetText(gStringMgr->AddText(buf)));
			}
		}
		if((ox != cur->x || oy != cur->y) && !(cur->Flags & C_BIT_INVISIBLE))
			return(TRUE);
	}
	return(FALSE);
}

long C_Waypoint::CheckHotSpots(long relX,long relY)
{
	WAYPOINTLIST *cur;

	LastWP_=NULL;
	cur=Root_;
	while(cur)
	{
		if(!(cur->Flags & C_BIT_INVISIBLE) && cur->Flags & C_BIT_ENABLED)
		{
			if(cur->Icon->CheckHotSpots(relX,relY))
				LastWP_=cur;
		}
		cur=cur->Next;
	}
	if(LastWP_)
	{
		SetRelXY(relX-GetX(),relY-GetY());
		return(LastWP_->ID);
	}
	return(0);
}

BOOL C_Waypoint::MouseOver(long relX,long relY,C_Base *)
{
	WAYPOINTLIST *cur;

	cur=Root_;
	while(cur)
	{
		if(!(cur->Flags & C_BIT_INVISIBLE) && cur->Flags & C_BIT_ENABLED)
		{
			if(cur->Icon && cur->Icon->MouseOver(relX,relY,cur->Icon)) // possible CTD fix
			{
				return(TRUE);
			}
		}
		cur=cur->Next;
	}
	return(FALSE);
}

BOOL C_Waypoint::Drag(GRABBER *Drag,WORD MouseX,WORD MouseY,C_Window *over)
{
	WAYPOINTLIST *Waypoint,*wk1,*wk2;
	long relx,rely;
	long x,y;
	float dx,dy;
	_TCHAR buf[15];
	F4CSECTIONHANDLE* Leave;

	if(GetFlags() & C_BIT_INVISIBLE || !(GetFlags() & C_BIT_ENABLED) || !Dragable(0))
		return(FALSE);

	if(over != Parent_)
		return(FALSE);

	if(!(GetFlags() & C_BIT_ABSOLUTE))
	{
		relx=MouseX - over->GetX();
		rely=MouseY- over->GetY();

		if(relx < over->ClientArea_[GetClient()].left || relx > over->ClientArea_[GetClient()].right)
			return(FALSE);
		if(rely < over->ClientArea_[GetClient()].top || rely > over->ClientArea_[GetClient()].bottom)
			return(FALSE);
	}

	if(LastWP_ == NULL)
		return(FALSE);

	if(!(LastWP_->Flags & C_BIT_DRAGABLE))
		return(FALSE);

	Leave=UI_Enter(Parent_);
	Waypoint=LastWP_;

#if 0
	cur=Root_;
	prev=NULL;
	if(cur != Waypoint)
	{
		prev=cur;
		while(cur->Next && cur->Next != Waypoint)
		{
			prev=cur;
			cur=cur->Next;
		}
	}

	next=Waypoint->Next;

	rect.left=Waypoint->x;
	rect.top=Waypoint->y;
	rect.right=Waypoint->x;
	rect.bottom=Waypoint->y;

	if(prev)
	{
		if(prev->x < rect.left)
			rect.left=prev->x;
		if(prev->y < rect.top)
			rect.top=prev->y;
		if(prev->x > rect.right)
			rect.right=prev->x;
		if(prev->y > rect.bottom)
			rect.bottom=prev->y;
//		prev->Icon->Refresh();
	}
	if(next)
	{
		if(next->x < rect.left)
			rect.left=next->x;
		if(next->y < rect.top)
			rect.top=next->y;
		if(next->x > rect.right)
			rect.right=next->x;
		if(next->y > rect.bottom)
			rect.bottom=next->y;
//		next->Icon->Refresh();
	}
//	Waypoint->Icon->Refresh();
#endif
	Dragging_=1;


	if(GetType() == C_TYPE_DRAGXY || GetType() == C_TYPE_DRAGX)
		x=Drag->ItemX_ + (MouseX - Drag->StartX_);
	else
		x=Waypoint->x;

	if(GetType() == C_TYPE_DRAGXY || GetType() == C_TYPE_DRAGY)
		y=Drag->ItemY_ + (MouseY - Drag->StartY_);
	else
		y=Waypoint->y;

	if(x < (over->ClientArea_[GetClient()].left-over->VX_[GetClient()]))
		x=over->ClientArea_[GetClient()].left-over->VX_[GetClient()];
	if(x > (over->ClientArea_[GetClient()].right-over->VX_[GetClient()]))
		x=over->ClientArea_[GetClient()].right-over->VX_[GetClient()];
	if(y < (over->ClientArea_[GetClient()].top-over->VY_[GetClient()]))
		y=over->ClientArea_[GetClient()].top-over->VY_[GetClient()];
	if(y > (over->ClientArea_[GetClient()].bottom-over->VY_[GetClient()]))
		y=over->ClientArea_[GetClient()].bottom-over->VY_[GetClient()];

	Waypoint->x=static_cast<short>(x);
	Waypoint->y=static_cast<short>(y);

	if(!WPScaleType_)
	{
		Waypoint->worldx=x/scale_;
		Waypoint->worldy=y/scale_;
	}
	else if(WPScaleType_ == 1)
	{
		Waypoint->worldy=-((float)exp((MaxWorldY_ - y)*0.034658441f)-1.0f) * 10000.0f; //(0.03nnn = 1/28.853)
		if(Waypoint->worldy > 0)
			Waypoint->worldy=0;
	}
	else if(WPScaleType_ == 2)
	{
		Waypoint->worldy=-(float)((MaxWorldY_ - y) * 100);
		if(Waypoint->worldy > 0)
			Waypoint->worldy=0;
	}
	Waypoint->Icon->SetXY(x-Waypoint->Icon->GetW()/2,y-Waypoint->Icon->GetH()/2);

	if(!(Waypoint->ID & 0x60000000))
	{
		wk1=FindID(Waypoint->ID-1);
		wk2=FindID(0x40000000+Waypoint->ID);
		if(wk1 && wk2)
		{
			dx=(Waypoint->worldx - wk1->worldx);
			dy=(Waypoint->worldy - wk1->worldy);

			wk2->worldx = wk1->worldx + dx * .5F;
			wk2->worldy = wk1->worldy + dy * .5F;
			wk2->x=(short)(wk2->worldx*scale_);
			wk2->y=(short)(wk2->worldy*scale_);
			wk2->Icon->SetXY(wk2->x - wk2->Icon->GetW()/2,wk2->y - wk2->Icon->GetH()/2);
			_stprintf(buf,"%6.1f",sqrt(dx * dx + dy * dy)*FT_TO_NM);
			wk2->Icon->SetAllLabel(gStringMgr->GetText(gStringMgr->AddText(buf)));
		}
		wk1=FindID(Waypoint->ID+1);
		wk2=FindID(0x40000000+Waypoint->ID+1);
		if(wk1 && wk2)
		{
			dx=(wk1->worldx - Waypoint->worldx);
			dy=(wk1->worldy - Waypoint->worldy);

			wk2->worldx = Waypoint->worldx + dx * .5F;
			wk2->worldy = Waypoint->worldy + dy * .5F;
			wk2->x=(short)(wk2->worldx*scale_);
			wk2->y=(short)(wk2->worldy*scale_);
			wk2->Icon->SetXY(wk2->x - wk2->Icon->GetW()/2,wk2->y - wk2->Icon->GetH()/2);
			_stprintf(buf,"%6.1f",sqrt(dx * dx + dy * dy)*FT_TO_NM);
			wk2->Icon->SetAllLabel(gStringMgr->GetText(gStringMgr->AddText(buf)));
		}
	}
	Parent_->RefreshClient(GetClient());

	if(Callback_)
		(*Callback_)(LastWP_->ID,C_TYPE_MOUSEMOVE,this);
	UI_Leave(Leave);

	return(TRUE);
}

void C_Waypoint::GetItemXY(long ID,long *x,long *y)
{
	WAYPOINTLIST *Waypoint;

	Waypoint=FindID(ID);
	if(Waypoint == NULL)
		return;

	*x=Waypoint->x;
	*y=Waypoint->y;
}

BOOL C_Waypoint::Drop(GRABBER *,WORD ,WORD ,C_Window *)
{
	Dragging_=0;
	return(0);
}

void C_Waypoint::SetSubParents(C_Window *par)
{
	WAYPOINTLIST *cur;

	cur=Root_;
	while(cur)
	{
		cur->Icon->SetParent(par);
		cur->Icon->SetSubParents(par);
		cur=cur->Next;
	}
}
