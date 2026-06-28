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
}

// OpenXR with the D3D11 graphics binding (must be defined before the platform header).
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include "OpenXRBackend.h"
#include "D3D11Backend.h"                  // g_pD3D11Backend->Hwnd() for the cursor mapping
#include "../../sim/INCLUDE/ivibedata.h"   // g_intellivibeData.In3D (menu vs 3D world)


OpenXRBackend* g_pOpenXRBackend = NULL;

// Menu UI surface cache (set by ImageBuffer::SwapBuffers, read by OpenXR_PumpFrame).
const void* g_pXrMenuSurface565 = NULL;
int g_xrMenuW = 0;
int g_xrMenuH = 0;

// Artscout - 2026 (VR menu): eliminate the cross-thread race on the menu 565 surface. The OLD design cached
// g_pXrMenuSurface565 = an ImageBuffer's m_pSysMem and let the pump (other thread) read it -> the buffer
// could be freed/resized mid-read (0xC0000005). Now the PRODUCER (PresentD3D11, where m_pSysMem is valid)
// COPIES the surface into a stable buffer under a lock, and the pump reads a snapshot of THAT. No race.
static CRITICAL_SECTION s_xrMenuCS;
static bool             s_xrMenuCSReady   = false;
static unsigned char*   s_xrMenuStable    = NULL;   // producer-filled, lock-protected
static long             s_xrMenuStableCap = 0;
static unsigned char*   s_xrMenuSnap      = NULL;   // pump-local snapshot (the convert reads this, no lock)
static long             s_xrMenuSnapCap   = 0;

static void XrMenuLockInit() { if (!s_xrMenuCSReady) { InitializeCriticalSection(&s_xrMenuCS); s_xrMenuCSReady = true; } }

// Producer (PresentD3D11 thread): copy the (valid-here) 565 surface into the stable buffer under the lock.
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
	XrSpace      appSpace;     // LOCAL reference space (app world origin)
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
	};
	std::vector<Swapchain> swapchains;   // one per eye

	int64_t      swapchainFormat;        // DXGI_FORMAT chosen for the eye color images

	// Menu mode: a single quad-layer swapchain showing the flat 2D UI, plus a
	// CPU-writable staging texture for the 565->RGBA upload.
	XrSwapchain  uiSwapchain;
	int          uiW, uiH;
	int64_t      uiFormat;               // R8G8B8A8 (UNORM or _SRGB)
	std::vector<XrSwapchainImageD3D11KHR> uiImages;
	ID3D11Texture2D* uiStaging;

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

	Impl()
		: instance(XR_NULL_HANDLE), systemId(XR_NULL_SYSTEM_ID), session(XR_NULL_HANDLE),
		  appSpace(XR_NULL_HANDLE), viewSpace(XR_NULL_HANDLE),
		  sessionState(XR_SESSION_STATE_UNKNOWN), sessionRunning(false),
		  viewConfigType(XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO),
		  swapchainFormat(0),
		  uiSwapchain(XR_NULL_HANDLE), uiW(0), uiH(0), uiFormat(0), uiStaging(NULL),
		  device(NULL), ctx(NULL),
		  nearZ(1.0f), farZ(80000.0f), haveHeadPose(false),
		  lastYaw(0.0f), lastPitch(0.0f), lastRoll(0.0f), haveHeadAngles(false),
		  inStereoFrame(false), currentEye(-1), haveSubmitFov(false),
		  menuQuadPending(false), recenterPending(false)
	{
		lastHeadPose.orientation.x = lastHeadPose.orientation.y = lastHeadPose.orientation.z = 0.0f;
		lastHeadPose.orientation.w = 1.0f;
		lastHeadPose.position.x = lastHeadPose.position.y = lastHeadPose.position.z = 0.0f;
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
	if (!device) { XrDbg("OpenXR: Init - null device\n"); return false; }
	Impl* p = m_impl;
	p->device = device;
	device->GetImmediateContext(&p->ctx);

	// The engine drives D3D11 from several threads (sim render, ui95 OutputLoop) and
	// the XR pump adds another; the immediate context is NOT thread-safe by default
	// (-> "CORRUPTED_MULTITHREADING" + crash). Turn on the runtime's internal locking
	// so concurrent context calls are serialized. Required once VR is on.
	{
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

	bool haveD3D11 = false, haveQuadViews = false, haveEyeGaze = false;
	for (uint32_t i = 0; i < extCount; ++i)
	{
		const char* n = exts[i].extensionName;
		if (strcmp(n, XR_KHR_D3D11_ENABLE_EXTENSION_NAME) == 0)              haveD3D11 = true;
		else if (strcmp(n, XR_VARJO_QUAD_VIEWS_EXTENSION_NAME) == 0)         haveQuadViews = true;
		else if (strcmp(n, XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME) == 0) haveEyeGaze = true;
	}
	if (!haveD3D11)
	{
		XrDbg("OpenXR: runtime lacks %s -- VR unavailable\n", XR_KHR_D3D11_ENABLE_EXTENSION_NAME);
		return false;
	}

	// --- 2. Create instance (D3D11 + optional quad-views + eye-gaze) ---------
	// Quad views (4 viewports: wide context + narrow focus per eye) + eye gaze let the
	// foveated-rendering API layer (e.g. Quad-Views-Foveated) do gaze-driven foveation.
	// The whole view/swapchain/render path below is N-view generic, so 2 or 4 just works.
	std::vector<const char*> enabledExts;
	enabledExts.push_back(XR_KHR_D3D11_ENABLE_EXTENSION_NAME);
	if (haveQuadViews) enabledExts.push_back(XR_VARJO_QUAD_VIEWS_EXTENSION_NAME);
	if (haveEyeGaze)   enabledExts.push_back(XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME);
	XrDbg("OpenXR: ext quadViews=%d eyeGaze=%d\n", (int)haveQuadViews, (int)haveEyeGaze);

	XrInstanceCreateInfo ici = { XR_TYPE_INSTANCE_CREATE_INFO };
	ici.enabledExtensionCount = (uint32_t)enabledExts.size();
	ici.enabledExtensionNames = enabledExts.data();
	strcpy(ici.applicationInfo.applicationName, "FreeFalcon");
	ici.applicationInfo.applicationVersion = 1;
	strcpy(ici.applicationInfo.engineName, "FFViper");
	ici.applicationInfo.engineVersion = 1;
	ici.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
	XR_BAIL(xrCreateInstance(&ici, &p->instance), "xrCreateInstance");

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

	// --- 4. D3D11 graphics requirements (mandatory before session) -----------
	PFN_xrGetD3D11GraphicsRequirementsKHR pfnReqs = NULL;
	xrGetInstanceProcAddr(p->instance, "xrGetD3D11GraphicsRequirementsKHR",
	                      (PFN_xrVoidFunction*)&pfnReqs);
	if (!pfnReqs) { XrDbg("OpenXR: xrGetD3D11GraphicsRequirementsKHR unavailable\n"); Shutdown(); return false; }
	XrGraphicsRequirementsD3D11KHR reqs = { XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR };
	XR_BAIL(pfnReqs(p->instance, p->systemId, &reqs), "xrGetD3D11GraphicsRequirementsKHR");

	// Warn (do not fail) if our existing device is on a different adapter than the
	// runtime wants -- on a single-GPU PC they match. Recreating the device on the
	// XR adapter is a follow-up if a mismatch is ever observed.
	{
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
	}

	// --- 5. Session ----------------------------------------------------------
	XrGraphicsBindingD3D11KHR gb = { XR_TYPE_GRAPHICS_BINDING_D3D11_KHR };
	gb.device = device;
	XrSessionCreateInfo sci = { XR_TYPE_SESSION_CREATE_INFO };
	sci.next = &gb;
	sci.systemId = p->systemId;
	XR_BAIL(xrCreateSession(p->instance, &sci, &p->session), "xrCreateSession");

	// --- 6. Reference spaces (app=LOCAL, head=VIEW) --------------------------
	XrReferenceSpaceCreateInfo rsci = { XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
	rsci.poseInReferenceSpace.orientation.w = 1.0f;
	rsci.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
	XR_BAIL(xrCreateReferenceSpace(p->session, &rsci, &p->appSpace), "xrCreateReferenceSpace(LOCAL)");
	rsci.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
	XR_BAIL(xrCreateReferenceSpace(p->session, &rsci, &p->viewSpace), "xrCreateReferenceSpace(VIEW)");

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

				ID3D11RenderTargetView* rtv = sc.rtvs[imgIndex];

				// Per-eye matrices (milestone 2 uses these; see handedness note above).
				float proj[16], view[16];
				XrProjectionToD3D(p->views[e].fov, p->nearZ, p->farZ, proj);
				XrPoseToView(p->views[e].pose, view);

				if (render)
				{
					render(user, (int)e, rtv, sc.width, sc.height, view, proj);
				}
				else
				{
					// Milestone 1: just clear so the headset shows a live frame.
					const float clear[4] = { 0.05f, 0.10f, 0.18f, 1.0f };
					p->ctx->ClearRenderTargetView(rtv, clear);
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

	p->uiW = w;
	p->uiH = h;
	XrDbg("OpenXR: menu quad swapchain %dx%d\n", w, h);
	return true;
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
			if (g_pD3D11Backend && g_pD3D11Backend->Hwnd())
			{
				POINT pt;
				HWND hwnd = g_pD3D11Backend->Hwnd();
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
		if (XR_SUCCEEDED(xrLocateSpace(p->viewSpace, p->appSpace, p->stereoFrameState.predictedDisplayTime, &rloc)) &&
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

	if (outRtv) *outRtv = sc.rtvs[idx];
	if (outW)   *outW = sc.width;
	if (outH)   *outH = sc.height;
	return true;
}

void OpenXRBackend::EndEye(int eye)
{
	Impl* p = m_impl;
	if (eye < 0 || eye >= (int)p->swapchains.size()) return;
	Impl::Swapchain& sc = p->swapchains[eye];

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
		// Artscout - 2026: submit the fov the engine actually rendered with (symmetric submitFov set
		// per eye from the headset's horizontal fov + eye aspect). Render fov == submit fov -> the
		// compositor maps the image undistorted and the two eyes fuse.
		pv.fov  = p->haveSubmitFov ? p->submitFov : p->views[eye].fov;
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
		p->ctx->ClearRenderTargetView(sc.rtvs[idx], (e == 0) ? red : green);
		p->ctx->Flush();
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
			if (p->swapchains[e].rtvs[i]) p->swapchains[e].rtvs[i]->Release();
		if (p->swapchains[e].handle != XR_NULL_HANDLE) xrDestroySwapchain(p->swapchains[e].handle);
	}
	p->swapchains.clear();

	if (p->uiStaging) { p->uiStaging->Release(); p->uiStaging = NULL; }
	p->uiImages.clear();
	if (p->uiSwapchain != XR_NULL_HANDLE) { xrDestroySwapchain(p->uiSwapchain); p->uiSwapchain = XR_NULL_HANDLE; }

	if (p->viewSpace != XR_NULL_HANDLE) { xrDestroySpace(p->viewSpace); p->viewSpace = XR_NULL_HANDLE; }
	if (p->appSpace  != XR_NULL_HANDLE) { xrDestroySpace(p->appSpace);  p->appSpace  = XR_NULL_HANDLE; }
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
