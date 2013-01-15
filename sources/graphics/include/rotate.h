#include "imagebuf.h"

void RotateBitmapMask (	ImageBuffer *srcbuf, ImageBuffer *destbuf, int angle, 
						RECT *srect=0, RECT *drect=0, int *startstop=0);

void RotateBitmap (	ImageBuffer *src, ImageBuffer *dest, 
					int angle, RECT *srect=0, RECT *drect=0);

void RotateBitmapMaskDouble (	ImageBuffer *srcbuf, ImageBuffer *destbuf, int angle, 
								RECT *srect=0, RECT *drect=0, int *startstop=0);

void RotateBitmapDouble (	ImageBuffer *src, ImageBuffer *dest, 
							int angle, RECT *srect=0, RECT *drect=0);

