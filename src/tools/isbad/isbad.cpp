#include "IsBad.h"
#include "windows.h"

#pragma warning(disable:4800)

// sfr: temp test
#include <stdio.h>

bool F4IsBadReadPtr(const void* lp, unsigned int ucb){
	BOOL ret = IsBadReadPtr(lp, ucb);
	if (ret){
		printf("bla");
	}
	return ret ? true : false;
}

bool F4IsBadCodePtr(void* lpfn){
	BOOL ret = IsBadCodePtr((FARPROC) lpfn);
	if (ret){
		printf("bla");
	}
	return ret ? true : false;
}

bool F4IsBadWritePtr(void* lp, unsigned int ucb){
	BOOL ret = IsBadWritePtr(lp, ucb);
	if (ret){
		printf("bla");
	}
	return ret ? true : false;
}

extern "C" int F4IsBadReadPtrC(const void* lp, unsigned int ucb){
	return F4IsBadReadPtr(lp, ucb);
}

extern "C" int F4IsBadCodePtrC(void* lpfn){
	return F4IsBadCodePtr(lpfn);
}

extern "C" int F4IsBadWritePtrC(void* lp, unsigned int ucb){
	return F4IsBadWritePtr(lp, ucb);
}