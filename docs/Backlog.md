# Backlog

Draft of epics, stories and tasks, to be turned into GitHub issues. Roadmap source of
truth: [DESIGN.md §10](../DESIGN.md).

## How this maps to GitHub

| Level | GitHub | Naming |
|---|---|---|
| **Epic** | a **Milestone** + a parent issue | `M3: MVP – move a cube` |
| **Story** | an issue (sub-issue of the epic) | something you can *see or do* when done |
| **Task** | checklist item in the story | a step of work |
| **Bug** | an issue with `type:bug` | what happened, what should have, steps to reproduce |

## Status

Every story says how complete it is:

| Status | Means | GitHub |
|---|---|---|
| **Ready** | Specific; can be started | — |
| **Draft** | Outline only; **Open:** names what's missing | label `status:draft` |
| **Spike** | Time-boxed investigation; delivers a decision + ready stories | label `type:spike` |

---

## M3 — MVP: move a cube

### 3.1 The world is seen from a fixed isometric camera · `area:camera` ✅
**Acceptance**
- [x] Orthographic projection: the cube is the same size anywhere on the grid
- [x] Locked angle — yaw 45°, pitch 30°; no rotation, no zoom
- [x] The whole grid fits in the window at 1280×720; resizing doesn't stretch it
- [x] Right-click picking still lands exactly under the cursor

**Tasks**
- [x] Orthographic mode in `Camera` (view height in world units)
- [x] Remove orbit state: yaw/distance interpolation, `Rotate`, `Zoom`
- [x] Remove rotate/zoom bindings from the Sandbox
- [x] Tune pitch and view height

### 3.2 Right-click makes the cube walk there · `area:input` ✅
**Acceptance**
- [x] Right-click on the ground: the cube walks to that point in a straight line at a
      constant speed and stops on it
- [x] Clicking again while walking redirects it immediately
- [x] Movement is smooth at any frame rate (interpolated by `alpha`)
- [x] Arrival is decided on simulation state, never on the interpolated position
- [x] Clicking the sky does nothing

**Tasks**
- [x] Cube position: previous + current, advanced in `OnUpdate`
- [x] Walk speed constant; snap to target when within one step
- [x] Marker shows the destination, hidden on arrival
- [x] Button messages update the cursor position (a click with no prior mouse move was
      cast from a stale position)

### 3.3 The cube reads as a cube · `area:renderer` ✅
**Acceptance**
- [x] Each face has a visibly different brightness from one fixed light direction
- [x] The grid stays unlit

**Tasks**
- [x] Vertex format gains a normal; cube becomes 24 vertices (4 per face)
- [x] Lighting constant buffer (light direction)
- [x] Lambert lighting in the shader

---

## M4 — Foundations

Stories are GitHub issues #1–#7: [milestones](https://github.com/MrThyssen/BlackwaterEngine/milestones).

---

## Camera

Built. Unticked checks are still to verify by hand; the direction tests wait for #1.

### C.1 The ground grid has no visible edge · `area:renderer`
Built; to verify by hand.

**Acceptance**
- [ ] Panning ±100 units in any direction never shows the grid's edge, at any window size
- [x] Grid lines stay on whole world units while the camera moves; they don't drift
- [x] The red X and blue Z axes stay at the world origin

**Tasks**
- [x] Grid mesh of ±50 units, drawn at the camera target snapped to whole units
- [x] Axes as their own mesh, fixed at the origin

### C.2 The camera pans with the arrow keys and the screen edges · `area:camera`
Built; to verify by hand.
Depends on C.1.

**Acceptance**
- [x] Holding an arrow key pans the view that way on screen (↑ shows more of the top of the screen) at 12 units/s
- [ ] The cursor within 8 px of a window edge pans that way; in a corner, diagonally
- [ ] Two directions at once are not faster than one
- [ ] Panning is smooth at any frame rate
- [x] Right-click lands under the cursor while panned and while panning
- [ ] No panning while the window is unfocused or the cursor is outside it
- [ ] The view can't pan past ±100 units

**Tasks**
- [x] `Camera`: movable target with previous + current, interpolated like the cube
- [x] Screen-up and screen-right as ground directions, from the yaw
- [ ] Tests for the pan directions (needs #1)
- [x] Window reports when the cursor leaves (`WM_MOUSELEAVE`; `PORT(M5)`)
- [x] Settings: pan speed, edge width, bounds

---

## M5 — Cross-platform

5.1–5.4 are done and verified on Windows; 5.5–5.7 need a Mac.

### MVP checklist
Every refactor keeps this. Stories refer to it instead of saying "works as before".
1. Window opens at 1280×720; title shows fps, 60 ups and the debug-layer state
2. 20×20 grid with the red X axis and blue Z axis through the origin
3. Cube shows three shades: top brightest, left medium, front darkest
4. Right-click on the ground: the cube walks there at 4 units/s in a straight line, smoothly,
   and stops on it; the marker shows the target and disappears on arrival
5. Right-click on the sky does nothing
6. Resize, minimise and restore work without stretching or errors; Alt+F4 closes
7. No D3D11 debug-layer errors or warnings

### 5.1 The engine draws through its own interface (RHI) · `area:renderer`
Ready.
Depends on #1.

**Acceptance**
- [ ] MVP checklist passes
- [ ] `Main.cpp` includes no D3D11 header and uses no `D3D11_` or `DXGI_` type
- [ ] All D3D11 code lives in `Blackwater.RHI.D3D11`; `Blackwater.Core` doesn't link `d3d11`
- [ ] All tests pass

**Tasks**
- [ ] Interface derived from what we use today: buffers, shaders, vertex layout, states, draw, clear, present
- [ ] Engine vertex-layout description replaces `D3D11_INPUT_ELEMENT_DESC`
- [ ] `git grep "PORT(M5)"` as the checklist for the renderer

### 5.2 Text is UTF-8 everywhere · `area:core`
Ready.
**Acceptance**
- [ ] With the title set to `Blackwater – æøå`, the window shows exactly that
- [ ] A shader path through a folder named `æøå` loads
- [ ] `git grep wchar_t` finds it only in Windows platform files

**Tasks**
- [ ] UTF-8 ↔ UTF-16 conversion at the Windows API boundary only
- [ ] Titles, paths and messages become `std::string` / `std::filesystem::path`
- [ ] Round-trip conversion tests (æøå, emoji, empty string)

### 5.3 The Sandbox starts from any folder · `area:core`
Ready.
**Acceptance**
- [ ] The Sandbox finds its shaders when started by double-click, from Rider, and from a
      terminal in `C:\`
- [ ] A missing shader file shows a message box naming the full path it tried

**Tasks**
- [ ] File-system module over `std::filesystem`; base path = the executable's folder
      (`PORT(M5)`: the app bundle on macOS)
- [ ] Replaces `ExecutableDirectory()` in `Shader.cpp`

### 5.4 Window and input run on SDL3 · `area:input`
Ready. SDL3 comes in through `FetchContent`, pinned to an exact version.
Depends on 5.2.

**Acceptance**
- [ ] MVP checklist passes
- [ ] Input tests pass; keys are the engine's `Key` enum, not VK codes
- [ ] A start-up failure (e.g. a broken shader) shows a message box with the error
- [ ] `Window.cpp`'s Win32 window procedure is gone

**Tasks**
- [ ] SDL3 added to the build (vendored vs `FetchContent` — decide)
- [ ] SDL window's native handle feeds the D3D11 swap chain
- [ ] SDL events feed `Input`; SDL entry point; `SDL_ShowSimpleMessageBox`

### 5.5 Spike: verify how shaders reach Metal · `area:build` `type:spike`
Spike: verify DXC → SPIR-V → SPIRV-Cross → MSL on a Mac.
Time-box: 2 sessions. Needs a Mac.

**Acceptance**
- [ ] `Basic.hlsl` converted to Metal's shader language and compiled on a Mac, by the chosen tool
- [ ] Decision written in DESIGN.md §12 (e.g. DXC → SPIR-V → SPIRV-Cross, or Apple's Metal
      shader converter)
- [ ] 5.6 and 5.7 rewritten with what was learned

### 5.6 The engine builds and tests on macOS · `area:build`
**Draft.** Open: the shader tool and how it installs on the macOS runner (from 5.5).
Depends on 5.1, 5.4, 5.5.

**Acceptance**
- [ ] On a Mac: configure + build with clang succeeds with no warnings
- [ ] `ctest` passes on macOS
- [ ] Every pull request is built and tested on Windows and macOS

**Tasks**
- [ ] Per-compiler flags in `Blackwater.Common`; macOS presets
- [ ] Shader conversion as a build step; macOS job in CI

### 5.7 The MVP runs on a Mac · `area:renderer`
**Draft.** Open: how our constant-buffer slots map to Metal (from 5.5).
Depends on 5.6.

**Acceptance**
- [ ] MVP checklist passes on a Mac (Metal replaces "D3D11 debug layer" in point 7)
- [ ] On a Retina display, the cube walks to exactly the clicked point

**Tasks**
- [ ] `Blackwater.RHI.Metal` via metal-cpp; Metal layer from the SDL window
- [ ] Mouse points vs drawable pixels (Retina scale) in picking

---

## M6 — Debug tools

Built after M5, so once: on SDL3, with D3D11 and Metal backends.

### 6.1 A debug overlay runs inside the game · `area:debug`
**Draft.** Open: the RHI's shape (from 5.1).
- [ ] Dear ImGui with its SDL3, D3D11 and Metal backends
- [ ] Toggle with a key; fps/ups and frame-time graph; toggles for debug drawing
- [ ] Console with a first command or two

### 6.2 Anything can be drawn for debugging · `area:debug` `area:renderer`
**Draft.** Open: the RHI's shape (from 5.1).
- [ ] Debug-draw API: line, circle, sphere, box, arrow, in world space
- [ ] With or without depth testing; lasts one frame or N seconds
- [ ] First users: the picking ray, the walk target

### 6.3 The game can be paused and slowed down · `area:debug`
**Draft.** Open: not refined yet.
- [ ] Game clock separate from real time: pause, step one frame, time scale
- [ ] Simulation uses game time; the debug overlay keeps running while paused

### 6.4 The engine takes command-line arguments · `area:debug`
**Draft.** Open: not refined yet.
- [ ] `--no-vsync`, `--log=<level>`, `--windowed`
- [ ] Unknown arguments are reported, not ignored

---

## Editor track

**Avalonia** (cross-platform .NET), Warcraft 3–style: the whole world is built in it. Each
editor milestone follows the engine system it edits.

### E0 The editor starts on Windows and macOS · `area:editor`
**Draft.** Open: docking library for Avalonia.
- [ ] `Blackwater.Editor` recreated as an Avalonia app; WPF project removed
- [ ] Window shell: menu, docking layout, empty panels; builds with `dotnet`

### E1 The editor shows the live engine · `area:editor` *(after M5)*
**Draft.** Open: the Interop API; needs M5.
- [ ] `Blackwater.Interop`: small C ABI DLL (create engine, render into a native view, resize)
- [ ] Viewport through Avalonia `NativeControlHost` (HWND / NSView)
- [ ] *Test map* launches the game on the open map

### E2 Units and abilities are edited as tables · `area:editor` *(with M7–M8)*
**Draft.** Open: the data format (M7).
- [ ] Object editor over the data files: units, abilities, items
- [ ] Changes hot-reload into the running game

### E3 Maps are built by placing things · `area:editor` *(after M7)*
**Draft.** Open: the map format (M7).
- [ ] Place, move, delete units and doodads in the viewport
- [ ] Save and load maps in the scene format

| Later | Editor | After |
|---|---|---|
| E4 | Asset browser: create, rename, move; references survive (GUIDs) | M10 |
| E5 | Terrain editor: height, texture painting | M12 |
| E6 | Trigger editor: event → condition → action | M12 |
| E7 | AI editor | M9, M12 |

---

## Later epics

To be broken into stories when they come up.

| Epic | Outcome |
|---|---|
| M7 Data & game objects | Game object model (decide: components vs ECS); generational handles; units and abilities in data files, hot-reloaded; hashed string IDs; scene save/load; config file |
| M8 Combat prototype (cubes) | One hero, 4 abilities from data, hotbar, cooldowns; a boss with telegraphed **AoE shapes** (circle, cone, rectangle) resolved on **impact**; health, mana; seeded RNG; combat events + log + damage meter; **sound** (miniaudio behind `AudioEngine`); ImGui HUD; `Trinity` target. *Fun checkpoint 1* |
| M9 Raid prototype (cubes) | Three heroes, selection, follow; **soldiers** (follow / hold / attack, corpses); minimal threat list + boss target rules; enemy AI and **danger map**; boss phases (state machines) + boss warnings; pull → wipe/kill → reset. *Fun checkpoint 2* |
| M10 Assets & models | Resource manager; GUID sidecars; **SQLite asset database**; glTF meshes, textures, materials; shadows |
| M11 Animation | Skeletons, skinning, quaternions, idle and walk |
| M12 World | Terrain; walkability, A\*, collision; **line of sight**; **trigger system** + trigger volumes; camera follow |
| M13 UI | Text rendering; HUD, hotbar, hero portraits, boss warnings |
| M14 RPG & story | Stats, items, dialogue, quests, save/load |
