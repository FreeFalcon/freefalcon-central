#ifndef _IMAGE_RSC_H_
#define _IMAGE_RSC_H_

//
// This class is TIED to the C_Resmgr class (which holds the actual image data)
//
// (So you need both to actually be able to draw a bitmap)
//


// First item MUST be     short Type
class ImageHeader
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_ART_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		long  Type;
		char  ID[32];
		long  flags;
		short centerx;
		short centery;
		short w;
		short h;
		long  imageoffset;
		long  palettesize;
		long  paletteoffset;
};

class IMAGE_RSC
{
#ifdef USE_SH_POOLS
	public:
		// Overload new/delete to use a SmartHeap pool
		void *operator new(size_t size) { return MemAllocPtr(UI_Pools[UI_ART_POOL],size,FALSE);	};
		void operator delete(void *mem) { if (mem) MemFreePtr(mem); };
#endif
	public:
		long		ID;
		C_Resmgr	*Owner;
		ImageHeader	*Header;

	private:
		// I have a BUNCH of VERY SIMILAR routines to speed up drawing

		// Blitting Functions...
		// using Palettes
		void Blit8BitFast(WORD *dest);
		void Blit8Bit(long doffset,long dwidth,WORD *dest);
		void Blit8BitTransparentFast(WORD *dest);
		void Blit8BitTransparent(long doffset,long dwidth,WORD *dest);
		void Blit8BitPart(long soffset,long scopy,long ssize,long doffset,long dwidth,WORD *dest);
		void Blit8BitTransparentPart(long soffset,long scopy,long ssize,long doffset,long dwidth,WORD *dest);

		// Straight Blitting
		void Blit16BitFast(WORD *dest);
		void Blit16Bit(long doffset,long dwidth,WORD *dest);
		void Blit16BitTransparentFast(WORD *dest);
		void Blit16BitTransparent(long doffset,long dwidth,WORD *dest);
		void Blit16BitPart(long soffset,long scopy,long ssize,long doffset,long dwidth,WORD *dest);
		void Blit16BitTransparentPart(long soffset,long scopy,long ssize,long doffset,long dwidth,WORD *dest);

		//XX 16->32
		void _Blit16BitTo32( long doffset, long dwidth, DWORD *dest );
		void _Blit16BitTransparentTo32( long doffset, long dwidth, DWORD *dest );
		void _Blit16BitPartTo32(long soffset,long scopy,long ssize,long doffset,long dwidth, DWORD *dest );
		void _Blit16BitTransparentPartTo32(long soffset,long scopy,long ssize,long doffset,long dwidth, DWORD *dest);

		//XX 8->32
		void _Blit8BitTo32(long doffset,long dwidth, DWORD *dest);		
		void _Blit8BitTransparentTo32(long doffset,long dwidth, DWORD *dest);
		void _Blit8BitPartTo32(long soffset,long scopy,long ssize,long doffset,long dwidth, DWORD *dest);
		void _Blit8BitTransparentPartTo32(long soffset,long scopy,long ssize,long doffset,long dwidth, DWORD *dest);
		


		// Blending Functions
		// using Palettes
		void Blend8BitFast(WORD *dest,long front,long back);
		void Blend8Bit(long doffset,long dwidth,WORD *dest,long front,long back, bool b32); //XX
		void Blend8BitTransparentFast(WORD *dest,long front,long back);
		void Blend8BitTransparent(long doffset,long dwidth,WORD *dest,long front,long back, bool b32 );//XX
		void Blend8BitPart(long soffset,long scopy,long ssize,long doffset,long dwidth,WORD *dest,long front,long back,bool b32);//XX
		void Blend8BitTransparentPart(long soffset,long scopy,long ssize,long doffset,long dwidth,WORD *dest,long front,long back,bool b32);//XX

		// Straight Blending
		void Blend16BitFast(WORD *dest,long front,long back);
		void Blend16Bit(long doffset,long dwidth,WORD *dest,long front,long back,bool b32);//XX
		void Blend16BitTransparentFast(WORD *dest,long front,long back);
		void Blend16BitTransparent(long doffset,long dwidth,WORD *dest,long front,long back,bool b32);//XX
		void Blend16BitPart(long soffset,long scopy,long ssize,long doffset,long dwidth,WORD *dest,long front,long back,bool b32);//XX
		void Blend16BitTransparentPart(long soffset,long scopy,long ssize,long doffset,long dwidth,WORD *dest,long front,long back,bool b32);//XX

	public:
		char *GetImage();
		WORD *GetPalette();

// VITAL NOTE:
//  it is VERY important that all the source & destination parameters are
//  within the DESTINATION
//  source checking is provided...
//  DESTINATION is NOT checked whatsoever
//
//  I personally am calling this from within the UI95 code where all clipping has already occured
//
		void Blit(SCREEN *surface,long sx,long sy,long sw,long sh,long dx,long dy);
		void Blend(SCREEN *surface,long sx,long sy,long sw,long sh,long dx,long dy,long front,long back);
		void ScaleDown8(SCREEN *surface,long *Rows,long *Cols,long dx,long dy,long dw,long dh,long offx,long offy);
		void ScaleUp8(SCREEN *surface,long *Rows,long *Cols,long dx,long dy,long dw,long dh);
		void ScaleDown8Overlay(SCREEN *surface,long *Rows,long *Cols,long dx,long dy,long dw,long dh,long offx,long offy,BYTE *ovr,WORD *Palette[]);
		void ScaleUp8Overlay(SCREEN *surface,long *Rows,long *Cols,long dx,long dy,long dw,long dh,BYTE *ovr,WORD *Palette[]);
};

#endif