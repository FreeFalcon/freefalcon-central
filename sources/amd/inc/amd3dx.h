/******************************************************
 *
 * Copyright (c) 1998 ADVANCED MICRO DEVICES, INC.
 * All Rights reserved.
 *
 * This software is unpublished and contains the trade 
 * secrets and confidential proprietary information of 
 * AMD.  Unless otherwise provided in the Software 
 * Agreement associated herewith, it is licensed in 
 * confidence "AS IS" and is not to be reproduced in 
 * whole or part by any means except for backup.  Use, 
 * duplication, or disclosure by the Government is 
 * subject to the restrictions in paragraph(b)(3)(B)of 
 * the Rights in Technical Data and Computer Software 
 * clause in DFAR 52.227-7013(a)(Oct 1988).  Software 
 * owned by Advanced Micro Devices Inc., One AMD Place, 
 * P.O. Box 3453, Sunnyvale, CA 94088-3453.
 *
 ******************************************************
 *
 * AMD3DX.H
 *
 * MACRO FORMAT
 * ============
 * This file contains inline assembly macros that
 * generate AMD-3D instructions in binary format.
 * Therefore, C or C++ programmer can use AMD-3D instructions
 * without any penalty in their C or C++ source code.
 *
 * The macro's name and format conventions are as follow:
 *
 *
 * 1. First argument of macro is a destination and
 *    second argument is a source operand.
 *      ex) _asm PFCMPEQ (mm3, mm4)
 *                         |    |
 *                        dst  src
 *
 * 2. The destination operand can be m0 to m7 only.
 *    The source operand can be any one of the register
 *    m0 to m7 or _eax, _ecx, _edx, _ebx, _esi, or _edi
 *    that contains effective address.
 *      ex) _asm PFRCP    (MM7, MM6)
 *      ex) _asm PFRCPIT2 (mm0, mm4)
 *      ex) _asm PFMUL    (mm3, _edi)
 *
 *  3. The prefetch(w) takes one src operand _eax, ecx, _edx,
 *     _ebx, _esi, or _edi that contains effective address.
 *      ex) _asm PREFETCH (_edi)
 *
 * For WATCOM C/C++ users, when using #pragma aux instead if 
 * _asm, all macro names should be prefixed by a p_ or P_. 
 * Macros should not be enclosed in quotes.
 *              ex) p_pfrcp (MM7,MM6)
 *
 * NOTE: Not all instruction macros, nor all possible
 *       combinations of operands have been explicitely
 *       tested. If any errors are found, please report
 *       them.
 *
 * EXAMPLE
 * =======
 * Following program doesn't do anything but it shows you
 * how to use inline assembly AMD-3D instructions in C.
 * Note that this will only work in flat memory model which
 * segment registers cs, ds, ss and es point to the same
 * linear address space total less than 4GB.
 *
 * Used Microsoft VC++ 5.0
 *
 * #include <stdio.h>
 * #include "amd3d.h"
 *
 * void main ()
 * {
 *      float x = (float)1.25;
 *      float y = (float)1.25;
 *      float z, zz;
 *
 *     _asm {
 *              movd mm1, x
 *              movd mm2, y
 *              pfmul (mm1, mm2)
 *              movd z, mm1
 *              femms
 *      }
 *
 *      printf ("value of z = %f\n", z);
 *
 *      //
 *      // Demonstration of using the memory instead of
 *      // multimedia register
 *      //
 *      _asm {
 *              movd mm3, x
 *              lea esi, y   // load effective address of y
 *              pfmul (mm3, _esi)
 *              movd zz, mm3
 *              femms
 *      }
 *
 *      printf ("value of zz = %f\n", zz);
 *  }
 *
 * #pragma aux EXAMPLE with WATCOM C/C++ v11.x
 * ===========================================
 *
 *    extern void Add(float *__Dest, float *__A, float *__B);
 *    #pragma aux Add =               \
 *            p_femms                 \
 *            "movd mm6,[esi]"        \
 *            p_pfadd(mm6,_edi)       \
 *            "movd [ebx],mm6"        \
 *            p_femms                 \
 *            parm [ebx] [esi] [edi];
 *
 **********************************************************/

#ifndef _K3DMACROSINCLUDED_
#define _K3DMACROSINCLUDED_

#if defined (__WATCOMC__)

// The WATCOM C/C++ version of the 3DNow! macros.
//
// The older, compbined register style for WATCOM C/C++ macros is not 
// supported.

/* Operand defines for instructions two operands */
#define _k3d_mm0_mm0 0xc0
#define _k3d_mm0_mm1 0xc1
#define _k3d_mm0_mm2 0xc2
#define _k3d_mm0_mm3 0xc3
#define _k3d_mm0_mm4 0xc4
#define _k3d_mm0_mm5 0xc5
#define _k3d_mm0_mm6 0xc6
#define _k3d_mm0_mm7 0xc7
#define _k3d_mm0_eax 0x00
#define _k3d_mm0_ecx 0x01
#define _k3d_mm0_edx 0x02
#define _k3d_mm0_ebx 0x03
#define _k3d_mm0_esi 0x06
#define _k3d_mm0_edi 0x07
#define _k3d_mm1_mm0 0xc8
#define _k3d_mm1_mm1 0xc9
#define _k3d_mm1_mm2 0xca
#define _k3d_mm1_mm3 0xcb
#define _k3d_mm1_mm4 0xcc
#define _k3d_mm1_mm5 0xcd
#define _k3d_mm1_mm6 0xce
#define _k3d_mm1_mm7 0xcf
#define _k3d_mm1_eax 0x08
#define _k3d_mm1_ecx 0x09
#define _k3d_mm1_edx 0x0a
#define _k3d_mm1_ebx 0x0b
#define _k3d_mm1_esi 0x0e
#define _k3d_mm1_edi 0x0f
#define _k3d_mm2_mm0 0xd0
#define _k3d_mm2_mm1 0xd1
#define _k3d_mm2_mm2 0xd2
#define _k3d_mm2_mm3 0xd3
#define _k3d_mm2_mm4 0xd4
#define _k3d_mm2_mm5 0xd5
#define _k3d_mm2_mm6 0xd6
#define _k3d_mm2_mm7 0xd7
#define _k3d_mm2_eax 0x10
#define _k3d_mm2_ecx 0x11
#define _k3d_mm2_edx 0x12
#define _k3d_mm2_ebx 0x13
#define _k3d_mm2_esi 0x16
#define _k3d_mm2_edi 0x17
#define _k3d_mm3_mm0 0xd8
#define _k3d_mm3_mm1 0xd9
#define _k3d_mm3_mm2 0xda
#define _k3d_mm3_mm3 0xdb
#define _k3d_mm3_mm4 0xdc
#define _k3d_mm3_mm5 0xdd
#define _k3d_mm3_mm6 0xde
#define _k3d_mm3_mm7 0xdf
#define _k3d_mm3_eax 0x18
#define _k3d_mm3_ecx 0x19
#define _k3d_mm3_edx 0x1a
#define _k3d_mm3_ebx 0x1b
#define _k3d_mm3_esi 0x1e
#define _k3d_mm3_edi 0x1f
#define _k3d_mm4_mm0 0xe0
#define _k3d_mm4_mm1 0xe1
#define _k3d_mm4_mm2 0xe2
#define _k3d_mm4_mm3 0xe3
#define _k3d_mm4_mm4 0xe4
#define _k3d_mm4_mm5 0xe5
#define _k3d_mm4_mm6 0xe6
#define _k3d_mm4_mm7 0xe7
#define _k3d_mm4_eax 0x20
#define _k3d_mm4_ecx 0x21
#define _k3d_mm4_edx 0x22
#define _k3d_mm4_ebx 0x23
#define _k3d_mm4_esi 0x26
#define _k3d_mm4_edi 0x27
#define _k3d_mm5_mm0 0xe8
#define _k3d_mm5_mm1 0xe9
#define _k3d_mm5_mm2 0xea
#define _k3d_mm5_mm3 0xeb
#define _k3d_mm5_mm4 0xec
#define _k3d_mm5_mm5 0xed
#define _k3d_mm5_mm6 0xee
#define _k3d_mm5_mm7 0xef
#define _k3d_mm5_eax 0x28
#define _k3d_mm5_ecx 0x29
#define _k3d_mm5_edx 0x2a
#define _k3d_mm5_ebx 0x2b
#define _k3d_mm5_esi 0x2e
#define _k3d_mm5_edi 0x2f
#define _k3d_mm6_mm0 0xf0
#define _k3d_mm6_mm1 0xf1
#define _k3d_mm6_mm2 0xf2
#define _k3d_mm6_mm3 0xf3
#define _k3d_mm6_mm4 0xf4
#define _k3d_mm6_mm5 0xf5
#define _k3d_mm6_mm6 0xf6
#define _k3d_mm6_mm7 0xf7
#define _k3d_mm6_eax 0x30
#define _k3d_mm6_ecx 0x31
#define _k3d_mm6_edx 0x32
#define _k3d_mm6_ebx 0x33
#define _k3d_mm6_esi 0x36
#define _k3d_mm6_edi 0x37
#define _k3d_mm7_mm0 0xf8
#define _k3d_mm7_mm1 0xf9
#define _k3d_mm7_mm2 0xfa
#define _k3d_mm7_mm3 0xfb
#define _k3d_mm7_mm4 0xfc
#define _k3d_mm7_mm5 0xfd
#define _k3d_mm7_mm6 0xfe
#define _k3d_mm7_mm7 0xff
#define _k3d_mm7_eax 0x38
#define _k3d_mm7_ecx 0x39
#define _k3d_mm7_edx 0x3a
#define _k3d_mm7_ebx 0x3b
#define _k3d_mm7_esi 0x3e
#define _k3d_mm7_edi 0x3f

#define _k3d_name_xlat_m0 _mm0
#define _k3d_name_xlat_m1 _mm1
#define _k3d_name_xlat_m2 _mm2
#define _k3d_name_xlat_m3 _mm3
#define _k3d_name_xlat_m4 _mm4
#define _k3d_name_xlat_m5 _mm5
#define _k3d_name_xlat_m6 _mm6
#define _k3d_name_xlat_m7 _mm7
#define _k3d_name_xlat_M0 _mm0
#define _k3d_name_xlat_M1 _mm1
#define _k3d_name_xlat_M2 _mm2
#define _k3d_name_xlat_M3 _mm3
#define _k3d_name_xlat_M4 _mm4
#define _k3d_name_xlat_M5 _mm5
#define _k3d_name_xlat_M6 _mm6
#define _k3d_name_xlat_M7 _mm7
#define _k3d_name_xlat_mm0 _mm0
#define _k3d_name_xlat_mm1 _mm1
#define _k3d_name_xlat_mm2 _mm2
#define _k3d_name_xlat_mm3 _mm3
#define _k3d_name_xlat_mm4 _mm4
#define _k3d_name_xlat_mm5 _mm5
#define _k3d_name_xlat_mm6 _mm6
#define _k3d_name_xlat_mm7 _mm7
#define _k3d_name_xlat_MM0 _mm0
#define _k3d_name_xlat_MM1 _mm1
#define _k3d_name_xlat_MM2 _mm2
#define _k3d_name_xlat_MM3 _mm3
#define _k3d_name_xlat_MM4 _mm4
#define _k3d_name_xlat_MM5 _mm5
#define _k3d_name_xlat_MM6 _mm6
#define _k3d_name_xlat_MM7 _mm7
#define _k3d_name_xlat_eax _eax
#define _k3d_name_xlat_ebx _ebx
#define _k3d_name_xlat_ecx _ecx
#define _k3d_name_xlat_edx _edx
#define _k3d_name_xlat_esi _esi
#define _k3d_name_xlat_edi _edi
#define _k3d_name_xlat_ebp _ebp
#define _k3d_name_xlat_EAX _eax
#define _k3d_name_xlat_EBX _ebx
#define _k3d_name_xlat_ECX _ecx
#define _k3d_name_xlat_EDX _edx
#define _k3d_name_xlat_ESI _esi
#define _k3d_name_xlat_EDI _edi
#define _k3d_name_xlat_EBP _ebp
#define _k3d_name_xlat__eax _eax
#define _k3d_name_xlat__ebx _ebx
#define _k3d_name_xlat__ecx _ecx
#define _k3d_name_xlat__edx _edx
#define _k3d_name_xlat__esi _esi
#define _k3d_name_xlat__edi _edi
#define _k3d_name_xlat__ebp _ebp
#define _k3d_name_xlat__EAX _eax
#define _k3d_name_xlat__EBX _ebx
#define _k3d_name_xlat__ECX _ecx
#define _k3d_name_xlat__EDX _edx
#define _k3d_name_xlat__ESI _esi
#define _k3d_name_xlat__EDI _edi
#define _k3d_name_xlat__EBP _ebp

#define _k3d_xglue3(a,b,c) a##b##c
#define _k3d_glue3(a,b,c) _k3d_xglue3(a,b,c)
#define _k3d_MODRM(dst, src) _k3d_glue3(_k3d,_k3d_name_xlat_##dst,_k3d_name_xlat_##src)

/* Operand defines for prefetch and prefetchw */

#define _k3d_pref_eax 0x00
#define _k3d_pref_ecx 0x01
#define _k3d_pref_edx 0x02
#define _k3d_pref_ebx 0x03
#define _k3d_pref_esi 0x06
#define _k3d_pref_edi 0x07
#define _k3d_pref_EAX 0x00
#define _k3d_pref_ECX 0x01
#define _k3d_pref_EDX 0x02
#define _k3d_pref_EBX 0x03
#define _k3d_pref_ESI 0x06
#define _k3d_pref_EDI 0x07
#define _k3d_prefw_eax 0x08
#define _k3d_prefw_ecx 0x09
#define _k3d_prefw_edx 0x0A
#define _k3d_prefw_ebx 0x0B
#define _k3d_prefw_esi 0x0E
#define _k3d_prefw_edi 0x0F
#define _k3d_prefw_EAX 0x08
#define _k3d_prefw_ECX 0x09
#define _k3d_prefw_EDX 0x0A
#define _k3d_prefw_EBX 0x0B
#define _k3d_prefw_ESI 0x0E
#define _k3d_prefw_EDI 0x0F

/* Defines for 3DNow! instructions */

#define PF2ID(dst, src)         db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0x1d
#define PFACC(dst, src)         db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0xae
#define PFADD(dst, src)         db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0x9e
#define PFCMPEQ(dst, src)       db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0xb0
#define PFCMPGE(dst, src)       db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0x90
#define PFCMPGT(dst, src)       db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0xa0
#define PFMAX(dst, src)         db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0xa4
#define PFMIN(dst, src)         db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0x94
#define PFMUL(dst, src)         db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0xb4
#define PFRCP(dst, src)         db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0x96
#define PFRCPIT1(dst, src)      db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0xa6
#define PFRCPIT2(dst, src)      db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0xb6
#define PFRSQRT(dst, src)       db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0x97
#define PFRSQIT1(dst, src)      db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0xa7
#define PFSUB(dst, src)         db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0x9a
#define PFSUBR(dst, src)        db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0xaa
#define PI2FD(dst, src)         db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0x0d
#define FEMMS                   db 0x0f, 0x0e
#define PAVGUSB(dst, src)       db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0xbf
#define PMULHRW(dst, src)       db 0x0f, 0x0f, _k3d_MODRM(dst, src), 0xb7
#define PREFETCH(src)           db 0x0f, 0x0d, _k3d_pref_##src
#define PREFETCHW(src)          db 0x0f, 0x0d, _k3d_prefw_##src


/* Memory/offset versions of the opcodes */

#define PF2IDM(dst,src,off)     db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0x1d
#define PFACCM(dst,src,off)     db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0xae
#define PFADDM(dst,src,off)     db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0x9e
#define PFCMPEQM(dst,src,off)   db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0xb0
#define PFCMPGEM(dst,src,off)   db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0x90
#define PFCMPGTM(dst,src,off)   db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0xa0
#define PFMAXM(dst,src,off)     db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0xa4
#define PFMINM(dst,src,off)     db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0x94
#define PFMULM(dst,src,off)     db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0xb4
#define PFRCPM(dst,src,off)     db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0x96
#define PFRCPIT1M(dst,src,off)  db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0xa6
#define PFRCPIT2M(dst,src,off)  db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0xb6
#define PFRSQRTM(dst,src,off)   db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0x97
#define PFRSQIT1M(dst,src,off)  db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0xa7
#define PFSUBM(dst,src,off)     db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0x9a
#define PFSUBRM(dst,src,off)    db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0xaa
#define PI2FDM(dst,src,off)     db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0x0d
#define PAVGUSBM(dst,src,off)   db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0xbf
#define PMULHRWM(dst,src,off)   db 0x0f, 0x0f, _k3d_MODRM(dst,src) | 0x40, off, 0xb7


/* Defines for 3DNow! instructions for use in pragmas */
#define p_pf2id(dst,src)		0x0f 0x0f _k3d_MODRM(dst,src) 0x1d
#define p_pfacc(dst,src)		0x0f 0x0f _k3d_MODRM(dst,src) 0xae
#define p_pfadd(dst,src)		0x0f 0x0f _k3d_MODRM(dst,src) 0x9e
#define p_pfcmpeq(dst,src)		0x0f 0x0f _k3d_MODRM(dst,src) 0xb0
#define p_pfcmpge(dst,src)		0x0f 0x0f _k3d_MODRM(dst,src) 0x90
#define p_pfcmpgt(dst,src)		0x0f 0x0f _k3d_MODRM(dst,src) 0xa0
#define p_pfmax(dst,src)		0x0f 0x0f _k3d_MODRM(dst,src) 0xa4
#define p_pfmin(dst,src)		0x0f 0x0f _k3d_MODRM(dst,src) 0x94
#define p_pfmul(dst,src)		0x0f 0x0f _k3d_MODRM(dst,src) 0xb4
#define p_pfrcp(dst,src)		0x0f 0x0f _k3d_MODRM(dst,src) 0x96
#define p_pfrcpit1(dst,src)		0x0f 0x0f _k3d_MODRM(dst,src) 0xa6
#define p_pfrcpit2(dst,src)		0x0f 0x0f _k3d_MODRM(dst,src) 0xb6
#define p_pfrsqrt(dst,src)		0x0f 0x0f _k3d_MODRM(dst,src) 0x97
#define p_pfrsqit1(dst,src)		0x0f 0x0f _k3d_MODRM(dst,src) 0xa7
#define p_pfsub(dst,src)		0x0f 0x0f _k3d_MODRM(dst,src) 0x9a
#define p_pfsubr(dst,src)		0x0f 0x0f _k3d_MODRM(dst,src) 0xaa
#define p_pi2fd(dst,src)		0x0f 0x0f _k3d_MODRM(dst,src) 0x0d
#define p_femms					0x0f 0x0e
#define p_pavgusb(dst,src)		0x0f 0x0f _k3d_MODRM(dst,src) 0xbf
#define p_pmulhrw(dst,src)		0x0f 0x0f _k3d_MODRM(dst,src) 0xb7
#define p_prefetch(src)			0x0f 0x0d _k3d_pref_##src
#define p_prefetchw(src)		0x0f 0x0d _k3d_prefw_##src


#define P_PF2IDM(dst,src,off)    0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0x1d
#define P_PFACCM(dst,src,off)    0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0xae
#define P_PFADDM(dst,src,off)    0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0x9e
#define P_PFCMPEQM(dst,src,off)  0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0xb0
#define P_PFCMPGEM(dst,src,off)  0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0x90
#define P_PFCMPGTM(dst,src,off)  0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0xa0
#define P_PFMAXM(dst,src,off)    0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0xa4
#define P_PFMINM(dst,src,off)    0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0x94
#define P_PFMULM(dst,src,off)    0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0xb4
#define P_PFRCPM(dst,src,off)    0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0x96
#define P_PFRCPIT1M(dst,src,off) 0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0xa6
#define P_PFRCPIT2M(dst,src,off) 0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0xb6
#define P_PFRSQRTM(dst,src,off)  0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0x97
#define P_PFRSQIT1M(dst,src,off) 0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0xa7
#define P_PFSUBM(dst,src,off)    0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0x9a
#define P_PFSUBRM(dst,src,off)   0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0xaa
#define P_PI2FDM(dst,src,off)    0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0x0d
#define P_PAVGUSBM(dst,src,off)  0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0xbf
#define P_PMULHRWM(dst,src,off)  0x0f 0x0f (_k3d_MODRM(dst,src) | 0x40) off 0xb7


#define P_PF2ID(dst,src)			p_pf2id(dst,src)
#define P_PFACC(dst,src)			p_pfacc(dst,src)
#define P_PFADD(dst,src)			p_pfadd(dst,src)
#define P_PFCMPEQ(dst,src)			p_pfcmpeq(dst,src)
#define P_PFCMPGE(dst,src)			p_pfcmpge(dst,src)
#define P_PFCMPGT(dst,src)			p_pfcmpgt(dst,src)
#define P_PFMAX(dst,src)			p_pfmax(dst,src)
#define P_PFMIN(dst,src)			p_pfmin(dst,src)
#define P_PFMUL(dst,src)			p_pfmul(dst,src)
#define P_PFRCP(dst,src)			p_pfrcp(dst,src)
#define P_PFRCPIT1(dst,src)			p_pfrcpit1(dst,src)
#define P_PFRCPIT2(dst,src)			p_pfrcpit2(dst,src)
#define P_PFRSQRT(dst,src)			p_pfrsqrt(dst,src)
#define P_PFRSQIT1(dst,src)			p_pfrsqit1(dst,src)
#define P_PFSUB(dst,src)			p_pfsub(dst,src)
#define P_PFSUBR(dst,src)			p_pfsubr(dst,src)
#define P_PI2FD(dst,src)			p_pi2fd(dst,src)
#define P_FEMMS						p_femms
#define P_PAVGUSB(dst,src)			p_pavgusb(dst,src)
#define P_PMULHRW(dst,src)			p_pmulhrw(dst,src)
#define P_PREFETCH(src)				p_prefetch(src)
#define P_PREFETCHW(src)			p_prefetchw(src)
#define p_pf2idm(dst,src,off)		P_PF2IDM(dst,src,off)
#define p_pfaccm(dst,src,off)		P_PFACCM(dst,src,off)
#define p_pfaddm(dst,src,off)		P_PFADDM(dst,src,off)
#define p_pfcmpeqm(dst,src,off)		P_PFCMPEQM(dst,src,off)
#define p_pfcmpgem(dst,src,off)		P_PFCMPGEM(dst,src,off)
#define p_pfcmpgtm(dst,src,off)		P_PFCMPGTM(dst,src,off)
#define p_pfmaxm(dst,src,off)		P_PFMAXM(dst,src,off)
#define p_pfminm(dst,src,off)		P_PFMINM(dst,src,off)
#define p_pfmulm(dst,src,off)		P_PFMULM(dst,src,off)
#define p_pfrcpm(dst,src,off)		P_PFRCPM(dst,src,off)
#define p_pfrcpit1m(dst,src,off)	P_PFRCPIT1M(dst,src,off)
#define p_pfrcpit2m(dst,src,off)	P_PFRCPIT2M(dst,src,off)
#define p_pfrsqrtm(dst,src,off)		P_PFRSQRTM(dst,src,off)
#define p_pfrsqit1m(dst,src,off)	P_PFRSQIT1M(dst,src,off)
#define p_pfsubm(dst,src,off)		P_PFSUBM(dst,src,off)
#define p_pfsubrm(dst,src,off)		P_PFSUBRM(dst,src,off)
#define p_pi2fdm(dst,src,off)		P_PI2FDM(dst,src,off)
#define p_pavgusbm(dst,src,off)		P_PAVGUSBM(dst,src,off)
#define p_pmulhrwm(dst,src,off)		P_PMULHRWM(dst,src,off)


#elif defined (_MSC_VER)
// The Microsoft Visual C++ version of the 3DNow! macros.

// Defines for operands.
#define _K3D_MM0 0xc0
#define _K3D_MM1 0xc1
#define _K3D_MM2 0xc2
#define _K3D_MM3 0xc3
#define _K3D_MM4 0xc4
#define _K3D_MM5 0xc5
#define _K3D_MM6 0xc6
#define _K3D_MM7 0xc7
#define _K3D_mm0 0xc0
#define _K3D_mm1 0xc1
#define _K3D_mm2 0xc2
#define _K3D_mm3 0xc3
#define _K3D_mm4 0xc4
#define _K3D_mm5 0xc5
#define _K3D_mm6 0xc6
#define _K3D_mm7 0xc7
#define _K3D_EAX 0x00
#define _K3D_ECX 0x01
#define _K3D_EDX 0x02
#define _K3D_EBX 0x03
#define _K3D_ESI 0x06
#define _K3D_EDI 0x07
#define _K3D_eax 0x00
#define _K3D_ecx 0x01
#define _K3D_edx 0x02
#define _K3D_ebx 0x03
#define _K3D_esi 0x06
#define _K3D_edi 0x07
#define _K3D_EAX 0x00
#define _K3D_ECX 0x01
#define _K3D_EDX 0x02
#define _K3D_EBX 0x03
#define _K3D_ESI 0x06
#define _K3D_EDI 0x07
#define _K3D_eax 0x00
#define _K3D_ecx 0x01
#define _K3D_edx 0x02
#define _K3D_ebx 0x03
#define _K3D_esi 0x06
#define _K3D_edi 0x07

// These defines are for compatibility with the previous version of the header file.
#define _K3D_M0   0xc0
#define _K3D_M1   0xc1
#define _K3D_M2   0xc2
#define _K3D_M3   0xc3
#define _K3D_M4   0xc4
#define _K3D_M5   0xc5
#define _K3D_M6   0xc6
#define _K3D_M7   0xc7
#define _K3D_m0   0xc0
#define _K3D_m1   0xc1
#define _K3D_m2   0xc2
#define _K3D_m3   0xc3
#define _K3D_m4   0xc4
#define _K3D_m5   0xc5
#define _K3D_m6   0xc6
#define _K3D_m7   0xc7
#define _K3D__EAX 0x00
#define _K3D__ECX 0x01
#define _K3D__EDX 0x02
#define _K3D__EBX 0x03
#define _K3D__ESI 0x06
#define _K3D__EDI 0x07
#define _K3D__eax 0x00
#define _K3D__ecx 0x01
#define _K3D__edx 0x02
#define _K3D__ebx 0x03
#define _K3D__esi 0x06
#define _K3D__edi 0x07

// General 3DNow! instruction format that is supported by 
// these macros. Note that only the most basic form of memory 
// operands are supported by these macros. 

#define InjK3DOps(dst,src,inst)                         \
{                                                       \
   _asm _emit 0x0f                                      \
   _asm _emit 0x0f                                      \
   _asm _emit ((_K3D_##dst & 0x3f) << 3) | _K3D_##src   \
   _asm _emit _3DNowOpcode##inst                        \
}

#define InjK3DMOps(dst,src,off,inst)                    \
{                                                       \
   _asm _emit 0x0f                                      \
   _asm _emit 0x0f                                      \
   _asm _emit (((_K3D_##dst & 0x3f) << 3) | _K3D_##src | 0x40) \
   _asm _emit off						\
   _asm _emit _3DNowOpcode##inst                        \
}

#define _3DNowOpcodePF2ID    0x1d
#define _3DNowOpcodePFACC    0xae
#define _3DNowOpcodePFADD    0x9e
#define _3DNowOpcodePFCMPEQ  0xb0
#define _3DNowOpcodePFCMPGE  0x90
#define _3DNowOpcodePFCMPGT  0xa0
#define _3DNowOpcodePFMAX    0xa4
#define _3DNowOpcodePFMIN    0x94
#define _3DNowOpcodePFMUL    0xb4
#define _3DNowOpcodePFRCP    0x96
#define _3DNowOpcodePFRCPIT1 0xa6
#define _3DNowOpcodePFRCPIT2 0xb6
#define _3DNowOpcodePFRSQRT  0x97
#define _3DNowOpcodePFRSQIT1 0xa7
#define _3DNowOpcodePFSUB    0x9a
#define _3DNowOpcodePFSUBR   0xaa
#define _3DNowOpcodePI2FD    0x0d
#define _3DNowOpcodePAVGUSB  0xbf
#define _3DNowOpcodePMULHRW  0xb7

#define PF2ID(dst,src)		InjK3DOps(dst, src, PF2ID)
#define PFACC(dst,src)		InjK3DOps(dst, src, PFACC)
#define PFADD(dst,src)		InjK3DOps(dst, src, PFADD)
#define PFCMPEQ(dst,src)	InjK3DOps(dst, src, PFCMPEQ)
#define PFCMPGE(dst,src)	InjK3DOps(dst, src, PFCMPGE)
#define PFCMPGT(dst,src)	InjK3DOps(dst, src, PFCMPGT)
#define PFMAX(dst,src)		InjK3DOps(dst, src, PFMAX)
#define PFMIN(dst,src)		InjK3DOps(dst, src, PFMIN)
#define PFMUL(dst,src)		InjK3DOps(dst, src, PFMUL)
#define PFRCP(dst,src)		InjK3DOps(dst, src, PFRCP)
#define PFRCPIT1(dst,src)	InjK3DOps(dst, src, PFRCPIT1)
#define PFRCPIT2(dst,src)	InjK3DOps(dst, src, PFRCPIT2)
#define PFRSQRT(dst,src)	InjK3DOps(dst, src, PFRSQRT)
#define PFRSQIT1(dst,src)	InjK3DOps(dst, src, PFRSQIT1)
#define PFSUB(dst,src)		InjK3DOps(dst, src, PFSUB)
#define PFSUBR(dst,src)		InjK3DOps(dst, src, PFSUBR)
#define PI2FD(dst,src)		InjK3DOps(dst, src, PI2FD)
#define PAVGUSB(dst,src)	InjK3DOps(dst, src, PAVGUSB)
#define PMULHRW(dst,src)	InjK3DOps(dst, src, PMULHRW)

#define FEMMS                                   \
{                                               \
   _asm _emit 0x0f                              \
   _asm _emit 0x0e                              \
}

#define PREFETCH(src)                           \
{                                               \
   _asm _emit 0x0f                              \
   _asm _emit 0x0d                              \
   _asm _emit 0x00 | src                        \
}

#define PREFETCHW(src)                          \
{                                               \
   _asm _emit 0x0f                              \
   _asm _emit 0x0d                              \
   _asm _emit 0x08 | src                        \
}

/* Memory/offset versions of the opcodes */
#define PAVGUSBM(dst,src,off)	InjK3DMOps(dst,src,off,PAVGUSB)
#define PF2IDM(dst,src,off)		InjK3DMOps(dst,src,off,PF2ID)
#define PFACCM(dst,src,off)		InjK3DMOps(dst,src,off,PFACC)
#define PFADDM(dst,src,off)		InjK3DMOps(dst,src,off,PFADD)
#define PFCMPEQM(dst,src,off)	InjK3DMOps(dst,src,off,PFCMPEQ)
#define PFCMPGEM(dst,src,off)	InjK3DMOps(dst,src,off,PFCMPGE)
#define PFCMPGTM(dst,src,off)	InjK3DMOps(dst,src,off,PFCMPGT)
#define PFMAXM(dst,src,off)		InjK3DMOps(dst,src,off,PFMAX)
#define PFMINM(dst,src,off)		InjK3DMOps(dst,src,off,PFMIN)
#define PFMULM(dst,src,off)		InjK3DMOps(dst,src,off,PFMUL)
#define PFRCPM(dst,src,off)		InjK3DMOps(dst,src,off,PFRCP)
#define PFRCPIT1M(dst,src,off)	InjK3DMOps(dst,src,off,PFRCPIT1)
#define PFRCPIT2M(dst,src,off)	InjK3DMOps(dst,src,off,PFRCPIT2)
#define PFRSQRTM(dst,src,off)	InjK3DMOps(dst,src,off,PFRSQRT)
#define PFRSQIT1M(dst,src,off)	InjK3DMOps(dst,src,off,PFRSQIT1)
#define PFSUBM(dst,src,off)		InjK3DMOps(dst,src,off,PFSUB)
#define PFSUBRM(dst,src,off)	InjK3DMOps(dst,src,off,PFSUBR)
#define PI2FDM(dst,src,off)		InjK3DMOps(dst,src,off,PI2FD)
#define PMULHRWM(dst,src,off)	InjK3DMOps(dst,src,off,PMULHRW)

#else

/* Assume built-in support for 3DNow! opcodes, replace macros with opcodes */
#define PAVGUSB(dst,src)	pavgusb		dst,src
#define PF2ID(dst,src)		pf2id		dst,src
#define PFACC(dst,src)		pfacc		dst,src
#define PFADD(dst,src)		pfadd		dst,src
#define PFCMPEQ(dst,src)	pfcmpeq		dst,src
#define PFCMPGE(dst,src)	pfcmpge		dst,src
#define PFCMPGT(dst,src)	pfcmpgt		dst,src
#define PFMAX(dst,src)		pfmax		dst,src
#define PFMIN(dst,src)		pfmin		dst,src
#define PFMUL(dst,src)		pfmul		dst,src
#define PFRCP(dst,src)		pfrcp		dst,src
#define PFRCPIT1(dst,src)	pfrcpit1	dst,src
#define PFRCPIT2(dst,src)	pfrcpit2	dst,src
#define PFRSQRT(dst,src)	pfrsqrt		dst,src
#define PFRSQIT1(dst,src)	pfrsqit1	dst,src
#define PFSUB(dst,src)		pfsub		dst,src
#define PFSUBR(dst,src)		fpsubr		dst,src
#define PI2FD(dst,src)		pi2fd		dst,src
#define PMULHRW(dst,src)	pmulhrw		dst,src
#define PREFETCH(src)		prefetch	src
#define PREFETCHW(src)		prefetchw	src

#define PAVGUSBM(dst,src,off)	pavgusb		dst,[src+off]
#define PF2IDM(dst,src,off)		PF2ID		dst,[src+off]
#define PFACCM(dst,src,off)		PFACC		dst,[src+off]
#define PFADDM(dst,src,off)		PFADD		dst,[src+off]
#define PFCMPEQM(dst,src,off)	PFCMPEQ		dst,[src+off]
#define PFCMPGEM(dst,src,off)	PFCMPGE		dst,[src+off]
#define PFCMPGTM(dst,src,off)	PFCMPGT		dst,[src+off]
#define PFMAXM(dst,src,off)		PFMAX		dst,[src+off]
#define PFMINM(dst,src,off)		PFMIN		dst,[src+off]
#define PFMULM(dst,src,off)		PFMUL		dst,[src+off]
#define PFRCPM(dst,src,off)		PFRCP		dst,[src+off]
#define PFRCPIT1M(dst,src,off)	PFRCPIT1	dst,[src+off]
#define PFRCPIT2M(dst,src,off)	PFRCPIT2	dst,[src+off]
#define PFRSQRTM(dst,src,off)	PFRSQRT		dst,[src+off]
#define PFRSQIT1M(dst,src,off)	PFRSQIT1	dst,[src+off]
#define PFSUBM(dst,src,off)		PFSUB		dst,[src+off]
#define PFSUBRM(dst,src,off)	PFSUBR		dst,[src+off]
#define PI2FDM(dst,src,off)		PI2FD		dst,[src+off]
#define PMULHRWM(dst,src,off)	PMULHRW		dst,[src+off]


#endif

/* Just to deal with lower case. */
#define pf2id(dst,src)			PF2ID(dst,src)
#define pfacc(dst,src)			PFACC(dst,src)
#define pfadd(dst,src)			PFADD(dst,src)
#define pfcmpeq(dst,src)		PFCMPEQ(dst,src)
#define pfcmpge(dst,src)		PFCMPGE(dst,src)
#define pfcmpgt(dst,src)		PFCMPGT(dst,src)
#define pfmax(dst,src)			PFMAX(dst,src)
#define pfmin(dst,src)			PFMIN(dst,src)
#define pfmul(dst,src)			PFMUL(dst,src)
#define pfrcp(dst,src)			PFRCP(dst,src)
#define pfrcpit1(dst,src)		PFRCPIT1(dst,src)
#define pfrcpit2(dst,src)		PFRCPIT2(dst,src)
#define pfrsqrt(dst,src)		PFRSQRT(dst,src)
#define pfrsqit1(dst,src)		PFRSQIT1(dst,src)
#define pfsub(dst,src)			PFSUB(dst,src)
#define pfsubr(dst,src)			PFSUBR(dst,src)
#define pi2fd(dst,src)			PI2FD(dst,src)
#define femms					FEMMS
#define pavgusb(dst,src)		PAVGUSB(dst,src)
#define pmulhrw(dst,src)		PMULHRW(dst,src)
#define prefetch(src)			PREFETCH(src)
#define prefetchw(src)			PREFETCHW(src)
#define pavgusbm(dst,src,off)	PAVGUSBM(dst,src,off)
#define pf2idm(dst,src,off)		PF2IDM(dst,src,off)
#define pfaccm(dst,src,off)		PFACCM(dst,src,off)
#define pfaddm(dst,src,off)		PFADDM(dst,src,off)
#define pfcmpeqm(dst,src,off)	PFCMPEQM(dst,src,off)
#define pfcmpgem(dst,src,off)	PFCMPGEM(dst,src,off)
#define pfcmpgtm(dst,src,off)	PFCMPGTM(dst,src,off)
#define pfmaxm(dst,src,off)		PFMAXM(dst,src,off)
#define pfminm(dst,src,off)		PFMINM(dst,src,off)
#define pfmulm(dst,src,off)		PFMULM(dst,src,off)
#define pfrcpm(dst,src,off)		PFRCPM(dst,src,off)
#define pfrcpit1m(dst,src,off)	PFRCPIT1M(dst,src,off)
#define pfrcpit2m(dst,src,off)	PFRCPIT2M(dst,src,off)
#define pfrsqrtm(dst,src,off)	PFRSQRTM(dst,src,off)
#define pfrsqit1m(dst,src,off)	PFRSQIT1M(dst,src,off)
#define pfsubm(dst,src,off)		PFSUBM(dst,src,off)
#define pfsubrm(dst,src,off)	PFSUBRM(dst,src,off)
#define pi2fdm(dst,src,off)		PI2FDM(dst,src,off)
#define pmulhrwm(dst,src,off)	PMULHRWM(dst,src,off)


#endif
