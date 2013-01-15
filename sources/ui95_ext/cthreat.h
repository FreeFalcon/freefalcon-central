#ifndef _THREAT_ARCS_H_
#define _THREAT_ARCS_H_

class C_Threat;

typedef struct
{
	long ID;
	long Type;
	long  Flags;
	long x,y;
	long Radius[8];
	C_Threat *Owner;
} THREAT_CIRCLE;

#define	HALFSQUAREROOT2	(0.5 * sqrt(2.0))
#define	MAXCIRCLESIZE	4096
#define	MAXOVERLAP		8

#define	CHECKCIRCLEOVERLAP

struct CircleEdge {
	long Left, Right;
};

class Circle {
	protected:
		static CircleEdge Edge[MAXCIRCLESIZE];
		static long MaxHeight, MaxWidth;
		static long MaxHeight1, MaxWidth1;
		static long CenterX, CenterY, Radius, Diagonal;
		static long CircleTop, CircleSize;
		static int	CircleTopAddress;
		static char *CircleBuffer;

#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
public:
	void SetBuffer (char *buffer) {
		CircleBuffer = buffer;
	};

	void SetDimension (long width, long height) {
		MaxWidth = width;
		MaxHeight = height;
		MaxWidth1 = width - 1;
		MaxHeight1 = height - 1;
	};

	void SetCenter (long centerx, long centery) {
		CenterX = centerx;
		CenterY = centery;
	};

	void SetRadius (long radius) {
		Radius = radius;
		Diagonal = (long) ((double) radius * HALFSQUAREROOT2 + 0.5);
	};

	void FillLeftEdge (long cx, long cy) {
		if (cy >= 0 && cy < MaxHeight && cx < MaxWidth) {
			if (cx < 0) cx = 0;
			Edge[cy].Left = cx;
		}
	};

	void FillRightEdge (long cx, long cy) {
		if (cy >= 0 && cy < MaxHeight && cx > 0) {
			if (cx > MaxWidth1) cx = MaxWidth1;
			Edge[cy].Right = cx;
		}
	};

	void FillLeftEdgeY (long cx, long cy) {
		if (cy >= 0 && cy < MaxHeight)
			Edge[cy].Left = cx;
	};

	void FillRightEdgeY (long cx, long cy) {
		if (cy >= 0 && cy < MaxHeight)
			Edge[cy].Right = cx;
	};

	void FillCirclePoints (long x, long y, long width) {
		if (y >= 0 && y < MaxHeight && x < MaxWidth) {
			long x1 = x + width;
			if (x1 >= 0) {
				CircleEdge *edge = &(Edge[y]);
				if (x < 0) x = 0;
				edge -> Left = x;
				if (x1 > MaxWidth1) x1 = MaxWidth1;
				edge -> Right = x1;
			}
		}
	};

	void FillVerticalLineUpLeft (long cx, long cy) {
		long	i;

		if (cx < 0) cx = 0;
		else if (cx > MaxWidth1) cx = MaxWidth1;

		cy--;
		for (i=0; i < Radius; i++) {
			FillLeftEdgeY (cx, cy);
			cy--;
		}
	};

	void FillVerticalLineUpRight (long cx, long cy) {
		long	i;

		if (cx < 0) cx = 0;
		else if (cx > MaxWidth1) cx = MaxWidth1;

		cy--;
		for (i=0; i < Radius; i++) {
			FillRightEdgeY (cx, cy);
			cy--;
		}
	};

	void FillVerticalLineDownLeft (long cx, long cy) {
		long	i;

		if (cx < 0) cx = 0;
		else if (cx > MaxWidth1) cx = MaxWidth1;

		for (i=0; i <= Radius; i++) {
			FillLeftEdgeY (cx, cy);
			cy++;
		}
	};

	void FillVerticalLineDownRight (long cx, long cy) {
		long	i;

		if (cx < 0) cx = 0;
		else if (cx > MaxWidth1) cx = MaxWidth1;

		for (i=0; i <= Radius; i++) {
			FillRightEdgeY (cx, cy);
			cy++;
		}
	};

	void FillDiagonalLineLeftUpLeft (long cx, long cy) {
		long	i;
		for (i=0; i < Diagonal; i++) {
			FillLeftEdge (cx, cy);
			cy--;
			cx--;
		}
	};

	void FillDiagonalLineLeftUpRight (long cx, long cy) {
		long	i;
		for (i=0; i < Diagonal; i++) {
			FillRightEdge (cx, cy);
			cy--;
			cx--;
		}
	};

	void FillDiagonalLineLeftDownLeft (long cx, long cy) {
		long	i;
		for (i=0; i < Diagonal; i++) {
			FillLeftEdge (cx, cy);
			cy++;
			cx--;
		}
	};

	void FillDiagonalLineLeftDownRight (long cx, long cy) {
		long	i;
		for (i=0; i < Diagonal; i++) {
			FillRightEdge (cx, cy);
			cy++;
			cx--;
		}
	};

	void FillDiagonalLineRightUpLeft (long cx, long cy) {
		long	i;
		for (i=0; i < Diagonal; i++) {
			FillLeftEdge (cx, cy);
			cy--;
			cx++;
		}
	};

	void FillDiagonalLineRightUpRight (long cx, long cy) {
		long	i;
		for (i=0; i < Diagonal; i++) {
			FillRightEdge (cx, cy);
			cy--;
			cx++;
		}
	};

	void FillDiagonalLineRightDownLeft (long cx, long cy) {
		long	i;
		for (i=0; i < Diagonal; i++) {
			FillLeftEdge (cx, cy);
			cy++;
			cx++;
		}
	};

	void FillDiagonalLineRightDownRight (long cx, long cy) {
		long	i;
		for (i=0; i < Diagonal; i++) {
			FillRightEdge (cx, cy);
			cy++;
			cx++;
		}
	};

	void InitBuffer ();
	void FillBuffer ();

	void CreateFilledArc0 ();
	void CreateFilledArc1 ();
	void CreateFilledArc2 ();
	void CreateFilledArc3 ();
	void CreateFilledArc4 ();
	void CreateFilledArc5 ();
	void CreateFilledArc6 ();
	void CreateFilledArc7 ();

	void CreateFilledArc (long octant);

	void CreateFilledCircle ();
	void CreateFilledCirclePoints (long x, long y);

};

class C_Threat : public C_Base
{
	protected:
		static Circle myCircle;
		C_Hash		*Root_;

	public:
		enum {
			THR_CIRCLE=0,
			THR_SLICE=1,
		};
		C_Threat();
		~C_Threat();

		void Setup(long ID,long type);
		void Cleanup();
		void AddCircle(long ID,long type,long worldx,long worldy,long radius); // all units are KM
		void UpdateCircle(long ID,long worldx,long worldy);
		void SetRadius(long ID,long slice,long radius);
		void Remove(long ID);
		THREAT_CIRCLE *GetThreat(long ID) { if(Root_) return((THREAT_CIRCLE*)Root_->Find(ID)); return(NULL); }
		void BuildOverlay(BYTE *overlay,long w,long h,float kmperpixel);
};


#endif