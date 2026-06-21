#include "IsBad.h"
#include "windows.h"

#pragma warning(disable:4800)

// PHASE 5 (modern Windows): IsBadReadPtr/IsBadWritePtr are DEPRECATED and UNRELIABLE -- under
// a debugger their internal AV probe interrupts execution/crashes on dangling
// pointers (0xDDDDDDDD etc.). VirtualQuery doesn't fault, but is TOO slow
// for hot paths (stalls the frame). Hence -- a fast O(1) check.
//
// IMPORTANT: a 32-bit LargeAddressAware process on 64-bit Windows gets UP TO ~4GB
// user-space, so VALID pointers can be above 3GB too. We must not reject by
// the range ">= 0xC0000000" -- that cut valid pointers (key commands,
// textures, MFD protected via F4IsBad* => all of it 'disappeared'). Check only
// the truly invalid: the null page, the top 64KB and the debug-fill markers
// of freed/uninitialized MSVC/CRT memory (catches object->prev==0xDDDDDDDD).

static inline bool PtrLooksBad(const void* lp)
{
	uintptr_t a = (uintptr_t)lp;
	if (a < 0x10000)     return true;   // NULL / null page
	if (a >= 0xFFFF0000) return true;   // top 64KB of the address space

	switch (a)
	{
	case 0xCCCCCCCC:   // uninitialized stack (/RTC)
	case 0xCDCDCDCD:   // uninitialized heap (debug new)
	case 0xDDDDDDDD:   // freed heap (debug delete)  <-- the real case
	case 0xFDFDFDFD:   // "no man's land" guard around debug blocks
	case 0xFEEEFEEE:   // freed LocalAlloc/HeapFree
	case 0xBAADF00D:   // uninitialized LocalAlloc
	case 0xABABABAB:   // HeapAlloc guard
		return true;
	}
	return false;
}

bool F4IsBadReadPtr(const void* lp, unsigned int /*ucb*/)
{
	return PtrLooksBad(lp);
}

bool F4IsBadCodePtr(void* lpfn)
{
	return PtrLooksBad(lpfn);
}

bool F4IsBadWritePtr(void* lp, unsigned int /*ucb*/)
{
	return PtrLooksBad(lp);
}

extern "C" int F4IsBadReadPtrC(const void* lp, unsigned int ucb)
{
	return F4IsBadReadPtr(lp, ucb);
}

extern "C" int F4IsBadCodePtrC(void* lpfn)
{
	return F4IsBadCodePtr(lpfn);
}

extern "C" int F4IsBadWritePtrC(void* lp, unsigned int ucb)
{
	return F4IsBadWritePtr(lp, ucb);
}
