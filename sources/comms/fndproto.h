/* fndproto.h - Copyright (c) Fri Dec 06 22:52:10 1996,  Spectrum HoloByte, Inc.  All Rights Reserved */
#ifndef _FNDPROTO_H_
#define _FNDPROTO_H_

#include <winsock2.h>

#ifdef __cplusplus
extern "C" {
#endif


int FindProtocols(LPWSAPROTOCOL_INFO *InstalledProtocols, int *NumProtocols);


#ifdef __cplusplus
}
#endif
#endif
