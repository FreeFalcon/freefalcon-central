//-----------------------------------------------------------------------------
// OpenXRBackend.cpp -- see header. VR bring-up over the D3D11 device.
//
// Milestone 1: full OpenXR plumbing (instance/system/session/swapchains + the
// xrWaitFrame/Begin/End loop). RunFrame(NULL) clears both eyes and submits the
// projection layer -> validates the headset path end-to-end. Per-eye view/proj
// are computed and handed to the render callback for milestone 2.
//-----------------------------------------------------------------------------
#include <windows.h>
#include <d3d11.h>
#include <d3d11_4.h>   // ID3D11Multithread (context multithread protection)
#include <vector>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>

// Artscout - 2026: TEMP diag -> OutputDebugString so it lands in the VS output log
// (MonoPrint goes to the mono card, not the debugger log). Remove with the diags.
static void XrDbg(const char* fmt, ...)
{
	char buf[256];
	va_list a; va_start(a, fmt);
	_vsnprintf(buf, sizeof(buf) - 1, fmt, a);
	va_end(a);
	buf[sizeof(buf) - 1] = 0;
	OutputDebugStringA(buf);
	// Artscout - 2026: also append to a file in the game cwd so OpenXR init/hand-tracking diagnostics are
	// visible WITHOUT a debugger (OutputDebugString needs DebugView). Same pattern as the vrray_diag dumps.
	FILE* f = fopen("openxr_diag.txt", "a");
	if (f) { fputs(buf, f); fclose(f); }
}

// OpenXR with the D3D11 + D3D12 graphics bindings (must be defined before the platform header).
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#define XR_USE_GRAPHICS_API_D3D12   // #DX12 п.5: dual-path VR (session bound to D3D11 or D3D12 by g_bUseD3D12)
#include <d3d12.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include "OpenXRBackend.h"
#include "D3D12Backend.h"                  // #DX12 п.5: g_pD3D12Backend (device + queue for the XR binding)
#include "d3d12/D3D12TextureManager.h"     // #DX12 п.5 A1: D3D12Texture (in-scene menu RTT copy source)
#include "../../sim/INCLUDE/ivibedata.h"   // g_intellivibeData.In3D (menu vs 3D world)


OpenXRBackend* g_pOpenXRBackend = NULL;

// Menu UI surface cache (set by ImageBuffer::SwapBuffers, read by OpenXR_PumpFrame).
const void* g_pXrMenuSurface565 = NULL;
int g_xrMenuW = 0;
int g_xrMenuH = 0;

// Artscout - 2026 (VR menu): eliminate the cross-thread race on the menu 565 surface. The OLD design cached
// g_pXrMenuSurface565 = an ImageBuffer's m_pSysMem and let the pump (other thread) read it -> the buffer
// could be freed/resized mid-read (0xC0000005). Now the PRODUCER (PresentGpu, where m_pSysMem is valid)
// COPIES the surface into a stable buffer under a lock, and the pump reads a snapshot of THAT. No race.
static CRITICAL_SECTION s_xrMenuCS;
static bool             s_xrMenuCSReady   = false;
static unsigned char*   s_xrMenuStable    = NULL;   // producer-filled, lock-protected
static long             s_xrMenuStableCap = 0;
static unsigned char*   s_xrMenuSnap      = NULL;   // pump-local snapshot (the convert reads this, no lock)
static long             s_xrMenuSnapCap   = 0;

static void XrMenuLockInit() { if (!s_xrMenuCSReady) { InitializeCriticalSection(&s_xrMenuCS); s_xrMenuCSReady = true; } }

// Producer (PresentGpu thread): copy the (valid-here) 565 surface into the stable buffer under the lock.
void OpenXR_CacheMenuSurface(const void* src565, int w, int h)
{
	if (!src565 || w <= 0 || h <= 0) return;
	XrMenuLockInit();
	const long need = (long)w * h * 2;
	EnterCriticalSection(&s_xrMenuCS);
	if (need > s_xrMenuStableCap)
	{
		unsigned char* n = (unsigned char*)realloc(s_xrMenuStable, need);
		if (n) { s_xrMenuStable = n; s_xrMenuStableCap = need; }
	}
	if (s_xrMenuStable && need <= s_xrMenuStableCap)
	{
		memcpy(s_xrMenuStable, src565, need);   // src == this thread's own m_pSysMem -> safe to read here
		g_xrMenuW = w; g_xrMenuH = h;
		g_pXrMenuSurface565 = s_xrMenuStable;   // non-null -> the pump has a menu surface to present
	}
	LeaveCriticalSection(&s_xrMenuCS);
}

// Consumer (pump): snapshot the stable buffer into a pump-local buffer (fast memcpy under the lock). The
// returned buffer is safe to read WITHOUT the lock (the producer never touches the snapshot buffer).
static const unsigned char* XrMenuSnapshot(int w, int h)
{
	if (!s_xrMenuCSReady || w <= 0 || h <= 0) return NULL;
	const long need = (long)w * h * 2;
	const unsigned char* out = NULL;
	EnterCriticalSection(&s_xrMenuCS);
	if (s_xrMenuStable && need > 0 && need <= s_xrMenuStableCap)
	{
		if (need > s_xrMenuSnapCap)
		{
			unsigned char* n = (unsigned char*)realloc(s_xrMenuSnap, need);
			if (n) { s_xrMenuSnap = n; s_xrMenuSnapCap = need; }
		}
		if (s_xrMenuSnap && need <= s_xrMenuSnapCap) { memcpy(s_xrMenuSnap, s_xrMenuStable, need); out = s_xrMenuSnap; }
	}
	LeaveCriticalSection(&s_xrMenuCS);
	return out;
}

//-----------------------------------------------------------------------------
// Error-check helper: log + bail. XR_FAILED is the canonical failure test.
//-----------------------------------------------------------------------------
#define XR_BAIL(call, what)                                                   \
	do {                                                                      \
		XrResult _r = (call);                                                 \
		if (XR_FAILED(_r)) {                                                  \
			XrDbg("OpenXR: %s failed (XrResult %d)\n", what, (int)_r);    \
			return false;                                                     \
		}                                                                     \
	} while (0)

//-----------------------------------------------------------------------------
// Private state (PIMPL): all OpenXR + D3D11 types live here, out of the header.
//-----------------------------------------------------------------------------
struct OpenXRBackend::Impl
{
	XrInstance   instance;
	XrSystemId   systemId;
	XrSession    session;
	XrSpace      appSpace;     // LOCAL reference space (app world origin, recentered)
	XrSpace      localRef;     // Artscout - 2026 (#67): PRISTINE LOCAL space, never recentered -- the fixed
	                           // measurement reference for recenter (so each recenter is absolute, not relative
	                           // to the already-shifted appSpace, which made the 2nd press toggle back).
	XrSpace      viewSpace;    // VIEW reference space (head), for the camera feed

	XrSessionState sessionState;
	bool           sessionRunning;

	XrViewConfigurationType viewConfigType;
	std::vector<XrViewConfigurationView> configViews;   // per-eye recommended size
	std::vector<XrView>                  views;         // per-eye located pose+fov

	struct Swapchain
	{
		XrSwapchain handle;
		int32_t     width;
		int32_t     height;
		std::vector<XrSwapchainImageD3D11KHR> images;
		std::vector<ID3D11RenderTargetView*>  rtvs;
		// #DX12 п.5: parallel D3D12 swapchain images + their RTV CPU handles (in Impl::rtvHeap12).
		std::vector<XrSwapchainImageD3D12KHR> images12;
		std::vector<unsigned __int64>         rtvs12;
	};
	std::vector<Swapchain> swapchains;   // one per eye

	int64_t      swapchainFormat;        // DXGI_FORMAT chosen for the eye color images

	// #DX12 п.5: D3D12 VR path (session bound to the D3D12 device+queue instead of D3D11). Selected by g_bUseD3D12.
	bool                       useD3D12;
	ID3D12Device*              d3d12Device;
	ID3D12CommandQueue*        d3d12Queue;
	ID3D12DescriptorHeap*      rtvHeap12;     // RTVs for all eye + UI swapchain images
	unsigned                   rtvInc12;
	unsigned                   rtvHead12;
	ID3D12CommandAllocator*    alloc12;       // for the eye clear / composite work
	ID3D12GraphicsCommandList* list12;
	ID3D12Fence*               fence12;
	unsigned __int64           fenceVal12;
	HANDLE                     fenceEvt12;

	// Menu mode: a single quad-layer swapchain showing the flat 2D UI, plus a
	// CPU-writable staging texture for the 565->RGBA upload.
	XrSwapchain  uiSwapchain;
	int          uiW, uiH;
	int64_t      uiFormat;               // R8G8B8A8 (UNORM or _SRGB)
	std::vector<XrSwapchainImageD3D11KHR> uiImages;
	ID3D11Texture2D* uiStaging;
	// #DX12 п.5: D3D12 UI swapchain images + an UPLOAD buffer for the 565->RGBA copy into them.
	std::vector<XrSwapchainImageD3D12KHR> uiImages12;
	ID3D12Resource*  uiUpload12;
	unsigned         uiRowPitch12;
	void*            menuTexD3D12;    // #DX12 п.5 A1: D3D12Texture* staged by SubmitInSceneMenuQuad, copied in EndStereoFrame

	ID3D11Device*        device;
	ID3D11DeviceContext* ctx;

	float nearZ, farZ;

	XrPosef lastHeadPose;
	bool    haveHeadPose;

	float   lastYaw, lastPitch, lastRoll;   // HMD head angles (rad) for the cockpit camera
	bool    haveHeadAngles;

	// Per-eye stereo frame state.
	bool         inStereoFrame;
	XrFrameState stereoFrameState;
	int          currentEye;
	std::vector<uint32_t> eyeImgIndex;                       // acquired swapchain image per eye
	std::vector<bool>     eyeAcquired;                       // eye image acquired this frame (deferred release)
	std::vector<XrCompositionLayerProjectionView> projViews; // per-eye projection-layer views
	float        eyeLatFeet[8];                              // signed lateral offset per eye (feet)
	XrFovf       submitFov;                                  // engine fov submitted in the layer
	bool         haveSubmitFov;

	// Artscout - 2026 (#59 VR menu): head-locked menu quad staged for THIS stereo frame (added by EndStereoFrame).
	bool                  menuQuadPending;
	XrCompositionLayerQuad menuQuad;

	// Artscout - 2026 (#67): recenter requested (any thread); applied by the render thread in BeginStereoFrame.
	bool                  recenterPending;

	// Artscout - 2026 (VR controllers, Phase 1): action-based input. Actions are declared once and bound per
	// interaction profile (touch/index/wmr/simple) -- the runtime maps them to whatever controller is present,
	// so we never enumerate per-vendor buttons. aim/grip are TRACKING POSES (not the squeeze button).
	XrActionSet  actionSet;
	// aim/grip = tracking POSES (not buttons). squeeze = the grip BUTTON, used ONLY to pick the active hand
	// (whoever squeezes owns the ray); no grab mechanic. trigger = click, thumb = switches/knobs, a/b = zoom/recenter.
	XrAction     aimAction, gripAction, squeezeAction, triggerAction, thumbAction, aBtnAction, bBtnAction;
	XrPath       handPath[2];              // 0 = /user/hand/left, 1 = /user/hand/right
	XrSpace      aimSpace[2], gripSpace[2];
	bool         inputReady;
	int          activeHand;               // 0=left, 1=right; default right, switched by whoever squeezes grip
	struct HandInput
	{
		bool    aimValid;  XrPosef aimPose;   // laser origin/direction (angled like a pointer)
		bool    gripValid; XrPosef gripPose;  // where the hand holds it (for the controller model, Phase 4)
		float   trigger;   bool    triggerDown;
		float   squeeze;   bool    squeezeDown;   // grip button -> active-hand switch (rising edge)
		float   thumbX, thumbY;
		bool    aBtn, bBtn;
	} hand[2];

	// Artscout - 2026 (VR hands): XR_EXT_hand_tracking. Index/knuckles synthesise a hand skeleton from the
	// controller's capacitive finger sensors (no camera module needed) -- the runtime returns 26 joint poses
	// per hand. Located every frame; if the runtime reports them not-active we fall back to the wireframe
	// controller. Extension entry points are resolved via xrGetInstanceProcAddr after the instance is created.
	bool                        handTrackingEnabled;      // ext enabled on the instance
	PFN_xrCreateHandTrackerEXT  pfnCreateHandTracker;
	PFN_xrDestroyHandTrackerEXT pfnDestroyHandTracker;
	PFN_xrLocateHandJointsEXT   pfnLocateHandJoints;
	XrHandTrackerEXT            handTracker[2];
	bool                        handJointsValid[2];
	XrHandJointLocationEXT      handJoints[2][XR_HAND_JOINT_COUNT_EXT];

	Impl()
		: instance(XR_NULL_HANDLE), systemId(XR_NULL_SYSTEM_ID), session(XR_NULL_HANDLE),
		  appSpace(XR_NULL_HANDLE), localRef(XR_NULL_HANDLE), viewSpace(XR_NULL_HANDLE),
		  sessionState(XR_SESSION_STATE_UNKNOWN), sessionRunning(false),
		  viewConfigType(XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO),
		  swapchainFormat(0),
		  uiSwapchain(XR_NULL_HANDLE), uiW(0), uiH(0), uiFormat(0), uiStaging(NULL),
		  device(NULL), ctx(NULL),
		  nearZ(1.0f), farZ(80000.0f), haveHeadPose(false),
		  lastYaw(0.0f), lastPitch(0.0f), lastRoll(0.0f), haveHeadAngles(false),
		  inStereoFrame(false), currentEye(-1), haveSubmitFov(false),
		  menuQuadPending(false), recenterPending(false),
		  actionSet(XR_NULL_HANDLE), aimAction(XR_NULL_HANDLE), gripAction(XR_NULL_HANDLE),
		  squeezeAction(XR_NULL_HANDLE), triggerAction(XR_NULL_HANDLE), thumbAction(XR_NULL_HANDLE),
		  aBtnAction(XR_NULL_HANDLE), bBtnAction(XR_NULL_HANDLE), inputReady(false), activeHand(1),
		  handTrackingEnabled(false), pfnCreateHandTracker(NULL), pfnDestroyHandTracker(NULL),
		  pfnLocateHandJoints(NULL),
		  // #DX12 п.5: D3D12 members must be zero-initialized (raw pointers/handles) -- otherwise
		  // EnsureUiSwapchain's `if(p->uiUpload12) Release()` derefs garbage (0xFFFF... read AV).
		  useD3D12(false), d3d12Device(NULL), d3d12Queue(NULL), rtvHeap12(NULL), rtvInc12(0), rtvHead12(0),
		  alloc12(NULL), list12(NULL), fence12(NULL), fenceVal12(0), fenceEvt12(NULL),
		  uiUpload12(NULL), uiRowPitch12(0), menuTexD3D12(NULL)
	{
		lastHeadPose.orientation.x = lastHeadPose.orientation.y = lastHeadPose.orientation.z = 0.0f;
		lastHeadPose.orientation.w = 1.0f;
		lastHeadPose.position.x = lastHeadPose.position.y = lastHeadPose.position.z = 0.0f;
		handPath[0] = handPath[1] = XR_NULL_PATH;
		aimSpace[0] = aimSpace[1] = gripSpace[0] = gripSpace[1] = XR_NULL_HANDLE;
		handTracker[0] = handTracker[1] = XR_NULL_HANDLE;
		handJointsValid[0] = handJointsValid[1] = false;
		memset(hand, 0, sizeof(hand));
	}
};

//=============================================================================
// Matrix helpers. Output is 4x4 ROW-MAJOR in D3DXMATRIX layout (row-vector
// convention, v' = v * M), matching the engine's matrices.
//
// NOTE (milestone 2): OpenXR is right-handed (+Y up, -Z forward); the engine is
// left-handed (it flips RH->LH via Flip.m02 in render3d.cpp). The projection
// below is a D3D LH off-center frustum with z in [0,1]; the view is the rigid
// inverse of the eye pose. The RH->LH reconciliation must be verified against the
// live scene when the per-eye render is wired -- for milestone 1 (clear only)
// these are computed but not visually used.
//=============================================================================
static void XrProjectionToD3D(const XrFovf& fov, float zn, float zf, float* m /*[16]*/)
{
	const float tanL = tanf(fov.angleLeft);
	const float tanR = tanf(fov.angleRight);
	const float tanU = tanf(fov.angleUp);
	const float tanD = tanf(fov.angleDown);
	const float w = tanR - tanL;
	const float h = tanU - tanD;

	for (int i = 0; i < 16; ++i) m[i] = 0.0f;
	m[0]  = 2.0f / w;                       // m00
	m[5]  = 2.0f / h;                       // m11
	m[8]  = (tanR + tanL) / w;              // m20 (x shear)
	m[9]  = (tanU + tanD) / h;              // m21 (y shear)
	m[10] = zf / (zf - zn);                 // m22
	m[11] = 1.0f;                           // m23
	m[14] = (zn * zf) / (zn - zf);          // m32
}

// Quaternion (x,y,z,w) -> row-major 3x3 stored into the upper-left of m[16].
static void XrQuatToMat(const XrQuaternionf& q, float* m /*[16]*/)
{
	const float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
	const float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
	const float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;

	m[0]  = 1.0f - 2.0f * (yy + zz); m[1]  = 2.0f * (xy + wz);        m[2]  = 2.0f * (xz - wy);        m[3]  = 0.0f;
	m[4]  = 2.0f * (xy - wz);        m[5]  = 1.0f - 2.0f * (xx + zz); m[6]  = 2.0f * (yz + wx);        m[7]  = 0.0f;
	m[8]  = 2.0f * (xz + wy);        m[9]  = 2.0f * (yz - wx);        m[10] = 1.0f - 2.0f * (xx + yy); m[11] = 0.0f;
	m[12] = 0.0f;                    m[13] = 0.0f;                    m[14] = 0.0f;                    m[15] = 1.0f;
}

// View matrix = inverse of the eye's rigid world transform (pose). Row-major.
static void XrPoseToView(const XrPosef& pose, float* m /*[16]*/)
{
	float R[16];
	XrQuatToMat(pose.orientation, R);

	// Inverse rotation is the transpose; inverse translation is -t * R^T.
	const float tx = pose.position.x, ty = pose.position.y, tz = pose.position.z;

	m[0] = R[0]; m[1] = R[4]; m[2] = R[8];  m[3] = 0.0f;
	m[4] = R[1]; m[5] = R[5]; m[6] = R[9];  m[7] = 0.0f;
	m[8] = R[2]; m[9] = R[6]; m[10] = R[10]; m[11] = 0.0f;
	// -t * R^T  (R^T rows are R columns)
	m[12] = -(tx * R[0] + ty * R[1] + tz * R[2]);
	m[13] = -(tx * R[4] + ty * R[5] + tz * R[6]);
	m[14] = -(tx * R[8] + ty * R[9] + tz * R[10]);
	m[15] = 1.0f;
}

// HMD orientation (OpenXR quaternion, RH: +X right, +Y up, -Z forward) -> Falcon head
// look angles yaw/pitch/roll (radians). Derived from the rotated forward/up vectors to
// avoid Euler-order ambiguity. SIGN_* let us flip an axis without touching the math if a
// direction comes out inverted on the headset.
static void XrQuatToYawPitchRoll(const XrQuaternionf& q, float& yaw, float& pitch, float& roll)
{
	const float x = q.x, y = q.y, z = q.z, w = q.w;
	// forward = R * (0,0,-1)
	const float fx = -2.0f * (x * z + w * y);
	const float fy =  2.0f * (w * x - y * z);
	const float fz =  2.0f * (x * x + y * y) - 1.0f;
	// up = R * (0,1,0)
	const float ux = 2.0f * (x * y - w * z);
	const float uy = 1.0f - 2.0f * (x * x + z * z);

	float p = fy; if (p < -1.0f) p = -1.0f; else if (p > 1.0f) p = 1.0f;

	const float SIGN_YAW = 1.0f, SIGN_PITCH = -1.0f, SIGN_ROLL = -1.0f;  // flip if inverted in HMD
	yaw   = SIGN_YAW   * atan2f(fx, -fz);   // look right (+X) -> +yaw
	pitch = SIGN_PITCH * asinf(p);          // look up (+Y)    -> +pitch
	roll  = SIGN_ROLL  * atan2f(ux, uy);    // head tilt
}

//=============================================================================
// Artscout - 2026 (VR controllers, Phase 1): action-based input.
// Actions are semantic (aim/grip pose, squeeze, trigger, thumbstick, A, B); we suggest bindings for each
// interaction profile and the runtime maps them to the connected controller. No per-vendor enumeration.
//=============================================================================
static XrPath XrStr2Path(XrInstance inst, const char* s)
{
	XrPath path = XR_NULL_PATH;
	xrStringToPath(inst, s, &path);
	return path;
}

// Suggest one profile's bindings from parallel {action, path-string} arrays. Non-fatal per profile
// (a runtime may reject a profile it doesn't know; other profiles still apply).
static void SuggestProfile(XrInstance inst, const char* profile,
                           const XrAction* acts, const char* const* paths, int count)
{
	std::vector<XrActionSuggestedBinding> binds;
	for (int i = 0; i < count; ++i)
	{
		XrPath bp = XrStr2Path(inst, paths[i]);
		if (bp == XR_NULL_PATH || acts[i] == XR_NULL_HANDLE) continue;
		XrActionSuggestedBinding b; b.action = acts[i]; b.binding = bp;
		binds.push_back(b);
	}
	if (binds.empty()) return;
	XrInteractionProfileSuggestedBinding sb = { XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
	sb.interactionProfile = XrStr2Path(inst, profile);
	sb.countSuggestedBindings = (uint32_t)binds.size();
	sb.suggestedBindings = binds.data();
	xrSuggestInteractionProfileBindings(inst, &sb);
}

bool OpenXRBackend::CreateInputActions()
{
	Impl* p = m_impl;
	p->inputReady = false;
	p->handPath[0] = XrStr2Path(p->instance, "/user/hand/left");
	p->handPath[1] = XrStr2Path(p->instance, "/user/hand/right");

	XrActionSetCreateInfo asci = { XR_TYPE_ACTION_SET_CREATE_INFO };
	strcpy(asci.actionSetName, "gameplay");
	strcpy(asci.localizedActionSetName, "Gameplay");
	asci.priority = 0;
	if (XR_FAILED(xrCreateActionSet(p->instance, &asci, &p->actionSet))) return false;

	struct ADef { XrAction* a; const char* name; const char* loc; XrActionType type; };
	ADef defs[] = {
		{ &p->aimAction,     "aim_pose",  "Aim Pose",    XR_ACTION_TYPE_POSE_INPUT     },
		{ &p->gripAction,    "grip_pose", "Grip Pose",   XR_ACTION_TYPE_POSE_INPUT     },
		{ &p->squeezeAction, "squeeze",   "Grip Button", XR_ACTION_TYPE_FLOAT_INPUT    },
		{ &p->triggerAction, "trigger",   "Trigger",     XR_ACTION_TYPE_FLOAT_INPUT    },
		{ &p->thumbAction,   "thumb",     "Thumbstick",  XR_ACTION_TYPE_VECTOR2F_INPUT },
		{ &p->aBtnAction,    "btn_a",     "Button A",    XR_ACTION_TYPE_BOOLEAN_INPUT  },
		{ &p->bBtnAction,    "btn_b",     "Button B",    XR_ACTION_TYPE_BOOLEAN_INPUT  },
	};
	for (int i = 0; i < (int)(sizeof(defs) / sizeof(defs[0])); ++i)
	{
		XrActionCreateInfo aci = { XR_TYPE_ACTION_CREATE_INFO };
		strcpy(aci.actionName, defs[i].name);
		strcpy(aci.localizedActionName, defs[i].loc);
		aci.actionType         = defs[i].type;
		aci.countSubactionPaths = 2;
		aci.subactionPaths      = p->handPath;
		if (XR_FAILED(xrCreateAction(p->actionSet, &aci, defs[i].a))) return false;
	}

	// Full-feature profiles share the same action order (aim,grip,squeeze,trigger,thumb,A,B x L/R).
	const XrAction A14[] = {
		p->aimAction, p->aimAction, p->gripAction, p->gripAction,
		p->squeezeAction, p->squeezeAction, p->triggerAction, p->triggerAction,
		p->thumbAction, p->thumbAction, p->aBtnAction, p->aBtnAction, p->bBtnAction, p->bBtnAction };

	// Oculus Touch (Quest/Rift): A/B on the right hand, X/Y on the left.
	const char* const touch[] = {
		"/user/hand/left/input/aim/pose",       "/user/hand/right/input/aim/pose",
		"/user/hand/left/input/grip/pose",      "/user/hand/right/input/grip/pose",
		"/user/hand/left/input/squeeze/value",  "/user/hand/right/input/squeeze/value",
		"/user/hand/left/input/trigger/value",  "/user/hand/right/input/trigger/value",
		"/user/hand/left/input/thumbstick",     "/user/hand/right/input/thumbstick",
		"/user/hand/left/input/x/click",        "/user/hand/right/input/a/click",
		"/user/hand/left/input/y/click",        "/user/hand/right/input/b/click" };
	SuggestProfile(p->instance, "/interaction_profiles/oculus/touch_controller", A14, touch, 14);

	// Valve Index: a/b on both hands, analog squeeze force.
	const char* const index[] = {
		"/user/hand/left/input/aim/pose",       "/user/hand/right/input/aim/pose",
		"/user/hand/left/input/grip/pose",      "/user/hand/right/input/grip/pose",
		"/user/hand/left/input/squeeze/force",  "/user/hand/right/input/squeeze/force",
		"/user/hand/left/input/trigger/value",  "/user/hand/right/input/trigger/value",
		"/user/hand/left/input/thumbstick",     "/user/hand/right/input/thumbstick",
		"/user/hand/left/input/a/click",        "/user/hand/right/input/a/click",
		"/user/hand/left/input/b/click",        "/user/hand/right/input/b/click" };
	SuggestProfile(p->instance, "/interaction_profiles/valve/index_controller", A14, index, 14);

	// Microsoft WMR motion controller: no A/B face buttons -> A=menu, B=trackpad click; squeeze is a click.
	const char* const wmr[] = {
		"/user/hand/left/input/aim/pose",       "/user/hand/right/input/aim/pose",
		"/user/hand/left/input/grip/pose",      "/user/hand/right/input/grip/pose",
		"/user/hand/left/input/squeeze/click",  "/user/hand/right/input/squeeze/click",
		"/user/hand/left/input/trigger/value",  "/user/hand/right/input/trigger/value",
		"/user/hand/left/input/thumbstick",     "/user/hand/right/input/thumbstick",
		"/user/hand/left/input/menu/click",     "/user/hand/right/input/menu/click",
		"/user/hand/left/input/trackpad/click", "/user/hand/right/input/trackpad/click" };
	SuggestProfile(p->instance, "/interaction_profiles/microsoft/motion_controller", A14, wmr, 14);

	// Khronos simple controller (fallback): only select + menu, no thumbstick/squeeze.
	const XrAction A8[] = {
		p->aimAction, p->aimAction, p->gripAction, p->gripAction,
		p->triggerAction, p->triggerAction, p->aBtnAction, p->aBtnAction };
	const char* const simple[] = {
		"/user/hand/left/input/aim/pose",     "/user/hand/right/input/aim/pose",
		"/user/hand/left/input/grip/pose",    "/user/hand/right/input/grip/pose",
		"/user/hand/left/input/select/click", "/user/hand/right/input/select/click",
		"/user/hand/left/input/menu/click",   "/user/hand/right/input/menu/click" };
	SuggestProfile(p->instance, "/interaction_profiles/khr/simple_controller", A8, simple, 8);

	// Action spaces (aim + grip per hand) -- need the session.
	for (int h = 0; h < 2; ++h)
	{
		XrActionSpaceCreateInfo sci = { XR_TYPE_ACTION_SPACE_CREATE_INFO };
		sci.poseInActionSpace.orientation.w = 1.0f;
		sci.subactionPath = p->handPath[h];
		sci.action = p->aimAction;  xrCreateActionSpace(p->session, &sci, &p->aimSpace[h]);
		sci.action = p->gripAction; xrCreateActionSpace(p->session, &sci, &p->gripSpace[h]);
	}

	XrSessionActionSetsAttachInfo ai = { XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
	ai.countActionSets = 1;
	ai.actionSets      = &p->actionSet;
	if (XR_FAILED(xrAttachSessionActionSets(p->session, &ai))) return false;

	p->inputReady = true;
	return true;
}

// Per-frame: sync actions, locate aim/grip poses, read states. Called from BeginStereoFrame after xrBeginFrame.
void OpenXRBackend::SyncControllers()
{
	Impl* p = m_impl;
	if (!p->inputReady || p->session == XR_NULL_HANDLE) return;
	XrActiveActionSet aas; aas.actionSet = p->actionSet; aas.subactionPath = XR_NULL_PATH;
	XrActionsSyncInfo si = { XR_TYPE_ACTIONS_SYNC_INFO };
	si.countActiveActionSets = 1;
	si.activeActionSets      = &aas;
	if (XR_FAILED(xrSyncActions(p->session, &si))) return;   // e.g. session not focused yet

	const XrTime t = p->stereoFrameState.predictedDisplayTime;
	for (int h = 0; h < 2; ++h)
	{
		OpenXRBackend::Impl::HandInput& hi = p->hand[h];

		XrSpaceLocation loc = { XR_TYPE_SPACE_LOCATION };
		hi.aimValid = (p->aimSpace[h] != XR_NULL_HANDLE
		    && XR_SUCCEEDED(xrLocateSpace(p->aimSpace[h], p->appSpace, t, &loc))
		    && (loc.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)
		    && (loc.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT));
		if (hi.aimValid) hi.aimPose = loc.pose;

		XrSpaceLocation gloc = { XR_TYPE_SPACE_LOCATION };
		hi.gripValid = (p->gripSpace[h] != XR_NULL_HANDLE
		    && XR_SUCCEEDED(xrLocateSpace(p->gripSpace[h], p->appSpace, t, &gloc))
		    && (gloc.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT)
		    && (gloc.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT));
		if (hi.gripValid) hi.gripPose = gloc.pose;

		XrActionStateGetInfo gi = { XR_TYPE_ACTION_STATE_GET_INFO };
		gi.subactionPath = p->handPath[h];

		XrActionStateFloat sf = { XR_TYPE_ACTION_STATE_FLOAT };
		gi.action = p->triggerAction;
		hi.trigger = (XR_SUCCEEDED(xrGetActionStateFloat(p->session, &gi, &sf)) && sf.isActive) ? sf.currentState : 0.0f;
		hi.triggerDown = hi.trigger > 0.6f;

		gi.action = p->squeezeAction;
		float sq = (XR_SUCCEEDED(xrGetActionStateFloat(p->session, &gi, &sf)) && sf.isActive) ? sf.currentState : 0.0f;
		bool sqDown = sq > 0.6f;
		if (sqDown && !hi.squeezeDown) p->activeHand = h;   // rising edge -> this hand owns the ray
		hi.squeeze = sq; hi.squeezeDown = sqDown;

		XrActionStateVector2f sv = { XR_TYPE_ACTION_STATE_VECTOR2F };
		gi.action = p->thumbAction;
		if (XR_SUCCEEDED(xrGetActionStateVector2f(p->session, &gi, &sv)) && sv.isActive) { hi.thumbX = sv.currentState.x; hi.thumbY = sv.currentState.y; }
		else { hi.thumbX = hi.thumbY = 0.0f; }

		XrActionStateBoolean sb = { XR_TYPE_ACTION_STATE_BOOLEAN };
		gi.action = p->aBtnAction;
		hi.aBtn = (XR_SUCCEEDED(xrGetActionStateBoolean(p->session, &gi, &sb)) && sb.isActive) ? (sb.currentState != XR_FALSE) : false;
		gi.action = p->bBtnAction;
		hi.bBtn = (XR_SUCCEEDED(xrGetActionStateBoolean(p->session, &gi, &sb)) && sb.isActive) ? (sb.currentState != XR_FALSE) : false;
	}

	// Artscout - 2026 (VR hands): locate the 26 hand joints for each hand in the app (LOCAL) space, this
	// frame's predicted time. handJointsValid[h] gates the skeleton render; when false the caller falls back
	// to the wireframe controller. Index/knuckles feed this from the grip's capacitive finger sensors.
	if (p->handTrackingEnabled && p->pfnLocateHandJoints)
	{
		for (int h = 0; h < 2; ++h)
		{
			p->handJointsValid[h] = false;
			if (p->handTracker[h] == XR_NULL_HANDLE) continue;
			XrHandJointsLocateInfoEXT li = { XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT };
			li.baseSpace = p->appSpace;
			li.time      = t;
			XrHandJointLocationsEXT locs = { XR_TYPE_HAND_JOINT_LOCATIONS_EXT };
			locs.jointCount     = XR_HAND_JOINT_COUNT_EXT;
			locs.jointLocations = p->handJoints[h];
			XrResult lr = p->pfnLocateHandJoints(p->handTracker[h], &li, &locs);
			if (XR_SUCCEEDED(lr) && locs.isActive)
				p->handJointsValid[h] = true;
			// TEMP DIAG: once every ~2s per hand, report why hands may be falling back to the wireframe.
			{ static int s_hn[2] = {0,0}; if ((s_hn[h]++ % 180) == 0)
				XrDbg("OpenXR: locateHandJoints hand %d -> result=%d isActive=%d valid=%d\n", h, (int)lr, (int)locs.isActive, (int)p->handJointsValid[h]); }
		}
	}
}

//=============================================================================
// Construction
//=============================================================================
OpenXRBackend::OpenXRBackend() : m_impl(new Impl()) {}
OpenXRBackend::~OpenXRBackend() { Shutdown(); delete m_impl; m_impl = NULL; }

bool OpenXRBackend::IsInitialized() const { return m_impl && m_impl->instance != XR_NULL_HANDLE; }
bool OpenXRBackend::IsSessionRunning() const { return m_impl && m_impl->sessionRunning; }
bool OpenXRBackend::IsQuadViews() const { return m_impl && m_impl->viewConfigType == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO; }
void OpenXRBackend::SetClipPlanes(float nearZ, float farZ) { m_impl->nearZ = nearZ; m_impl->farZ = farZ; }

//=============================================================================
// Init
//=============================================================================
bool OpenXRBackend::Init(ID3D11Device* device)
{
	Impl* p = m_impl;

	// #DX12 п.5: bind the session to the D3D12 device/queue (from g_pD3D12Backend) instead of D3D11.
	// The 'device' param is NULL under D3D12 -- the D3D12 devmgr branch calls Init(NULL).
	extern bool g_bUseD3D12;
	p->useD3D12 = g_bUseD3D12;
	if (p->useD3D12)
	{
		if (!g_pD3D12Backend || !g_pD3D12Backend->IsValid()) { XrDbg("OpenXR: Init - no D3D12 backend\n"); return false; }
		p->d3d12Device = g_pD3D12Backend->GetDevice();
		p->d3d12Queue  = g_pD3D12Backend->GetQueue();
		if (!p->d3d12Device || !p->d3d12Queue) { XrDbg("OpenXR: Init - null D3D12 device/queue\n"); return false; }
		// RTV heap + a command list/allocator/fence for the eye clear/composite work (D3D12 has no clear-by-view).
		D3D12_DESCRIPTOR_HEAP_DESC hd; ZeroMemory(&hd, sizeof(hd));
		hd.NumDescriptors = 32; hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		if (FAILED(p->d3d12Device->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&p->rtvHeap12)))) { XrDbg("OpenXR: RTV heap failed\n"); return false; }
		p->rtvInc12 = p->d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		p->rtvHead12 = 0;
		p->d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&p->alloc12));
		p->d3d12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, p->alloc12, NULL, IID_PPV_ARGS(&p->list12));
		if (p->list12) p->list12->Close();
		p->d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&p->fence12));
		p->fenceVal12 = 0;
		p->fenceEvt12 = CreateEvent(NULL, FALSE, FALSE, NULL);
	}
	else
	{
		if (!device) { XrDbg("OpenXR: Init - null device\n"); return false; }
		p->device = device;
		device->GetImmediateContext(&p->ctx);

		// The engine drives D3D11 from several threads (sim render, ui95 OutputLoop) and
		// the XR pump adds another; the immediate context is NOT thread-safe by default
		// (-> "CORRUPTED_MULTITHREADING" + crash). Turn on the runtime's internal locking
		// so concurrent context calls are serialized. Required once VR is on.
		ID3D11Multithread* mt = NULL;
		if (SUCCEEDED(p->ctx->QueryInterface(__uuidof(ID3D11Multithread), (void**)&mt)) && mt)
		{
			mt->SetMultithreadProtected(TRUE);
			mt->Release();
			XrDbg("OpenXR: D3D11 multithread protection enabled\n");
		}
		else
			XrDbg("OpenXR: WARNING could not enable D3D11 multithread protection\n");
	}

	// --- 1. Require the D3D11 graphics extension -----------------------------
	uint32_t extCount = 0;
	if (XR_FAILED(xrEnumerateInstanceExtensionProperties(NULL, 0, &extCount, NULL)))
	{
		XrDbg("OpenXR: no runtime / xrEnumerateInstanceExtensionProperties failed\n");
		return false;
	}
	std::vector<XrExtensionProperties> exts(extCount);
	for (uint32_t i = 0; i < extCount; ++i) { exts[i].type = XR_TYPE_EXTENSION_PROPERTIES; exts[i].next = NULL; }
	xrEnumerateInstanceExtensionProperties(NULL, extCount, &extCount, exts.empty() ? NULL : exts.data());

	bool haveD3D11 = false, haveD3D12 = false, haveQuadViews = false, haveEyeGaze = false, haveHandTracking = false;
	bool haveCtrlModelMSFT = false, haveRenderModelEXT = false, haveInteractionRenderModelEXT = false;
	for (uint32_t i = 0; i < extCount; ++i)
	{
		const char* n = exts[i].extensionName;
		if (strcmp(n, XR_KHR_D3D12_ENABLE_EXTENSION_NAME) == 0)              haveD3D12 = true;   // #DX12 п.5
		else if (strcmp(n, XR_KHR_D3D11_ENABLE_EXTENSION_NAME) == 0)         haveD3D11 = true;
		else if (strcmp(n, XR_VARJO_QUAD_VIEWS_EXTENSION_NAME) == 0)         haveQuadViews = true;
		else if (strcmp(n, XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME) == 0) haveEyeGaze = true;
		else if (strcmp(n, XR_EXT_HAND_TRACKING_EXTENSION_NAME) == 0)        haveHandTracking = true;  // real hands
		// Artscout - 2026 (VR controllers): probe for runtime-provided controller glTF model extensions. String
		// literals (not SDK macros) so it builds on older openxr headers. Any of these lets us fetch the REAL
		// controller mesh (glTF); EXT_render_model is the cross-vendor successor SteamVR/Valve is likelier to
		// expose than the MSFT one. interaction_render_model pairs models with our action-based bindings.
		else if (strcmp(n, "XR_MSFT_controller_model") == 0)                 haveCtrlModelMSFT = true;
		else if (strcmp(n, "XR_EXT_render_model") == 0)                      haveRenderModelEXT = true;
		else if (strcmp(n, "XR_EXT_interaction_render_model") == 0)          haveInteractionRenderModelEXT = true;
	}
	// Dump every advertised extension once so we can see exactly what THIS runtime (SteamVR/PVR/WMR) offers.
	XrDbg("OpenXR: %u instance extensions advertised by the runtime:\n", extCount);
	for (uint32_t i = 0; i < extCount; ++i) XrDbg("OpenXR:   ext[%u] = %s\n", i, exts[i].extensionName);
	XrDbg("OpenXR: controller-model extensions:  MSFT_controller_model=%d  EXT_render_model=%d  EXT_interaction_render_model=%d\n",
	      (int)haveCtrlModelMSFT, (int)haveRenderModelEXT, (int)haveInteractionRenderModelEXT);
	if (p->useD3D12 ? !haveD3D12 : !haveD3D11)   // #DX12 п.5: the active backend's graphics extension is mandatory
	{
		XrDbg("OpenXR: runtime lacks %s -- VR unavailable\n",
		      p->useD3D12 ? XR_KHR_D3D12_ENABLE_EXTENSION_NAME : XR_KHR_D3D11_ENABLE_EXTENSION_NAME);
		return false;
	}

	// --- 2. Create instance (D3D11 + optional quad-views + eye-gaze) ---------
	// Quad views (4 viewports: wide context + narrow focus per eye) + eye gaze let the
	// foveated-rendering API layer (e.g. Quad-Views-Foveated) do gaze-driven foveation.
	// The whole view/swapchain/render path below is N-view generic, so 2 or 4 just works.
	std::vector<const char*> enabledExts;
	enabledExts.push_back(p->useD3D12 ? XR_KHR_D3D12_ENABLE_EXTENSION_NAME : XR_KHR_D3D11_ENABLE_EXTENSION_NAME);   // #DX12 п.5
	if (haveQuadViews) enabledExts.push_back(XR_VARJO_QUAD_VIEWS_EXTENSION_NAME);
	if (haveEyeGaze)   enabledExts.push_back(XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME);
	// Artscout - 2026 (VR controllers): also ENABLE the controller render-model extension(s) so we can call
	// their functions later (loading the real controller glTF). Enumeration above only reports SUPPORT; using
	// the functions requires the extension to be in enabledExtensionNames here. Guarded by availability, and
	// with a fallback retry below -- if enabling them makes xrCreateInstance fail (unmet dependency on some
	// runtime), we drop them and retry so VR still comes up on the wireframe. The count split lets us peel
	// them back off precisely.
	const size_t coreExtCount = enabledExts.size();
	if (haveHandTracking)              enabledExts.push_back(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
	if (haveRenderModelEXT)            enabledExts.push_back("XR_EXT_render_model");
	if (haveInteractionRenderModelEXT) enabledExts.push_back("XR_EXT_interaction_render_model");
	if (haveCtrlModelMSFT)             enabledExts.push_back("XR_MSFT_controller_model");
	XrDbg("OpenXR: ext quadViews=%d eyeGaze=%d | enabling handTracking=%d renderModelEXT=%d interactionRenderModelEXT=%d ctrlModelMSFT=%d\n",
	      (int)haveQuadViews, (int)haveEyeGaze, (int)haveHandTracking, (int)haveRenderModelEXT, (int)haveInteractionRenderModelEXT, (int)haveCtrlModelMSFT);

	XrInstanceCreateInfo ici = { XR_TYPE_INSTANCE_CREATE_INFO };
	strcpy(ici.applicationInfo.applicationName, "FreeFalcon");
	ici.applicationInfo.applicationVersion = 1;
	strcpy(ici.applicationInfo.engineName, "FFViper");
	ici.applicationInfo.engineVersion = 1;
	ici.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
	ici.enabledExtensionCount = (uint32_t)enabledExts.size();
	ici.enabledExtensionNames = enabledExts.data();
	XrResult icr = xrCreateInstance(&ici, &p->instance);
	if (XR_FAILED(icr) && enabledExts.size() > coreExtCount)
	{
		// Retry without the optional render-model extensions so a missing dependency can't kill VR.
		XrDbg("OpenXR: xrCreateInstance failed (XrResult %d) WITH render-model exts; retrying without them\n", (int)icr);
		enabledExts.resize(coreExtCount);
		ici.enabledExtensionCount = (uint32_t)enabledExts.size();
		ici.enabledExtensionNames = enabledExts.data();
		icr = xrCreateInstance(&ici, &p->instance);
	}
	if (XR_FAILED(icr))
	{
		XrDbg("OpenXR: xrCreateInstance failed (XrResult %d)\n", (int)icr);
		return false;
	}
	// Hand tracking is usable only if it survived on the instance (the fallback retry peels ALL optional
	// exts off together, so if it fired, hand tracking is gone too). Entry points are resolved after this.
	p->handTrackingEnabled = haveHandTracking && (enabledExts.size() > coreExtCount);

	// --- 3. System (HMD) -----------------------------------------------------
	XrSystemGetInfo sgi = { XR_TYPE_SYSTEM_GET_INFO };
	sgi.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	if (XR_FAILED(xrGetSystem(p->instance, &sgi, &p->systemId)))
	{
		XrDbg("OpenXR: no HMD system available (headset off?)\n");
		Shutdown();
		return false;
	}

	// --- 3b. Prefer the quad-views (foveated) config if the runtime exposes it. ---
	// Everything downstream (configViews/views/swapchains/render loop/projection layer)
	// sizes off the view count, so this turns on 4-view rendering with no other change.
	p->viewConfigType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
	// v1 per-eye stereo uses the engine's symmetric projection + IPD position offset, which is
	// correct for STEREO (parallel) but NOT for QUAD's off-center (gaze-tracked) focus views. The
	// whole downstream path is N-view generic, so 4 views "just render"; with fixed foveation (no
	// eye tracking) the focus views are centered and the symmetric render stays internally consistent
	// (render fov == submit fov). Gated behind FFViper.cfg "UseQuadViews 1" -- EXPERIMENTAL, default
	// OFF so the proven 2-view stereo path stays the norm. (g_bUseQuadViews lives in f4config.cpp.)
	extern bool g_bUseQuadViews;
	if (g_bUseQuadViews && haveQuadViews)
	{
		uint32_t vcCount = 0;
		if (XR_SUCCEEDED(xrEnumerateViewConfigurations(p->instance, p->systemId, 0, &vcCount, NULL)) && vcCount)
		{
			std::vector<XrViewConfigurationType> vcs(vcCount);
			if (XR_SUCCEEDED(xrEnumerateViewConfigurations(p->instance, p->systemId, vcCount, &vcCount, vcs.data())))
				for (uint32_t i = 0; i < vcCount; ++i)
					if (vcs[i] == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO)
					{ p->viewConfigType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO; break; }
		}
	}
	XrDbg("OpenXR: view config = %s\n",
	      p->viewConfigType == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_QUAD_VARJO ? "QUAD_VARJO (4 views)" : "STEREO (2 views)");

	// --- 4/5. graphics requirements + session (bound to the active backend's device) --------
	if (p->useD3D12)   // #DX12 п.5: D3D12 graphics binding (device + command queue)
	{
		PFN_xrGetD3D12GraphicsRequirementsKHR pfnReqs = NULL;
		xrGetInstanceProcAddr(p->instance, "xrGetD3D12GraphicsRequirementsKHR", (PFN_xrVoidFunction*)&pfnReqs);
		if (!pfnReqs) { XrDbg("OpenXR: xrGetD3D12GraphicsRequirementsKHR unavailable\n"); Shutdown(); return false; }
		XrGraphicsRequirementsD3D12KHR reqs = { XR_TYPE_GRAPHICS_REQUIREMENTS_D3D12_KHR };
		XR_BAIL(pfnReqs(p->instance, p->systemId, &reqs), "xrGetD3D12GraphicsRequirementsKHR");

		XrGraphicsBindingD3D12KHR gb = { XR_TYPE_GRAPHICS_BINDING_D3D12_KHR };
		gb.device = p->d3d12Device;
		gb.queue  = p->d3d12Queue;
		XrSessionCreateInfo sci = { XR_TYPE_SESSION_CREATE_INFO };
		sci.next = &gb;
		sci.systemId = p->systemId;
		XR_BAIL(xrCreateSession(p->instance, &sci, &p->session), "xrCreateSession(D3D12)");
	}
	else
	{
		PFN_xrGetD3D11GraphicsRequirementsKHR pfnReqs = NULL;
		xrGetInstanceProcAddr(p->instance, "xrGetD3D11GraphicsRequirementsKHR",
		                      (PFN_xrVoidFunction*)&pfnReqs);
		if (!pfnReqs) { XrDbg("OpenXR: xrGetD3D11GraphicsRequirementsKHR unavailable\n"); Shutdown(); return false; }
		XrGraphicsRequirementsD3D11KHR reqs = { XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
		XR_BAIL(pfnReqs(p->instance, p->systemId, &reqs), "xrGetD3D11GraphicsRequirementsKHR");

		// Warn (do not fail) if our existing device is on a different adapter than the
		// runtime wants -- on a single-GPU PC they match.
		IDXGIDevice* dxgiDev = NULL;
		if (SUCCEEDED(device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDev)) && dxgiDev)
		{
			IDXGIAdapter* adapter = NULL;
			if (SUCCEEDED(dxgiDev->GetAdapter(&adapter)) && adapter)
			{
				DXGI_ADAPTER_DESC desc;
				if (SUCCEEDED(adapter->GetDesc(&desc)))
				{
					if (memcmp(&desc.AdapterLuid, &reqs.adapterLuid, sizeof(LUID)) != 0)
						XrDbg("OpenXR: WARNING device adapter LUID != XR adapter LUID (multi-GPU?)\n");
				}
				adapter->Release();
			}
			dxgiDev->Release();
		}

		XrGraphicsBindingD3D11KHR gb = { XR_TYPE_GRAPHICS_BINDING_D3D11_KHR };
		gb.device = device;
		XrSessionCreateInfo sci = { XR_TYPE_SESSION_CREATE_INFO };
		sci.next = &gb;
		sci.systemId = p->systemId;
		XR_BAIL(xrCreateSession(p->instance, &sci, &p->session), "xrCreateSession");
	}

	// --- 6. Reference spaces (app=LOCAL, head=VIEW) --------------------------
	XrReferenceSpaceCreateInfo rsci = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
	rsci.poseInReferenceSpace.orientation.w = 1.0f;
	rsci.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
	XR_BAIL(xrCreateReferenceSpace(p->session, &rsci, &p->appSpace), "xrCreateReferenceSpace(LOCAL)");
	// Artscout - 2026 (#67): a second, identity LOCAL space kept PRISTINE as the recenter measurement frame.
	XR_BAIL(xrCreateReferenceSpace(p->session, &rsci, &p->localRef), "xrCreateReferenceSpace(LOCAL ref)");
	rsci.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
	XR_BAIL(xrCreateReferenceSpace(p->session, &rsci, &p->viewSpace), "xrCreateReferenceSpace(VIEW)");

	// Artscout - 2026 (VR controllers, Phase 1): action-based input. Non-fatal -- VR still runs without it
	// (the cockpit falls back to the VR mouse when no controller is tracked).
	if (!CreateInputActions())
		XrDbg("OpenXR: controller input unavailable (continuing without controllers)\n");

	// Artscout - 2026 (VR hands): resolve the hand-tracking entry points and create a tracker per hand.
	// Non-fatal: any failure just leaves handTrackingEnabled effectively off and the wireframe is used.
	XrDbg("OpenXR: handTrackingEnabled(instance)=%d\n", (int)p->handTrackingEnabled);
	if (p->handTrackingEnabled)
	{
		// Authoritative check: the runtime may ADVERTISE the extension yet report the SYSTEM can't do hand
		// tracking (common when there's no camera and the controller driver doesn't synthesize a skeleton).
		XrSystemHandTrackingPropertiesEXT htp = { XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT };
		XrSystemProperties sp = { XR_TYPE_SYSTEM_PROPERTIES };
		sp.next = &htp;
		XrResult spr = xrGetSystemProperties(p->instance, p->systemId, &sp);
		XrDbg("OpenXR: system supportsHandTracking=%d (xrGetSystemProperties %d)\n", (int)htp.supportsHandTracking, (int)spr);

		xrGetInstanceProcAddr(p->instance, "xrCreateHandTrackerEXT",  (PFN_xrVoidFunction*)&p->pfnCreateHandTracker);
		xrGetInstanceProcAddr(p->instance, "xrDestroyHandTrackerEXT", (PFN_xrVoidFunction*)&p->pfnDestroyHandTracker);
		xrGetInstanceProcAddr(p->instance, "xrLocateHandJointsEXT",   (PFN_xrVoidFunction*)&p->pfnLocateHandJoints);
		XrDbg("OpenXR: hand PFNs create=%p locate=%p\n", (void*)p->pfnCreateHandTracker, (void*)p->pfnLocateHandJoints);
		if (p->pfnCreateHandTracker && p->pfnLocateHandJoints)
		{
			for (int h = 0; h < 2; ++h)
			{
				XrHandTrackerCreateInfoEXT hci = { XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT };
				hci.hand = (h == 0) ? XR_HAND_LEFT_EXT : XR_HAND_RIGHT_EXT;
				hci.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
				XrResult hr = p->pfnCreateHandTracker(p->session, &hci, &p->handTracker[h]);
				if (XR_FAILED(hr)) { p->handTracker[h] = XR_NULL_HANDLE; XrDbg("OpenXR: xrCreateHandTrackerEXT(hand %d) failed (%d)\n", h, (int)hr); }
			}
			XrDbg("OpenXR: hand tracking initialised (L=%d R=%d)\n", (int)(p->handTracker[0] != XR_NULL_HANDLE), (int)(p->handTracker[1] != XR_NULL_HANDLE));
		}
		else
		{
			p->handTrackingEnabled = false;
			XrDbg("OpenXR: hand-tracking entry points missing -- disabled\n");
		}
	}

	// --- 7. Per-eye view configuration ---------------------------------------
	uint32_t viewCount = 0;
	XR_BAIL(xrEnumerateViewConfigurationViews(p->instance, p->systemId, p->viewConfigType,
	        0, &viewCount, NULL), "xrEnumerateViewConfigurationViews(count)");
	p->configViews.resize(viewCount);
	for (uint32_t i = 0; i < viewCount; ++i) { p->configViews[i].type = XR_TYPE_VIEW_CONFIGURATION_VIEW; p->configViews[i].next = NULL; }
	XR_BAIL(xrEnumerateViewConfigurationViews(p->instance, p->systemId, p->viewConfigType,
	        viewCount, &viewCount, p->configViews.data()), "xrEnumerateViewConfigurationViews");
	p->views.resize(viewCount);
	for (uint32_t i = 0; i < viewCount; ++i) { p->views[i].type = XR_TYPE_VIEW; p->views[i].next = NULL; }

	// --- 8. Pick a color swapchain format ------------------------------------
	uint32_t fmtCount = 0;
	XR_BAIL(xrEnumerateSwapchainFormats(p->session, 0, &fmtCount, NULL), "xrEnumerateSwapchainFormats(count)");
	std::vector<int64_t> fmts(fmtCount);
	XR_BAIL(xrEnumerateSwapchainFormats(p->session, fmtCount, &fmtCount, fmts.data()), "xrEnumerateSwapchainFormats");
	const int64_t preferred[] = { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
	                              DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM };
	p->swapchainFormat = 0;
	for (int pf = 0; pf < 4 && p->swapchainFormat == 0; ++pf)
		for (uint32_t i = 0; i < fmtCount; ++i)
			if (fmts[i] == preferred[pf]) { p->swapchainFormat = preferred[pf]; break; }
	if (p->swapchainFormat == 0 && fmtCount > 0) p->swapchainFormat = fmts[0];

	// Menu quad swapchain must be an R8G8B8A8 variant: the 565->RGBA staging upload
	// is R8G8B8A8_UNORM and CopyResource needs the same format family. Prefer _SRGB
	// (the UI bytes are sRGB-encoded, like the desktop swapchain), else plain UNORM.
	const int64_t uiPref[] = { DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_R8G8B8A8_UNORM };
	p->uiFormat = 0;
	for (int pf = 0; pf < 2 && p->uiFormat == 0; ++pf)
		for (uint32_t i = 0; i < fmtCount; ++i)
			if (fmts[i] == uiPref[pf]) { p->uiFormat = uiPref[pf]; break; }

	// --- 9. Per-eye swapchains + RTVs ----------------------------------------
	p->swapchains.resize(viewCount);
	for (uint32_t e = 0; e < viewCount; ++e)
	{
		Impl::Swapchain& sc = p->swapchains[e];
		// Full per-eye resolution from OpenXR (per headset). The engine renders each eye at
		// this size: otwloop pushes it into the renderer's xRes/yRes so the projection aspect
		// matches the eye viewport (no distortion), and submits the engine fov.
		sc.width  = (int32_t)p->configViews[e].recommendedImageRectWidth;
		sc.height = (int32_t)p->configViews[e].recommendedImageRectHeight;

		// Artscout - 2026: optional per-eye resolution scale (Advanced page "OpenXR Resolution Scale", 70..100%).
		// Trades sharpness for GPU headroom. 100 = the runtime's recommended size (unchanged). The whole engine
		// derives the per-eye viewport/projection from sc.width/height, so scaling here flows everywhere.
		{
			extern int g_nVrResolutionScale;
			int scl = g_nVrResolutionScale;
			if (scl < 50)  scl = 50;
			if (scl > 100) scl = 100;
			if (scl < 100)
			{
				sc.width  = (sc.width  * scl) / 100;
				sc.height = (sc.height * scl) / 100;
				if (sc.width  < 256) sc.width  = 256;
				if (sc.height < 256) sc.height = 256;
			}
		}

		XrSwapchainCreateInfo scci = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
		scci.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
		scci.format     = p->swapchainFormat;
		scci.sampleCount = 1;
		scci.width  = sc.width;
		scci.height = sc.height;
		scci.faceCount = 1;
		scci.arraySize = 1;
		scci.mipCount  = 1;
		XR_BAIL(xrCreateSwapchain(p->session, &scci, &sc.handle), "xrCreateSwapchain");

		uint32_t imgCount = 0;
		XR_BAIL(xrEnumerateSwapchainImages(sc.handle, 0, &imgCount, NULL), "xrEnumerateSwapchainImages(count)");

		if (p->useD3D12)   // #DX12 п.5: D3D12 swapchain images -> RTVs in the shared rtvHeap12
		{
			sc.images12.resize(imgCount);
			for (uint32_t i = 0; i < imgCount; ++i) { sc.images12[i].type = XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR; sc.images12[i].next = NULL; }
			XR_BAIL(xrEnumerateSwapchainImages(sc.handle, imgCount, &imgCount,
			        (XrSwapchainImageBaseHeader*)sc.images12.data()), "xrEnumerateSwapchainImages(D3D12)");
			sc.rtvs12.resize(imgCount, 0);
			for (uint32_t i = 0; i < imgCount; ++i)
			{
				D3D12_CPU_DESCRIPTOR_HANDLE h = p->rtvHeap12->GetCPUDescriptorHandleForHeapStart();
				h.ptr += (SIZE_T)p->rtvHead12 * p->rtvInc12; p->rtvHead12++;
				D3D12_RENDER_TARGET_VIEW_DESC rd; ZeroMemory(&rd, sizeof(rd));
				// #DX12 п.5: the runtime picks an _SRGB swapchain format (fmt=29 R8G8B8A8_UNORM_SRGB), but the
				// renderer's PSOs bake R8G8B8A8_UNORM -> #613 RENDER_TARGET_FORMAT_MISMATCH and dropped draws.
				// Create the RTV with the UNORM cast of the same family (legal, same memory layout) so the eye
				// image accepts the scene draws (no gamma re-encode -- matches how the D3D11 eye path writes).
				DXGI_FORMAT rtvFmt = (DXGI_FORMAT)p->swapchainFormat;
				if (rtvFmt == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) rtvFmt = DXGI_FORMAT_R8G8B8A8_UNORM;
				else if (rtvFmt == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB) rtvFmt = DXGI_FORMAT_B8G8R8A8_UNORM;
				rd.Format = rtvFmt; rd.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
				p->d3d12Device->CreateRenderTargetView(sc.images12[i].texture, &rd, h);
				sc.rtvs12[i] = (unsigned __int64)h.ptr;
			}
		}
		else
		{
			sc.images.resize(imgCount);
			for (uint32_t i = 0; i < imgCount; ++i) { sc.images[i].type = XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR; sc.images[i].next = NULL; }
			XR_BAIL(xrEnumerateSwapchainImages(sc.handle, imgCount, &imgCount,
			        (XrSwapchainImageBaseHeader*)sc.images.data()), "xrEnumerateSwapchainImages");

			sc.rtvs.resize(imgCount, NULL);
			for (uint32_t i = 0; i < imgCount; ++i)
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtvd;
				ZeroMemory(&rtvd, sizeof(rtvd));
				rtvd.Format = (DXGI_FORMAT)p->swapchainFormat;
				rtvd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
				if (FAILED(device->CreateRenderTargetView(sc.images[i].texture, &rtvd, &sc.rtvs[i])))
				{
					XrDbg("OpenXR: CreateRenderTargetView failed (eye %u img %u)\n", e, i);
					Shutdown();
					return false;
				}
			}
		}
	}

	XrDbg("OpenXR: up -- %u views, %dx%d, fmt=%lld\n",
	          viewCount, p->swapchains[0].width, p->swapchains[0].height, (long long)p->swapchainFormat);
	return true;
}

//=============================================================================
// Event pump -- drives the session state machine. Shared by RunFrame/RunMenuFrame.
//=============================================================================
void OpenXRBackend::PollEvents()
{
	Impl* p = m_impl;
	XrEventDataBuffer ev;
	for (;;)
	{
		ev.type = XR_TYPE_EVENT_DATA_BUFFER;
		ev.next = NULL;
		if (xrPollEvent(p->instance, &ev) != XR_SUCCESS) break;   // XR_EVENT_UNAVAILABLE -> done

		if (ev.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED)
		{
			const XrEventDataSessionStateChanged& ssc = *(XrEventDataSessionStateChanged*)&ev;
			p->sessionState = ssc.state;
			switch (ssc.state)
			{
			case XR_SESSION_STATE_READY:
			{
				XrSessionBeginInfo bi = { XR_TYPE_SESSION_BEGIN_INFO };
				bi.primaryViewConfigurationType = p->viewConfigType;
				if (XR_SUCCEEDED(xrBeginSession(p->session, &bi)))
				{
					p->sessionRunning = true;
					XrDbg("OpenXR: session begun (running)\n");
				}
				break;
			}
			case XR_SESSION_STATE_STOPPING:
				xrEndSession(p->session);
				p->sessionRunning = false;
				XrDbg("OpenXR: session stopped\n");
				break;
			case XR_SESSION_STATE_EXITING:
			case XR_SESSION_STATE_LOSS_PENDING:
				p->sessionRunning = false;
				break;
			default:
				break;
			}
		}
		else if (ev.type == XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING)
		{
			p->sessionRunning = false;
		}
	}
}

// #DX12 п.5: clear an acquired XR D3D12 swapchain image (color) via the dedicated queue. XR D3D12 images
// start/rest in COMMON; we round-trip COMMON->RENDER_TARGET->COMMON so the runtime always gets a valid state.
// Synchronous (fence-wait) -- the runtime's xrReleaseSwapchainImage assumes our GPU work is done. Params are
// raw D3D12 interfaces (no access to OpenXRBackend::Impl's private type from a free function).
static void XrClearRtvD3D12(ID3D12CommandAllocator* alloc, ID3D12GraphicsCommandList* list,
                            ID3D12CommandQueue* queue, ID3D12Fence* fence, HANDLE evt, unsigned __int64* fenceVal,
                            ID3D12Resource* img, unsigned __int64 rtvPtr, const float clr[4])
{
	if (!alloc || !list || !queue || !fence || !img || !rtvPtr) return;
	alloc->Reset();
	list->Reset(alloc, NULL);
	D3D12_RESOURCE_BARRIER b; ZeroMemory(&b, sizeof(b));
	b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	b.Transition.pResource = img; b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	b.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
	b.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
	list->ResourceBarrier(1, &b);
	D3D12_CPU_DESCRIPTOR_HANDLE rtv; rtv.ptr = (SIZE_T)rtvPtr;
	list->OMSetRenderTargets(1, &rtv, FALSE, NULL);
	list->ClearRenderTargetView(rtv, clr, 0, NULL);
	D3D12_RESOURCE_BARRIER b2 = b;
	b2.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	b2.Transition.StateAfter  = D3D12_RESOURCE_STATE_COMMON;
	list->ResourceBarrier(1, &b2);
	list->Close();
	ID3D12CommandList* lists[] = { (ID3D12CommandList*)list };
	queue->ExecuteCommandLists(1, lists);
	(*fenceVal)++;
	queue->Signal(fence, *fenceVal);
	if (fence->GetCompletedValue() < *fenceVal) { fence->SetEventOnCompletion(*fenceVal, evt); WaitForSingleObject(evt, INFINITE); }
}

//=============================================================================
// RunFrame
//=============================================================================
bool OpenXRBackend::RunFrame(XrEyeRenderFn render, void* user)
{
	Impl* p = m_impl;
	if (!p->instance || p->session == XR_NULL_HANDLE) return false;

	PollEvents();
	if (!p->sessionRunning) return false;

	XrFrameWaitInfo fwi = { XR_TYPE_FRAME_WAIT_INFO };
	XrFrameState fs = { XR_TYPE_FRAME_STATE };
	if (XR_FAILED(xrWaitFrame(p->session, &fwi, &fs))) return false;

	XrFrameBeginInfo fbi = { XR_TYPE_FRAME_BEGIN_INFO };
	if (XR_FAILED(xrBeginFrame(p->session, &fbi))) return false;

	std::vector<XrCompositionLayerProjectionView> projViews;
	XrCompositionLayerProjection layer = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
	bool haveLayer = false;

	if (fs.shouldRender)
	{
		// Locate the per-eye views in app space at the predicted display time.
		XrViewState vs = { XR_TYPE_VIEW_STATE };
		XrViewLocateInfo vli = { XR_TYPE_VIEW_LOCATE_INFO };
		vli.viewConfigurationType = p->viewConfigType;
		vli.displayTime = fs.predictedDisplayTime;
		vli.space = p->appSpace;
		uint32_t viewCountOut = 0;
		XrResult lr = xrLocateViews(p->session, &vli, &vs, (uint32_t)p->views.size(), &viewCountOut, p->views.data());

		const bool posesValid = XR_SUCCEEDED(lr) &&
			(vs.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) &&
			(vs.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT);

		// Head pose (VIEW space relative to app space) for the cockpit camera feed.
		{
			XrSpaceLocation loc = { XR_TYPE_SPACE_LOCATION };
			if (XR_SUCCEEDED(xrLocateSpace(p->viewSpace, p->appSpace, fs.predictedDisplayTime, &loc)) &&
				(loc.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) &&
				(loc.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT))
			{
				p->lastHeadPose = loc.pose;
				p->haveHeadPose = true;
			}
		}

		if (posesValid)
		{
			projViews.resize(viewCountOut);
			for (uint32_t e = 0; e < viewCountOut; ++e)
			{
				Impl::Swapchain& sc = p->swapchains[e];

				uint32_t imgIndex = 0;
				XrSwapchainImageAcquireInfo ai = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
				if (XR_FAILED(xrAcquireSwapchainImage(sc.handle, &ai, &imgIndex))) continue;

				XrSwapchainImageWaitInfo wi = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
				wi.timeout = XR_INFINITE_DURATION;
				if (XR_FAILED(xrWaitSwapchainImage(sc.handle, &wi)))
				{
					XrSwapchainImageReleaseInfo ri = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
					xrReleaseSwapchainImage(sc.handle, &ri);
					continue;
				}

				const float clear[4] = { 0.05f, 0.10f, 0.18f, 1.0f };
				if (p->useD3D12)
				{
					// #DX12 п.5 Milestone 1: clear the eye image on the D3D12 queue (per-eye scene render = later increment).
					if (imgIndex < sc.images12.size())
						XrClearRtvD3D12(p->alloc12, p->list12, p->d3d12Queue, p->fence12, p->fenceEvt12, &p->fenceVal12,
						                sc.images12[imgIndex].texture, sc.rtvs12[imgIndex], clear);
				}
				else
				{
					ID3D11RenderTargetView* rtv = sc.rtvs[imgIndex];

					// Per-eye matrices (milestone 2 uses these; see handedness note above).
					float proj[16], view[16];
					XrProjectionToD3D(p->views[e].fov, p->nearZ, p->farZ, proj);
					XrPoseToView(p->views[e].pose, view);

					if (render)
						render(user, (int)e, rtv, sc.width, sc.height, view, proj);
					else
						p->ctx->ClearRenderTargetView(rtv, clear);   // Milestone 1: just clear
				}

				XrSwapchainImageReleaseInfo ri = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
				xrReleaseSwapchainImage(sc.handle, &ri);

				XrCompositionLayerProjectionView& pv = projViews[e];
				pv.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
				pv.next = NULL;
				pv.pose = p->views[e].pose;
				pv.fov  = p->views[e].fov;
				pv.subImage.swapchain = sc.handle;
				pv.subImage.imageRect.offset.x = 0;
				pv.subImage.imageRect.offset.y = 0;
				pv.subImage.imageRect.extent.width  = sc.width;
				pv.subImage.imageRect.extent.height = sc.height;
				pv.subImage.imageArrayIndex = 0;
			}

			layer.space = p->appSpace;
			layer.viewCount = (uint32_t)projViews.size();
			layer.views = projViews.data();
			haveLayer = true;
		}
	}

	XrCompositionLayerBaseHeader* layers[1] = { (XrCompositionLayerBaseHeader*)&layer };
	XrFrameEndInfo fei = { XR_TYPE_FRAME_END_INFO };
	fei.displayTime = fs.predictedDisplayTime;
	fei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
	fei.layerCount = haveLayer ? 1 : 0;
	fei.layers = haveLayer ? layers : NULL;
	xrEndFrame(p->session, &fei);
	return true;
}

//=============================================================================
// Menu quad swapchain: (re)create at the UI surface size.
//=============================================================================
bool OpenXRBackend::EnsureUiSwapchain(int w, int h)
{
	Impl* p = m_impl;
	if (w <= 0 || h <= 0 || p->uiFormat == 0) return false;
	if (p->uiSwapchain != XR_NULL_HANDLE && p->uiW == w && p->uiH == h) return true;

	if (p->uiSwapchain != XR_NULL_HANDLE) { xrDestroySwapchain(p->uiSwapchain); p->uiSwapchain = XR_NULL_HANDLE; }
	p->uiImages.clear();
	if (p->uiStaging) { p->uiStaging->Release(); p->uiStaging = NULL; }

	XrSwapchainCreateInfo ci = { XR_TYPE_SWAPCHAIN_CREATE_INFO };
	ci.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT |
	                XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
	ci.format = p->uiFormat;
	ci.sampleCount = 1;
	ci.width  = w;
	ci.height = h;
	ci.faceCount = 1;
	ci.arraySize = 1;
	ci.mipCount  = 1;
	if (XR_FAILED(xrCreateSwapchain(p->session, &ci, &p->uiSwapchain)))
	{
		XrDbg("OpenXR: UI xrCreateSwapchain failed (%dx%d)\n", w, h);
		return false;
	}

	uint32_t imgCount = 0;
	xrEnumerateSwapchainImages(p->uiSwapchain, 0, &imgCount, NULL);

	if (p->useD3D12)   // #DX12 п.5: D3D12 UI images + an UPLOAD buffer (256-aligned rows) for the 565->RGBA copy
	{
		p->uiImages12.resize(imgCount);
		for (uint32_t i = 0; i < imgCount; ++i) { p->uiImages12[i].type = XR_TYPE_SWAPCHAIN_IMAGE_D3D12_KHR; p->uiImages12[i].next = NULL; }
		xrEnumerateSwapchainImages(p->uiSwapchain, imgCount, &imgCount, (XrSwapchainImageBaseHeader*)p->uiImages12.data());

		if (p->uiUpload12) { p->uiUpload12->Release(); p->uiUpload12 = NULL; }
		p->uiRowPitch12 = (unsigned)(((w * 4) + 255) & ~255);
		D3D12_HEAP_PROPERTIES hpUp; ZeroMemory(&hpUp, sizeof(hpUp)); hpUp.Type = D3D12_HEAP_TYPE_UPLOAD;
		D3D12_RESOURCE_DESC bd; ZeroMemory(&bd, sizeof(bd));
		bd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; bd.Width = (UINT64)p->uiRowPitch12 * h; bd.Height = 1;
		bd.DepthOrArraySize = 1; bd.MipLevels = 1; bd.Format = DXGI_FORMAT_UNKNOWN; bd.SampleDesc.Count = 1; bd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		if (FAILED(p->d3d12Device->CreateCommittedResource(&hpUp, D3D12_HEAP_FLAG_NONE, &bd, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&p->uiUpload12))))
		{ XrDbg("OpenXR: UI upload buffer create failed\n"); return false; }
	}
	else
	{
		p->uiImages.resize(imgCount);
		for (uint32_t i = 0; i < imgCount; ++i) { p->uiImages[i].type = XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR; p->uiImages[i].next = NULL; }
		xrEnumerateSwapchainImages(p->uiSwapchain, imgCount, &imgCount, (XrSwapchainImageBaseHeader*)p->uiImages.data());

		D3D11_TEXTURE2D_DESC td;
		ZeroMemory(&td, sizeof(td));
		td.Width  = w;
		td.Height = h;
		td.MipLevels = 1;
		td.ArraySize = 1;
		td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;   // staging is UNORM; copy-compatible with the _SRGB swapchain
		td.SampleDesc.Count = 1;
		td.Usage = D3D11_USAGE_STAGING;
		td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		if (FAILED(p->device->CreateTexture2D(&td, NULL, &p->uiStaging)))
		{
			XrDbg("OpenXR: UI staging texture create failed\n");
			return false;
		}
	}

	p->uiW = w;
	p->uiH = h;
	XrDbg("OpenXR: menu quad swapchain %dx%d\n", w, h);
	return true;
}

// #DX12 п.5: copy a CPU RGBA8 image (w*h, tightly packed OR the UI's row pitch) into an acquired D3D12 UI
// swapchain image: fill the UPLOAD buffer (256-aligned rows), CopyTextureRegion (COMMON->COPY_DEST->COMMON),
// execute + fence. Returns after the copy is complete (the runtime releases the image right after).
static void XrCopyRgbaToUiImageD3D12(ID3D12Resource* upload, unsigned rowPitch, ID3D12CommandAllocator* alloc,
                                     ID3D12GraphicsCommandList* list, ID3D12CommandQueue* queue, ID3D12Fence* fence,
                                     HANDLE evt, unsigned __int64* fenceVal, ID3D12Resource* dstImg,
                                     const void* rgbaRows, unsigned srcRowPitch, int w, int h)
{
	if (!upload || !dstImg || !list) return;
	unsigned char* mapped = 0; D3D12_RANGE noRead; noRead.Begin = 0; noRead.End = 0;
	if (FAILED(upload->Map(0, &noRead, (void**)&mapped)) || !mapped) return;
	unsigned copyBytes = (unsigned)w * 4;
	for (int y = 0; y < h; ++y)
		memcpy(mapped + (size_t)y * rowPitch, (const unsigned char*)rgbaRows + (size_t)y * srcRowPitch, copyBytes);
	upload->Unmap(0, NULL);

	alloc->Reset();
	list->Reset(alloc, NULL);
	D3D12_RESOURCE_BARRIER b; ZeroMemory(&b, sizeof(b));
	b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	b.Transition.pResource = dstImg; b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	b.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON; b.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	list->ResourceBarrier(1, &b);
	D3D12_TEXTURE_COPY_LOCATION dl; ZeroMemory(&dl, sizeof(dl));
	dl.pResource = dstImg; dl.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX; dl.SubresourceIndex = 0;
	D3D12_TEXTURE_COPY_LOCATION sl; ZeroMemory(&sl, sizeof(sl));
	sl.pResource = upload; sl.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	sl.PlacedFootprint.Offset = 0; sl.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sl.PlacedFootprint.Footprint.Width = (UINT)w; sl.PlacedFootprint.Footprint.Height = (UINT)h; sl.PlacedFootprint.Footprint.Depth = 1;
	sl.PlacedFootprint.Footprint.RowPitch = rowPitch;
	list->CopyTextureRegion(&dl, 0, 0, 0, &sl, NULL);
	D3D12_RESOURCE_BARRIER b2 = b; b2.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST; b2.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
	list->ResourceBarrier(1, &b2);
	list->Close();
	ID3D12CommandList* lists[] = { (ID3D12CommandList*)list };
	queue->ExecuteCommandLists(1, lists);
	(*fenceVal)++; queue->Signal(fence, *fenceVal);
	if (fence->GetCompletedValue() < *fenceVal) { fence->SetEventOnCompletion(*fenceVal, evt); WaitForSingleObject(evt, INFINITE); }
}

// #DX12 п.5 A1: copy a same-size D3D12 texture (the in-scene menu RTT, already rendered on the eye list which has
// executed) straight into an acquired D3D12 UI swapchain image, then execute + fence. srcState is the menu RTT's
// current state (RENDER_TARGET after BindMenuRtt) -- transitioned to COPY_SOURCE and back; the UI image is COMMON
// on acquire (COMMON->COPY_DEST->COMMON, same contract as XrCopyRgbaToUiImageD3D12). Formats are RGBA8 (UNORM vs
// _SRGB are copy-compatible in the same family), sizes match (menu RTT and UI swapchain both DispWidth x DispHeight).
static void XrCopyD3D12TexToUiImage(ID3D12CommandAllocator* alloc, ID3D12GraphicsCommandList* list, ID3D12CommandQueue* queue,
                                    ID3D12Fence* fence, HANDLE evt, unsigned __int64* fenceVal,
                                    ID3D12Resource* srcRes, unsigned srcState, ID3D12Resource* dstImg)
{
	if (!alloc || !list || !queue || !srcRes || !dstImg) return;
	alloc->Reset();
	list->Reset(alloc, NULL);
	D3D12_RESOURCE_BARRIER b[2]; ZeroMemory(b, sizeof(b));
	b[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	b[0].Transition.pResource = srcRes; b[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	b[0].Transition.StateBefore = (D3D12_RESOURCE_STATES)srcState; b[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
	b[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	b[1].Transition.pResource = dstImg; b[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	b[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON; b[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	list->ResourceBarrier(2, b);
	list->CopyResource(dstImg, srcRes);
	b[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE; b[0].Transition.StateAfter = (D3D12_RESOURCE_STATES)srcState;
	b[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;   b[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
	list->ResourceBarrier(2, b);
	list->Close();
	ID3D12CommandList* lists[] = { (ID3D12CommandList*)list };
	queue->ExecuteCommandLists(1, lists);
	(*fenceVal)++; queue->Signal(fence, *fenceVal);
	if (fence->GetCompletedValue() < *fenceVal) { fence->SetEventOnCompletion(*fenceVal, evt); WaitForSingleObject(evt, INFINITE); }
}

//=============================================================================
// Artscout - 2026 (VR menu): the 565 source (g_pXrMenuSurface565 == an ImageBuffer's m_pSysMem) can be
// freed/resized by the UI/sim thread WHILE this main-thread pump reads it -- F4IsBadReadPtr only samples
// one instant, so a race still dangles the pointer MID-LOOP (0xC0000005 in the convert loop). Do the read
// under SEH so a stray access aborts to a clear frame instead of killing the process. POD-only locals ->
// no C++ object unwinding, so __try/__except is legal in this standalone helper.
//=============================================================================
static bool XrConvertMenu565ToRGBA(const void* src565, int srcW, int srcH, void* dstBase, unsigned rowPitch)
{
	__try
	{
		const unsigned short* s = (const unsigned short*)src565;
		for (int y = 0; y < srcH; ++y)
		{
			unsigned char* d = (unsigned char*)dstBase + (size_t)y * rowPitch;
			const unsigned short* srow = s + (size_t)y * srcW;
			for (int x = 0; x < srcW; ++x)
			{
				unsigned short px = srow[x];
				unsigned r = (px >> 11) & 0x1F, g = (px >> 5) & 0x3F, b = px & 0x1F;
				unsigned char* o = d + (size_t)x * 4;
				o[0] = (unsigned char)((r * 255 + 15) / 31);
				o[1] = (unsigned char)((g * 255 + 31) / 63);
				o[2] = (unsigned char)((b * 255 + 15) / 31);
				o[3] = 255;
			}
		}
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;   // source went away mid-read -> caller drops the panel for this frame
	}
}

//=============================================================================
// RunMenuFrame -- present the flat 2D UI as a head-locked quad panel.
//=============================================================================
bool OpenXRBackend::RunMenuFrame(const void* src565, int srcW, int srcH)
{
	Impl* p = m_impl;
	if (!p->instance || p->session == XR_NULL_HANDLE) return false;

	// #DX12 п.5: D3D12 VR menu panel -- convert the 565 UI into an RGBA buffer (+ cursor crosshair), copy it
	// into the D3D12 UI swapchain image, and submit it as a world-fixed quad. Self-contained (the D3D11 path
	// below is untouched). Same panel placement / cursor as D3D11.
	if (p->useD3D12)
	{
		PollEvents();
		if (!p->sessionRunning) return false;
		if (!EnsureUiSwapchain(srcW, srcH)) return RunFrame(NULL, NULL);

		XrFrameWaitInfo fwi = { XR_TYPE_FRAME_WAIT_INFO };
		XrFrameState fs = { XR_TYPE_FRAME_STATE };
		if (XR_FAILED(xrWaitFrame(p->session, &fwi, &fs))) return false;
		XrFrameBeginInfo fbi = { XR_TYPE_FRAME_BEGIN_INFO };
		if (XR_FAILED(xrBeginFrame(p->session, &fbi))) return false;

		XrCompositionLayerQuad quad = { XR_TYPE_COMPOSITION_LAYER_QUAD };
		bool haveLayer = false;
		(void)src565;
		const unsigned char* srcSnap = XrMenuSnapshot(srcW, srcH);
		if (fs.shouldRender && srcSnap && srcW > 0 && srcH > 0)
		{
			unsigned char* rgba = (unsigned char*)malloc((size_t)srcW * srcH * 4);
			if (rgba && XrConvertMenu565ToRGBA(srcSnap, srcW, srcH, rgba, (unsigned)srcW * 4))
			{
				// VR cursor crosshair at the real pointer, mapped window-client -> panel pixels.
				HWND hwnd = g_pD3D12Backend ? g_pD3D12Backend->Hwnd() : NULL;
				POINT pt; RECT rc;
				if (hwnd && GetCursorPos(&pt) && GetClientRect(hwnd, &rc))
				{
					ScreenToClient(hwnd, &pt);
					const int cw = rc.right - rc.left, ch = rc.bottom - rc.top;
					if (cw > 0 && ch > 0)
					{
						const int cx = (int)((long long)pt.x * srcW / cw), cy = (int)((long long)pt.y * srcH / ch), arm = 9;
						#define XR12_SETPX(X,Y,V) do { int _x=(X),_y=(Y); if(_x>=0&&_x<srcW&&_y>=0&&_y<srcH){ unsigned char* _o=rgba+((size_t)_y*srcW+_x)*4; _o[0]=_o[1]=_o[2]=(unsigned char)(V); _o[3]=255; } } while(0)
						for (int d = -arm; d <= arm; ++d) { XR12_SETPX(cx+d, cy-1, 0); XR12_SETPX(cx+d, cy+1, 0); XR12_SETPX(cx-1, cy+d, 0); XR12_SETPX(cx+1, cy+d, 0); }
						for (int d = -arm; d <= arm; ++d) { XR12_SETPX(cx+d, cy, 255); XR12_SETPX(cx, cy+d, 255); }
						#undef XR12_SETPX
					}
				}

				uint32_t idx = 0;
				XrSwapchainImageAcquireInfo ai = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
				if (XR_SUCCEEDED(xrAcquireSwapchainImage(p->uiSwapchain, &ai, &idx)))
				{
					XrSwapchainImageWaitInfo wi = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO }; wi.timeout = XR_INFINITE_DURATION;
					if (XR_SUCCEEDED(xrWaitSwapchainImage(p->uiSwapchain, &wi)) && idx < p->uiImages12.size())
						XrCopyRgbaToUiImageD3D12(p->uiUpload12, p->uiRowPitch12, p->alloc12, p->list12, p->d3d12Queue,
						                         p->fence12, p->fenceEvt12, &p->fenceVal12, p->uiImages12[idx].texture, rgba, (unsigned)srcW * 4, srcW, srcH);
					XrSwapchainImageReleaseInfo ri = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
					xrReleaseSwapchainImage(p->uiSwapchain, &ri);

					const float aspect = (float)srcW / (float)srcH, heightM = 1.3f, distM = 2.1f;
					quad.layerFlags = 0; quad.space = p->appSpace; quad.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
					quad.subImage.swapchain = p->uiSwapchain;
					quad.subImage.imageRect.offset.x = 0; quad.subImage.imageRect.offset.y = 0;
					quad.subImage.imageRect.extent.width = srcW; quad.subImage.imageRect.extent.height = srcH;
					quad.subImage.imageArrayIndex = 0;
					quad.pose.orientation.x = quad.pose.orientation.y = quad.pose.orientation.z = 0.0f; quad.pose.orientation.w = 1.0f;
					quad.pose.position.x = 0.0f; quad.pose.position.y = 0.0f; quad.pose.position.z = -distM;
					quad.size.width = heightM * aspect; quad.size.height = heightM;
					haveLayer = true;
				}
			}
			if (rgba) free(rgba);
		}

		XrCompositionLayerBaseHeader* layers[1] = { (XrCompositionLayerBaseHeader*)&quad };
		XrFrameEndInfo fei = { XR_TYPE_FRAME_END_INFO };
		fei.displayTime = fs.predictedDisplayTime; fei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
		fei.layerCount = haveLayer ? 1 : 0; fei.layers = haveLayer ? layers : NULL;
		xrEndFrame(p->session, &fei);
		return true;
	}

	PollEvents();
	if (!p->sessionRunning) return false;

	// If a panel can't be created (no R8G8B8A8 format), keep timing alive with a clear.
	if (!EnsureUiSwapchain(srcW, srcH))
	{
		return RunFrame(NULL, NULL);
	}

	XrFrameWaitInfo fwi = { XR_TYPE_FRAME_WAIT_INFO };
	XrFrameState fs = { XR_TYPE_FRAME_STATE };
	if (XR_FAILED(xrWaitFrame(p->session, &fwi, &fs))) return false;
	XrFrameBeginInfo fbi = { XR_TYPE_FRAME_BEGIN_INFO };
	if (XR_FAILED(xrBeginFrame(p->session, &fbi))) return false;

	XrCompositionLayerQuad quad = { XR_TYPE_COMPOSITION_LAYER_QUAD };
	bool haveLayer = false;

	// Artscout - 2026: read a STABLE pump-local snapshot of the menu surface (the producer copied it under
	// the lock in OpenXR_CacheMenuSurface), NOT the live ImageBuffer m_pSysMem -- so the convert can never
	// touch a freed/resized buffer (was the 0xC0000005 race). src565 is ignored in favour of the snapshot.
	(void)src565;
	const unsigned char* srcSnap = XrMenuSnapshot(srcW, srcH);
	const bool srcReadable = (srcSnap != NULL);

	if (fs.shouldRender && srcReadable)
	{
		// 565 -> RGBA8 into the staging texture (same conversion as BlitBitmap565).
		D3D11_MAPPED_SUBRESOURCE mr;
		if (SUCCEEDED(p->ctx->Map(p->uiStaging, 0, D3D11_MAP_WRITE, 0, &mr)))
		{
			// SEH-guarded 565->RGBA8 read (the source can be freed by another thread mid-loop).
			const bool convOk = XrConvertMenu565ToRGBA(srcSnap, srcW, srcH, mr.pData, mr.RowPitch);
			if (!convOk)
			{
				p->ctx->Unmap(p->uiStaging, 0);
			}
			else
			{

			// VR cursor: the flat UI uses the OS hardware cursor (not in m_pSysMem), so
			// it would be invisible on the panel. Draw a crosshair (white core + black
			// outline) at the real pointer position, mapped from window-client pixels to
			// panel pixels. Mouse clicks still land via the desktop window at that spot.
			if (g_pD3D12Backend && g_pD3D12Backend->Hwnd())   // Artscout - 2026 (D3D11 purge): HWND from the D3D12 backend
			{
				POINT pt;
				HWND hwnd = g_pD3D12Backend->Hwnd();
				RECT rc;
				if (GetCursorPos(&pt) && GetClientRect(hwnd, &rc))
				{
					ScreenToClient(hwnd, &pt);
					const int cw = rc.right - rc.left, ch = rc.bottom - rc.top;
					if (cw > 0 && ch > 0)
					{
						const int cx = (int)((long long)pt.x * srcW / cw);
						const int cy = (int)((long long)pt.y * srcH / ch);
						unsigned char* base = (unsigned char*)mr.pData;
						const int arm = 9;
						#define XR_SETPX(X,Y,V) do { int _x=(X),_y=(Y); \
							if (_x>=0 && _x<srcW && _y>=0 && _y<srcH) { \
								unsigned char* _o = base + (size_t)_y*mr.RowPitch + (size_t)_x*4; \
								_o[0]=_o[1]=_o[2]=(unsigned char)(V); _o[3]=255; } } while(0)
						for (int d = -arm; d <= arm; ++d)   // black outline first
						{
							XR_SETPX(cx+d, cy-1, 0); XR_SETPX(cx+d, cy+1, 0);
							XR_SETPX(cx-1, cy+d, 0); XR_SETPX(cx+1, cy+d, 0);
						}
						for (int d = -arm; d <= arm; ++d)   // white core on top
						{
							XR_SETPX(cx+d, cy, 255);
							XR_SETPX(cx, cy+d, 255);
						}
						#undef XR_SETPX
					}
				}
			}

			p->ctx->Unmap(p->uiStaging, 0);

			uint32_t idx = 0;
			XrSwapchainImageAcquireInfo ai = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
			if (XR_SUCCEEDED(xrAcquireSwapchainImage(p->uiSwapchain, &ai, &idx)))
			{
				XrSwapchainImageWaitInfo wi = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
				wi.timeout = XR_INFINITE_DURATION;
				if (XR_SUCCEEDED(xrWaitSwapchainImage(p->uiSwapchain, &wi)))
					p->ctx->CopyResource(p->uiImages[idx].texture, p->uiStaging);
				XrSwapchainImageReleaseInfo ri = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
				xrReleaseSwapchainImage(p->uiSwapchain, &ri);

				// World-fixed panel: placed in the app (LOCAL/recenter) space in front of
				// the headset's recenter origin, so it stays put when you look around
				// (does NOT follow the gaze, like the DCS 2D menu in VR).
				const float aspect  = (float)srcW / (float)srcH;
				const float heightM = 1.3f;   // panel height in meters
				const float distM   = 2.1f;   // distance forward (-Z); ~30cm further than before
				quad.layerFlags = 0;
				quad.space = p->appSpace;
				quad.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
				quad.subImage.swapchain = p->uiSwapchain;
				quad.subImage.imageRect.offset.x = 0;
				quad.subImage.imageRect.offset.y = 0;
				quad.subImage.imageRect.extent.width  = srcW;
				quad.subImage.imageRect.extent.height = srcH;
				quad.subImage.imageArrayIndex = 0;
				quad.pose.orientation.x = quad.pose.orientation.y = quad.pose.orientation.z = 0.0f;
				quad.pose.orientation.w = 1.0f;
				quad.pose.position.x = 0.0f;
				quad.pose.position.y = 0.0f;
				quad.pose.position.z = -distM;
				quad.size.width  = heightM * aspect;
				quad.size.height = heightM;
				haveLayer = true;
			}
			}   // Artscout - 2026: close the convOk 'else' (SEH-guarded 565 read succeeded)
		}
	}


	XrCompositionLayerBaseHeader* layers[1] = { (XrCompositionLayerBaseHeader*)&quad };
	XrFrameEndInfo fei = { XR_TYPE_FRAME_END_INFO };
	fei.displayTime = fs.predictedDisplayTime;
	fei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
	fei.layerCount = haveLayer ? 1 : 0;
	fei.layers = haveLayer ? layers : NULL;
	xrEndFrame(p->session, &fei);
	return true;
}

//=============================================================================
// Per-eye stereo frame (the 3D scene is rendered once per eye)
//=============================================================================
bool  OpenXRBackend::StereoActive() const { return m_impl->inStereoFrame; }
void  OpenXRBackend::SetCurrentEye(int eye) { m_impl->currentEye = eye; }
int   OpenXRBackend::CurrentEye() const { return m_impl->currentEye; }
int   OpenXRBackend::CurEyeW() const { int e = m_impl->currentEye; return (e >= 0 && e < (int)m_impl->swapchains.size()) ? m_impl->swapchains[e].width  : 0; }
int   OpenXRBackend::CurEyeH() const { int e = m_impl->currentEye; return (e >= 0 && e < (int)m_impl->swapchains.size()) ? m_impl->swapchains[e].height : 0; }
float OpenXRBackend::GetEyeLateralOffsetFeet(int eye) const
{
	if (eye < 0 || eye >= 8) return 0.0f;
	return m_impl->eyeLatFeet[eye];
}

// Artscout - 2026: per-eye fov half-angles (radians) from the runtime's located views. Used to set
// the engine's render FOV to the headset's actual per-eye FOV (so the world fills the lenses and the
// rendered image matches the submitted projection -> no double vision).
bool OpenXRBackend::GetEyeFovAngles(int eye, float* l, float* r, float* u, float* d) const
{
	Impl* p = m_impl;
	if (eye < 0 || eye >= (int)p->views.size()) return false;
	if (l) *l = p->views[eye].fov.angleLeft;
	if (r) *r = p->views[eye].fov.angleRight;
	if (u) *u = p->views[eye].fov.angleUp;
	if (d) *d = p->views[eye].fov.angleDown;
	return true;
}

void OpenXRBackend::SetSubmitFov(float h, float v)
{
	m_impl->submitFov.angleLeft  = -h * 0.5f;
	m_impl->submitFov.angleRight =  h * 0.5f;
	m_impl->submitFov.angleUp    =  v * 0.5f;
	m_impl->submitFov.angleDown  = -v * 0.5f;
	m_impl->haveSubmitFov = true;
}

// Returns: -1 = session not running (don't drive XR; render mono);
//           0 = frame begun but shouldRender false (render nothing; EndStereoFrame still required);
//           n = render n eyes.
int OpenXRBackend::BeginStereoFrame()
{
	Impl* p = m_impl;
	if (!p->instance || p->session == XR_NULL_HANDLE) return -1;
	PollEvents();
	if (!p->sessionRunning) return -1;

	XrFrameWaitInfo fwi = { XR_TYPE_FRAME_WAIT_INFO };
	p->stereoFrameState.type = XR_TYPE_FRAME_STATE; p->stereoFrameState.next = NULL;
	if (XR_FAILED(xrWaitFrame(p->session, &fwi, &p->stereoFrameState))) return -1;
	XrFrameBeginInfo fbi = { XR_TYPE_FRAME_BEGIN_INFO };
	if (XR_FAILED(xrBeginFrame(p->session, &fbi))) return -1;
	p->inStereoFrame = true;
	p->projViews.clear();
	p->menuQuadPending = false;   // Artscout - 2026 (#59): no stale menu quad carried into a new frame

	// Artscout - 2026 (#67): apply a pending recenter here -- the render thread owns appSpace and we have a
	// fresh predicted time, so this is safe (no destroy-while-in-use). Rebuild the LOCAL app space at the
	// current head yaw + position; pitch/roll dropped (level horizon). Done BEFORE the view locate below so
	// this very frame renders recentered.
	if (p->recenterPending)
	{
		p->recenterPending = false;
		XrSpaceLocation rloc = { XR_TYPE_SPACE_LOCATION };
		// Locate the head in the PRISTINE LOCAL frame (localRef), NOT the current appSpace. The new space is
		// built as LOCAL + off, so off must be the head pose in LOCAL. Measuring against the already-recentered
		// appSpace made off ~0 on the 2nd press -> newSpace ~= LOCAL -> the view snapped back to the un-recentered
		// origin (the "second press undoes it" toggle). With localRef every press is an absolute recenter
		// (height + depth + yaw) to wherever the head currently is.
		if (XR_SUCCEEDED(xrLocateSpace(p->viewSpace, p->localRef, p->stereoFrameState.predictedDisplayTime, &rloc)) &&
		    (rloc.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) &&
		    (rloc.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT))
		{
			XrPosef off;
			off.position = rloc.pose.position;   // origin under the head -> centers + resets eye height
			float qy = rloc.pose.orientation.y, qw = rloc.pose.orientation.w;
			float nrm = sqrtf(qy * qy + qw * qw);   // yaw-only (swing-twist about Y, OpenXR Y-up)
			if (nrm < 1e-6f) { off.orientation.x = off.orientation.y = off.orientation.z = 0.0f; off.orientation.w = 1.0f; }
			else             { off.orientation.x = 0.0f; off.orientation.y = qy / nrm; off.orientation.z = 0.0f; off.orientation.w = qw / nrm; }

			XrReferenceSpaceCreateInfo rsci = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
			rsci.referenceSpaceType   = XR_REFERENCE_SPACE_TYPE_LOCAL;
			rsci.poseInReferenceSpace = off;
			XrSpace newSpace = XR_NULL_HANDLE;
			if (XR_SUCCEEDED(xrCreateReferenceSpace(p->session, &rsci, &newSpace)) && newSpace != XR_NULL_HANDLE)
			{
				xrDestroySpace(p->appSpace);
				p->appSpace = newSpace;
				XrDbg("OpenXR: recenter applied (pos %.2f %.2f %.2f)\n", off.position.x, off.position.y, off.position.z);
			}
		}
	}

	// Artscout - 2026 (VR controllers, Phase 1): sync controller input for this frame (poses + buttons).
	SyncControllers();

	if (!p->stereoFrameState.shouldRender) return 0;

	XrViewState vs = { XR_TYPE_VIEW_STATE };
	XrViewLocateInfo vli = { XR_TYPE_VIEW_LOCATE_INFO };
	vli.viewConfigurationType = p->viewConfigType;
	vli.displayTime = p->stereoFrameState.predictedDisplayTime;
	vli.space = p->appSpace;
	uint32_t out = 0;
	if (XR_FAILED(xrLocateViews(p->session, &vli, &vs, (uint32_t)p->views.size(), &out, p->views.data()))
	    || !(vs.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT))
		return 0;

	const int n = (int)out;

	// Head pose -> look angles for the cockpit camera.
	XrSpaceLocation loc = { XR_TYPE_SPACE_LOCATION };
	if (XR_SUCCEEDED(xrLocateSpace(p->viewSpace, p->appSpace, p->stereoFrameState.predictedDisplayTime, &loc)) &&
	    (loc.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT))
	{
		p->lastHeadPose = loc.pose; p->haveHeadPose = true;
		XrQuatToYawPitchRoll(loc.pose.orientation, p->lastYaw, p->lastPitch, p->lastRoll);
		p->haveHeadAngles = true;
	}

	// Per-eye lateral offset (IPD), meters -> feet, relative to the head centre.
	float cx = 0.0f;
	for (int e = 0; e < n; ++e) cx += p->views[e].pose.position.x;
	if (n) cx /= (float)n;
	for (int e = 0; e < n && e < 8; ++e)
		p->eyeLatFeet[e] = (p->views[e].pose.position.x - cx) * 3.28084f;

	p->projViews.resize(n);
	p->eyeAcquired.assign(n, false);   // deferred release: track per-frame acquires
	return n;
}

bool OpenXRBackend::BeginEye(int eye, void** outRtv, int* outW, int* outH)
{
	Impl* p = m_impl;
	if (eye < 0 || eye >= (int)p->swapchains.size())
		return false;
	Impl::Swapchain& sc = p->swapchains[eye];

	uint32_t idx = 0;
	XrSwapchainImageAcquireInfo ai = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
	XrResult ar = xrAcquireSwapchainImage(sc.handle, &ai, &idx);
	if (XR_FAILED(ar)) return false;
	XrSwapchainImageWaitInfo wi = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
	wi.timeout = XR_INFINITE_DURATION;
	if (XR_FAILED(xrWaitSwapchainImage(sc.handle, &wi)))
	{
		XrSwapchainImageReleaseInfo ri = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
		xrReleaseSwapchainImage(sc.handle, &ri);
		return false;
	}

	if ((int)p->eyeImgIndex.size() < (int)p->swapchains.size())
		p->eyeImgIndex.resize(p->swapchains.size(), 0);
	p->eyeImgIndex[eye] = idx;
	if ((int)p->eyeAcquired.size() <= eye) p->eyeAcquired.resize(eye + 1, false);
	p->eyeAcquired[eye] = true;   // deferred release: released by ReleaseEyes() after both eyes render

	if (p->useD3D12)
	{
		// #DX12 п.5: open a D3D12 command list rendering INTO this eye's image (bind eye RTV + VR depth, clear).
		// The engine then draws the scene per eye via g_pRenderer straight into the eye image. EndEye executes it.
		if (idx < sc.images12.size())
			g_pD3D12Backend->BeginEyeFrame(sc.images12[idx].texture, sc.rtvs12[idx], sc.width, sc.height);
		if (outRtv) *outRtv = (void*)(SIZE_T)sc.rtvs12[idx];
	}
	else
	{
		if (outRtv) *outRtv = sc.rtvs[idx];
	}
	if (outW)   *outW = sc.width;
	if (outH)   *outH = sc.height;
	return true;
}

void OpenXRBackend::EndEye(int eye)
{
	Impl* p = m_impl;
	if (eye < 0 || eye >= (int)p->swapchains.size()) return;
	Impl::Swapchain& sc = p->swapchains[eye];

	// #DX12 п.5: execute the eye's command list (fill + flush the image) BEFORE the runtime releases it.
	if (p->useD3D12)
	{
		int idx = (eye < (int)p->eyeImgIndex.size()) ? (int)p->eyeImgIndex[eye] : -1;
		if (idx >= 0 && idx < (int)sc.images12.size())
			g_pD3D12Backend->EndEyeFrame(sc.images12[idx].texture);
	}

	// Artscout - 2026: release THIS eye's image immediately (matches the proven-working clear-only
	// path DiagClearEyesAndEnd, which releases each eye before acquiring the next). Deferred release
	// (ReleaseEyes after both eyes) is now a no-op since eyeAcquired is cleared here.
	{
		XrSwapchainImageReleaseInfo ri = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
		xrReleaseSwapchainImage(sc.handle, &ri);
		if (eye < (int)p->eyeAcquired.size()) p->eyeAcquired[eye] = false;
	}

	if (eye < (int)p->projViews.size())
	{
		XrCompositionLayerProjectionView& pv = p->projViews[eye];
		pv.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW; pv.next = NULL;
		pv.pose = p->views[eye].pose;
		// Artscout - 2026: submit the fov the engine actually rendered with. STEREO renders symmetric
		// (SetFOV) and submits the symmetric submitFov so the two eyes fuse. QUAD-VIEWS renders each view
		// with its TRUE off-axis (gaze) fov (SetVRFrustum(views[eye].fov)) -> it MUST submit that same
		// per-view fov so the foveated compositor places the focus inset where it was drawn. haveSubmitFov
		// is set only by the stereo path and is NEVER reset, so a stereo/menu frame before quad engaged
		// would leave it true and (wrongly) submit the focus view SYMMETRIC while it was rendered off-axis
		// -> the compositor shifts the focus content = reads as symbology "drift". So gate on the ACTUAL
		// view config, not just the stale flag.
		const bool quadCfg = (p->views.size() > 2);
		pv.fov  = (p->haveSubmitFov && !quadCfg) ? p->submitFov : p->views[eye].fov;
		pv.subImage.swapchain = sc.handle;
		pv.subImage.imageRect.offset.x = 0;
		pv.subImage.imageRect.offset.y = 0;
		pv.subImage.imageRect.extent.width  = sc.width;
		pv.subImage.imageRect.extent.height = sc.height;
		pv.subImage.imageArrayIndex = 0;
	}
}

// Artscout - 2026: TEMP DIAG -- replicate the M1 clear path INSIDE an already-begun stereo frame
// (sim thread): for each eye acquire/wait/clear(distinct color)/release, fill projViews, xrEndFrame.
// No engine rendering between eyes. If both eyes show their color -> two separate swapchains compose
// fine on this runtime/thread and the 2nd-eye-black is caused by the engine render between eyes.
// If the 2nd eye is black -> the second swapchain genuinely isn't composited (-> texture-array path).
void OpenXRBackend::DiagClearEyesAndEnd()
{
	Impl* p = m_impl;
	if (!p->inStereoFrame) return;
	const int n = (int)p->projViews.size();
	for (int e = 0; e < n && e < (int)p->swapchains.size(); ++e)
	{
		Impl::Swapchain& sc = p->swapchains[e];
		uint32_t idx = 0;
		XrSwapchainImageAcquireInfo ai = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
		if (XR_FAILED(xrAcquireSwapchainImage(sc.handle, &ai, &idx))) continue;
		XrSwapchainImageWaitInfo wi = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
		wi.timeout = XR_INFINITE_DURATION;
		if (XR_FAILED(xrWaitSwapchainImage(sc.handle, &wi)))
		{
			XrSwapchainImageReleaseInfo ri = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
			xrReleaseSwapchainImage(sc.handle, &ri);
			continue;
		}
		const float red[4]   = { 0.8f, 0.0f, 0.0f, 1.0f };
		const float green[4] = { 0.0f, 0.8f, 0.0f, 1.0f };
		if (p->useD3D12)
		{
			if (idx < sc.images12.size())
				XrClearRtvD3D12(p->alloc12, p->list12, p->d3d12Queue, p->fence12, p->fenceEvt12, &p->fenceVal12,
				                sc.images12[idx].texture, sc.rtvs12[idx], (e == 0) ? red : green);
		}
		else
		{
			p->ctx->ClearRenderTargetView(sc.rtvs[idx], (e == 0) ? red : green);
			p->ctx->Flush();
		}
		XrSwapchainImageReleaseInfo ri = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
		xrReleaseSwapchainImage(sc.handle, &ri);

		XrCompositionLayerProjectionView& pv = p->projViews[e];
		pv.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW; pv.next = NULL;
		pv.pose = p->views[e].pose;
		pv.fov  = p->views[e].fov;
		pv.subImage.swapchain = sc.handle;
		pv.subImage.imageRect.offset.x = 0;
		pv.subImage.imageRect.offset.y = 0;
		pv.subImage.imageRect.extent.width  = sc.width;
		pv.subImage.imageRect.extent.height = sc.height;
		pv.subImage.imageArrayIndex = 0;
	}

	XrCompositionLayerProjection layer = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
	layer.space = p->appSpace;
	layer.viewCount = (uint32_t)p->projViews.size();
	layer.views = p->projViews.data();
	XrCompositionLayerBaseHeader* layers[1] = { (XrCompositionLayerBaseHeader*)&layer };
	XrFrameEndInfo fei = { XR_TYPE_FRAME_END_INFO };
	fei.displayTime = p->stereoFrameState.predictedDisplayTime;
	fei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
	fei.layerCount = 1;
	fei.layers = layers;
	xrEndFrame(p->session, &fei);
	p->inStereoFrame = false;
	p->currentEye = -1;
}

// Artscout - 2026: release all eye swapchain images acquired this frame (deferred release).
// Call after BOTH eyes have rendered, before EndStereoFrame()/xrEndFrame.
void OpenXRBackend::ReleaseEyes()
{
	Impl* p = m_impl;
	for (int e = 0; e < (int)p->eyeAcquired.size() && e < (int)p->swapchains.size(); ++e)
	{
		if (!p->eyeAcquired[e]) continue;
		XrSwapchainImageReleaseInfo ri = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
		xrReleaseSwapchainImage(p->swapchains[e].handle, &ri);
		p->eyeAcquired[e] = false;
	}
}

// Artscout - 2026 (#59 VR menu): copy the drawn menu texture into the UI swapchain and stage a head-locked
// quad (VIEW space) to be composited on top of the projection layer by EndStereoFrame. Called once per stereo
// frame (sim thread) only while a comms/exit menu is up. Same machinery as RunMenuFrame's pre-3D quad, but in
// VIEW space (follows the head) and with source-alpha blend so only the menu pixels show.
bool OpenXRBackend::SubmitInSceneMenuQuad(void* menuTex, int w, int h)
{
	Impl* p = m_impl;
	if (!p->inStereoFrame || !menuTex || w <= 0 || h <= 0) return false;
	if (p->viewSpace == XR_NULL_HANDLE) return false;
	if (!EnsureUiSwapchain(w, h)) return false;

	extern float g_fVrMenuDist, g_fVrMenuHeight;

	// #DX12 п.5 A1: D3D12 in-scene menu quad. menuTex is a D3D12Texture* (the backend's menu RTT). It is drawn on
	// the EYE command list, which has NOT executed yet at this point -- so we CANNOT copy it into the UI image now.
	// Stage it + the quad here; EndStereoFrame does the actual UI-image copy AFTER the eye frame(s) execute+fence.
	if (p->useD3D12)
	{
		p->menuTexD3D12 = menuTex;

		const float aspect  = (float)w / (float)h;
		const float heightM = (g_fVrMenuHeight > 0.1f) ? g_fVrMenuHeight : 1.4f;
		const float distM   = (g_fVrMenuDist   > 0.1f) ? g_fVrMenuDist   : 1.8f;
		XrCompositionLayerQuad& q = p->menuQuad;
		memset(&q, 0, sizeof(q));
		q.type = XR_TYPE_COMPOSITION_LAYER_QUAD;
		q.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
		q.space = p->viewSpace;                  // head-locked (in front of the head, independent of gaze/quad-views)
		q.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
		q.subImage.swapchain = p->uiSwapchain;
		q.subImage.imageRect.extent.width  = w;
		q.subImage.imageRect.extent.height = h;
		q.subImage.imageArrayIndex = 0;
		q.pose.orientation.w = 1.0f;
		q.pose.position.z = -distM;
		q.size.width  = heightM * aspect;
		q.size.height = heightM;
		p->menuQuadPending = true;
		return true;
	}

	ID3D11Texture2D* src = (ID3D11Texture2D*)menuTex;

	uint32_t idx = 0;
	XrSwapchainImageAcquireInfo ai = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
	if (XR_FAILED(xrAcquireSwapchainImage(p->uiSwapchain, &ai, &idx))) return false;
	XrSwapchainImageWaitInfo wi = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
	wi.timeout = XR_INFINITE_DURATION;
	if (XR_SUCCEEDED(xrWaitSwapchainImage(p->uiSwapchain, &wi)))
		p->ctx->CopyResource(p->uiImages[idx].texture, src);
	XrSwapchainImageReleaseInfo ri = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
	xrReleaseSwapchainImage(p->uiSwapchain, &ri);

	extern float g_fVrMenuDist, g_fVrMenuHeight;
	const float aspect  = (float)w / (float)h;
	const float heightM = (g_fVrMenuHeight > 0.1f) ? g_fVrMenuHeight : 1.4f;
	const float distM   = (g_fVrMenuDist   > 0.1f) ? g_fVrMenuDist   : 1.8f;

	XrCompositionLayerQuad& q = p->menuQuad;
	memset(&q, 0, sizeof(q));
	q.type = XR_TYPE_COMPOSITION_LAYER_QUAD;
	q.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;  // menu alpha keys out the empty canvas
	q.space = p->viewSpace;                  // HEAD-LOCKED: stays in front of the head, independent of gaze/quad-views
	q.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
	q.subImage.swapchain = p->uiSwapchain;
	q.subImage.imageRect.extent.width  = w;
	q.subImage.imageRect.extent.height = h;
	q.subImage.imageArrayIndex = 0;
	q.pose.orientation.w = 1.0f;
	q.pose.position.z = -distM;
	q.size.width  = heightM * aspect;
	q.size.height = heightM;

	p->menuQuadPending = true;
	return true;
}

void OpenXRBackend::EndStereoFrame()
{
	Impl* p = m_impl;
	if (!p->inStereoFrame) return;

	// #DX12 п.5 A1: now that the eye command list(s) have executed + fenced (EndEyeFrame), the in-scene menu RTT is
	// fully rendered -> copy it into a UI swapchain image and finalize the head-locked quad staged by
	// SubmitInSceneMenuQuad. Deferred to here precisely because the RTT was drawn on the (then-unexecuted) eye list.
	if (p->useD3D12 && p->menuQuadPending && p->menuTexD3D12 && p->uiSwapchain != XR_NULL_HANDLE)
	{
		D3D12Texture* mt = (D3D12Texture*)p->menuTexD3D12;
		if (mt && mt->tex)
		{
			uint32_t idx = 0;
			XrSwapchainImageAcquireInfo ai = { XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
			if (XR_SUCCEEDED(xrAcquireSwapchainImage(p->uiSwapchain, &ai, &idx)))
			{
				XrSwapchainImageWaitInfo wi = { XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO }; wi.timeout = XR_INFINITE_DURATION;
				if (XR_SUCCEEDED(xrWaitSwapchainImage(p->uiSwapchain, &wi)) && idx < p->uiImages12.size())
					XrCopyD3D12TexToUiImage(p->alloc12, p->list12, p->d3d12Queue, p->fence12, p->fenceEvt12, &p->fenceVal12,
					                        mt->tex, mt->rtState, p->uiImages12[idx].texture);
				XrSwapchainImageReleaseInfo ri = { XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
				xrReleaseSwapchainImage(p->uiSwapchain, &ri);
			}
		}
		p->menuTexD3D12 = NULL;
	}

	XrCompositionLayerProjection layer = { XR_TYPE_COMPOSITION_LAYER_PROJECTION };
	const bool haveLayer = !p->projViews.empty();
	if (haveLayer)
	{
		layer.space = p->appSpace;
		layer.viewCount = (uint32_t)p->projViews.size();
		layer.views = p->projViews.data();
	}
	// Artscout - 2026 (#59 VR menu): projection layer first, then the head-locked menu quad ON TOP (if staged).
	XrCompositionLayerBaseHeader* layers[2];
	uint32_t nLayers = 0;
	if (haveLayer)          layers[nLayers++] = (XrCompositionLayerBaseHeader*)&layer;
	if (p->menuQuadPending) layers[nLayers++] = (XrCompositionLayerBaseHeader*)&p->menuQuad;

	XrFrameEndInfo fei = { XR_TYPE_FRAME_END_INFO };
	fei.displayTime = p->stereoFrameState.predictedDisplayTime;
	fei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
	fei.layerCount = nLayers;
	fei.layers = nLayers ? layers : NULL;
	xrEndFrame(p->session, &fei);

	p->inStereoFrame = false;
	p->currentEye = -1;
	p->menuQuadPending = false;   // consumed this frame
}

//=============================================================================
// Head pose accessor
//=============================================================================
bool OpenXRBackend::GetHeadPose(float outPos[3], float outQuat[4]) const
{
	if (!m_impl->haveHeadPose) return false;
	const XrPosef& h = m_impl->lastHeadPose;
	outPos[0] = h.position.x; outPos[1] = h.position.y; outPos[2] = h.position.z;
	outQuat[0] = h.orientation.x; outQuat[1] = h.orientation.y;
	outQuat[2] = h.orientation.z; outQuat[3] = h.orientation.w;
	return true;
}

bool OpenXRBackend::GetHeadYawPitchRoll(float* yaw, float* pitch, float* roll) const
{
	if (!m_impl->haveHeadAngles) return false;
	if (yaw)   *yaw   = m_impl->lastYaw;
	if (pitch) *pitch = m_impl->lastPitch;
	if (roll)  *roll  = m_impl->lastRoll;
	return true;
}

//----------------------------------------------------------------------------
// Artscout - 2026 (VR controllers, Phase 1): expose the latest per-hand input snapshot.
//----------------------------------------------------------------------------
int OpenXRBackend::GetActiveHand() const { return m_impl ? m_impl->activeHand : 1; }

bool OpenXRBackend::GetControllerState(int hand, ControllerState* out) const
{
	if (!m_impl || !out || hand < 0 || hand > 1) return false;
	const Impl::HandInput& hi = m_impl->hand[hand];
	out->aimValid = hi.aimValid;
	out->aimPos[0] = hi.aimPose.position.x; out->aimPos[1] = hi.aimPose.position.y; out->aimPos[2] = hi.aimPose.position.z;
	out->aimQuat[0] = hi.aimPose.orientation.x; out->aimQuat[1] = hi.aimPose.orientation.y;
	out->aimQuat[2] = hi.aimPose.orientation.z; out->aimQuat[3] = hi.aimPose.orientation.w;
	out->gripValid = hi.gripValid;
	out->gripPos[0] = hi.gripPose.position.x; out->gripPos[1] = hi.gripPose.position.y; out->gripPos[2] = hi.gripPose.position.z;
	out->gripQuat[0] = hi.gripPose.orientation.x; out->gripQuat[1] = hi.gripPose.orientation.y;
	out->gripQuat[2] = hi.gripPose.orientation.z; out->gripQuat[3] = hi.gripPose.orientation.w;
	out->trigger = hi.trigger; out->triggerDown = hi.triggerDown;
	out->squeeze = hi.squeeze; out->squeezeDown = hi.squeezeDown;
	out->thumbX = hi.thumbX;   out->thumbY = hi.thumbY;
	out->buttonA = hi.aBtn;    out->buttonB = hi.bBtn;
	return true;
}

bool OpenXRBackend::ControllerActive() const
{
	return m_impl && m_impl->inputReady && m_impl->hand[m_impl->activeHand].aimValid;
}

bool OpenXRBackend::GetControllerAimBody(int hand, float origin[3], float dir[3]) const
{
	if (!m_impl || hand < 0 || hand > 1) return false;
	const Impl::HandInput& hi = m_impl->hand[hand];
	if (!hi.aimValid) return false;
	const float M2FT = 3.28084f;
	const XrVector3f&    pos = hi.aimPose.position;
	const XrQuaternionf& q   = hi.aimPose.orientation;
	// TransformCameraCentricPoint (the cockpit projection) wants a vector FROM THE CAMERA (head) to the
	// point -- so the ray origin must be the controller RELATIVE TO THE HEAD, not absolute in appSpace.
	// Subtract the head position (same appSpace), then map OpenXR (RH,+Y up) -> Falcon body (x=fwd=-z,
	// y=right=+x, z=down=-y), in feet.
	const XrVector3f& hp = m_impl->lastHeadPose.position;
	float rx = pos.x - hp.x, ry = pos.y - hp.y, rz = pos.z - hp.z;
	// Empirically matched to the cockpit BUTTON frame (loc): a front-console button is (x<0, y>0, z<0),
	// while the naive GetHeadPosFeet map gave the ray (x>0, y>0, z>0). y matches; x & z are inverted (a
	// 180deg turn about the vertical). So x=+(-z_xr->flip)=+rz, y=+rx, z=+ry (feet).
	origin[0] = rz * M2FT; origin[1] = rx * M2FT; origin[2] = ry * M2FT;
	// Forward = rotate the quaternion by (0,0,-1) [OpenXR forward], i.e. -(third column of R):
	float fx = -2.0f * (q.x * q.z + q.w * q.y);
	float fy = -2.0f * (q.y * q.z - q.w * q.x);
	float fz = -(1.0f - 2.0f * (q.x * q.x + q.y * q.y));
	// Same (flipped x/z) body axis map for the direction (no metre scale; normalize).
	float bx = fz, by = fx, bz = fy;
	float len = sqrtf(bx * bx + by * by + bz * bz);
	if (len < 1e-6f) return false;
	dir[0] = bx / len; dir[1] = by / len; dir[2] = bz / len;
	return true;
}

// Grip pose position (where the hand holds the controller) in the Falcon BODY frame, relative to the head
// -- same flipped axis map as GetControllerAimBody. For drawing a controller marker. False if no grip pose.
bool OpenXRBackend::GetControllerGripBody(int hand, float origin[3]) const
{
	if (!m_impl || hand < 0 || hand > 1) return false;
	const Impl::HandInput& hi = m_impl->hand[hand];
	if (!hi.gripValid) return false;
	const float M2FT = 3.28084f;
	const XrVector3f& pos = hi.gripPose.position;
	const XrVector3f& hp  = m_impl->lastHeadPose.position;
	float rx = pos.x - hp.x, ry = pos.y - hp.y, rz = pos.z - hp.z;
	origin[0] = rz * M2FT; origin[1] = rx * M2FT; origin[2] = ry * M2FT;
	return true;
}

// Artscout - 2026 (VR controller model): the runtime's CURRENT interaction profile path for a hand
// (e.g. "/interaction_profiles/valve/index_controller"), so the caller can pick which controller mesh to
// draw (Index vs Touch/other). Empty string if unknown. Uses xrGetCurrentInteractionProfile + xrPathToString.
bool OpenXRBackend::GetInteractionProfile(int hand, char* out, int cap) const
{
	if (!m_impl || !out || cap < 1 || hand < 0 || hand > 1) return false;
	out[0] = 0;
	if (m_impl->session == XR_NULL_HANDLE || m_impl->handPath[hand] == XR_NULL_PATH) return false;
	XrInteractionProfileState ips = { XR_TYPE_INTERACTION_PROFILE_STATE };
	if (XR_FAILED(xrGetCurrentInteractionProfile(m_impl->session, m_impl->handPath[hand], &ips))) return false;
	if (ips.interactionProfile == XR_NULL_PATH) return false;
	uint32_t len = 0;
	if (XR_FAILED(xrPathToString(m_impl->instance, ips.interactionProfile, (uint32_t)cap, &len, out))) { out[0] = 0; return false; }
	return out[0] != 0;
}

// Artscout - 2026 (VR controller model): grip ORIENTATION as a body-frame basis (fwd/right/up, unit) so a
// mesh can be oriented like the real controller. Same axis map as GetControllerAimBody's direction
// (body = (z,x,y) of the OpenXR vector). The caller applies the same VrRayFlipH/V it uses for the ray.
bool OpenXRBackend::GetControllerGripBasis(int hand, float fwd[3], float right[3], float up[3]) const
{
	if (!m_impl || hand < 0 || hand > 1) return false;
	const Impl::HandInput& hi = m_impl->hand[hand];
	if (!hi.gripValid) return false;
	const XrQuaternionf& q = hi.gripPose.orientation;
	// Column vectors of the rotation matrix in OpenXR axes.
	float rx[3] = { 1.0f - 2.0f*(q.y*q.y + q.z*q.z), 2.0f*(q.x*q.y + q.w*q.z),       2.0f*(q.x*q.z - q.w*q.y) };       // R*(1,0,0) = right
	float uy[3] = { 2.0f*(q.x*q.y - q.w*q.z),       1.0f - 2.0f*(q.x*q.x + q.z*q.z), 2.0f*(q.y*q.z + q.w*q.x) };       // R*(0,1,0) = up
	float fz[3] = { -2.0f*(q.x*q.z + q.w*q.y),      -2.0f*(q.y*q.z - q.w*q.x),      -(1.0f - 2.0f*(q.x*q.x + q.y*q.y)) }; // R*(0,0,-1) = forward
	// Map OpenXR (vx,vy,vz) -> Falcon body (vz, vx, vy), same as GetControllerAimBody.
	fwd[0]   = fz[2]; fwd[1]   = fz[0]; fwd[2]   = fz[1];
	right[0] = rx[2]; right[1] = rx[0]; right[2] = rx[1];
	up[0]    = uy[2]; up[1]    = uy[0]; up[2]    = uy[1];
	return true;
}

// Artscout - 2026 (VR hands): true if the runtime returned a valid hand skeleton for THIS hand this frame.
// The caller (vcock) draws the skeleton when true, else falls back to the wireframe controller.
bool OpenXRBackend::HandJointsValid(int hand) const
{
	return m_impl && hand >= 0 && hand < 2 && m_impl->handTrackingEnabled && m_impl->handJointsValid[hand];
}

// Artscout - 2026 (VR hands): the 26 hand joints in the Falcon BODY frame (feet, relative to the head) --
// the SAME axis map as GetControllerGripBody (x=fwd=+z_xr, y=right=+x_xr, z=down=+y_xr). out must hold
// XR_HAND_JOINT_COUNT_EXT (26) xyz triples. Only joints with a valid position are written; validOut[j]
// flags which. Returns false if no valid skeleton this frame.
bool OpenXRBackend::GetHandJointsBody(int hand, float out[][3], bool validOut[]) const
{
	if (!HandJointsValid(hand)) return false;
	const float M2FT = 3.28084f;
	const XrVector3f& hp = m_impl->lastHeadPose.position;
	const XrHandJointLocationEXT* j = m_impl->handJoints[hand];
	for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; ++i)
	{
		bool ok = (j[i].locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0;
		validOut[i] = ok;
		if (!ok) { out[i][0] = out[i][1] = out[i][2] = 0.0f; continue; }
		const XrVector3f& pos = j[i].pose.position;
		float rx = pos.x - hp.x, ry = pos.y - hp.y, rz = pos.z - hp.z;
		out[i][0] = rz * M2FT; out[i][1] = rx * M2FT; out[i][2] = ry * M2FT;
	}
	return true;
}

// Artscout - 2026 (#67 VR recenter): rebuild the app reference space so the user's CURRENT head pose
// (yaw + position) becomes the origin -- fixes the view drifting off (and "flying into the ground" when
// the runtime recenters under us). Standard seated recenter: locate the head in the current appSpace, take
// position + the YAW-ONLY part of the orientation (swing-twist about Y; pitch/roll dropped so the horizon
// stays level), and make a NEW LOCAL space at that offset. The head, located in the new space, lands at the
// origin facing forward -> the cockpit camera (which reads GetHead*) recenters. Drive from the sim thread.
// Public: request a recenter from ANY thread. The actual appSpace swap is done by the render thread in
// BeginStereoFrame (it owns appSpace + has a fresh predicted display time), avoiding a destroy-while-in-use race.
bool OpenXRBackend::Recenter()
{
	if (!m_impl->session) return false;
	m_impl->recenterPending = true;
	return true;
}


// Artscout - 2026: HMD head orientation as a Falcon-body basis (forward/right/up), gimbal-free.
// Replaces the Euler (yaw/pitch/roll -> BuildHeadMatrix) path which divided by zero / flipped at
// pitch = +-90 (look straight down at your feet) and wandered near the poles. OpenXR is RH
// (x=right, y=up, z=back); Falcon body is x=fwd, y=right, z=down -> map (vx,vy,vz)->(-vz,vx,-vy).
// The engine's head matrix columns are [at(forward), rt(right), up], so we hand those back directly.
bool OpenXRBackend::GetHeadBasis(float at[3], float right[3], float up[3]) const
{
	if (!m_impl->haveHeadPose) return false;
	const XrQuaternionf& q = m_impl->lastHeadPose.orientation;
	const float x = q.x, y = q.y, z = q.z, w = q.w;
	// forward = R*(0,0,-1), up = R*(0,1,0)  (OpenXR frame)
	const float fx = -2.0f * (x * z + w * y), fy = 2.0f * (w * x - y * z), fz = 2.0f * (x * x + y * y) - 1.0f;
	const float ux = 2.0f * (x * y - w * z), uy = 1.0f - 2.0f * (x * x + z * z), uz = 2.0f * (y * z + w * x);
	// Convert to Falcon body. 'at' matches the proven Euler forward (-fz,fx,-fy); 'upc' is the
	// engine's third-column convention (body-down at identity), so head roll comes through naturally.
	float at_[3]  = { -fz, fx, -fy };
	float upc[3]  = {  uz, -ux, uy };
	auto norm3 = [](float v[3]) { float s = sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]); if (s > 1e-6f) { v[0]/=s; v[1]/=s; v[2]/=s; } };
	norm3(at_);
	// rt = upc x at  ; up = at x rt  (re-orthonormalize). at & up are perpendicular -> no singularity.
	float rt_[3] = { upc[1]*at_[2] - upc[2]*at_[1], upc[2]*at_[0] - upc[0]*at_[2], upc[0]*at_[1] - upc[1]*at_[0] };
	norm3(rt_);
	float up_[3] = { at_[1]*rt_[2] - at_[2]*rt_[1], at_[2]*rt_[0] - at_[0]*rt_[2], at_[0]*rt_[1] - at_[1]*rt_[0] };
	norm3(up_);
	if (at)    { at[0]=at_[0]; at[1]=at_[1]; at[2]=at_[2]; }
	if (right) { right[0]=rt_[0]; right[1]=rt_[1]; right[2]=rt_[2]; }
	if (up)    { up[0]=up_[0]; up[1]=up_[1]; up[2]=up_[2]; }
	return true;
}

// Artscout - 2026 (VR quad-views): per-VIEW orientation basis (Falcon body frame), from this frame's
// located view pose. Same conversion as GetHeadBasis but for views[eye].pose -- needed because the
// quad-views FOCUS views (2,3) are GAZE-tracked: their pose looks where the eyes look, not straight
// ahead. Rendering them from the head orientation (GetHeadBasis) while the compositor expects the gaze
// pose makes the focus inset double + the cockpit/displays "follow the gaze". Use this so render
// orientation == submitted view pose. Falls back to head basis logic if the eye index is invalid.
bool OpenXRBackend::GetEyeBasis(int eye, float at[3], float right[3], float up[3]) const
{
	Impl* p = m_impl;
	if (eye < 0 || eye >= (int)p->views.size()) return false;
	const XrQuaternionf& q = p->views[eye].pose.orientation;
	const float x = q.x, y = q.y, z = q.z, w = q.w;
	const float fx = -2.0f * (x * z + w * y), fy = 2.0f * (w * x - y * z), fz = 2.0f * (x * x + y * y) - 1.0f;
	const float ux = 2.0f * (x * y - w * z), uy = 1.0f - 2.0f * (x * x + z * z), uz = 2.0f * (y * z + w * x);
	float at_[3]  = { -fz, fx, -fy };
	float upc[3]  = {  uz, -ux, uy };
	auto norm3 = [](float v[3]) { float s = sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]); if (s > 1e-6f) { v[0]/=s; v[1]/=s; v[2]/=s; } };
	norm3(at_);
	float rt_[3] = { upc[1]*at_[2] - upc[2]*at_[1], upc[2]*at_[0] - upc[0]*at_[2], upc[0]*at_[1] - upc[1]*at_[0] };
	norm3(rt_);
	float up_[3] = { at_[1]*rt_[2] - at_[2]*rt_[1], at_[2]*rt_[0] - at_[0]*rt_[2], at_[0]*rt_[1] - at_[1]*rt_[0] };
	norm3(up_);
	if (at)    { at[0]=at_[0]; at[1]=at_[1]; at[2]=at_[2]; }
	if (right) { right[0]=rt_[0]; right[1]=rt_[1]; right[2]=rt_[2]; }
	if (up)    { up[0]=up_[0]; up[1]=up_[1]; up[2]=up_[2]; }
	return true;
}

// Artscout - 2026: HMD head position (relative to the recentered LOCAL origin), converted to the
// Falcon aircraft body frame in FEET, for 6DOF positional tracking (lean in/out/sideways). OpenXR
// is RH: x=right, y=up, z=back. Falcon body: x=forward, y=right, z=down.
bool OpenXRBackend::GetHeadPosFeet(float* fwd, float* right, float* down) const
{
	if (!m_impl->haveHeadPose) return false;
	const XrVector3f& p = m_impl->lastHeadPose.position;
	const float M2FT = 3.28084f;
	if (fwd)   *fwd   = -p.z * M2FT;   // OpenXR -z = forward
	if (right) *right =  p.x * M2FT;   // OpenXR +x = right
	if (down)  *down  = -p.y * M2FT;   // OpenXR +y = up -> body +z is down
	return true;
}

//=============================================================================
// Shutdown
//=============================================================================
void OpenXRBackend::Shutdown()
{
	Impl* p = m_impl;
	if (!p) return;

	for (size_t e = 0; e < p->swapchains.size(); ++e)
	{
		for (size_t i = 0; i < p->swapchains[e].rtvs.size(); ++i)
			if (p->swapchains[e].rtvs[i]) p->swapchains[e].rtvs[i]->Release();   // D3D11 RTVs (D3D12 RTVs live in rtvHeap12)
		if (p->swapchains[e].handle != XR_NULL_HANDLE) xrDestroySwapchain(p->swapchains[e].handle);
	}
	p->swapchains.clear();

	// #DX12 п.5: release the D3D12 VR resources (borrowed device/queue are NOT released).
	if (p->fenceEvt12) { CloseHandle(p->fenceEvt12); p->fenceEvt12 = NULL; }
	if (p->list12)    { p->list12->Release();    p->list12 = NULL; }
	if (p->alloc12)   { p->alloc12->Release();   p->alloc12 = NULL; }
	if (p->fence12)   { p->fence12->Release();   p->fence12 = NULL; }
	if (p->rtvHeap12) { p->rtvHeap12->Release(); p->rtvHeap12 = NULL; }

	if (p->uiStaging) { p->uiStaging->Release(); p->uiStaging = NULL; }
	p->uiImages.clear();
	if (p->uiUpload12) { p->uiUpload12->Release(); p->uiUpload12 = NULL; }   // #DX12 п.5
	p->uiImages12.clear();
	if (p->uiSwapchain != XR_NULL_HANDLE) { xrDestroySwapchain(p->uiSwapchain); p->uiSwapchain = XR_NULL_HANDLE; }

	// Artscout - 2026 (VR hands): destroy the hand trackers.
	if (p->pfnDestroyHandTracker)
		for (int h = 0; h < 2; ++h)
			if (p->handTracker[h] != XR_NULL_HANDLE) { p->pfnDestroyHandTracker(p->handTracker[h]); p->handTracker[h] = XR_NULL_HANDLE; }

	// Artscout - 2026 (VR controllers): tear down input action spaces + set.
	for (int h = 0; h < 2; ++h)
	{
		if (p->aimSpace[h]  != XR_NULL_HANDLE) { xrDestroySpace(p->aimSpace[h]);  p->aimSpace[h]  = XR_NULL_HANDLE; }
		if (p->gripSpace[h] != XR_NULL_HANDLE) { xrDestroySpace(p->gripSpace[h]); p->gripSpace[h] = XR_NULL_HANDLE; }
	}
	if (p->actionSet != XR_NULL_HANDLE) { xrDestroyActionSet(p->actionSet); p->actionSet = XR_NULL_HANDLE; }
	p->inputReady = false;

	if (p->viewSpace != XR_NULL_HANDLE) { xrDestroySpace(p->viewSpace); p->viewSpace = XR_NULL_HANDLE; }
	if (p->appSpace  != XR_NULL_HANDLE) { xrDestroySpace(p->appSpace);  p->appSpace  = XR_NULL_HANDLE; }
	if (p->localRef  != XR_NULL_HANDLE) { xrDestroySpace(p->localRef);  p->localRef  = XR_NULL_HANDLE; }
	if (p->session   != XR_NULL_HANDLE) { xrDestroySession(p->session); p->session   = XR_NULL_HANDLE; }
	if (p->instance  != XR_NULL_HANDLE) { xrDestroyInstance(p->instance); p->instance = XR_NULL_HANDLE; }

	if (p->ctx) { p->ctx->Release(); p->ctx = NULL; }
	p->device = NULL;
	p->sessionRunning = false;
}

//=============================================================================
// OpenXR_PumpFrame -- single-threaded XR frame driver, called from the main
// message loop so the headset gets continuous frames even though the 2D UI only
// repaints on change. In the 3D world: eye layer (M1 clear). In menus: the cached
// flat UI as a quad panel. xrWaitFrame inside paces this to the headset rate.
//=============================================================================
bool OpenXR_PumpFrame()
{
	if (!g_bUseOpenXR || g_pOpenXRBackend == NULL) return false;

	// Sole owner of the XR frame loop (main thread). D3D11 multithread protection makes
	// the context safe even though sim/ui95 threads also render. RunFrame/RunMenuFrame
	// poll events first (so the session starts on READY), then submit a frame.
	if (g_intellivibeData.In3D)
	{
		// In 3D the scene is rendered on the SIM thread; driving the XR frame from this
		// (main) thread too contends on the immediate context (even with MT protection)
		// and starves the sim -> long hangs / black monitor / audio stutter. So the main
		// pump does NOTHING in 3D. M2b will drive the eye frames from the sim render path
		// where the scene is actually drawn. Also drop the menu cache (the menu
		// ImageBuffer is freed/recreated across 3D entry/exit -> dangling pointer).
		g_pXrMenuSurface565 = NULL;
		return false;
	}
	if (g_pXrMenuSurface565)
		return g_pOpenXRBackend->RunMenuFrame(g_pXrMenuSurface565, g_xrMenuW, g_xrMenuH);
	return g_pOpenXRBackend->RunFrame(NULL, NULL);              // menu, no UI cached yet: poll + clear
}
