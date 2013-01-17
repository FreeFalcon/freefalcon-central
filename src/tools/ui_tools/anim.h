#ifndef _ANIM_H_
#define _ANIM_H_

#define RLE_END			0x8000
#define RLE_SKIPROW		0x4000
#define RLE_SKIPCOL		0x2000
#define RLE_REPEAT			0x1000
#define RLE_NO_DATA		0xffff

#define RLE_KEYMASK		0xf000
#define RLE_COUNTMASK	0x0fff

#define COMP_NONE  0
#define COMP_RLE   1
#define COMP_DELTA 2

typedef struct
{
	char  Header[4];
	long  Version;
	long  Width;
	long  Height;
	long  Frames;
	short Compression;
	short BytesPerPixel;
	long  Background;
	char  Start[];
} ANIMATION;

typedef struct
{
	long Size;
	char Data[];
} ANIM_FRAME;

#endif // _ANIM_H_
