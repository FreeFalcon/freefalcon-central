#ifndef CAPIOPT_H
#define CAPIOPT_H

/*  Application defined data, etc */

/* comment/uncomment this as wished */
/* #define DEBUG_COMMS  */

#define LOAD_DLLS

//#define capi_DEBUG
//#define DEBUG_RECEIVE
//#define DEBUG_SEND

//#define capi_NET_DEBUG_FEATURES

// compression stuff
#include "utils/Lzss.h"
#define ComAPICompress LZSS_Compress
#define ComAPIDecompress LZSS_Expand


#endif
