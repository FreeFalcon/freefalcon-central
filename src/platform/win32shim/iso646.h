// Artscout - 2026 (#104, Linux Ф1): <iso646.h> the way MSVC means it -- the alternative operators as MACROS.
//
// This is not pedantry, it is required by the codebase. In standard C++ `and`/`or`/`not` are KEYWORDS and <iso646.h>
// is empty; MSVC (without /permissive-) instead ships them as macros. The engine depends on the macro model in two
// opposite directions at once:
//   1. it spells operators as words everywhere (`if (a and b)`), and
//   2. dxdefines.h does `#undef or` ... `#define or ||` around an inline asm block -- which is legal only if `or`
//      is a macro. Against a compiler where it is a keyword, that line is a hard error.
// So the Linux build compiles with -fno-operator-names (turning the keywords off) and force-includes this header,
// which reinstates them as macros. Both spellings then behave exactly as they do on Windows.
//
// Force-included by the build (see the ffplatform/CMake flags), because plenty of TUs use `and` without including
// anything -- they inherit MSVC's built-in behaviour on Windows.
#ifndef FF_WIN32SHIM_ISO646_H
#define FF_WIN32SHIM_ISO646_H

#ifndef and
#define and &&
#define and_eq &=
#define bitand &
#define bitor |
#define compl ~
#define not !
#define not_eq !=
#define or ||
#define or_eq |=
#define xor ^
#define xor_eq ^=
#endif

#endif // FF_WIN32SHIM_ISO646_H
