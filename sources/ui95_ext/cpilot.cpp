#include "stdafx.h"

C_Pilot::C_Pilot() : C_Control()
{
	_SetCType_(_CNTL_PILOT_);

	Font_=1;
	state_=0;
	skill_=0;
	slot_=0;
	isplayer_=0;
	Color_[0]=0x00ffffff;
	Color_[1]=0x0000ff00;
	Callsign_=NULL;
	vuID=FalconNullId;

	DefaultFlags_=C_BIT_ENABLED|C_BIT_REMOVE|C_BIT_MOUSEOVER;
}

C_Pilot::C_Pilot(char **stream) : C_Control(stream)
{
}

C_Pilot::C_Pilot(FILE *fp) : C_Control(fp)
{
}

C_Pilot::~C_Pilot()
{
	Cleanup();
}

long C_Pilot::Size()
{
	return(0);
}

void C_Pilot::Setup(long ID,short Type)
{
	SetID(ID);
	SetType(Type);
	SetDefaultFlags();
	SetReady(1);

	Callsign_=new O_Output;
	Callsign_->SetOwner(this);
	Callsign_->SetTextWidth(30);
}

void C_Pilot::Cleanup(void)
{
	if(Callsign_)
	{
		Callsign_->Cleanup();
		delete Callsign_;
		Callsign_=NULL;
	}
}

void C_Pilot::SetCallsign(short x,short y,_TCHAR *text)
{
	Callsign_->SetXY(x,y);
	Callsign_->SetText(text);
}

void C_Pilot::SetState(short state)
{
	state_=static_cast<short>(state & 1);
	Callsign_->SetFgColor(Color_[state_]);
	Refresh();
}

long C_Pilot::CheckHotSpots(long relx,long rely)
{
	if(GetFlags() & C_BIT_INVISIBLE || !(GetFlags() & C_BIT_ENABLED) || !Ready())
		return(0);

	if(relx >= GetX() && rely >= GetY() && relx <= (GetX() + GetW()) && rely <= GetY() + GetH())
		return(GetID());

	return(0);
}

BOOL C_Pilot::Process(long ID,short HitType)
{
	gSoundMgr->PlaySound(GetSound(HitType));

	if(Callback_)
		(*Callback_)(ID,HitType,this);

	return(FALSE);
}

void C_Pilot::Refresh()
{
	if(GetFlags() & C_BIT_INVISIBLE || Parent_ == NULL)
		return;

	Parent_->SetUpdateRect(GetX(),GetY(),GetX()+GetW(),GetY()+GetH(),GetFlags(),GetClient());
}

void C_Pilot::Draw(SCREEN *surface,UI95_RECT *cliprect)
{
	if(GetFlags() & C_BIT_INVISIBLE || Parent_ == NULL)
		return;
	if(Callsign_)
		Callsign_->Draw(surface,cliprect);
	if(MouseOver_ || (GetFlags() & C_BIT_FORCEMOUSEOVER))
		HighLite(surface,cliprect);
}

void C_Pilot::SetSubParents(C_Window *)
{
	if (Callsign_)
	{
		long w,h;

		Callsign_->SetFont(Font_);
		Callsign_->SetFgColor(Color_[0]);
		Callsign_->SetInfo();

		w=Callsign_->GetX() + Callsign_->GetW();
		h=Callsign_->GetY() + Callsign_->GetH();

		SetWH(w,h+2);
	}
}
