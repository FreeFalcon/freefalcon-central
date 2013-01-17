#ifndef _3DEJ_FILEMEM_H_
#define _3DEJ_FILEMEM_H_

//___________________________________________________________________________

#include "define.h"
#include "fileio.h"

//___________________________________________________________________________

class CFileMemory {
  public:
	CFileMemory()			{ buffer = NULL; bytesLeft = 0; };
	virtual ~CFileMemory()	{};

	GLint		glOpenFileMem (const char *filename);
	void		glReadFileMem ();
	void		glCloseFileMem ();

	GLuint		glReadCharMem ();
	GLint		glReadMem (void *target, GLint totalbytes);
	GLint		glSetFilePosMem (GLint offset, GLint mode);

	GLint		glBytesLeft ()		{ return bytesLeft; };
	void		*glBufferAddress ()	{ return buffer; };
	int			glFileHandle ()	{ return CurrentFile.getfilehandle(); };

  protected:
	GLubyte		*buffer;
	GLubyte		*bufferEnd;
	GLubyte		*CurrentMemoryPointer;
	GLint		bytesLeft;

	CFileIO		CurrentFile;
};

//___________________________________________________________________________

class CImageFileMemory : public CFileMemory {
public:
	CImageFileMemory();
	virtual ~CImageFileMemory() {};
	GLint		imageType;
	GLImageInfo	image;
	GLbyte		fileName[_MAX_PATH];
};

#endif
