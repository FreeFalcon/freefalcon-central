#ifndef PORTIO_COMPAT_H
#define PORTIO_COMPAT_H
//*******************************************************************************************************
// portio_compat.h  --  port-I/O stubs (_outp/_outpw/_inp/_inpw)
//
// These functions lived in <conio.h> of old MSVC. In the modern CRT the declarations are gone, BUT the names remain
// compiler intrinsics -- so we can't provide our own definition (an inline function) (C2169 'cannot
// define an intrinsic'). Define them as no-op function-like MACROS: the macro expands at the
// preprocessing stage, before the name is recognized as an intrinsic, and there is no actual port-I/O.
//
// They were used only to write to the Mono card (a monochrome debug monitor) via ports.
// On modern Windows direct port access from user-mode is forbidden (GP-fault), and the hardware is long gone.
//
// IMPORTANT: include AFTER <conio.h> so the macro doesn't break a possible declaration in conio.h.
//*******************************************************************************************************

#undef _outp
#undef _outpw
#undef _inp
#undef _inpw

#define _outp(port, v) ((int)(v)) // byte write -- no-op, returns the value
#define _outpw(port, v)                                                        \
    ((unsigned short)(v)) // word write -- no-op, returns the value
#define _inp(port) (0) // byte read -- always 0
#define _inpw(port) ((unsigned short)0) // word read -- always 0

// Non-_MSC_VER compilers (clang on the Linux build) take the un-prefixed <conio.h> spellings (outp/outpw/inp/inpw,
// e.g. Mono2d.cpp's `#else` branch). Those names are not clang intrinsics, so map them to the same no-ops.
#undef outp
#undef outpw
#undef inp
#undef inpw
#define outp(port, v) ((int)(v))
#define outpw(port, v) ((unsigned short)(v))
#define inp(port) (0)
#define inpw(port) ((unsigned short)0)

#endif // PORTIO_COMPAT_H
