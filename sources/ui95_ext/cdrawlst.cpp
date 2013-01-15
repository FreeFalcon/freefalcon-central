#include "stdafx.h"

C_DrawList::C_DrawList() : C_Control()
{
	_SetCType_(_CNTL_DRAWLIST_);
	Root_=NULL;
	Last_=NULL;
	DefaultFlags_=C_BIT_ENABLED;
}

C_DrawList::C_DrawList(char **stream) : C_Control(stream)
{
}

C_DrawList::C_DrawList(FILE *fp) : C_Control(fp)
{
}

C_DrawList::~C_DrawList()
{
	if(Root_)
		Cleanup();
}

long C_DrawList::Size()
{
	return(0);
}

void C_DrawList::Setup(long ID,short Type)
{
	SetID(ID);
	SetType(Type);
	SetDefaultFlags();
	SetReady(1);
	Root_=new C_Hash;
	Root_->Setup(5);
	SetMenu(0);
}

void C_DrawList::Cleanup(void)
{
	Root_->Cleanup();
	delete Root_;
	Root_=NULL;
	SetMenu(0);
	Last_=NULL;
}

void C_DrawList::Add(MAPICONLIST *item)
{
	if(!Root_)
		return;

	if(Root_->Find(item->ID))
		return;

	Root_->Add(item->ID,item);
}

void C_DrawList::Remove(long ID)
{
	if(!Root_)
		return;

	Root_->Remove(ID);
}

long C_DrawList::CheckHotSpots(long relX,long relY)
{
	MAPICONLIST *item;
	C_HASHNODE *me;
	long curidx;
	long x,y,w,h;

	if(Flags_ & C_BIT_INVISIBLE || Parent_ == NULL)
		return(0);

	Last_=NULL;

	item=(MAPICONLIST*)Root_->GetFirst(&me,&curidx);
	while(item)
	{
		if(!(item->Flags & C_BIT_INVISIBLE) && item->Flags & C_BIT_ENABLED)
		{
			x=item->x+item->Icon->GetX();
			y=item->y+item->Icon->GetY();
			w=item->Icon->GetW();
			h=item->Icon->GetH();

			if(relX >= x && relY >= y && relX < (x+w) && relY < (y+h))
			{
				Last_=item;
				SetMenu(Last_->Owner->GetMenu());
			}
		}
		item=(MAPICONLIST*)Root_->GetNext(&me,&curidx);
	}
	if(Last_)
		return(GetID());
	return(0);
}

BOOL C_DrawList::Process(long ID,short HitType)
{
	gSoundMgr->PlaySound(GetSound(HitType));
	if(Last_ && Last_->Owner)
		return(Last_->Owner->Process(ID,HitType));
	return(FALSE);
}

void C_DrawList::Refresh()
{
	MAPICONLIST *item;
	C_HASHNODE *me;
	long curidx;

	if(Flags_ & C_BIT_INVISIBLE || Parent_ == NULL)
		return;

	item=(MAPICONLIST*)Root_->GetFirst(&me,&curidx);
	while(item)
	{
		if(!(item->Flags & C_BIT_INVISIBLE) && item->Flags & C_BIT_ENABLED)
		{
			if(item->Icon)
			{
				item->Owner->SetXY(item->x,item->y);
				item->Icon->Refresh();
				if(!(GetFlags() & C_BIT_NOLABEL))
				{
					if(item->Div)
						item->Div->Refresh();
					if(item->Brig)
						item->Brig->Refresh();
					if(item->Bat)
						item->Bat->Refresh();
					if(item->Label)
						item->Label->Refresh();
				}
			}
		}
		item=(MAPICONLIST*)Root_->GetNext(&me,&curidx);
	}
}

void C_DrawList::Draw(SCREEN *surface,UI95_RECT *cliprect)
{
	MAPICONLIST *item;
	C_HASHNODE *me;
	long curidx;

	if(Flags_ & C_BIT_INVISIBLE || Parent_ == NULL)
		return;

	item=(MAPICONLIST*)Root_->GetFirst(&me,&curidx);
	while(item)
	{
		if(!(item->Flags & C_BIT_INVISIBLE) && item->Flags & C_BIT_ENABLED)
		{
			if(item->Icon)
			{
				item->Owner->SetXY(item->x,item->y);
				item->Icon->Draw(surface,cliprect);
				if(!(GetFlags() & C_BIT_NOLABEL))
				{
					if(item->Div)
						item->Div->Draw(surface,cliprect);
					if(item->Brig)
						item->Brig->Draw(surface,cliprect);
					if(item->Bat)
						item->Bat->Draw(surface,cliprect);
					if(item->Label && !item->Div && !item->Brig && !item->Bat)
						item->Label->Draw(surface,cliprect);
				}
			}
		}
		item=(MAPICONLIST*)Root_->GetNext(&me,&curidx);
	}
}
