#include "isbad.h"
#include "windows.h"

#pragma warning(disable : 4800)

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
    if (a < 0x10000)
        return true; // NULL / null page

    // Artscout - 2026 (#60 x64 ROOT): the old upper guard "a >= 0xFFFF0000" meant "top 64KB of a 32-bit
    // address space". In a 64-bit build 0xFFFF0000 is merely ~4GB, and VALID heap pointers routinely live
    // ABOVE 4GB. So this rejected EVERY pointer >= ~4GB as "bad" -> under VR (the heap grows past 4GB from
    // the eye buffers / MSAA / quad-views) all high-address drawables were flagged bad by the ~456 F4IsBad*
    // guards -> DrawBeyond/RemoveObject skipped valid objects -> ALL world objects vanished and the list
    // corrupted (exit-crash). Flat/windowed stays under 4GB -> never tripped, which is why it was VR-only.
    // Fix: on x64 the top-64KB guard is the top of the 64-bit space; reject only that + the debug-fill
    // poison markers (both full-width and the low-dword 32-bit form that can land in a 64-bit field).
#if defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64) ||                 \
    defined(__x86_64__) || defined(__amd64__) || defined(__aarch64__) ||       \
    (defined(__SIZEOF_POINTER__) && __SIZEOF_POINTER__ == 8)
    // #104: the MSVC-only _WIN64/_M_X64 macros are undefined on Linux/clang, so this x64 branch was skipped and
    // the x86 `a >= 0xFFFF0000` guard ran -- flagging EVERY >4GB heap pointer as bad. On LP64 Linux the whole heap
    // lives far above 4GB, so all ~456 F4IsBad* guards returned "bad": FindWindow(this)->NULL (dead menu clicks),
    // world objects skipped, etc. Add the GCC/clang 64-bit predicates so Linux takes the correct 64-bit path.
    if (a >= 0xFFFFFFFFFFFF0000ull)
        return true; // genuine top 64KB of the 64-bit address space

    switch (a)
    {
    case 0xCCCCCCCCCCCCCCCCull: // uninitialized stack (/RTC)
    case 0xCDCDCDCDCDCDCDCDull: // uninitialized heap (debug new)
    case 0xDDDDDDDDDDDDDDDDull: // freed heap (debug delete)
    case 0xFDFDFDFDFDFDFDFDull: // "no man's land" guard
    case 0xFEEEFEEEFEEEFEEEull: // freed LocalAlloc/HeapFree
        return true;
    }
    if ((a >> 32) ==
        0) // a 32-bit fill sitting in the low dword (e.g. 0x00000000DDDDDDDD)
    {
        switch ((unsigned int)a)
        {
        case 0xCCCCCCCC:
        case 0xCDCDCDCD:
        case 0xDDDDDDDD:
        case 0xFDFDFDFD:
        case 0xFEEEFEEE:
        case 0xBAADF00D:
        case 0xABABABAB:
            return true;
        }
    }
#else
    if (a >= 0xFFFF0000)
        return true;   // x86: genuine top 64KB

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
#endif
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
