#ifndef _3DEJ_IMAGE_H_
#define _3DEJ_IMAGE_H_

//___________________________________________________________________________

#include "grinline.h"
#include "filemem.h"

//___________________________________________________________________________
// Error codes

#define	NO_CODE					-1
#define	GOOD_READ				0
#define	GOOD_WRITE				0
#define	BAD_FILE				1
#define	BAD_READ				2
#define	BAD_WRITE				2
#define	UNEXPECTED_EOF			3
#define	BAD_CODE				4
#define	BAD_FIRSTCODE			5
#define	BAD_ALLOC				6
#define	BAD_SYMBOLSIZE			7
#define	BAD_BITMAPWIDTH			8
#define	BAD_COMPRESSION			9
#define	BAD_COLORDEPTH			10
#define	BAD_FORMAT				11

//___________________________________________________________________________

#pragma pack (push, 1)

// TGA
struct TGA_HEADER {
	GLubyte		identsize, colormaptype, imagetype;
	GLushort	colormapstart, colormaplength;
	GLubyte		colormapbits;
	GLushort	xstart, ystart, width, height;
	GLubyte		bits, descriptor;
};

// BMP files header
struct BMP_HEADER {
	GLshort		bfType;
	GLint		bfSize;
	GLint		bfres;
	GLint		bfOffs;
};

struct BMP_INFO {
	GLint		biSize;
	GLint		biWidth;
	GLint		biHeight;
	GLshort		biPlanes;
	GLshort		biBitCount;
	GLint		biCompression;
	GLint		biSizeImage;
	GLint		biXPelsPerMetre;
	GLint		biYPelsPerMetre;
	GLint		biClrUsed;
	GLint		biClrImportant;
};

struct BMP_RGBQUAD {
	GLubyte	rgbBlue;
	GLubyte	rgbGreen;
	GLubyte	rgbRed;
	GLubyte	rgbres;
};

// gif file typedefs
struct GIFHEADER {
	GLbyte  	sig[6];
	GLushort	screenwidth,screenheight;
	GLubyte 	flags, background, aspect;
};

struct IMAGEBLOCK {
	GLushort	left, top, width, height;
	GLubyte 	flags;
};

struct CONTROLBLOCK {
	GLbyte  	blocksize, flags;
	GLushort	delay;
	GLbyte  	transparent_colour, terminator;
};

struct PLAINTEXT {
	GLbyte  	blocksize;
	GLushort	left, top, gridwidth, gridheight;
	GLbyte  	cellwidth, cellheight, forecolour, backcolour;
};

struct APPLICATION {
	GLbyte  	blocksize, applstring[8], authentication[3];
};

// lbm file typedef
struct LBM_BMHD {
	GLshort	width, height;
	GLshort	x, y;
	GLbyte	nPlanes, masking, compression, pad1;
	GLushort	transparentColor;
	GLbyte 	xAspect, yAspect;
	GLshort	pageW, pageH;
};

// pcx file typedef
struct PCXHEAD {
	GLbyte	manufacturer, version, encoding, bits_per_pixel;
	GLshort	xmin, ymin, xmax, ymax, hres, vres;
	GLbyte  	palette[48], reserved, colour_planes;
	GLshort 	bytes_per_line, palette_type;
	GLbyte  	filler[58];
};

// apl file typedef
struct APL_HEADER {
	GLuint		magic;
	GLushort	width;
	GLushort	height;
};

#pragma pack (pop)

//___________________________________________________________________________
// Exported function prototypes

GLubyte	*ConvertImage (GLImageInfo *fi, GLint mode, GLuint *chromakey = NULL);
GLint 	UnpackGIF (CImageFileMemory *fi);
GLint 	UnpackLBM (CImageFileMemory *fi);
GLint 	UnpackPCX (CImageFileMemory *fi);
GLint	ReadBMP (CImageFileMemory *fi);
GLint	ReadAPL (CImageFileMemory *fi);
GLint	ReadTGA (CImageFileMemory *fi);

GLint	ReadDDS(CImageFileMemory *fi); //JAM 22Sep03

GLint	WritePCX(int fileHandle, GLImageInfo *image);

GLint	GIF_UnpackImage (GLint bits, CImageFileMemory *fi,GLint currentFlag);
void   	GIF_SkipExtension (CImageFileMemory *fi);
GLulong	*ReadLBMColorMap (CImageFileMemory *fi);
GLubyte	*ReadLBMBody (CImageFileMemory *fi, LBM_BMHD *lpHeader,GLint doIFF);

//___________________________________________________________________________

inline GLint CheckImageType (const char *file)
{
	char	ext[10];
	GLint	i;

	glGetFileExtension (file, ext);
	if (!strnicmp (ext, "GIF", 3)) 
		i = IMAGE_TYPE_GIF;
	else if (!strnicmp (ext,"LBM",3)) 
		i = IMAGE_TYPE_LBM;
	else if (!strnicmp (ext,"PCX",3)) 
		i = IMAGE_TYPE_PCX;
	else if (!strnicmp (ext,"BMP",3)) 
		i = IMAGE_TYPE_BMP;
	else if (!strnicmp (ext,"APL",3)) 
		i = IMAGE_TYPE_APL;
	else if (!strnicmp (ext,"TGA",3)) 
		i = IMAGE_TYPE_TGA;

	//JAM 22Sep03
	else if (!strnicmp (ext,"DDS",3))
		i = IMAGE_TYPE_DDS;
	//JAM

	else
		i = IMAGE_TYPE_UNKNOWN;

	return	i;
};

inline GLint ReadTextureImage (CImageFileMemory *fi)
{
	GLint	i;
	switch (fi -> imageType) {
		case IMAGE_TYPE_GIF:
			i = UnpackGIF (fi);
			break;
		case IMAGE_TYPE_LBM:
			i = UnpackLBM (fi);
			break;
		case IMAGE_TYPE_PCX:
			i = UnpackPCX (fi);
			break;
		case IMAGE_TYPE_BMP:
			i = ReadBMP (fi);
			break;
		case IMAGE_TYPE_APL:
			i = ReadAPL (fi);
			break;
		case IMAGE_TYPE_TGA:
			i = ReadTGA (fi);
			break;

		//JAM 22Sep03
		case IMAGE_TYPE_DDS:
			i = ReadDDS (fi);
			break;
		//JAM

		default:
			i = NO_CODE;
			break;
	}
	fi -> glCloseFileMem ();
	return i;
};

inline GLint   motr2intl (GLint l)
{
	return(((l & 0xff000000L) >> 24) + ((l & 0x00ff0000L) >> 8) +
		   ((l & 0x0000ff00L) << 8) +   ((l & 0x000000ffL) << 24));
}

inline GLint motr2inti (GLint n)
{
	return(((n & 0xff00) >> 8) | ((n & 0x00ff) << 8));
}

//___________________________________________________________________________

#endif