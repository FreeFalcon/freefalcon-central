#ifndef	EXCEPTIONHANDLER_H
#define	EXCEPTIONHANDLER_H

// Copyright © 1998 Bruce Dawson.

// We forward declare PEXCEPTION_POINTERS so that the function
// prototype doesn't needlessly require windows.h.
typedef struct _EXCEPTION_POINTERS EXCEPTION_POINTERS, *PEXCEPTION_POINTERS;
int __cdecl RecordExceptionInfo(PEXCEPTION_POINTERS data, const char *Message);

// Call this function if you want to test the exception handler by crashing.
// Pass any integer to specify a particular way of crashing.
void __cdecl CrashTestFunction(int CrashCode);

/*
// Sample usage - put the code that used to be in main into HandledMain.
// To hook it in to an MFC app add exceptionattacher.cpp from the mfctest
// application into your project.
int main(int argc, char *argv[])
{
	int Result = -1;
	__try
	{
		Result = HandledMain(argc, argv);
	}
	__except(RecordExceptionInfo(GetExceptionInformation(), "main thread"))
	{
		// Do nothing here - RecordExceptionInfo() has already done
		// everything that is needed. Actually this code won't even
		// get called unless you return EXCEPTION_EXECUTE_HANDLER from
		// the __except clause.
	}
	return Result;
}
*/

#endif
