#include "stdafx.h"

enum
{
	BID_DROPDOWN=50300,
	BID_SCROLLCAP_TOP_ON=50013,
	BID_SCROLLCAP_TOP_OFF=50014,
	BID_SCROLLCAP_BOTTOM_ON=50015,
	BID_SCROLLCAP_BOTTOM_OFF=50016,
};


C_ATO_Flight::C_ATO_Flight() : C_Control()
{
	_SetCType_(_CNTL_FLIGHT_);
	Section_=0;
	State_=0;
	TaskX_=0;
	TaskY_=0;
	IconBg_.left=0;
	IconBg_.top=0;
	IconBg_.right=0;
	IconBg_.bottom=0;
	FlightBg_.left=0;
	FlightBg_.top=0;
	FlightBg_.right=0;
	FlightBg_.bottom=0;

	IconBgColor_[0]=0;
	IconBgColor_[1]=0;
	FlightBgColor_[0]=0;
	FlightBgColor_[1]=0;

	vuID=FalconNullId;
	Icon_=NULL;
	Task_=NULL;
	Callsign_=NULL;
	Planes_=NULL;
	Airbase_=NULL;
	Status_=NULL;
	Defaultflags_=C_BIT_ENABLED|C_BIT_REMOVE|C_BIT_MOUSEOVER;
}

C_ATO_Flight::C_ATO_Flight(char **stream) : C_Control(stream)
{
}

C_ATO_Flight::C_ATO_Flight(FILE *fp) : C_Control(fp)
{
}

C_ATO_Flight::~C_ATO_Flight()
{
}

long C_ATO_Flight::Size()
{
	return(0);
}

void C_ATO_Flight::Setup(long ID,short Type)
{
	SetID(ID);
	SetType(Type);
	SetDefaultFlags();
}

void C_ATO_Flight::InitFlight(C_Handler *Handler)
{
	Icon_=new O_Output;
	Icon_->SetOwner(this);
	Icon_->SetFlags(Flags_|C_BIT_HCENTER|C_BIT_VCENTER);
	Callsign_=new O_Output;
	Callsign_->SetOwner(this);
	Callsign_->SetFlags(Flags_);
	Planes_=new O_Output;
	Planes_->SetOwner(this);
	Planes_->SetFlags(Flags_);
	Airbase_=new O_Output;
	Airbase_->SetOwner(this);
	Airbase_->SetFlags(Flags_);
	Status_=new O_Output;
	Status_->SetOwner(this);
	Status_->SetFlags(Flags_);

	Task_=new C_ListBox;
	Task_->Setup(GetID(),0,Handler);
	Task_->SetFlags(Flags_);
	Task_->SetWH(110,15);
//	Task_->AddScrollBar(BID_SCROLLCAP_TOP_OFF,BID_SCROLLCAP_TOP_ON,BID_SCROLLCAP_BOTTOM_OFF,BID_SCROLLCAP_BOTTOM_ON);
	Task_->SetDropDown(BID_DROPDOWN);
	Task_->SetNormColor(0xe0e0e0);
	Task_->SetSelColor(0xe0e0e0);
	Task_->SetDisColor(0xe0e0e0);
	Task_->SetParent(Parent_);
	Task_->SetClient(GetClient());

	SetReady(1);
}

void C_ATO_Flight::Cleanup()
{
	if(Icon_)
	{
		Icon_->Cleanup();
		delete Icon_;
		Icon_=NULL;
	}
	if(Callsign_)
	{
		Callsign_->Cleanup();
		delete Callsign_;
		Callsign_=NULL;
	}
	if(Task_)
	{
		Task_->Cleanup();
		delete Task_;
		Task_=NULL;
	}
	if(Planes_)
	{
		Planes_->Cleanup();
		delete Planes_;
		Planes_=NULL;
	}
	if(Airbase_)
	{
		Airbase_->Cleanup();
		delete Airbase_;
		Airbase_=NULL;
	}
	if(Status_)
	{
		Status_->Cleanup();
		delete Status_;
		Status_=NULL;
	}
}

void C_ATO_Flight::SetXY(long x,long y)
{
	x_=x;
	y_=y;

	if(Task_)
		Task_->SetXY(x+TaskX_,y+TaskY_);
}

void C_ATO_Flight::SetFont(long ID)
{
	if(Callsign_)
	{
		Callsign_->SetFont(ID);
		Callsign_->SetInfo();
	}
	if(Task_)
		Task_->SetFont(ID);
	if(Planes_)
	{
		Planes_->SetFont(ID);
		Planes_->SetInfo();
	}
	if(Airbase_)
	{
		Airbase_->SetFont(ID);
		Airbase_->SetInfo();
	}
	if(Status_)
	{
		Status_->SetFont(ID);
		Status_->SetInfo();
	}
}

void C_ATO_Flight::SetSubParents(C_Window *)
{
	if(Task_)
	{
		Task_->SetClient(GetClient());
		Task_->SetParent(Parent_);
		Task_->SetSubParents(Parent_);
	}
}

long C_ATO_Flight::CheckHotSpots(long relX,long relY)
{
	if(relX >= GetX() && relX <= (GetX()+GetW()) && relY >= GetY() && relY <= (GetY()+GetH()))
	{
		if(Task_->CheckHotSpots(relX,relY))
			Section_=1;
		else
			Section_=0;
		return(GetID());
	}
	return(0);
}

void C_ATO_Flight::SetDefaultFlags()
{
	SetFlags(Defaultflags_);
}

long C_ATO_Flight::GetDefaultFlags()
{
	return(Defaultflags_);
}

BOOL C_ATO_Flight::Process(long ID,short HitType)
{
	gSoundMgr->PlaySound(GetSound(HitType));
	if(Section_)
		Task_->Process(ID,HitType);
	if(Callback_)
		(*Callback_)(ID,HitType,this);

	return(FALSE);
}

void C_ATO_Flight::Refresh()
{
	if(!Ready() || Flags_ & C_BIT_INVISIBLE || Parent_ == NULL)
		return;

	Parent_->SetUpdateRect(GetX(),GetY(),GetX()+GetW(),GetY()+GetH(),Flags_,GetClient());
}

void C_ATO_Flight::Draw(SCREEN *surface,UI95_RECT *cliprect)
{
	if(!Ready() || GetFlags() & C_BIT_INVISIBLE || Parent_ == NULL)
		return;

	Parent_->BlitFill(surface,IconBgColor_[State_&1],GetX()+IconBg_.left,GetY()+IconBg_.top,IconBg_.right,IconBg_.bottom,Flags_,Client_,cliprect);
	if(State_)
		Parent_->BlitFill(surface,FlightBgColor_[State_&1],GetX()+FlightBg_.left,GetY()+FlightBg_.top,FlightBg_.right,FlightBg_.bottom,Flags_,Client_,cliprect);

	if(Icon_)
		Icon_->Draw(surface,cliprect);
	if(Callsign_)
		Callsign_->Draw(surface,cliprect);
	if(Task_)
		Task_->Draw(surface,cliprect);
	if(Planes_)
		Planes_->Draw(surface,cliprect);
	if(Airbase_)
		Airbase_->Draw(surface,cliprect);
	if(Status_)
		Status_->Draw(surface,cliprect);
	if(MouseOver_ || (GetFlags() & C_BIT_FORCEMOUSEOVER))
		HighLite(surface,cliprect);
}
