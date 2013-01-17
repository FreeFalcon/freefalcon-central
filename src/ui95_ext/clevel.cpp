#include "stdafx.h"

C_Level::C_Level() : C_Base()
{
	short i;

	DrawArea_.left=0;
	DrawArea_.top=0;
	DrawArea_.right=0;
	DrawArea_.bottom=0;
	MinValue_=0;
	MaxValue_=0;
	Start_=0;
	End_=0;
	Count_=0;

	for(i=0;i<_LEVEL_MAX_TEAMS_;i++)
	{
		Root_[i]=NULL;
		Color_[i]=0;
	}
}

C_Level::~C_Level()
{
	Cleanup();
}

long C_Level::Size()
{
	return(0);
}

void C_Level::Setup(long ID,short type)
{
	SetID(ID);
	SetType(type);
}

void C_Level::Cleanup()
{
	short i;
	LEVEL *cur,*last;

	for(i=0;i<_LEVEL_MAX_TEAMS_;i++)
	{
		cur=Root_[i];
		while(cur)
		{
			last=cur;
			cur=cur->Next;
			delete last;
		}
		Root_[i]=NULL;
	}
}

void C_Level::AddPoint(short team,short value)
{
	LEVEL *newval,*cur;

	if(team >= _LEVEL_MAX_TEAMS_)
		return;

	newval=new LEVEL;
	newval->value=value;
	newval->y=0;
	newval->Next=NULL;

	if(team == 1)
		Count_++;

	if(MinValue_ > value)
		MinValue_=value;
	if(MinValue_ < 0)
		MinValue_=0;
	if(MaxValue_ < value)
		MaxValue_=value;

	// KCK: Round Max value
	if (MaxValue_ > 5000)
		MaxValue_ = ((MaxValue_ + 999)/1000) * 1000;
	else if (MaxValue_ > 1000)
		MaxValue_ = ((MaxValue_ + 499)/500) * 500;
	else if (MaxValue_ > 500)
		MaxValue_ = ((MaxValue_ + 99)/100) * 100;
	else if (MaxValue_ > 100)
		MaxValue_ = ((MaxValue_ + 49)/50) * 50;
	else 
		MaxValue_ = ((MaxValue_ + 9)/10) * 10;

	if(!Root_[team])
	{
		Root_[team]=newval;
	}
	else
	{
		cur=Root_[team];
		while(cur->Next)
			cur=cur->Next;
		cur->Next=newval;
	}
	SetReady(1);
}

void C_Level::CalcPositions()
{
	short i;
	float yscale;
	LEVEL *cur;

	if(!Ready())
		return;

	if((MaxValue_-MinValue_) < 1)
		yscale=0;
	else
		yscale=(float)(DrawArea_.bottom-DrawArea_.top) / (float)(MaxValue_ - MinValue_) ;

	for(i=0;i<_LEVEL_MAX_TEAMS_;i++)
	{
		cur=Root_[i];
		while(cur)
		{
			// KCK: a value of -1 means don't draw
			if (cur->value < 0)
				cur->y = -1;
			else
				cur->y=static_cast<short>(DrawArea_.bottom - (short)((float)(cur->value - MinValue_) * yscale));
			
			cur=cur->Next;
		}
	}
}

void C_Level::Refresh()
{
	if(!Ready() || (GetFlags() & C_BIT_INVISIBLE) || !Parent_ || !Count_)
		return;

	Parent_->SetUpdateRect(DrawArea_.left,DrawArea_.top,DrawArea_.right,DrawArea_.bottom,GetFlags(),GetClient());
}

void C_Level::Draw(SCREEN *surface,UI95_RECT *cliprect)
{
	short i;
	LEVEL *cur,*prev;
	float levelx;
	float xstep;
	if(!Ready() || (GetFlags() & C_BIT_INVISIBLE) || !Parent_ || !Count_)
		return;

	if(Count_ < 2)
		xstep=static_cast<float>(DrawArea_.right - DrawArea_.left);
	else
		xstep=(float)(DrawArea_.right - DrawArea_.left) / (float)(Count_-1);

	for(i=0;i<_LEVEL_MAX_TEAMS_;i++)
	{
		// KCK: Don't draw teams with no colors.. It clobbers the teams we want to see
		// with black and looks like nothing's being drawn.
		if (Color_[i])
		{
			levelx=(float)DrawArea_.left;
			prev=Root_[i];
			if(prev->Next)
				cur=prev->Next;
			else
				cur=prev;
			while(cur)
			{
				// KCK: A level of -1 means this team isn't active for the time period we're drawing
				if (cur->y >= 0)
					Parent_->DrawLine(surface,Color_[i],FloatToInt32(levelx),prev->y,FloatToInt32(levelx+xstep),cur->y,GetFlags(),GetClient(),cliprect);
				levelx+=xstep;
				prev=cur;
				cur=cur->Next;
			}
		}
	}
}
