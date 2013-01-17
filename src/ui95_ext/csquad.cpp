#include "stdafx.h"

C_Squadron::C_Squadron() : C_Control()
{
	_SetCType_(_CNTL_SQUAD_);
	State_=0;

	BaseID_=0;

	IconBg_.left=0;
	IconBg_.top=0;
	IconBg_.right=0;
	IconBg_.bottom=0;
	InfoBg_.left=0;
	InfoBg_.top=0;
	InfoBg_.right=0;
	InfoBg_.bottom=0;

	NumVehicles_=0;
	NumPilots_=0;
	NumPlayers_=0;

	IconBgColor_[0]=0;
	IconBgColor_[1]=0;
	InfoBgColor_[0]=0;
	InfoBgColor_[1]=0;

	vuID=FalconNullId;
	Icon_=NULL;
	Name_=NULL;
	Planes_=NULL;
	Pilots_=NULL;
	Players_=NULL;
	Defaultflags_=C_BIT_ENABLED|C_BIT_REMOVE|C_BIT_MOUSEOVER;
}

C_Squadron::C_Squadron(char **stream) : C_Control(stream)
{
}

C_Squadron::C_Squadron(FILE *fp) : C_Control(fp)
{
}

C_Squadron::~C_Squadron()
{
}

long C_Squadron::Size()
{
	return(0);
}

void C_Squadron::Setup(long ID,short Type)
{
	SetID(ID);
	SetType(Type);
	SetDefaultFlags();
}

void C_Squadron::InitSquadron()
{
	Icon_=new O_Output;
	Icon_->SetOwner(this);
	Icon_->SetFlags(Flags_|C_BIT_HCENTER|C_BIT_VCENTER);
	Name_=new O_Output;
	Name_->SetOwner(this);
	Name_->SetFlags(Flags_);
	Planes_=new O_Output;
	Planes_->SetOwner(this);
	Planes_->SetFlags(Flags_);
	Pilots_=new O_Output;
	Pilots_->SetOwner(this);
	Pilots_->SetFlags(Flags_);
	Players_=new O_Output;
	Players_->SetOwner(this);
	Players_->SetFlags(Flags_);

	SetReady(1);
}

void C_Squadron::Cleanup()
{
	if(Icon_)
	{
		Icon_->Cleanup();
		delete Icon_;
		Icon_=NULL;
	}
	if(Name_)
	{
		Name_->Cleanup();
		delete Name_;
		Name_=NULL;
	}
	if(Planes_)
	{
		Planes_->Cleanup();
		delete Planes_;
		Planes_=NULL;
	}
	if(Pilots_)
	{
		Pilots_->Cleanup();
		delete Pilots_;
		Pilots_=NULL;
	}
	if(Players_)
	{
		Players_->Cleanup();
		delete Players_;
		Players_=NULL;
	}
}

void C_Squadron::SetFont(long ID)
{
	if(Name_)
	{
		Name_->SetFont(ID);
		Name_->SetInfo();
	}
	if(Planes_)
	{
		Planes_->SetFont(ID);
		Planes_->SetInfo();
	}
	if(Pilots_)
	{
		Pilots_->SetFont(ID);
		Pilots_->SetInfo();
	}
	if(Players_)
	{
		Players_->SetFont(ID);
		Players_->SetInfo();
	}
}

long C_Squadron::CheckHotSpots(long relX,long relY)
{
	if(relX >= GetX() && relX <= (GetX()+GetW()) && relY >= GetY() && relY <= (GetY()+GetH()))
		return(GetID());

	return(0);
}

void C_Squadron::SetDefaultFlags()
{
	SetFlags(Defaultflags_);
}

long C_Squadron::GetDefaultFlags()
{
	return(Defaultflags_);
}

BOOL C_Squadron::Process(long ID,short HitType)
{
	gSoundMgr->PlaySound(GetSound(HitType));
	if(Callback_)
		(*Callback_)(ID,HitType,this);
	return(FALSE);
}

void C_Squadron::Refresh()
{
	if(!Ready() || Flags_ & C_BIT_INVISIBLE || Parent_ == NULL)
		return;

	Parent_->SetUpdateRect(GetX(),GetY(),GetX()+GetW(),GetY()+GetH(),Flags_,GetClient());
}

void C_Squadron::Draw(SCREEN *surface,UI95_RECT *cliprect)
{
	if(!Ready() || GetFlags() & C_BIT_INVISIBLE || Parent_ == NULL)
		return;

	Parent_->BlitFill(surface,IconBgColor_[State_&1],GetX()+IconBg_.left,GetY()+IconBg_.top,IconBg_.right,IconBg_.bottom,Flags_,Client_,cliprect);
	if(State_)
		Parent_->BlitFill(surface,InfoBgColor_[State_&1],GetX()+InfoBg_.left,GetY()+InfoBg_.top,InfoBg_.right,InfoBg_.bottom,Flags_,Client_,cliprect);

	if(Icon_)
		Icon_->Draw(surface,cliprect);
	if(Name_)
		Name_->Draw(surface,cliprect);
	if(Planes_)
		Planes_->Draw(surface,cliprect);
	if(Pilots_)
		Pilots_->Draw(surface,cliprect);
	if(Players_)
		Players_->Draw(surface,cliprect);
	if(MouseOver_ || (GetFlags() & C_BIT_FORCEMOUSEOVER))
		HighLite(surface,cliprect);
}
