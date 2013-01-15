#ifndef _C_PLAYER_H_
#define _C_PLAYER_H_

class C_Player : public C_Control
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
		short		muted_;
		short		ignored_;

		COLORREF	Color_[3];

		O_Output	*Icon_;
		O_Output	*Name_;
		O_Output	*Status_;

		TREELIST	*Owner_;

		VU_ID		vuID;
	public:

		C_Player();
		C_Player(char **stream);
		C_Player(FILE *fp);
		~C_Player();
		long Size();
		void Save(char **)		{ ; }
		void Save(FILE *)		{ ; }

		void Setup(long ID,short Type);
		void Cleanup();
		void InitEntity();

		void SetOwner(TREELIST *limb)						{ Owner_=limb; }
		TREELIST *GetOwner()								{ return(Owner_); }

		void SetIcon(short x,short y,long ImageID)			{ if(Icon_) { Icon_->SetXY(x,y); Icon_->SetImage(ImageID); Icon_->SetInfo(); } }
		void SetIcon(short x,short y,IMAGE_RSC *Image)		{ if(Icon_) { Icon_->SetXY(x,y); Icon_->SetImage(Image); Icon_->SetInfo(); } }
		void SetName(short x,short y,TCHAR *str) 	        { if(Name_) { Name_->SetXY(x,y); Name_->SetText(str); Name_->SetInfo(); } }
		void SetStatus(short x,short y,long ImageID)		{ if(Status_) { Status_->SetXY(x,y); Status_->SetImage(ImageID); Status_->SetInfo(); } }
		void SetStatus(short x,short y,IMAGE_RSC *Image)	{ if(Status_) { Status_->SetXY(x,y); Status_->SetImage(Image); Status_->SetInfo(); } }

		void SetIcon(IMAGE_RSC *Image)						{ if(Icon_) Icon_->SetImage(Image); }
		void SetName(_TCHAR *str)							{ if(Name_) { Name_->SetText(str); Name_->SetInfo(); } }
		void SetStatus(IMAGE_RSC *Image)					{ if(Status_) Status_->SetImage(Image); }

		void SetColor(COLORREF Off,COLORREF On,COLORREF ign)	{ Color_[0]=Off; Color_[1]=On; Color_[2]=ign; }

		void SetState(short newstate);
		short GetState()									{ return(State_); }

		void SetMute(short mute);
		short GetMute()										{ return(muted_); }

		void SetIgnore(short ignore);
		short GetIgnore()									{ return(ignored_); }

		_TCHAR *GetName()									{ if(Name_) return(Name_->GetText()); return(NULL); }

		void SetFont(long FontID);

		long CheckHotSpots(long relX,long relY);
		void SetDefaultFlags();
		long GetDefaultFlags();
		BOOL Process(long ID,short HitType);
		BOOL Dragable(long)	{ return FALSE; }
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
		void SetSubParents(C_Window *);

		void SetVUID(VU_ID id)								{ vuID=id; }
		VU_ID GetVUID()										{ return(vuID); }
};

#endif
