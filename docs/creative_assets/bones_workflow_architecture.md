# Bones Workflow Architecture — the runnable pipeline (the jig)

The production architecture for the bones asset loop. Third pillar beside
`anatomy_program.md` (the mechanism) and `asset_factory_battle_plan.md` (the lanes).
Where those say *what* and *why*, THIS says *how the loop runs without thrashing*.

Charter: turn Ace's founding 220-bone catalog ([[founding-anatomy-research]]) into
a shipped skeleton via a repeatable loop that is DATA-driven, so iteration edits
numbers, never pipeline code. Look target = Nintendo/BotW stylized on a Switch2/
MacBook budget ([[ace-visual-profile]]).

---

## 0. The core principle — three things kept separate

Thrash comes from mixing what changes often with what must stay frozen. So:

| Layer | What | Changes | Owner |
|---|---|---|---|
| **DATA** | bone table, proportions, dimension overlay | every iteration | Ace's catalog + presets |
| **STAGE CODE** | the 8 pipeline stages, the ~6 primitives | almost never (frozen once proven) | me |
| **CONTROL LOOP** | which region, which gate, lock state | per region | the loop driver |

**Iron law: an adjustment edits DATA, never STAGE CODE.** "The femur's too thick"
→ change a number in the bone table. If a fix ever needs stage-code changes, that's
a primitive-map gap (rare, deliberate, version-bumped) — not routine tuning.

---

## 1. Data contracts (the five files)

### 1.1 Bone table — `bones.json` (derived from Ace's docx catalog)
The 220-row master. One row per bone; columns exactly mirror the catalog plus
generation params:
```json
{ "id": 47, "region": "thoracic spine", "name": "thoracic vertebra 6",
  "rig": "t06", "shape_type": "irregular vertebra", "primitive": "vertebra",
  "anchor": {"joint": "t06", "parent": "t05"},
  "params": { "body_r": 0.017, "spinous_len": 0.052, "transverse": 0.020, "rib_socket": true },
  "note": "rib socket controls cage arc", "class": "generated", "lock": false }
```
- `rig` is Ace's `l_`/`r_` name — FROZEN at import (§4), because it propagates to
  the armature, the muscle-layer anchors, and the wound coordinate system.
- `params` are the ONLY field routine iteration touches.
- `note` is Ace's generator note — carries silhouette/gameplay intent verbatim.

### 1.2 Shape-type → primitive map — `primitive_map.json` (FROZEN vocabulary)
Ace's 44 shape labels collapse to **6 primitive families**, all already built &
proven in `skeleton_detail.py`:

| Primitive | Ace shape labels (examples) | count | generator |
|---|---|---|---|
| **long_bone** | long, long slender, long curved bone, long mini, *taper | ~92 | `fusiform` + ball ends (head/condyles) |
| **arc** | flat curved arc (ribs), curve tube, u-arc | ~26 | `sweep` (parallel-transport) |
| **plate** | flat/curved plate, triangular, wing, *flat plate | ~15 | `blade` / `poly3d` |
| **vertebra** | irregular {small,,large} vertebra, ring (atlas) | ~24 | `vertebra` composite (body+ring+spinous+transverse) |
| **block** | short {pebble,wedge,block,saddle,hook,dome,boat,cube,heel} | ~40 | `box`/`ellipsoid` blend |
| **shell** | irregular, compound shell, strut, support, lever, sesamoid | ~21 | `composite` (multi-primitive cage) |
| *(rig-only)* | control node(s), ik/fk chain, curve chain, surface shell | ~8 | no mesh — armature / deferred skin layer |

This table is the leverage: **~6 functions cover 220 bones.** It changes only when
a genuinely new form appears (then: add a primitive, version-bump, re-freeze).

### 1.3 Proportions — `proportions.json` (owns all magnitudes)
Adopt `pixel_rig_factory/humanoid_body_shape_v1.json`'s landmark ratios (ground 0,
ankle 7, knee 20, pelvis 35, chest 43, shoulder 60, neck 64, chin 68, eye 86,
top 96) as normalized-to-height coefficients. Presets (male/female/child/Nintendo)
= coefficient sets, exactly as `anatomy_program.md` §2.2. This replaces every
eyeballed height in the spike.

### 1.4 Dimension overlay — `anthropometry.json` (OPTIONAL, provenance-carrying)
Real mm bone lengths IF sourced (Ace's remembered research isn't tabulated on
disk — [[founding-anatomy-research]]). Each entry `{bone, value_mm, source, license}`.
Absent → proportions carry sizing. Present → overrides proportional lengths and
stamps provenance for the commercial path.

### 1.5 Rig / skeleton graph — `rig.json`
Joint frames the bones anchor to (the `anchor.joint` targets). Rig names = the
future armature (the rig-for-free dividend). Resolved from proportions at RESOLVE.

---

## 2. The pipeline — 8 fixed stages

```
RESOLVE  proportions → world joint frames (rig)
PLACE    each bone: read anchor joint(s) + params → primitive(shape_type) → mesh in place
ASSEMBLE collect a region (or whole skeleton) into a collection
RENDER   front/side/¾ + silhouette (Workbench), + Ace-sketch overlay at matched ortho cam
REVIEW   my eyes → structured findings (reads? shape-type right? nests? depth?); Ace redlines
ADJUST   findings → edits to bones.json params / proportions.json ONLY
GATE     evaluate the region's gate (§3); pass → lock; fail → back to PLACE
EMIT     GLB + provenance manifest + catalog integration (class = generated|authored)
```
Stage code is written once, frozen after the primitive map proves out. Everything
after that is DATA looping through these stages.

---

## 3. Gates (three levels, cheapest first)

- **Per-bone** — reads as the named bone; correct shape-type family; grounded/
  anchored right; lowest part bottoms at its joint (no float, no sink).
- **Per-region** — bones nest with neighbours; no interpenetration; **side-view
  depth correct** (the spike's weak axis — an explicit gate, not an afterthought);
  region silhouette reads.
- **Full-skeleton** — all 220 present; proportions match the landmark ratios;
  front AND side silhouettes read; determinism check (re-run → byte-identical).

A gate is a checklist I evaluate on the render with my own eyes (standing law:
never claim a render shows what it doesn't) + Ace's redline. Passing locks it.

---

## 4. Anti-thrash laws

1. **DATA-only adjustment** (§0 iron law).
2. **Region locking.** The catalog is already ordered by region — iterate one
   region to gate-pass, then set `lock:true`. Locked params don't reopen without an
   explicit version bump. Global re-renders stay allowed; region *values* freeze.
3. **One source of truth per fact.** Bone list = catalog. Magnitudes = proportions.
   Method = primitive map. Names = rig. Never duplicate a fact across files.
4. **Names frozen at import.** Rig names propagate to armature + muscle anchors +
   wound coords — renaming later is a cascade. Lock them on day one.
5. **Determinism.** Pure vertex math, seeded; byte-identical regen so the pins/
   linter discipline from the 23-kit factory applies unchanged.
6. **No silent primitive edits.** A stage-code/primitive change is a versioned
   event with its own before/after render, never folded into a tuning pass.

---

## 5. How it stacks — up (layers) and out (factory)

**Up (the layer stack, `anatomy_program.md` §13):** muscle → skin panels → clothes
are each a NEW data set (their own `*.json`) against the SAME stage code + SAME rig.
Muscle rows anchor by rig name; skin panels are per bone-region; the **wound format**
`{bone, span, sector, depth_layer, lip}` is defined in bone-coordinates here, so the
zombie brush (later) and seeded variants (now) both just author wound lists. Bones
first because every upper layer references bone rig names and joint frames.

**Out (the conveyor + the 23-kit factory):** EMIT feeds the atlas/queue ledger
(row state: listed → generated → review → authored/locked) and reuses the factory's
render/measure/lint tooling. Class split holds: skeleton blockout = generated-class
(palette bone tile); hand-finished heroes = authored-class (baked albedo allowed,
never regenerated over). Provenance manifest rides every EMIT.

---

## 6. Integration debts (come due at first catalog bone, tracked not deferred-silently)
GLB export from the spike; compound collision mode for a many-part skeleton (the
mixed-tag trap, BLD-6 lesson i); linter `class:` field; determinism proven through
the pin pipeline; LEGAL_CATEGORIES entry for the bone kit. None block iteration;
all block SHIP.

---

## 7. First execution slice (when the loop hat starts)
1. Build `bones.json` from `bone_catalog_clean.txt` (220 rows) + `primitive_map.json`
   (the §1.2 table) + `proportions.json` (from body_shape landmarks). One-time.
2. Freeze rig names; RESOLVE the rig.
3. Run the loop region-by-region in catalog order, skull → face → spine → ribs →
   girdles → arms/hands → pelvis → legs/feet. Gate + lock each.
4. Full-skeleton gate. EMIT as generated-class bone kit.
5. THEN layer up (muscle) — same jig, new data.

Regions with the highest bone count and my spike's weakest reads — **face (13),
hands (19×2), feet (19×2), full vertebra set (24)** — are where Ace's catalog adds
the most over the spike (which simplified them). Those get the real coverage here.
