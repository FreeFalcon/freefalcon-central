/******************************************************************************/
/*                                                                            */
/*  Unit Name : file.cpp                                                      */
/*                                                                            */
/*  Abstract  : Source file for functions implementing the SimlibFileClass.   */
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
#include "stdhdr.h"
#include "direct.h"
#include "simfile.h"
#include "errno.h"
#include "error.h"
#include "f4find.h"
#include "falclib.h"
#include "F4Thread.h"
#include "cmpclass.h"
extern "C" {
#include "codelib/resources/reslib/src/resmgr.h"
};

void SwapCRLF(char *buf);

/*-------------------*/
/* Private Functions */
/*-------------------*/
#ifdef _DEBUG
int fnumOpen = 0;
#endif

/*----------------------------------------------------*/
/* Memory Allocation for externals declared elsewhere */
/*----------------------------------------------------*/
SIM_FLOAT SimLibMinorFrameTime = 0.02F;
SIM_FLOAT SimLibMinorFrameRate = 50.0F;
SIM_FLOAT SimLibMajorFrameTime = 0.06F;
SIM_FLOAT SimLibMajorFrameRate = 16.667F;
SIM_FLOAT SimLibTimeOfDay;
SIM_ULONG SimLibElapsedTime;
float SimLibElapsedSeconds; // COBRA - RED - Added Variable of Elasped Simulation Seconds
float SimLibFrameElapsed, SimLibLastFrameTime; // COBRA - RED - Added Variable of Elasped Frame Time
SIM_UINT SimLibFrameCount = 0;
SIM_INT SimLibMinorPerMajor = 3;

/*------------------*/
/* Public Functions */
/*------------------*/

/********************************************************************/
/*                                                                  */
/* Routine: SimlibFileClass::SimlibFileClass (void)             */
/*                                                                  */
/* Description:                                                     */
/*    Constructor function for the SimlibFileClass.  Initializes  */
/*    the file data array to all unused.                            */
/*                                                                  */
/* Inputs:                                                          */
/*    None                                                          */
/*                                                                  */
/* Outputs:                                                         */
/*    None                                                          */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SimlibFileClass::SimlibFileClass(void)
{
    fptr = NULL;
    rights = 0;
    lastOp = -1;
}

/********************************************************************/
/*                                                                  */
/* Routine: SimlibFileClass* SimlibFileClass::Open                  */
/*            (char *, SIM_INT)                                     */
/*                                                                  */
/* Description:                                                     */
/*    Open the named file with the desired permissions and */
/*    attributes.                                                   */
/*                                                                  */
/* Inputs:                                                          */
/*    SIM_FILE_NAME fname - Name of the file to open                */
/*    SIM_INT flags       - Permissions and attributes desired      */
/*                                                                  */
/* Outputs:                                                         */
/*    File handle of the open file or NULL on error                 */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
SimlibFileClass* SimlibFileClass::Open(char *fName, int flags)
{
    SimlibFileClass* fHandle;
    char access[4] = {0};
    char fileName[_MAX_PATH];
    int offset, len;

    // What do we want to do with the file ?
    if (flags bitand SIMLIB_UPDATE)
    {
        if (flags bitand SIMLIB_READWRITE)
            strcpy(access, "r+\0");
        else if (flags bitand SIMLIB_WRITE)
            strcpy(access, "a+\0");
    }
    else if (flags bitand SIMLIB_READ)
    {
        strcpy(access, "r\0");
    }
    else if (flags bitand SIMLIB_READWRITE)
    {
        strcpy(access, "w+\0");
    }
    else if (flags bitand SIMLIB_WRITE)
    {
        strcpy(access, "w\0");
    }

    // Is this a binary file ?
    if (flags bitand SIMLIB_BINARY)
        strcat(access, "b\0");
    else
        strcat(access, "t\0");

    // Find the file in the database
    if (flags bitand SIMLIB_READ)
    {
        if (F4FindFile(fName, fileName, _MAX_PATH, &offset, &len) == NULL)
            strcpy(fileName, fName);
    }
    else
        strcpy(fileName, fName);

    // Try the actual open
    fHandle = new SimlibFileClass;
    fHandle->fptr = ResFOpen(fileName, access);

    // Did it open ?
    if (fHandle->fptr == NULL)
    {
        delete fHandle;
        fHandle = NULL;

        if (errno == ENOENT)
            SimLibErrno = ENOTFOUND;
        else
            SimLibErrno = EACCESS;

        MonoPrint("Unable to open %s\n", fName);
    }
    else
    {
#ifdef _DEBUG
        fnumOpen ++;
#endif

        // Set the file data for this handle
        fHandle->rights = flags;
        fHandle->lastOp = -1;

        // Fully qualified pathname for resources
        strcpy(fHandle->fName, fileName);
    }

    // Return the handle or SIMLIB_ERR
    return (fHandle);
}

/********************************************************************/
/*                                                                  */
/* Routine: int SimlibFileClass::ReadLine (char *, int)             */
/*                                                                  */
/* Description:                                                     */
/*    Read from the given file up to the next new-line or max len   */
/*                                                                  */
/* Inputs:                                                          */
/*    char *buf      - String buffer to fill with data              */
/*    int max_len    - Maximum number of characters to read         */
/*                                                                  */
/* Outputs:                                                         */
/*    SIMLIB_OK on success, SIMLIB_ERR on failure                   */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
int SimlibFileClass::ReadLine(char *buf, int max_len)
{
    int retval = SIMLIB_ERR;

    F4Assert(fptr);
    F4Assert(rights bitand SIMLIB_READ);

    char *cp;

    // Skip Comments
    while ((cp = fgets(buf, max_len, fptr)) not_eq NULL)
    {
        //RESMANAGER KLUDGE
        SwapCRLF(buf);

        //            if (buf[0] not_eq ';' and buf[0] not_eq '\r')
        if (buf[0] not_eq ';' and buf[0] not_eq '\n')
            break;
    }

    if (cp == NULL)
    {
        SimLibErrno = EEOF;
        return retval;
    }

    //RESMANAGER KLUDGE
    if (strchr(buf, '\r'))
        *(strchr(buf, '\r')) = 0;

    // Strip the trailing new-line
    if ( not feof(fptr))
    {
        if (buf[strlen(buf) - 1] == '\n')
            buf[strlen(buf) - 1] = 0;

        retval = SIMLIB_OK;
        lastOp = SIMLIB_READ;
    }
    else
        SimLibErrno = EEOF;

    return (retval);
}

/********************************************************************/
/*                                                                  */
/* Routine: int SimlibFileClass::WriteLine (char *)                 */
/*                                                                  */
/* Description:                                                     */
/*    Write the given line onto the file.                           */
/*                                                                  */
/* Inputs:                                                          */
/*    char *buf          - String buffer to write out               */
/*                                                                  */
/* Outputs:                                                         */
/*    SIMLIB_OK on success, SIMLIB_ERR on failure                   */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
int SimlibFileClass::WriteLine(char *buf)
{
    SIM_INT retval = SIMLIB_ERR;

    // Can we write
    F4Assert(fptr);
    F4Assert(rights bitand SIMLIB_WRITE);

    if (fprintf(fptr, "%s\n", buf) < 0)
        SimLibErrno = EOUTPUT;
    else
    {
        retval = SIMLIB_OK;
        lastOp = SIMLIB_WRITE;
    }

    return (retval);
}

/********************************************************************/
/*                                                                  */
/* Routine: int SimlibFileClass::Read (void*, int)                  */
/*                                                                  */
/* Description:                                                     */
/*    Read from the given file up to max_len bytes                  */
/*                                                                  */
/* Inputs:                                                          */
/*    void *buf      - Buffer to fill with data                     */
/*    int max_len    - Maximum number of bytes to read              */
/*                                                                  */
/* Outputs:                                                         */
/*    SIMLIB_OK on success, SIMLIB_ERR on failure                   */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
int SimlibFileClass::Read(void* buffer, unsigned int max_len)
{
    int retval = SIMLIB_ERR;

    F4Assert(fptr);
    F4Assert(rights bitand SIMLIB_READ);

    if (fread(buffer, 1, max_len, fptr) < max_len)
        SimLibErrno = EEOF;
    else
    {
        retval = SIMLIB_OK;
        lastOp = SIMLIB_READ;
    }

    return (retval);
}

/********************************************************************/
/*                                                                  */
/* Routine: int SimlibFileClass::Write (void*, int)                 */
/*                                                                  */
/* Description:                                                     */
/*    Write max_len bytes on the given file.                        */
/*                                                                  */
/* Inputs:                                                          */
/*    void* buf      - Buffer of data to write                      */
/*    int max_len    - Number of bytes to write                     */
/*                                                                  */
/* Outputs:                                                         */
/*    SIMLIB_OK on success, SIMLIB_ERR on failure                   */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
int SimlibFileClass::Write(void* buffer, int max_len)
{
    SIM_INT retval = SIMLIB_ERR;

    F4Assert(fptr);
    F4Assert(rights bitand SIMLIB_WRITE);

    if (fwrite(buffer, 1, max_len, fptr) < (unsigned int)max_len)
        SimLibErrno = EOUTPUT;
    else
    {
        retval = SIMLIB_OK;
        lastOp = SIMLIB_WRITE;
    }

    return (retval);
}

/********************************************************************/
/*                                                                  */
/* Routine: char* SimlibFileClass::GetNext(void)                    */
/*                                                                  */
/* Description:                                                     */
/*    Read the next element from the given file.                    */
/*                                                                  */
/* Inputs:                                                          */
/*    none                                                          */
/*                                                                  */
/* Outputs:                                                         */
/*    Next string found in the file.                                */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
char *SimlibFileClass::GetNext(void)
{
    static char aline[160];
    int is_comment;

    // Can we read
    F4Assert(fptr);
    F4Assert(rights bitand SIMLIB_READ);

    do
    {
        fscanf(fptr, "%s", aline);
        SwapCRLF(aline);

        if (aline[0] == ';' or aline[0] == '#')
        {
            if (fgets(aline, 160, fptr) == NULL)
            {
                break;
            }

            is_comment = TRUE;
        }
        else
        {
            is_comment = FALSE;
        }
    }
    while (is_comment);

    return (aline);
}

/********************************************************************/
/*                                                                  */
/* Routine: int SimlibFileClass::Close(void)                        */
/*                                                                  */
/* Description:                                                     */
/*    Close the file associated with the given handle.              */
/*                                                                  */
/* Inputs:                                                          */
/*    None                                                          */
/*                                                                  */
/* Outputs:                                                         */
/*    SIMLIB_OK on success, SIMLIB_ERR on failure                   */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
int SimlibFileClass::Close(void)
{
    SIM_INT retval = SIMLIB_ERR;

    F4Assert(fptr);

    if (ResFClose(fptr) == 0)
    {
#ifdef _DEBUG
        fnumOpen --;
#endif
        fptr = NULL;
        rights = 0;
        lastOp = -1;
        retval = SIMLIB_OK;
    }
    else
        SimLibErrno = EACCESS;

    return (retval);
}

/********************************************************************/
/*                                                                  */
/* Routine: int SimlibFileClass::Position(int offset, int origin)   */
/*                                                                  */
/* Description:                                                     */
/*    Set the file ptr to the appropriate place                     */
/*                                                                  */
/* Inputs:                                                          */
/*    offset - number of bytes to move                              */
/*    orgin  - where to start counting from                         */
/*                                                                  */
/* Outputs:                                                         */
/*    SIMLIB_OK on success, SIMLIB_ERR on failure                   */
/*                                                                  */
/*  Development History :                                           */
/*  Date      Programer           Description                       */
/*------------------------------------------------------------------*/
/*  23-Jan-95 LR                  Initial Write                     */
/*                                                                  */
/********************************************************************/
int SimlibFileClass::Position(int offset, int origin)
{
    int retval = SIMLIB_ERR;

    F4Assert(fptr);

    if (fseek(fptr, offset, origin) == 0)
        retval = SIMLIB_OK;

    return (retval);
}

void SwapCRLF(char* buf)
{
    int len = strlen(buf);
    int i;

    for (i = 0; i < len; i++)
    {
        if (buf[i] == '\r')
        {
            memmove(buf + i, buf + i + 1, len - i - 1);
            buf[len - 1] = 0;
        }
    }
}
