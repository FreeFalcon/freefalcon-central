
#include "stdafx.h"
#include "shi\ConvFtoI.h"
#include "grmath.h"
#include "rotate.h"
#include "Falclib\Include\IsBad.h"


// Rotation with fixed point
void RotateBitmapMask(ImageBuffer *srcbuf, ImageBuffer *destbuf, int angle, RECT *srect, RECT *drect, int *startstop)
{
	ShiAssert( srect );
	ShiAssert( drect );

	if (!startstop)
	{
		RotateBitmap (srcbuf, destbuf, angle, srect, drect);
		return;
	}

	char *source = (char *) srcbuf ->Lock();
	ShiAssert( source );

	if (!source) // JB 011125 CTD fix when ALT Tab
		return;

	char *dest = (char *) destbuf ->Lock();
	ShiAssert( dest );

	if (!dest) // JB 011125 CTD fix when ALT Tab
	{
		srcbuf->Unlock();
		return;
	}

	if(destbuf->PixelSize() == 2)
	{
		int dStride = destbuf -> targetStride();
		int sStride = srcbuf -> targetStride();
		int destx = drect -> left;
		int desty = drect -> top;
		int width = drect -> right - destx;
		int height = drect -> bottom - desty;
		int sx = (srect -> right + srect -> left) >> 1;
		int sy = (srect -> bottom + srect -> top) >> 1;

		float sine, cosine;
		glGetSinCos (&sine, &cosine, angle);
		float x = (float) (width >> 1);
		float y = (float) (height >> 1);
		float xc =  x * cosine; 
		float xs = -x * sine;
		float ys =  y * sine;   
		float yc =  y * cosine;
		x = 1.0f / x; y = 1.0f / y;

		sx += (int) (-xc - ys);
		sy += (int) (-xs - yc);

		char *dptr = dest + (destx << 1) + desty * dStride;
		char *sptr = source + (sx << 1) + sy * sStride;

		float x1 = x * 1024.0f; 
		float y1 = y * 1024.0f; 
		int isdHx = (int) (xc * x1);
		int isdHy = (int) (xs * x1);
		int isdVx = (int) (ys * y1);
		int isdVy = (int) (yc * y1);
		int iyx = 0;
		int iyy = 0;

		int	i;
		for(i=0; i < height; i++)
		{
			int start = *startstop++;
			int stop = *startstop++;
			int k = ((iyx >> 10) << 1) + (iyy >> 10) * sStride;
			char *sptr1 = sptr + k;
			char *dptr1 = dptr + (start << 1);

			int ixx = (int) (start * isdHx);
			int ixy = (int) (start * isdHy);
			int	j;

			for(j=start; j < stop; j++)
			{
				int k = ((ixx >> 10) << 1) + (ixy >> 10) * sStride;
				ixx += isdHx;
				ixy += isdHy;
				short *dptr = (short *) dptr1;
				short *sptr = (short *) (sptr1+k);
				*dptr = *sptr;
				dptr1 += 2;
			}

			dptr += dStride;
			iyx += isdVx;
			iyy += isdVy;
		}
	}

	else if(destbuf->PixelSize() == 4)
	{
		int dStride = destbuf -> targetStride();
		int sStride = srcbuf -> targetStride();
		int destx = drect -> left;
		int desty = drect -> top;
		int width = drect -> right - destx;
		int height = drect -> bottom - desty;
		int sx = (srect -> right + srect -> left) >> 1;
		int sy = (srect -> bottom + srect -> top) >> 1;

		float sine, cosine;
		glGetSinCos (&sine, &cosine, angle);
		float x = (float) (width >> 1);
		float y = (float) (height >> 1);
		float xc =  x * cosine; 
		float xs = -x * sine;
		float ys =  y * sine;   
		float yc =  y * cosine;
		x = 1.0f / x; y = 1.0f / y;

		sx += (int) (-xc - ys);
		sy += (int) (-xs - yc);

		char *dptr = dest + (destx << 2) + desty * dStride;
		char *sptr = source + (sx << 2) + sy * sStride;

		float x1 = x * 1024.0f; 
		float y1 = y * 1024.0f; 
		int isdHx = (int) (xc * x1);
		int isdHy = (int) (xs * x1);
		int isdVx = (int) (ys * y1);
		int isdVy = (int) (yc * y1);
		int iyx = 0;
		int iyy = 0;

		int	i;
		for(i=0; i < height; i++)
		{
			int start = *startstop++;
			int stop = *startstop++;
			int k = ((iyx >> 10) << 2) + (iyy >> 10) * sStride;
			char *sptr1 = sptr + k;
			char *dptr1 = dptr + (start << 2);

			int ixx = (int) (start * isdHx);
			int ixy = (int) (start * isdHy);
			int	j;

			for(j=start; j < stop; j++)
			{
				int k = ((ixx >> 10) << 2) + (ixy >> 10) * sStride;
				ixx += isdHx;
				ixy += isdHy;
				DWORD *dptr = (DWORD *) dptr1;
				DWORD *sptr = (DWORD *) (sptr1+k);
				*dptr = *sptr;
				dptr1 += 4;
			}

			dptr += dStride;
			iyx += isdVx;
			iyy += isdVy;
		}
	}

	else ShiAssert(false);		// unsupported

	destbuf -> Unlock();
	
	ShiAssert(srcbuf != NULL);
	if (srcbuf) // JB 010318 CTD
		srcbuf -> Unlock();
}

void RotateBitmap(ImageBuffer *srcbuf, ImageBuffer *destbuf, int angle, RECT *srect, RECT *drect)
{
	ShiAssert( srect );
	ShiAssert( drect );

	char *source = (char *) srcbuf ->Lock();
	ShiAssert( source );
	char *dest = (char *) destbuf ->Lock();
	ShiAssert( dest );

	int dStride = destbuf -> targetStride();
	int sStride = srcbuf -> targetStride();
	int destx = drect -> left;
	int desty = drect -> top;
	int width = drect -> right - destx;
	int height = drect -> bottom - desty;
	int sx = (srect -> right + srect -> left) >> 1;
	int sy = (srect -> bottom + srect -> top) >> 1;

	float	sine, cosine;

	glGetSinCos (&sine, &cosine, angle);
	float x = (float) (width >> 1);
	float y = (float) (height >> 1);
	float xc =  x * cosine; 
	float xs = -x * sine;
	float ys =  y * sine;   
	float yc =  y * cosine;
	x = 1.0f / x; y = 1.0f / y;

	sx += (int) (-xc - ys);
	sy += (int) (-xs - yc);

	char *dptr = dest + (destx << 1) + desty * dStride;
	char *sptr = source + (sx << 1) + sy * sStride;

	float x1 = x * 1024.0f; 
	float y1 = y * 1024.0f; 
	int isdHx = (int) (xc * x1);
	int isdHy = (int) (xs * x1);
	int isdVx = (int) (ys * y1);
	int isdVy = (int) (yc * y1);
	int iyx = 0;
	int iyy = 0;

	int	i;
	for(i=0; i < height; i++)
	{
		int k = ((iyx >> 10) << 1) + (iyy >> 10) * sStride;
		char *sptr1 = sptr + k;
		char *dptr1 = dptr;
		int ixx = 0;
		int ixy = 0;
		int	j;

		for (j=0; j < width; j++)
		{
			int k = ((ixx >> 10) << 1) + (ixy >> 10) * sStride;
			ixx += isdHx;
			ixy += isdHy;
			short *dptr = (short *) dptr1;
			short *sptr = (short *) (sptr1+k);
			*dptr = *sptr;
			dptr1 += 2;
		}

		dptr += dStride;
		iyx += isdVx;
		iyy += isdVy;
	}

	destbuf -> Unlock();
	srcbuf -> Unlock();
}


void RotateBitmapMaskDouble(ImageBuffer *srcbuf, ImageBuffer *destbuf, int angle, RECT *srect, RECT *drect, int *startstop)
{
	ShiAssert( srect );
	ShiAssert( drect );

	if (!startstop)
	{
		RotateBitmapDouble (srcbuf, destbuf, angle, srect, drect);
		return;
	}

	char *source = (char *) srcbuf ->Lock();
	ShiAssert( source );
	char *dest = (char *) destbuf ->Lock();
	ShiAssert( dest );

	if(destbuf->PixelSize() == 2)
	{
		int dStride	= destbuf->targetStride();
		int sStride	= srcbuf->targetStride();
		int destx	= drect->left;
		int desty	= drect->top;
		int width	= srect->right - srect->left;
		int height	= srect->bottom - srect->top;
		int sx		= (srect->right + srect->left) >> 1;
		int sy		= (srect->bottom + srect->top) >> 1;

		float sine, cosine;
		glGetSinCos (&sine, &cosine, angle);
		float x = (float) (width >> 1);
		float y = (float) (height >> 1);
		float xc =  x * cosine; 
		float xs = -x * sine;
		float ys =  y * sine;   
		float yc =  y * cosine;
		x = 1.0f / x; y = 1.0f / y;

		sx += (int) (-xc - ys);
		sy += (int) (-xs - yc);

		char *dptr = dest + (destx << 1) + desty * dStride;
		char *sptr = source + (sx << 1) + sy * sStride;

		float x1 = x * 1024.0f; 
		float y1 = y * 1024.0f; 
		int isdHx = (int) (xc * x1);
		int isdHy = (int) (xs * x1);
		int isdVx = (int) (ys * y1);
		int isdVy = (int) (yc * y1);
		int iyx = 0;
		int iyy = 0;

		int	i;
		for(i=0; i < height; i++)
		{
			int start = *startstop++;
			int stop = *startstop++;
			int k = ((iyx >> 10) << 1) + (iyy >> 10) * sStride;
			char *sptr1 = sptr + k;
			char *dptr1 = dptr + (start << 2);

			int ixx = (int) (start * isdHx);
			int ixy = (int) (start * isdHy);
			int	j;

			for(j=start; j < stop; j++)
			{
				int k = ((ixx >> 10) << 1) + (ixy >> 10) * sStride;
				ixx += isdHx;
				ixy += isdHy;
				short *dptr = (short *) dptr1;
				short *sptr = (short *) (sptr1+k);

				// Write the pixels (TODO:  If we got it aligned first, we could write DWORDs at a time)
				*(dptr)				= *sptr;
				*(dptr+1)			= *sptr;
				*(dptr+dStride/2)	= *sptr;
				*(dptr+dStride/2+1)	= *sptr;

				dptr1 += 4;		// Skip four byte (since we write two pixels each time)
			}

			dptr += dStride*2;	// Skip two lines (since we write two each time)
			iyx += isdVx;
			iyy += isdVy;
		}
	}

	else if(destbuf->PixelSize() == 4)
	{
		int dStride	= destbuf->targetStride();
		int sStride	= srcbuf->targetStride();
		int destx	= drect->left;
		int desty	= drect->top;
		int width	= srect->right - srect->left;
		int height	= srect->bottom - srect->top;
		int sx		= (srect->right + srect->left) >> 1;
		int sy		= (srect->bottom + srect->top) >> 1;

		float sine, cosine;
		glGetSinCos (&sine, &cosine, angle);
		float x = (float) (width >> 1);
		float y = (float) (height >> 1);
		float xc =  x * cosine; 
		float xs = -x * sine;
		float ys =  y * sine;   
		float yc =  y * cosine;
		x = 1.0f / x; y = 1.0f / y;

		sx += (int) (-xc - ys);
		sy += (int) (-xs - yc);

		char *dptr = dest + (destx << 2) + desty * dStride;
		char *sptr = source + (sx << 2) + sy * sStride;

		float x1 = x * 1024.0f; 
		float y1 = y * 1024.0f; 
		int isdHx = (int) (xc * x1);
		int isdHy = (int) (xs * x1);
		int isdVx = (int) (ys * y1);
		int isdVy = (int) (yc * y1);
		int iyx = 0;
		int iyy = 0;

		int	i;
		for(i=0; i < height; i++)
		{
			int start = *startstop++;
			int stop = *startstop++;
			int k = ((iyx >> 10) << 2) + (iyy >> 10) * sStride;
			char *sptr1 = sptr + k;
			char *dptr1 = dptr + (start << 3);

			int ixx = (int) (start * isdHx);
			int ixy = (int) (start * isdHy);
			int	j;

			for (j=start; j < stop; j++)
			{
				int k = ((ixx >> 10) << 2) + (ixy >> 10) * sStride;
				ixx += isdHx;
				ixy += isdHy;
				DWORD *dptr = (DWORD *) dptr1;
				DWORD *sptr = (DWORD *) (sptr1+k);

				// Write the pixels (TODO:  If we got it aligned first, we could write DWORDs at a time)
				*(dptr)				= *sptr;
				*(dptr+1)			= *sptr;
				*(dptr+(dStride/4))	= *sptr;
				*(dptr+(dStride/4)+1)	= *sptr;

				dptr1 += 8;		// Skip four byte (since we write two pixels each time)
			}

			dptr += dStride*2;	// Skip two lines (since we write two each time)
			iyx += isdVx;
			iyy += isdVy;
		}
	}

	else ShiAssert(false);		// unsupported

	destbuf -> Unlock();
	srcbuf -> Unlock();
}

void RotateBitmapDouble(ImageBuffer *srcbuf, ImageBuffer *destbuf, int angle, RECT *srect, RECT *drect)
{
	ShiAssert( srect );
	ShiAssert( drect );

	char *source = (char *) srcbuf->Lock();
	ShiAssert( source );
	char *dest = (char *) destbuf->Lock();
	ShiAssert( dest );

	int dStride	= destbuf->targetStride();
	int sStride	= srcbuf->targetStride();
	int destx	= drect->left;
	int desty	= drect->top;
	int width	= srect->right - srect->left;
	int height	= srect->bottom - srect->top;
	int sx		= (srect->right + srect->left) >> 1;
	int sy		= (srect->bottom + srect->top) >> 1;

	float	sine, cosine;

	glGetSinCos (&sine, &cosine, angle);
	float x = (float) (width >> 1);
	float y = (float) (height >> 1);
	float xc =  x * cosine; 
	float xs = -x * sine;
	float ys =  y * sine;   
	float yc =  y * cosine;
	x = 1.0f / x; y = 1.0f / y;

	sx += (int) (-xc - ys);
	sy += (int) (-xs - yc);

	char *dptr = dest + (destx << 1) + desty * dStride;
	char *sptr = source + (sx << 1) + sy * sStride;

	float x1 = x * 1024.0f; 
	float y1 = y * 1024.0f; 
	int isdHx = (int) (xc * x1);
	int isdHy = (int) (xs * x1);
	int isdVx = (int) (ys * y1);
	int isdVy = (int) (yc * y1);
	int iyx = 0;
	int iyy = 0;

	int	i;
	for (i=0; i < height; i++)
	{
		int k = ((iyx >> 10) << 1) + (iyy >> 10) * sStride;
		char *sptr1 = sptr + k;
		char *dptr1 = dptr;
		int ixx = 0;
		int ixy = 0;
		int	j;

		for (j=0; j < width; j++)
		{
			int k = ((ixx >> 10) << 1) + (ixy >> 10) * sStride;
			ixx += isdHx;
			ixy += isdHy;
			short *dptr = (short *) dptr1;
			short *sptr = (short *) (sptr1+k);

			// Write the pixels (TODO:  If we got it aligned first, we could write DWORDs at a time)
			*(dptr)				= *sptr;
			*(dptr+1)			= *sptr;
			*(dptr+dStride/2)	= *sptr;
			*(dptr+dStride/2+1)	= *sptr;

			dptr1 += 4;		// Skip four byte (since we write two pixels each time)
		}

		dptr += dStride*2;	// Skip two lines (since we write two each time)
		iyx += isdVx;
		iyy += isdVy;
	}

	destbuf -> Unlock();
	srcbuf -> Unlock();
}

