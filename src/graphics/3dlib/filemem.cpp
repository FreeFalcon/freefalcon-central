#include "define.h"
#include "grinline.h"
#include "filemem.h"


CImageFileMemory::CImageFileMemory(): CFileMemory()
{
    image.image = NULL;
    image.palette = NULL;
}

GLint CFileMemory::glOpenFileMem(const char *filename)
{
    ShiAssert(buffer == NULL);

    // Open the file and get its size
    if ( not CurrentFile.openread(filename))
        return -1; // JPO fail it.

    bytesLeft = CurrentFile.getfilesize();

    if (bytesLeft <= 0)
    {
        bytesLeft = 0;
        return (-1);
    }

    // Allocate memory for the whole file contents
    buffer = (GLubyte*)glAllocateMemory(bytesLeft);

    if ( not buffer)
    {
        CurrentFile.closefile();
        bytesLeft = 0;
        return (-1);
    }

    // Store some handy pointers into our memory buffer
    CurrentMemoryPointer = buffer;
    bufferEnd = buffer + bytesLeft;

    return (1);
} /* glOpenFileMem */

void CFileMemory::glReadFileMem()
{
    ShiAssert(buffer);
    CurrentFile.readdata(buffer, bytesLeft);
}

void CFileMemory::glCloseFileMem()
{
    CurrentFile.closefile();
    glReleaseMemory((char *)buffer);
    buffer = NULL;
    bytesLeft = 0;
} /* glCloseFileMem */

GLuint CFileMemory::glReadCharMem()
{
    ShiAssert(buffer);
    ShiAssert(bytesLeft > 0);

    bytesLeft--;
    return *CurrentMemoryPointer++;
} /* glReadCharMem */

GLint CFileMemory::glReadMem(void *target, GLint total)
{
    ShiAssert(buffer);

    if (total >= bytesLeft) total = bytesLeft;

    memcpy(target, CurrentMemoryPointer, total);
    CurrentMemoryPointer += total;
    bytesLeft -= total;

    return (total);
} /* glReadMem */

GLint CFileMemory::glSetFilePosMem(GLint offset, GLint mode)
{
    ShiAssert(buffer);

    switch (mode)
    {
        case 0:
            CurrentMemoryPointer = buffer + offset;
            break;

        case 1:
            CurrentMemoryPointer = CurrentMemoryPointer + offset;
            break;

        case 2:
            CurrentMemoryPointer = bufferEnd + offset;
            break;
    }

    if (CurrentMemoryPointer < buffer) return -1;

    if (CurrentMemoryPointer >= bufferEnd) return 1;

    return 0;
} /* glSetFilePosMem */

