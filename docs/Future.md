# Future Development Ideas

## Work areas


| Area | What | Boundary | When |
|---|---|---|---|
| **Audio** | Sound engine (miniaudio, Windows + macOS) | `Blackwater.Audio` — [spec](Audio.md) | now |
| **Metal backend** | macOS renderer behind the RHI | `Blackwater.RHI.Metal` | M5 |
| **Physics / collision** | 2D collision, walkability, trigger volumes, AoE queries | `Blackwater.Physics` — [spec](Physics.md) | M8 |
| **CI** | GitHub Actions: build + run tests on every pull request | `.github/workflows/` | M4 |
| **Debug overlay / profiler** | Dear ImGui: frame graphs, console, debug-draw toggles, live state | Reads engine data, never changes it | M4 |
| **Lighting & shading** | Directional light → Blinn-Phong → shadows | Shaders + a lighting constant buffer | M3 → M6 |
| **UI / HUD** | Health bars, ability hotbar, raid frames (not ImGui) | Draws 2D on top of the frame | M9 |
| **Asset pipeline** | `.meta` sidecars, SQLite asset database, import/validate models and textures | Separate tool + editor | M6 |
| **Editor** | `Blackwater.Editor` — **Avalonia**, Windows + macOS | Its own .NET project | E0 anytime |
| **Lua scripting** | Gameplay scripts, triggers | Embedded Lua behind a small API | later |
| **Particles / VFX** | Spell effects, dust, sparks | Its own renderer pass | after combat |
| **Gamepad** | XInput controller support | Feeds `Input` like the mouse does | anytime |
| **Save / load** | Serialise game state | Plain data, no renderer | M11 |
| **Logging** | Levels, categories, file output | Tiny, used by everyone | M4 |

## Developing on macOS

Until M5 the engine is Windows-only. Before then, a Mac developer can:
- build and test **Audio** and **Physics** natively — neither may include platform headers;
- run the full engine in a Windows VM (Parallels on Apple Silicon).

From M5, the whole engine builds and runs natively on macOS through the Metal backend.

## Tracking

Work is tracked on a **GitHub Projects** Kanban board:
- one **issue** per task, linked to its pull request;
- a GitHub **milestone** per roadmap milestone (M3, M4, …);
- an **area label** per module (`area:renderer`, `area:audio`, `area:physics`, …) so each
  person can filter to their own work.

## Workflow

- **Branch per feature**, merge through a **pull request**, the other person reviews.
- **`main` always builds.** Once CI exists, it enforces this.
- Small commits, messages that say *why*.
- Agree a module's public header *before* building its inside.
- **Trim passes:** at milestones, we go through the code and cut what isn't earning its
  place — over-long comments, speculative code, anything that reads as filler.

## Before the first outside contribution

- [ ] Pick a repo licence
- [ ] Protect `main` on GitHub (require a pull request)
