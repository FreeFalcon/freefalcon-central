/*****************************************************************************
 *
 * Copyright (c) 1996-1999 ADVANCED MICRO DEVICES, INC. All Rights reserved.
 *
 * This software is unpublished and contains the trade secrets and
 * confidential proprietary information of AMD. Unless otherwise
 * provided in the Software Agreement associated herewith, it is
 * licensed in confidence "AS IS" and is not to be reproduced in
 * whole or part by any means except for backup. Use, duplication,
 * or disclosure by the Government is subject to the restrictions
 * in paragraph(b)(3)(B)of the Rights in Technical Data and
 * Computer Software clause in DFAR 52.227-7013(a)(Oct 1988).
 * Software owned by Advanced Micro Devices Inc., One AMD Place
 * P.O. Box 3453, Sunnyvale, CA 94088-3453.
 *
 *****************************************************************************
 *
 * JFDCT_LIB.C
 *
 * AMD3D 3D library code: JPEG Discrete Cosine Transform
 *
 *	BETA RELEASE
 *
 *****************************************************************************/

#include <amd3dx.h>

#ifdef _MSC_VER
// We don't need EMMS instruction warnings
#pragma warning(disable:4799)
#endif

static float fdct_ws[64];
static float a1[2] = {0.707106781f, 0.707106781f};
static float a2[2] = {0.541196100f, 1.306562965f};
static float a4 =  1.306562965f;
static float a5 = 0.382683433f;
static float PMOne[2] = {1.0f, -1.0f};

void _a_jpeg_fdct(float *data)
{
	float tmp7;

	__asm
	{
		femms
   		mov			ecx, 7;
      	mov			ebx, offset fdct_ws;
      	mov			edx, data;
      	mov			eax, edx;
 
DCT_Pass1_Loop:

		movq  		mm0,[eax];     
		movq  		mm4,[eax+0x8];                 

        movq  		mm3,mm0;              
        movq  		mm5,[eax+0x10];                     

        movq  		mm7,mm4;               
        movq  		mm1,[eax+0x18];             

        punpckldq  	mm2,mm1;
        punpckldq  	mm6,mm5;

        punpckhdq 	mm1,mm2;
        punpckhdq 	mm5,mm6;

		pfadd 		(mm0,mm1)
		pfadd 		(mm4,mm5)

		pfsub 		(mm3,mm1)
		pfsub 		(mm7,mm5)

        punpckldq 	mm5,mm4;
        movq  		mm2,mm0;

        movq  		mm6,mm3;
        punpckhdq 	mm3,mm3;

        punpckhdq 	mm4,mm5;           
        movd  		tmp7,mm6;
                   
		pfadd 		(mm3,mm7)
		pfacc 		(mm7,mm6)
		pfadd 		(mm0,mm4)
		pfsub 		(mm2,mm4)

        movq  		mm6, QWORD PTR a1             
        movq  		mm1,mm7

        movq  		mm4,mm0
		pfacc 		(mm0,mm0)

        movq  		mm5,mm2
		pfacc 		(mm2,mm2)

        pfmul		mm4,PMOne
        movd  		[ebx],mm0
        punpckldq 	mm2,mm3
        movq  		mm0, QWORD PTR tmp7

		pfacc 		(mm4,mm4)
        pfmul		mm1,PMOne

		pfmul 		(mm2,mm6)
        punpckldq 	mm5,mm0;    

        movd  		mm6, a5;             
		pfacc 		(mm1,mm1)

        movq  		mm3,mm2;   
        movq  		mm0, QWORD PTR a2; 

		pfadd 		(mm2,mm5)
		pfmul 		(mm1,mm6)

		pfsub 		(mm5,mm3)
        movd  		[ebx+0x80],mm4;  

		pfmul 		(mm0,mm7)
        punpckldq 	mm1,mm1; 

        movd  		[ebx+0x40],mm2;
        movd  		[ebx+0xc0],mm5;

		pfadd 		(mm0,mm1)
        punpckhdq  	mm5,mm2;

        movq  		mm1,mm0;
		pfadd 		(mm0,mm5)

		pfsub 		(mm5,mm1)
        movd  		[ebx+0xa0],mm0;

        movd  		[ebx+0x60],mm5;
        punpckhdq 	mm0,mm0;

        movd  		[ebx+0x20],mm0;
        punpckhdq 	mm5,mm5;

        movd  		[ebx+0xe0],mm5;

        add 		eax,32;
	    add 		ebx,4;
	    dec 		ecx;
	    jns 		DCT_Pass1_Loop;

   	    mov			ecx, 7;
        mov			ebx, offset fdct_ws;
        mov			edx, data;
        mov			eax, edx;                  

DCT_Pass2_Loop:

        movq  		mm0,[ebx];
        movq  		mm4,[ebx+0x8];

        movq  		mm3,mm0;
        movq  		mm5,[ebx+0x10];

        movq  		mm7,mm4;
        movq  		mm1,[ebx+0x18];

        punpckldq  	mm2,mm1;
        punpckldq  	mm6,mm5;

        punpckhdq 	mm1,mm2;
        punpckhdq 	mm5,mm6;

		pfadd 		(mm0,mm1)
		pfadd 		(mm4,mm5)

		pfsub 		(mm3,mm1)
		pfsub 		(mm7,mm5)

        punpckldq 	mm5,mm4
        movq  		mm2,mm0

        movq  		mm6,mm3
        punpckhdq 	mm3,mm3

        punpckhdq 	mm4,mm5
        movd  		tmp7,mm6

		pfadd 		(mm3,mm7)
		pfacc 		(mm7,mm6)

		pfadd 		(mm0,mm4)
		pfsub 		(mm2,mm4)

        movq  		mm6, QWORD PTR a1;
        movq  		mm1,mm7;

        movq  		mm4,mm0
		pfacc 		(mm0,mm0)

        movq  		mm5,mm2
		pfacc 		(mm2,mm2)

        pfmul		mm4,PMOne;    
        movd  		[eax],mm0;

        punpckldq 	mm2,mm3;
        movq  		mm0,QWORD PTR tmp7;

		pfacc 		(mm4,mm4)
        pfmul 		mm1,PMOne
		

		pfmul 		(mm2,mm6)
        punpckldq 	mm5,mm0;

        movd  		mm6,a5
		pfacc 		(mm1,mm1)

        movq  		mm3,mm2;
        movq  		mm0, QWORD PTR a2;

		pfadd 		(mm2,mm5)
		pfmul 		(mm1,mm6)

		pfsub 		(mm5,mm3)
        movd  		[eax+0x80],mm4

		pfmul 		(mm0,mm7)
        punpckldq 	mm1,mm1

        movd  		[eax+0x40],mm2
        movd  		[eax+0xc0],mm5

		pfadd 		(mm0,mm1)
        punpckhdq  	mm5,mm2

        movq  		mm1,mm0
		pfadd 		(mm0,mm5)

		pfsub 		(mm5,mm1)
        movd  		[eax+0xa0],mm0
 
        movd  		[eax+0x60],mm5
        punpckhdq 	mm0,mm0

        movd  		[eax+0x20],mm0
        punpckhdq 	mm5,mm5

        movd  		[eax+0xe0],mm5

        add 		ebx,32
        add 		eax,4
        dec 		ecx
        jns 		DCT_Pass2_Loop
        femms
    }
}