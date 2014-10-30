/***************************************************************************\
    Palette.h
    Scott Randolph
    April 1, 1997

 Provide a class to manage MPR palettes.
\***************************************************************************/
#include <cISO646>
#include "stdafx.h"
#include "TOD.h"
#include "Palette.h"
#include "Tex.h"


static DXContext *rc = NULL;

//sfr: for debugging I put this in the cpp file
Palette::Palette()
{
    refCount = 0;
    palHandle = NULL;
    memset(paletteData, 0, sizeof(paletteData));
};

Palette::~Palette()
{
    ShiAssert(refCount == 0);
};


/***************************************************************************\
 Store some useful global information.  The RC is used for loading.
 This means that at present, only one device at a time can load textures
 through this interface.
\***************************************************************************/
void Palette::SetupForDevice(DXContext *texRC)
{
    rc = texRC;
}


/***************************************************************************\
 This is called when we're done working with a given device (as
 represented by an RC).
\***************************************************************************/
//void Palette::CleanupForDevice( MPRHandle_t texRC )
void Palette::CleanupForDevice(DXContext *texRC)
{
    rc = NULL;
}


/***************************************************************************\
 Given a 256 entry, 32 bit per element palette, initialize our data
 structures.  The data that is passed in is copied, so can be freed
 by the caller after we return.
\***************************************************************************/
void Palette::Setup32(DWORD *data32)
{
    ShiAssert(palHandle == NULL);
    ShiAssert(data32);

    // Start our reference count at 1
    refCount = 1;

    // Copy the palette entries
    memcpy(paletteData, data32, sizeof(paletteData));
}


/***************************************************************************\
 Given a 256 entry, 24 bit per element palette, initialize our data
 structures.  The data that is passed in is copied, so can be freed
 by the caller after we return.
\***************************************************************************/
void Palette::Setup24(BYTE *data24)
{
    DWORD *to;
    BYTE *stop;

    ShiAssert(palHandle == NULL);
    ShiAssert(data24);

    // Start our reference count at 1
    refCount = 1;

    // Convert from 24 to 32 bit palette entries
    to = &paletteData[0];
    stop = data24 + 768;

    while (data24 < stop)
    {
        *to = ((BYTE)(*(data24))) |
              ((BYTE)(*(data24 + 1)) <<  8) |
              ((BYTE)(*(data24 + 2)) << 16) |
              ((BYTE)(*(data24)) << 24); // Repeat Red component for alpha channel
        to++;
        data24 += 3;
    }
}


/***************************************************************************\
 Set the MPR palette entries for the given palette.
\***************************************************************************/
void Palette::UpdateMPR(DWORD *pal)
{
    ShiAssert(rc not_eq NULL);
    ShiAssert(pal);

    if ( not rc) // JB 010615 CTD
        return;

    // OW FIXME Error checking
    if (palHandle == NULL)
        palHandle = new PaletteHandle(rc->m_pDD, 32, 256);

    ShiAssert(palHandle);
    palHandle->Load(MPR_TI_PALETTE, 32, 0, 256, (BYTE*)pal);
}


/***************************************************************************\
 Note interest in this palette.
\***************************************************************************/
void Palette::Reference(void)
{
    //JAM 10Oct03 - WTF is going on here????
    ShiAssert(refCount >= 0);

    refCount++;
}


/***************************************************************************\
 Free MPR palette resources (if we were the last one using it).
\***************************************************************************/
int Palette::Release(void)
{
    //JAM 30Sep03
    // ShiAssert( refCount > 0 );

    if (refCount > 0) 
        refCount--;

    //JAM

    if (refCount == 0)
    {
        if (palHandle)
        {
            ShiAssert(rc not_eq NULL);

            delete palHandle;
            palHandle = NULL;
        }
    }

    return refCount;
}


/***************************************************************************\
    Update the light levels on our MPR palette without affecting our
 stored palette entries.
\***************************************************************************/
void Palette::LightTexturePalette(Tcolor *light)
{
    DWORD pal[256];
    DWORD *to, *stop;
    BYTE *from;
    float r, g, b;

    r = light->r;
    g = light->g;
    b = light->b;

    // Set up the loop control
    to = pal + 1;
    from = (BYTE*)(paletteData + 1);
    stop = pal + 256;

    // We skip the first entry to allow for chroma keying
    pal[0] = paletteData[0];

    // Build the lite version of the palette in temporary storage
    while (to < stop)
    {
        *to  = (FloatToInt32(*(from)   * r)) // Red
               bitor (FloatToInt32(*(from + 1) * g) << 8) // Green
               bitor (FloatToInt32(*(from + 2) * b) << 16) // Blue
               bitor ((*(from + 3)) << 24); // Alpha
        from += 4;
        to ++;
    }

    // Send the new lite palette to MPR
    UpdateMPR(pal);
}



/***************************************************************************\
    Update the light levels on our MPR palette without affecting our
 stored palette entries.

 Works within a palette entry range
\***************************************************************************/
void Palette::LightTexturePaletteRange(Tcolor *light, int palStart, int palEnd)
{
    DWORD pal[256];
    DWORD *to, *stop;
    BYTE *from;
    float r, g, b;

    r = light->r;
    g = light->g;
    b = light->b;

    // Set up the loop control
    to = pal;
    from = (BYTE*)(paletteData);
    stop = pal + palStart;

    // Just copy the entries until we reach the start index
    while (to < stop)
    {
        *to++ = *(DWORD*)from;
        from += 4;
    }

    // Now light the specified range
    stop = pal + palEnd + 1;

    while (to < stop)
    {
        *to++  = (FloatToInt32(*(from)   * r)) // Red
                 bitor (FloatToInt32(*(from + 1) * g) << 8) // Green
                 bitor (FloatToInt32(*(from + 2) * b) << 16) // Blue
                 bitor ((*(from + 3)) << 24); // Alpha
        from += 4;
    }

    // Now copy the values beyond the ending index
    stop = pal + 256;

    while (to < stop)
    {
        *to++ = *(DWORD*)from;
        from += 4;
    }

    // Send the new lite palette to MPR
    UpdateMPR(pal);
}



/***************************************************************************\
    Update the light levels on our MPR palette without affecting our
 stored palette entries.  This one does special processing to brighten
 certain palette entries at night.
\***************************************************************************/
void Palette::LightTexturePaletteBuilding(Tcolor *light)
{
    DWORD pal[256];
    DWORD *to, *stop;
    BYTE *from;
    float r, g, b;

    r = light->r;
    g = light->g;
    b = light->b;

    // Special case for NVG mode
    if (TheTimeOfDay.GetNVGmode())
    {
        r = 0.0f;
        g = NVG_LIGHT_LEVEL;
        b = 0.0f;
    }

    // Set up the loop control
    to = pal + 1;
    from = (BYTE*)(paletteData + 1);
    stop = pal + 248;

    // We skip the first entry to allow for chroma keying
    pal[0] = paletteData[0];

    // Darken the "normal" palette entries
    while (to < stop)
    {
        *to  = (FloatToInt32(*(from)   * r)) // Red
               bitor (FloatToInt32(*(from + 1) * g) << 8) // Green
               bitor (FloatToInt32(*(from + 2) * b) << 16) // Blue
               bitor ((*(from + 3)) << 24); // Alpha
        from += 4;
        to ++;
    }

    // Only turn on the lights if it is dark enough
    if (light->g > 0.5f)
    {
        stop = pal + 256;

        while (to < stop)
        {
            *to  = (FloatToInt32(*(from)   * r)) // Red
                   bitor (FloatToInt32(*(from + 1) * g) << 8) // Green
                   bitor (FloatToInt32(*(from + 2) * b) << 16) // Blue
                   bitor ((*(from + 3)) << 24); // Alpha
            from += 4;
            to ++;
        }
    }
    else
    {
        // TODO: Blend these in gradually
        if (TheTimeOfDay.GetNVGmode())
        {
            *to = 0xFF00FF00;
            to++;
            *to = 0xFF00FF00;
            to++;
            *to = 0xFF00FF00;
            to++;
            *to = 0xFF00FF00;
            to++;
            *to = 0xFF00FF00;
            to++;
            *to = 0xFF00FF00;
            to++;
            *to = 0xFF00FF00;
            to++;
            *to = 0xFF00FF00;
            to++;
        }
        else
        {
            *to = 0xFF0000FF;
            to++;
            *to = 0xFF0F30BE;
            to++;
            *to = 0xFFFF0000;
            to++;
            *to = 0xFFAD0000;
            to++;
            *to = 0xFFABD34C;
            to++;
            *to = 0xFF9BB432;
            to++;
            *to = 0xFF87C5F0;
            to++;
            *to = 0xFF61B2EA;
            to++;
        }
    }

    // Send the new lite palette to MPR
    UpdateMPR(pal);
}



/***************************************************************************\
    Update the light levels on our MPR palette without affecting our
 stored palette entries.
\***************************************************************************/
void Palette::LightTexturePaletteReflection(Tcolor *light)
{
    DWORD pal[256];
    DWORD *to, *stop;
    BYTE *from;
    float r, g, b;

    r = light->r;
    g = light->g;
    b = light->b;

    // Special case for NVG mode
    if (TheTimeOfDay.GetNVGmode())
    {
        r = 0.0f;
        g = NVG_LIGHT_LEVEL;
        b = 0.0f;
    }

    // Set up the loop control
    to = pal + 1;
    from = (BYTE*)(paletteData + 1);
    stop = pal + 256;

    // We skip the first entry to allow for chroma keying
    pal[0] = paletteData[0];

    // Build the lite version of the palette in temporary storage
    while (to < stop)
    {
        *to  = (FloatToInt32(*(from)   * r)) // Red
               bitor (FloatToInt32(*(from + 1) * g) << 8) // Green
               bitor (FloatToInt32(*(from + 2) * b) << 16) // Blue
               bitor (0x26000000); // Alpha
        from += 4;
        to ++;
    }

    // Send the new lite palette to MPR
    UpdateMPR(pal);
}

// PaletteHandle
///////////////////////////////////////

#ifdef _DEBUG
DWORD PaletteHandle::m_dwNumHandles = 0; // Number of instances
DWORD PaletteHandle::m_dwTotalBytes = 0; // Total number of bytes allocated (including bitmap copies and object size)
#endif

PaletteHandle::PaletteHandle(IDirectDraw7 *pDD, UInt16 PalBitsPerEntry, UInt16 PalNumEntries)
{
    DWORD dwFlags = DDPCAPS_8BIT;

    if (PalNumEntries == 0x100) dwFlags or_eq DDPCAPS_ALLOW256;

    DWORD pal[256];
    ZeroMemory(pal, sizeof(DWORD) * PalNumEntries);

    HRESULT hr = 0;

    if (pDD)
        hr = pDD->CreatePalette(dwFlags, (LPPALETTEENTRY) pal, &m_pIDDP, NULL);

    ShiAssert(SUCCEEDED(hr));

    if (SUCCEEDED(hr))
        m_nNumEntries = PalNumEntries;

    m_pPalData = new DWORD[256];
    ShiAssert(m_pPalData);

#ifdef _DEBUG
    InterlockedIncrement((long *) &m_dwNumHandles); // Number of instances
    InterlockedExchangeAdd((long *) &m_dwTotalBytes, sizeof(*this));
    InterlockedExchangeAdd((long *) &m_dwTotalBytes, sizeof(DWORD[256]) * 2);
#endif
}

PaletteHandle::~PaletteHandle()
{
#ifdef _DEBUG
    //InterlockedDecrement((long *) &m_dwNumHandles); // Number of instances
    //InterlockedExchangeAdd((long *) &m_dwTotalBytes, -sizeof(*this));
    //InterlockedExchangeAdd((long *) &m_dwTotalBytes, -(sizeof(DWORD[256]) * 2));
    //InterlockedExchangeAdd((long *) &m_dwTotalBytes, -(m_arrAttachedTextures.size() * 4));
#endif

    // Detach from textures
    for (int i = 0 ; (static_cast<unsigned int>(i) < m_arrAttachedTextures.size()) ; i++)
    {
        m_arrAttachedTextures[i]->PaletteDetach(this);
    }

    m_arrAttachedTextures.clear();

    // Release palette interface
    if (m_pIDDP)
    {
        m_pIDDP->Release();
        m_pIDDP = NULL;
    }

    if (m_pPalData) delete[] m_pPalData;
}

void PaletteHandle::Load(UInt16 info, UInt16 PalBitsPerEntry, UInt16 index, UInt16 entries, UInt8 *PalBuffer)
{
    ShiAssert(m_pIDDP and entries <= m_nNumEntries);

    if ( not m_pIDDP or not m_pPalData) return;

    if ((DWORD *) PalBuffer not_eq m_pPalData)
        memcpy(m_pPalData, PalBuffer, sizeof(DWORD) * entries);

    // Convert palette
    DWORD dwTmp;

    for (int i = 0; i < m_nNumEntries; i++)
    {
        dwTmp = m_pPalData[i];
        m_pPalData[i] = RGBA_MAKE(RGBA_GETBLUE(dwTmp), RGBA_GETGREEN(dwTmp), RGBA_GETRED(dwTmp), RGBA_GETALPHA(dwTmp));
    }

    HRESULT hr = m_pIDDP->SetEntries(NULL, index, entries, (LPPALETTEENTRY) PalBuffer);
    ShiAssert(SUCCEEDED(hr));

    if (SUCCEEDED(hr))
    {
        // Reload attached textures
        for (int i = 0 ; static_cast<unsigned int>(i) < m_arrAttachedTextures.size() ; i++)
            m_arrAttachedTextures[i]->Reload();
    }
}

void PaletteHandle::AttachToTexture(TextureHandle *pTex)
{
    ShiAssert(pTex);

    if ( not pTex) return;

    std::vector<TextureHandle *>::iterator it = GetAttachedTextureIndex(pTex);

    ShiAssert(it == m_arrAttachedTextures.end()); // do not attach twice

    if (it not_eq m_arrAttachedTextures.end())
        return;

    m_arrAttachedTextures.push_back(pTex);
    pTex->PaletteAttach(this);

#ifdef _DEBUG
    InterlockedExchangeAdd((long *) &m_dwTotalBytes, sizeof(pTex));
#endif
}

void PaletteHandle::DetachFromTexture(TextureHandle *pTex)
{
    ShiAssert(pTex);

    if ( not pTex) return;

    std::vector<TextureHandle *>::iterator it = GetAttachedTextureIndex(pTex);
    ShiAssert(it not_eq m_arrAttachedTextures.end()); // do not detach twice

    if (it == m_arrAttachedTextures.end())
        return;

    m_arrAttachedTextures.erase(it);
    pTex->PaletteDetach(this);

#ifdef _DEBUG
    //InterlockedExchangeAdd((long *) &m_dwTotalBytes, -sizeof(pTex));
#endif
}

std::vector<TextureHandle *>::iterator PaletteHandle::GetAttachedTextureIndex(TextureHandle *pTex)
{
    std::vector<TextureHandle *>::iterator it;

    for (it = m_arrAttachedTextures.begin(); it not_eq m_arrAttachedTextures.end(); it++)
        if (*it == pTex) return it;

    return m_arrAttachedTextures.end();
}

#ifdef _DEBUG
void PaletteHandle::MemoryUsageReport()
{
}
#endif
