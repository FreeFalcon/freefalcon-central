// This should go -- Use SHIerror.h instead.  SCR 7/26/97

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


********************************************************************/


#ifndef _SHI__ASSERT_H_
#define _SHI__ASSERT_H_


/* Make defining of DEBUG and _DEBUG automagic */
/* If one is defined, the both are defined     */

#if defined(DEBUG)||defined(_DEBUG)
#ifndef DEBUG
#define DEBUG
#endif
#ifndef _DEBUG
#define _DEBUG
#endif
#endif


#include "../../codelib/include/shi/chipsets/m_i486/massert.h"



/****************************************************/
/*  SHI_DEBUG_LINE(LINE)                            */
/*                                                  */
/*  PARAMS: LINE - "C" to include                   */
/*                                                  */
/*  FUNCTIONS: Includes LINE if DEBUG is defined,   */
/*  otherwise nothing.  This is equivalent to:      */
/*                                                  */
/*    #ifdef DEBUG                                  */
/*    LINE;                                         */
/*    #endif                                        */
/*                                                  */
/*  Thus is only useful in increasing (hopefully)   */
/*  the readability of a code segment.              */
/*                                                  */
/****************************************************/

#ifdef DEBUG
#define SHI_DEBUG_LINE(LINE) LINE
#else

#if defined(SHI_NO_EXTRA_SEMICOLONS)
#define SHI_DEBUG_LINE(LINE) ((void)0)  /* some compilers don't like extra semi-colons */
#else
#define SHI_DEBUG_LINE(LINE)
#endif

#endif



/****************************************************/
/*  SHI_ASSERT                                      */
/*                                                  */
/*  PARAMS: X - Truth to assert                     */
/*          Y - Debug string (not always used)      */
/*                                                  */
/*  FUNCTIONS: Exact function is machine dependant. */ 
/*  If no macro is defined in the machine specific  */
/*  assert.h, we attempt to generate a memory       */
/*  exception.                                      */
/*                                                  */
/****************************************************/

#ifndef SHI_ASSERT

#ifndef DEBUG
#define SHI_ASSERT(X,Y)
#else
#define SHI_ASSERT(X,Y) if (!(X)) { *((unsigned int *) 0x00) = 0; }
#endif

#else 

#ifndef DEBUG
#undef  SHI_ASSERT
#define SHI_ASSERT(X,Y)
#endif

#endif


/****************************************************/
/*  SHI_PRINTF                                      */
/*                                                  */
/*  PARAMS: X - standard printf parameters          */
/*                                                  */
/*  FUNCTIONS: Wrapper a debugging printf.          */ 
/*                                                  */
/****************************************************/

#if 0
#if (!defined(SHI_PRINTF) || !defined(DEBUG))
#undef  SHI_PRINTF
#define SHI_PRINTF(X)
#endif
#endif

#endif  /* _SHI__ASSERT_H_ */



















