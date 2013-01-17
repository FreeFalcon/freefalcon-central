#include "stdafx.h"

C_Entity::C_Entity() : C_Control()
{
	_SetCType_(_CNTL_ENTITY_);
	State_=0;
	IconBg_.left=0;
	IconBg_.top=0;
	IconBg_.right=0;
	IconBg_.bottom=0;
	InfoBg_.left=0;
	InfoBg_.top=0;
	InfoBg_.right=0;
	InfoBg_.bottom=0;

	IconBgColor_[0]=0;
	IconBgColor_[1]=0;
	InfoBgColor_[0]=0;
	InfoBgColor_[1]=0;

	Icon_=NULL;
	Name_=NULL;
	Status_=NULL;
	Operational_=0;
	vuID=FalconNullId;
	Defaultflags_=C_BIT_ENABLED|C_BIT_REMOVE|C_BIT_MOUSEOVER|C_BIT_USEBGFILL;
}

C_Entity::C_Entity(char **stream) : C_Control(stream)
{
}

C_Entity::C_Entity(FILE *fp) : C_Control(fp)
{
}

C_Entity::~C_Entity()
{
}

long C_Entity::Size()
{
	return(0);
}

void C_Entity::Setup(long ID,short Type)
{
	SetID(ID);
	SetType(Type);
	SetDefaultFlags();
}

void C_Entity::InitEntity()
{
	Icon_=new O_Output;
	Icon_->SetOwner(this);
	Icon_->SetFlags(Flags_|C_BIT_HCENTER|C_BIT_VCENTER);
	Name_=new O_Output;
	Name_->SetOwner(this);
	Name_->SetFlags(Flags_);
	Status_=new O_Output;
	Status_->SetOwner(this);
	Status_->SetFlags(Flags_);

	SetReady(1);
}

void C_Entity::Cleanup()
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
	if(Status_)
	{
		Status_->Cleanup();
		delete Status_;
		Status_=NULL;
	}
}

void C_Entity::SetFont(long ID)
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
}

long C_Entity::CheckHotSpots(long relX,long relY)
{
	if(relX >= GetX() && relX <= (GetX()+GetW()) && relY >= GetY() && relY <= (GetY()+GetH()))
		return(GetID());

	return(0);
}

void C_Entity::SetDefaultFlags()
{
	SetFlags(Defaultflags_);
}

long C_Entity::GetDefaultFlags()
{
	return(Defaultflags_);
}

BOOL C_Entity::Process(long ID,short HitType)
{
	gSoundMgr->PlaySound(GetSound(HitType));
	if(Callback_)
		(*Callback_)(ID,HitType,this);

	return(FALSE);
}

void C_Entity::Refresh()
{
	if(!Ready() || Flags_ & C_BIT_INVISIBLE || Parent_ == NULL)
		return;

	Parent_->SetUpdateRect(GetX(),GetY(),GetX()+GetW(),GetY()+GetH(),Flags_,GetClient());
}

void C_Entity::Draw(SCREEN *surface,UI95_RECT *cliprect)
{
	if(!Ready() || GetFlags() & C_BIT_INVISIBLE || Parent_ == NULL)
		return;

	if(GetFlags() & C_BIT_USEBGFILL)
	{
		Parent_->BlitFill(surface,IconBgColor_[State_&1],GetX()+IconBg_.left,GetY()+IconBg_.top,IconBg_.right,IconBg_.bottom,Flags_,Client_,cliprect);
		if(State_)
			Parent_->BlitFill(surface,InfoBgColor_[State_&1],GetX()+InfoBg_.left,GetY()+InfoBg_.top,InfoBg_.right,InfoBg_.bottom,Flags_,Client_,cliprect);
	}

	if(Icon_)
		Icon_->Draw(surface,cliprect);
	if(Name_)
		Name_->Draw(surface,cliprect);
	if(Status_)
		Status_->Draw(surface,cliprect);
	if(MouseOver_ || (GetFlags() & C_BIT_FORCEMOUSEOVER))
		HighLite(surface,cliprect);
}
