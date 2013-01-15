#include "stdafx.h"

enum
{
	BID_DROPDOWN=50300,
	BID_SCROLLCAP_TOP_ON=50013,
	BID_SCROLLCAP_TOP_OFF=50014,
	BID_SCROLLCAP_BOTTOM_ON=50015,
	BID_SCROLLCAP_BOTTOM_OFF=50016,
};


C_ATO_Package::C_ATO_Package() : C_Control()
{
	_SetCType_(_CNTL_PACKAGE_);
	Section_=0;
	State_=0;
	WPState_=0;

	Color_[0]=0xeeeeee;
	Color_[1]=0xffff00;

	vuID=FalconNullId;
	Title_=NULL;
	ShowWP_=NULL;
	Defaultflags_=C_BIT_ENABLED|C_BIT_REMOVE|C_BIT_MOUSEOVER;
}

C_ATO_Package::C_ATO_Package(char **stream) : C_Control(stream)
{
}

C_ATO_Package::C_ATO_Package(FILE *fp) : C_Control(fp)
{
}

C_ATO_Package::~C_ATO_Package()
{
}

long C_ATO_Package::Size()
{
	return(0);
}

void C_ATO_Package::Setup(long ID,short Type)
{
	SetID(ID);
	SetType(Type);
	SetDefaultFlags();
}

void C_ATO_Package::InitPackage(C_Handler *)
{
	Title_=new O_Output;
	Title_->SetOwner(this);
	Title_->SetFlags(Flags_);
	ShowWP_=new O_Output;
	ShowWP_->SetOwner(this);
	ShowWP_->SetFlags(Flags_);
	SetReady(1);
}

void C_ATO_Package::Cleanup()
{
	if(Title_)
	{
		Title_->Cleanup();
		delete Title_;
		Title_=NULL;
	}
	if(ShowWP_)
	{
		ShowWP_->Cleanup();
		delete ShowWP_;
		ShowWP_=NULL;
	}
}

void C_ATO_Package::SetCheckBox(short x,short y,long off,long on)
{
	Image_[0]=gImageMgr->GetImage(off);
	Image_[1]=gImageMgr->GetImage(on);
	ShowWP_->SetXY(x,y);
	ShowWP_->SetImage(Image_[0]);
	ShowWP_->SetInfo();
}

void C_ATO_Package::SetState(short state)
{
	State_=static_cast<short>(state & 1);

	Title_->SetFgColor(Color_[State_]);
}

void C_ATO_Package::SetWPState(short state)
{
	WPState_=static_cast<short>(state & 1);

	ShowWP_->SetImage(Image_[WPState_]);
	ShowWP_->Refresh();
}

void C_ATO_Package::SetFont(long ID)
{
	if(Title_)
	{
		Title_->SetFont(ID);
		Title_->SetInfo();
	}
}

long C_ATO_Package::CheckHotSpots(long relX,long relY)
{
	if(relX >= GetX() && relX <= (GetX()+GetW()) && relY >= GetY() && relY <= (GetY()+GetH()))
	{
		if(relX >= (GetX()+ShowWP_->GetX()) && relX <= (GetX()+ShowWP_->GetX()+ShowWP_->GetW())
			&& relY >= (GetY()+ShowWP_->GetY()) && relY <= (GetY()+ShowWP_->GetY()+ShowWP_->GetH()))
			Section_=1;
		else
			Section_=0;
		return(GetID());
	}
	return(0);
}

void C_ATO_Package::SetDefaultFlags()
{
	SetFlags(Defaultflags_);
}

long C_ATO_Package::GetDefaultFlags()
{
	return(Defaultflags_);
}

BOOL C_ATO_Package::Process(long ID,short HitType)
{
	gSoundMgr->PlaySound(GetSound(HitType));
	if(HitType == C_TYPE_LMOUSEUP)
	{
		if(Section_)
			SetWPState(static_cast<short>(WPState_ ^ 1));
	}
	if(Callback_)
		(*Callback_)(ID,HitType,this);
	return(FALSE);
}

void C_ATO_Package::Refresh()
{
	if(!Ready() || Flags_ & C_BIT_INVISIBLE || Parent_ == NULL)
		return;

	Parent_->SetUpdateRect(GetX(),GetY(),GetX()+GetW(),GetY()+GetH(),Flags_,GetClient());
}

void C_ATO_Package::Draw(SCREEN *surface,UI95_RECT *cliprect)
{
	if(!Ready() || GetFlags() & C_BIT_INVISIBLE || Parent_ == NULL)
		return;

	if(Title_)
		Title_->Draw(surface,cliprect);
	if(ShowWP_)
		ShowWP_->Draw(surface,cliprect);
	if(MouseOver_ || (GetFlags() & C_BIT_FORCEMOUSEOVER))
		HighLite(surface,cliprect);
}
