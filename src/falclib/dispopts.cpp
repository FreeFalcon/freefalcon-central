/***************************************************************************\
    Dispopts.h
    Miro "Jammer" Torrielli
    06Oct03

 - Begin Major Rewrite
 - sfr: this file needs serialization badly. Its a mess to add new options
\***************************************************************************/
// Artscout - 2026: tinyxml2 FIRST, before any falclib/windows header. falclib does
// #define new DEBUG_NEW, which breaks tinyxml2's inline methods (XMLDocument becomes
// "incomplete"); STL/tinyxml2 must precede that. Same rule as controlsxml.cpp. We qualify
// types as tinyxml2:: (there is another global XMLDocument from MSXML/Falcon headers).
#include "extlibs/tinyxml2/tinyxml2.h"   // include root ..\.. = src\

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

    // Artscout - 2026: new Graphics/Advanced options. MSAA on (4x) by default. VR OFF by default so the
    // flat desktop path stays the norm -- the user opts into OpenXR via the Advanced checkbox. Res scale 100%.
    bMsaaEnable = true;
    nMsaaSamples = 4;
    nAnisotropicSamples = 16;   // Artscout - 2026: default max anisotropy (on/off = bAnisotropicFiltering)
    bUseOpenXR = false;
    bUseQuadViews = false;
    nVrResolutionScale = 100;

    m_texMode = TEX_MODE_DDS;

    FalconDisplay.SetSimMode(DispWidth, DispHeight, DispDepth);
}

int DisplayOptionsClass::LoadOptions(char *filename)
{
    char path[_MAX_PATH];

    // Artscout - 2026: display options are now XML (display.xml) instead of the old raw-fwrite(this)
    // binary display.dsp. The binary format silently persisted struct padding / uninitialized 0xCC and
    // had no field names, so adding an option meant a size mismatch that wiped the file -- and a single
    // garbage byte (DispWidth=52428) killed the device on 3D entry (see options-binary-corruption). XML
    // is self-describing and forward/backward compatible: unknown elements are ignored, missing elements
    // keep their Initialize() default. Start from defaults, then overlay whatever the file provides.
    Initialize();

    sprintf(path, "%s\\config\\%s.xml", FalconDataDirectory, filename);

    tinyxml2::XMLDocument doc;

    if (doc.LoadFile(path) not_eq tinyxml2::XML_SUCCESS)
    {
        // No file yet (first run / fresh install) or unreadable -> keep defaults and write a clean one.
        MonoPrint("Display options: no/invalid %s -> defaults\n", path);
        SaveOptions();
        return TRUE;
    }

    tinyxml2::XMLElement *root = doc.FirstChildElement("display");

    if (root)
    {
        int tmp;
        tinyxml2::XMLElement *e;

        if ((e = root->FirstChildElement("resolution")))
        {
            if (e->QueryIntAttribute("width",  &tmp) == tinyxml2::XML_SUCCESS) DispWidth  = (unsigned short)tmp;
            if (e->QueryIntAttribute("height", &tmp) == tinyxml2::XML_SUCCESS) DispHeight = (unsigned short)tmp;
            e->QueryIntAttribute("depth", &DispDepth);
        }

        if ((e = root->FirstChildElement("video")))
        {
            if (e->QueryIntAttribute("card",   &tmp) == tinyxml2::XML_SUCCESS) DispVideoCard   = (unsigned char)tmp;
            if (e->QueryIntAttribute("driver", &tmp) == tinyxml2::XML_SUCCESS) DispVideoDriver = (unsigned char)tmp;
        }

        if ((e = root->FirstChildElement("render")))
        {
            e->QueryBoolAttribute("render2texture",  &bRender2Texture);
            e->QueryBoolAttribute("render2Dcockpit", &bRender2DCockpit);
            e->QueryBoolAttribute("anisotropic",     &bAnisotropicFiltering);
            e->QueryIntAttribute ("anisoLevel",      &nAnisotropicSamples);   // Artscout - 2026: max anisotropy 1..16
            e->QueryBoolAttribute("linearmip",       &bLinearMipFiltering);
            e->QueryBoolAttribute("mipmapping",      &bMipmapping);
            e->QueryBoolAttribute("zbuffer",         &bZBuffering);
            e->QueryBoolAttribute("fontTexel",       &bFontTexelAlignment);
            e->QueryBoolAttribute("specular",        &bSpecularLighting);
            e->QueryBoolAttribute("screenBiasFix",   &bScreenCoordinateBiasFix);
            if (e->QueryIntAttribute("texMode", &tmp) == tinyxml2::XML_SUCCESS) m_texMode = (TEXMODE)tmp;
        }

        if ((e = root->FirstChildElement("window")))
            e->QueryBoolAttribute("windowed", &bWindowed);

        if ((e = root->FirstChildElement("msaa")))
        {
            e->QueryBoolAttribute("enable",  &bMsaaEnable);
            e->QueryIntAttribute("samples",  &nMsaaSamples);
        }

        if ((e = root->FirstChildElement("vr")))
        {
            e->QueryBoolAttribute("openxr",    &bUseOpenXR);
            e->QueryBoolAttribute("quadviews", &bUseQuadViews);
            e->QueryIntAttribute("resScale",   &nVrResolutionScale);
        }
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

    // Artscout - 2026: clamp the new option ranges (UI slider bounds; protects against a hand-edited XML).
    if (nMsaaSamples       < 1  or nMsaaSamples       > 8)   nMsaaSamples       = 4;
    if (nAnisotropicSamples < 1 or nAnisotropicSamples > 16) nAnisotropicSamples = 16;
    if (nVrResolutionScale < 50 or nVrResolutionScale > 100) nVrResolutionScale = 100;

    {
        extern bool g_bUseD3D11, g_bUseD3D12;
        if (g_bUseD3D11 or g_bUseD3D12)   // #DX12: GPU mode has no DDraw -> force RTT/2D-cockpit like D3D11
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
    char path[_MAX_PATH];

    sprintf(path, "%s\\config\\display.xml", FalconDataDirectory);

    // Artscout - 2026: never persist garbage dimensions -- a corrupt save poisons the next load
    // (DispWidth=52428 -> 52428x52428 device death on 3D entry). Clamp to sane bounds before writing.
    if (DispWidth  < 320 or DispWidth  > 16384) DispWidth  = 1920;
    if (DispHeight < 240 or DispHeight > 16384) DispHeight = 1080;
    if (nMsaaSamples       < 1  or nMsaaSamples       > 8)   nMsaaSamples       = 4;
    if (nAnisotropicSamples < 1 or nAnisotropicSamples > 16) nAnisotropicSamples = 16;
    if (nVrResolutionScale < 50 or nVrResolutionScale > 100) nVrResolutionScale = 100;

    tinyxml2::XMLDocument doc;
    doc.InsertEndChild(doc.NewDeclaration());
    tinyxml2::XMLElement *root = doc.NewElement("display");
    doc.InsertEndChild(root);

    tinyxml2::XMLElement *e;

    e = doc.NewElement("resolution");
    e->SetAttribute("width",  (int)DispWidth);
    e->SetAttribute("height", (int)DispHeight);
    e->SetAttribute("depth",  DispDepth);
    root->InsertEndChild(e);

    e = doc.NewElement("video");
    e->SetAttribute("card",   (int)DispVideoCard);
    e->SetAttribute("driver", (int)DispVideoDriver);
    root->InsertEndChild(e);

    e = doc.NewElement("render");
    e->SetAttribute("render2texture",  bRender2Texture);
    e->SetAttribute("render2Dcockpit", bRender2DCockpit);
    e->SetAttribute("anisotropic",     bAnisotropicFiltering);
    e->SetAttribute("anisoLevel",      nAnisotropicSamples);   // Artscout - 2026: max anisotropy 1..16
    e->SetAttribute("linearmip",       bLinearMipFiltering);
    e->SetAttribute("mipmapping",      bMipmapping);
    e->SetAttribute("zbuffer",         bZBuffering);
    e->SetAttribute("fontTexel",       bFontTexelAlignment);
    e->SetAttribute("specular",        bSpecularLighting);
    e->SetAttribute("screenBiasFix",   bScreenCoordinateBiasFix);
    e->SetAttribute("texMode",         (int)m_texMode);
    root->InsertEndChild(e);

    e = doc.NewElement("window");
    e->SetAttribute("windowed", bWindowed);
    root->InsertEndChild(e);

    e = doc.NewElement("msaa");
    e->SetAttribute("enable",  bMsaaEnable);
    e->SetAttribute("samples", nMsaaSamples);
    root->InsertEndChild(e);

    e = doc.NewElement("vr");
    e->SetAttribute("openxr",    bUseOpenXR);
    e->SetAttribute("quadviews", bUseQuadViews);
    e->SetAttribute("resScale",  nVrResolutionScale);
    root->InsertEndChild(e);

    if (doc.SaveFile(path) not_eq tinyxml2::XML_SUCCESS)
    {
        MonoPrint("Couldn't save display options (%s)\n", path);
        return FALSE;
    }

    return TRUE;
}

void DisplayOptionsClass::SetDevCaps(unsigned int devCaps)
{
    DisplayOptionsClass::iDeviceCaps = devCaps;
}

unsigned int DisplayOptionsClass::GetDevCaps()
{
    return DisplayOptionsClass::iDeviceCaps;
}
