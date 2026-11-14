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

### 3.3 The cube reads as a cube · `area:renderer`
**Acceptance**
- [ ] Each face has a visibly different brightness from one fixed light direction
- [ ] The grid stays unlit

**Tasks**
- [ ] Vertex format gains a normal; cube becomes 24 vertices (4 per face)
- [ ] Lighting constant buffer (light direction)
- [ ] Lambert lighting in the shader

---

## M4 — Tests, collaboration & debug tools

Debug tools land right after the MVP: everything after them is easier to build and to test.

### 4.1 Tests run with one command and show in Rider · `area:tests`
- [ ] Choose framework (doctest vs hand-rolled)
- [ ] `Blackwater.Tests` target, registered with CTest
- [ ] `ctest --preset …` runs them; Rider's test view shows them

### 4.2 Input behaves correctly under the fixed timestep · `area:tests` `area:input`
- [ ] Press seen by exactly one update; quick click (down+up between steps) still seen
- [ ] Autorepeat doesn't re-trigger a press; focus loss releases everything

### 4.3 Picking maths is proven · `area:tests` `area:camera`
- [ ] Ray vs ground: hit, parallel, behind
- [ ] Screen centre maps to the camera target on the ground

### 4.4 Every pull request is built and tested automatically · `area:build`
- [ ] GitHub Actions: Windows build + tests on each PR
- [ ] Badge in README

### 4.5 The repo is ready for a second developer · `area:docs`
- [ ] Licence chosen and added
- [ ] `main` protected (PR required)
- [ ] Labels, milestones and boards created

### 4.6 The Sandbox plays sound · `area:audio`
- [ ] miniaudio backend behind the existing `AudioEngine` API
- [ ] Click on right-click, music on start, category volumes work

### 4.7 The engine logs what it does · `area:debug`
- [ ] Levels (error, warning, info, verbose) and categories (render, input, audio, …)
- [ ] Output to the debugger (`OutputDebugString` on Windows, stderr on macOS) and a log file
- [ ] `BW_CHECK` failures are logged before they throw

### 4.8 Anything can be drawn for debugging · `area:debug` `area:renderer`
- [ ] Debug-draw API: line, circle, sphere, box, arrow, in world space
- [ ] With or without depth testing; lasts one frame or N seconds
- [ ] First users: the picking ray, the walk target

### 4.9 A debug overlay runs inside the game · `area:debug`
- [ ] Dear ImGui with its D3D11 and Win32 backends (SDL3 + Metal backends at M5)
- [ ] Toggle with a key; fps/ups and frame-time graph; toggles for debug drawing
- [ ] Console with a first command or two

### 4.10 The game can be paused and slowed down · `area:debug`
- [ ] Game clock separate from real time: pause, step one frame, time scale
- [ ] Simulation uses game time; the debug overlay keeps running while paused

### 4.11 The engine takes command-line arguments · `area:debug`
- [ ] `--no-vsync`, `--log=<level>`, `--windowed`
- [ ] Unknown arguments are reported, not ignored

### 4.12 Crashes and leaks leave evidence · `area:debug`
- [ ] Unhandled exceptions write a minidump next to the executable
- [ ] Debug builds report CRT memory leaks and D3D11 live objects on exit

---

## M5 — Cross-platform

### 5.1 The engine draws through its own interface · `area:renderer`
- [ ] RHI interface derived from what the engine actually uses today
- [ ] D3D11 code moved into `Blackwater.RHI.D3D11`
- [ ] Sandbox contains no D3D types (`git grep PORT(M5)` is the checklist)

### 5.2 Window and input run on SDL3 · `area:input`
- [ ] SDL3 window replaces Win32; engine `Key` enum replaces VK codes
- [ ] Portable entry point and error message box
- [ ] Existing Input tests still pass

### 5.3 The engine builds on macOS · `area:build`
- [ ] Per-compiler flags; macOS presets; portable modules build with clang
- [ ] Shaders cross-compiled HLSL → MSL at build time
- [ ] CI gains a macOS job

### 5.4 The MVP runs on a Mac · `area:renderer`
- [ ] Metal backend via metal-cpp
- [ ] Same Sandbox, same behaviour as on Windows

### 5.5 Text is UTF-8 everywhere · `area:core`
- [ ] All engine strings are UTF-8 `std::string`; no `wchar_t` outside platform code
- [ ] Danish test string (æøå) round-trips through window title, log and file paths

### 5.6 Files are found the same way on both platforms · `area:core`
- [ ] File-system module over `std::filesystem`
- [ ] Base path from the executable (Windows) / app bundle (macOS), never the working directory
- [ ] Shader loading uses it (replaces `ExecutableDirectory()` in `Shader.cpp`)

---

## Editor track

The editor is **Avalonia** (cross-platform .NET), replacing the untouched WPF template.

### E0 The editor starts on Windows and macOS · `area:editor`
- [ ] `Blackwater.Editor` recreated as an Avalonia app; WPF project removed
- [ ] Window shell: menu, docking layout, empty panels; builds with `dotnet`

### E1 The editor shows the live engine · `area:editor` *(after M5)*
- [ ] `Blackwater.Interop`: small C ABI DLL (create engine, render into a native view, resize)
- [ ] Viewport through Avalonia `NativeControlHost` (HWND / NSView)

### E2 Assets can be browsed and managed · `area:editor` *(with M6)*
- [ ] Asset browser on top of the asset database
- [ ] Create, delete, rename, move assets; references survive moves (GUIDs)

---

## Later epics

To be broken into stories when they come up.

| Epic | Outcome |
|---|---|
| M6 Assets & models | Resource manager (load once, share, lifetimes); hashed string IDs; GUID sidecars; **SQLite asset database** as a derived, rebuildable index; config file; glTF meshes, textures, materials |
| M7 Animation | Skeletons, skinning, quaternions, idle and walk |
| M8 World | Game object model (decide: components vs ECS); walkability grid, A\*, collision; **trigger volumes**; simple level loading; camera follow; **camera panning** (mouse at screen edges, arrow keys); `Trinity` target |
| M9 Combat | Ability hotbar, cooldowns, an enemy; **AoE shapes** (circle, cone, rectangle); combat events + combat log; state machines for abilities and boss phases |
| M10 Party | Three heroes, selection, follow AI |
| M11 RPG & story | Stats, items, dialogue, quest flags, save/load |
