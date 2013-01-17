
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


#ifndef _SHI__SHI_H_
#define _SHI__SHI_H_

#if defined(_cplusplus)
extern "C" {
#endif


/*  Load in the standard SHI headers that are generic  */

#include "../../codelib/include/shi/targets.h"
#include "../../codelib/include/shi/types.h"
#include "../../codelib/include/shi/xyzzy.h"


/****************************************************/
/*  CHIPSET_INCLUDE                                 */
/*                                                  */
/*  PARAMS: THE_CHIP - Chipset designator           */
/*          THE_FILE - The include file             */
/*                                                  */
/*  FUNCTIONS: Builds the approrate include for     */
/*  the desired chipset-specific header file.       */
/*                                                  */
/****************************************************/

  /* Temp hack to make Proton compile */

#if defined (__ICL)
#define CHIPSET_INCLUDE(CHIP,FILE)  <shi/chipsets/m_i586/ ## FILE ##>
#else 
#define CHIPSET_INCLUDE(CHIP,FILE)  <shi/chipsets/ ## CHIP ##  / ## FILE ##>
#endif

/****************************************************/
/*  BUILDING_FOR_CHIP                               */
/*                                                  */
/*  PARAMS: HIP - Chipset designator                */
/*                                                  */
/*  FUNCTIONS: Builds a comparison for chipset      */
/*  specific code.                                  */
/*                                                  */
/****************************************************/

#define BUILDING_FOR_CHIP(CHIP)  (CHIPSET_ID == (SHI_CS_ ## THE_CHIP))



/*  Load in the remainder of SHI header files.   */
/*  (i.e. those dependant on one or more macros  */
/*  defined above.                               */


#include "../../codelib/include/shi/int.h"
#include "../../codelib/include/shi/float.h"
#include "../../codelib/include/shi/assert.h"

#ifdef _cplusplus
};
#endif


#endif /* _SHI__SHI_H_ */

