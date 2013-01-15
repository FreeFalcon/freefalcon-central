#pragma optimize( "", off )

#include <stdio.h>
#include <winsock2.h>


DWORD GetWinsock2Version(DWORD *major, DWORD *minor, DWORD *release);


main(int argc, char **argv)
{
  DWORD version, minor, release;
 
  printf("MicroProse: ");
  if (GetWinsock2Version(&version,&minor, &release))
  {
    printf("Winsock Version %d.%d",version,minor);
    if(release) printf("  (RELEASE)\n");
	 else        printf("  (BETA)\n");
  }
  else
    printf("Winsock2 not Installed\n");


return 0;
}

/************************************************************ 
   Returns: Version number if Winsock 2 is installed 
            0 if NOT installed
		
   Also returns in parameters
            *major = Major version
            *minor = Minor version, 
            *release  = 1 if RELEASE version 
                      = 0 if BETA   version
**************************************************************/

DWORD GetWinsock2Version(DWORD *major, DWORD *minor, DWORD *release)
{
  int len, size;
  SOCKET socket;
  int wsaStatus;
  WSADATA wsaData;
  DWORD buflen=0;


  *major = *minor = *release = 0;

  buflen = SearchPath(NULL, "WS2_32.DLL", NULL,0,NULL,NULL);	

  if(buflen == 0)
	  return 0;


  if(wsaStatus=WSAStartup(MAKEWORD(2,0), &wsaData))  
  {
      return 0;
  }
  else
  {
	  *major = LOBYTE(wsaData.wVersion);
	  *minor = HIBYTE(wsaData.wVersion);
  }



   socket = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, 0, 0, 0);

   /* inquire MAX Buffer size for UDP */
   /* BETA version of Winsock dll returns size = 0 for this inquiry */

   len = sizeof(size);

   getsockopt(socket,SOL_SOCKET,SO_MAX_MSG_SIZE,(char *)&size,&len);

   closesocket(socket);
  
   WSACleanup();



   if (size) *release = 1;

  
   return *major;
}
