# Engine Systems

Which engine systems Blackwater builds, when, and which it skips — triaged against what
Trinity actually needs. Source list: Jason Gregory, *Game Engine Architecture*.

**Legend:** ✅ done · 🔜 planned (milestone) · ⏳ later · ❌ skip

Two facts shape most verdicts: **raids are instances** (load a level, play, leave — no open
world, no streaming), and **the team is two people** (anything that mainly solves
team-scale problems costs more than it saves). One fact pushes the other way: raid
mechanics must be **seen** to be debugged, so debug tools come early.

## Start-up and shut-down

| System | Verdict | Notes |
|---|---|---|
| Subsystem order | ✅ | No singletons: `Application` owns subsystems as members; C++ constructs in declaration order, destroys in reverse |
| Log manager | 🔜 M4 | |
| Controller manager | ✅ / ⏳ | That's `Input`; gamepad later |
| Scene manager | 🔜 M8 | |
| Mesh / material managers | 🔜 M6 | One resource manager, not one per type |
| Particle system manager | ⏳ | With VFX, after combat |
| Crash handler | ⏳ | Minidump on unhandled exception (Windows) |
| Dynamic library manager | ❌ | Static libraries by design |
| Archive (pack files) | ❌ | Loose files; revisit only if shipping |

## Memory

| System | Verdict | Notes |
|---|---|---|
| Frame (linear) allocator | ⏳ | Per-frame scratch. Only when profiling shows allocation cost |
| Pool allocator | ⏳ | Particles, projectiles, combat events |
| Double-buffered allocator | ⏳ | Data handed from one frame to the next |
| Double-ended stack allocator | ❌ | Console-era level loading |

## Containers, strings, text

| System | Verdict | Notes |
|---|---|---|
| Containers | ✅ standard library | Own containers only when profiling demands. **No Boost** |
| Hashed string IDs | 🔜 M6 | `constexpr` hash of names → 32/64-bit IDs |
| Unicode | 🔜 M5 | **UTF-8 everywhere**; UTF-16 only at the Windows API boundary. `wchar_t` is 16-bit on Windows, 32-bit on macOS |
| Localization | ⏳ | String table keyed by ID once there is player-facing text; æøå need only UTF-8 and a font with the glyphs |

## Configuration, CLI, file system

| System | Verdict | Notes |
|---|---|---|
| Config file | 🔜 M6 | Resolution, vsync, volumes, keybinds (JSON) |
| Command-line arguments | 🔜 M4 | `--no-vsync`, `--log=…`, later `--level=…` |
| In-game console | 🔜 M4 | Dear ImGui |
| File system | 🔜 M5 | Over `std::filesystem` (handles `\` vs `/`). Paths resolve from the executable / app bundle, never the working directory |
| Async I/O | ❌ | Only needed for streaming |

## Resources

| System | Verdict | Notes |
|---|---|---|
| Runtime resource manager | 🔜 M6 | Load once, share, reference-counted lifetimes, composite resources |
| Resource GUIDs | 🔜 M6 | In `.meta` sidecars; references by GUID, not path |
| Asset database | 🔜 M6 | **SQLite**, a derived and rebuildable index — never the source of truth (see DESIGN.md §8) |

## Game loop and time

| System | Verdict | Notes |
|---|---|---|
| Message pump, fixed timestep, render loop | ✅ M0 | |
| Game clock (pause, time scale) | 🔜 M4 | Cooldowns and boss timers run on game time |
| Local timelines | 🔜 M7 | Animation clips |
| Event-based updates | 🔜 M9 | Combat events |
| Multithreaded game loop | ❌ | Single-threaded at this scale; audio has its own thread |

## Debug and development tools

| System | Verdict | Notes |
|---|---|---|
| Logging + tracing | 🔜 M4 | `OutputDebugString` on Windows, stderr on macOS, plus a log file |
| Debug drawing | 🔜 M4 | Lines, circles, spheres, boxes, arrows; with/without depth. Aggro radii, AoE zones, paths, rays |
| Debug UI, console | 🔜 M4 | **Dear ImGui** |
| Debug camera, pause | 🔜 M4 | Free camera in debug builds only |
| Cheats | ⏳ | Console commands |
| In-game profiling | 🔜 M4 | Frame graph in ImGui; **Tracy** for deep CPU profiling; RenderDoc/PIX for GPU |
| Memory stats, leak detection | 🔜 M4 | CRT debug heap + D3D11 live-object report |
| Screenshots | ⏳ | Back buffer → PNG |
| Movie capture | ❌ | Use OBS |

## Collision

See [Physics.md](Physics.md). 2D on the ground plane.

| System | Verdict | Notes |
|---|---|---|
| Characters vs world / each other | 🔜 M8 | Circles vs grid, circle vs circle |
| Trigger volumes | 🔜 M8 | Fire zones, room entry, phase boundaries |
| AoE shapes | 🔜 M9 | Circle, cone, rectangle (OBB + separating-axis theorem) |
| AABB broad phase | 🔜 M8 | |
| Continuous collision | ❌ | Simple sweeps for projectiles are enough |
| Rigid bodies, spring-mass, destructibles | ❌ | Not Trinity. If ever needed: Jolt Physics |

## Audio

See [Audio.md](Audio.md). Relevant: distance attenuation and screen-position panning.
Background only: wave physics and decibels. Skip: full 3D audio, propagation, polar
patterns.

## Parallelism and maths

| Topic | Verdict |
|---|---|
| Concurrency concepts, Flynn's taxonomy | Read, don't build |
| SIMD | ✅ via DirectXMath |
| GPGPU | ❌ (maybe compute-shader particles much later) |
| Vectors, matrices | ✅ |
| AABB → OBB → quaternions | 🔜 in that order (M4 debug draw → M9 AoE → M7 animation) |

## Gameplay foundation

| System | Verdict | Notes |
|---|---|---|
| Game object model | 🔜 M8 | Decide: components on game objects vs full ECS |
| Data-driven content | 🔜 | Abilities, bosses and encounters in JSON — new raid content shouldn't need new C++ |
| Unique IDs + handles | 🔜 M8 | Generational handles, not raw pointers |
| Messaging / events | 🔜 M9 | The event stream doubles as a **combat log** |
| State machines | 🔜 M9 | AI, ability states, **boss phases** |
| Encounter flow | 🔜 M9 | Pull → phases → wipe or kill → reset |
| Spawning, queries | 🔜 M8 | Spawn points; "enemies within 8 m" |
| Update order | 🔜 M8 | Fixed per step: input → AI → abilities → physics → animation |
| Scripting (Lua) | ⏳ | Boss mechanics, if JSON timelines stop being enough |
| Level loading | 🔜 M8 | One raid = one level file |
| World editor | ⏳ | Avalonia, E-track |
| Saved games | ⏳ M11 | |
| World streaming, chunks, binary object images | ❌ | Instances; JSON descriptors |
