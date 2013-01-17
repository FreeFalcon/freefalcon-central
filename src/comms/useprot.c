/* useprot.c - Copyright (c) Fri Dec 06 23:56:57 1996,  Spectrum HoloByte, Inc.  All Rights Reserved */
#include <winsock2.h>

#pragma optimize( "", off ) // JB 010718

int UseProtocol(IN LPWSAPROTOCOL_INFO Proto)
{
  if (!
      (Proto->dwServiceFlags1 & XP1_CONNECTIONLESS) &&
      (Proto->dwServiceFlags1 & XP1_CONNECTIONLESS) &&
      (Proto->dwServiceFlags1 & XP1_GUARANTEED_DELIVERY) &&
      (Proto->dwServiceFlags1 & XP1_GUARANTEED_ORDER))
    {
    
        returnTRUE;
    
    } else {
        
        return FALSE;
    }
} // UseProtocol
