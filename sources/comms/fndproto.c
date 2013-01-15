/* fndproto.c - Copyright (c) Fri Dec 06 22:51:24 1996,  Spectrum HoloByte, Inc.  All Rights Reserved */
#include <winsock2.h>

#pragma optimize( "", off ) // JB 010718

/*++

Routine Description:

    Finds out about all transport protocols installed on the local
    machine and saves.

Implementation:

    This function uses WSAEnumProtocols to find out about all
    installed protocols on the local machine.  It stores this
    information in two variables which are global to this file;
    InstalledProtocols is a pointer to a buffer of WSAPROTOCOL_INFO
    structs, while NumProtocols is the number of protocols in that
    buffer.

Arguments:

    None.

Return Value:

    TRUE - Successfully initialized the protocol buffer.

    FALSE - Some kind of problem arose.  The user is informed of the
    error.

--*/
int FindProtocols(LPWSAPROTOCOL_INFO *InstalledProtocols, int *NumProtocols)
{
  DWORD BufferSize = 0;       /* size of InstalledProtocols buffer */
    

  /* Call WSAEnumProtocols to figure out how big of a buffer we need. */
  *NumProtocols = WSAEnumProtocols(NULL,
                                   NULL,
                                   &BufferSize);

  if((*NumProtocols != SOCKET_ERROR) && (WSAGetLastError() != WSAENOBUFS))
    {
      /* Were in trouble!! */
      /* MessageBox(GlobalFrameWindow, "WSAEnumProtocols is broken.", "Error", 
         MB_OK | MB_ICONSTOP | MB_SETFOREGROUND); */
      goto Fail;
    }
    
  /* Allocate a buffer, call WSAEnumProtocols to get an array of
     WSAPROTOCOL_INFO structs. */
  *InstalledProtocols = (LPWSAPROTOCOL_INFO)malloc(BufferSize);
  if(*InstalledProtocols == NULL)
    {
      /*
        MessageBox(GlobalFrameWindow, "malloc failed.", "Error", 
        MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        */
        goto Fail;
    }
  *NumProtocols = WSAEnumProtocols(NULL,
                                   (LPVOID)*InstalledProtocols,
                                   &BufferSize);
  if(*NumProtocols == SOCKET_ERROR)
    {
      /* uh-oh */
      /*
        wsprintf(MsgText, "WSAEnumProtocols failed.  Error Code: %d",
        WSAGetLastError());
        MessageBox(GlobalFrameWindow, MsgText, "Error", 
        MB_OK | MB_ICONSTOP | MB_SETFOREGROUND);
        */
      goto Fail;
    }
  return 1;

Fail:

  WSACleanup();
  return 0;
}
