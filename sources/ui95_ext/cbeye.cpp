#include "stdafx.h"
#include "campaign\include\team.h"
#include "campaign\include\package.h"
#include "ui\include\filters.h"

C_BullsEye::C_BullsEye() : C_Base()
{
	_SetCType_(_CNTL_BULLSEYE_);
	Color_=0;
	Scale_=1.0f;
	DefaultFlags_=C_BIT_ENABLED;
	WorldX_ = WorldY_ = 0;
}

C_BullsEye::C_BullsEye(char **stream) : C_Base(stream)
{
}

C_BullsEye::C_BullsEye(FILE *fp) : C_Base(fp)
{
}

C_BullsEye::~C_BullsEye()
{
}

long C_BullsEye::Size()
{
	return(0);
}

void C_BullsEye::Setup(long ID,short Type)
{
	SetID(ID);
	SetType(Type);
	SetDefaultFlags();
	SetReady(1);
}

void C_BullsEye::Cleanup(void)
{
}

void C_BullsEye::SetPos(float x,float y)
{
	WorldX_=x;
	WorldY_=y;

	SetXY(static_cast<long>(WorldX_ * Scale_),static_cast<long>(WorldY_ * Scale_));//! 
}

void C_BullsEye::SetScale(float scl)
{
	Scale_=scl;
	SetXY(static_cast<long>(WorldX_ * Scale_),static_cast<long>(WorldY_ * Scale_));
	SetWH((short)(BullsEyeLines[0][1] * Scale_)*2,(short)(BullsEyeLines[0][1] * Scale_)*2);
}

void C_BullsEye::Refresh()
{
	if(GetFlags() & C_BIT_INVISIBLE || Parent_ == NULL)
		return;

	Parent_->SetUpdateRect(GetX()-GetW()/2,GetY()-GetH()/2,GetX()+GetW()/2+1,GetY()+GetH()/2+1,GetFlags(),GetClient());
}

void C_BullsEye::Draw(SCREEN *surface,UI95_RECT *cliprect)
{
	long i;
	long x1,y1,x2,y2;

	if(GetFlags() & C_BIT_INVISIBLE || Parent_ == NULL)
		return;

	for(i=0;i<NUM_BE_CIRCLES;i++)
	{
		Parent_->DrawCircle(surface,Color_,GetX(),GetY(),BullsEyeRadius[i]*Scale_,GetFlags(),GetClient(),cliprect);
	}

	for(i=0;i<NUM_BE_LINES;i++)
	{
		x1=GetX() + (short)(BullsEyeLines[i][0] * Scale_);
		y1=GetY() + (short)(BullsEyeLines[i][1] * Scale_);
		x2=GetX() + (short)(BullsEyeLines[i][2] * Scale_);
		y2=GetY() + (short)(BullsEyeLines[i][3] * Scale_);
		Parent_->DrawLine(surface,Color_,x1,y1,x2,y2,GetFlags(),GetClient(),cliprect);
	}
}
