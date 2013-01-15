#include "stdafx.h"

C_Feature::C_Feature() : C_Control()
{
	_SetCType_(_CNTL_ENTITY_);
	State_=0;
	Name_=NULL;
	Status_=NULL;
	Value_=NULL;
	Operational_=0;
	vuID=FalconNullId;
	featureID_=0;
	Defaultflags_=C_BIT_ENABLED|C_BIT_REMOVE|C_BIT_MOUSEOVER;
}

C_Feature::C_Feature(char **stream) : C_Control(stream)
{
}

C_Feature::C_Feature(FILE *fp) : C_Control(fp)
{
}

C_Feature::~C_Feature()
{
}

long C_Feature::Size()
{
	return(0);
}

void C_Feature::Setup(long ID,short Type)
{
	SetID(ID);
	SetType(Type);
	SetDefaultFlags();
}

void C_Feature::InitEntity()
{
	Name_=new O_Output;
	Name_->SetOwner(this);
	Name_->SetFlags(Flags_);
	Name_->SetTextWidth(50);
	Status_=new O_Output;
	Status_->SetOwner(this);
	Status_->SetFlags(Flags_);
	Value_=new O_Output;
	Value_->SetOwner(this);
	Value_->SetFlags(Flags_);

	SetReady(1);
}

void C_Feature::Cleanup()
{
	if(Name_)
	{
		Name_->Cleanup();
		delete Name_;
		Name_=NULL;
	}
	if(Status_)
	{
		Status_->Cleanup();
		delete Status_;
		Status_=NULL;
	}
	if(Value_)
	{
		Value_->Cleanup();
		delete Value_;
		Value_=NULL;
	}
}

void C_Feature::SetState(short newstate)
{
	State_=static_cast<short>(newstate & 1);

	if(Name_)
	{
		Name_->SetFgColor(Color_[State_]);
		Name_->Refresh();
	}
	if(Status_)
	{
		Status_->SetFgColor(Color_[State_]);
		Status_->Refresh();
	}
	if(Value_)
	{
		Value_->SetFgColor(Color_[State_]);
		Value_->Refresh();
	}
}

void C_Feature::SetFont(long ID)
{
	if(Name_)
	{
		Name_->SetFont(ID);
		Name_->SetInfo();
	}
	if(Status_)
	{
		Status_->SetFont(ID);
		Status_->SetInfo();
	}
	if(Value_)
	{
		Value_->SetFont(ID);
		Value_->SetInfo();
	}
}

long C_Feature::CheckHotSpots(long relX,long relY)
{
	if(relX >= GetX() && relX <= (GetX()+GetW()) && relY >= GetY() && relY <= (GetY()+GetH()))
		return(GetID());

	return(0);
}

void C_Feature::SetDefaultFlags()
{
	SetFlags(Defaultflags_);
}

long C_Feature::GetDefaultFlags()
{
	return(Defaultflags_);
}

void C_Feature::SetSubParents(C_Window *)
{
	long w,h;

	w=0;
	h=0;
	if(Name_)
	{
		w=Name_->GetX() + Name_->GetW();
		h=Name_->GetY() + Name_->GetH();
	}
	if(Status_)
	{
		if((Status_->GetX() + Status_->GetW()) > w)
			w=Status_->GetX() + Status_->GetW();
		if((Status_->GetY() + Status_->GetH()) > w)
			h=Status_->GetY() + Status_->GetH();
	}
	if(Value_)
	{
		if((Value_->GetX() + Value_->GetW()) > w)
			w=Value_->GetX() + Value_->GetW();
		if((Value_->GetY() + Value_->GetH()) > w)
			h=Value_->GetY() + Value_->GetH();
	}
	SetWH(w,h);
}

BOOL C_Feature::Process(long ID,short HitType)
{
	gSoundMgr->PlaySound(GetSound(HitType));
	if(Callback_)
		(*Callback_)(ID,HitType,this);

	return(FALSE);
}

void C_Feature::Refresh()
{
	if(!Ready() || Flags_ & C_BIT_INVISIBLE || Parent_ == NULL)
		return;

	Parent_->SetUpdateRect(GetX(),GetY(),GetX()+GetW(),GetY()+GetH(),Flags_,GetClient());
}

void C_Feature::Draw(SCREEN *surface,UI95_RECT *cliprect)
{
	if(!Ready() || GetFlags() & C_BIT_INVISIBLE || Parent_ == NULL)
		return;

	if(Name_)
		Name_->Draw(surface,cliprect);
	if(Status_)
		Status_->Draw(surface,cliprect);
	if(Value_)
		Value_->Draw(surface,cliprect);
	if(MouseOver_ || (GetFlags() & C_BIT_FORCEMOUSEOVER))
		HighLite(surface,cliprect);
}
