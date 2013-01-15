#ifndef _C_FEATURE_H_
#define _C_FEATURE_H_

class C_Feature : public C_Control
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
		uchar		Operational_;
		uchar		FeatureValue_;
		long		featureID_;

		O_Output	*Name_;
		O_Output	*Status_;
		O_Output	*Value_;

		COLORREF	Color_[2];
		TREELIST	*Owner_;

		VU_ID		vuID;

	public:

		C_Feature();
		C_Feature(char **stream);
		C_Feature(FILE *fp);
		~C_Feature();
		long Size();
		void Save(char **)	  { ; }
		void Save(FILE *)	  { ; }

		void Setup(long ID,short Type);
		void Cleanup();
		void InitEntity();

		void SetOwner(TREELIST *limb)					{ Owner_=limb; }
		TREELIST *GetOwner()							{ return(Owner_); }

		void SetName(short x,short y,long ID)			{ if(Name_ && gStringMgr) { Name_->SetXY(x,y); Name_->SetText(gStringMgr->GetText(ID)); Name_->SetInfo(); } }
		void SetName(short x,short y,_TCHAR *str)		{ if(Name_) { Name_->SetXY(x,y); Name_->SetText(str); Name_->SetInfo(); } }
		void SetStatus(short x,short y,long ID)			{ if(Status_ && gStringMgr) { Status_->SetXY(x,y); Status_->SetText(gStringMgr->GetString(ID)); Status_->SetInfo(); } }
		void SetStatus(short x,short y,_TCHAR *str)		{ if(Status_) { Status_->SetXY(x,y); Status_->SetText(str); Status_->SetInfo(); } }
		void SetValue(short x,short y,long ID)			{ if(Value_ && gStringMgr) { Value_->SetXY(x,y); Value_->SetText(gStringMgr->GetString(ID)); Value_->SetInfo(); } }
		void SetValue(short x,short y,_TCHAR *str)		{ if(Value_) { Value_->SetXY(x,y); Value_->SetText(str); Value_->SetInfo(); } }

		void SetName(long ID)							{ if(Name_ && gStringMgr) { Name_->SetText(gStringMgr->GetText(ID)); Name_->SetInfo(); } }
		void SetName(_TCHAR *str)						{ if(Name_) { Name_->SetText(str); Name_->SetInfo(); } }
		void SetStatus(long ID)							{ if(Status_ && gStringMgr) { Status_->SetText(gStringMgr->GetString(ID)); Status_->SetInfo(); } }
		void SetStatus(_TCHAR *str)						{ if(Status_) { Status_->SetText(str); Status_->SetInfo(); } }
		void SetValue(long ID)							{ if(Value_ && gStringMgr) { Value_->SetText(gStringMgr->GetString(ID)); Value_->SetInfo(); } }
		void SetValue(_TCHAR *str)						{ if(Value_) { Value_->SetText(str); Value_->SetInfo(); } }

		void SetState(short newstate);
		short GetState()								{ return(State_); }

		void SetColor(COLORREF off,COLORREF on)			{ Color_[0]=off; Color_[1]=on; }

		void SetOperational(uchar s)					{ Operational_=s; }
		uchar GetOperational()							{ return(Operational_); }

		void SetFeatureValue(uchar s)					{ FeatureValue_=s; }
		uchar GetFeatureValue()							{ return(FeatureValue_); }

		_TCHAR *GetName()								{ if(Name_) return(Name_->GetText()); return(NULL); }
		_TCHAR *GetStatus()								{ if(Status_) return(Status_->GetText()); return(NULL); }
		_TCHAR *GetValue()								{ if(Value_) return(Value_->GetText()); return(NULL); }

		void SetFont(long FontID);

		long CheckHotSpots(long relX,long relY);
		void SetDefaultFlags();
		long GetDefaultFlags();
		BOOL Process(long ID,short HitType);
		BOOL Dragable(long)	{ return FALSE;}
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);
		void SetSubParents(C_Window *);

		void SetVUID(VU_ID id) { vuID=id; }
		VU_ID GetVUID() { return(vuID); }
		void SetFeatureID(long id) { featureID_=id; }
		long GetFeatureID() { return(featureID_); }
};

#endif
