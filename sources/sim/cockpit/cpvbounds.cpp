#include "stdafx.h"

#include "cpvbounds.h"

// sfr: added 2 scale factors
void ConvertRecttoVBounds(RECT *rect, ViewportBounds *vbounds, int width, int height, float hScale, float vScale) {		//Wombat778 10-06-2003 Changes scale from int to float

		float		halfWidth;
		float		halfHeight;

		halfWidth			= width * 0.5F;
		halfHeight			= height * 0.5F;

		vbounds->top		= -(rect->top * vScale - halfHeight) / halfHeight;
		vbounds->left		= (rect->left * hScale - halfWidth) / halfWidth;
		vbounds->bottom	    = -(rect->bottom * vScale - halfHeight) / halfHeight;
		vbounds->right		= (rect->right * hScale - halfWidth) / halfWidth;
}