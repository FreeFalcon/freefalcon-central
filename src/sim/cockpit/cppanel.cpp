#include "stdafx.h"
#include "cpmanager.h"
#include "button.h"
#include "cppanel.h"
#include "cplight.h"
#include "cpadi.h"
#include "cphsi.h"
#include "cpindicator.h"
#include "sinput.h"
#include "FalcLib/include/playerop.h"
#include "FalcLib/include/dispopts.h"
//sfr: for cockpit light
#include "aircrft.h"
#include "simdrive.h" 

#if CPPANEL_STL
using namespace std;
#endif

void TestBounds(int, int, int*, int, int, int);

// Cursor Bounds not viewport bounds

void TestBounds(int position, int dimension, int* pcursorIndex, int cursor0, int cursor1, int cursor2) {

	if(position <= dimension / 4) {
		if(cursor0 != -1) {
			*pcursorIndex = cursor0;
		}
		else {
			*pcursorIndex = cursor1;
		}
	}
	else if(position > dimension / 4 && position < 3 * dimension / 4) {
		*pcursorIndex = cursor1;
	}
	else {
		if(cursor2 != -1) {
			*pcursorIndex = cursor2;
		}
		else {
			*pcursorIndex = cursor1;
		}
	}
}



//====================================================//
// CPPanel::CPPanel
//====================================================//
CPPanel::CPPanel(PanelInitStr* ppanelInitStr) {
	int		i;

	mIdNum						= ppanelInitStr->idNum;
									
	mxPanelOffset				= ppanelInitStr->xOffset;
	myPanelOffset				= ppanelInitStr->yOffset;	
									
	mNumSurfaces				= ppanelInitStr->numSurfaces;
	mpSurfaceData				= ppanelInitStr->psurfaceData;
									
	mNumObjects					= ppanelInitStr->numObjects;
#if CPPANEL_STL

#else
	mpObjectIDs					= ppanelInitStr->pobjectIDs;
#endif
	mHudFont                = ppanelInitStr->hudFont;
	mMFDFont                = ppanelInitStr->mfdFont;
	mDEDFont                = ppanelInitStr->dedFont;
							
	#ifdef USE_SH_POOLS
	if(mNumObjects > 0) {
		mpObjects = (CPObject **)MemAllocPtr(gCockMemPool,sizeof(CPObject *)*mNumObjects,FALSE);
	}
	#else
	mpObjects					= new CPObject*[mNumObjects];
	#endif
									
									
	mNumButtonViews			= ppanelInitStr->numButtonViews;
	mpButtonViewIDs			= ppanelInitStr->pbuttonViewIDs;

	mpButtonViews				= NULL;
#ifdef USE_SH_POOLS
	if(mNumButtonViews > 0) {
		mpButtonViews = (CPButtonView **)MemAllocPtr(gCockMemPool,sizeof(CPButtonView *)*mNumButtonViews,FALSE);
	}
#else
	mpButtonViews				= new CPButtonView*[mNumButtonViews];
#endif
																
	mpOTWImage					= ppanelInitStr->pOtwImage;
									
	mPan							= ppanelInitStr->pan * DTR;
	mTilt							= ppanelInitStr->tilt * DTR;

	mHScale						= ppanelInitStr->hScale;
	mVScale						= ppanelInitStr->vScale;
	ppanelInitStr->maskTop	= (int)(ppanelInitStr->maskTop * mVScale);
	mMaskTop						= (ppanelInitStr->cockpitHeight * 0.5F - ppanelInitStr->maskTop) / (ppanelInitStr->cockpitHeight * 0.5F);

	mDoGeometry					= ppanelInitStr->doGeometry;
	
	for(i = 0; i < BOUNDS_TOTAL; i++) {
		if(ppanelInitStr->pviewRects[i]) {

			#ifdef USE_SH_POOLS
			mpViewBounds[i] = (ViewportBounds *)MemAllocPtr(gCockMemPool,sizeof(ViewportBounds),FALSE);
			#else
			mpViewBounds[i]	= new ViewportBounds;
			#endif

			ConvertRecttoVBounds(ppanelInitStr->pviewRects[i], 
										mpViewBounds[i], 
										ppanelInitStr->cockpitWidth, 
										ppanelInitStr->cockpitHeight, 
										mHScale, mVScale);

			delete ppanelInitStr->pviewRects[i];
		}
		else {

			mpViewBounds[i]	= NULL;
		}
	}

	mDefaultCursor				= ppanelInitStr->defaultCursor;
	mMouseBounds				= ppanelInitStr->mouseBounds;
	mAdjacentPanels			= ppanelInitStr->adjacentPanels;

   memcpy (osbLocation, ppanelInitStr->osbLocation, sizeof (osbLocation));

	delete ppanelInitStr;
}


//====================================================//
// CPPanel::~CPPanel
//====================================================//

CPPanel::~CPPanel() {
	int	i;

	if (mpSurfaceData) {
		delete [] mpSurfaceData;
	}

#if CPPANEL_STL
	mpObjectIDs.clear();
	mpButtonViewsIDs.clear();

	for (
		vector<CPObject*>::iterator it=mpObjects.begin();
		it != mpObjects.end();
		++it
	){
		delete *it;
	}
	mpObjects.clear();
	for (vector<CPButtonView*>::iterator it=mpButtonViews.begin(); it != mpButtonViews.end(); ++it){
		delete *it;
	}
	mpButtonViews.clear();

#else
	if (mpObjectIDs)
	{
		delete [] mpObjectIDs;
		delete [] mpObjects;
	}
	if(mpButtonViewIDs) {
		delete [] mpButtonViewIDs;
	}
	if(mpButtonViews) {
		delete [] mpButtonViews;
	}
#endif

	for(i = 0; i < BOUNDS_TOTAL; i++) {
		if(mpViewBounds[i]) {
			delete mpViewBounds[i];
		}
	}

}

//====================================================//
// CPPanel::Exec
//====================================================//

void CPPanel::Exec(SimBaseClass* pOwnship, int CycleBit) {

	int i;

	for(i = 0; i < mNumObjects; i++) {
		if(mpObjects[i]->mCycleBits & CycleBit) {
			mpObjects[i]->Exec(pOwnship);
		}
	}
	if (CycleBit & END_CYCLE) {
	    for(i = 0; i < mNumButtonViews; i++) {
			mpButtonViews[i]->UpdateView();
	    }
	}
}

//====================================================//
// CPPanel::SetDirtyFlags
//====================================================//

void CPPanel::SetDirtyFlags() {
	int		i;

	for(i = 0; i < mNumObjects; i++) {
		mpObjects[i]->SetDirtyFlag();
	}

	for(i = 0; i < mNumButtonViews; i++) {
		mpButtonViews[i]->SetDirtyFlag();
	}
}

//====================================================//
// CPPanel::Display
//====================================================//

void CPPanel::DisplayBlit() {
	int		i;

	if(!DisplayOptions.bRender2DCockpit)	// OW: dont loop in fast 2d mode ("blitting" handled by DisplayBlit3D)
	{
		// loop thru and display all surfaces for this panel
		F4EnterCriticalSection(OTWDriver.pCockpitManager->mpCockpitCritSec);

		for(i = 0; i < mNumSurfaces; i++)
		{

	#if 0

			// Added for TOD Effects 2/9/98 (changed mpSurfaceData to mpLitSurfData)
			mpLitSurfData[i].psurface->DisplayBlit(mpLitSurfData[i].transparencyType, mpLitSurfData[i].persistant,
														&mpLitSurfData[i].destRect, mxPanelOffset, myPanelOffset);
	#else
			mpSurfaceData[i].psurface->DisplayBlit(mpSurfaceData[i].transparencyType, mpSurfaceData[i].persistant,
														&mpSurfaceData[i].destRect, mxPanelOffset, myPanelOffset);
	#endif

	   }

		F4LeaveCriticalSection(OTWDriver.pCockpitManager->mpCockpitCritSec);
	}
 
	// loop thru and display all objects for this panel
	for(i = 0; i < mNumObjects; i++) {
		mpObjects[i]->DisplayBlit();
	}

	// loop thru and display all buttons for this panel
	for(i = 0; i < mNumButtonViews; i++) {
		mpButtonViews[i]->DisplayBlit();
	}
}

// OW
void CPPanel::DisplayBlit3D() {
	int		i;

	// loop thru and display all surfaces for this panel

	F4EnterCriticalSection(OTWDriver.pCockpitManager->mpCockpitCritSec);


	int	OffsetX=(int)OTWDriver.pCockpitManager->PitTurbulence.x / 2;
	int	OffsetY=(int)OTWDriver.pCockpitManager->PitTurbulence.y / 2;



	for(i = 0; i < mNumSurfaces; i++) {

#if 0

		// Added for TOD Effects 2/9/98 (changed mpSurfaceData to mpLitSurfData)
		mpLitSurfData[i].psurface->DisplayBlit3D(mpLitSurfData[i].transparencyType, mpLitSurfData[i].persistant,
													&mpLitSurfData[i].destRect, mxPanelOffset, myPanelOffset);
#else
		mpSurfaceData[i].psurface->DisplayBlit3D(mpSurfaceData[i].transparencyType, mpSurfaceData[i].persistant,
													&mpSurfaceData[i].destRect, mxPanelOffset + OffsetX , myPanelOffset + OffsetY);
#endif

   }
 
	F4LeaveCriticalSection(OTWDriver.pCockpitManager->mpCockpitCritSec);

	// loop thru and display all objects for this panel
	for(i = 0; i < mNumObjects; i++)
		mpObjects[i]->DisplayBlit3D();

//Wombat778 3-23-04  Enabled now that there is code to back this up
	// currently only supported for views			
	// loop thru and display all buttons for this panel
	for(i = 0; i < mNumButtonViews; i++) {
		mpButtonViews[i]->DisplayBlit3D();
	}
}

//====================================================//
// CPPanel::Display
//====================================================//

void CPPanel::DisplayDraw() {
	int		i;

	OTWDriver.renderer->SetOffset(OTWDriver.pCockpitManager->PitTurbulence.x, OTWDriver.pCockpitManager->PitTurbulence.y);
	// loop thru and display all objects for this panel
	for(i = 0; i < mNumObjects; i++) {
		mpObjects[i]->DisplayDraw();
	}
	OTWDriver.renderer->SetOffset(0.0f, 0.0f);
}

//====================================================//
// CPPanel::POVDispatch
//====================================================//

BOOL CPPanel::POVDispatch(int direction)
{
	int	newPanel = -1;
	BOOL	viewChanging = FALSE;

	switch (direction)
	{
		case POV_N:
			newPanel = mAdjacentPanels.N;
			break;

		case POV_NE:
			newPanel = mAdjacentPanels.NE;
			break;

		case POV_E:
			newPanel = mAdjacentPanels.E;
			break;

		case POV_SE:
			newPanel = mAdjacentPanels.SE;
			break;

		case POV_S:
			newPanel = mAdjacentPanels.S;
			break;

		case POV_SW:
			newPanel = mAdjacentPanels.SW;
			break;

		case POV_W:
			newPanel = mAdjacentPanels.W;
			break;

		case POV_NW:
			newPanel = mAdjacentPanels.NW;
			break;
	}

	if(newPanel >= 0) {
		OTWDriver.pCockpitManager->SetActivePanel(newPanel);
		viewChanging = TRUE;
	}

	return viewChanging;
}

//====================================================//
// CPPanel::Dispatch
//====================================================//

BOOL CPPanel::Dispatch(int* cursorIndex, int event, int xpos, int ypos) {

	BOOL	found = FALSE;
	BOOL	viewChanging = FALSE;
	int	i = 0;
	int	border;
	int	height;
	int	width;
	int	newPanel = -1;
	int	cursor0, cursor1, cursor2;

	if(mDefaultCursor >= 0) {
		while((!found) && (i < mNumButtonViews)) {
			found = mpButtonViews[i]->HandleEvent(cursorIndex, event, xpos, ypos);
			i++;
		}

		if(!found) { //check directional cursors
			border = OTWDriver.pCockpitManager->mMouseBorder;
			height = DisplayOptions.DispHeight;
			width	 = DisplayOptions.DispWidth;

			if(event == CP_MOUSE_BUTTON0 || event == CP_MOUSE_BUTTON1) {

				if(ypos < border) {
					TestBounds(xpos, width, &newPanel, mAdjacentPanels.NW, mAdjacentPanels.N, mAdjacentPanels.NE);
				}
				else if(ypos > height - border) {
					TestBounds(xpos, width, &newPanel, mAdjacentPanels.SW, mAdjacentPanels.S, mAdjacentPanels.SE);
				}
				else if(xpos < border) {
					TestBounds(ypos, height, &newPanel, mAdjacentPanels.NW, mAdjacentPanels.W, mAdjacentPanels.SW);
				}
				else if(xpos > width - border) {
					TestBounds(ypos, height, &newPanel, mAdjacentPanels.NE, mAdjacentPanels.E, mAdjacentPanels.SE);
				}

				if(newPanel >= 0) {
					OTWDriver.pCockpitManager->SetActivePanel(newPanel);
					viewChanging = TRUE;
				}

				*cursorIndex = mDefaultCursor;
			}

			else if(event == CP_MOUSE_MOVE) {

				cursor0 = -1;
				cursor1 = -1;
				cursor2 = -1;

				if(ypos < border) {

					if(mAdjacentPanels.NW > -1)
						cursor0 = NW_CURSOR;
					if(mAdjacentPanels.N > -1)
						cursor1 = N_CURSOR;
					if(mAdjacentPanels.NE > -1)
						cursor2 = NE_CURSOR;

					TestBounds(xpos, width, cursorIndex, cursor0, cursor1, cursor2);
				}
				else if(ypos > height - border) {

					if(mAdjacentPanels.SW > -1)
						cursor0 = SW_CURSOR;
					if(mAdjacentPanels.S > -1)
						cursor1 = S_CURSOR;
					if(mAdjacentPanels.SE > -1)
						cursor2 = SE_CURSOR;

					TestBounds(xpos, width, cursorIndex, cursor0, cursor1, cursor2);
				}
				else if(xpos < border) {

					if(mAdjacentPanels.NW > -1)
						cursor0 = NW_CURSOR;
					if(mAdjacentPanels.W > -1)
						cursor1 = W_CURSOR;
					if(mAdjacentPanels.SW > -1)
						cursor2 = SW_CURSOR;

					TestBounds(ypos, height, cursorIndex, cursor0, cursor1, cursor2);
				}
				else if(xpos > width - border) {

					if(mAdjacentPanels.NE > -1)
						cursor0 = NE_CURSOR;
					if(mAdjacentPanels.E > -1)
						cursor1 = E_CURSOR;
					if(mAdjacentPanels.SE > -1)
						cursor2 = SE_CURSOR;

					TestBounds(ypos, height, cursorIndex, cursor0, cursor1, cursor2);
				}
				else {
					*cursorIndex = mDefaultCursor;
				}

				if(*cursorIndex == -1) {
					*cursorIndex = mDefaultCursor;
				}
			}

			else {

				*cursorIndex = mDefaultCursor;
			}
		}
	}

	return viewChanging;
}


BOOL CPPanel::GetViewportBounds(ViewportBounds* bounds, int viewPort) {
	
	BOOL returnValue = FALSE;

	if(mpViewBounds[viewPort]) {

		*bounds = *mpViewBounds[viewPort];
		returnValue = TRUE;
	}

	return(returnValue);
}



void CPPanel::CreateLitSurfaces(float lightLevel)
{
	
	int	i;

	for(i = 0; i < mNumSurfaces; i++)
		mpSurfaceData[i].psurface->CreateLit();

	for(i = 0; i < mNumObjects; i++)
		mpObjects[i]->CreateLit();

	for(i = 0; i < mNumButtonViews; i++)		//Wombat778 3-23-04 Added for rendered buttonviews
		mpButtonViews[i]->CreateLit();

	SetTOD(lightLevel);
}


void CPPanel::DiscardLitSurfaces(void) {
	
	int	i;

	for(i = 0; i < mNumSurfaces; i++) {

		mpSurfaceData[i].psurface->DiscardLit();
	}

	for(i = 0; i < mNumObjects; i++) {

		mpObjects[i]->DiscardLit();
	}

	for(i = 0; i < mNumButtonViews; i++) {	//Wombat778 3-23-04 Added for rendered buttonviews

		mpButtonViews[i]->DiscardLit();
	}

}


/** sfr: this functions manipulates the cockpit palette. It takes into account if
 * flood light is on, instrument lights are on, and the enviroment lighting.
 * @param lightLevel the enviroment light level
*/
void CPPanel::SetPalette(){
	// flood and instrument lights
	float fLight[3], iLight[3];
	OTWDriver.pCockpitManager->ComputeLightFactors(fLight, iLight);

	// apply lighting to the palette, cockpit lights have their own palette, like HSI and ADI
	DWORD *panelPlt;
	DWORD outPlt[256], lightPlt[256], adiHsiPlt[256];
	panelPlt = gpTemplatePalette;

	// set the new palette, from 1, because 0 is transparency
	outPlt[0] = (OTWDriver.renderer->GetGreenMode()) ? 
		CalculateNVGColor(CalculateColor(panelPlt[0], fLight[0], fLight[1], fLight[2])):
		CalculateColor(panelPlt[0], fLight[0], fLight[1], fLight[2])
	;
	adiHsiPlt[0] = lightPlt[0] = outPlt[0];
	for (int i=1;i<(256-48);i++){
		// all except hsi and adi, which use instrument light in everything
		outPlt[i] = (OTWDriver.renderer->GetGreenMode()) ?
			CalculateNVGColor(CalculateColor(panelPlt[i], fLight[0], fLight[1], fLight[2])) :			
			CalculateColor(panelPlt[i], fLight[0], fLight[1], fLight[2])
		;
		lightPlt[i] = outPlt[i];
		adiHsiPlt[i] = (OTWDriver.renderer->GetGreenMode()) ?
			CalculateNVGColor(CalculateColor(panelPlt[i], iLight[0], iLight[1], iLight[2])) :			
			CalculateColor(panelPlt[i], iLight[0], iLight[1], iLight[2])
		;
	}
	for (int i=256-48;i<256;i++){
		// adi and hsi are like the rest here
		adiHsiPlt[i] = outPlt[i] = (OTWDriver.renderer->GetGreenMode()) ?
			CalculateNVGColor(CalculateColor(panelPlt[i], iLight[0], iLight[1], iLight[2])) :			
			CalculateColor(panelPlt[i], iLight[0], iLight[1], iLight[2])
		;
		// but for lights, use full brightness for last 48 colors always
		lightPlt[i] = (OTWDriver.renderer->GetGreenMode()) ?
			CalculateNVGColor(panelPlt[i]):
			panelPlt[i]
		;
	}

	//we have the new palette at outPlt, 
	//apply it to everything, except for lights, which use their own palette
	for (int i=mNumSurfaces-1; i>=0; i--){
		mpSurfaceData[i].psurface->Translate3D(outPlt);
	}
	for (int i = mNumObjects-1; i >= 0; i--){
		if (dynamic_cast<CPLight*>(mpObjects[i]) != NULL){
			mpObjects[i]->Translate3D(lightPlt);
		}
		else if (
			(dynamic_cast<CPHsi*>(mpObjects[i]) != NULL) || 
			(dynamic_cast<CPAdi*>(mpObjects[i]) != NULL) ||
			(dynamic_cast<CPIndicator*>(mpObjects[i]) != NULL)
		){
			mpObjects[i]->Translate3D(adiHsiPlt);
		}
		else {
			mpObjects[i]->Translate3D(outPlt);
		}
	}
	for (int i = mNumButtonViews-1; i>=0; i--){
		mpButtonViews[i]->Translate3D(outPlt);
	}
}

void CPPanel::SetTOD(float lightLevel)
{
	SetPalette();
	return;
	
	int pixelsize = (gpTemplateSurface)?gpTemplateSurface->PixelSize() : TemplateInfo->pixelsize;

		DWORD		palette16[256];
		DWORD		*palTgt		= palette16;
		GLulong		*palData	= gpTemplatePalette;
		GLulong		*palEnd		= palData + 208;	// Entries 208 through 255 are special...

		DWORD	lighting;
		DWORD	inColor;
		DWORD	outColor;
		int		i;
		DWORD	mask;
		BYTE red, green, blue;

		ShiAssert(FALSE == F4IsBadReadPtr(palData, 256 * sizeof (*palData)));
		ShiAssert(FALSE == F4IsBadWritePtr(palTgt, 256 * sizeof (*palTgt)));

		// Just copy the chromakey color without lighting it
		
		if (!gpTemplateSurface)
			*palTgt++ = TemplateInfo->Pixel32toPixel32( *palData++ );
		else								
			*palTgt++ = gpTemplateSurface->Pixel32toPixel32( *palData++ );

		// Now walk the palette and create a 32 bit lit version
		if(OTWDriver.renderer->GetGreenMode())
		{
			// Convert from floating point to a 16.16 fixed point representation
			lighting = FloatToInt32(lightLevel * 65536.0f);

			// Use only green
			mask = 0xFF00FF00;

			// Light the palette entries and convert to 16 bit colors
			do
			{
				inColor = *palData;

				outColor  = ((((inColor)     & 0xFF) * lighting) >> 16);
				outColor |= ((((inColor>>8)  & 0xFF) * lighting) >> 8) & 0x0000FF00;
				outColor |= ((((inColor>>16) & 0xFF) * lighting))      & 0x00FF0000;

				blue		= (BYTE)(((outColor) >> 16) & 0xFF);
				green		= (BYTE)(((outColor) >> 8)  & 0xFF);
				red		= (BYTE)((outColor)	  		 & 0xFF);
				outColor = ( ((BYTE) (0.299F * red + 0.587F * green + 0.114 * blue)) & 0xFF) << 8;

				if (!gpTemplateSurface)
					*palTgt = TemplateInfo->Pixel32toPixel32( outColor );
				else													
					*palTgt = gpTemplateSurface->Pixel32toPixel32( outColor );


				palData++;
				palTgt++;
			} while (palData < palEnd);

			// Entries 208 through 255 are special (unlit)
			palEnd += (256-208);

			do
			{
				blue		= (BYTE)(((*palData) >> 16) & 0xFF);
				green		= (BYTE)(((*palData) >> 8)  & 0xFF);
				red		= (BYTE)((*palData)	       & 0xFF);
				outColor = ( (int) (0.299F * red + 0.587F * green + 0.114 * blue) & 0xFF) << 8;

				if (!gpTemplateSurface)
					*palTgt = TemplateInfo->Pixel32toPixel32( outColor );
				else													
					*palTgt = gpTemplateSurface->Pixel32toPixel32( outColor );
				
				palData++;
				palTgt++;
			} while (palData < palEnd);
		}

		else
		{ 
			// Convert from floating point to a 16.16 fixed point representation
			lighting = FloatToInt32(lightLevel * 65536.0f);

			// Use full color
			mask = 0xFFFFFFFF;

			// Light the palette entries and convert to 16 bit colors
			do
			{
				inColor = *palData;

				outColor  = ((((inColor)     & 0xFF) * lighting) >> 16);
				outColor |= ((((inColor>>8)  & 0xFF) * lighting) >> 8) & 0x0000FF00;
				outColor |= ((((inColor>>16) & 0xFF) * lighting))      & 0x00FF0000;
							
				if (!gpTemplateSurface)
					*palTgt = TemplateInfo->Pixel32toPixel32( outColor & mask );
				else													
					*palTgt = gpTemplateSurface->Pixel32toPixel32( outColor & mask );

				palData++;
				palTgt++;
			} while (palData < palEnd);

			// Entries 208 through 255 are special (unlit)
			palEnd += (256-208);

			do
			{						
				if (!gpTemplateSurface)
					*palTgt = TemplateInfo->Pixel32toPixel32( *palData & mask );
				else													
					*palTgt = gpTemplateSurface->Pixel32toPixel32( *palData & mask );

				palData++;
				palTgt++;
			} while (palData < palEnd);
		}

		// Now convert all the 8 bit sources to 16 bit lit versions
		if (gpTemplateSurface&&gpTemplateImage)
			Translate8to32( palette16, 
						gpTemplateImage,			//	8 bit color indexes of template
						gpTemplateSurface );		//	16 bit ImageBuffer

		for (i=mNumSurfaces-1; i>=0; i--)
			mpSurfaceData[i].psurface->Translate(palette16);

		for (i = 0; i < mNumObjects; i++)
			mpObjects[i]->Translate((DWORD *)palette16);

		if(DisplayOptions.bRender2DCockpit)
		{
			DWORD		palette16[256];
			DWORD		*palTgt		= palette16;
			GLulong		*palData	= gpTemplatePalette;
			GLulong		*palEnd		= palData + 208;	// Entries 208 through 255 are special...

			DWORD	lighting;
			DWORD	inColor;
			DWORD	outColor;
			int		i;
			DWORD	mask;
			BYTE red, green, blue;

			// Just copy the chromakey color without lighting it
			*palTgt++ = *palData++;

			// Now walk the palette and create a 32 bit lit version
			if(OTWDriver.renderer->GetGreenMode())
			{
				// Convert from floating point to a 16.16 fixed point representation
				lighting = FloatToInt32(lightLevel * 65536.0f);

				// Use only green
				mask = 0xFF00FF00;

				// Light the palette entries and convert to 16 bit colors
				do
				{
					inColor = *palData;

					outColor  = ((((inColor)     & 0xFF) * lighting) >> 16);
					outColor |= ((((inColor>>8)  & 0xFF) * lighting) >> 8) & 0x0000FF00;
					outColor |= ((((inColor>>16) & 0xFF) * lighting))      & 0x00FF0000;

					blue		= (BYTE)(((outColor) >> 16) & 0xFF);
					green		= (BYTE)(((outColor) >> 8)  & 0xFF);
					red		= (BYTE)((outColor)	  		 & 0xFF);
					outColor = ( ((BYTE) (0.299F * red + 0.587F * green + 0.114 * blue)) & 0xFF) << 8;
					outColor |= 0xff000000;		// OW add alpha

					*palTgt = outColor;

					palData++;
					palTgt++;
				} while (palData < palEnd);

				// Entries 208 through 255 are special (unlit)
				palEnd += (256-208);

				do
				{
					blue		= (BYTE)(((*palData) >> 16) & 0xFF);
					green		= (BYTE)(((*palData) >> 8)  & 0xFF);
					red		= (BYTE)((*palData)	       & 0xFF);
					outColor = ( (int) (0.299F * red + 0.587F * green + 0.114 * blue) & 0xFF) << 8;
					outColor |= 0xff000000;		// OW add alpha

					if (!gpTemplateSurface)
						*palTgt = TemplateInfo->Pixel32toPixel32( outColor );
					else													
						*palTgt = gpTemplateSurface->Pixel32toPixel32( outColor );
				
					palData++;
					palTgt++;
				} while (palData < palEnd);
			}

			else
			{ 
				// Convert from floating point to a 16.16 fixed point representation
				lighting = FloatToInt32(lightLevel * 65536.0f);

				// Use full color
				mask = 0xFFFFFFFF;

				// Light the palette entries and convert to 16 bit colors
				do
				{
					inColor = *palData;

					outColor  = ((((inColor)     & 0xFF) * lighting) >> 16);
					outColor |= ((((inColor>>8)  & 0xFF) * lighting) >> 8) & 0x0000FF00;
					outColor |= ((((inColor>>16) & 0xFF) * lighting))      & 0x00FF0000;
					outColor |= 0xff000000;		// OW add alpha

					*palTgt = outColor & mask;

					palData++;
					palTgt++;
				} while (palData < palEnd);

				// Entries 208 through 255 are special (unlit)
				palEnd += (256-208);

				do
				{
					*palTgt = *palData & mask;
					palData++;
					palTgt++;
				} while (palData < palEnd);
			}

			for (i=mNumSurfaces-1; i>=0; i--)
				mpSurfaceData[i].psurface->Translate3D(palette16);

			for (i = 0; i < mNumObjects; i++)
				mpObjects[i]->Translate3D(palette16);

			for (i = 0; i < mNumButtonViews; i++)		//Wombat778 3-23-04  Added for rendered buttonviews (buttonviews arent objects)
				mpButtonViews[i]->Translate3D(palette16);

		}

}


//====================================================//
// CPPanel::DoGeometry
//====================================================//

BOOL CPPanel::DoGeometry(void)
{
	return mDoGeometry;
}
