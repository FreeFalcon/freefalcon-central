#ifndef _ATO_SPECIAL_H_
#define _ATO_SPECIAL_H_

#define _MAX_PLANE_ICONS_ 4
class C_ATO_Flight : public C_Control
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		long		Defaultflags_;

		short		Section_;
		short		State_;
		TREELIST	*Owner_;
		short		IconX_,IconY_;
		short		TaskX_,TaskY_;
		long		DefaultFlags_;

		COLORREF	IconBgColor_[2];
		COLORREF	FlightBgColor_[2];

		UI95_RECT	IconBg_;
		UI95_RECT	FlightBg_;
		C_ListBox	*Task_;
		O_Output	*Icon_;
		O_Output	*Callsign_;
		O_Output	*Planes_;
		O_Output	*Airbase_;
		O_Output	*Status_;

		VU_ID		vuID;
	public:

		enum
		{
			_NOTHING_=0,
			_ICON_AREA_,
			_FLIGHT_AREA_,
			_TASK_,
		};

		C_ATO_Flight();
		C_ATO_Flight(char **stream);
		C_ATO_Flight(FILE *fp);
		~C_ATO_Flight();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }

		void Setup(long ID,short Type);
		void Cleanup();
		void InitFlight(C_Handler *Handler);

		void SetOwner(TREELIST *limb) { Owner_=limb; }
		TREELIST *GetOwner() { return(Owner_); }
		void SetXY(long x,long y);

		void SetIconBg(short x,short y,short w,short h)		{ IconBg_.left=x; IconBg_.top=y; IconBg_.right=w; IconBg_.bottom=h; }
		void SetFlightBg(short x,short y,short w,short h)	{ FlightBg_.left=x; FlightBg_.top=y; FlightBg_.right=w; FlightBg_.bottom=h; }
		void SetTask(short x,short y,LISTBOX *tasklist) { TaskX_=x; TaskY_=y; if(Task_) Task_->SetRoot(tasklist); }

		void SetIcon(short x,short y,long ImageID)		{ if(Icon_) { Icon_->SetXY(x,y); Icon_->SetImage(ImageID); Icon_->SetInfo(); } }
		void SetIcon(short x,short y,IMAGE_RSC *Image)	{ if(Icon_) { Icon_->SetXY(x,y); Icon_->SetImage(Image); Icon_->SetInfo(); } }
		void SetCallsign(short x,short y,_TCHAR *str)	{ if(Callsign_) { Callsign_->SetXY(x,y); Callsign_->SetText(str); Callsign_->SetInfo(); } }
		void SetAirbase(short x,short y,_TCHAR *str)	{ if(Airbase_) { Airbase_->SetXY(x,y); Airbase_->SetText(str); Airbase_->SetInfo(); } }
		void SetPlanes(short x,short y,_TCHAR *str)		{ if(Planes_) { Planes_->SetXY(x,y); Planes_->SetText(str); Planes_->SetInfo(); } }
		void SetStatus(short x,short y,_TCHAR *str)		{ if(Status_) { Status_->SetXY(x,y); Status_->SetText(str); Status_->SetInfo(); } }

		void SetIcon(long ImageID)					{ if(Icon_) Icon_->SetImage(ImageID); }
		void SetIcon(IMAGE_RSC *Image)				{ if(Icon_) Icon_->SetImage(Image); }
		void SetTask(LISTBOX *tasklist)				{ if(Task_) Task_->SetRoot(tasklist); }
		void SetCallsign(_TCHAR *str)				{ if(Callsign_) { Callsign_->SetText(str); Callsign_->SetInfo(); } }
		void SetAirbase(_TCHAR *str)				{ if(Airbase_) { Airbase_->SetText(str); Airbase_->SetInfo(); } }
		void SetPlanes(_TCHAR *str)					{ if(Planes_) { Planes_->SetText(str); Planes_->SetInfo(); } }
		void SetStatus(_TCHAR *str)					{ if(Status_) { Status_->SetText(str); Status_->SetInfo(); } }
		void SetCurrentTask(long ID)				{ if(Task_) Task_->SetValue(ID); }

		void SetIconBgColor(COLORREF Off,COLORREF On) { IconBgColor_[0]=Off; IconBgColor_[1]=On; }
		void SetFlightBgColor(COLORREF Off,COLORREF On) { FlightBgColor_[0]=Off; FlightBgColor_[1]=On; }

		void SetFont(long FontID);

		void SetState(short newstate) { State_= static_cast<short>(newstate & 1); }
		short GetState() { return(State_); }

		UI95_RECT GetIconBg()			{ return(IconBg_); }
		UI95_RECT GetFlightBg()			{ return(FlightBg_); }
		_TCHAR *GetCurrentTask()	{ if(Task_) return(Task_->GetText()); return(NULL); }
		long GetCurrentTaskID()		{ if(Task_) return(Task_->GetTextID()); return(0); }
		_TCHAR *GetCallsign()		{ if(Callsign_) return(Callsign_->GetText()); return(NULL); }
		_TCHAR *GetAirbase()		{ if(Airbase_) return(Airbase_->GetText()); return(NULL); }
		_TCHAR *GetPlanes()			{ if(Planes_) return(Planes_->GetText()); return(NULL); }
		_TCHAR *GetStatus()			{ if(Status_) return(Status_->GetText()); return(NULL); }

		C_ListBox *GetTaskCtrl()	{ return(Task_); }
		O_Output *GetIconCtrl()		{ return(Icon_); }
		O_Output *GetCallsignCtrl() { return(Callsign_); }
		O_Output *GetAirbaseCtrl()	{ return(Airbase_); }
		O_Output *GetPlanesCtrl()	{ return(Planes_); }
		O_Output *GetStatusCtrl()	{ return(Status_); }

		void SetSubParents(C_Window *);
		long CheckHotSpots(long relX,long relY);
		void SetDefaultFlags();
		long GetDefaultFlags();
		BOOL Process(long ID,short HitType);
		BOOL Dragable(long) { return FALSE;	}
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);

		void SetVUID(VU_ID id) { vuID=id; }
		VU_ID GetVUID() { return(vuID); }
};

#endif
