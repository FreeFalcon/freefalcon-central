#ifndef _PILOT_INFO_H_
#define _PILOT_INFO_H_

#include "vutypes.h"

class C_Pilot : public C_Control
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
		short state_;
		short skill_;
		short slot_;
		short isplayer_;

		VU_ID	 vuID;

		O_Output *Callsign_;

	public:
		C_Pilot();
		C_Pilot(char **stream);
		C_Pilot(FILE *fp);
		~C_Pilot();

		long Size();
		void Save(char **)	{ ; }
		void Save(FILE *)	{ ; }
		
		void Setup(long ID,short Type);
		void Cleanup(void);

		void SetCallsign(short x,short y,_TCHAR *call);
		void SetColor(short state,COLORREF color) { Color_[state & 1]=color; }
		void SetFont(long ID) { Font_=ID; }
		void SetSlot(short slot) { slot_=slot; }
		short GetSlot() { return(slot_); }
		void SetSkill(short skill) { skill_=skill; }
		short GetSkill() { return(skill_); }
		void SetPlayer(short isplayer) { isplayer_=isplayer; }
		short GetPlayer() { return(isplayer_); }
		void SetState(short state);
		short GetState() { return(state_); }
		long GetFont() { return(Font_); }
		void SetSubParents(C_Window *);
		void SetDefaultFlags() { SetFlags(DefaultFlags_); }
		long GetDefaultFlags() { return(DefaultFlags_); }
		long CheckHotSpots(long relx,long rely);
		BOOL Process(long ID,short hittype);
		void Refresh();
		void Draw(SCREEN *surface,UI95_RECT *cliprect);

		void SetVUID(VU_ID id) { vuID=id; }
		VU_ID GetVUID() { return(vuID); }
};

#endif
