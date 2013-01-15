/* protofilt.c - Copyright (c) Fri Dec 06 23:34:35 1996,  Spectrum HoloByte, Inc.  All Rights Reserved */
#include <winsock2.h>

#pragma optimize( "", off ) // JB 010718

/*++

Routine Description:

    Returns true if Proto is suitable for use by Chat.

Arguments:

    Proto -- Points to a protocol information struct.

Return Value:

    TRUE -- We likes it.

    FALSE -- Get this chintzy protocol out of here!

--*/
int UseProtocol(IN LPWSAPROTOCOL_INFO Proto)
{
  if(!
     (Proto->dwServiceFlags1 & XP1_CONNECTIONLESS) /*&&*/
     /*(Proto->dwServiceFlags1 & XP1_MULTIPOINT_DATA_PLANE) &&*/
     /*(Proto->dwServiceFlags1 & XP1_SUPPORT_MULTIPOINT)*/
     )
    {
      return 1;
    
    }
  else
    {
      return 0;
    }
}
