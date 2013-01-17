#include <windows.h>
#include "f4thread.h"
#include "vu2.h"
#include <queue.h>
#include "falcuser.h"
#include "debuggr.h"

CommsQueue *gUICommsQ=NULL;

/* static char *QueueType[]=
{
	"_Q_NOTHING_",
	"_Q_SESSION_ADD_",
	"_Q_SESSION_REMOVE_",
	"_Q_SESSION_UPDATE_",
	"_Q_GAME_ADD_",
	"_Q_GAME_REMOVE_",
	"_Q_GAME_UPDATE_",
}; */

F4CSECTIONHANDLE* QueueCritical=NULL;

CommsQueue::~CommsQueue()
{
}

void CommsQueue::Setup(HWND hwnd)
{
	appwin_=hwnd;
	PostPending=FALSE;
	QueueCritical = F4CreateCriticalSection("QueueCritical");
}

void CommsQueue::Cleanup()
{
	F4EnterCriticalSection(QueueCritical);
	while(Root_)
		Remove();
	F4LeaveCriticalSection(QueueCritical);
	F4DestroyCriticalSection(QueueCritical);
	QueueCritical = NULL; // JB 010108
}

void CommsQueue::Add(short itemtype,VU_ID SessionID,VU_ID GameID)
{
	QUEUEITEM *cur,*q;

	// MonoPrint("%s\n",QueueType[itemtype]);

	F4EnterCriticalSection(QueueCritical);
	q=new QUEUEITEM;
	q->Type=itemtype;
	q->SessionID=SessionID;
	q->GameID=GameID;
	q->Next=NULL;

	if(!Root_)
		Root_=q;
	else
	{
		cur=Root_;
		while(cur->Next)
			cur=cur->Next;
		cur->Next=q;
	}
	if(!PostPending)
	{
		PostMessage(appwin_,FM_UI_UPDATE_GAMELIST,0,0);
		PostPending=TRUE;
	}
	F4LeaveCriticalSection(QueueCritical);

}

QUEUEITEM *CommsQueue::Remove()
{
	QUEUEITEM *dl;

	F4EnterCriticalSection(QueueCritical);
	dl=Root_;
	Root_=Root_->Next;
	delete dl;
	if(!Root_)
		PostPending=FALSE;
	F4LeaveCriticalSection(QueueCritical);
	return(Root_);
}
