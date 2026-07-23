# Anatomy Program — the parametric figure system

Second pillar of the asset factory, sibling to `detailing_program.md`. Where the
detailing program raises props from blockout to detailed, THIS program builds the
hardest content class — living figures (humans, animals, fantasy races) — as
**parametric equations** rather than hand-modelled meshes.

Ace's charge (2026-07-23): "skeleton/muscle parts connected right-to-scale is
highly important work... if we can make this work, then anything will work." This
is the critical-path proof of the equation model. It is spec'd here to
implementation level so the build step makes no new design decisions.

Governing frame: [[factory-equation-model-expansion]] (three-lane model),
`asset_factory_battle_plan.md` (pipeline), `asset-workflow-doctrine` (component-
first: parts, where-used, derivation chains).

---

## 0. The Demand (formalized)

**Problem.** Figures are the one content class that (a) every human viewer is an
expert on — wrong proportions scream — and (b) does not compress to a short
equation if approached as surface sculpting. Hand-modelling 20k+ assets, many of
them figures and their variants, is a multi-decade effort.

**Insight (the écorché method).** Ateliers have taught figure construction as a
layered *procedure* for centuries: bones first, then muscle masses, then skin.
That procedure IS an equation. We implement the sculptor's abstraction (~25–30
muscle masses), not the surgeon's (~650), because that abstraction level is what
parameterizes cleanly.

**Demand.** A system where:
- a **species** is defined once as a joint graph + muscle table (topology only);
- a **body type** (male/female/heavy/child/…) is a **coefficient set** over that
  species — no new geometry authored;
- **scale is always correct** because no part carries absolute coordinates; every
  part anchors to a named joint and inherits scale from the skeleton;
- **fantasy races** are **grafts** — subgraph re-parenting with caliber-matched
  scaling at the junction;
- output is both a **factory blockout GLB** (generated-class, immediate catalog
  use) and an **editable scaffold .blend** (authored-class, for Ace's hero
  finishing — the 80%→100% hand-off).

**Definition of done (system).** The five-rung ladder (§10) passes, with rung 3 —
"female by coefficients only" — as the falsifiable gate. **Done (per figure).**
Reads as its subject at a glance; proportions match the calibration reference;
all masses are separable and named; the skeleton doubles as a valid armature;
contracts hold; scaffold opens clean for hand-finishing.

---

## 1. The Layered Model

Three layers, each a pure function of the layer below. Nothing in an upper layer
knows absolute coordinates; it knows only *which joints it connects*.

| Layer | Input | Output | Function |
|---|---|---|---|
| **L1 — Skeleton** | species topology + proportion preset | world joint positions + bone volumes | `resolve_skeleton(species, preset)` |
| **L2 — Muscle** | resolved skeleton + muscle table + mass scalars | ~25–30 lofted mass meshes | `loft_muscles(skeleton, table, preset)` |
| **L3 — Skin envelope** | bone + muscle volumes | one manifold shell (or pass-through écorché) | `build_envelope(volumes, voxel)` |

**The scale law (the load-bearing rule).** Scale bugs come from parts carrying
absolute coordinates. So no part ever does. L1 owns all scale; L2/L3 derive from
it. Change a bone length in the preset and every muscle re-derives its endpoints
automatically, because a muscle stores *joint addresses*, not positions. This is
what makes body types free — see §5.

**Coordinate convention (matches the engine + existing pipeline).** Z up, +Y
forward (figure faces +Y), X = character's right. Feet ground at Z=0. Mirror axis
= X (left/right symmetry). Sockets/grafts are yaw-about-Z-friendly like the rest
of the catalog. Bounds authored identity-TRS per `Object.hpp` (the engine rotates
at runtime) — bake any part rotation into vertices (Tier-A `rotate_verts`).

---

## 2. Data Schemas (the weeds)

All data is JSON (authored/harvested) resolved by Python at generation. Three
files fully specify a producible figure: one **species**, one **preset**, and
(for a race) a **graft list**.

### 2.1 Species file — topology only (`species/<name>.json`)

Separates **topology** (fixed per species: parent structure, bone directions,
which parameter each bone/joint reads) from **magnitude** (the preset). This split
is the entire reason "female by coefficients only" can work.

```json
{
  "name": "human",
  "up": [0,0,1], "forward": [0,1,0],
  "root": "pelvis",
  "joints": [
    { "name": "pelvis",     "parent": null,       "dir": [0,0,1],  "len": "pelvis_rise",  "caliber": "hip_w" },
    { "name": "spine_low",  "parent": "pelvis",    "dir": [0,0,1],  "len": "spine_low",    "caliber": "waist_w" },
    { "name": "spine_up",   "parent": "spine_low", "dir": [0,0,1],  "len": "spine_up",     "caliber": "chest_w" },
    { "name": "neck",       "parent": "spine_up",  "dir": [0,0,1],  "len": "neck",         "caliber": "neck_w" },
    { "name": "head",       "parent": "neck",      "dir": [0,0,1],  "len": "head",         "caliber": "skull_w" },
    { "name": "clav_r",     "parent": "spine_up",  "dir": [1,0,0.1],"len": "clavicle",     "caliber": "clav_w" },
    { "name": "shoulder_r", "parent": "clav_r",    "dir": [1,0,-0.1],"len":"upper_arm_gap","caliber": "shoulder_w" },
    { "name": "elbow_r",    "parent": "shoulder_r","dir": [0.2,0,-1],"len":"upper_arm",    "caliber": "upperarm_w" },
    { "name": "wrist_r",    "parent": "elbow_r",   "dir": [0.1,0,-1],"len":"forearm",      "caliber": "forearm_w" },
    { "name": "hand_r",     "parent": "wrist_r",   "dir": [0,0,-1], "len": "hand",         "caliber": "wrist_w" },
    { "name": "hip_r",      "parent": "pelvis",    "dir": [1,0,-0.2],"len":"pelvis_half",  "caliber": "hip_joint_w" },
    { "name": "knee_r",     "parent": "hip_r",     "dir": [0,0,-1], "len": "thigh",        "caliber": "thigh_w" },
    { "name": "ankle_r",    "parent": "knee_r",    "dir": [0,0.05,-1],"len":"shin",        "caliber": "shin_w" },
    { "name": "foot_r",     "parent": "ankle_r",   "dir": [0,1,-0.1],"len":"foot",         "caliber": "ankle_w" }
    /* _l joints mirror _r across X — generated, not authored (see §2.4) */
  ],
  "graft_points": ["neck", "shoulder_r", "hip_r", "ankle_r", "wrist_r"],
  "muscles": "human_muscles",
  "pose": "neutral"
}
```

- `dir` = **unit direction** of the bone in the PARENT's frame (normalized at
  load). Topology, fixed per species.
- `len` = **name of a preset length parameter**, NOT a number. The bone vector =
  `normalize(dir) * preset.len[<len>]`.
- `caliber` = name of a preset width parameter; the attachment diameter at the
  joint (used for part sizing AND graft caliber-matching, §6).
- `graft_points` = joints where a donor subgraph may attach (§6).
- Left side is generated by mirroring `_r` → `_l` across X (§2.4) so the species
  file authors one side only.

### 2.2 Preset file — magnitudes only (`presets/<name>.json`)

The coefficient set. Same species + different preset = different body, zero new
geometry.

```json
{
  "name": "male_avg", "species": "human", "unit": "m",
  "len": {
    "pelvis_rise": 0.12, "spine_low": 0.18, "spine_up": 0.22, "neck": 0.08,
    "head": 0.24, "clavicle": 0.16, "upper_arm": 0.32, "forearm": 0.27,
    "hand": 0.19, "pelvis_half": 0.10, "thigh": 0.44, "shin": 0.42, "foot": 0.26,
    "upper_arm_gap": 0.04, "pelvis_half_": 0.10
  },
  "caliber": {
    "hip_w": 0.30, "waist_w": 0.28, "chest_w": 0.34, "neck_w": 0.12,
    "skull_w": 0.19, "shoulder_w": 0.14, "upperarm_w": 0.11, "forearm_w": 0.09,
    "wrist_w": 0.06, "thigh_w": 0.16, "shin_w": 0.11, "ankle_w": 0.07,
    "clav_w": 0.05, "hip_joint_w": 0.13, "clavicle_span": 0.40
  },
  "muscle_mass": { "global": 1.0, "arm": 1.0, "leg": 1.0, "torso": 1.0, "neck": 1.0 },
  "envelope": { "voxel": 0.015, "fat": 0.0 }
}
```

**Female preset = this file with deltas (the rung-3 test).** Expected starting
deltas, refined against reference: `shoulder_w ×0.85`, `hip_w ×1.12`,
`chest_w ×0.92`, overall length `×0.93`, `muscle_mass.global 0.72`,
`envelope.fat 0.04`. If producing `female_avg` needs ANY hand geometry edit
beyond changing these numbers, the mechanism has failed and we redesign before
proceeding. That is the gate.

### 2.3 Muscle table (`muscles/<name>.json`)

Each row is a mass lofted between two joint-anchored points. `profile` is a
radial belly shape — literally a lathe/sweep profile (reuses the Tier-A `sweep`
helper with varying radius). Fusiform = fat in the middle.

```json
{
  "name": "human_muscles",
  "rows": [
    { "name": "deltoid_r",  "from": ["shoulder_r", [0,0,0.02]], "to": ["elbow_r",[0,0,0.35]],
      "profile": [[0,0.4],[0.25,1.0],[0.6,0.7],[1,0.2]], "r": "shoulder_w", "mass": "arm", "mat": "muscle" },
    { "name": "biceps_r",   "from": ["shoulder_r",[0.02,0.04,0]], "to": ["elbow_r",[0,0.03,0]],
      "profile": [[0,0.3],[0.5,1.0],[1,0.4]], "r": "upperarm_w", "mass": "arm", "mat": "muscle" },
    { "name": "triceps_r",  "from": ["shoulder_r",[0,-0.04,0]], "to": ["elbow_r",[0,-0.03,0]],
      "profile": [[0,0.5],[0.4,1.0],[1,0.5]], "r": "upperarm_w", "mass": "arm", "mat": "muscle" },
    { "name": "forearm_r",  "from": ["elbow_r",[0,0,0]], "to": ["wrist_r",[0,0,0]],
      "profile": [[0,1.0],[0.3,0.9],[1,0.4]], "r": "forearm_w", "mass": "arm", "mat": "muscle" },
    { "name": "pec_r",      "from": ["spine_up",[0.06,0.10,0.04]], "to": ["shoulder_r",[0,0.06,0]],
      "profile": [[0,0.6],[0.5,1.0],[1,0.5]], "r": "chest_w", "mass": "torso", "mat": "muscle" }
    /* …see §3 for the full ~25–30 sculptor's-mass list… */
  ]
}
```

- `from`/`to` = `[joint_name, local_offset]`. The mesh spans the two resolved
  world points. Offsets are in the joint's local frame, in preset units.
- `profile` = `[[t, radius_frac], …]`, t∈[0,1] along from→to.
- `r` = which caliber parameter sets the base radius; final radius(t) =
  `radius_frac(t) * preset.caliber[r] * preset.muscle_mass[mass]`.
- `mass` = which mass-scalar group; `mat` = material key (§8).

### 2.4 Mirroring rule

Species/muscle files author the **right** side (`_r`) and midline parts only.
A load-time pass generates `_l` by reflecting across X (negate X of dir/offset,
swap handedness of orientation). One source of truth per bilateral pair — this
also prevents the mirrored-profile copy-paste hazard the river kit hit
(BLD-5 lesson g).

---

## 3. The Sculptor's Muscle Set (~28 masses)

The blockout muscle list. L/R doubles the bilateral ones. This is the calibration
target harvested/generated écorché references refine (§9).

| Region | Masses (bilateral unless noted) | Anchors (from → to) |
|---|---|---|
| **Neck** | sternocleidomastoid; neck_back (midline) | skull → clavicle; head → spine_up |
| **Torso front** | pectoralis; rectus_abdominis (midline); external_oblique | spine_up → shoulder; ribcage → pelvis; ribcage → hip |
| **Torso back** | trapezius (midline); latissimus; erector (midline) | skull → spine_up/shoulder; spine_up → pelvis; pelvis → spine_up |
| **Shoulder/arm** | deltoid; biceps; triceps; forearm (flexor+extensor as one taper) | per §2.3 |
| **Hip/leg** | gluteus; quadriceps; hamstring; adductor; calf; tibialis | pelvis→hip; hip→knee; hip→knee(back); hip→knee(inner); knee→ankle; knee→ankle(front) |
| **Extremities (blockout mass)** | hand; foot | wrist→hand tip; ankle→toe |

Head/face is **skull-driven** at this tier (a landmark volume, not lofted
muscle) — faces are the one region reserved for hero hand-finishing or diffusion-
lane intake (§9), never factory-final.

Bones rendered for the écorché output: long bones as capsules/lathes along their
segments; pelvis, ribcage, skull as landmark blockout volumes; spine as a swept
segmented tube.

---

## 4. The Functions (equation-library additions)

New shared module `anatomy.py`, built on the Tier-A helpers (`lathe`, `sweep`,
`rotate_verts`, smooth-shading). Signatures:

```
resolve_skeleton(species, preset) -> { joint_name: Frame }      # Frame = pos + orientation, world space
  # walk graph from root; pos[child] = pos[parent] + orient[parent] * (dir * preset.len[len_id])
  # returns every joint's world frame; ground so min foot Z == 0

bone_volumes(skeleton, species, preset) -> [Mesh]               # capsules/lathes along segments + landmark blocks
loft_muscles(skeleton, table, preset)  -> [Mesh]                # one fusiform sweep per row (§2.3)
build_envelope(meshes, voxel, fat=0)   -> Mesh                  # boolean-union + voxel_remesh -> manifold shell
graft(base_skel, base_species, donor_species, donor_preset, op) -> (skeleton, meshes)   # §6

emit_figure(species, preset, muscles, mode) -> outputs
  # mode="ecorche" -> separated bone+muscle masses (authored scaffold + GLB)
  # mode="skin"    -> envelope shell (GLB blockout + scaffold)
  # always also emits: armature (skeleton as bones), provenance manifest
```

The muscle loft is the sweep helper with radius(t) from the profile — anatomy
buys in on Tier-A rather than inventing primitives. Building `anatomy.py`
exercises and hardens `sweep`/`lathe`, so it doubles as a Tier-A stress test.

---

## 5. Body Types = Coefficient Sets

Presets are the whole variation mechanism:
- **male_avg / female_avg** — §2.2 deltas.
- **heavy / lean** — `muscle_mass.global` + `envelope.fat`.
- **child** — length deltas AND ratio change (larger head fraction — the classic
  younger-canon shift), not a uniform scale-down.
- **tall / short / broad / narrow** — single-parameter nudges.

Because muscles are joint-addressed, a preset change propagates through all three
layers with zero geometry authoring. A "body-type pack" is a folder of preset
JSONs. This is the derivation-chain doctrine ([[asset-workflow-doctrine]]) at the
equation tier: each preset is one edit-hop from its neighbor, and we name the hop.

---

## 6. Grafts = Fantasy Races (the socket system, applied to anatomy)

A graft re-parents a donor subgraph onto a base joint. It is the catalog's proven
plug/receiver socket discipline with **joints as socket families** and **caliber
as compatibility**:

```json
{ "name": "minotaur", "base": ["human","male_avg"],
  "grafts": [
    { "receiver": "neck",    "donor": ["bull","adult"],  "donor_root": "neck",  "scale": "caliber_match" },
    { "receiver": "ankle_r", "donor": ["bull","adult"],  "donor_root": "hock",  "scale": "caliber_match" },
    { "receiver": "ankle_l", "donor": ["bull","adult"],  "donor_root": "hock",  "scale": "caliber_match" }
  ] }
```

**Caliber-match rule:** uniformly scale the incoming subgraph by
`caliber(receiver) / caliber(donor_root)` so the cross-sections meet cleanly at
the junction; orient the donor root to the receiver's frame. That one rule lets
any head meet any neck, any leg any hip, across species. Grafts are pure data —
a race is a base preset + a graft list. This is v2 of a shipped seam
(cross-kit socket interchange, e.g. tavern chandelier on interior hook), not a
cold start; the mannequin kit's `_figure`/`_quad` split already proved the
quadruped variant path.

Declare a canonical **joint vocabulary** doc (locked before build) so `neck`,
`shoulder`, `hip`, `hock`, `wing_root` mean the same thing across every species —
graft compatibility depends on it, exactly as socket-family string names do.

---

## 7. The Scaffold Contract (authored-class output)

Every figure run yields TWO artifacts:

1. **Blockout GLB** — generated-class, palette-textured, immediate catalog use
   (NPC stand-ins, level-blocking, crowd fill). Same contract as today's kits.
2. **Scaffold .blend** — authored-class. NOTE (2026-07-23 revision): the
   Blender sculpt hand-off is OPTIONAL, not the primary loop — Ace's leverage
   is drawing and visual judgment, not the DCC workflow, and no pipeline step
   may REQUIRE Ace inside Blender. The PRIMARY loop is: Ace draws (memory
   sketches, any quality, phone photo fine) → factory builds → render with
   sketch overlaid at a matched ortho camera → Ace redlines the render →
   coefficients/tables adjust. Ace's sketches are the #1 calibration reference
   (above harvested scans and generated refs — native art direction, zero
   license risk). The scaffold remains available for the rare case Ace *wants*
   to touch a hero by hand:
   - each anatomical mass a **separate named object** (`biceps_r`, `deltoid_r`,
     `pelvis`…) so Ace can grab "left forearm" directly;
   - **mirror modifier live on X** — edit one side, both update;
   - origins at joint centers; clean pivots; regions in named collections;
   - the **armature** (from L1) already bound — rig-for-free dividend;
   - naming convention carries through to the rig and the engine.

**The re-entry law (sibling to generated-stays-generated).** Once Ace hand-
finishes a scaffold, the result is an **authored-class** asset: it re-enters the
catalog as authored, and the factory NEVER regenerates over it. This convention
MUST exist before the first hero is finished, or a reintegration pass will one
day clobber hand work.

---

## 8. Authored-class lane + baked-texture allowance (RULED 2026-07-23)

**Ace ruled YES to the baked-texture allowance.** So:

- **`authored/` directory**, sibling to the generated kits. Holds hand-finished
  scaffolds AND diffusion/scan intake meshes (§9).
- **Never regenerated over** (§7 re-entry law).
- **Per-asset baked albedo ALLOWED** (and normal/roughness later, gated on the
  engine-lane answer of whether the renderer samples them). Budget: albedo ≤
  1024², ≤ N maps/asset (set N when the first hero lands); flagged in the
  manifest. This is the Lever-F material-richness tier the detailing program
  reserved for heroes — now unlocked for authored-class only.
- **Generated-class keeps the palette-tile-only law untouched.** The linter gains
  an `class: authored|generated` field and applies the texture law only to
  generated. Authored-class still validates bounds, grounding, collision,
  sockets, and the provenance manifest (§9).

The `muscle`/`bone`/`skin` material keys in §2–3 map to palette tiles for the
blockout GLB (e.g. a flesh-toned and a bone tile added to the palette) and to
baked albedo in the finished authored version.

---

## 9. Harvest-first sourcing + license gate + provenance

**Order per part (Ace's rule):** free 3D scan → free texture/image → generate the
gap. Never generate what already exists free-and-clean.

**Harvest targets (agent research pass will verify licenses live):**
- *Écorché / anatomy 3D:* Smithsonian Open Access, Scan the World, Three D Scans
  (classical écorché statuary exists here), Z-Anatomy, BodyParts3D.
- *Proportion canons (numbers → presets):* published 8-head figure conventions,
  anthropometry tables, animal proportion sheets. These become the default preset
  values directly.
- *Parametric humans to study (not vendor):* MakeHuman, MB-Lab — study slider
  logic and attachment reasoning; re-implement in our idiom.
- *Textures:* Poly Haven, AmbientCG (CC0) — likely cover much of the Tier-1 tile
  list in `reference_requests.md` without generation.

**Diffusion lane (Ace's RTX 5090 + Hunyuan3D)** fills the sculptural gap — statues,
ornate props, creature heads — as authored-class intake. Prompt disciplines:
single object / neutral gray bg / soft even light / 3/4 view / chunky solid forms;
avoid thin/transparent/multi-object. Écorché figures generate as calibration
reference; graft-stock heads generate "pre-cut at the neck joint."

**COMMERCIAL LICENSE GATE (absolute — game + packs are sold).** CC0/public-domain
preferred; CC-BY workable WITH a maintained attribution ledger; **NonCommercial
and ShareAlike are OUT**. Every conveyor item carries a **provenance manifest**
from day one — `{ source, license, url, date, lane }` — a MANDATORY intake field
and a linter check for authored-class. Unretrofittable at 20k assets; non-
negotiable now.

**`intake_mesh.py`** (diffusion/scan → authored-class): import → orient/ground to
Z=0 → scale to the atlas's declared target dims → voxel remesh → decimate to poly
budget → collision AABB → measure pins → attach provenance manifest → gallery
render → linter (authored-class) → queue-state flip.

---

## 10. The Five-Rung Proof Ladder

Each rung is a small tranche in normal factory rhythm (preflight → box generate →
suite → gallery → review). Ordered by rising risk; rung 3 is the falsifiable gate.

1. **Parametric arm** — shoulder→hand: skeleton segment + deltoid/biceps/triceps/
   forearm lofts + envelope. *Accept:* masses anchor correctly; changing
   `upper_arm` length moves everything coherently; envelope is manifold.
2. **Male écorché** — full `human` + `male_avg`, mode=ecorche. *Accept:* reads as
   a standing figure; all ~28 masses present and separable; proportions match a
   canon reference within tolerance; skeleton is a valid armature.
3. **Female by coefficients ONLY** — `human` + `female_avg`, zero geometry edits.
   *Accept (THE GATE):* a correct female blockout with only preset deltas. Any
   required hand edit = mechanism failure → redesign before rung 4.
4. **Quadruped** — a `wolf` or `horse` species (new topology + preset, same
   functions), reusing the mannequin `_quad` precedent. *Accept:* the functions
   are species-agnostic; only data changed.
5. **One graft** — a beast-head race (e.g. minotaur head-graft) via §6. *Accept:*
   caliber-match junction is clean; race = base preset + graft list, pure data.

By rung 3, Ace can hand-finish the male scaffold while the factory continues.

---

## 11. Risks & burndown

- **Hunyuan écorché quality poor** → museum scans + canon numbers carry the
  calibration; diffusion is a bonus, not a dependency.
- **Envelope reads mushy** → it's a blockout tier; tune voxel size; the scaffold
  hand-off is designed to cover the last mile. Don't over-invest pre-hand-off.
- **Muscle tube-loft too crude** → correct for this tier; profiles + more masses
  refine within the same equation.
- **Female-by-coefficients fails (rung 3)** → that is the designed gate; stop and
  redesign the topology/preset split rather than papering over with geometry.
- **Joint-naming drift across species** → lock the canonical joint vocabulary doc
  FIRST; graft compatibility depends on it (mirrors socket-family string rules).
- **Scale/unit confusion** → the direction-topology / magnitude-preset split is
  the defense; everything resolves to meters at `resolve_skeleton` time; ground
  to Z=0 as the final step.

---

## 12. Execution order (integrates with the factory, doesn't replace it)

0. Lock the **canonical joint vocabulary** + freeze the §2 schemas (this doc).
1. Build **`anatomy.py`** on the Tier-A helpers (also hardens `sweep`/`lathe`).
2. Rungs 1→5 as tranches, each independently reviewed; rung 3 is a hard gate.
3. In parallel (Lane A / Ace's 5090): run the **harvest + sourcing survey**;
   generate écorché refs + graft-stock heads; land `intake_mesh.py` + the
   authored-class linter changes + the provenance manifest.
4. Stand up the **scaffold contract** + authored-class `authored/` lane before the
   first hero finish.

Rides the three-lane rhythm from [[factory-equation-model-expansion]]; the
skeleton layer is the future armature, so this program also seeds the animation
track when the engine lane opens it. Tier-A unlocks and Phase-2 prop detailing
continue alongside — anatomy is a new belt, not a replacement.

---

## 13. THE LAYER STACK RE-SCOPE (Ace's brush ruling, 2026-07-23 — supersedes §10's framing)

**Ace's vision:** the zombie system is a *brush* — a mature creative tool that
wipes away at a body to expose deeper layers (skull and brain under the face,
muscle under an arm), plus a palette treatment. The brush itself is **out of
scope for now**. **The scope is the layers**: skeleton, muscle, tendons, skin,
clothes — built so that one future brush multiplies across every human, animal,
and grafted creature without generating new parts.

**The architectural insight that makes the brush cheap later:** we GENERATE
figures, so "wiping away" is not destructive mesh editing — it is **omission at
generation time**. The brush (later) and seeded variants (today) are two
authoring frontends for the same data:

**Wound format (exists NOW, brush-writable LATER).** A wound row =
`{ bone, span [t0,t1], sector [ang0,ang1], depth_layer, lip }` — a region in
*bone coordinates* (position along a named bone × angle around it) plus which
layer it exposes. The skeleton graph is the body's coordinate system, so wounds
are portable across presets, poses, species, and grafts by construction. Seeds
author wound lists today; the brush authors them interactively someday; the
generator resolves either by omitting/trimming covering layers in that region.

**Brush-readiness laws (bind every layer we build from here):**
1. **Completeness-under** — a layer may only be wiped if the layer beneath is
   modeled everywhere beneath it. Skull+brain must exist under the face; muscle
   coverage must close its gaps before skin ships. (Re-prioritizes the skull:
   cranium, jaw, teeth, brain volume — head wounds are THE zombie read.)
2. **Strict nesting / radial offsets** — skin ⊃ muscle ⊃ bone with a guaranteed
   offset between layers; no accidental poke-through (the coplanar law, radial).
   Checkable: a pre-checker asserts layer N contains layer N−1 per region.
3. **Region addressing** — skin is authored as **panels per bone-region**
   (scalp/face/torso-front/back/per-limb-segment), seams on anatomical lines.
   Panels make wound omission trivial, give the brush snap edges, and solve the
   skin-envelope problem regionally instead of as one risky global union.
4. **Palette states as data** — living/zombie/skeletal = per-layer material
   remap tables (skin→pallid, muscle→ichor, bone→yellowed), not new assets.
5. **Composite output** — a variant ships only its resolved visible surface
   (outer layer minus wounds + exposed lower layers at wounds), never the full
   stack; Switch-class budgets survive because layers are a *source* concept.

**Layer roadmap (the re-scoped program):**
- **L0 skeleton+** — upgrade skull (cranium/jaw/teeth/brain), rib hints, pelvis
  shape. - **L1 muscle** — close coverage gaps; tendons at hero tier (wrists/
  ankles/neck). - **L2 skin panels** — per-region offset hulls over L1.
- **L3 clothes** — garments as joint-anchored masses over skin; torn cloth =
  the same wound mechanic one layer up. NOTE: L3 is the *living NPC pipeline*
  (guards, townsfolk are clothed humans) — this roadmap serves the whole game,
  zombies are one palette+wound state of it.
- Wound format + palette tables spec'd from day one; brush tool parked.

**Next falsifiable spike:** one figure, one authored wound row — face region,
depth=bone — proving wipe-by-construction: skin panel omitted, skull+brain
exposed, lip ring at the boundary. If that reads, the brush model is real.

## 14. Commercial note (design constraint, not a work item yet)

Parametric écorché/anatomy systems and body-type packs have an established paying
audience (art students, sculptors, indie devs). Equation-lane output is
**procedural, not "AI-generated,"** which is an advantage under marketplace AI-
disclosure rules; only diffusion-lane assets carry the AI tag. Productization
(FBX/OBJ export, material lift, pack curation) is a LATER pass and must not derail
the game — but two habits, adopted now, keep the door open at zero cost: the
**license gate** and the **provenance ledger** (§9).
