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

## 4. Combat rules

Raid feel without WoW's full combat model. Fights are about **positioning**, not cast-watching.

| | Decision |
|---|---|
| **Casting** | No cast bars, no interrupts. Abilities fire, then resolve |
| **Resource** | Mana only |
| **Hit table** | Hit or miss, plus crit. **Facing** still matters for cones and cleaves |
| **Timing** | **Impact**: damage is set when an ability fires; who is hit is decided when the telegraph expires or the projectile lands, so dodging works |
| **Line of sight** | Spells and telegraphs respect it — "hide behind the pillar" |
| **Threat** | Minimal, in M9: per-enemy threat list, attack the top, taunt jumps to the top. Boss target rules (random, farthest, not-the-target) read the same list |
| **Time** | No pause, no slow-motion. `F10` opens the menu |
| **RNG** | Seeded and deterministic, so a wipe can be replayed |

### Soldiers

Small companion units fight beside the heroes. They are a resource for hero abilities: a meat
shield, corpses for a necromancer to raise, targets for a support hero's heals and buffs
(damage, attack speed).

- Dozens on screen: AI stays cheap — follow, attack, avoid danger. Separation keeps them from
  clumping.
- Death is a state: a corpse stays for a while and can be raised.
- Commands stay at follow / hold / attack my target — no micromanagement.
- Soldiers and the AI heroes read the same **danger map** that telegraphs write into.

### UI

- 3 hero portraits with health and mana; health bars over units. No raid frames, no click-to-heal.
- **Boss warnings**: timers and "move" alerts, read from the encounter's own timeline data.
- **Combat log and damage meter.**

### Encounters

Rebuilt WoW fights serve as internal test encounters with placeholder names. Shipped bosses are
original; simple archetypes (tank-and-spank) are fine with their own name, setting and twist.

---

## 5. Open questions

- [ ] The hero you start with — class and role?
- [ ] The three heroes: fixed party, or recruited along the way?
- [ ] Setting — what is this world, and why is it dark?
- [ ] World structure: one continuous map or discrete zones?
- [ ] Final name
