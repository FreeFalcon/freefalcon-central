/***************************************************************************\
    Dispopts.h
    Miro "Jammer" Torrielli
    06Oct03

 - Begin Major Rewrite
 - sfr: this file needs serialization badly. Its a mess to add new options
\***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "dispopts.h"
#include "f4find.h"
#include "graphics/include/devmgr.h"
#include "dispcfg.h"
#include "Debuggr.h"

DisplayOptionsClass DisplayOptions;

unsigned int DisplayOptionsClass::iDeviceCaps = D3DDEVCAPS_HWTRANSFORMANDLIGHT; // sfr: default display caps

DisplayOptionsClass::DisplayOptionsClass(void)
{
    Initialize();
}

void DisplayOptionsClass::Initialize(void)
{
    // Artscout - 2026: zero the whole object first so padding bytes and any field not explicitly set
    // below are 0, never 0xCC (debug uninitialized fill). SaveOptions does a raw fwrite(this), so any
    // leftover 0xCC would be persisted and read back as garbage on the next run -- this is exactly how
    // DispWidth became 0xCCCC (=52428) -> 52428x52428 swapchain/depth/MSAA death on 3D entry. Safe:
    // DisplayOptionsClass has no virtual methods (no vtable to clobber).
    memset(this, 0, sizeof(*this));

    DispWidth = 1920;	// 3D default -- Full HD (was 1024x768)
    DispHeight = 1080;
    DispDepth = 32;  //Cobra - always use 32-bit depth
    DispVideoCard = 0;
    DispVideoDriver = 0;
    bRender2Texture = TRUE;
    bAnisotropicFiltering = TRUE;
    bLinearMipFiltering = TRUE;
    bMipmapping = TRUE;
    bZBuffering = TRUE;
    bRender2DCockpit = TRUE;
    bFontTexelAlignment = TRUE;
    bScreenCoordinateBiasFix = true; //Wombat778 4-01-04
    bSpecularLighting = true;
    bWindowed = false; // #33: default the 3D session to fullscreen
    m_texMode = TEX_MODE_DDS;

    FalconDisplay.SetSimMode(DispWidth, DispHeight, DispDepth);
}

int DisplayOptionsClass::LoadOptions(char *filename)
{
    DWORD size;
    FILE *fp;
    size_t success = 0;
    char path[_MAX_PATH];

    sprintf(path, "%s\\config\\%s.dsp", FalconDataDirectory, filename);
    fp = fopen(path, "rb");

    if ( not fp)
    {
        MonoPrint("Couldn't open display options\n");
        Initialize();
        fp = fopen(path, "wb");
        fclose(fp);
        return TRUE;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size not_eq sizeof(class DisplayOptionsClass))
    {
        MonoPrint("Old display options format detected\n");
        Initialize();
        fclose(fp);
        return TRUE;
    }

    success = fread(this, 1, size, fp);
    fclose(fp);

    if (success not_eq size)
    {
        MonoPrint("Failed to read display options\n", filename);
        Initialize();
        return TRUE;
    }

    //========================================
    // FRB - Force Z-Buffering
    DisplayOptions.bZBuffering = TRUE;
    // FRB - Force Specular Lighting
    //DisplayOptions.bSpecularLighting = TRUE;
    //DDS textures only
    DisplayOptions.m_texMode = TEX_MODE_DDS;
    DisplayOptions.DispDepth = 32;  //Cobra - always use 32-bit depth
    //========================================

    // Artscout - 2026: sanitize loaded values. A corrupt/old options file (written from a struct with
    // uninitialized fields in a debug session = 0xCCCC) poisons D3D11: DispWidth/Height = 52428 ->
    // 52428x52428 swapchain/depth/MSAA on 3D entry (CreateTexture2D INVALIDDIMENSIONS + swapchain "no
    // buffers" -> dead device); bRender2DCockpit = FALSE routes the cockpit/cursor to the dead D3D7
    // DDraw path (NULL m_pBltTarget crash) AND skips the rendered-cursor texture bake (Translate3D),
    // so the cursor vanishes. Clamp the resolution and, under D3D11, force the rendered path (the only
    // working one there).
    if (DispWidth  < 320 or DispWidth  > 16384) DispWidth  = 1920;
    if (DispHeight < 240 or DispHeight > 16384) DispHeight = 1080;
    {
        extern bool g_bUseD3D11;
        if (g_bUseD3D11)
        {
            DisplayOptions.bRender2DCockpit = TRUE;
            // Artscout - 2026: force render-to-texture under D3D11. The bRender2Texture==FALSE path is a
            // dead DDraw blit workaround (gmcomposit.cpp "heart of darkness": output->targetSurface() and
            // m_pBackupBuffer/m_pRenderBuffer Blt'd directly), all NULL under D3D11. With it FALSE the GM
            // radar never renders into its off-screen RTT -> ground map is black, and SetBeam crashes on
            // m_pBackupBuffer->targetSurface()->Blt (NULL deref). TRUE = the only working path here.
            DisplayOptions.bRender2Texture = TRUE;
        }
    }

    const char *buf;
    int i = 0;

    // Make sure the chosen sim video driver is still legal
    buf = FalconDisplay.devmgr.GetDriverName(i);

    while (buf)
    {
        i++;
        buf = FalconDisplay.devmgr.GetDriverName(i);
    }

    if (i <= DispVideoDriver)
    {
        DispVideoDriver = 0;
        DispVideoCard = 0;
    }

    // Make sure the chosen sim video device is still legal
    buf = FalconDisplay.devmgr.GetDeviceName(DispVideoDriver, i);

    while (buf)
    {
        i++;
        buf = FalconDisplay.devmgr.GetDeviceName(DispVideoDriver, i);
    }

    if (i <= DispVideoCard)
    {
        DispVideoDriver = 0;
        DispVideoCard = 0;
    }

    FalconDisplay.SetSimMode(DispWidth, DispHeight, DispDepth);

    return TRUE;
}

int DisplayOptionsClass::SaveOptions(void)
{
    FILE *fp;
    size_t success = 0;
    char path[_MAX_PATH];

    sprintf(path, "%s\\config\\display.dsp", FalconDataDirectory);

    if ((fp = fopen(path, "wb")) == NULL)
    {
        MonoPrint("Couldn't save display options");
        return FALSE;
    }

    // Artscout - 2026: never persist garbage dimensions -- a corrupt save poisons the next load
    // (DispWidth=52428 -> 52428x52428 device death on 3D entry). Clamp to sane bounds before writing.
    if (DispWidth  < 320 or DispWidth  > 16384) DispWidth  = 1920;
    if (DispHeight < 240 or DispHeight > 16384) DispHeight = 1080;

    success = fwrite(this, sizeof(class DisplayOptionsClass), 1, fp);
    fclose(fp);

    if (success == 1)
        return TRUE;

    return FALSE;
}

void DisplayOptionsClass::SetDevCaps(unsigned int devCaps)
{
    DisplayOptionsClass::iDeviceCaps = devCaps;
}

unsigned int DisplayOptionsClass::GetDevCaps()
{
    return DisplayOptionsClass::iDeviceCaps;
}
