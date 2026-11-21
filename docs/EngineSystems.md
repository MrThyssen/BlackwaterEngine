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
| Scene manager | 🔜 M7 | |
| Mesh / material managers | 🔜 M10 | One resource manager, not one per type |
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
| Hashed string IDs | 🔜 M7 | `constexpr` hash of names → 32/64-bit IDs |
| Unicode | 🔜 M5 | **UTF-8 everywhere**; UTF-16 only at the Windows API boundary. `wchar_t` is 16-bit on Windows, 32-bit on macOS |
| Localization | ⏳ | String table keyed by ID once there is player-facing text; æøå need only UTF-8 and a font with the glyphs |

## Configuration, CLI, file system

| System | Verdict | Notes |
|---|---|---|
| Config file | 🔜 M7 | Resolution, vsync, volumes, keybinds (JSON) |
| Command-line arguments | 🔜 M6 | `--no-vsync`, `--log=…`, later `--level=…` |
| In-game console | 🔜 M6 | Dear ImGui |
| File system | 🔜 M5 | Over `std::filesystem` (handles `\` vs `/`). Paths resolve from the executable / app bundle, never the working directory |
| Async I/O | ❌ | Only needed for streaming |

## Resources

| System | Verdict | Notes |
|---|---|---|
| Runtime resource manager | 🔜 M10 | Load once, share, reference-counted lifetimes, composite resources |
| Resource GUIDs | 🔜 M10 | In `.meta` sidecars; references by GUID, not path |
| Asset database | 🔜 M10 | **SQLite**, a derived and rebuildable index — never the source of truth (see DESIGN.md §8) |

## Game loop and time

| System | Verdict | Notes |
|---|---|---|
| Message pump, fixed timestep, render loop | ✅ M0 | |
| Game clock (pause, time scale) | 🔜 M6 | Cooldowns and boss timers run on game time |
| Local timelines | 🔜 M11 | Animation clips |
| Event-based updates | 🔜 M8 | Combat events |
| Multithreaded game loop | ❌ | Single-threaded at this scale; audio has its own thread |

## Debug and development tools

| System | Verdict | Notes |
|---|---|---|
| Logging + tracing | 🔜 M4 | `OutputDebugString` on Windows, stderr on macOS, plus a log file |
| Debug drawing | 🔜 M6 | Lines, circles, spheres, boxes, arrows; with/without depth. Aggro radii, AoE zones, paths, rays |
| Debug UI, console | 🔜 M6 | **Dear ImGui** |
| Debug camera, pause | 🔜 M6 | Free camera in debug builds only |
| Cheats | ⏳ | Console commands |
| In-game profiling | 🔜 M6 | Frame graph in ImGui; **Tracy** for deep CPU profiling; RenderDoc/PIX for GPU |
| Memory stats, leak detection | 🔜 M4 | CRT debug heap + D3D11 live-object report |
| Screenshots | ⏳ | Back buffer → PNG |
| Movie capture | ❌ | Use OBS |

## Collision

See [Physics.md](Physics.md). 2D on the ground plane.

| System | Verdict | Notes |
|---|---|---|
| Characters vs world / each other | 🔜 M12 | Circles vs grid, circle vs circle |
| Trigger volumes | 🔜 M12 | Fire zones, room entry, phase boundaries |
| AoE shapes | 🔜 M8 | Circle, cone, rectangle (OBB + separating-axis theorem) |
| AABB broad phase | 🔜 M12 | |
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
| AABB → OBB → quaternions | 🔜 in that order (M6 debug draw → M8 AoE → M11 animation) |

## Gameplay foundation

| System | Verdict | Notes |
|---|---|---|
| Game object model | 🔜 M7 | Decide: components on game objects vs full ECS |
| Data-driven content | 🔜 | Abilities, bosses and encounters in JSON — new raid content shouldn't need new C++ |
| Unique IDs + handles | 🔜 M7 | Generational handles, not raw pointers |
| Messaging / events | 🔜 M8 | The event stream doubles as a **combat log** |
| State machines | 🔜 M9 | AI, ability states, **boss phases** |
| Encounter flow | 🔜 M9 | Pull → phases → wipe or kill → reset |
| Spawning, queries | 🔜 M7 | Spawn points; "enemies within 8 m" |
| Update order | 🔜 M7 | Fixed per step: input → AI → abilities → physics → animation |
| Scripting (Lua) | ⏳ | Boss mechanics, if JSON timelines stop being enough |
| Level loading | 🔜 M7 | One raid = one map file (saved by the editor) |
| World editor | ⏳ | Avalonia, E-track |
| Saved games | ⏳ M14 | |
| World streaming, chunks, binary object images | ❌ | Instances; JSON descriptors |
