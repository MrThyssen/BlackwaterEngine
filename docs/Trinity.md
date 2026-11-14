# Trinity — Game Design

> *Trinity* (working name) is the game built on the **Blackwater** engine.
> Engine design lives in [../DESIGN.md](../DESIGN.md).
>
> **Rule:** Trinity links Blackwater. Blackwater never knows Trinity exists.

---

## 1. Premise

A **single-player 2.5D isometric RPG**: 3D characters in a 3D world, seen from a fixed
isometric camera. Diablo / Baldur's Gate lineage.

**The feeling to evoke: being in a raid** in the popular MMORPGs — but single-player, with
the constraint that the player controls at most **three heroes**, each with **4–6
abilities**. That trio is the "Trinity". The game starts with **one hero**; the other two
arrive once one hero plays well.

Around the raids: explore, find gear, make choices that shape the story.
Tone: **dark and grounded.**

---

## 2. Camera & controls

| | |
|---|---|
| **Camera** | **Fixed isometric** — locked angle, no rotation, no zoom |
| **Pan camera** | Mouse at the screen edges, or **↑ ↓ ← →** |
| **Move** | **Right-click** the ground |
| **Abilities** | **`1` `2` `3`** and **`Q` `W` `E` `D`** |
| **Attack** | Right-click an enemy |
| **Select** *(party, later)* | Left-click a hero, `Tab` to cycle |

---

## 3. Systems

Built in this order — each one only once the previous plays well.

1. **Movement** — click to move; later, pathfinding around obstacles.
2. **Combat** — real-time, auto-attack plus a small ability hotbar with cooldowns. Enemies
   with health and basic aggression.
3. **Progression & gear** — levels, stats, loot with rarity, equipment that changes stats.
4. **Party** — three heroes with distinct roles. The two you aren't driving follow and fight
   on simple AI; this must not turn into micromanagement.
5. **Story & choices** — branching dialogue backed by a flag store: dialogue and quests read
   and write named state, which is enough to make choices persist.

---

## 4. Open questions

- [ ] The hero you start with — class and role?
- [ ] The three heroes: fixed party, or recruited along the way?
- [ ] Setting — what is this world, and why is it dark?
- [ ] World structure: one continuous map or discrete zones?
- [ ] Final name
