// Artscout - 2026 (#104, Linux Ф1): <conio.h> stub -- MSVC console I/O. Old campaign code pulls it; the console
// funcs are dev-only / unused at runtime. _getch/_kbhit map to stdio no-ops. Linux-only.
#ifndef FF_WIN32SHIM_CONIO_H
#define FF_WIN32SHIM_CONIO_H
#include <cstdio>
static inline int _getch(void)
{
    return getchar();
}
static inline int _getche(void)
{
    return getchar();
}
static inline int _kbhit(void)
{
    return 0;
}
static inline int _putch(int c)
{
    return putchar(c);
}
static inline int _cputs(const char* s)
{
    return fputs(s, stdout);
}
#define getch _getch
#define getche _getche
#define kbhit _kbhit
#define putch _putch
#endif
