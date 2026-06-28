//-----------------------------------------------------------------------------
// OpenXRBackend.h
//
// VR bring-up for the D3D11 render path (see openxr-feasibility memory). Owns the
// OpenXR instance / system / session bound to the existing D3D11 device created by
// D3D11Backend, the per-eye swap chains, and the xrWaitFrame/xrBeginFrame/
// xrEndFrame loop. Selected at runtime by g_bUseOpenXR; when off this object is
// never created and the flat (mono, desktop-window) path is completely untouched.
//
// Milestone 1: session comes up, both eyes are cleared each frame and the layer is
// submitted (validates the whole OpenXR plumbing in the headset). Milestone 2
// renders the real 3D scene per eye via the XrEyeRenderFn callback. Per-eye
// view/projection are passed to that callback -- they are NOT global, which is the
// core VR invariant the renderer was designed around.
//
// PIMPL: the OpenXR + d3d11 types are kept entirely in the .cpp so this header can
// be included by the sim frame loop (otwloop.cpp) and DevMgr.cpp without pulling
// openxr_platform.h / d3d11.h into those large legacy translation units.
//-----------------------------------------------------------------------------
#ifndef _OPENXRBACKEND_H_
#define _OPENXRBACKEND_H_

struct ID3D11Device;

// Render callback for one eye. Invoked between xrAcquire/ReleaseSwapchainImage.
//   user       - opaque pointer passed through RunFrame
//   eye        - 0 = left, 1 = right
//   rtv        - ID3D11RenderTargetView* of the eye's swap-chain image (void* to
//                keep d3d11.h out of this header)
//   width/height - eye image size in pixels
//   viewMatrix / projMatrix - 4x4 row-major (D3DXMATRIX layout) for THIS eye
// The callback owns clearing + drawing into rtv.
typedef void (*XrEyeRenderFn)(void* user, int eye, void* rtv, int width, int height,
                              const float* viewMatrix, const float* projMatrix);

class OpenXRBackend
{
public:
	OpenXRBackend();
	~OpenXRBackend();

	// Create instance/system/session bound to 'device' + reference spaces +
	// per-eye swap chains. Returns false (logs via MonoPrint) on any failure so
	// the caller keeps the flat path. Safe to call once.
	bool Init(ID3D11Device* device);
	void Shutdown();

	bool IsInitialized() const;       // instance + session created
	bool IsSessionRunning() const;    // runtime says frames should be submitted
	// Artscout - 2026 (#58/#60): the ACTUAL view config the live session was created with (QUAD_VARJO=4 views
	// vs STEREO=2). Render/mouse must branch off THIS, not the g_bUseQuadViews option -- the session is created
	// once and not recreated when the option is toggled in-game, so the option can disagree with reality until
	// the next app restart. Branching off reality keeps the picture consistent instead of garbling the cockpit.
	bool IsQuadViews() const;         // session is 4-view foveated quad (not 2-view stereo)

	// Drive one XR frame: poll events, wait/begin frame, locate the per-eye views,
	// then for each eye acquire the swap-chain image, invoke 'render' (or clear if
	// NULL), release it, and submit the projection layer in xrEndFrame. Returns
	// false (no-op) when the session is not running yet.
	bool RunFrame(XrEyeRenderFn render, void* user);

	// Menu / pre-3D mode: present the flat 2D UI (the engine's 565 composite) as a
	// head-locked XrCompositionLayerQuad panel in the headset, instead of the eye
	// projection. Drives the same xrWaitFrame/Begin/End loop. src565 = RGB565 CPU
	// bitmap (srcW x srcH). Returns false (no-op) when the session is not running.
	bool RunMenuFrame(const void* src565, int srcW, int srcH);

	// ---- True per-eye stereo (the 3D scene is rendered once per eye) ----------------
	// BeginStereoFrame: poll events, xrWaitFrame/BeginFrame, locate the per-eye views +
	// head pose. Returns the number of eyes to render (0 = don't render this frame, but
	// EndStereoFrame must still be called). Drive from the sim render thread.
	int  BeginStereoFrame();
	// Acquire eye's swap-chain image; returns its RTV (ID3D11RenderTargetView*) + size.
	bool BeginEye(int eye, void** outRtv, int* outW, int* outH);
	// Records eye's projection view (no swapchain release -- see ReleaseEyes).
	void EndEye(int eye);
	// Artscout - 2026: release ALL eye images acquired this frame; call after both eyes render,
	// before EndStereoFrame (deferred release fixes the 2nd-eye-black on Pimax/PiOpenXR).
	void ReleaseEyes();
	void DiagClearEyesAndEnd();   // Artscout - 2026: TEMP diag -- M1-style clear of both eyes, no engine render
	bool GetEyeFovAngles(int eye, float* l, float* r, float* u, float* d) const;  // per-eye fov half-angles (rad)
	bool GetHeadPosFeet(float* fwd, float* right, float* down) const;  // HMD position -> Falcon body, feet (6DOF)
	bool GetHeadBasis(float at[3], float right[3], float up[3]) const; // HMD orientation -> Falcon body basis (gimbal-free)
	bool GetEyeBasis(int eye, float at[3], float right[3], float up[3]) const; // per-VIEW orientation (gaze-tracked focus views)
	// Artscout - 2026 (#59 VR menu): submit the in-3D comms/exit menu as a head-locked quad layer composited
	// ON TOP of the projection layer(s). menuTex = a backend RGBA8 texture with the menu drawn (transparent
	// elsewhere). Copies it into the UI swapchain and stages a quad in VIEW space (always in front of the
	// head, independent of quad-views/gaze). The quad is added by EndStereoFrame. Returns false if not staged.
	bool SubmitInSceneMenuQuad(void* menuTex, int w, int h);

	// Submit the projection layer (all eyes) -- balances BeginStereoFrame's xrBeginFrame.
	void EndStereoFrame();
	bool StereoActive() const;                 // true between Begin/EndStereoFrame
	void SetCurrentEye(int eye);               // which eye the camera setup is for (-1=none)
	int  CurrentEye() const;
	float GetEyeLateralOffsetFeet(int eye) const;  // signed lateral eye offset (IPD) in feet
	// The fov the engine actually rendered with (symmetric). Submitted in the projection
	// layer so the compositor maps the image undistorted (must match what was rendered).
	void SetSubmitFov(float hFovRad, float vFovRad);

	void SetClipPlanes(float nearZ, float farZ);

	// Most recent located head pose in the app (LOCAL) space, for feeding the
	// cockpit camera (milestone 3). Returns false until a pose is available.
	bool GetHeadPose(float outPosXYZ[3], float outQuatXYZW[4]) const;

	// Head orientation as yaw/pitch/roll (radians, Falcon look convention) derived from
	// the latest located HMD pose -- fed into the cockpit camera so head motion looks
	// around. Returns false until a pose is available.
	bool GetHeadYawPitchRoll(float* yaw, float* pitch, float* roll) const;

	// Artscout - 2026 (#67): recenter the headset -- rebuild the app (LOCAL) reference space at the current
	// head yaw + position, so "forward"/"center" become where you are looking now and the eye height resets.
	// Pitch/roll are NOT inherited (the horizon stays level). PUBLIC -- called by the SimRecenterVR key command;
	// only sets a flag, the actual space rebuild happens on the render thread in BeginStereoFrame.
	bool Recenter();

private:
	void PollEvents();                       // drive the session state machine
	bool EnsureUiSwapchain(int w, int h);    // (re)create the menu quad swapchain

	struct Impl;
	Impl* m_impl;
};

// Single global instance, created in DXContext::Init after the D3D11 device is up
// when g_bUseOpenXR is set. NULL when VR is off.
extern OpenXRBackend* g_pOpenXRBackend;

// Runtime selector for the VR path. Default OFF -- the flat desktop path is the
// normal mode and must always remain available.
extern bool g_bUseOpenXR;

// Last flat 2D-UI surface (RGB565) seen by the screen present, cached for the menu
// VR panel. Set by ImageBuffer::SwapBuffers (menu frames); read by OpenXR_PumpFrame.
extern const void* g_pXrMenuSurface565;
extern int g_xrMenuW;
extern int g_xrMenuH;

// Artscout - 2026 (VR menu): the producer (ImageBuffer::PresentD3D11, where m_pSysMem is valid) calls this
// to COPY the 565 surface into a lock-protected stable buffer that the pump reads -- avoids the cross-thread
// use-after-free on m_pSysMem. Replaces directly assigning g_pXrMenuSurface565 = m_pSysMem.
void OpenXR_CacheMenuSurface(const void* src565, int w, int h);

// Drive exactly ONE OpenXR frame from the main message loop (single-threaded owner
// of the XR frame loop). In the 3D world it submits the eye layer (milestone 1:
// clear); in menus it submits the cached UI as a quad panel. No-op when VR is off
// or the session is not running. This is what gives the headset continuous frames
// even though the 2D UI only repaints on change. Returns true if a frame was
// actually submitted (session running); false otherwise (caller should yield).
bool OpenXR_PumpFrame();

#endif // _OPENXRBACKEND_H_
