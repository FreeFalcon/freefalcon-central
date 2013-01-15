#include "stdafx.h"

C_MapMover::C_MapMover() : C_Control()
{
	_SetCType_(_CNTL_MAP_MOVER_);
	SX_=0;
	SY_=0;
	Draging_=0;
	DrawCallback_=NULL;
	DefaultFlags_=C_BIT_ENABLED|C_BIT_REMOVE|C_BIT_DRAGABLE|C_BIT_MOUSEOVER;
}

C_MapMover::C_MapMover(char **stream) : C_Control(stream)
{
}

C_MapMover::C_MapMover(FILE *fp) : C_Control(fp)
{
}

C_MapMover::~C_MapMover()
{
}

long C_MapMover::Size()
{
	return(0);
}

void C_MapMover::Setup(long ID,short Type)
{
	SetID(ID);
	SetType(Type);
	SetDefaultFlags();
	SetReady(1);
}

void C_MapMover::Cleanup()
{
	DrawCallback_=NULL;

	SetReady(0);
}

long C_MapMover::CheckHotSpots(long relX,long relY)
{
	if(GetFlags() & C_BIT_INVISIBLE || !(GetFlags() & C_BIT_ENABLED) || !Ready())
		return(0);

	if(relX >= (GetX()) && relX <= (GetX()+GetW()) && relY >= (GetY()) && relY <= (GetY()+GetH()))
	{
		SetRelXY(relX-GetX(),relY-GetY());
		return(GetID());
	}
	return(0);
}

BOOL C_MapMover::Process(long,short HitType)
{
	if(!Ready()) return(FALSE);

	switch(HitType)
	{
		case C_TYPE_LMOUSEDOWN:
			break;
		case C_TYPE_LMOUSEUP:
			SX_=0;
			SY_=0;
			Refresh();
			break;
		case C_TYPE_LMOUSEDBLCLK:
		case C_TYPE_RMOUSEDOWN:
		case C_TYPE_RMOUSEUP:
		case C_TYPE_RMOUSEDBLCLK:
			break;
	}
	gSoundMgr->PlaySound(GetSound(HitType));
	if(Callback_)
		(*Callback_)(GetID(),HitType,this);
	if(HitType == C_TYPE_LMOUSEUP)
		Draging_=0;
	return(TRUE);
}

void C_MapMover::Refresh()
{
	if(!Ready() || GetFlags() & C_BIT_INVISIBLE || Parent_ == NULL)
		return;

	Parent_->SetUpdateRect(GetX(),GetY(),GetX()+GetW()+1,GetY()+GetH()+1,GetFlags(),GetClient());
}

void C_MapMover::Draw(SCREEN *,UI95_RECT *)
{
	if(!Ready()) return;

	if(GetFlags() & C_BIT_INVISIBLE)
		return;

	if(DrawCallback_)
		(*DrawCallback_)(GetID(),C_TYPE_NOTHING,this);
}

BOOL C_MapMover::Drag(GRABBER *Drag,WORD MouseX,WORD MouseY,C_Window *)
{
	F4CSECTIONHANDLE* Leave;
	if(GetFlags() & C_BIT_INVISIBLE)
		return(FALSE);

	Leave=UI_Enter(Parent_);
	Draging_=1;
	SX_=Drag->ItemX_ + (MouseX - Drag->StartX_);
	SY_=Drag->ItemY_ + (MouseY - Drag->StartY_);

	Drag->StartX_=MouseX;
	Drag->StartY_=MouseY;

	if(Callback_){
		(*Callback_)(GetID(),C_TYPE_MOUSEMOVE,this);
	}
	UI_Leave(Leave);
	return(TRUE);
}

BOOL C_MapMover::Wheel(int increment, WORD MouseX, WORD MouseY){

	F4CSECTIONHANDLE *Leave=UI_Enter(Parent_);
	Increment_ = increment;
	if(Callback_){
		(*Callback_)(GetID(),C_TYPE_MOUSEWHEEL,this);
	}
	UI_Leave(Leave);

	return TRUE;
}

void C_MapMover::GetItemXY(long,long *x,long *y)
{
	*x=0;
	*y=0;
}
