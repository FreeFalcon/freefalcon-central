
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

  Stuff for level-2 macros and other odd assorted stuff

********************************************************************/


#ifndef _SHI__XYZZY_H_
#define _SHI__XYZZY_H_


/****************************************************/
/*  SHI_XYZZY_CAT                                   */
/*                                                  */
/*  PARAMS: A - Front part                          */
/*          B - Back part                           */
/*                                                  */
/*  FUNCTIONS: This is for use in level-2 macros    */
/*  (since the ANSI-C preprocessor sucks.)  It      */
/*  simply concatenates A and B.                    */
/*                                                  */
/****************************************************/

#define SHI_XYZZY_CAT(A,B) A ## B



/****************************************************/
/*  SHI_XYZZY_CAT3                                  */
/*                                                  */
/*  PARAMS: A - Front part                          */
/*          B - Middle part                         */
/*          C - Back part                           */
/*                                                  */
/*  FUNCTIONS: This is for use in level-2 macros    */
/*  (since the ANSI-C preprocessor sucks.)  It      */
/*  simply concatenates A, B and C.                 */
/*                                                  */
/****************************************************/

#define SHI_XYZZY_CAT3(A,B,C) A ## B ## C



/****************************************************/
/*  SHI_XYZZY_CAT5                                  */
/*                                                  */
/*  PARAMS: A, B, C, D, E                           */
/*                                                  */
/*  FUNCTIONS: This is for use in level-2 macros    */
/*  (since the ANSI-C preprocessor sucks.)  It      */
/*  simply concatenates A-E                         */
/*                                                  */
/****************************************************/

#define SHI_XYZZY_CAT5(A,B,C,D,E) A ## B ## C ## D ## E 



/****************************************************/
/*  SHI_XYZZY_STRING                                */
/*                                                  */
/*  PARAMS: STR - Macro to make into string         */
/*                                                  */
/*  FUNCTIONS: This is for use forcing cpp to       */
/*  expand the macro 'STR' and then make it into    */
/*  a string.                                       */
/*                                                  */
/****************************************************/

#define SHI_XYZZY_STRING_L(X) #X
#define SHI_XYZZY_STRING(X)   SHI_XYZZY_STRING_L(X)



/****************************************************/
/*  SHI_TARGET_ID                                   */
/*                                                  */
/*  PARAMS: X - Name to insert                      */
/*                                                  */
/*  FUNCTIONS: This is creates an "ident" string    */
/*  $Configure: X - CHIPSET_ID_STR $                */
/*                                                  */
/*   SEE: ./chipsets/mint.doc                       */
/*                                                  */
/****************************************************/

#define SHI_TARGET_ID(X)  "$Configure: " #X " - " CHIPSET_ID_STR " $"


/****************************************************/
/*  SHI_COPYRIGHT_NOTICE                            */
/*                                                  */
/*  PARAMS: X - Year of copyright                   */
/*                                                  */
/*  FUNCTIONS: This is creates an "ident" string    */
/*  $Copyright: X MicroProse, Inc.  All rights      */
/*  reserved $                                      */
/*                                                  */
/****************************************************/

#define SHI_COPYRIGHT_NOTICE(X)  "$Copyright: " #X " MicroProse, Inc.  All rights reserved $"


#endif /* _SHI__XYZZY_H_ */




