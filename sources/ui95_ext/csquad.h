#ifndef _SQUAD_SPECIAL_H_
#define _SQUAD_SPECIAL_H_

class C_Squadron : public C_Control
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		long		Defaultflags_;

		long		BaseID_;
		short		State_;
		short		IconX_,IconY_;
		short		NumVehicles_;
		short		NumPilots_;
		short		NumPlayers_;

		COLORREF	IconBgColor_[2];
		COLORREF	InfoBgColor_[2];

		UI95_RECT	IconBg_;
		UI95_RECT	InfoBg_;
		O_Output	*Icon_;
		O_Output	*Name_;
		O_Output	*Planes_;
		O_Output	*Pilots_;
		O_Output	*Players_;

		TREELIST	*Owner_;

		VU_ID		vuID;

	public:

		C_Squadron();
		C_Squadron(char **stream);
		C_Squadron(FILE *fp);
		~C_Squadron();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }

		void Setup(long ID,short Type);
		void Cleanup();
		void InitSquadron();

		void SetOwner(TREELIST *limb) { Owner_=limb; }
		TREELIST *GetOwner() { return(Owner_); }

		void SetIconBg(short x,short y,short w,short h)		{ IconBg_.left=x; IconBg_.top=y; IconBg_.right=w; IconBg_.bottom=h; }
		void SetInfoBg(short x,short y,short w,short h)		{ InfoBg_.left=x; InfoBg_.top=y; InfoBg_.right=w; InfoBg_.bottom=h; }

		void SetIcon(short x,short y,long ImageID)		{ if(Icon_) { Icon_->SetXY(x,y); Icon_->SetImage(ImageID); Icon_->SetInfo(); } }
		void SetName(short x,short y,_TCHAR *str)		{ if(Name_) { Name_->SetXY(x,y); Name_->SetText(str); Name_->SetInfo(); } }
		void SetPlanes(short x,short y,_TCHAR *str)		{ if(Planes_) { Planes_->SetXY(x,y); Planes_->SetText(str); Planes_->SetInfo(); } }
		void SetPilots(short x,short y,_TCHAR *str)		{ if(Pilots_) { Pilots_->SetXY(x,y); Pilots_->SetText(str); Pilots_->SetInfo(); } }
		void SetPlayers(short x,short y,_TCHAR *str)	{ if(Players_) { Players_->SetXY(x,y); Players_->SetText(str); Players_->SetInfo(); } }

		void SetIcon(long ImageID)					{ if(Icon_) Icon_->SetImage(ImageID); }
		void SetName(_TCHAR *str)					{ if(Name_) { Name_->SetText(str); Name_->SetInfo(); } }
		void SetPlanes(_TCHAR *str)					{ if(Planes_) { Planes_->SetText(str); Planes_->SetInfo(); } }
		void SetPilots(_TCHAR *str)					{ if(Pilots_) { Pilots_->SetText(str); Pilots_->SetInfo(); } }
		void SetPlayers(_TCHAR *str)				{ if(Players_) { Players_->SetText(str); Players_->SetInfo(); } }

		void SetIconBgColor(COLORREF Off,COLORREF On) { IconBgColor_[0]=Off; IconBgColor_[1]=On; }
		void SetInfoBgColor(COLORREF Off,COLORREF On) { InfoBgColor_[0]=Off; InfoBgColor_[1]=On; }

		void SetState(short newstate)	{ State_=static_cast<short>(newstate & 1); }
		short GetState()				{ return(State_); }

		void SetBaseID(long CampID)		{ BaseID_=CampID; }
		void SetNumVehicles(short i)	{ NumVehicles_=i; }
		void SetNumPilots(short i)		{ NumPilots_=i; }
		void SetNumPlayers(short i)		{ NumPlayers_=i; }

		long GetBaseID()				{ return(BaseID_); }
		short GetNumVehicles()			{ return(NumVehicles_); }
		short GetNumPilots()			{ return(NumPilots_); }
		short GetNumPlayers()			{ return(NumPlayers_); }

		void SetFont(long FontID);

		UI95_RECT GetIconBg()			{ return(IconBg_); }
		UI95_RECT GetInfoBg()			{ return(InfoBg_); }
		_TCHAR *GetName()			{ if(Name_) return(Name_->GetText()); return(NULL); }
		_TCHAR *GetPlanes()			{ if(Planes_) return(Planes_->GetText()); return(NULL); }
		_TCHAR *GetPilots()			{ if(Pilots_) return(Pilots_->GetText()); return(NULL); }
		_TCHAR *GetPlayers()		{ if(Players_) return(Players_->GetText()); return(NULL); }

		O_Output *GetIconCtrl()		{ return(Icon_); }
		O_Output *GetNameCtrl()		{ return(Name_); }
		O_Output *GetPlanesCtrl()	{ return(Planes_); }
		O_Output *GetPlayersCtrl()	{ return(Players_); }

		long CheckHotSpots(long relX,long relY);
		void SetDefaultFlags();
		long GetDefaultFlags();
		BOOL Process(long ID,short HitType);
		BOOL Dragable(long)			{ return FALSE; }
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);

		void SetVUID(VU_ID id) { vuID=id; }
		VU_ID GetVUID() { return(vuID); }
};

#endif
