#include "stdafx.h"
#include <windows.h>
#include "chandler.h"
#include "ui95_ext.h"

/* 2001-05-10 S.G. */#include "..\CAMPAIGN\INCLUDE\CampTerr.h"
/* 2001-05-10 S.G. */#include "..\CAMPAIGN\INCLUDE\Division.h"
/* 2001-05-10 S.G. */#include "..\CAMPAIGN\INCLUDE\Package.h"
/* 2001-05-10 S.G. */#include "..\CAMPAIGN\INCLUDE\Team.h"
/* 2001-05-10 S.G. */#include "..\UI\Include\cmap.h"
/* 2001-05-10 S.G. */#include "..\UI\Include\gps.h"
/* 2001-05-10 S.G. */#include "..\UI\Include\urefresh.h"
/* 2001-05-10 S.G. */extern GlobalPositioningSystem *gGps;

Circle C_Threat::myCircle;

C_Threat::C_Threat() : C_Base()
{
	Root_=NULL;
}

C_Threat::~C_Threat()
{
	if(Root_)
		Cleanup();
}

void C_Threat::Setup(long ID,long type)
{
	SetID(ID);
	SetType(static_cast<short>(type));
	SetReady(1);

	Root_=new C_Hash;
	Root_->Setup(10);
	Root_->SetFlags(C_BIT_REMOVE);
}

void C_Threat::Cleanup()
{
	if(Root_)
	{
		Root_->Cleanup();
		delete Root_;
		Root_=NULL;
	}
}

void C_Threat::AddCircle(long ID,long type,long worldx,long worldy,long radius)
{
	THREAT_CIRCLE *circle;
	long i;

	if(!Root_ || ID < 1 || !radius)
		return;

	if(Root_->Find(ID))
		return;

	circle=new THREAT_CIRCLE;
	circle->ID=ID;
	circle->Type=type;
	circle->Flags=0;
	for(i=0;i<8;i++)
		circle->Radius[i]=radius;
	circle->x=worldx;
	circle->y=worldy;
	circle->Owner=this;

	Root_->Add(ID,circle);
}

void C_Threat::UpdateCircle(long ID,long worldx,long worldy)
{
	THREAT_CIRCLE *circle;

	if(!Root_ || ID < 1)
		return;

	circle=(THREAT_CIRCLE*)Root_->Find(ID);
	if(circle)
	{
		circle->x=worldx;
		circle->y=worldy;
	}
}

void C_Threat::Remove(long ID)
{
	if(!Root_ || ID < 1)
		return;

	Root_->Remove(ID);
}

void C_Threat::SetRadius(long ID,long slice,long radius)
{
	THREAT_CIRCLE *circle;

	if(slice < 0 || slice >= 8)
		return;

	circle=(THREAT_CIRCLE*)Root_->Find(ID);
	if(circle)
		circle->Radius[slice]=radius;
}

void C_Threat::BuildOverlay(BYTE *overlay,long w,long h,float pixelsperkm)
{
	THREAT_CIRCLE *circle;
	C_HASHNODE *cur;
	long curidx;
	long i;

	if(Flags_ & C_BIT_INVISIBLE || !Root_ || !overlay)
		return;

	myCircle.SetBuffer ((char*)overlay);
	myCircle.SetDimension (w, h);

	circle=(THREAT_CIRCLE*)Root_->GetFirst(&cur,&curidx);
	while(circle) {
// 2001-05-10 MODIFIED BY S.G. NEED TO LOOK UP THE MAP ITEM FLAG TO SEE IF IT CAN BE DISPLAYED OR NOT...
//		if(!(circle->Flags & C_BIT_INVISIBLE)) {
		UI_Refresher *gpsItem=NULL;
		if(!(circle->Flags & C_BIT_INVISIBLE) && ((gpsItem=(UI_Refresher*)gGps->Find(circle->ID)) && gpsItem->MapItem_ && !(gpsItem->MapItem_->Flags & C_BIT_INVISIBLE))) {
			myCircle.SetCenter(static_cast<long>(static_cast<float>(circle->x) * pixelsperkm), 
							   static_cast<long>(static_cast<float>(circle->y) * pixelsperkm));
			if(circle->Type == THR_CIRCLE) {
				if(circle->Radius[0] > 3)
				{
					myCircle.SetRadius(static_cast<long>(static_cast<float>(circle->Radius[0]) * pixelsperkm));
					myCircle.CreateFilledCircle ();
				}
			}
			else if(circle->Type == THR_SLICE) {
				for (i=0; i < 8; i++) {
					if (circle->Radius[i] > 3) {
						myCircle.SetRadius(static_cast<long>(static_cast<float>(circle->Radius[i]) * pixelsperkm));
						myCircle.CreateFilledArc (i);
					}
				}
			}
		}
		circle=(THREAT_CIRCLE*)Root_->GetNext(&cur,&curidx);
	}
}

CircleEdge Circle::Edge[MAXCIRCLESIZE];
long Circle::MaxHeight;
long Circle::MaxWidth;
long Circle::MaxHeight1;
long Circle::MaxWidth1;
long Circle::CenterX;
long Circle::CenterY;
long Circle::Radius;
long Circle::Diagonal;
long Circle::CircleTop;
long Circle::CircleSize;
int	Circle::CircleTopAddress;
char *Circle::CircleBuffer;

void Circle::InitBuffer ()
{
	CircleTop = CenterY - Radius;
	if (CircleTop < 0) CircleTop = 0;
	else if (CircleTop > MaxHeight1) CircleTop = MaxHeight1;
	CircleSize = CenterY + Radius;
	if (CircleSize < 0) CircleSize = 0;
	else if (CircleSize > MaxHeight1) CircleSize = MaxHeight1;
	CircleSize = CircleSize - CircleTop + 1;
	CircleTopAddress = CircleTop * MaxWidth;

	memset (&Edge[CircleTop], -1, CircleSize*sizeof(CircleEdge));
}

void Circle::FillBuffer ()
{
	int	i, j, k;
	char *buf = CircleBuffer + CircleTopAddress;
	CircleEdge *edge = Edge + CircleTop;

	for (i=0; i < CircleSize;i++) {
		k = edge -> Left;
		if (k >= 0) {
			j = edge -> Right - k;
			if (j >= 0) {
				char *buf1 = buf + k;
				do {
#ifdef CHECKCIRCLEOVERLAP
					if (*buf1 < MAXOVERLAP) 
						*buf1 += 1;
#else
					*buf1 = 4;
#endif
					buf1++;
					j--;
				} while (j >= 0);
			}
		}
		edge++;
		buf += MaxWidth;
	}
}

void Circle::CreateFilledCircle ()
{
	InitBuffer ();
	
	long x = 0;
	long y = Radius;
	long d = 1 - Radius;
	long dE = 3;
	long dSE = -2 * Radius + 5;

	CreateFilledCirclePoints (x, y);
	while (y > x) {
		if (d < 0) {
			d += dE;
			dE += 2;
			dSE += 2;
			x++;
		}
		else {
			d += dSE;
			dE += 2;
			dSE += 4;
			x++;
			y--;
		}
		CreateFilledCirclePoints (x, y);
	}

	FillBuffer ();
}

void Circle::CreateFilledCirclePoints (long x, long y)
{
	long	cx, size;
		
	cx = CenterX - x;
	size = x << 1;
	FillCirclePoints (cx, CenterY + y, size);
	FillCirclePoints (cx, CenterY - y, size);

	cx = CenterX - y;
	size = y << 1;
	FillCirclePoints (cx, CenterY + x, size);
	FillCirclePoints (cx, CenterY - x, size);
}

void Circle::CreateFilledArc0 ()
{
	FillVerticalLineDownLeft (CenterX, CenterY);
	FillDiagonalLineRightDownRight (CenterX, CenterY);

	long x = 0;
	long y = Radius;
	long d = 1 - Radius;
	long dE = 3;
	long dSE = -2 * Radius + 5;
	FillRightEdge (CenterX + x, CenterY + y);

	while (y > x) {
		if (d < 0) {
			d += dE;
			dE += 2;
			dSE += 2;
			x++;
		}
		else {
			d += dSE;
			dE += 2;
			dSE += 4;
			x++;
			y--;
		}
		FillRightEdge (CenterX + x, CenterY + y);
	}
}

void Circle::CreateFilledArc1 ()
{
	FillDiagonalLineRightDownLeft (CenterX+2, CenterY+1);

	long x = 0;
	long y = Radius;
	long d = 1 - Radius;
	long dE = 3;
	long dSE = -2 * Radius + 5;
	FillRightEdge (CenterX + y, CenterY + x);

	while (y > x) {
		if (d < 0) {
			d += dE;
			dE += 2;
			dSE += 2;
			x++;
		}
		else {
			d += dSE;
			dE += 2;
			dSE += 4;
			x++;
			y--;
		}
		FillRightEdge (CenterX + y, CenterY + x);
	}
}

void Circle::CreateFilledArc2 ()
{
	FillDiagonalLineRightUpLeft (CenterX+1, CenterY);

	long x = 0;
	long y = Radius;
	long d = 1 - Radius;
	long dE = 3;
	long dSE = -2 * Radius + 5;
	FillRightEdge (CenterX + y, CenterY - x);

	while (y > x) {
		if (d < 0) {
			d += dE;
			dE += 2;
			dSE += 2;
			x++;
		}
		else {
			d += dSE;
			dE += 2;
			dSE += 4;
			x++;
			y--;
		}
		FillRightEdge (CenterX + y, CenterY - x);
	}
}

void Circle::CreateFilledArc3 ()
{
	FillVerticalLineUpLeft (CenterX, CenterY);
	FillDiagonalLineRightUpRight (CenterX, CenterY);

	long x = 0;
	long y = Radius;
	long d = 1 - Radius;
	long dE = 3;
	long dSE = -2 * Radius + 5;
	FillRightEdge (CenterX + x, CenterY - y);

	while (y > x) {
		if (d < 0) {
			d += dE;
			dE += 2;
			dSE += 2;
			x++;
		}
		else {
			d += dSE;
			dE += 2;
			dSE += 4;
			x++;
			y--;
		}
		FillRightEdge (CenterX + x, CenterY - y);
	}
}

void Circle::CreateFilledArc4 ()
{
	FillVerticalLineUpRight (CenterX-1, CenterY);
	FillDiagonalLineLeftUpLeft (CenterX, CenterY);

	long x = 0;
	long y = Radius;
	long d = 1 - Radius;
	long dE = 3;
	long dSE = -2 * Radius + 5;
	FillLeftEdge (CenterX - x, CenterY - y);

	while (y > x) {
		if (d < 0) {
			d += dE;
			dE += 2;
			dSE += 2;
			x++;
		}
		else {
			d += dSE;
			dE += 2;
			dSE += 4;
			x++;
			y--;
		}
		FillLeftEdge (CenterX - x, CenterY - y);
	}
}

void Circle::CreateFilledArc5 ()
{
	FillDiagonalLineLeftUpRight (CenterX-1, CenterY);

	long x = 0;
	long y = Radius;
	long d = 1 - Radius;
	long dE = 3;
	long dSE = -2 * Radius + 5;
	FillLeftEdge (CenterX - y, CenterY - x);

	while (y > x) {
		if (d < 0) {
			d += dE;
			dE += 2;
			dSE += 2;
			x++;
		}
		else {
			d += dSE;
			dE += 2;
			dSE += 4;
			x++;
			y--;
		}
		FillLeftEdge (CenterX - y, CenterY - x);
	}
}

void Circle::CreateFilledArc6 ()
{
	FillDiagonalLineLeftDownRight (CenterX-2, CenterY+1);

	long x = 0;
	long y = Radius;
	long d = 1 - Radius;
	long dE = 3;
	long dSE = -2 * Radius + 5;
	FillLeftEdge (CenterX - y, CenterY + x);

	while (y > x) {
		if (d < 0) {
			d += dE;
			dE += 2;
			dSE += 2;
			x++;
		}
		else {
			d += dSE;
			dE += 2;
			dSE += 4;
			x++;
			y--;
		}
		FillLeftEdge (CenterX - y, CenterY + x);
	}
}

void Circle::CreateFilledArc7 ()
{
	FillVerticalLineDownRight (CenterX-1, CenterY);
	FillDiagonalLineLeftDownLeft (CenterX, CenterY);

	long x = 0;
	long y = Radius;
	long d = 1 - Radius;
	long dE = 3;
	long dSE = -2 * Radius + 5;
	FillLeftEdge (CenterX - x, CenterY + y);

	while (y > x) {
		if (d < 0) {
			d += dE;
			dE += 2;
			dSE += 2;
			x++;
		}
		else {
			d += dSE;
			dE += 2;
			dSE += 4;
			x++;
			y--;
		}
		FillLeftEdge (CenterX - x, CenterY + y);
	}
}

void Circle::CreateFilledArc (long octant)
{
	InitBuffer ();
	switch (octant) {
		case 0:
			CreateFilledArc0 ();
			break;
		case 1:
			CreateFilledArc1 ();
			break;
		case 2:
			CreateFilledArc2 ();
			break;
		case 3:
			CreateFilledArc3 ();
			break;
		case 4:
			CreateFilledArc4 ();
			break;
		case 5:
			CreateFilledArc5 ();
			break;
		case 6:
			CreateFilledArc6 ();
			break;
		case 7:
			CreateFilledArc7 ();
			break;
	}
	FillBuffer ();
}
