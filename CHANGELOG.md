# FFViper — what this fork adds over base FreeFalcon

FFViper is a fork of **FreeFalcon** (Falcon 4.0 lineage). The old fixed‑function Direct3D 7
engine has been replaced by modern renderers — first D3D11, now **native Direct3D 12 and
Vulkan** — and the VR goal that drove the render work has landed: the sim flies in a headset
through **OpenXR**. Around that sit a modernized toolchain and an x64 build, a rebuilt
controls/input system, per‑pilot profiles, a native Linux build, and a long list of gameplay,
stability and audio fixes.

Everything below is relative to upstream FreeFalcon.

---

## 1. Render engine (D3D7 → D3D11 → D3D12 / Vulkan)

The entire fixed‑function D3D7 pipeline was replaced with a D3D11 backend.

* **D3D11 backend & device bring‑up** — new `D3D11Backend` (swapchain, back buffer, depth,
  feature level 11.0), `D3D11Renderer`, `D3D11GeometryBuffer`, `D3D11TextureManager`.
* **Fixed‑function emulation in HLSL** — `FFEmu.hlsl` reproduces the legacy render states
  (texture stages, chroma‑key, fog, alpha test, gouraud, material/emissive/specular) via a
  state map; the screen path (`ContextMPR`) and the object path both funnel into it.
* **Render‑to‑texture cockpit displays** — HUD, DED, RWR, MFDs render to a texture atlas and
  are composited onto the 3D cockpit panels; text/line symbology and colors fixed.
* **MSAA** for the 3D scene (resolve to back buffer, with fallback).
* **Textures** — DXT/NVTT pipeline, palette textures re‑baked on palette/TOD change, cockpit
  BSP textures, world‑object SRV binding.
* **Terrain** — fixed black ground (day/night multitexture, perspective), far‑tiles fog
  transition, pragmatic water shimmer, near/far draw‑distance tuning.
* **Lighting & effects** — object material/lighting (no longer flat‑white), dynamic lights
  (muzzle/explosion flashes), explosions/smoke/tracers/particles, depth‑test fixes so effects
  aren’t culled, objects no longer draw through terrain (depth/occlusion).
* **Afterburner** — reworked as additive emissive surfaces with a warm day/night gradient (closer
  to a real F‑16 plume) instead of the dim, washed‑out original.
* **Resolution & window** — settings‑driven resolution incl. widescreen modes (720p/1080p/2K/4K),
  windowed‑mode toggle, rendered mouse cursor.
* **3D cockpit** — camera orientation fixes (mirror/back‑facing), cockpit texture architecture
  on the 3D path (the dead 2D cockpit path is retired).
* **Legacy D3D7 removed** — the dead Direct3D 7 / DirectDraw code paths were stripped out across the
  engine (state machine, light/VB managers, contexts) and the live ones ported to D3D11; D3D7 is no
  longer used or executed at runtime.

### Native D3D12 and Vulkan

D3D11 was in turn retired: it served as the reference while the two modern backends were brought
up, and its code is now gone. Both live behind one backend‑neutral `IRenderer` / `IRenderBackend`
interface, and the renderer is picked in the graphics options.

* **Direct3D 12 backend** — own device/swapchain bring‑up, PSO cache, descriptor ring, per‑frame
  constant/vertex/index ring allocators, texture uploads on the copy queue, and DRED‑assisted
  device‑removal diagnostics. It is the default renderer.
* **Vulkan backend** — device/swapchain bring‑up on VMA, render passes for the scene, off‑screen
  RTT and menu targets, deferred resource destruction (freeing an asset under a live frame used to
  hang the GPU), and optional validation/sync‑validation layers behind config knobs.
* **Bindless textures** in both backends — D3D12 through `ResourceDescriptorHeap` (SM 6.6), Vulkan
  through descriptor indexing, with slots recycled when a texture is destroyed.
* **Reversed‑Z depth** and a D32 depth buffer, which is what finally stopped distant terrain from
  z‑fighting and "swimming".
* **MSAA** and anisotropic filtering on both backends; **off‑screen RTT** cockpit displays,
  sensor video and the 3D model viewer all render through the same neutral path.
* **x64** — the engine builds and runs as a 64‑bit binary (registry, file and asm paths ported).

### One shader language, compiled into the binary

* Both backends now build from the **same HLSL sources**. DXC compiles each entry point twice —
  DXIL for D3D12, SPIR‑V for Vulkan — from one manifest, so the two backends can no longer drift
  apart the way separate HLSL and GLSL sets did.
* The bytecode is **baked into the executable**; no `.hlsl` or `.spv` files ship next to the game,
  and a shader edit cannot silently ship stale bytecode.

### Terrain

* **GPU terrain** replacing the CPU‑built grid, with LOD "connector" seams (one shared surface
  between LODs instead of overlapping layers, so no z‑fight and no edge shimmer).
* **Mesh‑shader terrain** (D3D12 and Vulkan): a toroidal clipmap streams elevation posts and tile
  slots on the GPU, amplification shaders do the LOD pick and frustum cull, and one dispatch draws
  the ground. Falls back to the classic path when the hardware or the knob says no.
* Fixed black ground, far‑tile fog transition, and the day/night ground level — including the
  sensor pass, which used to read the TV/IR lamp as a black sun and drive the ground to night.

### Sensor video and night vision

* **TGP / Maverick / LANTIRN** render a real 3D scene into the MFD texture atlas on every backend,
  not just symbology, with the sensor zone scissored so the picture cannot splatter over the
  HUD/DED, and a reduced terrain radius so the zoomed sensor does not flood the command list.
* **NVG** greens the world through the object, terrain and screen shaders alike, with tube gain,
  scanlines, grain and vignette, and a night ground level lifted so the goggles show a lit scene.

## 2. VR (OpenXR)

The long‑term goal of the render work, now shipped.

* **OpenXR session** with stereo rendering and full 6DoF head tracking, on D3D12 and Vulkan.
* **Single‑pass stereo** — view instancing on D3D12, multiview on Vulkan — so the world is drawn
  once for both eyes; **quad views** and foveated rendering are supported where the runtime offers
  them (Varjo/Pimax), with the focus pair driving anything resolution‑dependent.
* **Cockpit in VR** — RTT displays, HUD through a proper collimator (the symbology sits at
  infinity instead of painted on the glass), canopy, and a head‑pan limit.
* **VR interface** — the 2D menus, comms and AWACS panels are composited onto XR quad layers with
  a pointer ray and a 3D mouse cursor; the in‑3D menu and the exit dialog work in the headset.
* **Touch controllers** — models, click/pointer input, a ring menu, and skeletal hand tracking
  with orientation‑based skinning.
* **Correct headset gamma** — the eye images are handed to the runtime with the exact bits the
  monitor gets. The Vulkan path used to blit into the sRGB swapchain image, which encoded gamma a
  second time and washed the headset out; it now decodes in the copy shader to cancel it.

## 3. Modernized toolchain

* Migrated math from legacy **D3DX → DirectXMath**.
* Builds against a **modern Windows SDK** (packing/macro/intrinsic fixes); the x64 port that
  groundwork was for is done.
* **NVTT** texture tooling.
* All in‑repo source comments translated to **English** (the fork is pushed to a public repo).

## 4. New controls / input system

* **DCS‑style controls window** — a function × device table replacing the old per‑category list,
  with categories and BMS‑style labels.
* **XML‑based control config** — `config\controls.xml` catalog (offline‑generated) and runtime
  `keyboard.xml` / `<GUID>.xml` / `axismapping.xml`, replacing `keystrokes.key` /
  `axismapping.dat` at runtime.
* **GUID‑stable bindings** — axes and buttons are keyed to the device GUID, so they survive
  device re‑enumeration / re‑plugging.
* **Button‑assignment window** with Cyrillic UI text rendered through GDI in `ui95`, auto‑detect
  of pressed buttons, a stuck‑button guard, a “clear” column, in‑window function search/filter,
  and a restored list scrollbar.
* Modern **DirectInput** devices, scroll‑wheel functions, pitch/roll split out into the axes
  window.

## 5. Per‑pilot profiles (controls **and** logbook)

* Each pilot owns `config\profiles\<dir>\` holding `keyboard.xml`, `<GUID>.xml`,
  `axismapping.xml`, `logbook.xml`, `stats.plc`, `options.pop`, `rules.rul`.
* `config\profiles.xml` maps callsign → numbered dir (default pilot = dir 0 / folder `default`)
  and records the active pilot; the last selection persists across launches.
* The **logbook moved out of the encrypted `.lbk`** into readable `logbook.xml` (legacy `.lbk`
  is still read once and migrated). Player options and rules likewise live in the profile.
* Pilot list, rename and “new pilot” all operate through `profiles.xml`; nothing per‑pilot is
  written to `config\` the old way anymore.

## 6. Gameplay & QoL

* **Instant Action**: unlimited ammo / chaff / flares (independent of options); fixed a weapon
  count leak that blocked missile launches.
* **MRM / AIM‑120**: launches in maddog (MRM) as it should; HUD MRM redesign (wider circle,
  speed/altitude inside, BMS‑style).
* **Differential braking** (per‑wheel brakes + steering).
* **Audio**: restored radio chatter / ST80 voice codec.

## 7. Stability (corruption / hangs / crashes)

* **Object‑list lifetime overhaul** — recursive lock serializing render vs sim/campaign access,
  idempotent insert, out‑of‑line `DrawableObject` destructor that unlinks on delete, SEH‑guarded
  draw/update callbacks; fixes the use‑after‑free (0xDD), self‑cycle OOM, and enter/exit‑3D hangs.
* **Release‑only 2D‑vanish bug** — root‑caused to an ODR struct‑packing mismatch
  (`#pragma pack` leak across translation units) and fixed by pinning the layout of the
  render‑context headers.
* Fixed the **memory leak on 3D enter/exit** (`std::bad_alloc` on repeated entry), the
  null model‑node load hang, the padlock (F3) crash, and dynamic‑campaign enter/exit hangs.
* **Menu 3D‑viewer black‑out** — the UI model viewer (Munitions, etc.) drew straight to the D3D11
  back buffer and flipped Present into the in‑sim compositing path, blacking out the menu; it now
  renders to an off‑screen target and is read back into the 2D menu surface.

## 8. Installer

* **WiX patch installer** that overlays the updated `FFViper.exe` + changed data assets
  (shaders, config, default profile, edited art) onto an existing FreeFalcon 6 install, with
  registry auto‑detection of the game folder.

## 9. Linux

The fork runs natively on Linux — no Wine, no translation layer. Vulkan is the renderer there,
and the same OpenXR path gives VR.

* **CMake + clang build** of the whole tree, reading the source lists straight out of the Windows
  `.vcxproj` files so the two builds cannot drift apart.
* **win32 foundation shim** — `<windows.h>` and friends reimplemented for the engine (handles,
  sync, files, strings, time, COM stubs), with an **SDL3** window/event/input layer and **OpenAL**
  in place of DirectSound.
* Path separators and file lookups made case‑ and separator‑tolerant, since the game data was
  authored on a case‑insensitive filesystem.

---

*Targeted but not yet shipped: water/surf shaders, canopy reflections/rain, ejection‑seat texture,
radar‑lock reliability, throttle/RPM sync at mission start, and a thermal model for the IR sensor
image.*
