#ifndef __MSGID_H__
#define __MSGID_H__ 

const int MSG_EXIT				= -10;
const int MSG_HELP				= -11;
const int MSG_SHOW_PHRASES		= -12;

// These are the AWACS commands
const int AWACS_COMMANDS		= 1;

// These are the ATC commands
const int ATC_COMMANDS			= 41;

// These are the Wingmen commands
const int WINGMEN_COMMANDS		= 81;

// This must be larger than the last command
const int LAST_COMMAND			= 999;

#endif