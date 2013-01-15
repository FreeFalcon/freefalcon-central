#include "stdafx.h"
#include "IsBad.h"
	   
_TCHAR *OrdinalString(long value);

#define ICON_UKN 10126 // 2002-02-21 S.G.
extern int gShowUnknown; // 2002-02-21 S.G.

static void DeleteCB(void *record)
{
	MAPICONLIST *last;
	last=(MAPICONLIST *)record;
	if(!last)
		return;

	ShiAssert(FALSE == F4IsBadReadPtr(last, sizeof(*last))); //JPO
	if (F4IsBadReadPtr(last, sizeof(MAPICONLIST))) // JB 010305 CTD
		return; // JB 010305 CTD

	if(last->Icon)
	{
		last->Icon->Cleanup();
		delete last->Icon;
	}					   
	if(last->Label)
	{
		last->Label->Cleanup();
		delete last->Label;
	}
	if(last->Div)
	{
		last->Div->Cleanup();
		delete last->Div;
	}
	if(last->Brig)
	{
		last->Brig->Cleanup();
		delete last->Brig;
	}
	if(last->Bat)
	{
		last->Bat->Cleanup();
		delete last->Bat;
	}
	if(last->Detect)
	{
	    if (last->Detect->LowRadar) { // JPO fix memory leak fix
		delete [] last->Detect->LowRadar->arcs;
		delete last->Detect->LowRadar;
	    }
	    delete last->Detect;
	}
	delete last;
}

C_MapIcon::C_MapIcon() : C_Control()
{
	short i;

	Root_=NULL;
	Last_=NULL;
	_SetCType_(_CNTL_MAPICON_);
	SetReady(0);
	ShowCircles_=0;
	DefaultFlags_=C_BIT_ENABLED|C_BIT_SELECTABLE|C_BIT_HCENTER|C_BIT_MOUSEOVER;
	SetDefaultFlags();
	Team_=0;
	for(i=0;i<NUM_DIRECTIONS;i++)
	{
		Icons_[i][0]=NULL;
		Icons_[i][1]=NULL;
	}
	scale_=1.0f;
	LastTime_=0;
	CurTime_=0;
}

C_MapIcon::C_MapIcon(char **stream) : C_Control(stream)
{
}

C_MapIcon::C_MapIcon(FILE *fp) : C_Control(fp)
{
}

C_MapIcon::~C_MapIcon()
{
	if(Root_)
		Cleanup();
}

long C_MapIcon::Size()
{
	return(0);
}

void C_MapIcon::Setup(long ID,short Type)
{
	// Init Hash Table
	Root_=new C_Hash;
	Root_->Setup(512);
	Root_->SetCallback(DeleteCB);
	Root_->SetFlags(C_BIT_REMOVE);

	SetID(ID);
	SetType(Type);
	SetGroup(0);
}

void C_MapIcon::Cleanup()
{
	if(Root_)
	{
		Root_->Cleanup();
		delete Root_;
	}
	Root_=NULL;
	Last_=NULL;
}

BOOL C_MapIcon::ShowByType(long typemask)
{
	C_HASHNODE *current;
	long curidx;
	MAPICONLIST *cur;
	BOOL retval=FALSE;

	cur=(MAPICONLIST*)Root_->GetFirst(&current,&curidx);

	while(cur)
	{
		if(cur->Type & typemask)
		{
			cur->Flags &= ~C_BIT_INVISIBLE;
			retval=TRUE;
		}
		cur=(MAPICONLIST*)Root_->GetNext(&current,&curidx);
	}
	return(retval);
}

BOOL C_MapIcon::HideByType(long typemask)
{
	C_HASHNODE *current;
	long curidx;
	MAPICONLIST *cur;
	BOOL retval=FALSE;

	cur=(MAPICONLIST*)Root_->GetFirst(&current,&curidx);

	while(cur)
	{
		if(cur->Type & typemask)
		{
			cur->Flags |= C_BIT_INVISIBLE;
			retval=TRUE;
		}
		cur=(MAPICONLIST*)Root_->GetNext(&current,&curidx);
	}
	return(retval);
}

void C_MapIcon::Show()
{
	C_HASHNODE *current;
	long curidx;
	MAPICONLIST *cur;

	cur=(MAPICONLIST*)Root_->GetFirst(&current,&curidx);

	while(cur)
	{
		cur->Flags &= ~C_BIT_INVISIBLE;
		cur=(MAPICONLIST*)Root_->GetNext(&current,&curidx);
	}
}

void C_MapIcon::Hide()
{
	C_HASHNODE *current;
	long curidx;
	MAPICONLIST *cur;

	cur=(MAPICONLIST*)Root_->GetFirst(&current,&curidx);

	while(cur)
	{
		cur->Flags |= C_BIT_INVISIBLE;
		cur=(MAPICONLIST*)Root_->GetNext(&current,&curidx);
	}
}

BOOL C_MapIcon::Process(long ID,short HitType)
{
	if(Last_)
	{
		gSoundMgr->PlaySound(GetSound(HitType));
		if(Callback_)
			(*Callback_)(ID,HitType,this);
		return(TRUE);
	}
	return(FALSE);
}

void C_MapIcon::SetMainImage(long OffID,long OnID)
{
	short i;

	Icons_[0][0]=gImageMgr->GetImageRes(OffID);
	Icons_[0][1]=gImageMgr->GetImageRes(OnID);

	for(i=1;i<NUM_DIRECTIONS;i++)
	{
		Icons_[i][0]=Icons_[0][0];
		Icons_[i][1]=Icons_[0][1];
	}

	if(Icons_[0][0])
		SetReady(1);
	else
		SetReady(0);
}

void C_MapIcon::SetMainImage(short idx,long OffID,long OnID)
{
	if(idx >= NUM_DIRECTIONS)
		return;

	Icons_[idx][0]=gImageMgr->GetImageRes(OffID);
	Icons_[idx][1]=gImageMgr->GetImageRes(OnID);

	if(Icons_[0][0])
		SetReady(1);
	else
		SetReady(0);
}

void C_MapIcon::RemapIconImages()
{
	IMAGE_RSC *img;
	C_HASHNODE *current;
	long curidx;
	MAPICONLIST *cur;
	F4CSECTIONHANDLE *Leave;

	Leave=UI_Enter(Parent_);
	cur=(MAPICONLIST*)Root_->GetFirst(&current,&curidx);
	while(cur)
	{
		if(cur->Icon)
		{
			img=(IMAGE_RSC *)Icons_[cur->state][0]->Find(cur->ImageID);
			if(!img)
			{
				MonoPrint("C_MapIcon::RemapIconImages() Image ID (%1ld) - Not found\n",cur->ImageID);
				return;
			}
			cur->Icon->SetImage(img);
		}
		cur=(MAPICONLIST*)Root_->GetNext(&current,&curidx);
	}
	UI_Leave(Leave);
}


MAPICONLIST *C_MapIcon::AddIconToList(
	long CampID,
	short type, long ImageID,
	float x, float y,
	short Dragable, _TCHAR *str, 
	long DivID, long BrigID, long BatID,
	long newstatus, long newstate,
	DETECTOR *detector
){
	MAPICONLIST *newitem;
	IMAGE_RSC *img = NULL;
	_TCHAR buf[10];

	if(!ImageID || Root_->Find(CampID) || !Icons_[0])
		return(NULL);

	newitem=new MAPICONLIST;
	if(newitem == NULL)
		return(NULL);

	if (Icons_[newstate][0])
		img=(IMAGE_RSC *)Icons_[newstate][0]->Find(ImageID);

	if(!img)
	{
		MonoPrint("C_MapIcon::AddIconToList() Image ID (%1ld) - Not found\n",ImageID);
		return(NULL);
	}
	if(img->Header->Type != _RSC_IS_IMAGE_)
	{
		MonoPrint("C_MapIcon::AddIconToList() Image ID (%1ld) - Not an Image\n",ImageID);
		return(NULL);
	}

	newitem->ID=CampID;
	newitem->Type=type;
	newitem->worldx=x;
	newitem->worldy=y;
	newitem->x=(short)(scale_*x);
	newitem->y=(short)(scale_*y);
	newitem->state=newstate;
	newitem->Flags=C_BIT_ENABLED;
	if(Dragable)
		newitem->Flags |= C_BIT_DRAGABLE;
	newitem->ImageID=ImageID;
	newitem->Dragable=Dragable;
	newitem->Status=newstatus;
	newitem->Detect=detector;
	newitem->Owner=this;
	newitem->Icon=new O_Output;
	newitem->Icon->SetOwner(this);
	newitem->Icon->SetXY(-img->Header->centerx,-img->Header->centery);
	newitem->Icon->SetImage(img);

	newitem->Icon->SetWH(img->Header->w,img->Header->h);
	newitem->Div=NULL;
	newitem->Brig=NULL;
	newitem->Bat=NULL;
	if(DivID)
	{
		_stprintf(buf,"%1ld",DivID);
		newitem->Div=new O_Output;
		newitem->Div->SetOwner(this);
		newitem->Div->SetFont(Font_);
		newitem->Div->SetFgColor(0x00f0f0f0);
		newitem->Div->SetText(gStringMgr->GetText(gStringMgr->AddText(buf)));
		newitem->Div->SetXY(newitem->Icon->GetX()-4,-img->Header->centery);
		newitem->Div->SetFlags(C_BIT_RIGHT);
		newitem->Div->SetInfo();
	}
	if(BrigID)
	{
		_stprintf(buf,"%1ld",BrigID);
		newitem->Brig=new O_Output;
		newitem->Brig->SetOwner(this);
		newitem->Brig->SetFont(Font_);
		newitem->Brig->SetFgColor(0x00f0f0f0);
		newitem->Brig->SetXY(newitem->Icon->GetX()+newitem->Icon->GetW() + 5,-img->Header->centery);
		newitem->Brig->SetFlags(C_BIT_LEFT);
		newitem->Brig->SetText(gStringMgr->GetText(gStringMgr->AddText(buf)));
		newitem->Brig->SetInfo();
	}
	if(BatID)
	{
		_stprintf(buf,"%1ld",BatID);
		newitem->Bat=new O_Output;
		newitem->Bat->SetOwner(this);
		newitem->Bat->SetFgColor(0x00f0f0f0);
		newitem->Bat->SetFont(Font_);
		newitem->Bat->SetXY(newitem->Icon->GetX()+newitem->Icon->GetW()/2,img->Header->centery+2);
		newitem->Bat->SetFlags(C_BIT_HCENTER);
		newitem->Bat->SetText(gStringMgr->GetText(gStringMgr->AddText(buf)));
		newitem->Bat->SetInfo();
	}
	newitem->Label=new O_Output;
	newitem->Label->SetOwner(this);
	newitem->Label->SetFont(Font_);
	newitem->Label->SetFgColor(0x00f0f0f0);
	newitem->Label->SetFlags(GetFlags());
	newitem->Label->SetXY(newitem->Icon->GetX()+newitem->Icon->GetW()/2,img->Header->centery+5);
	newitem->Label->SetFlags(C_BIT_HCENTER);
	if(str)
		newitem->Label->SetText(gStringMgr->GetText(gStringMgr->AddText(str)));
	newitem->Label->SetInfo();

	Root_->Add(CampID,newitem);
	return(newitem);
}

void C_MapIcon::RemoveIcon(long ID)
{
	F4CSECTIONHANDLE *Leave;

	Leave=UI_Enter(Parent_);
	Root_->Remove(ID);
	UI_Leave(Leave);
}

void C_MapIcon::SetLabel(long ID,_TCHAR *txt)
{
	MAPICONLIST *cur;

	cur=(MAPICONLIST*)Root_->Find(ID);
	if(cur)
		cur->Label->SetText(gStringMgr->GetText(gStringMgr->AddText(txt)));
}

void C_MapIcon::SetColor(long ID,COLORREF color)
{
	MAPICONLIST *cur;

	cur=(MAPICONLIST*)Root_->Find(ID);
	if(cur)
		cur->Label->SetFgColor(color);
}

_TCHAR *C_MapIcon::GetLabel(long ID)
{
	MAPICONLIST *cur;

	cur=(MAPICONLIST*)Root_->Find(ID);
	if(cur)
		return(cur->Label->GetText());
	return(NULL);
}

void C_MapIcon::SetTextOffset(long ID,short x,short y)
{
	MAPICONLIST *cur;

	cur=(MAPICONLIST*)Root_->Find(ID);
	if(cur)
		cur->Label->SetXY(x,y);
}

void C_MapIcon::SetScaleFactor(float scale)
{
	C_HASHNODE *current;
	long curidx;
	MAPICONLIST *cur;

	if(scale <= 0.0f || scale == scale_)
		return;

	scale_=scale;

	cur=(MAPICONLIST*)Root_->GetFirst(&current,&curidx);
	while(cur)
	{
		cur->x=(short)(cur->worldx*scale_);
		cur->y=(short)(cur->worldy*scale_);
		cur=(MAPICONLIST*)Root_->GetNext(&current,&curidx);
	}
}

long C_MapIcon::GetHelpText()
{
	long ID=0;

	if(!OverLast_)
		return(0);

	if(OverLast_->Label)
		ID=gStringMgr->AddText(OverLast_->Label->GetText());

	return(ID);
}

void C_MapIcon::Refresh(MAPICONLIST *icon)
{
	F4CSECTIONHANDLE *Leave;

// 2020-02-21 MODIFIED BY S.G. Even if that type of airplane isn't showned, if it's an Unknown, deal with it
//	if(!Ready() || (GetFlags() & C_BIT_INVISIBLE) || (!icon) || (Parent_ == NULL))
	if(!Ready() || (!icon) || (Parent_ == NULL))
		return;

	// 2002-02-21 ADDED BY S.G. 'Fog of war code'. If an enemy flight and not identified, not editing a TE and 'showUnknown' isn't set, hide it
	if (!gShowUnknown && icon->ImageID == ICON_UKN) {
			icon->Flags |= C_BIT_INVISIBLE;
			Leave=UI_Enter(Parent_);
			SetXY(icon->x,icon->y);
			icon->Icon->Refresh();
			UI_Leave(Leave);
	}
	if ((!gShowUnknown || icon->ImageID != ICON_UKN) && (GetFlags() & C_BIT_INVISIBLE)) // If the template shouldn't be displayed and it's not an unknown and we're not looking at unknown, then don't continue otherwise this will display it
		return;
	// END OF ADDED SECTION 2002-02-21

	if(!(icon->Flags & C_BIT_ENABLED) || (icon->Flags & C_BIT_INVISIBLE))
		return;

	if(icon->Icon)
	{
		Leave=UI_Enter(Parent_);
		SetXY(icon->x,icon->y);
		icon->Icon->Refresh();
		if(!(GetFlags() & C_BIT_NOLABEL))
		{
			if(icon->Div)
				icon->Div->Refresh();
			if(icon->Brig)
				icon->Brig->Refresh();
			if(icon->Bat)
				icon->Bat->Refresh();
			if(icon->Label)
				icon->Label->Refresh();
		}
		UI_Leave(Leave);
	}
}

void C_MapIcon::Refresh()
{
	C_HASHNODE *current;
	long curidx;
	MAPICONLIST *cur;
	F4CSECTIONHANDLE *Leave;

// 2020-02-21 MODIFIED BY S.G. Even if that type of airplane isn't showned, if it's an Unknown, deal with it
//	if(!Ready() || (GetFlags() & C_BIT_INVISIBLE) || (Parent_ == NULL))
	if(!Ready() || (Parent_ == NULL))
		return;

	Leave=UI_Enter(Parent_);
	cur=(MAPICONLIST*)Root_->GetFirst(&current,&curidx);
	while(cur)
	{
		// 2002-02-21 ADDED BY S.G. 'Fog of war code'. If an enemy flight and not identified, not editing a TE and 'showUnknown' isn't set, hide it
		if (!gShowUnknown && cur->ImageID == ICON_UKN) {
				cur->Flags |= C_BIT_INVISIBLE;
				SetXY(cur->x,cur->y);
				cur->Icon->Refresh();
		}

//		if (!(!(gShowUnknown && cur->ImageID == ICON_UKN) && (GetFlags() & C_BIT_INVISIBLE))) { // (From above) If the template shouldn't be displayed and it's not an unknown and we're not looking at unknown, then don't continue otherwise this will display it
		if ((gShowUnknown && cur->ImageID == ICON_UKN) || !(GetFlags() & C_BIT_INVISIBLE)) { // If the template shouldn't be displayed and it's not an unknown and we're not looking at unknown, then don't continue otherwise this will display it
		// END OF ADDED SECTION 2002-02-21
			if(!(cur->Flags & C_BIT_INVISIBLE) && cur->Flags & C_BIT_ENABLED)
			{
				if(cur->Icon)
				{
					SetXY(cur->x,cur->y);
					cur->Icon->Refresh();
					if(!(GetFlags() & C_BIT_NOLABEL))
					{
						if(cur->Div)
							cur->Div->Refresh();
						if(cur->Brig)
							cur->Brig->Refresh();
						if(cur->Bat)
							cur->Bat->Refresh();
						if(cur->Label)
							cur->Label->Refresh();
					}
				}
			}
		}
		cur=(MAPICONLIST*)Root_->GetNext(&current,&curidx);
	}
	UI_Leave(Leave);
}

// 2002-02-23 ADDED BY S.G. Need to include them for the gMapMgr hack below to work
#include "../Campaign/Include/team.h"
#include "../Campaign/Include/package.h"
#include "../Campaign/Include/division.h"
#include "../ui/include/cmap.h"
extern C_Map *gMapMgr;
// END OF ADDED SECTION 2002-02-23

void C_MapIcon::Draw(SCREEN *surface,UI95_RECT *cliprect)
{
	MAPICONLIST *cur;
	C_HASHNODE *CurHash;
	long HashIdx;

// 2020-02-21 MODIFIED BY S.G. Even if that type of airplane isn't showned, if it's an Unknown, deal with it if we ask for unknown to be seen
//	if(!Ready() || GetFlags() & C_BIT_INVISIBLE || Parent_ == NULL) return;
	if(!Ready() || (!gShowUnknown && (GetFlags() & C_BIT_INVISIBLE)) || Parent_ == NULL)
		return;

	// 2002-02-23 ADDED BY S.G. Test if it's an AirUnits C_MapIcon and only those will go in if the invisible flag is set
	int airUnit = FALSE;
	if (gShowUnknown && gMapMgr && (GetFlags() & C_BIT_INVISIBLE)) {
		for (int i = 0; i < _MAX_TEAMS_ && !airUnit; i++) {
			if (gMapMgr->GetTeam(i).AirUnits) {
				for (int j = 0; j <_MAP_NUM_AIR_TYPES_ && !airUnit; j++) {
					if (gMapMgr->GetTeam(i).AirUnits->Type[j] == this)
						airUnit = TRUE;
				}
			}
		}
		if (!airUnit)
			return;
	}
	// END OF ADDED SECTION 2002-02-23

	cur=(MAPICONLIST*)Root_->GetFirst(&CurHash,&HashIdx);
	while(cur)
	{
		// 2002-02-21 ADDED BY S.G. 'Fog of war code'. If an enemy flight and not identified, not editing a TE and 'showUnknown' isn't set, hide it
		if (!gShowUnknown && cur->ImageID == ICON_UKN)
				cur->Flags |= C_BIT_INVISIBLE;

		if ((gShowUnknown && cur->ImageID == ICON_UKN) || !(GetFlags() & C_BIT_INVISIBLE)) { // If the template shouldn't be displayed and it's not an unknown and we're not looking at unknown, then don't continue otherwise this will display it
		// END OF ADDED SECTION 2002-02-21
			if(!(cur->Flags & C_BIT_INVISIBLE) && cur->Flags & C_BIT_ENABLED)
			{
				if(cur->Icon){
					SetXY(cur->x,cur->y);
					if(cur->Detect && ShowCircles_)
					{
						if(ShowCircles_ & LOW_SAM)
						{
							if(cur->Detect->LowSam)
								Parent_->DrawCircle(surface,0x0000ff,cur->x,cur->y,cur->Detect->LowSam * scale_,Flags_,Client_,cliprect);
						}
						if(ShowCircles_ & HIGH_SAM)
						{
							if(cur->Detect->HighSam)
								Parent_->DrawCircle(surface,0x0000aa,cur->x,cur->y,cur->Detect->HighSam * scale_,Flags_,Client_,cliprect);
						}
						if(ShowCircles_ & HIGH_RADAR)
						{
							if(cur->Detect->HighRadar)
								Parent_->DrawCircle(surface,0xaa0000,cur->x,cur->y,cur->Detect->HighRadar * scale_,Flags_,Client_,cliprect);
						}
						if(ShowCircles_ & LOW_RADAR)
						{
							if(cur->Detect->LowRadar && cur->Detect->LowRadar->arcs && cur->Detect->LowRadar->arcs[0].range)
								Parent_->DrawCircle(surface,0xff0000,cur->x,cur->y,cur->Detect->LowRadar->arcs[0].range * scale_,Flags_,Client_,cliprect);
						}
					}
					cur->Icon->Draw(surface,cliprect);
					if(!(GetFlags() & C_BIT_NOLABEL))
					{
						if(cur->Div)
							cur->Div->Draw(surface,cliprect);
						if(cur->Brig)
							cur->Brig->Draw(surface,cliprect);
						if(cur->Bat)
							cur->Bat->Draw(surface,cliprect);
						if(cur->Label && (!cur->Div && !cur->Brig && !cur->Bat))
							cur->Label->Draw(surface,cliprect);
					}
				}
			}
		}
		cur=(MAPICONLIST*)Root_->GetNext(&CurHash,&HashIdx);
	}
}

MAPICONLIST *C_MapIcon::FindID(long iID)
{
	return((MAPICONLIST*)Root_->Find(iID));
}

BOOL C_MapIcon::UpdateInfo(MAPICONLIST *icon,float x,float y,long newstatus,long newstate)
{
	short ox,oy;
	F4CSECTIONHANDLE *Leave=NULL;

	if(!icon) return(FALSE);

	if(icon->worldx != x || icon->worldy != y || icon->state != newstate)
	{
		if(icon->Status != newstatus)
		{
			icon->Status=newstatus;
			newstatus=-1;
		}
		if(icon->state != newstate)
		{
			icon->state=newstate;
			newstate=-1;
		}
		icon->worldx=x;
		icon->worldy=y;
		ox=icon->x;
		oy=icon->y;
		if(ox != ((icon->worldx*scale_)) || oy != (icon->worldy*scale_))
		{
			if(!(icon->Flags & C_BIT_INVISIBLE) && icon->Flags & C_BIT_ENABLED)
			{
				Leave=UI_Enter(icon->Owner->GetParent());
				Refresh(icon);
			}
		}
		icon->x=(short)(icon->worldx*scale_);
		icon->y=(short)(icon->worldy*scale_);
		if((ox != icon->x) || (oy != icon->y) || (icon->state != newstate) || (icon->Status != newstatus))
		{
			if(icon->Icon)
				icon->Icon->SetImage((IMAGE_RSC*)Icons_[icon->state][0]->Find(icon->ImageID));
			Refresh(icon);
			UI_Leave(Leave);
			return(TRUE);
		}
		UI_Leave(Leave);
	}
	return(FALSE);
}

BOOL C_MapIcon::UpdateInfo(long ID,float x,float y,long newstatus,long newstate)
{
	MAPICONLIST *cur;
	short ox,oy;

	cur=(MAPICONLIST*)Root_->Find(ID);
	if(cur == NULL)
		return(FALSE);

	if(cur->worldx != x || cur->worldy != y)
	{
		cur->Status=newstatus;
		cur->state=newstate;
		cur->worldx=x;
		cur->worldy=y;
		ox=cur->x;
		oy=cur->y;
		cur->x=(short)(cur->worldx*scale_);
		cur->y=(short)(cur->worldy*scale_);
		if((ox != cur->x || oy != cur->y) && !(cur->Flags & C_BIT_INVISIBLE))
			return(TRUE);
	}
	return(FALSE);
}

long C_MapIcon::CheckHotSpots(long relX,long relY)
{
	C_HASHNODE *current;
	long curidx;
	MAPICONLIST *cur;
	long x,y,w,h;

	if(!Ready() || GetFlags() & C_BIT_INVISIBLE || Parent_ == NULL || !(GetFlags() & C_BIT_ENABLED)) return(0);

	Last_=NULL;
	cur=(MAPICONLIST*)Root_->GetFirst(&current,&curidx);
	while(cur)
	{
		if(!(cur->Flags & C_BIT_INVISIBLE) && cur->Flags & C_BIT_ENABLED)
		{
			x=cur->x+cur->Icon->GetX();
			y=cur->y+cur->Icon->GetY();
			w=cur->Icon->GetW();
			h=cur->Icon->GetH();

			if(relX >= x && relY >= y && relX < (x+w) && relY < (y+h))
				Last_=cur;
		}
		cur=(MAPICONLIST*)Root_->GetNext(&current,&curidx);
	}
	if(Last_)
	{
		SetRelXY(relX-GetX(),relY-GetY());
		return(Last_->ID);
	}
	return(0);
}

BOOL C_MapIcon::MouseOver(long relX,long relY,C_Base *)
{
	C_HASHNODE *current;
	long curidx;
	MAPICONLIST *cur;
	long x,y,w,h;

	if(!Ready() || GetFlags() & C_BIT_INVISIBLE || Parent_ == NULL) return(0);

	OverLast_=NULL;
	cur=(MAPICONLIST*)Root_->GetFirst(&current,&curidx);
	while(cur)
	{
		if(!(cur->Flags & C_BIT_INVISIBLE) && cur->Flags & C_BIT_ENABLED)
		{
			x=cur->x+cur->Icon->GetX();
			y=cur->y+cur->Icon->GetY();
			w=cur->Icon->GetW();
			h=cur->Icon->GetH();

			if(relX >= x && relY >= y && relX < (x+w) && relY < (y+h))
				OverLast_=cur;
		}
		cur=(MAPICONLIST*)Root_->GetNext(&current,&curidx);
	}
	if(OverLast_)
	{
		SetXY(OverLast_->x+OverLast_->Icon->GetX(),OverLast_->y+OverLast_->Icon->GetY());
		SetWH(OverLast_->Icon->GetW(),OverLast_->Icon->GetH());
		return(TRUE);
	}
	return(FALSE);
}

BOOL C_MapIcon::Drag(GRABBER *Drag,WORD MouseX,WORD MouseY,C_Window *over)
{
	long x,y;
	long relx,rely;
	F4CSECTIONHANDLE* Leave;

	if(GetFlags() & C_BIT_INVISIBLE || !(GetFlags() & C_BIT_ENABLED) || !(GetFlags() & C_BIT_DRAGABLE))
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

	if(!Last_)
		return(FALSE);

	if(!(Last_->Flags & C_BIT_DRAGABLE))
		return(FALSE);

	Leave=UI_Enter(Parent_);
	Refresh();
	x=Drag->ItemX_ + (MouseX - Drag->StartX_);
	y=Drag->ItemY_ + (MouseY - Drag->StartY_);

	if(x < (over->ClientArea_[GetClient()].left-over->VX_[GetClient()]))
		x=over->ClientArea_[GetClient()].left-over->VX_[GetClient()];
	if(x > (over->ClientArea_[GetClient()].right-over->VX_[GetClient()]))
		x=over->ClientArea_[GetClient()].right-over->VX_[GetClient()];
	if(y < (over->ClientArea_[GetClient()].top-over->VY_[GetClient()]))
		y=over->ClientArea_[GetClient()].top-over->VY_[GetClient()];
	if(y > (over->ClientArea_[GetClient()].bottom-over->VY_[GetClient()]))
		y=over->ClientArea_[GetClient()].bottom-over->VY_[GetClient()];

	Last_->x=static_cast<short>(x);//! 
	Last_->y=static_cast<short>(y);//! 
	Last_->worldx = x/scale_;
	Last_->worldy = y/scale_;
	Refresh();
	if(Callback_)
		(*Callback_)(Last_->ID,C_TYPE_MOUSEMOVE,this);
	UI_Leave(Leave);
	return(TRUE);
}

void C_MapIcon::GetItemXY(long ID,long *x,long *y)
{
	MAPICONLIST *Icon;

	Icon=(MAPICONLIST*)Root_->Find(ID);
	if(Icon == NULL)
		return;

	*x=Icon->x;
	*y=Icon->y;
}

