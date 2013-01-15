#ifndef __F4ISBAD__
#define __F4ISBAD__

#ifdef __cplusplus

/** sfr: IsBad.h these functions are very useful for DEBUGGING. Dont you fuck use this to fix stuff like JB did. */

bool F4IsBadReadPtr(const void* lp, unsigned int ucb);
bool F4IsBadCodePtr(void* lpfn);
bool F4IsBadWritePtr(void* lp, unsigned int ucb);

#else

/** sfr: C version of those functions. Its good to have them so we can place breakpoints. */

int F4IsBadReadPtrC(const void* lp, unsigned int ucb);
int F4IsBadCodePtrC(void* lpfn);
int F4IsBadWritePtrC(void* lp, unsigned int ucb);

#endif

#endif