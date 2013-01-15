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
 * DETECT.C
 *
 * AMD3D 3D library code: Code to detect for 3DNow! capability.
 *
 *	BETA RELEASE
 *
 *****************************************************************************/

#include <amdlib.h>

int has3DNow (void)
{
	__asm {
		pushfd					// save EFLAGS to stack.
		pop		eax				// store EFLAGS in EAX.
		mov		edx, eax		// save in EBX for testing later.
		xor		eax, 0x200000	// switch bit 21.
		push	eax				// copy "changed" value to stack.
		popfd					// save "changed" EAX to EFLAGS.
		pushfd
		pop		eax
		cmp		eax, edx		// See if bit changeable.
		jz		No3DNow			// CPU doesn't support CPUID instruction.

		mov		eax,0x80000000	// Check for support of extended functions.
		CPUID
		cmp		eax,0x80000001	// Make sure function 0x80000001 supported.
		jb		No3DNow
                                // Now we know extended functions are supported.
		mov		eax,0x80000001	// Get extended features.
		xor		edx, edx		// Clear edx - not really necessary.
		CPUID
        test    edx,0x80000000	// edx contains extended feature flags
		jz		No3DNow			// bit 31 = 1 => 3DNow!
	}

	return 1;

No3DNow:
	return 0;
}

// eof