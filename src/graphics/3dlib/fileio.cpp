#include <cISO646>
#include "stdafx.h"
#include "fileio.h"
#include "GraphicsRes.h"
#include "xmmintrin.h"


GLint CFileIO::openread(const char *filename)
{
    ShiAssert(file == -1); // Make sure we're not double opening this file

    file = GR_OPEN(filename, O_BINARY bitor O_RDONLY);

    return (file >= 0); // Return FALSE if we got a negative number
}

GLint CFileIO::openwrite(const char *filename, GLint binary)
{
    ShiAssert(file == -1); // Make sure we're not double opening this file

    int mode = O_RDWR bitor O_CREAT bitor O_TRUNC;

    if (binary) mode or_eq O_BINARY;
    else mode or_eq O_TEXT;

    file = GR_OPEN(filename, mode);

    return (file >= 0); // Return FALSE if we got a negative number
}

void CFileIO::closefile()
{
    if (file not_eq -1)
    {
        GR_CLOSE(file);
        file = -1;
    }
}

GLint CFileIO::getfilehandle()
{
    return file;
}

GLint CFileIO::getfileptr()
{
    return GR_TELL(file);
}

GLint CFileIO::getfilesize()
{
#ifdef GRAPHICS_USE_RES_MGR
    return ResSizeFile(file);
#else
    GLint size;
    GLint pos;

    pos = tell(file); // Remember where we are in the file
    lseek(file, 0, SEEK_END); // Go to the end
    size = size = tell(file); // Get the length of the file
    lseek(file, pos, SEEK_SET); // Go back to our starting point
    return size;
#endif
}

GLint CFileIO::movefileptr(GLint offset, GLint origin)
{
    return GR_SEEK(file, offset, origin);
}

GLint CFileIO::eof()
{
    return ::eof(file);
}

GLint CFileIO::writedata(void *buf, GLint len)
{
    if (len == 0) len = strlen((char *) buf);

    return GR_WRITE(file, buf, len);
}

GLbyte CFileIO::read_char()
{
    GLbyte data;

    GR_READ(file, &data, 1);
    return data;
}

GLshort CFileIO::read_short()
{
    GLshort data;

    GR_READ(file, &data, 2);
    return data;
}

GLint CFileIO::read_int()
{
    GLint data;

    GR_READ(file, &data, 4);
    return data;
}

GLfloat CFileIO::read_float()
{
    GLfloat data;

    GR_READ(file, &data, 4);
    return data;
}

GLdouble CFileIO::read_double()
{
    GLdouble data;

    GR_READ(file, &data, 8);
    return data;
}

GLint CFileIO::readdata(void *buf, GLint len)
{
    return GR_READ(file, buf, len);
}

void CFileIO::read_string(char *string)
{
    GLbyte data;

    do
    {
        GR_READ(file, &data, 1);
        *string++ = data;
    }
    while (data);
}

