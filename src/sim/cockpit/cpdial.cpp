#include "stdafx.h"
#include "cpdial.h"
#include "Graphics/Include/renderow.h"
#include "otwdrive.h"

#include "Graphics/Include/grinline.h" //Wombat778 3-26-04
extern bool g_bFilter2DPit; //Wombat778 3-30-04

CPDial::CPDial(ObjectInitStr *pobjectInitStr, DialInitStr* pdialInitStr) : CPObject(pobjectInitStr)
{
    int i;
    mlTrig trig;

    mSrcRect = pdialInitStr->srcRect;
    mRadius0 = pdialInitStr->radius0;
    mRadius1 = pdialInitStr->radius1;
    mRadius2 = pdialInitStr->radius2;

    mColor[0][0] = pdialInitStr->color0;
    mColor[0][1] = pdialInitStr->color1;
    mColor[0][2] = pdialInitStr->color2;

    mColor[1][0] = CalculateNVGColor(pdialInitStr->color0);
    mColor[1][1] = CalculateNVGColor(pdialInitStr->color1);
    mColor[1][2] = CalculateNVGColor(pdialInitStr->color2);

    mxCenter = mSrcRect.left + mWidth / 2;
    myCenter = mSrcRect.top + mHeight / 2;

    mEndPoints = pdialInitStr->endPoints;
    mpValues = pdialInitStr->pvalues;
    mpPoints = pdialInitStr->ppoints;

    mDialValue = 0.0F;

#ifdef USE_SH_POOLS
    mpCosPoints = (float *)MemAllocPtr(gCockMemPool, sizeof(float) * mEndPoints, FALSE);
    mpSinPoints = (float *)MemAllocPtr(gCockMemPool, sizeof(float) * mEndPoints, FALSE);
#else
    mpCosPoints = new float[mEndPoints];
    mpSinPoints = new float[mEndPoints];
#endif

    for (i = 0; i < mEndPoints; i++)
    {
        mlSinCos(&trig, mpPoints[i]);
        mpCosPoints[i] = trig.cos;
        mpSinPoints[i] = trig.sin;
    }

    //Wombat778 3-26-04  Added following for rendered, textured, needles
    IsRendered = pdialInitStr->IsRendered;

    if (IsRendered)
    {
        //if the needle has the rendering line, use destrect for the center
        // instead of the source(because now the src is actually used for something)
        //Wombat778 3-30-04 remove scaling that gets inserted by cpobject
        mxCenter = (int)((mDestRect.left / mHScale) + (mWidth / 2));
        //Wombat778 3-30-04 remove scaling that gets inserted by cpobject
        myCenter = (int)((mDestRect.top / mVScale) + (mHeight / 2));

        if (DisplayOptions.bRender2DCockpit)
        {
            mpSourceBuffer = pdialInitStr->sourcedial;
        }

        angle = 0.0f;
    }

    //Wombat778 end
}

CPDial::~CPDial()
{
    delete [] mpValues;
    delete [] mpPoints;
    delete [] mpSinPoints;
    delete [] mpCosPoints;

    if (IsRendered and DisplayOptions.bRender2DCockpit)
    {
        glReleaseMemory((char*) mpSourceBuffer);
    }
}

void CPDial::Exec(SimBaseClass* pOwnship)
{
    BOOL found = FALSE;
    int i = 0;
    int xOffset0 = 0;
    int yOffset0 = 0;
    int xOffset1 = 0;
    int yOffset1 = 0;
    int xOffset2 = 0;
    int yOffset2 = 0;
    int xOffset3 = 0;
    int yOffset3 = 0;
    float delta = 0.0F;
    float slope = 0.0F;
    float deflection = 0.0F;
    float cosDeflection = 0.0F;
    float sinDeflection = 0.0F;
    float cosSecondDeflection = 0.0F;
    float sinSecondDeflection = 0.0F;
    mlTrig  trig;

    mpOwnship = pOwnship;

    //get the value from ac model
    if (mExecCallback)
    {
        mExecCallback(this);
    }

    do
    {
        if (mDialValue <= mpValues[0])
        {
            found = TRUE;
            xOffset0 = FloatToInt32(mRadius0 * mpCosPoints[0]);
            yOffset0 = -FloatToInt32(mRadius0 * mpSinPoints[0]);
            xOffset1 = FloatToInt32(mRadius1 * mpCosPoints[0]);
            yOffset1 = -FloatToInt32(mRadius1 * mpSinPoints[0]);

            mlSinCos(&trig, mpPoints[0] + 0.7854F);
            cosSecondDeflection = trig.cos;
            sinSecondDeflection = trig.sin;

            xOffset2 = FloatToInt32(mRadius2 * trig.cos);
            yOffset2 = -FloatToInt32(mRadius2 * trig.sin);

            mlSinCos(&trig, mpPoints[0] - 0.7854F);
            xOffset3 = FloatToInt32(mRadius2 * trig.cos);
            yOffset3 = -FloatToInt32(mRadius2 * trig.sin);

            angle = mpPoints[0]; //Wombat778 7-09-04
        }
        else if (mDialValue >= mpValues[mEndPoints - 1])
        {
            found = TRUE;
            xOffset0 = FloatToInt32(mRadius0 * mpCosPoints[mEndPoints - 1]);
            yOffset0 = -FloatToInt32(mRadius0 * mpSinPoints[mEndPoints - 1]);
            xOffset1 = FloatToInt32(mRadius1 * mpCosPoints[mEndPoints - 1]);
            yOffset1 = -FloatToInt32(mRadius1 * mpSinPoints[mEndPoints - 1]);

            mlSinCos(&trig, mpPoints[mEndPoints - 1] + 0.7854F);
            cosSecondDeflection = trig.cos;
            sinSecondDeflection = trig.sin;

            xOffset2 = FloatToInt32(mRadius2 * trig.cos);
            yOffset2 = -FloatToInt32(mRadius2 * trig.sin);

            mlSinCos(&trig, mpPoints[mEndPoints - 1] - 0.7854F);
            xOffset3 = FloatToInt32(mRadius2 * trig.cos);
            yOffset3 = -FloatToInt32(mRadius2 * trig.sin);

            angle = mpPoints[mEndPoints - 1]; //Wombat778 7-09-04
        }
        else if ((mDialValue >= mpValues[i]) and (mDialValue < mpValues[i + 1]))
        {
            found = TRUE;

            delta = mpPoints[i + 1] - mpPoints[i];

            // sfr: removing this makes it possible to have CW and CCW rotation
            if ((mpCPManager->GetMajorVersion() == 0) and ((mpCPManager->GetMinorVersion() == 0)))
            {
                if (delta > 0.0F)
                {
                    delta -= (2 * PI);
                }
            }


            slope = delta / (mpValues[i + 1] - mpValues[i]);
            deflection = (float)(mpPoints[i] + (slope * (mDialValue - mpValues[i])));
            //Wombat778 3-26-04  Make a copy of the angle to be used when using rendered needles
            angle = deflection;

            if (deflection < -PI)
            {
                deflection += (2 * PI);
            }
            else if (deflection > PI)
            {
                deflection -= (2 * PI);
            }

            mlSinCos(&trig, deflection);
            cosDeflection = trig.cos;
            sinDeflection = trig.sin;
            mlSinCos(&trig, deflection + 0.7854F);
            cosSecondDeflection = trig.cos;
            sinSecondDeflection = trig.sin;


            xOffset0 = FloatToInt32(mRadius0 * cosDeflection);
            yOffset0 = -FloatToInt32(mRadius0 * sinDeflection);
            xOffset1 = FloatToInt32(mRadius1 * cosDeflection);
            yOffset1 = -FloatToInt32(mRadius1 * sinDeflection);

            xOffset2 = FloatToInt32(mRadius2 * cosSecondDeflection);
            yOffset2 = -FloatToInt32(mRadius2 * sinSecondDeflection);

            mlSinCos(&trig, deflection - 0.7854F);
            xOffset3 = FloatToInt32(mRadius2 * trig.cos);
            yOffset3 = -FloatToInt32(mRadius2 * trig.sin);
        }


        if (found)
        {
            mxPos0 = (int)((mxCenter + xOffset0) * mHScale);
            myPos0 = (int)((myCenter + yOffset0) * mVScale);

            mxPos1 = (int)((mxCenter + xOffset1) * mHScale);
            myPos1 = (int)((myCenter + yOffset1) * mVScale);

            mxPos2 = (int)((mxCenter + xOffset2) * mHScale);
            myPos2 = (int)((myCenter + yOffset2) * mVScale);

            mxPos3 = (int)((mxCenter + xOffset3) * mHScale);
            myPos3 = (int)((myCenter + yOffset3) * mVScale);
        }
        else
        {
            i++;
        }
    }
    while (( not found) and (i < mEndPoints));

    SetDirtyFlag(); //VWF FOR NOW
}

void CPDial::DisplayDraw()
{

    mDirtyFlag = TRUE;

    if ( not mDirtyFlag)
    {
        return;
    }

    if (IsRendered and DisplayOptions.bRender2DCockpit)
    {
        //Handle in DisplayBlit3D
        return;
    }

    OTWDriver.renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);

    //sfr: added for night lighting
    DWORD color[3]; // derived from original color

    for (int i = 0; i < 3; i++)
    {
        color[i] = OTWDriver.pCockpitManager->ApplyLighting(mColor[0][i], true);
    }

    OTWDriver.renderer->SetColor(color[1]);
    OTWDriver.renderer->Render2DTri(
        (float)mxPos0, (float)myPos0, (float)mxPos1,
        (float)myPos1, (float)mxPos2, (float)myPos2
    );
    OTWDriver.renderer->SetColor(color[2]);
    OTWDriver.renderer->Render2DTri(
        (float)mxPos0, (float)myPos0, (float)mxPos1,
        (float)myPos1, (float)mxPos3, (float)myPos3
    );
    mDirtyFlag = FALSE;
}


//Wombat778 3-26-04 Additional functions for rendering the image
//Wombat778 3-22-04 helper function to keep the displayblit3d tidy.
void RenderNeedlePoly(TextureHandle *pTex, tagRECT *destrect, GLint alpha, float angle)
{

    float x = destrect->left + ((float)(destrect->right - destrect->left) / 2.0f);
    float y = destrect->top + ((float)(destrect->bottom - destrect->top) / 2.0f);

    const float angsin = sin(angle);
    const float angcos = cos(angle);

    OTWDriver.renderer->CenterOriginInViewport();
    OTWDriver.renderer->SetViewport(-1.0F, 1.0F, 1.0F, -1.0F);

    // Setup vertices
    float fStartU = 0;
    float fMaxU = (float) pTex->m_nWidth / (float) pTex->m_nActualWidth;
    fMaxU -= fStartU;
    float fStartV = 0;
    float fMaxV = (float) pTex->m_nHeight / (float) pTex->m_nActualHeight;
    fMaxV -= fStartV;

    TwoDVertex pVtx[4];
    ZeroMemory(pVtx, sizeof(pVtx));

    pVtx[0].x = (Float_t)destrect->left;
    pVtx[0].y = (Float_t)destrect->top;
    pVtx[0].u = (Float_t)fStartU;
    pVtx[0].v = (Float_t)fStartV;
    pVtx[0].r = pVtx[0].g = pVtx[0].b = pVtx[0].a = 1.0f;

    pVtx[1].x = (Float_t)destrect->right;
    pVtx[1].y = (Float_t)destrect->top;
    pVtx[1].u = (Float_t)fMaxU;
    pVtx[1].v = (Float_t)fStartV;
    pVtx[1].r = pVtx[1].g = pVtx[1].b = pVtx[1].a = 1.0f;

    pVtx[2].x = (Float_t)destrect->right;
    pVtx[2].y = (Float_t)destrect->bottom;
    pVtx[2].u = (Float_t)fMaxU;
    pVtx[2].v = (Float_t)fMaxV;
    pVtx[2].r = pVtx[2].g = pVtx[2].b = pVtx[2].a = 1.0f;

    pVtx[3].x = (Float_t)destrect->left;
    pVtx[3].y = (Float_t)destrect->bottom;
    pVtx[3].u = (Float_t)fStartU;
    pVtx[3].v = (Float_t)fMaxV;
    pVtx[3].r = pVtx[3].g = pVtx[3].b = pVtx[3].a = 1.0f;

    for (int i = 0; i < 4; i++)
    {
        float tempx = pVtx[i].x;
        pVtx[i].x = x + (angcos * (pVtx[i].x - x)) + (-angsin * (pVtx[i].y - y));
        pVtx[i].y = y + (angsin * (tempx - x)) + (angcos * (pVtx[i].y - y));
    }

    // COBRA - RED - Pit Vibrations
    OTWDriver.pCockpitManager->AddTurbulence(pVtx);

    OTWDriver.renderer->context.RestoreState(alpha);
    OTWDriver.renderer->context.SelectTexture1((GLint) pTex);
    OTWDriver.renderer->context.DrawPrimitive(MPR_PRM_TRIFAN, MPR_VI_COLOR bitor MPR_VI_TEXTURE, 4, pVtx, sizeof(pVtx[0]));
}


void CPDial::DisplayBlit3D()
{

    mDirtyFlag = TRUE;

    if ( not mDirtyFlag)
    {
        return;
    }

    if ( not IsRendered or not DisplayOptions.bRender2DCockpit)
    {
        return;
    }

    if (g_bFilter2DPit)
    {
        //Wombat778 3-30-04 Add option to filter
        //adjust the angle so that a needle pointing up corresponds to up on the template
        RenderNeedlePoly(m_arrTex[0], &mDestRect, STATE_CHROMA_TEXTURE, -(angle - (0.5f * PI)));
    }
    else
    {
        //adjust the angle so that a needle pointing up corresponds to up on the template
        RenderNeedlePoly(m_arrTex[0], &mDestRect, STATE_ALPHA_TEXTURE_NOFILTER, -(angle - (0.5f * PI)));
    }

    mDirtyFlag = FALSE;
}


void CPDial::CreateLit(void)
{
    if (IsRendered and DisplayOptions.bRender2DCockpit)
    {
        try
        {
            const DWORD dwMaxTextureWidth =
                mpOTWImage->GetDisplayDevice()->GetDefaultRC()->m_pD3DHWDeviceDesc->dwMaxTextureWidth;
            const DWORD dwMaxTextureHeight =
                mpOTWImage->GetDisplayDevice()->GetDefaultRC()->m_pD3DHWDeviceDesc->dwMaxTextureHeight;
            m_pPalette =
                new PaletteHandle(mpOTWImage->GetDisplayDevice()->GetDefaultRC()->m_pDD, 32, 256);

            if ( not m_pPalette)
            {
                throw _com_error(E_OUTOFMEMORY);
            }

            // Check if we can use a single texture
            if (
                ((int)dwMaxTextureWidth >= mSrcRect.right - mSrcRect.left) and 
                ((int)dwMaxTextureHeight >= mSrcRect.bottom - mSrcRect.top)
            )
            {
                TextureHandle *pTex = new TextureHandle;

                if ( not pTex)
                {
                    throw _com_error(E_OUTOFMEMORY);
                }

                m_pPalette->AttachToTexture(pTex);

                if (
 not pTex->Create("CPDial", MPR_TI_PALETTE bitor MPR_TI_CHROMAKEY, 8,
                                  (int)(mSrcRect.right - mSrcRect.left),
                                  (int)(mSrcRect.bottom - mSrcRect.top))
                )
                {
                    throw _com_error(E_FAIL);
                }

                if ( not pTex->Load(0, 0xFFFF0000, (BYTE*)mpSourceBuffer, true, true))
                {
                    // soon to be re-loaded by CPSurface::Translate3D
                    throw _com_error(E_FAIL);
                }

                m_arrTex.push_back(pTex);
            }
        }
        catch (_com_error e)
        {
            MonoPrint("CPDial::CreateLit - Error 0x%X (%s)\n", e.Error(), e.ErrorMessage());
            DiscardLit();
        }
    }
}

//Wombat778 3-24-04 end
