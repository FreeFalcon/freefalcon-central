
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

  shi/access.h - Defines macros for accessing memory as a certain
  size regardless of the type of an identifier.

  WARNING:  These macros do not handle alignment issues.  This is
  the engineer's responsablity.  This file is not included by
  default because their use indicates knowledge of memory packing
  and placement.  Therefore there are inherient barriers to
  portablity in their use.  If needed, careful white-box testing
  of all cases should be implemented to insure correct behavior.

********************************************************************/


#ifndef _SHI__ACCESS_H_
#define _SHI__ACCESS_H_


/****************************************************/
/*  SHI_INT_ACCESS(ID,SIZE)                         */
/*                                                  */
/*  PARAMS: ID   - Address to access                */
/*          SIZE - Number of bits to access         */
/*                                                  */
/*  FUNCTIONS: Access a specific memory location    */
/*  (ID) and treat it as a integer of SIZE bits     */
/*                                                  */
/****************************************************/

#define SHI_INT_ACCESS(ID,SIZE) *((SHI_XYZZY_CAT(UInt,SIZE) *) &(ID))


/****************************************************/
/*  SHI_INT{8,16,...}_ACCESS                        */
/*                                                  */
/*  PARAMS: ID - Starting address                   */
/*                                                  */
/*  FUNCTIONS: Access for specific sizes.           */
/*                                                  */
/****************************************************/

#define SHI_INT8_ACCESS(ID)    SHI_INT_ACCESS(ID,8)
#define SHI_INT16_ACCESS(ID)   SHI_INT_ACCESS(ID,16)
#define SHI_INT32_ACCESS(ID)   SHI_INT_ACCESS(ID,32)
#define SHI_INT64_ACCESS(ID)   SHI_INT_ACCESS(ID,64)
#define SHI_INT128_ACCESS(ID)  SHI_INT_ACCESS(ID,128)
#define SHI_INT256_ACCESS(ID)  SHI_INT_ACCESS(ID,256)


#endif /* _SHI__ACCESS_H_ */


