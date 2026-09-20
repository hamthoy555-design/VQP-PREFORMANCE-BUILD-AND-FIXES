# Vivify Quest Port
------

## What's new in this build (0.4.14)

* **Mirror glitch fix** — Beat Saber's mirror cameras were flipping the global
  stereo shader keywords every frame (Left eye → Right eye → ...), which is what
  caused the offset/glitchy geometry inside mirrors. Only the main camera is
  allowed to drive stereo keyword state now.
* **Resource safety limits** — heavy maps can no longer exhaust the Quest and
  crash the game. Render-texture memory is budgeted (~8M px by default,
  hard-capped at 32M px), secondary cameras are capped at 8, screen textures at
  16, live prefabs at 96, and note-visual fragments per note at 12. When a map
  exceeds a cap the mod fails soft (drops that visual) and logs a warning
  instead of crashing.
* **Smoother song start** — prefab pre-instantiation is spread across frames
  (4/frame) instead of one huge hitch when the first event fires.
* **Faster restarts** — practice-mode / quick-retry restarts reuse the already
  loaded bundle and warm asset caches instead of re-deserializing everything.
* **Lower per-frame overhead** — camera property writes, overlay camera setup
  and note-replacement component scrubbing are cached/throttled instead of
  running full tilt every frame.
* **Crash hardening** — all Vivify custom events are dispatched inside a
  try/catch (a malformed event logs an error instead of crashing the game), and
  several destroyed-object / null-instance edge cases were fixed.
* **New settings toggles** — "Safe Mode Limits" (limits on/off) and "Allow
  Windows bundles" (risky fallback for maps with no Android bundle; off by
  default). "Disable Custom Vivify Note Visuals" now actually works.

## Installation For Making Your Own
(You can make your own. give credits down below)

This is How To Build it

```
qpm restore
qpm s build
qpm s qmod
```

If it says "qpm is not recognized" then you don't have qpm. if it says pwsh is not avaiable. then change the script commands in qpm.json and in scripts to powershell. OR get latest powershell.

If you do have QPM installed but still gives an error. do .\qpm (with a .\ in it)

## How to install it onto your quest.

Most People don't know how to install it when they are new. this is a tutorial.

Go onto [ModsBeforeFriday](https://mbf.bsquest.xyz/) And connect your quest to your PC (Via ADB).
Now you have to own beatsaber and have it installed. if you do not own beatsaber you need to buy it on quest. if you do not have it installed. please install it before continuing.
Now if you have beatsaber installed and own beatsaber. you are gonna let MBF connect to your quest. it might give a warning. but you are gonna select 1.40.8 as the modded version. (BEST MODDED VERSION RIGHT NOW) and accept. now its gonna take 10 - 20 minutes for MBF to finish. (might take longer depending on your wifi) Now when MBF says app is modded. you can install any mod you want. (Get Chroma, NE, and ME, For best gameplay on maps.) and after you do so. just go to where vivify.qmod is. then drag and drop it into MBF. or click "ADD FILES" then select the .qmod

## Dependencies
(For the .qmod)

* beatsaber-hook
* custom-types
* custom-json-data
* tracks
* bsml
* songcore
* paper2_scotland2
* web-utils
* metacore

(For The Project.) (in qpm.json)

* beatsaber-hook ^6.4.2
* bs-cordl 4008.*
* custom-types ^0.18.4
* custom-json-data ^0.24.5
* tracks ^2.5.3
* songcore ^1.1.26
* bsml ^0.4.55
* config-utils ^2.0.3
* web-utils *
* scotland2 ^0.1.6
* paper2_scotland2 ^4.6.4
* conditional-dependencies ^0.3.0
* sombrero ^0.1.43
* cpp-semver ^0.1.2
* metacore ^1.0.3

MOST OF THESE ARE BASE CORE MODS INSTALLED WHEN YOU USE MBF. MOST ARE NEEDED TO BE INSTALLED MANUALLY.

## Credits
Below is the credits to most of this source. i do not want to steal code from others so this is credits to them. (REMEMBER WHEN YOU BORROW CODE. ALWAYS GIVE CREDITS YA LIL SKID)

* [Axo-lotl](https://github.com/axo-lotl)
* [LookingForScripts1](https://github.com/Lookingforscripts1)

I borrowed most of this code from LookingForScripts. Thank you to them for getting this port to work!
