#ifndef _WAYPOINTS_H_
#define _WAYPOINTS_H_

#define WAYPOINT_LABEL 30

class WAYPOINTLIST
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		long			ID;
		long			Type;
		long			Flags;
		long			Group;
		short			state;
		short			x,y;
		float			worldx,worldy;
		C_Button		*Icon;
		short			Dragable;
		COLORREF		LineColor_[3];
		WAYPOINTLIST	*Next;
};

class C_Waypoint : public C_Control
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		WAYPOINTLIST	*Root_,*LastWP_;

		long			Font_;

		IMAGE_RSC		*Images_[8];

		float			MinWorldX_,MinWorldY_;
		float			MaxWorldX_,MaxWorldY_;

		short			WPScaleType_;

		short			Dragging_;
		UI95_RECT		last_;
		float			scale_;
		long			DefaultFlags_;

	public:
		C_Waypoint();
		C_Waypoint(char **stream);
		C_Waypoint(FILE *fp);
		~C_Waypoint();
		long Size();
		void Save(char **)		 { ; }
		void Save(FILE *)		 { ; }

		// Setup Functions
		void Setup(long ID,short Type);
		void Cleanup(void);
		void SetMainImage(long imageID);
		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }
		void SetWorldRange(float minx,float miny,float maxx,float maxy) { MinWorldX_=minx; MinWorldY_=miny; MaxWorldX_=maxx; MaxWorldY_=maxy; }

		void SetFont(long ID) { Font_=ID; }
		long GetFont() { return(Font_); }

		WAYPOINTLIST *AddWaypointToList(long CampID,short type,long NormID,long SelID,long OthrID,float x,float y,short Dragable);
		BOOL UpdateInfo(long ID,float x,float y);
		void EraseWaypointList();
		void EraseWaypointGroup(long groupid);
		BOOL ShowByType(long mask);
		BOOL HideByType(long mask);
		void SetScaleFactor(float scale);
		void SetScaleType(short scaletype);
 		WAYPOINTLIST *FindID(long ID);
		void SetWPGroup(long ID,long group);
		void SetState(long ID,short state);
		void SetGroupState(long group,short state);
		void SetLabel(long ID,_TCHAR *txt);
		void SetLabelColor(long ID,COLORREF norm,COLORREF sel,COLORREF othr);
		void SetLineColor(long ID,COLORREF norm,COLORREF sel,COLORREF othr);
		_TCHAR *GetLabel(long ID);
		void SetTextOffset(long ID,short x,short y);

		WAYPOINTLIST *GetRoot() { return(Root_); }
		WAYPOINTLIST *GetLast() { return(LastWP_); }

		// Handler/Window Functions
		long CheckHotSpots(long relX,long relY);
		BOOL Dragable(long) { return(GetFlags() & C_BIT_DRAGABLE);}
		BOOL Process(long ID,short ButtonHitType);
 		BOOL MouseOver(long relX,long relY,C_Base *);
		void GetItemXY(long ID,long *x,long *y);
		BOOL Drag(GRABBER *Drag,WORD MouseX,WORD MouseY,C_Window *over);
		BOOL Drop(GRABBER *,WORD ,WORD ,C_Window *);
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
		short  Dragging() { return(Dragging_); }
		void SetSubParents(C_Window *par);
};

#endif

