#ifndef _UI_LOGBOOK_
#define _UI_LOGBOOK_

const int MODIFIED	= 0x01;
const int EDITABLE	= 0x02;
const int OPPONENT	= 0x04;

/*
typedef struct
{
	int flags;
}UI_LOG;

extern UI_LOG LogState;
*/
extern int LogState;
#endif