#ifndef _ATO_PACKAGE_H_
#define _ATO_PACKAGE_H_

class C_ATO_Package : public C_Control
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_CONTROL_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	private:
		long		Defaultflags_;

		short			Section_;
		short		State_;
		short		WPState_;
		TREELIST	*Owner_;
		long		DefaultFlags_;
		IMAGE_RSC	*Image_[2];

		COLORREF	Color_[2];
		O_Output	*Title_;
		O_Output	*ShowWP_;
		VU_ID		vuID;

	public:

		C_ATO_Package();
		C_ATO_Package(char **stream);
		C_ATO_Package(FILE *fp);
		~C_ATO_Package();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }

		void Setup(long ID,short Type);
		void Cleanup();
		void InitPackage(C_Handler *);

		void SetOwner(TREELIST *limb) { Owner_=limb; }
		TREELIST *GetOwner() { return(Owner_); }

		void SetTitle(short x,short y,_TCHAR *txt)			{ if(Title_) { Title_->SetXY(x,y); Title_->SetText(txt); Title_->SetInfo(); } }
		void SetCheckBox(short x,short y,long off,long on);
		void SetTitle(_TCHAR *txt)						{ if(Title_) Title_->SetText(txt); }
		void SetFont(long FontID);

		void SetColors(COLORREF offcolor,COLORREF oncolor) { Color_[0]=offcolor; Color_[1]=oncolor; }

		void SetState(short newstate);
		short GetState() { return(State_); }

		void SetWPState(short newstate);
		short GetWPState() { return(WPState_); }

		long CheckHotSpots(long relX,long relY);
		void SetDefaultFlags();
		long GetDefaultFlags();
		BOOL Process(long ID,short HitType);
		BOOL Dragable(long)	{ return FALSE;	}
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);

		void SetVUID(VU_ID id) { vuID=id; }
		VU_ID GetVUID() { return(vuID); }
};

#endif
