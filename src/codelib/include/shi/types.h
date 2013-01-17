
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


#ifndef _SHI__TYPES_H_
#define _SHI__TYPES_H_


/****************************************************/
/*  SHI_TYPE_ERROR                                  */
/*                                                  */
/*  PARAMS: ERR_STR  - Error string                 */
/*          ALT_TYPE - Alternate type to use        */
/*                                                  */
/*  FUNCTION: Wrapper for unsupported types.  If    */
/*  STRICT_TYPES is defined, use of an unsupported  */
/*  type will stop compliation, otherwise the       */
/*  Alternate type will be used.                    */
/*                                                  */
/****************************************************/

#if defined(STRICT_TYPES)
#define SHI_TYPE_ERROR(ERR_STR,ALT_TYPE) "Error: ERR_STR"
#else
#define SHI_TYPE_ERROR(ERR_STR,ALT_TYPE) ALT_TYPE
#endif



/****************************************************/
/*  GET_NUM_BITS(TYPE)                              */
/*                                                  */
/*  PARAMS: TYPE                                    */
/*                                                  */
/*  FUNCTIONS: Returns the number of bit in TYPE    */
/*                                                  */
/****************************************************/

#define BITS_PER_BYTE 8		/* Currently true for all */

#define GET_NUM_BITS(TYPE) (BITS_PER_BYTE*sizeof(TYPE))


#endif /* _SHI__TYPES_H_  */












