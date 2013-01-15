#ifndef _ART_RESOURCE_H_
#define _ART_RESOURCE_H_

enum
{
	_RSC_8_BIT_		=0x00000001,
	_RSC_16_BIT_	=0x00000002,
	_RSC_COLORKEY_	=0x40000000,

	_RSC_IS_IMAGE_	=100,
	_RSC_IS_SOUND_	=101,
	_RSC_IS_FLAT_	=102,
};

struct ImageFmt
{
	long  Type;
	char	ID[32];
	long	flags;
	short	centerx;
	short	centery;
	short	w;
	short	h;
	long	imageoffset;
	long	palettesize;
	long	paletteoffset;
};

struct SoundFmt
{
	long  Type;
	char  ID[32];
	long  flags;
	short Channels;
	short SoundType;
	long  offset;
	long  headersize;
};

struct FlatFmt
{
	long  Type;
	char  ID[32];
	long  offset;
	long  size;
};

#endif

