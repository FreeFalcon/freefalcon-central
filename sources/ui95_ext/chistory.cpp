#include "stdafx.h"

C_History::C_History() : C_Base()
{
	short i;
	Data_=NULL;
	Count_=0;

	for(i=0;i<_HIST_MAX_TEAMS_;i++)
		ImageID_[i]=0;
}

C_History::C_History(char **stream) : C_Base(stream)
{
}

C_History::C_History(FILE *fp) : C_Base(fp)
{
}

C_History::~C_History()
{
	if(Data_)
		Cleanup();
}

long C_History::Size()
{
	return(0);
}

void C_History::Setup(long ID,short type,short count)
{
	SetID(ID);
	SetType(type);
	SetXY(0,0);

	Count_=count;
	Data_=new O_Output[Count_];
}

void C_History::Cleanup()
{
	Count_=0;
	if(Data_)
	{
		delete Data_;
		Data_=NULL;
	}
}

void C_History::AddIconSet(short idx,short team,short x,short y)
{
	if(idx < Count_)
	{
		Data_[idx].SetOwner(this);
		Data_[idx].SetImage(ImageID_[team]);
		Data_[idx].SetXY(x,y);
		Data_[idx].SetFlags(C_BIT_HCENTER|C_BIT_VCENTER);
		Data_[idx].SetInfo();
	}
}

void C_History::Refresh()
{
	if(!Ready() || (GetFlags() & C_BIT_INVISIBLE) || !Parent_)
		return;

	Parent_->RefreshClient(GetClient());
}

void C_History::Draw(SCREEN *surface,UI95_RECT *cliprect)
{
	short i;
	if(!Ready() || (GetFlags() & C_BIT_INVISIBLE) || !Parent_)
		return;

	for(i=0;i<Count_;i++)
		Data_[i].Draw(surface,cliprect);
}
