#ifndef _C_ENTITY_H_
#define _C_ENTITY_H_

class C_Entity : public C_Control
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		long		Defaultflags_;

		short		State_;
		short		IconX_,IconY_;
		uchar		Operational_;

		COLORREF	IconBgColor_[2];
		COLORREF	InfoBgColor_[2];

		UI95_RECT	IconBg_;
		UI95_RECT	InfoBg_;
		O_Output	*Icon_;
		O_Output	*Name_;
		O_Output	*Status_;

		TREELIST	*Owner_;

		VU_ID		vuID;
	public:

		C_Entity();
		C_Entity(char **stream);
		C_Entity(FILE *fp);
		~C_Entity();
		long Size();
		void Save(char **)	  { ; }
		void Save(FILE *)	  { ; }

		void Setup(long ID,short Type);
		void Cleanup();
		void InitEntity();

		void SetOwner(TREELIST *limb)					{ Owner_=limb; }
		TREELIST *GetOwner()							{ return(Owner_); }

		void SetIconBg(short x,short y,short w,short h)			{ IconBg_.left=x; IconBg_.top=y; IconBg_.right=w; IconBg_.bottom=h; }
		void SetInfoBg(short x,short y,short w,short h)			{ InfoBg_.left=x; InfoBg_.top=y; InfoBg_.right=w; InfoBg_.bottom=h; }

		void SetIcon(short x,short y,long ImageID)			{ if(Icon_) { Icon_->SetXY(x,y); Icon_->SetImage(ImageID); Icon_->SetInfo(); } }
		void SetIcon(short x,short y,IMAGE_RSC *Image)		{ if(Icon_) { Icon_->SetXY(x,y); Icon_->SetImage(Image); Icon_->SetInfo(); } }
		void SetName(short x,short y,_TCHAR *str)			{ if(Name_) { Name_->SetXY(x,y); Name_->SetText(str); Name_->SetInfo(); } }
		void SetStatus(short x,short y,_TCHAR *str)			{ if(Status_) { Status_->SetXY(x,y); Status_->SetText(str); Status_->SetInfo(); } }

		void SetIcon(IMAGE_RSC *Image)					{ if(Icon_) Icon_->SetImage(Image); }
		void SetName(_TCHAR *str)						{ if(Name_) { Name_->SetText(str); Name_->SetInfo(); } }
		void SetStatus(_TCHAR *str)						{ if(Status_) { Status_->SetText(str); Status_->SetInfo(); } }

		void SetIconBgColor(COLORREF Off,COLORREF On)	{ IconBgColor_[0]=Off; IconBgColor_[1]=On; }
		void SetInfoBgColor(COLORREF Off,COLORREF On)	{ InfoBgColor_[0]=Off; InfoBgColor_[1]=On; }

		void SetState(short newstate)					{ State_=static_cast<short>(newstate & 1); }
		short GetState()								{ return(State_); }

		void SetOperational(uchar s)					{ Operational_=s; }
		uchar GetOperational()							{ return(Operational_); }

		_TCHAR *GetName()								{ if(Name_) return(Name_->GetText()); return(NULL); }
		_TCHAR *GetStatus()								{ if(Status_) return(Status_->GetText()); return(NULL); }

		void SetFont(long FontID);

		UI95_RECT GetIconBg()								{ return(IconBg_); }
		UI95_RECT GetInfoBg()								{ return(InfoBg_); }
		O_Output *GetIcon()									{ return(Icon_); }
		long CheckHotSpots(long relX,long relY);
		void SetDefaultFlags();
		long GetDefaultFlags();
		BOOL Process(long ID,short HitType);
		BOOL Dragable(long)		{ return FALSE;	}
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);

		void SetVUID(VU_ID id) { vuID=id; }
		VU_ID GetVUID() { return(vuID); }
};

#endif
