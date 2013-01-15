#ifndef _UI_QUEUE_H_
#define _UI_QUEUE_H_

typedef struct QStr QUEUEITEM;

enum
{
	_Q_NOTHING=0,
	_Q_SESSION_ADD_,
	_Q_SESSION_REMOVE_,
	_Q_SESSION_UPDATE_,
	_Q_GAME_ADD_,
	_Q_GAME_REMOVE_,
	_Q_GAME_UPDATE_,
};

struct QStr
{
	VU_ID SessionID;
	VU_ID GameID;
	short Type;
	QUEUEITEM *Next;
};

class CommsQueue
{
	private:
		HWND appwin_;
	public:
		QUEUEITEM *Root_;
		BOOL PostPending;

		CommsQueue()
		{
			Root_=NULL;
			appwin_=NULL;
			PostPending=FALSE;
		}
		~CommsQueue();

		void Setup(HWND hwnd);
		void Cleanup();
		void Add(short itemtype,VU_ID SessionID,VU_ID GameID);
		QUEUEITEM *Remove();
};

extern CommsQueue *gUICommsQ;
extern F4CSECTIONHANDLE* QueueCritical;

#endif