#ifndef _TCP_H
#define _TCP_H
/* data structures for COMMS TCP interface */

#ifdef __cplusplus
extern "C"{
#endif

typedef struct tcpheader {
  unsigned short header_base;
  unsigned short size;
  unsigned short inv_size;
  /*int  header_size; */
} tcpHeader;


typedef struct comtcphandle {
  struct comapihandle apiheader;

  int buffer_size;
  WSABUF send_buffer;
  WSABUF recv_buffer;
  WSADATA wsaData;

  struct sockaddr_in Addr;

  SOCKET send_sock;
  SOCKET recv_sock;
  unsigned long sendmessagecount;
  unsigned long recvmessagecount;
  unsigned long sendwouldblockcount;
  unsigned long recvwouldblockcount;

  HANDLE lock;
  HANDLE ThreadHandle;
  short ThreadActive;
  short timeoutsecs;
  short handletype;
  short state;
  short ListenPort;
  int referencecount;
  int messagesize;
  int headersize;
  char *recv_buffer_start;
  int bytes_needed_for_header;
  long bytes_needed_for_message;
  int bytes_recvd_for_message;
  tcpHeader *Header;
  void (*connect_callback_func)(struct comapihandle *c,int retcode);
  void (*accept_callback_func)(struct comapihandle *c,int retcode);
  unsigned long timestamp;
} ComTCP;


#ifdef __cplusplus
}
#endif

#endif
