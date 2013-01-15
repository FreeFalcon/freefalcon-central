#ifndef _DOG_FLIGHT_INFO_H_
#define _DOG_FLIGHT_INFO_H_

#include "vutypes.h"

class C_Dog_Flight : public C_Control
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
		COLORREF Color_[2];
		IMAGE_RSC *Image_[2];

		short state_;

		VU_ID	 vuID;

		O_Output	*Icon_;
		O_Output	*Callsign_;
		O_Output	*Aircraft_;

	public:
		C_Dog_Flight();
		C_Dog_Flight(char **stream);
		C_Dog_Flight(FILE *fp);
		~C_Dog_Flight();
		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }
		void Setup(long ID,short Type);
		void Cleanup(void);

		void SetIcon(short x,short y,IMAGE_RSC *dark,IMAGE_RSC *lite);
		void SetCallsign(short x,short y,_TCHAR *call);
		void SetAircraft(short x,short y,_TCHAR *aircraft);
		void SetFont(long ID) { Font_=ID; }
		void SetState(short state);
		short GetState() { return(state_); }
		long GetFont() { return(Font_); }
		void SetColor(short state,COLORREF color) { Color_[state & 1]=color; }
		void SetSubParents(C_Window *);
		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }

		O_Output *GetIcon()			{ return(Icon_); }
		O_Output *GetCallsign()		{ return(Callsign_); }
		O_Output *GetAircraft()		{ return(Aircraft_); }

		long CheckHotSpots(long relx,long rely);
		BOOL Process(long ID,short hittype);
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);

		void SetVUID(VU_ID id) { vuID=id; }
		VU_ID GetVUID() { return(vuID); }
};

#endif
