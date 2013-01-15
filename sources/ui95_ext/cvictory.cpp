#include "stdafx.h"

C_Victory::C_Victory() : C_Control()
{
	_SetCType_(_CNTL_VICTORY_);
	Defaultflags_=C_BIT_ENABLED|C_BIT_REMOVE|C_BIT_MOUSEOVER|C_BIT_USEBGFILL;
	Font_=1;
	Section_=0;
	State_=0;
	TeamX_=0;
	ActionX_=0;
	TargetX_=0;
	ArgsX_=0;
	PointsX_=0;
	Color_[0]=0xeeeeee;
	Color_[1]=0x00ff00;
	Color_[2]=0xff0000;
	Number_=NULL;
	Team_=NULL;
	Action_=NULL;
	Target_=NULL;
	Args_=NULL;
	Points_=NULL;
	ptr_=NULL;
	Owner_=NULL;
}

C_Victory::C_Victory(char **stream) : C_Control(stream)
{
}

C_Victory::C_Victory(FILE *fp) : C_Control(fp)
{
}

C_Victory::~C_Victory()
{
}

long C_Victory::Size()
{
	return(0);
}

void C_Victory::Setup(long ID,short Type)
{
	SetID(ID);
	SetType(Type);
	SetDefaultFlags();
}

void C_Victory::Cleanup()
{
	if(Team_)
	{
		Team_->Cleanup();
		delete Team_;
		Team_=NULL;
	}
	if(Action_)
	{
		Action_->Cleanup();
		delete Action_;
		Action_=NULL;
	}
	if(Target_)
	{
		Target_->Cleanup();
		delete Target_;
		Target_=NULL;
	}
	if(Args_)
	{
		Args_->Cleanup();
		delete Args_;
		Args_=NULL;
	}
	if(Points_)
	{
		Points_->Cleanup();
		delete Points_;
		Points_=NULL;
	}
	SetReady(0);
}

void C_Victory::SetFont(long ID)
{
	if(Number_)
		Number_->SetFont(ID);
	if(Team_)
		Team_->SetFont(ID);
	if(Action_)
		Action_->SetFont(ID);
	if(Target_)
		Target_->SetFont(ID);
	if(Args_)
		Args_->SetFont(ID);
	if(Points_)
		Points_->SetFont(ID);
}

void C_Victory::SetState(short state)
{
	F4CSECTIONHANDLE *Leave;

	if(State_ > 1)
		return;

	Leave=UI_Enter(Parent_);
	State_=state;
	if(Number_)
		Number_->SetFGColor(Color_[State_]);
	if(Team_)
	{
		Team_->SetNormColor(Color_[State_]);
		Team_->SetSelColor(Color_[State_]);
	}
	if(Action_)
	{
		Action_->SetNormColor(Color_[State_]);
		Action_->SetSelColor(Color_[State_]);
	}
	if(Target_)
		Target_->SetFgColor(0,Color_[State_]);
	if(Args_)
	{
		Args_->SetNormColor(Color_[State_]);
		Args_->SetSelColor(Color_[State_]);
	}
	if(Points_)
		Points_->SetFgColor(Color_[State_]);
	Refresh();
	UI_Leave(Leave);
}

long C_Victory::CheckHotSpots(long relx,long rely)
{
	if(relx >= GetX() && rely >= GetY() && relx <= (GetX()+GetW()) && rely <= (GetY()+GetH()))
	{
		Section_=0;
		if(Team_)
			Section_=static_cast<short>(Team_->CheckHotSpots(relx,rely));
		if(Action_ && !Section_)
			Section_=static_cast<short>(Action_->CheckHotSpots(relx,rely));
		if(Target_ && !Section_)
			Section_=static_cast<short>(Target_->CheckHotSpots(relx,rely));
		if(Args_ && !Section_)
			Section_=static_cast<short>(Args_->CheckHotSpots(relx,rely));
		if(Points_ && !Section_)
			Section_=static_cast<short>(Points_->CheckHotSpots(relx,rely));

		SetRelXY(relx-GetX(),rely-GetY());
		return(GetID());
	}
	return(0);
}

void C_Victory::SetDefaultFlags()
{
	SetFlags(Defaultflags_);
}

long C_Victory::GetDefaultFlags()
{
	return(Defaultflags_);
}

BOOL C_Victory::Process(long ID,short HitType)
{
	if(Callback_)
		(*Callback_)(ID,HitType,this);
	if(!State_)
		SetState(1);
	switch(Section_)
	{
		case 1:
			if(Team_)
				return(Team_->Process(ID,HitType));
			break;
		case 2:
			if(Action_)
				return(Action_->Process(ID,HitType));
			break;
		case 3:
			if(Target_)
				return(Target_->Process(ID,HitType));
			break;
		case 4:
			if(Args_)
				return(Args_->Process(ID,HitType));
			break;
		case 5:
			if(Points_)
				return(Points_->Process(ID,HitType));
			break;
	}
	return(FALSE);
}

void C_Victory::Activate()
{
	if(Section_ == 5)
		Points_->Activate();
}

void C_Victory::Deactivate()
{
	Points_->Deactivate();
}

void C_Victory::SetX(long x)
{
	x_=x;
	if(Number_)
		Number_->SetX(GetX()+NumberX_);
	if(Team_)
		Team_->SetX(GetX()+TeamX_);
	if(Action_)
		Action_->SetX(GetX()+ActionX_);
	if(Target_)
		Target_->SetX(GetX()+TargetX_);
	if(Args_)
		Args_->SetX(GetX()+ArgsX_);
	if(Points_)
		Points_->SetX(GetX()+PointsX_);
}

void C_Victory::SetXY(long x,long y)
{
	x_=x;
	y_=y;
	if(Number_)
		Number_->SetXY(GetX()+NumberX_,y);
	if(Team_)
		Team_->SetXY(GetX()+TeamX_,y);
	if(Action_)
		Action_->SetXY(GetX()+ActionX_,y);
	if(Target_)
		Target_->SetXY(GetX()+TargetX_,y);
	if(Args_)
		Args_->SetXY(GetX()+ArgsX_,y);
	if(Points_)
		Points_->SetXY(GetX()+PointsX_,y);
}

void C_Victory::SetXYWH(long x,long y,long w,long h)
{
	x_=x;
	y_=y;
	w_=w;
	h_=h;
	if(Number_)
		Number_->SetXY(GetX()+NumberX_,y);
	if(Team_)
		Team_->SetXY(GetX()+TeamX_,y);
	if(Action_)
		Action_->SetXY(GetX()+ActionX_,y);
	if(Target_)
		Target_->SetXY(GetX()+TargetX_,y);
	if(Args_)
		Args_->SetXY(GetX()+ArgsX_,y);
	if(Points_)
		Points_->SetXY(GetX()+PointsX_,y);
}

void C_Victory::Refresh()
{
	if(!Ready() || Flags_ & C_BIT_INVISIBLE || Parent_ == NULL)
		return;

	Parent_->SetUpdateRect(GetX(),GetY(),GetX()+GetW(),GetY()+GetH(),Flags_,GetClient());
}

void C_Victory::Draw(SCREEN *surface,UI95_RECT *cliprect)
{
	if(!Ready() || GetFlags() & C_BIT_INVISIBLE || Parent_ == NULL)
		return;

	if(Number_)
		Number_->Draw(surface,cliprect);
	if(Team_)
		Team_->Draw(surface,cliprect);
	if(Action_)
		Action_->Draw(surface,cliprect);
	if(Target_)
		Target_->Draw(surface,cliprect);
	if(Args_)
		Args_->Draw(surface,cliprect);
	if(Points_)
		Points_->Draw(surface,cliprect);
	if(MouseOver_ || (GetFlags() & C_BIT_FORCEMOUSEOVER))
		HighLite(surface,cliprect);
}

BOOL C_Victory::CheckKeyboard(uchar DKScanCode,uchar Ascii,uchar ShiftStates,long RepeatCount)
{
	if(Section_ == 5)
		return(Points_->CheckKeyboard(DKScanCode,Ascii,ShiftStates,RepeatCount));
	return(FALSE);
}

void C_Victory::SetSubParents(C_Window *Parent)
{
	if(Number_)
	{
		Number_->SetFlagBitOn(GetFlags() & C_BIT_ABSOLUTE);
		Number_->SetClient(GetClient());
		Number_->SetParent(Parent);
		Number_->SetSubParents(Parent);
	}
	if(Team_)
	{
		Team_->SetFlagBitOn(GetFlags() & C_BIT_ABSOLUTE);
		Team_->SetClient(GetClient());
		Team_->SetParent(Parent);
		Team_->SetSubParents(Parent);
	}
	if(Action_)
	{
		Action_->SetFlagBitOn(GetFlags() & C_BIT_ABSOLUTE);
		Action_->SetClient(GetClient());
		Action_->SetParent(Parent);
		Action_->SetSubParents(Parent);
	}
	if(Target_)
	{
		Target_->SetFlagBitOn(GetFlags() & C_BIT_ABSOLUTE);
		Target_->SetClient(GetClient());
		Target_->SetParent(Parent);
		Target_->SetSubParents(Parent);
	}
	if(Args_)
	{
		Args_->SetFlagBitOn(GetFlags() & C_BIT_ABSOLUTE);
		Args_->SetClient(GetClient());
		Args_->SetParent(Parent);
		Args_->SetSubParents(Parent);
	}
	if(Points_)
	{
		Points_->SetFlagBitOn(GetFlags() & C_BIT_ABSOLUTE);
		Points_->SetClient(GetClient());
		Points_->SetParent(Parent);
		Points_->SetSubParents(Parent);
	}
}