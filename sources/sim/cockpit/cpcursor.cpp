#include "stdafx.h"
#include "cpmanager.h"
#include "cpcursor.h"


//====================================================//
// CPCursor::CPCursor
//====================================================//

CPCursor::CPCursor(CursorInitStr* pcursorInitStr) {

	mIdNum		= pcursorInitStr->idNum;
	mSrcRect		= pcursorInitStr->srcRect;
	
	mxHotspot	= pcursorInitStr->xhotSpot;
	myHotspot	= pcursorInitStr->yhotSpot;

	mpOTWImage	= pcursorInitStr->pOtwImage;
	mpTemplate	= pcursorInitStr->pTemplate;	
}

void CPCursor::Display(void) {

}
