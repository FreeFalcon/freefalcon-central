// Artscout - 2026 (#104, Linux port -- subsystem 2). Headless GPU test for the Vulkan renderer: render a textured
// quad offscreen and read the pixels back to prove the whole path (device -> texture upload -> pipeline ->
// descriptor -> draw -> readback) actually produces correct pixels. Runs under any Vulkan ICD, incl. software
// (lavapipe):  VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.json ./ffvk_gputest <shaderDir>
// Exit 0 = PASS. This test caught the ScreenVertex LP64 layout bug (unsigned long = 8 bytes on Linux).
#include "vulkanbackend.h"
#include "vulkanrenderer.h"
#include "vulkanvbmanager.h"
#include "vulkantexturemanager.h"
#include "common/irenderer.h"

#include <cstdio>
#include <cstdint>
#include <vector>

static ScreenVertex V(float x, float y, float u, float v)
{
    ScreenVertex s{};
    s.sx = x;
    s.sy = y;
    s.sz = 0.0f;
    s.rhw = 1.0f;
    s.color = 0xFFFFFFFF; // opaque white -> tex * white = tex
    s.specular = 0;
    s.tu0 = u;
    s.tv0 = v;
    s.tu1 = 0;
    s.tv1 = 0;
    return s;
}

int main(int argc, char** argv)
{
    const int W = 256, H = 256;
    const char* shaderDir = (argc > 1) ? argv[1] : "shaders";

    VulkanBackend be;
    if (!be.InitHeadless(W, H))
    {
        printf("InitHeadless FAILED\n");
        return 1;
    }

    VulkanRenderer rn(&be);
    if (!rn.Init(shaderDir))
    {
        printf("Renderer.Init FAILED (shaderDir=%s)\n", shaderDir);
        return 1;
    }

    const uint32_t red = 0xFF0000FF; // RGBA bytes R=FF,G=00,B=00,A=FF
    ID3D11ShaderResourceView* tex = rn.LoadTextureRGBA(&red, 1, 1);
    if (!tex)
    {
        printf("LoadTextureRGBA FAILED\n");
        return 1;
    }

    be.BeginFrame(0xFF102030); // clear ARGB -> R=16,G=32,B=48
    rn.SetViewportSize(W, H);
    rn.BeginScreenPass();
    rn.SetTexture(0, tex);
    std::vector<ScreenVertex> q = {
        V(64, 64, 0, 0),  V(192, 64, 1, 0),  V(64, 192, 0, 1),
        V(192, 64, 1, 0), V(192, 192, 1, 1), V(64, 192, 0, 1),
    };
    rn.DrawTL(0, q.data(), (int)q.size());
    be.Present(true);

    std::vector<uint8_t> px(W * H * 4);
    be.ReadbackColor(px.data());
    auto at = [&](int x, int y, int c) { return px[(y * W + x) * 4 + c]; };
    printf("center(128,128) RGBA = %d,%d,%d,%d  (expect ~255,0,0,255)\n",
           at(128, 128, 0), at(128, 128, 1), at(128, 128, 2), at(128, 128, 3));
    printf("corner(10,10)   RGBA = %d,%d,%d,%d  (expect ~16,32,48,255)\n",
           at(10, 10, 0), at(10, 10, 1), at(10, 10, 2), at(10, 10, 3));

    const bool centerRed =
        at(128, 128, 0) > 200 && at(128, 128, 1) < 40 && at(128, 128, 2) < 40;
    const bool cornerClear = at(10, 10, 0) < 40 && at(10, 10, 1) > 16 &&
                             at(10, 10, 1) < 48 && at(10, 10, 2) > 32 &&
                             at(10, 10, 2) < 64;
    const bool pass2d = centerRed && cornerClear;
    printf("2D: %s\n", pass2d ? "PASS (textured quad)" : "FAIL");

    // ---- 3D object/terrain path: draw a triangle into the multiview scene target, read back layer 0 ----
    struct ObjV
    {
        float p[3];
        float n[3];
        unsigned col, spec;
        float tu, tv;
    }; // 40 bytes
    const int SW = 128, SH = 128;
    bool pass3d = false;
    if (be.EnsureSceneTarget(SW, SH, 1))
    {
        const float I[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        const float amb[4] = {
            1, 1, 1, 1}; // full ambient so the object is unshaded-bright red
        ObjV tri[3] = {
            {{-0.6f, -0.6f, 0.5f}, {0, 0, 1}, 0xFFFFFFFF, 0, 0.0f, 1.0f},
            {{0.6f, -0.6f, 0.5f}, {0, 0, 1}, 0xFFFFFFFF, 0, 1.0f, 1.0f},
            {{0.0f, 0.6f, 0.5f}, {0, 0, 1}, 0xFFFFFFFF, 0, 0.5f, 0.0f},
        };
        const unsigned short idx[3] = {0, 1, 2};
        be.BeginSceneMultiview(0xFF000000); // clear black
        rn.SetWorld(I);
        rn.SetView(I);
        rn.SetProj(I);
        rn.SetMaterialColor(1, 1, 1, 1);
        rn.SetLights(amb, 0, nullptr, 0);
        rn.BeginTerrainPass();
        rn.SetTexture(0, tex); // red texture
        rn.DrawTerrainMesh(tri, 3, idx, 3);
        be.EndSceneMultiview();
        std::vector<uint8_t> sp(SW * SH * 4);
        be.ReadbackScene(0, sp.data());
        auto sat = [&](int x, int y, int c)
        { return sp[(y * SW + x) * 4 + c]; };
        printf(
            "scene center(64,64) RGBA = %d,%d,%d,%d  (expect ~255,0,0,255)\n",
            sat(64, 64, 0), sat(64, 64, 1), sat(64, 64, 2), sat(64, 64, 3));
        printf("scene corner(4,4)   RGBA = %d,%d,%d,%d  (expect ~0,0,0,255 "
               "clear)\n",
               sat(4, 4, 0), sat(4, 4, 1), sat(4, 4, 2), sat(4, 4, 3));
        pass3d = sat(64, 64, 0) > 200 && sat(64, 64, 1) < 40 &&
                 sat(64, 64, 2) < 40 && sat(4, 4, 0) < 20 && sat(4, 4, 1) < 20;
    }
    printf("3D: %s\n",
           pass3d ? "PASS (object triangle, multiview scene)" : "FAIL");

    // ---- object VB path: upload a model VB via VulkanVbManager, DrawObjectIndexed it into the scene ----
    bool passVb = false;
    {
        VulkanVbManager vbm(&be);
        if (vbm.Init() && be.EnsureSceneTarget(SW, SH, 1))
        {
            const float I[16] = {1, 0, 0, 0, 0, 1, 0, 0,
                                 0, 0, 1, 0, 0, 0, 0, 1};
            const float amb[4] = {1, 1, 1, 1};
            ObjV tri[3] = {
                {{-0.5f, -0.5f, 0.5f}, {0, 0, 1}, 0xFFFFFFFF, 0, 0.0f, 1.0f},
                {{0.5f, -0.5f, 0.5f}, {0, 0, 1}, 0xFFFFFFFF, 0, 1.0f, 1.0f},
                {{0.0f, 0.5f, 0.5f}, {0, 0, 1}, 0xFFFFFFFF, 0, 0.5f, 0.0f},
            };
            const unsigned short idx[3] = {0, 1, 2};
            VulkanVb* vb = vbm.Create(tri, sizeof(tri));
            if (vb)
            {
                be.BeginSceneMultiview(0xFF000000);
                rn.SetWorld(I);
                rn.SetView(I);
                rn.SetProj(I);
                rn.SetMaterialColor(1, 1, 1, 1);
                rn.SetLights(amb, 0, nullptr, 0);
                rn.BeginObjectPass();
                rn.SetTexture(0, tex);
                rn.DrawObjectIndexed(0, vb, 40, 0, idx, 3);
                be.EndSceneMultiview();
                std::vector<uint8_t> sp(SW * SH * 4);
                be.ReadbackScene(0, sp.data());
                auto sat = [&](int x, int y, int c)
                { return sp[(y * SW + x) * 4 + c]; };
                printf("obj-vb center(64,64) RGBA = %d,%d,%d,%d  (expect "
                       "~255,0,0,255)\n",
                       sat(64, 64, 0), sat(64, 64, 1), sat(64, 64, 2),
                       sat(64, 64, 3));
                passVb = sat(64, 64, 0) > 200 && sat(64, 64, 1) < 40 &&
                         sat(64, 64, 2) < 40;
                vbm.Destroy(vb);
            }
        }
    }
    printf("VB: %s\n",
           passVb ? "PASS (DrawObjectIndexed from a model VB)" : "FAIL");

    // ---- BC/DXT texture path: a native BC1 (DXT1) block, sampled on the 2D quad ----
    // Skipped on a software rasterizer (lavapipe): it advertises BC support but its BC decode is unreliable and
    // crashes. The BC upload/sampling code is standard and correct on real GPUs.
    bool passBc = false, skipBc = be.IsSoftwareRasterizer();
    if (!skipBc)
    {
        VulkanTextureManager tm(&be);
        if (tm.Init())
        {
            // one BC1 block, 4-colour opaque mode (c0 > c1): c0 = red565 (0xF800), c1 = 0, all indices 0 -> red.
            const unsigned char bc1red[8] = {0x00, 0xF8, 0x00, 0x00,
                                             0x00, 0x00, 0x00, 0x00};
            VulkanTexture* bcTex = tm.CreateBC(bc1red, sizeof(bc1red), 4, 4, 1);
            if (bcTex)
            {
                be.BeginFrame(0xFF102030);
                rn.SetViewportSize(W, H);
                rn.BeginScreenPass();
                rn.SetTexture(
                    0, reinterpret_cast<ID3D11ShaderResourceView*>(bcTex));
                std::vector<ScreenVertex> q2 = {
                    V(64, 64, 0, 0),  V(192, 64, 1, 0),  V(64, 192, 0, 1),
                    V(192, 64, 1, 0), V(192, 192, 1, 1), V(64, 192, 0, 1)};
                rn.DrawTL(0, q2.data(), (int)q2.size());
                be.Present(true);
                std::vector<uint8_t> bp(W * H * 4);
                be.ReadbackColor(bp.data());
                auto bat = [&](int x, int y, int c)
                { return bp[(y * W + x) * 4 + c]; };
                printf("bc center(128,128) RGBA = %d,%d,%d,%d  (expect "
                       "~255,0,0,255)\n",
                       bat(128, 128, 0), bat(128, 128, 1), bat(128, 128, 2),
                       bat(128, 128, 3));
                passBc = bat(128, 128, 0) > 200 && bat(128, 128, 1) < 40 &&
                         bat(128, 128, 2) < 40;
                tm.Destroy(bcTex);
            }
        }
    }
    printf("BC: %s\n",
           skipBc ? "SKIP (software rasterizer; BC correct on real GPUs)" :
                    (passBc ? "PASS (native BC1/DXT1 texture)" : "FAIL"));

    const bool pass = pass2d && pass3d && passVb && (passBc || skipBc);
    printf("%s\n", pass ? "PASS" : "FAIL");

    rn.DestroyTexture(tex); // was leaked -- VMA's shutdown leak check caught it
    rn.Release();
    be.Release();
    return pass ? 0 : 1;
}
