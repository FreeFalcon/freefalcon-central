/***************************************************************************\
    Dispopts.h
    Miro "Jammer" Torrielli
    06Oct03

 - Begin Major Rewrite
\***************************************************************************/
#ifndef _DISPLAY_OPTIONS_
#define _DISPLAY_OPTIONS_


class DisplayOptionsClass
{
public:
    unsigned short DispWidth;
    unsigned short DispHeight;
    unsigned char DispVideoCard;
    unsigned char DispVideoDriver;
    int DispDepth;
    bool bRender2Texture;
    bool bAnisotropicFiltering;
    bool bLinearMipFiltering;
    bool bMipmapping;
    bool bZBuffering;
    bool bRender2DCockpit;
    bool bFontTexelAlignment;
    bool bSpecularLighting;
    bool bScreenCoordinateBiasFix; //Wombat778 4-01-04
    bool
        bWindowed; // #33: run the 3D session in a window (false = borderless fullscreen)

    // Artscout - 2026: graphics options added to the Graphics/Advanced setup pages. Persisted here
    // (XML now, see dispopts.cpp) and pushed into the engine globals at startup (winmain) + on Apply.
    bool bMsaaEnable; // 3D-scene + RTT multisample AA on/off (Graphics page)
    int nMsaaSamples; // requested MSAA sample count 1..8 (snapped to a supported level in the backend)
    int nAnisotropicSamples; // max anisotropy 1..16 for the anisotropic filter (Graphics page). On/off = bAnisotropicFiltering.
    bool
        bUseOpenXR; // VR via OpenXR (Advanced page) -> g_bUseOpenXR. Default OFF = flat desktop path.
    bool
        bUseQuadViews; // foveated quad-views (Advanced page) -> g_bUseQuadViews
    int nVrResolutionScale; // per-eye swapchain resolution scale in percent 50..100 -> g_nVrResolutionScale
    int nRenderer; // Artscout - 2026 (#104): render backend -- 0 = DirectX 12, 1 = Vulkan -> g_bUseVulkan

    enum TEXMODE
    {
        TEX_MODE_16 = 70159,
        TEX_MODE_32,
        TEX_MODE_DDS,
    };
    TEXMODE m_texMode;

    DisplayOptionsClass(void);
    void Initialize(void);
    int LoadOptions(char *filename = "display");
    int SaveOptions(void);
    static void SetDevCaps(unsigned int devCaps);
    static unsigned int GetDevCaps();

private:
    // sfr: used for enumerating only some drivers, not a player option but a command line switch
    // so static, it wont be saved
    static unsigned int iDeviceCaps;
};

extern DisplayOptionsClass DisplayOptions;


#endif
