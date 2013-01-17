#include "stdafx.h"

C_Dog_Flight::C_Dog_Flight() : C_Control()
{
	_SetCType_(_CNTL_DOG_FLIGHT_);

	Font_=1;
	Icon_=NULL;
	Callsign_=NULL;
	Aircraft_=NULL;
	state_=0;
	Color_[0]=0x00ffffff;
	Color_[1]=0x0000ff00;
	Image_[0]=NULL;
	Image_[1]=NULL;
	vuID=FalconNullId;

	DefaultFlags_=C_BIT_ENABLED|C_BIT_REMOVE|C_BIT_MOUSEOVER;
}

C_Dog_Flight::C_Dog_Flight(char **stream) : C_Control(stream)
{
}

C_Dog_Flight::C_Dog_Flight(FILE *fp) : C_Control(fp)
{
}

C_Dog_Flight::~C_Dog_Flight()
{
	Cleanup();
}

long C_Dog_Flight::Size()
{
	return(0);
}

void C_Dog_Flight::Setup(long ID,short Type)
{
	SetID(ID);
	SetType(Type);
	SetDefaultFlags();
	SetReady(1);

	Icon_=new O_Output;
	Icon_->SetOwner(this);
	Callsign_=new O_Output;
	Callsign_->SetOwner(this);
	Callsign_->SetTextWidth(30);
	Aircraft_=new O_Output;
	Aircraft_->SetOwner(this);
	Aircraft_->SetTextWidth(30);
}

void C_Dog_Flight::Cleanup(void)
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
	if(Aircraft_)
	{
		Aircraft_->Cleanup();
		delete Aircraft_;
		Aircraft_=NULL;
	}
}

void C_Dog_Flight::SetIcon(short x,short y,IMAGE_RSC *dark,IMAGE_RSC *lite)
{
	Image_[0]=dark;
	Image_[1]=lite;
	Icon_->SetXY(x,y);
	Icon_->SetImage(dark);
}

void C_Dog_Flight::SetCallsign(short x,short y,_TCHAR *text)
{
	Callsign_->SetXY(x,y);
	Callsign_->SetText(text);
}

void C_Dog_Flight::SetAircraft(short x,short y,_TCHAR *text)
{
	Aircraft_->SetXY(x,y);
	Aircraft_->SetText(text);
}

void C_Dog_Flight::SetState(short state)
{
	state_=static_cast<short>(state & 1);
	Icon_->SetImage(Image_[state_]);
	Callsign_->SetFgColor(Color_[state_]);
	Aircraft_->SetFgColor(Color_[state_]);
	Refresh();
}

long C_Dog_Flight::CheckHotSpots(long relx,long rely)
{
	if(GetFlags() & C_BIT_INVISIBLE || !(GetFlags() & C_BIT_ENABLED) || !Ready())
		return(0);

	if(relx >= GetX() && rely >= GetY() && relx <= (GetX() + GetW()) && rely <= GetY() + GetH())
		return(GetID());

	return(0);
}

BOOL C_Dog_Flight::Process(long ID,short HitType)
{
	gSoundMgr->PlaySound(GetSound(HitType));

	if(Callback_)
		(*Callback_)(ID,HitType,this);

	return(FALSE);
}

void C_Dog_Flight::Refresh()
{
	if(GetFlags() & C_BIT_INVISIBLE || Parent_ == NULL)
		return;

	Parent_->SetUpdateRect(GetX(),GetY(),GetX()+GetW(),GetY()+GetH(),GetFlags(),GetClient());
}

void C_Dog_Flight::Draw(SCREEN *surface,UI95_RECT *cliprect)
{
	if(GetFlags() & C_BIT_INVISIBLE || Parent_ == NULL)
		return;
	if(Icon_)
		Icon_->Draw(surface,cliprect);
	if(Callsign_)
		Callsign_->Draw(surface,cliprect);
	if(Aircraft_)
		Aircraft_->Draw(surface,cliprect);
	if(MouseOver_ || (GetFlags() & C_BIT_FORCEMOUSEOVER))
		HighLite(surface,cliprect);
}

void C_Dog_Flight::SetSubParents(C_Window *)
{
	long w,h;

	if(!Icon_ || !Callsign_ || !Aircraft_)
		return;

	Callsign_->SetFont(Font_);
	Callsign_->SetFgColor(Color_[0]);
	Callsign_->SetInfo();
	Aircraft_->SetFont(Font_);
	Aircraft_->SetFgColor(Color_[0]);
	Aircraft_->SetInfo();

	w=Icon_->GetX() + Icon_->GetW();
	h=Icon_->GetY() + Icon_->GetH();

	if((Callsign_->GetX() + Callsign_->GetW()) > w)
		w=Callsign_->GetX() + Callsign_->GetW();
	if((Callsign_->GetY() + Callsign_->GetH()) > h)
		h=Callsign_->GetY() + Callsign_->GetH();

	if((Aircraft_->GetX() + Aircraft_->GetW()) > w)
		w=Aircraft_->GetX() + Aircraft_->GetW();
	if((Aircraft_->GetY() + Aircraft_->GetH()) > h)
		h=Aircraft_->GetY() + Aircraft_->GetH();

	SetWH(w,h+2);
}
