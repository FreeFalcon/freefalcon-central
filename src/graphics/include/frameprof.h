#ifndef FRAMEPROF_H
#define FRAMEPROF_H
// #107 PERF: backend-neutral VR frame profiler. Accumulate per-phase CPU time (mainly fence WAITS + the big DrawScene/
// terrain phases) and dump an average every ~120 frames. Works on BOTH the Vulkan and D3D12 VR paths so their frame
// cost can be compared directly (same shared DrawScene/TerrainGpu code). Implemented in VulkanBackend.cpp (owns the
// accumulator); the callers are the shared renderer (otw.cpp / TerrainGpu.cpp) + the VR drivers (otwloop.cpp).
// Enabled by g_bVulkanProfile (FFViper.cfg "set g_bVulkanProfile 1"); zero cost when off.

void FrameProf_FrameStart(); // top of the VR frame (RenderVulkanVR / the D3D12 VR frame)
void FrameProf_EndFrame(); // end of the VR frame -- dumps the [VKPROF] line every ~120 frames
void FrameProf_DrawScene(double ms); // whole renderer->DrawScene (world record)
void FrameProf_BlitGroup(
    double ms); // per-group blit (xrWaitSwapchainImage) -- Vulkan path only
void FrameProf_TerrainGpu(
    double ms); // TerrainGpu_Render whole (accumulate + flush)
void FrameProf_TerrainSpan(
    double
        ms); // CPU span/vertex build (BuildVertexSet/TransformVertexSet, runs regardless)
void FrameProf_Objects(double ms); // object ring-list draw + DrawBeyond
void FrameProf_TerrAcc(double ms); // terrain bucket ACCUMULATE (CPU grid build)
void FrameProf_TerrFlush(
    double ms); // terrain bucket FLUSH (DrawTerrainMesh submission)
void FrameProf_CountDraw(); // one object draw recorded
void FrameProf_CountTerrainDraw(); // one terrain bucket draw
// #107 PERF stage 2: decompose the ~7-10ms of CPU_FRAME the categories above do not cover (GPU is idle at 0.4ms, so
// the whole Vulkan-vs-D3D12 gap is CPU record time -- find WHERE).
void FrameProf_RenderVR(
    double
        ms); // the whole RenderVulkanVR body (CPU_FRAME minus this = the rest of RenderFrame)
void FrameProf_TailEye(
    double
        ms); // one per-eye tail body (BeginTailView..EndTailView incl. VCock_Exec), accumulated
void FrameProf_VCock(
    double
        ms); // one VCock_Exec (RTT displays + HUD + 2D + controller model), accumulated
void FrameProf_PreVR(
    double
        ms); // RenderFrame from FrameStart to the RenderVulkanVR call (camera/view/misc logic)
void FrameProf_PostVR(
    double ms); // RenderFrame after RenderVulkanVR to EndFrame (present/misc)
void FrameProf_XrWaitFrame(
    double
        ms); // xrWaitFrame (the runtime's frame-pacing BLOCK, inside PreVR) -- pacing, not work

#endif // FRAMEPROF_H
