#ifndef  _AVIFILE_
#define  _AVIFILE_

#include "io.h"
#include "fcntl.h"

#ifdef   __cplusplus
extern   "C"
{
#endif

#define  AVI_OPEN(a,b)     _open(a,b)
#define  AVI_CLOSE(a)      _close(a)
#define  AVI_SEEK(a,b,c)   _lseek(a,b,c)
#define  AVI_READ(a,b,c)   _read(a,b,c)   

#ifdef   __cplusplus
}
#endif

#endif   /* _AVIFILE_ */