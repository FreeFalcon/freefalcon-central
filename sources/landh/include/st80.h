/*
 *   Project:		StreamTalk(TM)80 (8000bps,8kHz) codec
 *   Workfile:		st80.h
 *   Authors:		Alfred Wiesen, Luc Boschmans
 *   Created:		30 August 1995
 *   Last update:	29 May 1996
 *   DLL Version:	3.00
 *   Revision:          StreamTalk Product Line.
 *   Comment:
 *
 *	(C) Copyright 1993-96 Lernout & Hauspie Speech Products N.V. (TM)
 *	All rights reserved. Company confidential.
 */

# ifndef __ST80_H  /* avoid multiple include */

# define __ST80_H

/*
 *  Type definition for the L&H functions returned values
 */

typedef DWORD LH_ERRCODE;

typedef struct CodecInfo_tag {
   WORD wPCMBufferSize;
   WORD wCodedBufferSize;
   WORD wBitsPerSamplePCM;
   DWORD dwSampleRate;
   WORD wFormatSubTag;
   char wFormatSubTagName[40];
   DWORD dwDLLVersion;
} CODECINFO, near *PCODECINFO, far *LPCODECINFO;

typedef struct CodecInfoEx_tag {
   WORD		wFormatSubTag;
   char		szFormatSubTagName[80];
   BOOL 	bIsVariableBitRate;
   BOOL 	bIsRealTimeEncoding;
   BOOL 	bIsRealTimeDecoding;
   BOOL 	bIsFloatingPoint;
   WORD		wInputDataFormat;
   DWORD 	dwInputSampleRate;
   WORD 	wInputBitsPerSample;
   DWORD 	nAvgBytesPerSec;
   WORD 	wInputBufferSize;
   WORD 	wCodedBufferSize;
   WORD 	wMinimumCodedBufferSize;
   DWORD 	dwDLLVersion;
   WORD 	cbSize;  // size of extra data, not declared here.
} CODECINFOEX, near *PCODECINFOEX, far *LPCODECINFOEX;



/*
 *  Possible values for the LH_ERRCODE type
 */

# define LH_SUCCESS (0)    /* everything is OK */
# define LH_EFAILURE (-1)  /* something went wrong */
# define LH_EBADARG (-2)   /* one of the given argument is incorrect */
# define LH_BADHANDLE (-3) /* bad handle passed to function */

/*
 *  Possible flags for the dwFlags parameter of Open_Coder
 */

# define LINEAR_PCM_16_BIT	0x0001
# define LINEAR_PCM_8_BIT	0x0002
# define ADPCM_G721		0x0004
# define MULAW			0x0008
# define ALAW			0x0010

/*
 *  Some real types are defined here
 */

# ifdef __cplusplus
	# define LH_PREFIX extern "C"
# else
	# define LH_PREFIX
# endif

# define LH_SUFFIX FAR PASCAL



LH_PREFIX HANDLE LH_SUFFIX
	ST80_Open_Coder( DWORD dwFlags );

LH_PREFIX LH_ERRCODE LH_SUFFIX
	ST80_Encode(
  HANDLE hAccess,
  LPBYTE inputBufferPtr,
  LPWORD inputBufferLength,
  LPBYTE outputBufferPtr,
  LPWORD outputBufferLength
  );

LH_PREFIX LH_ERRCODE LH_SUFFIX
	ST80_Close_Coder( HANDLE hAccess);

LH_PREFIX HANDLE LH_SUFFIX
	ST80_Open_Decoder( DWORD dwFlags );

LH_PREFIX LH_ERRCODE LH_SUFFIX
	ST80_Decode(
  HANDLE hAccess,
  LPBYTE inputBufferPtr,
  LPWORD inputBufferLength,
  LPBYTE outputBufferPtr,
  LPWORD outputBufferLength
  );

LH_PREFIX LH_ERRCODE LH_SUFFIX
	ST80_Close_Decoder( HANDLE hAccess);

LH_PREFIX void LH_SUFFIX
	ST80_GetCodecInfo(LPCODECINFO lpCodecInfo);

LH_PREFIX void LH_SUFFIX
	ST80_GetCodecInfoEx(LPCODECINFOEX lpCodecInfoEx,DWORD cbSize);

# endif  /* avoid multiple include */

