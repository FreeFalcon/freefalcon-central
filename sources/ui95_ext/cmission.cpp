#include "stdafx.h"

C_Mission::C_Mission() : C_Control()
{
	_SetCType_(_CNTL_MISSION_);

	State_=0;
	TakeOffTime_=0;
	MissionID_=0;
	PackageID_=0;
	StatusID_=0;
	PriorityID_=0;
	Font_ = 0; // JPO

	Color_[0]=0xe0e0e0; // Not Selected
	Color_[1]=0x00ff00; // Selected
	Color_[2]=0xffff00; // Player is in this mission
	Color_[3]=0x00ff00; // Player is in this mission & current mission

	TakeOff_=NULL;
	Mission_=NULL;
	Package_=NULL;
	Status_=NULL;
	Priority_=NULL;

	vuID=FalconNullId;
	Owner_=NULL;

	DefaultFlags_=C_BIT_ENABLED|C_BIT_REMOVE|C_BIT_MOUSEOVER;
}

C_Mission::C_Mission(char **stream) : C_Control(stream)
{
}

C_Mission::C_Mission(FILE *fp) : C_Control(fp)
{
}

C_Mission::~C_Mission()
{
}

long C_Mission::Size()
{
	return(0);
}

void C_Mission::Setup(long ID,short Type)
{
	SetID(ID);
	SetType(Type);
	SetDefaultFlags();
	SetReady(1);

	TakeOff_=new O_Output;
	TakeOff_->SetOwner(this);
	TakeOff_->SetFont(Font_);
	TakeOff_->SetTextWidth(10);
	Mission_=new O_Output;
	Mission_->SetOwner(this);
	Mission_->SetFont(Font_);
	Package_=new O_Output;
	Package_->SetOwner(this);
	Package_->SetFont(Font_);
	Package_->SetTextWidth(6);
	Status_=new O_Output;
	Status_->SetOwner(this);
	Status_->SetFont(Font_);
	Priority_=new O_Output;
	Priority_->SetOwner(this);
	Priority_->SetFont(Font_);
	Priority_->SetTextWidth(4);
}

void C_Mission::Cleanup(void)
{
	if(TakeOff_)
	{
		TakeOff_->Cleanup();
		delete TakeOff_;
		TakeOff_=NULL;
	}
	if(Mission_)
	{
		Mission_->Cleanup();
		delete Mission_;
		Mission_=NULL;
	}
	if(Package_)
	{
		Package_->Cleanup();
		delete Package_;
		Package_=NULL;
	}
	if(Status_)
	{
		Status_->Cleanup();
		delete Status_;
		Status_=NULL;
	}
	if(Priority_)
	{
		Priority_->Cleanup();
		delete Priority_;
		Priority_=NULL;
	}
}

void C_Mission::SetFont(long)
{
	if(TakeOff_)
		TakeOff_->SetFont(Font_);
	if(Mission_)
		Mission_->SetFont(Font_);
	if(Package_)
		Package_->SetFont(Font_);
	if(Status_)
		Status_->SetFont(Font_);
	if(Priority_)
		Priority_->SetFont(Font_);
}

void C_Mission::SetTakeOff(short x,short y,_TCHAR *txt)
{
	if(TakeOff_)
	{
		TakeOff_->SetXY(x,y);
		TakeOff_->SetText(txt);
	}
}

void C_Mission::SetMission(short x,short y,_TCHAR *txt)
{ 
	if(Mission_)  
	{ 
		Mission_->SetXY(x,y); 
		Mission_->SetText(gStringMgr->GetText(gStringMgr->AddText(txt)));
	} 
}

void C_Mission::SetPackage(short x,short y,_TCHAR *txt)
{ 
	if(Package_)  
	{ 
		Package_->SetXY(x,y); 
		Package_->SetText(txt); 
	} 
}
void C_Mission::SetStatus(short x,short y,_TCHAR *txt)
{
	if(Status_)
	{
		Status_->SetXY(x,y); 
		Status_->SetText(gStringMgr->GetText(gStringMgr->AddText(txt))); 
	}
}
void C_Mission::SetPriority(short x,short y,_TCHAR *txt)
{
	if(Priority_)
	{
		Priority_->SetXY(x,y); 
		Priority_->SetText(txt); 
	}
}

void C_Mission::SetTakeOff(_TCHAR *txt)  
{ 
	if(TakeOff_)  
		TakeOff_->SetText(txt); 
}

void C_Mission::SetMission(_TCHAR *txt)  
{ 
	if(Mission_)  
		Mission_->SetText(gStringMgr->GetText(gStringMgr->AddText(txt))); 
}

void C_Mission::SetPackage(_TCHAR *txt)  
{ 
	if(Package_)  
		Package_->SetText(txt); 
}

void C_Mission::SetStatus(_TCHAR *txt)   
{ 
	if(Status_)   
		Status_->SetText(gStringMgr->GetText(gStringMgr->AddText(txt))); 
}

void C_Mission::SetPriority(_TCHAR *txt)   
{ 
	if(Priority_)   
		Priority_->SetText(txt); 
}

long C_Mission::CheckHotSpots(long relx,long rely)
{
	if(GetFlags() & C_BIT_INVISIBLE || !(GetFlags() & C_BIT_ENABLED) || !Ready())
		return(0);

	if(relx >= GetX() && rely >= GetY() && relx <= (GetX()+GetW()) && rely <= (GetY()+GetH()))
		return(GetID());
	return(0);
}

BOOL C_Mission::Process(long ID,short HitType)
{
	gSoundMgr->PlaySound(GetSound(HitType));
	if(HitType == C_TYPE_LMOUSEUP)
	{
		SetState(static_cast<short>(GetState() | 1));
		Refresh();
	}

	if(Callback_)
		(*Callback_)(ID,HitType,this);

	return(FALSE);
}


void C_Mission::Refresh()
{
	if(GetFlags() & C_BIT_INVISIBLE || Parent_ == NULL)
		return;

	Parent_->SetUpdateRect(GetX(),GetY(),GetX()+GetW(),GetY()+GetH(),GetFlags(),GetClient());
}

void C_Mission::Draw(SCREEN *surface,UI95_RECT *cliprect)
{
	short i;
	if(GetFlags() & C_BIT_INVISIBLE || Parent_ == NULL || !Ready())
		return;

	i=GetState();

	if(TakeOff_)
	{
		TakeOff_->SetFgColor(Color_[i]);
		TakeOff_->Draw(surface,cliprect);
	}
	if(Mission_)
	{
		Mission_->SetFgColor(Color_[i]);
		Mission_->Draw(surface,cliprect);
	}
	if(Package_)
	{
		Package_->SetFgColor(Color_[i]);
		Package_->Draw(surface,cliprect);
	}
	if(Status_)
	{
		Status_->SetFgColor(Color_[i]);
		Status_->Draw(surface,cliprect);
	}
	if(Priority_)
	{
		Priority_->SetFgColor(Color_[i]);
		Priority_->Draw(surface,cliprect);
	}
	if(MouseOver_ || (GetFlags() & C_BIT_FORCEMOUSEOVER))
		HighLite(surface,cliprect);
}
