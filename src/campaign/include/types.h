/* Types.h - integer types defined for portability between compilers and
 * notational convenience.
 *
 * Copyright (c) 1992 Jim Kent.  This file may be freely used, modified,
 * copied and distributed.  This file was first published as part of
 * an article for Dr. Dobb's Journal March 1993 issue.
 */

#ifndef TYPES_H         /* Prevent file from being included twice. */
#define TYPES_H

typedef unsigned char UBYTE, BYTE;
typedef unsigned short USHORT;
typedef unsigned long ULONG;

typedef signed char SBYTE;
typedef signed short SSHORT, SHORT;
typedef signed long SLONG, LONG;

// Artscout - 2026 (Linux port): the win32 shim already defines DWORD as uint32_t (correct 32-bit
// width under LP64). Redefining it as `unsigned long` (64-bit on Linux) conflicts; skip when the
// shim is active. On Windows FF_WIN32SHIM_WINDOWS_H is never defined, so this is unchanged there.
#ifndef FF_WIN32SHIM_WINDOWS_H
typedef unsigned long DWORD;
#endif
typedef unsigned short WORD;

typedef int COUNTER;
typedef int INDEX;

typedef void *ANY_PTR;
typedef char *STR_PTR;

typedef int Boolean; /* TRUE or FALSE value. */
typedef int ErrCode; /* ErrXXX or Success. */
typedef int FileHandle; /* OS file handle. */

/* Values for ErrCodes */
#define Success 0 /* Things are fine. */
#define Error -1 /* Unclassified error. */
/* Various error codes flic reader can get. */
#define ErrNoMemory -2 /* Not enough memory. */
#define ErrBadFlic -3 /* File isn't a flic. */
#define ErrBadFrame -4 /* Bad frame in flic. */
#define ErrOpen -5 /* Couldn't open file.  Check errno. */
#define ErrRead -6 /* Couldn't read file.  Check errno. */
#define ErrDisplay -7 /* Couldn't open display. */
#define ErrClock -8 /* Couldn't open clock. */
#define ErrKey -9 /* Couldn't open keyboard. */
#define ErrCancel -10 /* User cancelled. */
#define ErrIsFLI -11 /* Doesnt support FLI */

#endif /* TYPES_H */
