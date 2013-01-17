#ifndef _MISSION_INFO_H_
#define _MISSION_INFO_H_

class C_Mission : public C_Control
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		long DefaultFlags_;

		long Font_;

		long TakeOffTime_;
		short MissionID_;
		short PackageID_;
		short StatusID_;
		short PriorityID_;
		short State_;

		COLORREF Color_[4]; //0-off,1=on,2=playerflt,3=disabled

		O_Output *TakeOff_;
		O_Output *Mission_;
		O_Output *Package_;
		O_Output *Status_;
		O_Output *Priority_;

		TREELIST *Owner_;
		VU_ID		vuID;

	public:
		C_Mission();
		C_Mission(char **stream);
		C_Mission(FILE *fp);
		~C_Mission();
		long Size();
		void Save(char **)		{ ; }
		void Save(FILE *)		{ ; }

		void Setup(long ID,short Type);
		void Cleanup(void);

		void SetTakeOffTime(long ID)			{ TakeOffTime_=ID; }
		long GetTakeOffTime()					{ return(TakeOffTime_); }
		void SetMissionID(short ID)				{ MissionID_=ID; }
		short GetMissionID()					{ return(MissionID_); }
		void SetPackageID(short ID)				{ PackageID_=ID; }
		short GetPackageID()					{ return(PackageID_); }
		void SetStatusID(short ID)				{ StatusID_=ID; }
		short GetStatusID()						{ return(StatusID_); }
		void SetPriorityID(short ID)			{ PriorityID_=ID; }
		short GetPriorityID()					{ return(PriorityID_); }

		void SetState(short state)				{ State_=static_cast<short>(state & 3); }
		short GetState()						{ return(State_); }

		void SetNormalColor(COLORREF color)		{ Color_[0]=color; }
		void SetSelectedColor(COLORREF color)	{ Color_[1]=color; }
		void SetPlayerColor(COLORREF color)		{ Color_[2]=color; }
		void SetDisabledColor(COLORREF color)	{ Color_[3]=color; }

		void SetTakeOff(short x,short y,_TCHAR *txt);
		void SetMission(short x,short y,_TCHAR *txt);
		void SetPackage(short x,short y,_TCHAR *txt);
		void SetStatus(short x,short y,_TCHAR *txt);
		void SetPriority(short x,short y,_TCHAR *txt);

		void SetTakeOff(_TCHAR *txt);
		void SetMission(_TCHAR *txt);
		void SetPackage(_TCHAR *txt);
		void SetStatus(_TCHAR *txt);
		void SetPriority(_TCHAR *txt);

		void SetFont(long);

		_TCHAR *GetTakeOff()	{ if(TakeOff_)	return(TakeOff_->GetText());  return(NULL); }
		_TCHAR *GetMission()	{ if(Mission_)	return(Mission_->GetText());  return(NULL); }
		_TCHAR *GetPackage()	{ if(Package_)	return(Package_->GetText());  return(NULL); }
		_TCHAR *GetStatus()		{ if(Status_)	return(Status_->GetText());   return(NULL); }
		_TCHAR *GetPriority()	{ if(Priority_)	return(Priority_->GetText()); return(NULL); }

		void SetOwner(TREELIST *item)			{ Owner_=item; }
		TREELIST *GetOwner()					{ return(Owner_); }

		long GetFont() { return(Font_); }
		void SetDefaultFlags()					{ SetFlags(DefaultFlags_); }
		long GetDefaultFlags()					{ return(DefaultFlags_); }
		long CheckHotSpots(long relx,long rely);
		BOOL Process(long ID,short hittype);
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);

		void SetVUID(VU_ID id) { vuID=id; }
		VU_ID GetVUID() { return(vuID); }
};

#endif

