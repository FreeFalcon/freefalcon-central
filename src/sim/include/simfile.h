/******************************************************************************/
/*                                                                            */
/*  Unit Name : simfile.h                                                     */
/*                                                                            */
/*  Abstract  : Header file with class definition for SIMLIB_FILE_CLASS and   */
/*              defines used in its implementation.                           */
/*                                                                            */
/*  Dependencies : Auto-Generated                                             */
/*                                                                            */
/*  Operating System : MS-DOS 6.2, Windows 3.1                                */
/*                                                                            */
/*  Compiler : MSVC V1.5                                                      */
/*                                                                            */
/*  Naming Conventions :                                                      */
/*                                                                            */
/*      Public Functions    : Mixed case with no underscores                  */
/*      Private Functions   : Mixed case with no underscores                  */
/*      Public Functions    : Mixed case with no underscores                  */
/*      Global Variables    : Mixed case with no underscores                  */
/*      Classless Functions : Mixed case with no underscores                  */
/*      Classes             : All upper case seperated by an underscore       */
/*      Defined Constants   : All upper case seperated by an underscore       */
/*      Macros              : All upper case seperated by an underscore       */
/*      Structs/Types       : All upper case seperated by an underscore       */
/*      Private Variables   : All lower case seperated by an underscore       */
/*      Public Variables    : All lower case seperated by an underscore       */
/*      Local Variables     : All lower case seperated by an underscore       */
/*                                                                            */
/*  Development History :                                                     */
/*  Date      Programer           Description                                 */
/*----------------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                               */
/*                                                                            */
/******************************************************************************/
#ifndef _SIMFILE_H
#define _SIMFILE_H

#define SIMLIB_MAX_FILE_NAME_LENGTH    _MAX_PATH
#define SIMLIB_MAX_OPEN_FILES          20
#define SIMLIB_UPDATE                  0x1
#define SIMLIB_CREATE                  0x2
#define SIMLIB_READ                    0x4
#define SIMLIB_WRITE                   0x8
#define SIMLIB_BINARY                  0x10
#define SIMLIB_READWRITE               (SIMLIB_READ & SIMLIB_WRITE)

#define SIMFILE_CUR                    SEEK_CUR
#define SIMFILE_START                  SEEK_SET
#define SIMFILE_END                    SEEK_END

typedef char   SimlibFileName[SIMLIB_MAX_FILE_NAME_LENGTH];

/*-----------------*/
/* Library Classes */
/*-----------------*/
class SimlibFileClass
{
	private:
	   FILE           *fptr;
	   int            rights;
	   int            lastOp;
	   SimlibFileName fName;

	public:
		SimlibFileClass (void);
		static SimlibFileClass* Open (char *fname, int flags);
		int ReadLine (char *buf, int maxLen);
		int WriteLine (char *buf);
		int Read (void *buffer, unsigned int maxLen);
		int Write (void *buffer, int maxLen);
		char *GetNext (void);
		int Close(void);
      int Position (int offset, int origin);
      int GetPosition (void);
};

#endif
