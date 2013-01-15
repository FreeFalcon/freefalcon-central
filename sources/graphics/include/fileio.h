#ifndef _3DEJ_FILEIO_H_
#define _3DEJ_FILEIO_H_

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <sys\stat.h>
#include "define.h"

#define	FILEIO_SEEK_SET	0
#define	FILEIO_SEEK_CUR	1	
#define	FILEIO_SEEK_END	2

class CFileIO {
//-------------------------------------------------------------------

int	file;

//-------------------------------------------------------------------
public:
	CFileIO () {
		file = -1;
	}
	virtual ~CFileIO () {
		if (file != -1) {
			closefile();
		}
	}
//-------------------------------------------------------------------
	GLint 		openread (const char *filename);
	GLint 		openwrite (const char *filename, GLint binary=0);
	void 		closefile ();
	GLint		getfilehandle ();
	GLint 		getfileptr ();
	GLint 		getfilesize ();
	GLint 		eof ();
	GLint 		movefileptr (GLint offset=0, GLint origin=FILEIO_SEEK_SET);
	GLbyte 		read_char ();
	GLshort		read_short ();
	GLint 		read_int ();
	GLfloat		read_float ();
	GLdouble	read_double ();
	GLint		readdata (void *buf, GLint len);
	void		read_string (char *string);
	GLint		writedata (void *buf, GLint len=0);
//-------------------------------------------------------------------
};

#endif
