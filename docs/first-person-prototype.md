# Optional first-person prototype

Choose **Options → Gameplay → View Mode → Classic View / First-Person View**.
The client-wide preference is `[Camera] ViewMode` in the existing `config.ini`
beside the executable: `0` is Classic (also the missing/invalid default), `1`
is First-Person. Changes save immediately. Menus and temporary camera states do
not change this preference. No OpenMU code, configuration or protocol changes
are required or included.

## Controls and behavior

Classic keeps the existing camera, shortcuts, cursor, click-to-move and actions.
First-Person zooms from the continuously updated classic camera to eye level in
0.65 seconds; switching back zooms out to the current classic view. Loading,
death, teleportation and cinematics suspend first-person operation; a ready
world applies the saved preference again. Looking uses relative mouse motion,
full yaw and pitch limited to 80 degrees above/below horizontal.

In first-person gameplay:

- ↑/↓ move forward/backward; ←/→ strafe relative to horizontal camera yaw.
  Combined keys and rotated views select one of eight neighbouring tiles.
- Movement uses the original pathfinder, `SendMove`, `MovePath`, speed and
  collision rules. Requests require the previous step to finish and are at least
  100 ms apart. A blocked neighbour does not start a detour. Releasing or turning
  waits for the current tile step, including its normal animation, to finish.
  No new cancellation or rotation protocol is introduced.
- The crosshair and picking ray share the world viewport centre. LMB uses the
  existing monster/NPC/item/object handlers; empty-ground LMB cannot start a
  click-to-move path. RMB uses the existing selected-skill handler, including
  its original targeted/area rules and resource/range checks.
- Open a window with its existing shortcut, or **hold Alt to release the
  pointer** for HUD buttons. Options, inventory, character windows, NPC/shop
  dialogs and other registered windows release capture. Chat/control focus
  prevents movement. Close the window/release Alt to resume. Held movement
  keys/buttons must be released before they can activate again.
- Gameplay requests SDL relative mouse mode for continuous turning and pointer
  confinement. Alt, menus and focus loss release it; returning to gameplay
  reacquires it. Keyboard focus is never grabbed, so Alt-Tab remains available.
  Editor windows do not acquire gameplay capture. The normal game cursor returns
  while UI owns input; the crosshair hides. **WSLg mouse confinement is currently
  unresolved**; see the platform limitation below.
- The camera follows the animated head position, including riding poses and
  jumping skill animations such as Hell Fire. A small upward offset puts the
  view near eye level. Head animation changes position; mouse input controls aim.
- **F10** unlocks/locks mouse-wheel zoom, as in Classic. While gameplay has
  captured the pointer, scroll down to pull behind the player into third person;
  scroll up to return to the head. Zoom moves 50 world units per notch, from
  0 to 600 units along the viewing direction. The character's body/equipment
  becomes visible at 50 units. Arrow movement and mouse look remain active.
  **F11** returns this camera to the head position and horizontal pitch. Its distance is independent
  of Classic zoom, survives view/map switches, and resets when the client closes.
  While a UI window or Alt owns the pointer, scrolling belongs to the UI.
- The local character's animation and body shadow still run at head distance;
  its visible body is omitted there. Other characters retain their original
  rendering and selection.

WASD retains its original shortcuts in both modes. Inventory remains accessible
with I/V and character information with C. Arrow keys belong to focused UI
controls/chat while a window or text field owns input.

## Integration and tuning

- `Data/GameConfig/GameConfig*`: local preference, validation/default and save.
- `UI/Options/OptionWindow*`, both themes' `option_window.rml`,
  `Localization/Game.en.resx`: live setting and English localization fallback.
- `Camera/FirstPersonCamera*`, `CameraManager*`: classic-backed interpolation,
  lifecycle, projection and frustum. Optional `[Camera]` INI keys (read at startup) tune
  `FirstPersonFov=90` (45–120 degrees at 4:3), `FirstPersonHeadOffset=12`
  (−30–60 world units × model scale above the animated head),
  `FirstPersonEyeHeight=180` (40–300 world units × model scale, used only when
  a valid animated head is unavailable), `FirstPersonSensitivityMilliDegrees=150`
  (1–1000, divided by 1000 for degrees/pixel), `FirstPersonPitchLimit=80`
  (10–85 degrees) and `FirstPersonTransitionMs=650` (100–3000 ms). Values are
  clamped. Normal saves preserve these manually edited keys. The near plane
  remains a named 5-unit constant in `FirstPersonCamera.cpp`.
- `Input/FirstPersonInput*`, `App/Platform/Windows/Winmain.cpp` (shared SDL pump
  on Linux too), `UI/Core/WindowSystem*` and `WindowCommon.cpp`: capture,
  release/arming, UI precedence and shortcut ownership.
- `Input/FirstPersonMovement*`, `FirstPersonDirection.cpp`, `Engine/Object/ZzzInterface.cpp`: adjacent tile
  requests through original movement/action eligibility. `RequestIntervalMs`
  sets the minimum request interval. Direction quantization ignores pitch and
  input magnitude; the existing movement pipeline determines diagonal speed.
- `Scenes/MainScene.cpp`, `Input/Selection.cpp`: centre picking using the
  existing ray/selection implementation, without selecting the local hero.
- `UI/HUD/FirstPersonCrosshair*`, `first_person_crosshair.rml and both themes’ first_person_crosshair.rcss` and Rml
  runtime/stacking hooks: themeable, noninteractive aiming overlay.
- `Engine/Object/ZzzCharacter.cpp`: local body omission after animation/shadow.
  `ZzzObject.cpp` and the camera's orbital accessor tolerate the alternate view.

## Build and launch (WSL/Linux)

The shell for this work is Linux in `/home/artur/src/MuMain`. Docker Compose
under `tools/InGameTests/` runs a disposable OpenMU test server from a pinned
image; it neither builds the client nor mounts its source. It was not started,
reset or changed for this work. Use the native client build:

```sh
cd /home/artur/src/MuMain
cmake --preset linux-x64 -DMU_COPY_RUNTIME_ASSETS=ON
cmake --build --preset linux-x64-release --target Main -j 8
cd out/build/linux-x64/src/Release
./Main
```

Keep the existing compatible server address in the runtime `config.ini`.
The `.so` network library must be beside `Main`. For a Windows executable use
`docs/build/windows/wsl.md` or a native Windows preset; Linux .NET cannot
produce the Windows Native AOT DLL. Do not rebuild/reset the server for this
feature. See `docs/build/linux/console.md` for platform dependencies.

Runtime asset copying is ON in the local build tree, so the Release directory
is runnable directly after building. Source assets live in `src/bin/Data` and `src/bin/fonts`.

## Validation record

Verified in this session:

```sh
cmake --build --preset linux-x64-release --target Main test_first_person_direction test_first_person_input rml_stacking_order_tests -j 8
ctest --test-dir out/build/linux-x64 -C Release -R 'First-person|Classic and character selection|every document|only the main scene|options window stands' --output-on-failure
python3 tools/check_rml_rcss_syntax.py
python3 tools/check_rml_rcss_drift.py
python3 tools/check_rml_bound_geometry.py
python3 tools/check_layout_transform_users.py
```

The Linux Release client executable and Native AOT network library built. The
10 selected CTest cases passed, including the new arrow-ownership regression
tests; direction tests contain 60 passing assertions.
All four UI guards passed (146 RML documents, 242 stylesheets; contract coverage
113 documents / 339 theme variants). The first sandboxed .NET build failed to
create its task host; running the same build outside the sandbox resolved it.
Existing compiler warnings remain. No Windows build was attempted.

A fresh isolated runtime was assembled at
`/tmp/mumain-first-person-qrffmtr3` with copied binaries/config and symlinks to
the source Data/fonts and built shaders. Its timed startup command was:

```sh
cd /tmp/mumain-first-person-qrffmtr3
timeout --signal=INT --kill-after=5s 20s ./Main
```

It initialized rendering, loaded the network library, received the existing local
server's server list, and logged reaching character selection. The timed run
ended with exit 124, as expected. Logs reported unavailable sound/music assets,
a missing icon in the temporary runtime and RmlUi inline-property warnings.
This establishes startup/network smoke coverage, **not first-person world
behavior**. No gameplay controls, Options interaction, combat or respawn were
scripted/verified. Server services/configuration were not changed.

The temporary runtime above records the initial smoke test. Use the repository
build/run commands above for the current arrow-key version; its config is
separate from the old temporary copy. Compilation and automated
tests do not establish gameplay correctness.

## Manual acceptance walkthrough

All gameplay items below are **implemented but not runtime-tested** unless a
later validation entry explicitly says otherwise:

| Area | Checks |
| --- | --- |
| Settings | Both exact View Mode choices; missing/invalid defaults; local restart persistence; live switching; UI reflects preference; menus retain preference |
| Classic regression | Original camera/cursor, click-to-move, targeting, actions and shortcuts; no capture, crosshair or arrow-key translation |
| Camera | One world-entry transition; interrupted/reversed switching; animated head tracking while walking, riding Uniria/Dinorant/Dark Horse/Fenrir and casting Hell Fire; F10 wheel zoom and F11 return; body/equipment visibility when pulled back; Classic zoom preserved; stationary/full-yaw/clamped-pitch look |
| Ownership | Cursor/crosshair and capture with Options, inventory, chat, shops, NPC dialogs, Alt, focus loss and return; held-key/button release |
| Lifecycle | Map changes, teleportation, death/respawn and character selection/re-entry |
| Movement | Mathematical yaw/strafe/diagonal mapping **verified** by unit tests; held movement, turning while walking, release, walls/corners, request cadence, reconciliation and animation **implemented but not runtime-tested** |
| Actions | Monsters, NPCs, items, supported objects, targeted/area RMB skills, range/move-to-target behavior, empty-ground LMB and UI click-through |

1. Start with `ViewMode=0` (or no key), log in, and confirm Classic controls.
2. Select First-Person in Options, close Options, release keys/buttons and
   observe the zoom and capture. Look while stationary, walk all directions,
   turn while holding ↑, release, and test walls/corners.
3. Test a monster with LMB and selected targeted/area skills with RMB; then an
   NPC, a dropped item and an operable object. Click empty ground.
4. Open inventory, character, chat, shops and Options. Check pointer ownership,
   no simultaneous attacks/casts, no stuck keys, and Alt-based HUD access.
5. Switch to Classic and retest normal movement/actions/shortcuts. Switch back,
   restart and verify the preference and single world-entry transition.
6. Test teleport/map change, respawn and Alt-Tab. Check packet cadence and
   movement reconciliation against an unchanged compatible server.

## Known limits

- This is a prototype until the above server-dependent walkthrough is completed.
  Short paths may show tile-boundary pauses; no claim of FPS-style stopping or
  verified jitter-free movement is made.
- Original selection uses entity/object bounding boxes and existing priority
  rules. It has no new geometry-depth occlusion test; aiming through scenery,
  very close entities, elevated items and tall-object boxes need live testing.
- Existing move-to-target actions can retain their original longer paths;
  directional keys take over after such a path finishes. Their range rules
  and skill ground-projection behavior are unchanged.
- Animated head tracking includes positional bobbing and sudden skill motion.
  It uses the current character animation's head; transformations without a
  configured head use the fallback eye height. There is no camera collision
  sweep: walls, ceilings, and attachments/effects can clip, particularly when
  zoomed behind the player. Mount-specific clearance needs an in-game check.
- Looking does not send stand/rotation packets. Facing follows original
  movement and action semantics; strafing does not introduce a new server pose.
- First-person gameplay camera is suspended for editor FreeFly and cinematics.
  UI scale/theme/resolution changes require the manual layout sweep described
  in `docs/rmlui-ui-system/building-new-ui.md`.

## Input activation correction

The first implementation incorrectly used `!InputEnable` to release mouse
capture. That legacy text-entry flag is false in normal main-scene gameplay;
it is not a gameplay permission flag. The controller now uses window/text
focus, modal UI, focus loss and explicit pointer release to decide ownership.
Movement uses the arrow keys. A regression test exercises arrow ownership with
`InputEnable=false`, and confirms text/message UI and Classic mode retain keys.
Mouse capture failures are reported in the input log once per failure episode.

The arrow-key correction was rebuilt with runtime assets enabled using the
build/run commands above. The client and input test executable compiled, all
10 selected tests passed, and the UI guards passed during the build. Live mouse
capture and arrow movement require an in-game retest using the rebuilt Release
executable. Close any older temporary client before launching this version.

## Animated head and independent zoom refinement

The camera now anchors to the animated head with a 12-unit upward offset,
scaled with the character. Riding poses and Hell Fire jumps move the camera;
mouse input still controls its orientation. Models without a valid animated
head use the 180-unit fallback height. Adjust `FirstPersonHeadOffset` in the
runtime `[Camera]` section and restart to fine-tune clearance.

F10-unlocked scrolling now moves this view between head distance and third
person, independently of Classic zoom. The visible body/equipment returns
when pulled back; F11 returns to head distance. This runtime distance survives
map and view changes but is not saved across client restarts.

Validation: the Linux Release client built, and 15 selected tests passed.
These cover input/direction, UI stacking, head tracking/fallback, independent
zoom locking/bounds/reset, and the shipped player animation file. The real
Uniria/Dinorant riding, Dark Horse and Fenrir poses elevate the anchor, and the
Hell Fire sequence raises it during the jump. This is automated animation-data
coverage; visual mount clearance, motion comfort and in-game zoom still need
manual testing. No camera collision sweep was added.

## WSLg pointer capture limitation and recovery

The attempted extra window grab and one-pixel centre confinement did not fix
pointer escape in the user's WSLg test and made mouse look worse. Those changes
have been removed. Passing the earlier unit tests did not validate desktop
confinement.

The client now discards a motion event if handling it acquires or reacquires
capture, so a movement generated in UI mode cannot immediately turn the camera.
Invalid non-finite deltas are also ignored. F11 resets both camera distance and
pitch to the horizontal head view. Initial capture/backend information and a
small number of motion samples are written to `MuError.log` beside `Main` to
support diagnosis.

The Linux Release client rebuilt and all 11 selected first-person tests passed,
including capture-transition filtering and a camera update test that resets
pitch with F11 and then resumes looking. These are application tests, not proof
of working pointer confinement through WSLg.

In the subsequent user-assisted runtime test, the user confirmed mouse look
works again under X11, but the Windows pointer still escapes. The log records
`driver=x11`, `relative=true`, and ordinary small mouse deltas. A second run
using `SDL_VIDEO_DRIVER=wayland ./Main` reported `relative=true` but no logged
motion samples; the user confirmed mouse look fails and the pointer escapes.
Neither backend provides working confinement in this WSLg environment.

The symptoms match [WSLg issue #240](https://github.com/microsoft/wslg/issues/240),
which describes chaotic relative movement and failure to lock the cursor in
first-person games. The tests establish that SDL reporting capture success is
insufficient here; they do not prove the precise failure inside WSLg.
For the recovered X11 mouse-look behavior, use:

```sh
cd /home/artur/src/MuMain/out/build/linux-x64/src/Release
SDL_VIDEO_DRIVER=x11 ./Main
```

Reliable pointer confinement remains unresolved. A native Windows client would
bypass WSLg's mouse path. Windows Visual Studio C++ Build Tools and the .NET
runtime are installed locally, but `dotnet.exe --list-sdks` reports no SDKs;
a playable Windows build requires the Windows .NET 10 SDK for its network DLL.
No native Windows build has been validated in this session.
