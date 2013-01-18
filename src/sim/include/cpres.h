#include <stdio.h>

// ALL RESMGR CODE ADDITIONS START HERE
#define _CP_USE_RES_MGR_

#ifndef _CP_USE_RES_MGR_ // DON'T USE RESMGR

#define CP_HANDLE FILE
#define CP_OPEN   fopen
#define CP_READ   fread
#define CP_CLOSE  fclose
#define CP_SEEK   fseek
#define CP_TELL   ftell

#else // USE RESMGR

#include "cmpclass.h"
extern "C"
{
#include "codelib/resources/reslib/src/resmgr.h"
}

#define CP_HANDLE FILE
#define CP_OPEN   RES_FOPEN
#define CP_READ   RES_FREAD
#define CP_CLOSE  RES_FCLOSE
#define CP_SEEK   RES_FSEEK
#define CP_TELL   RES_FTELL

#endif
