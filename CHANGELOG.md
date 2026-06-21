# FFViper — what this fork adds over base FreeFalcon

FFViper is a fork of **FreeFalcon** (Falcon 4.0 lineage). The headline change is a full
**Direct3D 11 render port** of the old Direct3D 7 engine, plus a modernized toolchain, a
rebuilt controls/input system, a new per‑pilot profile system, and a long list of gameplay,
stability and audio fixes. The end goal of the render work is VR.

Everything below is relative to upstream FreeFalcon.

---

## 1. Direct3D 11 render port (D3D7 → D3D11)

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

## 2. Modernized toolchain

* Migrated math from legacy **D3DX → DirectXMath**.
* Builds against a **modern Windows SDK** (packing/macro/intrinsic fixes); groundwork for x64.
* **NVTT** texture tooling.
* All in‑repo source comments translated to **English** (the fork is pushed to a public repo).

## 3. New controls / input system

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

## 4. Per‑pilot profiles (controls **and** logbook)

* Each pilot owns `config\profiles\<dir>\` holding `keyboard.xml`, `<GUID>.xml`,
  `axismapping.xml`, `logbook.xml`, `stats.plc`, `options.pop`, `rules.rul`.
* `config\profiles.xml` maps callsign → numbered dir (default pilot = dir 0 / folder `default`)
  and records the active pilot; the last selection persists across launches.
* The **logbook moved out of the encrypted `.lbk`** into readable `logbook.xml` (legacy `.lbk`
  is still read once and migrated). Player options and rules likewise live in the profile.
* Pilot list, rename and “new pilot” all operate through `profiles.xml`; nothing per‑pilot is
  written to `config\` the old way anymore.

## 5. Gameplay & QoL

* **Instant Action**: unlimited ammo / chaff / flares (independent of options); fixed a weapon
  count leak that blocked missile launches.
* **MRM / AIM‑120**: launches in maddog (MRM) as it should; HUD MRM redesign (wider circle,
  speed/altitude inside, BMS‑style).
* **Differential braking** (per‑wheel brakes + steering).
* **Audio**: restored radio chatter / ST80 voice codec.

## 6. Stability (corruption / hangs / crashes)

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

## 7. Installer

* **WiX patch installer** that overlays the updated `FFViper.exe` + changed data assets
  (shaders, config, default profile, edited art) onto an existing FreeFalcon 6 install, with
  registry auto‑detection of the game folder.

---

*Targeted but not yet shipped: water/surf shaders, volumetric clouds, canopy reflections/rain,
ejection‑seat texture, radar‑lock reliability, throttle/RPM sync at mission start, and OpenXR/VR
(the long‑term goal of the D3D11 work).*
