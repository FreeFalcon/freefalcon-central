#ifndef _RLE_H_
#define _RLE_H_

long CompressRLE8Bit(uchar *Src,uchar *Dest,long srcsize);
long DecompressRLE8Bit(uchar *Src,uchar *Dest,long Size);

long CompressRLE16Bit(WORD *Src,WORD *Dest,long srcsize);
long DecompressRLE16Bit(WORD *Src,WORD *Dest,long Size);

#endif