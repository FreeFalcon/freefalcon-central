#include "stdafx.h"
#include "falclib.h"
#include "dispcfg.h"
#include "cpadi.h"
#include "simbase.h"
#include "cphsi.h"
#include "dispcfg.h"
#include "Graphics/Include/grinline.h"
#include "dispopts.h"
#include "flightData.h"
#include "Graphics/Include/renderow.h"
#include "otwdrive.h"
#include "FalcLib/include/playerop.h"
#include "FalcLib/include/dispopts.h"
#include "aircrft.h" //MI
#include "simdrive.h" //MI


extern bool g_bRealisticAvionics; //MI
extern bool g_bINS; //MI
extern bool g_bCockpitAutoScale; //Wombat778 10-06-2003
extern bool g_bFilter2DPit; //Wombat778 3-30-04

//====================================================//
// CPAdi::CPAdi
//====================================================//
CPAdi::CPAdi(ObjectInitStr *pobjectInitStr, ADIInitStr *padiInitStr) : CPObject(pobjectInitStr)
{

    int i;
    float x;
    float y;
    float radiusSquared;
    int arraySize;
    int halfArraySize;
    float halfCockpitWidth;
    float halfCockpitHeight;

    //MI inialize
    mPitch = 0.0F;
    mRoll = 0.0F;

    mColor[0][0] = padiInitStr->color0; //0xFF20A2C8;
    mColor[1][0] = CalculateNVGColor(mColor[0][0]);
    mColor[0][1] = padiInitStr->color1; //0xff808080;
    mColor[1][1] = CalculateNVGColor(mColor[0][1]);
    mColor[0][2] = padiInitStr->color2; //0xffffffff;
    mColor[1][2] = CalculateNVGColor(mColor[0][2]);
    mColor[0][3] = padiInitStr->color3; //0xFF6CF3F3; ILS Bars, light yellow
    mColor[1][3] = CalculateNVGColor(mColor[0][3]);
    mColor[0][4] = padiInitStr->color4; //0xFF6CF3F3; A/C reference symbol, light yellow
    mColor[1][4] = CalculateNVGColor(mColor[0][4]);


    mSrcRect = padiInitStr->srcRect;
    mILSLimits = padiInitStr->ilsLimits;

    mSrcHalfHeight = (mSrcRect.bottom - mSrcRect.top + 1) / 2;

    mMinPitch = -89.9F * DTR;
    mMaxPitch = 89.9F * DTR;
    mTanVisibleBallHalfAngle = (float)tan(25.0F * DTR);


    // Setup the circle limits
    mRadius = max(mWidth, mHeight);
    mRadius = (mRadius + 1) / 2;
    arraySize = (int)mRadius * 4;

#ifdef USE_SH_POOLS
    mpADICircle = (int *)MemAllocPtr(gCockMemPool, sizeof(int) * arraySize, FALSE);
#else
    mpADICircle = new int[arraySize];
#endif

    radiusSquared = (float) mRadius * mRadius;

    halfArraySize = arraySize / 2;

    for (i = 0; i < halfArraySize; i++)
    {

        y = (float) abs(i - mRadius);
        x = (float)sqrt(radiusSquared - y * y);
        mpADICircle[i * 2 + 1] = mRadius + (int)x; //right
        mpADICircle[i * 2 + 0] = mRadius - (int)x; //left
    }

    halfCockpitWidth = (float) DisplayOptions.DispWidth * 0.5F;
    halfCockpitHeight = (float) DisplayOptions.DispHeight * 0.5F;

    mLeft = (mDestRect.left - halfCockpitWidth) / halfCockpitWidth;
    mRight = (mDestRect.right - halfCockpitWidth) / halfCockpitWidth;
    mTop = -(mDestRect.top - halfCockpitHeight) / halfCockpitHeight;
    mBottom = -(mDestRect.bottom - halfCockpitHeight) / halfCockpitHeight;

    mpAircraftBarData[0][0] = 0.1F;
    mpAircraftBarData[0][1] = 0.5F;
    mpAircraftBarData[1][0] = 0.3F;
    mpAircraftBarData[1][1] = 0.5F;
    mpAircraftBarData[2][0] = 0.4F;
    mpAircraftBarData[2][1] = 0.6F;
    mpAircraftBarData[3][0] = 0.5F;
    mpAircraftBarData[3][1] = 0.5F;
    mpAircraftBarData[4][0] = 0.6F;
    mpAircraftBarData[4][1] = 0.6F;
    mpAircraftBarData[5][0] = 0.7F;
    mpAircraftBarData[5][1] = 0.5F;
    mpAircraftBarData[6][0] = 0.9F;
    mpAircraftBarData[6][1] = 0.5F;

    for (i = 0; i < NUM_AC_BAR_POINTS; i++)
    {
        mpAircraftBar[i][0] = (unsigned)(mpAircraftBarData[i][0] * (float) mWidth * mHScale) + mDestRect.left;
        mpAircraftBar[i][1] = (unsigned)(mpAircraftBarData[i][1] * (float) mHeight * mVScale) + mDestRect.top;
    }

    mLeftLimit = (int)(mHScale * mILSLimits.left);
    mRightLimit = (int)(mHScale * mILSLimits.right);
    mTopLimit = (int)(mVScale * mILSLimits.top);
    mBottomLimit = (int)(mVScale * mILSLimits.bottom);

    mHorizScale = (float)(mRightLimit - mLeftLimit) / HORIZONTAL_SCALE;
    mVertScale = (float)(mBottomLimit - mTopLimit) / VERTICAL_SCALE;

    mHorizCenter = (unsigned)(abs(int(mRightLimit - mLeftLimit)) * 0.5 + mLeftLimit);
    mVertCenter = (unsigned)(abs(int(mBottomLimit - mTopLimit)) * 0.5 + mTopLimit);

    mHorizBarPos = mTopLimit;
    mVertBarPos = mLeftLimit;

    mDoBackRect = padiInitStr->doBackRect;


    mBackSrc.top = padiInitStr->backSrc.top;
    mBackSrc.left = padiInitStr->backSrc.left;
    mBackSrc.bottom = padiInitStr->backSrc.bottom;
    mBackSrc.right = padiInitStr->backSrc.right;

    mBackDest.top = (long)(mVScale * padiInitStr->backDest.top);
    mBackDest.left = (long)(mHScale * padiInitStr->backDest.left);
    mBackDest.bottom = (long)(mVScale * (padiInitStr->backDest.bottom + 1));
    mBackDest.right = (long)(mHScale * (padiInitStr->backDest.right + 1));

    mpSourceBuffer = padiInitStr->pBackground;
    mpSurfaceBuffer = NULL;

    //MI
    Persistant = pobjectInitStr->persistant;
    LastMainADIPitch = 0.0F;
    LastMainADIRoll = 0.0F;
    LastBUPPitch = 0.0F;
    LastBUPRoll = 0.0F;


    if (DisplayOptions.bRender2DCockpit and not mDoBackRect)
    {
        mpSourceBuffer = padiInitStr->sourceadi;
    }
    //Wombat778 10-06-2003 Added following lines to set up a temporary buffer for the ADI
    //this is unnecessary in using rendered pit
    else if (g_bCockpitAutoScale and ((mHScale not_eq 1.0f) or (mVScale not_eq 1.0f)))
    {
        ADIBuffer = new ImageBuffer;
        ADIBuffer->Setup(&FalconDisplay.theDisplayDevice,
                         (int)((mDestRect.right - mDestRect.left) / mHScale),
                         (int)((mDestRect.bottom - mDestRect.top) / mVScale),
                         SystemMem, None, FALSE
                        );  //Wombat778 10-06 2003 Setup a new imagebuffer the size of the
        ADIBuffer->SetChromaKey(0xFFFF0000);
    }

    //Wombat778 10-06-2003 End of added code

}

//====================================================//
// CPAdi::~CPAdi
//====================================================//

CPAdi::~CPAdi(void)
{

    delete [] mpADICircle;

    if (mpSurfaceBuffer)
    {
        mpSurfaceBuffer->Cleanup();
        delete mpSurfaceBuffer;
    }

    if (DisplayOptions.bRender2DCockpit or mDoBackRect)   //Wombat778 3-24-04
    {
        glReleaseMemory((char*) mpSourceBuffer);
    }
    //Wombat778 10-06-2003 Added following lines to destroy the temporary imagebuffer;
    //this is unnecessary in using rendered pit
    else if (g_bCockpitAutoScale and ((mHScale not_eq 1.0f) or (mVScale not_eq 1.0f)))
    {
        if (ADIBuffer)
        {
            ADIBuffer->Cleanup();
            delete ADIBuffer;
        }
    }

    //Wombat778 10-06-2003 End of added code


}



//====================================================//
// CPAdi::CreateLit
//====================================================//

void CPAdi::CreateLit(void)
{
    const DWORD dwMaxTextureWidth = mpOTWImage->GetDisplayDevice()->GetDefaultRC()->m_pD3DHWDeviceDesc->dwMaxTextureWidth;
    const DWORD dwMaxTextureHeight = mpOTWImage->GetDisplayDevice()->GetDefaultRC()->m_pD3DHWDeviceDesc->dwMaxTextureHeight;


    if (DisplayOptions.bRender2DCockpit) //Wombat778 3-24-04 we are gonna do this my way...dont know about all that crap below
    {
        try
        {
            const DWORD dwMaxTextureWidth = mpOTWImage->GetDisplayDevice()->GetDefaultRC()->m_pD3DHWDeviceDesc->dwMaxTextureWidth;
            const DWORD dwMaxTextureHeight = mpOTWImage->GetDisplayDevice()->GetDefaultRC()->m_pD3DHWDeviceDesc->dwMaxTextureHeight;
            m_pPalette = new PaletteHandle(mpOTWImage->GetDisplayDevice()->GetDefaultRC()->m_pDD, 32, 256);

            if ( not m_pPalette)
                throw _com_error(E_OUTOFMEMORY);

            // Check if we can use a single texture
            if ((LONG)dwMaxTextureWidth >= (mSrcRect.right - mSrcRect.left) and (LONG)dwMaxTextureHeight >= (mSrcRect.bottom - mSrcRect.top))
            {
                TextureHandle *pTex = new TextureHandle;

                if ( not pTex)
                    throw _com_error(E_OUTOFMEMORY);

                m_pPalette->AttachToTexture(pTex);

                if ( not pTex->Create("CPAdi", MPR_TI_PALETTE bitor MPR_TI_CHROMAKEY, 8, (UInt16)(mSrcRect.right - mSrcRect.left), (UInt16)(mSrcRect.bottom - mSrcRect.top)))
                    throw _com_error(E_FAIL);

                if ( not pTex->Load(0, 0xFFFF0000, (BYTE*) mpSourceBuffer, true, true)) // soon to be re-loaded by CPSurface::Translate3D
                    throw _com_error(E_FAIL);

                m_arrTex.push_back(pTex);
            }

        }
        catch (_com_error e)
        {
            MonoPrint("CPAdi::CreateAdiView - Error 0x%X (%s)\n", e.Error(), e.ErrorMessage());
            DiscardLit();
        }
    }
    else


        // revert to legacy rendering if we can't use a single texture even if Fast 2D Cockpit is active
        if ( not DisplayOptions.bRender2DCockpit or
            (DisplayOptions.bRender2DCockpit and 
             (mDoBackRect and ((LONG)dwMaxTextureWidth < mBackSrc.right or (LONG)dwMaxTextureHeight < mBackSrc.bottom))))
        {
            if (mDoBackRect)
            {
                mpSurfaceBuffer = new ImageBuffer;

                MPRSurfaceType front = (FalconDisplay.theDisplayDevice.IsHardware() and DisplayOptions.bRender2DCockpit) ? LocalVideoMem : SystemMem;

                if ( not mpSurfaceBuffer->Setup(&FalconDisplay.theDisplayDevice, mBackSrc.right, mBackSrc.bottom, front, None) and front == LocalVideoMem)
                {
                    // Retry with system memory if ouf video memory
#ifdef _DEBUG
                    OutputDebugString("CPAdi::CreateLit - Probably out of video memory. Retrying with system memory)\n");
#endif

                    BOOL bResult = mpSurfaceBuffer->Setup(&FalconDisplay.theDisplayDevice, mBackSrc.right, mBackSrc.bottom, SystemMem, None);

                    if ( not bResult) return;
                }

                mpSurfaceBuffer->SetChromaKey(0xFFFF0000);
            }

            else
                mpSurfaceBuffer = NULL;
        }

        else
        {
            try
            {
                if (mDoBackRect)
                {
                    m_pPalette = new PaletteHandle(mpOTWImage->GetDisplayDevice()->GetDefaultRC()->m_pDD, 32, 256);

                    if ( not m_pPalette)
                        throw _com_error(E_OUTOFMEMORY);

                    TextureHandle *pTex = new TextureHandle;

                    if ( not pTex)
                        throw _com_error(E_OUTOFMEMORY);

                    m_pPalette->AttachToTexture(pTex);

                    if ( not pTex->Create("CPAdi", MPR_TI_PALETTE bitor MPR_TI_CHROMAKEY, 8, (UInt16)mBackSrc.right, (UInt16)mBackSrc.bottom))
                        throw _com_error(E_FAIL);

                    if ( not pTex->Load(0, 0xFFFF0000, (BYTE*) mpSourceBuffer, true, true)) // soon to be re-loaded by CPObject::Translate3D
                        throw _com_error(E_FAIL);

                    m_arrTex.push_back(pTex);
                }

                else
                    mpSurfaceBuffer = NULL;
            }

            catch (_com_error e)
            {
                MonoPrint("CPAdi::CreateLit - Error 0x%X (%s)\n", e.Error(), e.ErrorMessage());
                DiscardLit();
            }
        }
}

//====================================================//
// CPAdi::DiscardLit
//====================================================//

void CPAdi::DiscardLit(void)
{
    if (mDoBackRect)
    {
        if (mpSurfaceBuffer)
            mpSurfaceBuffer->Cleanup();

        delete mpSurfaceBuffer;
        mpSurfaceBuffer = NULL;
    }

    CPObject::DiscardLit();
}

//====================================================//
// CPAdi::Exec
//====================================================//

void CPAdi::Exec(SimBaseClass *pSimBaseClass)
{
    float PercentHalfXscale;

    mpOwnship = pSimBaseClass;

    //MI
    if (g_bRealisticAvionics and g_bINS)
    {
        AircraftClass *playerAC = SimDriver.GetPlayerAircraft();

        if (playerAC and Persistant == 1)  //backup ADI, continue to function until out of energy
        {
            if (playerAC->INSState(AircraftClass::BUP_ADI_OFF_IN))
            {
                //make a check for the BUP ADI energy here when ready
                mPitch = cockpitFlightData.pitch;
                mRoll = cockpitFlightData.roll;
                LastBUPPitch = mPitch;
                LastBUPRoll = mRoll;
            }
            else
            {
                mPitch = LastBUPPitch;
                mRoll = LastBUPRoll;
            }
        }
        else if (playerAC and not playerAC->INSState(AircraftClass::INS_ADI_OFF_IN))
        {
            //stay where you currently are
            mPitch = LastMainADIPitch;
            mRoll = LastMainADIRoll;
        }
        else
        {
            mPitch = cockpitFlightData.pitch;
            mRoll = cockpitFlightData.roll;
            LastMainADIPitch = mPitch;
            LastMainADIRoll = mRoll;
        }
    }
    else
    {
        mPitch = cockpitFlightData.pitch;
        mRoll = cockpitFlightData.roll;
    }

    // Bound the pitch angle so we don't go beyond +/- 30 deg for now
    mPitch = max(mPitch, mMinPitch);
    mPitch = min(mPitch, mMaxPitch);

    // Compute the slide distance based on the pitch angle
    PercentHalfXscale = mPitch / (25.0F * DTR); //tan(mPitch) / mTanVisibleBallHalfAngle;
    mSlide = mRadius * (float)PercentHalfXscale;

    if (gNavigationSys)
    {
        ExecILS();
    }

    SetDirtyFlag(); //VWF FOR NOW
}

//====================================================//
// CPAdi::ExecILS
//====================================================//

void CPAdi::ExecILS()
{
    float gpDeviation;
    float gsDeviation;

    if (mpCPManager->mHiddenFlag and (gNavigationSys->GetInstrumentMode() == NavigationSystem::TACAN or gNavigationSys->GetInstrumentMode() == NavigationSystem::NAV))
        return;

    if ((gNavigationSys->GetInstrumentMode() == NavigationSystem::ILS_TACAN or
         gNavigationSys->GetInstrumentMode() == NavigationSystem::ILS_NAV) and 
        gNavigationSys->GetILSAttribute(NavigationSystem::GP_DEV, &gpDeviation))
    {
        gNavigationSys->GetILSAttribute(NavigationSystem::GS_DEV, &gsDeviation);

        gpDeviation = min(max(gpDeviation, -HORIZONTAL_SCALE * 0.5F), HORIZONTAL_SCALE * 0.5F);
        gsDeviation = min(max(gsDeviation, -VERTICAL_SCALE * 0.5F), VERTICAL_SCALE * 0.5F);

        mpCPManager->ADIGpDevReading = mpCPManager->ADIGpDevReading + 0.1F * (gpDeviation - mpCPManager->ADIGpDevReading);
        mpCPManager->ADIGsDevReading = mpCPManager->ADIGsDevReading + 0.1F * (gsDeviation - mpCPManager->ADIGsDevReading);

        mpCPManager->ADIGpDevReading = min(max(mpCPManager->ADIGpDevReading, -HORIZONTAL_SCALE * 0.5F), HORIZONTAL_SCALE * 0.5F);
        mpCPManager->ADIGsDevReading = min(max(mpCPManager->ADIGsDevReading, -VERTICAL_SCALE * 0.5F), VERTICAL_SCALE * 0.5F);

        mVertBarPos = (unsigned) abs(FloatToInt32(mHorizScale * mpCPManager->ADIGpDevReading + mHorizCenter)); // Calc location of deviation bars
        mHorizBarPos = (unsigned) abs(FloatToInt32(mVertScale * -mpCPManager->ADIGsDevReading + mVertCenter));

        mVertBarPos = max(mVertBarPos, mLeftLimit); // Bound the positions
        mVertBarPos = min(mVertBarPos, mRightLimit);

        mHorizBarPos = min(mHorizBarPos, mBottomLimit);
        mHorizBarPos = max(mHorizBarPos, mTopLimit);

        mpCPManager->mHiddenFlag = FALSE;
    }

    else
    {
        if (mpCPManager->mHiddenFlag)
            return;

        else if (mVertBarPos <= mDestRect.left and mHorizBarPos >= mDestRect.bottom)
        {
            mpCPManager->mHiddenFlag = TRUE;
            mVertBarPos = mDestRect.left;
            mHorizBarPos = mDestRect.bottom;
            mpCPManager->ADIGpDevReading = -HORIZONTAL_SCALE;
            mpCPManager->ADIGsDevReading = -VERTICAL_SCALE;
        }

        else
        {
            mpCPManager->ADIGpDevReading = mpCPManager->ADIGpDevReading + 0.1F * (-HORIZONTAL_SCALE * 1.5F - mpCPManager->ADIGpDevReading);
            mpCPManager->ADIGsDevReading = mpCPManager->ADIGsDevReading + 0.1F * (-VERTICAL_SCALE * 1.5F - mpCPManager->ADIGsDevReading);

            mVertBarPos = (unsigned) abs(FloatToInt32(mHorizScale * mpCPManager->ADIGpDevReading + mHorizCenter)); // Calc location of deviation bars
            mHorizBarPos = (unsigned) abs(FloatToInt32(mVertScale * -mpCPManager->ADIGsDevReading + mVertCenter));

            mVertBarPos = max(mVertBarPos, mDestRect.left); // Bound the positions
            mVertBarPos = min(mVertBarPos, mDestRect.right);

            mHorizBarPos = min(mHorizBarPos, mDestRect.bottom);
            mHorizBarPos = max(mHorizBarPos, mDestRect.top);
        }
    }
}

//====================================================//
// CPAdi::ExecILSNone
//====================================================//

void CPAdi::ExecILSNone()
{
    mHorizBarPos = mTopLimit;
    mVertBarPos = mLeftLimit;
}


//Wombat778 3-23-04 Add support for rendered adi.  Much faster than blitting.

void RenderADIPoly(tagRECT *srcrect, tagRECT *srcloc, tagRECT *destrect, GLint alpha, TextureHandle *pTex, float angle) //Wombat778 3-22-04 helper function to keep the displayblit3d tidy.
{
    int entry;

    float x1, y1;
    const float angsin = sin(angle);
    const float angcos = cos(angle);

    float x = destrect->left + ((float)(destrect->right - destrect->left) / 2.0f);
    float y = destrect->top + ((float)(destrect->bottom - destrect->top) / 2.0f);
    float Radius = (float)(destrect->bottom - destrect->top) / 2.0f;
    float hw_rate = OTWDriver.pCockpitManager->mHScale / OTWDriver.pCockpitManager->mVScale;



    OTWDriver.renderer->CenterOriginInViewport();
    OTWDriver.renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);

    // Setup vertices
    float fStartU = 0;
    float fMaxU = (float) pTex->m_nWidth / (float) pTex->m_nActualWidth;
    fMaxU -= fStartU;
    float fStartV = 0;
    float fMaxV = (float) pTex->m_nHeight / (float) pTex->m_nActualHeight;
    fMaxV -= fStartV;


    fStartU = ((float)(srcloc->left - srcrect->left) / (float)(srcrect->right - srcrect->left)) * fMaxU;
    fMaxU = ((float)(srcloc->right - srcrect->left) / (float)(srcrect->right - srcrect->left)) * fMaxU;

    fStartV = ((float)(srcloc->top - srcrect->top) / (float)(srcrect->bottom - srcrect->top)) * fMaxV;
    fMaxV = ((float)(srcloc->bottom - srcrect->top) / (float)(srcrect->bottom - srcrect->top)) * fMaxV;

    TwoDVertex pVtx[90];
    ZeroMemory(pVtx, sizeof(pVtx));


    for (entry = 0; entry <= CircleSegments - 2; entry++)
    {

        // Compute the end point of this next segment
        x1 = (x + Radius * CircleX[entry]);
        y1 = (y + Radius * CircleY[entry]);

        // Draw the segment

        pVtx[entry].x = x1;
        pVtx[entry].y = y1;

        float tempx = pVtx[entry].x;
        pVtx[entry].x = x + ((angcos * (pVtx[entry].x - x)) + (-angsin * (pVtx[entry].y - y))) * hw_rate;
        pVtx[entry].y = y + (angsin * (tempx - x)) + (angcos * (pVtx[entry].y - y));

        x1 = (x + Radius * CircleX[entry] * hw_rate);
        pVtx[entry].u = fStartU + (((float)(x1 - destrect->left) / (float)(destrect->right - destrect->left)) * (fMaxU - fStartU));
        pVtx[entry].v = fStartV + (((float)(y1 - destrect->top) / (float)(destrect->bottom - destrect->top)) * (fMaxV - fStartV));

        pVtx[entry].r = pVtx[entry].g = pVtx[entry].b = pVtx[entry].a = 1.0f;
    }

    OTWDriver.renderer->context.RestoreState(alpha);
    OTWDriver.renderer->context.SelectTexture1((GLint) pTex);
    OTWDriver.renderer->context.DrawPrimitive(MPR_PRM_TRIFAN, MPR_VI_COLOR bitor MPR_VI_TEXTURE, 90, pVtx, sizeof(pVtx[0]));
}

//====================================================//
// CPAdi::DisplayBlit
//====================================================//



void CPAdi::DisplayBlit3D()
{
    if ( not mDirtyFlag)
        return;

    if ( not DisplayOptions.bRender2DCockpit) //handle in displayblit
        return;

    if ( not m_arrTex.size())
        return; // handled in DisplayBlit

    if (mDoBackRect)
    {
        TextureHandle *pTex = m_arrTex[0];

        // Setup vertices
        float fStartU = 0;
        float fMaxU = (float) pTex->m_nWidth / (float) pTex->m_nActualWidth;
        fMaxU -= fStartU;

        float fStartV = 0;
        float fMaxV = (float) pTex->m_nHeight / (float) pTex->m_nActualHeight;
        fMaxV -= fStartV;

        TwoDVertex pVtx[4];
        ZeroMemory(pVtx, sizeof(pVtx));
        pVtx[0].x = (float)mBackDest.left;
        pVtx[0].y = (float)mBackDest.top;
        pVtx[0].u = fStartU;
        pVtx[0].v = fStartV;
        pVtx[0].r = pVtx[0].g = pVtx[0].b = pVtx[0].a = 1.0f;
        pVtx[1].x = (float)mBackDest.right;
        pVtx[1].y = (float)mBackDest.top;
        pVtx[1].u = fMaxU;
        pVtx[1].v = fStartV;
        pVtx[1].r = pVtx[1].g = pVtx[1].b = pVtx[1].a = 1.0f;
        pVtx[2].x = (float)mBackDest.right;
        pVtx[2].y = (float)mBackDest.bottom;
        pVtx[2].u = fMaxU;
        pVtx[2].v = fMaxV;
        pVtx[2].r = pVtx[2].g = pVtx[2].b = pVtx[2].a = 1.0f;
        pVtx[3].x = (float)mBackDest.left;
        pVtx[3].y = (float)mBackDest.bottom;
        pVtx[3].u = fStartU;
        pVtx[3].v = fMaxV;
        pVtx[3].r = pVtx[3].g = pVtx[3].b = pVtx[3].a = 1.0f;

        // COBRA - RED - Pit Vibrations
        OTWDriver.pCockpitManager->AddTurbulence(pVtx);

        // Setup state
        OTWDriver.renderer->context.RestoreState(STATE_ALPHA_TEXTURE_NOFILTER);
        OTWDriver.renderer->context.SelectTexture1((GLint) pTex);

        // Render it (finally)
        OTWDriver.renderer->context.DrawPrimitive(MPR_PRM_TRIFAN, MPR_VI_COLOR bitor MPR_VI_TEXTURE, 4, pVtx, sizeof(pVtx[0]));
    }

    //Wombat778 3-24-04
    else if (DisplayOptions.bRender2DCockpit)
    {
        RECT srcRect, DestRect = mDestRect;
        // COBRA - RED - Pit Vibrations
        int OffsetX = (int)OTWDriver.pCockpitManager->PitTurbulence.x;
        int OffsetY = (int)OTWDriver.pCockpitManager->PitTurbulence.y;
        DestRect.top += OffsetY;
        DestRect.bottom += OffsetY;
        DestRect.left += OffsetX;
        DestRect.right += OffsetX;

        // Build the source rectangle
        srcRect.top = (LONG)(mSrcRect.top + mSrcHalfHeight - mRadius - (int) mSlide);
        srcRect.left = (LONG)(mSrcRect.left);
        srcRect.bottom = (LONG)(srcRect.top + (mDestRect.bottom - mDestRect.top) / mVScale);
        srcRect.right = (LONG)(srcRect.left + (mDestRect.right - mDestRect.left) / mHScale);

        if (g_bFilter2DPit) //Wombat778 3-30-04 Add option to filter
            RenderADIPoly(&mSrcRect, &srcRect, &DestRect, STATE_CHROMA_TEXTURE, m_arrTex[0], -mRoll);
        else
            RenderADIPoly(&mSrcRect, &srcRect, &DestRect, STATE_ALPHA_TEXTURE_NOFILTER, m_arrTex[0], -mRoll);
    }

}

void CPAdi::DisplayBlit(void)
{
    RECT srcRect;
    mDirtyFlag = TRUE;

    if ( not mDirtyFlag)
        return;

    if (DisplayOptions.bRender2DCockpit) //Handle in Displayblit3d
        return;

    if (mDoBackRect and m_arrTex.size() == 0)
    {
        if (mpSurfaceBuffer)
            mpOTWImage->Compose(mpSurfaceBuffer, &mBackSrc, &mBackDest);
    }

    // Build the source rectangle
    srcRect.top = (LONG)(mSrcRect.top + mSrcHalfHeight - mRadius - (int) mSlide);
    srcRect.left = (LONG)(mSrcRect.left);
    srcRect.bottom = (LONG)(srcRect.top + (mDestRect.bottom - mDestRect.top) / mVScale);
    srcRect.right = (LONG)(srcRect.left + (mDestRect.right - mDestRect.left) / mHScale);





    //Wombat778 10-06-2003, modified following lines. allows ADI to scale properly when using cockpit auto scaling
    if (g_bCockpitAutoScale and ((mHScale not_eq 1.0f) or (mVScale not_eq 1.0f)))   //dont run this code if the var is set but no scaling is occuring
    {

        RECT temprect;

        temprect.top = 0;
        temprect.left = 0;
        temprect.right = (LONG)((mDestRect.right - mDestRect.left) / mHScale);
        temprect.bottom = (LONG)((mDestRect.bottom - mDestRect.top) / mVScale);

        ADIBuffer->Clear(0xFFFF0000); //clear the temp buffer with chromakey blue;
        ADIBuffer->ComposeRoundRot(mpTemplate, &srcRect, &temprect, -mRoll, mpADICircle); //Rotate the image from template to temp buffer
        mpOTWImage->ComposeTransparent(ADIBuffer, &temprect, &mDestRect);

    }

    else
    {
        mpOTWImage->ComposeRoundRot(mpTemplate, &srcRect, &mDestRect, -mRoll, mpADICircle);
    }

    //Wombat778 10-26-2003 End of modified lines

    mDirtyFlag = FALSE;
}

//====================================================//
// CPAdi::Display
//====================================================//

void CPAdi::DisplayDraw(void)
{
    int i;
    float width = 2;

    //MI make them a bit smaller
    if (g_bRealisticAvionics)
    {
        width = 1.0F;
    }

    // sfr: looks nonsense...
    mDirtyFlag = TRUE;

    if ( not mDirtyFlag)
    {
        return;
    }

    OTWDriver.renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);
    DWORD color = OTWDriver.pCockpitManager->ApplyLighting(mColor[0][4], true);
    OTWDriver.renderer->SetColor(color);

    for (i = 0; i < NUM_AC_BAR_POINTS - 1; i++)
    {
        OTWDriver.renderer->Render2DLine(
            (float)mpAircraftBar[i][0],
            (float)mpAircraftBar[i][1],
            (float)mpAircraftBar[i + 1][0],
            (float)mpAircraftBar[i + 1][1]
        );
    }

    if ( not mpCPManager->mHiddenFlag)
    {
        //OTWDriver.renderer->SetColor(mColor[OTWDriver.renderer->GetGreenMode() not_eq 0][1]);
        OTWDriver.renderer->Render2DTri((float)mVertBarPos, (float)mTopLimit - 3.0F * width, (float)mVertBarPos + width,
                                        (float)mTopLimit - 3.0F * width, (float)mVertBarPos + width, (float)mBottomLimit + 3.0F * width);
        OTWDriver.renderer->Render2DTri((float)mVertBarPos, (float)mTopLimit - 3.0F * width, (float)mVertBarPos,
                                        (float)mBottomLimit + 3.0F * width, (float)mVertBarPos + width, (float)mBottomLimit + 3.0F * width);
        //OTWDriver.renderer->SetColor(mColor[OTWDriver.renderer->GetGreenMode() not_eq 0][2]);
        OTWDriver.renderer->Render2DTri((float)mVertBarPos, (float)mTopLimit - 3.0F * width, (float)mVertBarPos - width,
                                        (float)mTopLimit - 3.0F * width, (float)mVertBarPos - width, (float)mBottomLimit + 3.0F * width);
        OTWDriver.renderer->Render2DTri((float)mVertBarPos, (float)mTopLimit - 3.0F * width, (float)mVertBarPos,
                                        (float)mBottomLimit + 3.0F * width, (float)mVertBarPos - width, (float)mBottomLimit + 3.0F * width);
        //OTWDriver.renderer->SetColor(mColor[OTWDriver.renderer->GetGreenMode() not_eq 0][1]);
        OTWDriver.renderer->Render2DTri((float)mLeftLimit - 3.0F * width, (float)mHorizBarPos, (float)mLeftLimit - 3.0F * width,
                                        (float)mHorizBarPos + width, (float)mRightLimit + 3.0F * width, (float)mHorizBarPos + width);
        OTWDriver.renderer->Render2DTri((float)mLeftLimit - 3.0F * width, (float)mHorizBarPos, (float)mRightLimit + 3.0F * width,
                                        (float)mHorizBarPos, (float)mRightLimit + 3.0F * width, (float)mHorizBarPos + width);
        //OTWDriver.renderer->SetColor(mColor[OTWDriver.renderer->GetGreenMode() not_eq 0][2]);
        OTWDriver.renderer->Render2DTri((float)mLeftLimit - 3.0F * width, (float)mHorizBarPos, (float)mLeftLimit - 3.0F * width,
                                        (float)mHorizBarPos - 2.0F, (float)mRightLimit + 3.0F * width, (float)mHorizBarPos - width);  //MI (float)mHorizBarPos - 2.0F);
        OTWDriver.renderer->Render2DTri((float)mLeftLimit - 3.0F * width, (float)mHorizBarPos, (float)mRightLimit + 3.0F * width,
                                        (float)mHorizBarPos, (float)mRightLimit + 3.0F * width, (float)mHorizBarPos - width);  //MI (float)mHorizBarPos - 2.0F);

    }

    mDirtyFlag = FALSE;
}


void CPAdi::Translate(WORD* palette16)
{
    if (mDoBackRect)
    {
        if (mpSurfaceBuffer)
            Translate8to16(palette16, mpSourceBuffer, mpSurfaceBuffer); // 8 bit color indexes of individual surfaces
    } // 16 bit ImageBuffers
}

void CPAdi::Translate(DWORD* palette32)
{
    if (mDoBackRect)
    {
        if (mpSurfaceBuffer)
            Translate8to32(palette32, mpSourceBuffer, mpSurfaceBuffer); // 8 bit color indexes of individual surfaces
    } // 32 bit ImageBuffers
}
