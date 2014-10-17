
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

/********************************************************************

   Generic integer macros and defines are made within this file.
   Machine specific items are in ./machine/${TARGET}/mint.h

********************************************************************/

#ifndef _SHI__INT_H_
#define _SHI__INT_H_


/**  Include the chipset specific typedefs and defines  **/

#include "../../codelib/include/shi/chipsets/m_i486/mint.h"


/********************************************************************/


/* CHIPSET_INT_POW is only defined in mint.h files  */
/* where the value differs from CHIPSET_MAX_INT_POW */

#ifndef CHIPSET_INT_POW
#define CHIPSET_INT_POW CHIPSET_MAX_INT_POW
#endif


/* CHIPSET_MAX_INT_DEF_POW is only defined in mint.h files  */
/* where the value differs from CHIPSET_MAX_INT_POW         */

#ifndef CHIPSET_MAX_INT_DEF_POW
#define CHIPSET_MAX_INT_DEF_POW CHIPSET_MAX_INT_POW
#endif


/* CHIPSET_BYTE_POW */

#ifndef CHIPSET_BYTE_POW
#define CHIPSET_BYTE_POW 3
#endif

/********************************************************************/

/* Derived macros from mint.h */

#define NUM_BYTES_INT         (0x01 << CHIPSET_INT_POW)
#define NUM_BYTES_MAX_INT     (0x01 << CHIPSET_MAX_INT_POW)
#define NUM_BTYES_MAX_DEF_INT (0x01 << CHIPSET_MAX_INT_DEF_POW)

#define NUM_BITS_INT          (0x01 << (CHIPSET_BYTE_POW + CHIPSET_INT_POW))
#define NUM_BITS_MAX_INT      (0x01 << (CHIPSET_BYTE_POW + CHIPSET_MAX_INT_POW))
#define NUM_BITS_MAX_DEF_INT  (0x01 << (CHIPSET_BYTE_POW + CHIPSET_MAX_INT_DEF_POW ))

/********************************************************************/


/****************************************************/
/*  GET_U?INT_MIN(BITS)                             */
/*  GET_U?INT_MAX(BITS)                             */
/*                                                  */
/*  PARAMS: BITS - Number of bits in the integer    */
/*                                                  */
/*  FUNCTIONS: Returns the minimum or maximum       */
/*  storable value in an integer with BITS bits.    */
/*                                                  */
/****************************************************/

#define GET_UINT_MIN(BITS)  (0)
#define GET_UINT_MAX(BITS)  ((1<<(BITS))-1)
#define GET_INT_MIN(BITS)   (-(1<<((BITS)-1)))
#define GET_INT_MAX(BITS)   ((1<<((BITS)-1))-1)



/****************************************************/
/*  INT[n]_MIN                                      */
/*  U?INT[n]_MAX                                    */
/*                                                  */
/*  Define the minimum and maximum integer values   */
/*  that can be contained within an integer with    */
/*  "n" bits of percision.                          */
/*                                                  */
/****************************************************/

//#define INT8_MIN      (-128)
//#define INT8_MAX      127
//#define UINT8_MAX     255U

//#define INT16_MIN     (-32768)
//#define INT16_MAX     32767
//#define UINT16_MAX    65535U

//#define INT32_MIN     (-2147483648)
//#define INT32_MAX     2147483647
//#define UINT32_MAX    4294967295U

#if (NUM_BITS_MAX_DEF_INT >= 64)
#define INT64_MIN     (-9223372036854775808LL)
#define INT64_MAX     9223372036854775807LL
#define UINT64_MAX    18446744073709551615LLU
#endif


/****************************************************/
/*  TYPEDEF_U?INT(NAME,BITS)                        */
/*                                                  */
/*  PARAMS: NAME - Name to typedef                  */
/*          BITS - Number of bits in the integer    */
/*                                                  */
/*  Generates a typedef for NAME of an integer      */
/*  with BITS number of bits.  If BITS is a goofy   */
/*  number, you will get strange errors.            */
/*                                                  */
/*  This is intended to allow the user to make a    */
/*  define that specifies the number of bits        */
/*  required for a value, then use that define      */
/*  within TYPEDEF_INT to make the typedef.  This   */
/*  way only one value needs updating.              */
/*                                                  */
/****************************************************/

#define TYPEDEF_INT(NAME,BITS)   typedef SHI_XYZZY_CAT(Int,BITS)  NAME
#define TYPEDEF_UINT(NAME,BITS)  typedef SHI_XYZZY_CAT(UInt,BITS) NAME


/****************************************************/
/*  TYPEDEF_U?INT_PTR(NAME,BITS)                    */
/*                                                  */
/*  PARAMS: NAME - Name to typedef                  */
/*          BITS - Number of bits in integer to     */
/*                 which the pointer points         */
/*                                                  */
/*  FUNCTION: This is equivalent to -               */
/*                                                  */
/*    typedef Int${BITS}* ${NAME}                   */
/*                                                  */
/****************************************************/

#define TYPEDEF_INT_PTR(NAME,BITS)   typedef SHI_XYZZY_CAT(Int,BITS) *NAME
#define TYPEDEF_UINT_PTR(NAME,BITS)  typedef SHI_XYZZY_CAT(UInt,BITS)*NAME


/****************************************************/
/*  TYPEDEF_U?INT_HND(NAME,BITS)                    */
/*                                                  */
/*  PARAMS: NAME - Name to typedef                  */
/*          BITS - Number of bits in integer to     */
/*                 which the handle refers          */
/*                                                  */
/*  FUNCTION: This is equivalent to -               */
/*                                                  */
/*    typedef Int${BITS}** ${NAME}                  */
/*                                                  */
/****************************************************/

#define TYPEDEF_INT_HND(NAME,BITS)   typedef SHI_XYZZY_CAT3(Int,BITS)  **NAME
#define TYPEDEF_UINT_HND(NAME,BITS)  typedef SHI_XYZZY_CAT3(UInt,BITS) **NAME


typedef int                   Int;
typedef unsigned int          UInt;

#if (NUM_BITS_MAX_DEF_INT >= 64)

typedef unsigned Int64   UInt64;

#if (NUM_BITS_MAX_DEF_INT >= 128)

typedef unsigned Int128  UInt128;

#else

#define Int128   SHI_TYPE_ERROR("No 128-bit integers", Int64)
#define UInt128  SHI_TYPE_ERROR("No 128-bit integers", UInt64)

#endif

#define Int64   SHI_TYPE_ERROR("No 64-bit integers", Int32)
#define UInt64  SHI_TYPE_ERROR("No 64-bit integers", UInt32)

#endif

#endif /* _SHI__INT_H_ */






