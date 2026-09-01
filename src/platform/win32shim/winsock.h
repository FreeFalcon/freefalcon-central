// Artscout - 2026 (#104, Linux Ф1): <winsock.h> -> BSD sockets. Winsock IS the BSD API with a thin Microsoft accent,
// so this is a genuine mapping, not a stub: socket/bind/connect/send/recv and sockaddr_in are the same calls and the
// same struct underneath. What actually differs is spelled out below -- the handful of places Microsoft diverged.
#ifndef FF_WIN32SHIM_WINSOCK_H
#define FF_WIN32SHIM_WINSOCK_H

// WSADATA/WSAStartup below are spelled in terms of WORD and BYTE, so this header needs the Win32 base types. The real
// <winsock.h> pulls them itself (via <windef.h>) and is self-contained; this one was not, so the comms sources that
// include <winsock.h> with no prior <windows.h> (udp.c, rudp.c, capi.c, ws2init.c, ComList.cpp) stopped at "unknown
// type name 'WORD'". No cycle here: windows.h does not include winsock.h.
#include <windows.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>

// A Windows SOCKET is an opaque handle whose invalid value is ~0; a POSIX socket is an int fd whose failure value is
// -1. Keeping SOCKET signed is what makes the codebase's `if (s == INVALID_SOCKET)` and `if (s < 0)` both correct.
typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)

// Winsock closes with closesocket() and reports through WSAGetLastError(); POSIX uses close() and errno. These are
// real inline functions (not macros) so they can also be taken by address -- ws2init.c binds CAPI_closesocket etc. to
// them exactly as it binds CAPI_WSAStartup to WSAStartup. Signatures match the WSFN_* typedefs in wsprotos.h.
inline int closesocket(SOCKET s)
{
    return ::close(s);
}
inline int WSAGetLastError(void)
{
    return errno;
}
inline void WSASetLastError(int e)
{
    errno = e;
}
#define WSAEWOULDBLOCK EWOULDBLOCK
#define WSAEINPROGRESS EINPROGRESS
#define WSAECONNRESET ECONNRESET
#define WSAECONNREFUSED ECONNREFUSED
#define WSAECONNABORTED ECONNABORTED
#define WSAENOTCONN ENOTCONN
#define WSAETIMEDOUT ETIMEDOUT
#define WSAEADDRINUSE EADDRINUSE
#define WSAEINTR EINTR
#define WSAEMSGSIZE EMSGSIZE
#define WSAENOTSOCK ENOTSOCK
#define WSAENETDOWN ENETDOWN
#define WSAENETRESET ENETRESET
#define WSAEINVAL EINVAL
#define WSAEFAULT EFAULT
#define WSAEISCONN EISCONN
#define WSAEALREADY EALREADY
#define WSAESHUTDOWN ESHUTDOWN
#define WSAEOPNOTSUPP EOPNOTSUPP
#define WSAEADDRNOTAVAIL EADDRNOTAVAIL
#define WSAEAFNOSUPPORT EAFNOSUPPORT
#define WSAEHOSTUNREACH EHOSTUNREACH
#define WSAENETUNREACH ENETUNREACH
// WSANOTINITIALISED means "WSAStartup was never called" -- a Winsock-lifecycle error with no POSIX errno equivalent.
// The comms code only ever switches on it (never on Linux does it occur, since our WSAStartup can't fail), so it just
// needs a compile-time value distinct from every real errno; use its genuine Windows numeric value (10093), which is
// far above the POSIX errno range and so cannot collide with another `case` in the same switch.
#define WSANOTINITIALISED (10093)

// ioctlsocket(FIONBIO) is the Windows way to set non-blocking; ioctl(FIONBIO) is the POSIX equivalent.
inline int ioctlsocket(SOCKET s, long cmd, unsigned long* argp)
{
    return ::ioctl(s, (unsigned long)cmd, argp);
}

// WSAStartup/WSACleanup exist because Winsock needs explicit library init. BSD sockets do not -- so these honestly
// succeed with nothing to do, rather than pretending to initialise something.
typedef struct WSAData
{
    WORD wVersion, wHighVersion;
    char szDescription[257];
    char szSystemStatus[129];
    unsigned short iMaxSockets, iMaxUdpDg;
    char* lpVendorInfo;
} WSADATA, *LPWSADATA;
#define MAKEWORD_WS(lo, hi) ((WORD)(((BYTE)(lo)) | (((WORD)((BYTE)(hi))) << 8)))
inline int WSAStartup(WORD wVersionRequested, LPWSADATA lpWSAData)
{
    if (lpWSAData)
    {
        std::memset(lpWSAData, 0, sizeof(*lpWSAData));
        lpWSAData->wVersion = wVersionRequested;
        lpWSAData->wHighVersion = wVersionRequested;
        std::strcpy(lpWSAData->szDescription, "BSD sockets (win32shim)");
    }
    return 0;
}
inline int WSACleanup(void)
{
    return 0;
}

typedef struct hostent HOSTENT, *LPHOSTENT;
typedef struct servent SERVENT, *LPSERVENT;
typedef struct sockaddr SOCKADDR, *LPSOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN, *LPSOCKADDR_IN;
typedef struct in_addr IN_ADDR, *LPIN_ADDR;
typedef struct timeval TIMEVAL, *LPTIMEVAL;
// No FD_SET_T here. It was "typedef struct fd_set FD_SET_T;", which does not compile: glibc declares fd_set as a
// typedef of an ANONYMOUS struct (sys/select.h:70), so there is no `struct fd_set` tag to reference. Nothing in the
// codebase named FD_SET_T anyway -- the select() users take glibc's fd_set and FD_SET() as-is (transprt.cpp:107).
// It stayed hidden until the WORD fix above let the compiler reach this line.

#endif // FF_WIN32SHIM_WINSOCK_H
