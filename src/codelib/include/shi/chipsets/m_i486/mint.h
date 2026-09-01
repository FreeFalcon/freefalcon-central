
/********************************************************************/
/*  Copyright (C) 1997 MicroProse, Inc. All rights reserved         */
/*                                                                  */
/*  Programs, statements and coded instructions within this file    */
/*  contain unpublished and proprietary information of MicroProse,  */
/*  Inc. and are thus protected by the Federal and International    */
/*  Copyright laws. They may not be copied, duplicated or disclosed */
/*  to third parties in any form, in whole or in part, without the  */
/*  prior written consent of MicroProse, Inc.                       */
/*                                                                  */
/********************************************************************/

#ifndef _MINT_H_
#define _MINT_H_

#define CHIPSET_ID SHI_CS_m_i486
#define CHIPSET_ID_STR "Intel i486"
#define CHIPSET_ID_NUM 486

#define CHIPSET_MAX_INT_POW 2     /* See MINT.DOC */

#define CHIPSET_LITTLE_ENDIAN

typedef signed char Int8;
typedef short Int16;

typedef unsigned char UInt8;
typedef unsigned short UInt16;

/* Int32/UInt32 promise exactly 32 bits, and `long` delivers that only under ILP32/LLP64 -- i.e. on the Win32 and
 * Win64 this was written for. Under Linux's LP64 `long` is 64 bits, so the old unconditional `typedef unsigned long
 * UInt32` made a type whose name lied about its width: it silently doubled every struct laid over a file format and
 * mismatched the 32-bit DWORD (Tmap.cpp:267 hands &UInt32 to ReadFile's LPDWORD, which stopped compiling).
 *
 * Windows keeps `long` deliberately, and this is NOT cosmetic: MSVC's DWORD is `unsigned long`, and `unsigned long*`
 * and `unsigned int*` are distinct types there even though both are 32-bit. Switching Windows to uint32_t would break
 * every ReadFile(..., &UInt32var, ...) it currently compiles. So the split is by data model, not by OS taste: each
 * side names the 32-bit type that its own DWORD agrees with. */
#if defined(_WIN32)
typedef long Int32;
typedef unsigned long UInt32;
#else
#include <stdint.h>
typedef int32_t Int32;
typedef uint32_t UInt32;
#endif

#endif /* _MINT_H_ */
