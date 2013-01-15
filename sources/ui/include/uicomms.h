#ifndef _UI_COMMS_H_
#define _UI_COMMS_H_

enum
{
	_UI_TRACK_FLAG00=0x00000001,
	_UI_TRACK_FLAG01=0x00000002,
	_UI_TRACK_FLAG02=0x00000004,
	_UI_TRACK_FLAG03=0x00000008,
	_UI_TRACK_FLAG04=0x00000010,
	_UI_TRACK_FLAG05=0x00000020,
	_UI_TRACK_FLAG06=0x00000040,
	_UI_TRACK_FLAG07=0x00000080,
	_UI_TRACK_FLAG08=0x00000100,
	_UI_TRACK_FLAG09=0x00000200,
	_UI_TRACK_FLAG10=0x00000400,
	_UI_TRACK_FLAG11=0x00000800,
	_UI_TRACK_FLAG12=0x00001000,
	_UI_TRACK_FLAG13=0x00002000,
	_UI_TRACK_FLAG14=0x00004000,
	_UI_TRACK_FLAG15=0x00008000,
	_UI_TRACK_FLAG16=0x00010000,
	_UI_TRACK_FLAG17=0x00020000,
	_UI_TRACK_FLAG18=0x00040000,
	_UI_TRACK_FLAG19=0x00080000,
	_UI_TRACK_FLAG20=0x00100000,
	_UI_TRACK_FLAG21=0x00200000,
	_UI_TRACK_FLAG22=0x00400000,
	_UI_TRACK_FLAG23=0x00800000,
	_UI_TRACK_FLAG24=0x01000000,
	_UI_TRACK_FLAG25=0x02000000,
	_UI_TRACK_FLAG26=0x04000000,
	_UI_TRACK_FLAG27=0x08000000,
	_UI_TRACK_FLAG28=0x10000000,
	_UI_TRACK_FLAG29=0x20000000,
	_UI_TRACK_FLAG30=0x40000000,
	_UI_TRACK_FLAG31=0x80000000,
};

extern DWORD gUI_Tracking_Flag;

enum
{
	COMMS_UP_NEWGROUP=1,
	COMMS_UP_DELETEGROUP,
	COMMS_UP_UPDATEGROUP,
	COMMS_UP_NEWSESSION,
	COMMS_UP_DELETESESSION,
	COMMS_UP_UPDATESESSION,
};

enum // GAME Status Message Types
{
	GAME_WAITING=0,
	GAME_STARTED,
	GAME_OVER,
};

#include "f4comms.h"
#include "comdata.h"
//#include "iacomms.h"
#include "dfcomms.h"
//#include "tecomms.h"
#include "cpcomms.h"

#ifndef FALCSESS_H
#include "falcsess.h"
#endif

#include "ui\include\uihash.h"

#include "ui\include\logbook.h"
#include "ui\include\stats.h"

class CHATSTR
{
	public:
		VU_ID ID_;
		_TCHAR *Text_;

	public:
};

class UIComms
{
	private:
		_TCHAR			User_[64];
		BOOL			Online_;
		BOOL			Update_;
		char			Status_;
		char			InCampaign_;
		VU_ID			TargetGame_;		// The game we're interested in

		PlayerStats		*GameStats_;

		UI_Hash			*RemoteLogbooks_;

		void (*Callback_[game_MaxGameTypes])(short uptype,VU_ID game,VU_ID session);

	public:
		HWND			AppWnd_;

		UIComms();
		~UIComms();

		BOOL Setup(HWND hwnd);
		void SetCallback(int game,void (*rtn)(short,VU_ID,VU_ID));
		void StartComms(ComDataClass *ComData);
		void StartCommsDoneCB(int success);
		void StopComms();
		void Cleanup();
		BOOL LookAtGame(VuGameEntity *game);
		VuGameEntity* GetTargetGame();
		VU_ID GetTargetGameID() { return(TargetGame_); }
		void SetUserInfo();
		_TCHAR *GetUserInfo() { return(&User_[0]); }

		BOOL Online()					{ return(Online_); }

		void SetStatus(char stat)		{ Status_=stat; }
		char GetStatus()				{ return(Status_); }
		BOOL RemoteUpdate()				{ return(Update_); }
		void Updated()					{ Update_=FALSE; }
		VuGameEntity* GetGame()			{ return FalconLocalGame; }

		void *GetRemoteLB(long playerID);
		void SendLogbook(VU_ID requester);
		void SendImage(uchar type,VU_ID requester);
		void ReceiveLogbook(VU_ID from,LB_PILOT *pilot);
		void ReceiveImage(VU_ID from,uchar type,short sec,short blksize,long offset,long size,uchar *data);

		// Campaign Stuff
		void SetCampaignFlag(char gameType) { InCampaign_=gameType; }
		char InCampaign()				{ return(InCampaign_); }
		FalconSessionEntity *FindCampaignPlayer(VU_ID FlightID, uchar planeid);
		void SendGameStatus(uchar status);

		void SetStatsFile(char *filename);
		void SaveStats();
		void LoadStats();
		void UpdateGameIter();
		void RemoteUpdateIter(long newIter);
};

extern UIComms *gCommsMgr;
#endif // _COMMS_UI_H_
