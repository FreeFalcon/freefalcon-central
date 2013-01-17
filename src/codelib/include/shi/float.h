
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

  Defines for "real" floating point types

  THIS FILE IS A WORK IN PROGRESS...AND NEEDS LOTS OF WORK

********************************************************************/


#ifndef _SHI__FLOAT_H_
#define _SHI__FLOAT_H_

/* Also use the compiler's floating-point defines */

#include <float.h>

/* Define the float type consistantly with the */
/* rest of the portable header-file system     */

typedef float       Float32;
typedef double      Float64;

/* Compile time percision macros */

#define TYPEDEF_FLOAT(NAME,BITS)     typedef SHI_XYZZY_CAT(Float,BITS)   NAME
#define TYPEDEF_FLOAT_PTR(NAME,BITS) typedef SHI_XYZZY_CAT(Float,BITS)*  NAME
#define TYPEDEF_FLOAT_HND(NAME,BITS) typedef SHI_XYZZY_CAT(Float,BITS)** NAME


/*  SEE: access.h  Same rules apply  */

#define SHI_FLOAT32_ACCESS(NUM) (*(Float32*)&NUM)
#define SHI_FLOAT64_ACCESS(NUM) (*(Float64*)&NUM)


#endif /* _SHI__FLOAT_H_ */


