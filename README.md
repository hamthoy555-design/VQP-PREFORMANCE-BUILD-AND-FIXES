# Vivify Quest Port

A standalone Quest port and performance-focused compatibility layer for Vivify maps in Beat Saber 1.40.8.

This project is intended to make demanding Vivify maps more stable on Quest without removing their intended visuals. It adds fail-soft safety limits, mirror fixes, performance controls, crash hardening, and compatibility behavior for maps that expect PC-only capabilities.

> **Current release: v0.4.21**
>
> Install the packaged `Vivify2.qmod` through ModsBeforeFriday (MBF).

---

## What this port does

Vivify maps can use custom prefabs, animated objects, secondary cameras, render textures, blits, screen effects, custom notes, custom sabers, trails, debris, and other effects. Those features are expensive on standalone Quest hardware because the headset has a much smaller GPU and memory budget than a PC.

This port focuses on four goals:

1. **Keep the map's visuals working whenever possible.**
2. **Reduce expensive work before reducing visible quality.**
3. **Fail safely when a map requests more resources than Quest can handle.**
4. **Prevent malformed map data or destroyed Unity objects from crashing Beat Saber.**

The main HMD camera is intentionally protected. Performance reductions target secondary cameras and temporary effect buffers first; the main headset view is not deliberately rendered at a lower resolution.

---

## Current features

### Performance Mode

**Performance Mode is enabled by default.** It combines several safe reductions:

- Secondary and mirror cameras render at approximately **70% resolution**.
- The main HMD camera remains at its normal resolution.
- Mirror cameras can update at a reduced rate instead of re-rendering the whole scene every headset frame.
- Temporary blit/post-processing buffers use approximately **75% resolution**.
- The final processed image is still copied to the normal full-resolution HMD target.
- Static, untracked prefabs can be reused through a conservative object pool.
- Animated or Tracks-bound objects are not pooled, preventing animation state from leaking between objects.
- Camera setup, camera-property writes, song-time reads, shader checks, and other repeated work are cached or throttled where safe.

These changes target the most expensive work on Quest: full-scene secondary-camera renders, full-resolution temporary blits, repeated allocation/destruction, and unnecessary per-frame interop calls.

### Performance settings

The Vivify settings page includes these performance controls:

- **Performance Mode** — enables the balanced Quest performance reductions. Default: on.
- **Mirrors at 30 FPS** — updates secondary cameras every third frame. Default: on.
- **Dynamic Effect Resolution** — lowers only temporary effect buffers, not the final HMD target. Default: on.
- **Asset Culling** — reduces the scale of Vivify objects only while they are completely outside the real camera frustum. Default: on.

Performance reductions are deliberately conservative. The port does not blindly delete screen effects, disable all objects behind the player, or lower the main HMD render resolution.

### Asset culling

Asset culling uses the camera's actual view to shrink stuff that isn't in it

- Objects remain at normal size when any part of their bounds could be visible.
- Objects outside the padded frustum for a short continuous delay can be scaled down.

- Objects restore immediately when they return to view.

- The test includes the object's renderer bounds, not only its transform position.
- A padding margin intentionally favors keeping objects large rather than accidentally shrinking something visible.

- Animators and particles keep running; objects are not disabled, so they cannot get stuck invisible.

- The original post-spawn scale is captured correctly before an object is reduced and restored.

Asset culling is scale-based rather than a renderer-removal system. It is intended to reduce pixel cost for unseen geometry while preserving object state.

### Mirror and secondary-camera fixes

The mirror system has received multiple layers of protection:

- Mirror and secondary cameras render mono into flat render targets instead of inheriting stereo rendering from the main camera.
- Secondary cameras no longer drive the global stereo shader state.
- The main gameplay camera and Vivify gameplay overlay camera are the only cameras allowed to establish stereo keyword state.
- Mirror-rig saber clones are ignored by custom saber replacement logic, preventing duplicated sabers in mirrors.
- Dead saber replacement entries are cleaned up instead of stacking across respawns.
- Secondary camera color targets use a 24-bit depth buffer for proper depth testing.
- Mirror targets clear to transparent black before rendering so stale or undefined render-target contents do not appear as random colored pixels.
- Render-texture allocations are tracked and released when cameras or textures are removed.
- Temporary blit targets are released after the effect chain has been inactive for a short period.
- Secondary camera creation has a hard cap to prevent mirror stacks from exhausting Quest resources.

The main HMD camera is not lowered in resolution by these mirror changes.

### Safe Mode Limits

**Safe Mode Limits is enabled by default.** It is a fail-soft resource budget, not a visual-disable switch.

When enabled, Vivify refuses only new allocations that would exceed a safety budget. Existing valid objects and effects are not intentionally removed just because Safe Mode is on.

Current protection includes:

- Render-texture pixel budget: default approximately **24 million pixels**.
- Hard render-texture ceiling: **32 million pixels**.
- Maximum secondary cameras: **8**.
- Maximum declared screen textures: **16**.
- Maximum active Vivify prefabs: **96 by default**, with a hard implementation ceiling.
- Maximum note-visual fragments: **12 by default**.
- Maximum replaced notes: **512**.
- Maximum active blit effects: **96**.
- Maximum render-texture dimension: **4096**.
- Trail duration, sampling frequency, and granularity are clamped to safe ranges.

When a limit is reached, the requested resource is refused and a throttled warning is written instead of allowing an uncontrolled allocation that could crash or reset the headset. Enable **Vivify Debug Logging** to see these warnings in logcat.

The default pixel budget was increased from 8M to 24M because a full-screen effect combined with a mirror could otherwise be rejected even when the map was still within a reasonable Quest budget. Existing configurations that still contain the untouched old 8M default are migrated automatically. Manually selected limits remain respected.

You can turn Safe Mode Limits off, but this removes the resource guard and increases the chance of GPU-driver resets, memory pressure, or a game crash on heavy maps.

### Note visuals and colors

- Custom note prefabs receive the live red/blue color for the correct hand.
- Color is applied only to replacement materials whose shader exposes `_Color`.
- Materials without `_Color` keep their own intended appearance.
- Bomb prefabs are excluded from note-color tinting.
- The unsafe `FindObjectOfType<ColorManager>` path was removed. In Beat Saber 1.40, `ColorManager` is not a Unity object, so that lookup could throw when the first note appeared.
- The color manager is obtained from Beat Saber's saber model controllers instead.
- Note-color application is exception-guarded so an interop problem does not unwind through Tracks, Chroma, or CustomJSONData hooks.
- The expensive full component scrub that previously ran repeatedly over every replaced note now runs at spawn time.
- Dead note replacements are cleaned up without scanning all components every frame.

### Custom sabers and trails

- Vanilla sabers can be hidden only when a Vivify map actually assigns a non-additive custom saber model.
- Maps with no custom saber assignment keep the normal vanilla sabers.
- Additive saber assignments and trail-only assignments do not incorrectly hide the vanilla saber.
- Custom Vivify sabers and trails remain active.
- The setting **Hide Vanilla Sabers on Custom-Saber Maps** is enabled by default and can be changed in the Vivify settings.
- Saber colors are applied with cached material-property handling.
- Mirror clones are skipped so custom sabers are not spawned a second time inside reflections.

### Prefab loading and pooling

- Prefab warm-up is spread across frames instead of instantiating every declared object in one startup hitch.
- Warm-up defaults to four prefab instances per update while Safe Mode is enabled.
- Warm instances are reused when appropriate.
- Static, untracked prefabs can be pooled after destruction and reused later.
- Prefabs with Tracks bindings or animators are destroyed normally rather than pooled.
- Null or destroyed prefab instances are checked before use.
- Active prefab counts are capped.
- Transform data is applied before asset-culling registration so culling restores the correct scale.
- Warm restarts and practice/retry flows reuse the loaded bundle where possible instead of re-deserializing everything.

### Crash and lifecycle hardening

- Vivify custom-event dispatch is wrapped so malformed event data is logged instead of escaping through the game hook chain.
- Destroyed Unity objects are checked before dereferencing them.
- Null results from `Instantiate` are handled safely.
- Dead cameras, renderers, sabers, trails, notes, and prefabs are cleaned up.
- Material-property-block cleanup avoids disposing native blocks that may still be referenced by Unity.
- Camera and render-texture resources are released during map reset.
- Pause and resume handling disables expensive cameras and blits temporarily, then restores animations and camera state safely.
- Overlay-camera recreation is guarded during scene teardown so it cannot attach a depth-only camera to a menu scene and produce a black screen.
- Camera property application is cached and only repeated when the camera or property generation changes.
- Song time is cached per frame to avoid repeated IL2CPP calls.

### Screen effects and blits

Vivify blits, screen textures, and valid image effects are preserved by default.

- Blit materials are checked for valid and supported shaders.
- Unsupported or internal error shaders receive a safe fallback path where possible.
- Temporary blit textures are cached and reused.
- Dynamic Effect Resolution lowers only the temporary intermediate buffers.
- The final output is still sent to the normal destination texture.
- Beat-0 filmgrain isolation can be enabled separately.
- A Disable All Blits option exists for troubleshooting, but it is off by default.
- Safe Mode does not inspect PNG contents or blindly delete image overlays.
- Screen textures are capped and logged when their allocation would exceed the safety budget.

A map's own PNG/image effect may still intentionally cover the view. The port does not remove a valid map-authored effect automatically because doing so would also remove intended visuals. Use Vivify Debug Logging and a log captured while the affected map is actually playing when diagnosing a specific overlay.

### Fake AudioLink compatibility

**Fake AudioLink is enabled by default.**

Vivify registers the `AudioLink` SongCore capability so maps that require AudioLink are accepted as playable on Quest without requiring the PC AudioLink mod.

This compatibility declaration:

- Lets AudioLink-required songs pass the capability check.
- Does not replace the song's audio.
- Does not remove Vivify visuals.
- Does not claim to recreate every PC AudioLink audio-reactive texture or shader behavior.
- May allow a map to load while some effects that require AudioLink's actual runtime data remain static or partially functional.

The option is available as **Fake AudioLink (allow AudioLink songs)**. Changes to capability registration are safest after restarting Beat Saber.

### Compatibility and isolation settings

Other available settings include:

- **Multipass Rendering** — controls Vivify's stereo keyword behavior.
- **Disable Custom Vivify Note Visuals** — disables custom note replacements without disabling unrelated map visuals.
- **Disable Vivify Visuals In Multiplayer** — prevents Vivify visuals from interfering with multiplayer sessions. Default: on.
- **Disable CreateCamera/Depth** — releases or refuses Vivify secondary-camera and depth resources for troubleshooting.
- **Disable VR Center Adjust** — isolates Vivify's VRCenterAdjust interaction when another mod or map causes a problem.
- **Allow Windows bundles** — risky fallback for maps with no Android bundle. Default: off.
- **Vivify Debug Logging** — logs resource limits, camera creation, shader repair, blit decisions, note colors, and other diagnostics.

---

## Installation on Quest

1. Install Beat Saber 1.40.8 and mod it with [ModsBeforeFriday](https://mbf.bsquest.xyz/).
2. Connect the Quest to your PC through ADB and let MBF detect the headset.
3. Select Beat Saber **1.40.8** as the modded version when MBF asks.
4. Wait for MBF to finish patching Beat Saber.
5. Download or locate `Vivify2.qmod`.
6. Drag `Vivify2.qmod` into MBF, or select it with **ADD FILES**.
7. Install the required map dependencies such as Chroma, Noodle Extensions, and Mapping Extensions when a map requires them.
8. Launch Beat Saber and open the Vivify settings to confirm the desired performance and safety options.

The packaged file produced by this project is:

```text
Vivify2.qmod
```

The raw native library is also produced at:

```text
build/libVivify.so
```

The `.qmod` is the file intended for normal MBF installation.

---

## Building the project

### Requirements

- Windows development environment
- Quest Package Manager (QPM)
- CMake
- Ninja or another supported CMake generator
- Android NDK
- PowerShell for the packaging scripts

The build environment used for the Quest port contains Android NDK r27c, CMake, Ninja, and QPM.

### Standard commands

```bash
qpm restore
qpm s build
qpm s qmod
```

If `qpm` is not recognized, install Quest Package Manager or run the local executable if the project provides one:

```powershell
.\qpm restore
.\qpm s build
.\qpm s qmod
```

If the scripts report that `pwsh` is unavailable, install modern PowerShell or adapt the script command to the PowerShell executable available on the machine. The project uses `qpm.json`, `qpm.shared.json`, `qpm_defines.cmake`, and the `scripts` directory for build and packaging configuration.

### Build output

```text
build/libVivify.so
Vivify2.qmod
```

The qmod contains `mod.json` and the native library in a flat package layout suitable for MBF.

---

## Dependencies

### Dependencies packaged/required by the qmod

- beatsaber-hook
- custom-types
- custom-json-data
- tracks
- bsml
- songcore
- paper2_scotland2
- web-utils
- metacore

### Project dependencies

- beatsaber-hook `^6.4.2`
- bs-cordl `4008.*`
- custom-types `^0.18.4`
- custom-json-data `^0.24.5`
- tracks `^2.5.3`
- songcore `^1.1.26`
- bsml `^0.4.55`
- config-utils `^2.0.3`
- web-utils `*`
- scotland2 `^0.1.6`
- paper2_scotland2 `^4.6.4`
- conditional-dependencies `^0.3.0`
- sombrero `^0.1.43`
- cpp-semver `^0.1.2`
- metacore `^1.0.3`

Many core dependencies are installed automatically by MBF. A particular map may still require additional map-specific mods.

---

## Version history

### v0.4.21 — Performance controls and pooling

- Added Performance Mode, enabled by default.
- Secondary/mirror cameras render at approximately 70% resolution.
- Added Mirrors at 30 FPS, updating secondary cameras every third frame.
- Added Dynamic Effect Resolution for temporary blit buffers at approximately 75% resolution.
- The main HMD camera and final destination remain full resolution.
- Added conservative pooling for static, untracked prefabs.
- Animated and Tracks-bound prefabs are not pooled.
- Retained frustum-based asset culling and cached camera/material work.

### v0.4.20 — Safe Mode preserves normal screen effects

- Increased the untouched Safe Mode render-texture default from 8M to 24M pixels.
- Kept the hard ceiling at 32M pixels.
- Migrated old untouched 8M configurations automatically.
- Preserved manually selected custom limits.
- Prevented normal full-screen effects plus a mirror from being rejected unnecessarily.

### v0.4.19 — AudioLink compatibility and mirror clear safety

- Added Fake AudioLink capability registration, enabled by default.
- AudioLink-required maps can pass the SongCore capability check on Quest.
- Mirror render targets clear to transparent black before rendering.
- Preserved valid screen effects and Vivify visuals.

### v0.4.18 — Mirror rendering fixes

- Added 24-bit depth buffers to secondary/mirror color targets.
- Forced secondary cameras to render mono into flat textures.
- Prevented non-main cameras from leaving stereo shader keywords enabled during mirror passes.
- Prevented mirror-rig saber clones from receiving duplicate custom saber replacements.
- Cleaned dead saber replacement entries to prevent stacking after respawns.

### v0.4.17 — Frustum-based asset culling

- Replaced the fixed behind-player cone with a real camera-frustum test.
- Kept objects full size when any part of their bounds could be visible.
- Added object bounds, FOV padding, off-screen delay, and immediate restoration.
- Fixed false culling when looking up or to the sides.

### v0.4.16 — Note-spawn crash fix and culling accuracy

- Removed the invalid `FindObjectOfType<ColorManager>` path.
- Resolved the color manager from saber model controllers.
- Guarded note-color interop from exceptions.
- Fixed incorrect scale restoration after culling.
- Added culling hysteresis to reduce edge flicker.

### v0.4.15 — Note colors, sabers, culling, and lag

- Fixed custom notes showing the wrong color for one hand.
- Added shader `_Color` filtering to avoid black or incorrectly tinted materials.
- Excluded bombs from note-color tinting.
- Added conditional vanilla saber hiding for custom-saber maps.
- Added initial Asset Culling setting, enabled by default.
- Moved expensive note component cleanup to spawn time.
- Reduced repeated per-frame note work.

### v0.4.14 and earlier — Initial stability, mirrors, and safety work

- Added the first mirror stereo-state fix.
- Added resource safety caps for cameras, textures, prefabs, fragments, and render-texture pixels.
- Spread prefab warm-up over multiple frames.
- Reused loaded bundle state on warm restarts.
- Cached camera properties and repeated runtime work.
- Added malformed-event exception hardening.
- Added destroyed-object and null-instantiation checks.
- Added Safe Mode Limits, Windows-bundle fallback control, and the working custom-note-visual toggle.

---

## Known limitations

- This is a standalone Quest port, not the full PC Vivify runtime.
- Fake AudioLink satisfies the capability requirement but cannot reproduce every AudioLink shader/data feed.
- Performance settings have been designed to preserve visuals, but a Quest headset test is still required for each especially demanding map.
- Safe Mode can refuse a genuinely dangerous new resource. The refusal is intentional and is logged when Vivify Debug Logging is enabled.
- A map-authored image or screen effect may intentionally cover the HMD view. The port does not automatically remove valid effects based only on their image content.
- The port does not currently lower the main HMD camera resolution.
- Quality and performance vary by Quest model, refresh rate, installed mods, map complexity, and the number of simultaneous secondary cameras.

For a difficult map, test in this order:

1. Keep **Safe Mode Limits** on.
2. Keep **Performance Mode**, **Mirrors at 30 FPS**, and **Dynamic Effect Resolution** on.
3. Enable **Vivify Debug Logging**.
4. Test the map at the intended headset refresh rate.
5. Only disable individual isolation settings for diagnosis.
6. Disable Safe Mode only as a last resort for a map that genuinely needs more resources.

DO NOT TURN ANYTHING OFF THAT YOU THINK WILL MAKE YOU LAG CAUSE IT WILL AND I AM NOT RESPONSIBLE FOR YOUR ACTIONS 

---

## Credits

This project builds on the work and ideas of the Beat Saber modding community. Please preserve these credits when redistributing the port or its source.

- [Axo-lotl](https://github.com/axo-lotl)
- [LookingForScripts1](https://github.com/Lookingforscripts1)

Most of the original port was borrowed or adapted from work by LookingForScripts. Thank you for making the initial Quest port possible.

Additional credit is due to the maintainers of the dependencies listed above, including Beat Saber Hook, Scotland2, Custom Types, Custom JSON Data, Tracks, SongCore, BSML, Paper2 Scotland2, Web Utils, MetaCore, and the wider Quest modding community.

---

## License and redistribution

Check the upstream project and dependency licenses before redistributing modified source or packaged builds. Keep the original credits and do not present borrowed upstream work as entirely original.
