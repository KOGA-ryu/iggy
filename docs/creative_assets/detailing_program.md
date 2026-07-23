# Asset Detailing Program — Work Plan

Formalizes the demand to raise the asset catalog from blockout fidelity to
detailed fidelity, systematically and safely. Current state: **23 kits / ~700
assets**, all built from axis-aligned boxes + 8-sided ("octagon") primitives, so
curved forms read faceted and surfaces read flat. This program makes "add more
detail" a repeatable, contract-safe pipeline instead of ad-hoc edits.

---

## 0. The Demand (formalized)

**Problem.** Every kit is at blockout tier: boxes + 8-gon cylinders, no trim, no
sub-detail, uniform flat-albedo tiles. Assets read as placeholders.

**Demand.** Raise each asset ≥1 fidelity tier (target: blockout → **Detailed**;
hero kits → **Hero**) **without breaking any contract** — asset IDs, asset count,
category, collision mode, sockets (name + location + role + compatibility), the
tile-set per asset, and grounding must all be preserved; bounds may grow.

**Definition of done (per asset).** (a) reads as its object at a glance, not a
box approximation; (b) primary curved forms are round, not faceted; (c) carries
appropriate secondary detail (trim/moldings/fittings/panels); (d) all contracts
hold; (e) passes the full preflight + suite; (f) visually confirmed in the
gallery. **Done (program).** Every kit re-integrated, catalog green, a
before/after gallery per kit.

---

## 1. Fidelity Rubric (the tiers)

| Tier | Name | Marker |
|---|---|---|
| T0 | Blockout | boxes + 8-gon primitives, no trim (current state) |
| T1 | Refined | correct proportions, chamfered primary edges, basic moldings, 16-gon curves |
| T2 | **Detailed** (program target) | sub-parts (panels, studs, fittings, turned profiles), 16–24-gon curves, secondary forms, per-instance variation |
| T3 | Hero (heist/armory/lighting) | T2 + material richness (edge wear, bevel-shading in albedo) + variation, gated on real textures |

---

## 2. Detail Levers (techniques, ranked by anti-blocky payoff)

- **Lever A — Segment count (biggest single win).** Replace the fixed 8-gon
  primitives with parametric N-gon helpers (16 or 24 sides). Smooths every
  cylinder, cone, sphere, disc across ALL kits. Global helper upgrade.
- **Lever B — Sub-parts.** Moldings, cornices, chamfers-as-boxes, studs/rivets,
  panel insets, feet, finials, fittings, straps, edge banding.
- **Lever C — Profile lathe.** A revolve helper (2D profile → turned solid) for
  balusters, goblet stems, candlesticks, column shafts, table legs, urns.
- **Lever D — Proportion / silhouette.** Fix stubby or slab-like proportions;
  taper limbs, trunks, blades, legs.
- **Lever E — Variation.** Seeded per-instance jitter (size/rotation/offset) so
  repeated elements (leaves, coins, staves, produce, animals) differ.
- **Lever F — Material richness (gated on Ace's textures).** Edge wear, dirt,
  bevel-shading baked into albedo, per-material roughness. Blocked until the
  Tier-1 seamless textures land (see `reference_requests.md`).

---

## 3. Custom Tooling

### Already built (this session)
- `assemble_kit.py` — head + shared block + body, auto export-prefix.
- `emit_kit.py` — lint spec block + static_mesh rows + dedup delta (measured).
- `splice_kit.py` — splices all contract files + pin + cmake, off the prior kit.
- `make_roster.py` — auto roster fixture + acceptance test.
- `render_gallery.py` / `render_roster.py` — generalized self-review renders.

### To build (prioritized)
1. **`ngon.py` — parametric N-gon primitive set** (Lever A). `make_ngon_column/
   frustum/log/wheel/ball/disc(..., seg=16)`. Drop-in replacement for the octagon
   helpers; a per-kit or global `SEG` constant. **Highest leverage.**
2. **`detail_helpers.py`** — `bevel_box` (chamfered box), `lathe(profile, seg)`
   (revolve), `torus_ring`, `molding_strip`, `stud_row`, `taper_limb`. (Levers B/C)
3. **`reintegrate_kit.py`** — one command: regenerate → emit_kit → replace the
   kit's spec map → re-measure pins → lint --write → build+suite. Collapses the
   whole re-integration to a single step.
4. **`detail_audit.py`** — scores each asset (part-count, vertex-count, distinct
   segment-counts) → flags assets still at T0 so nothing is missed.
5. **Static pre-checkers** (run before Blender, catch what the linter/preflight
   miss): `check_sockets.py` (name is `[a-z0-9_]`, ≤64, unique, valid role/
   compat), `check_grounding.py` (min-z per asset), `check_degenerate.py` (zero-
   extent boxes), `check_zfight.py` (coplanar-coincident different-material faces).
6. **Linter additions** — fold the socket-name `[a-z0-9_]` rule + a per-kit
   detail-budget floor into `lint_contracts.py`.
7. **`before_after.py`** — side-by-side old/new gallery montage per kit.
8. **`variation.py`** — seeded jitter helpers for organic kits (Lever E).

---

## 4. Protocols

- **P1 — Strict Detail Contract.** Preserve: asset IDs, count, `CATEGORY`,
  collision mode, every `add_socket` (name + location + role + compat), the
  tile-set per asset, grounding. No new tiles unless a pin-update is planned.
  Bounds may grow. Never touch the shared helper block signatures.
- **P2 — The Detail Loop (per kit).** read generator → apply levers → static
  pre-checks (P5 tools) → adversarial preflight (P3) → regenerate on box →
  `emit_kit` re-measure → replace spec map → `lint --write` + validate → build +
  full suite → visual gallery + before/after review → commit.
- **P3 — Preflight lens set (adversarial, skeptic-verified).** (1) crash/export,
  (2) grounding + below-floor, (3) socket-integrity (unchanged from original),
  (4) z-fight/coplanar (added trim), (5) **detail-delta** (part/vertex count
  actually rose; no asset regressed to fewer parts).
- **P4 — Pin protocol: MEASURE, never guess.** `textures` pin moves only if new
  tiles embed; `materialBindings` dedup pin moves only if per-GLB material COUNT
  changes (adding parts with existing tiles does NOT change it); bounds are
  re-measured via `emit_kit` + spec-map replace.
- **P5 — Re-integration protocol.** Sockets fixed ⇒ fixtures pass untouched.
  Regenerate → `emit_kit` → replace the kit's spec map (bounds) → `lint --write`
  → build + suite → commit. (Automate as `reintegrate_kit.py`.)
- **P6 — My-eyes review (lenses miss these).** Always check: export paths carry
  the category prefix; the head has imports/OUT_DIR; socket names are `[a-z0-9_]`;
  no module-constants dropped by assembly. (Every one bit us this session.)
- **P7 — Commit discipline.** my-hunks-only; lane-isolation check before commit;
  one kit or one coherent batch per commit; measure-then-pin; reconcile stays
  BLOCKED until Ace signals Codex's marathon has landed.
- **P8 — The catalog-failure trap.** The C++ catalog enforces rules the linter
  does not (socket name chars, socket frame, mesh validity). ONE bad asset makes
  `catalog.failures` non-empty, which fails EVERY roster/fixture test at once.
  Diagnose by dumping `catalog.failures {sourcePath, reasonCode}`. The static
  pre-checkers (P5) exist to catch these before generation.

---

## 5. EXHAUSTIVE DETAIL LIST (per kit → per asset-family)

Directives are concrete geometry to add under P1. "16/24-gon" = apply Lever A.

### Gameplay heroes (target T3)

**heist** — chests: raised panel insets on faces + lid, iron corner brackets +
feet, keyhole escutcheon, curved (16-gon) barrel-lid staves; **goblet/reliquary/
coins:** lathe the goblet stem+knop+foot, coin edge milling + a spilled scatter,
reliquary gable tracery + finials + gem cabochons, gem facet cuts (lathe); vault
door: recessed panels, boss rivple ring, wheel-lock spokes+hub detail.
**armory** — blades: a fuller (recessed centerline) + ricasso + tapered tip;
hilts: langets/collar, lathe the pommel + grip rings; shields: 16-gon round +
rim stud ring + boss dome + spokes; helms: comb/crest, riveted brow, cheek
plates; racks: turned uprights, pegs.
**lighting** — candelabra: lathe stem + drip pans + scrolled arms; chandeliers:
tiered 16-gon rings + individual chain links + candle cups; sconces: scrolled
backplate + shaped arm; hearth: mantel molding, log-stack + ember variation;
16-gon everywhere (candles, cups, bowls, posts).
**mannequin** — taper every limb (Lever D via stacked/lathe segments), add
hand + foot blocks, a jaw/brow to the head, collar/belt/cuff separations, better
shoulder + hip mass; keep the pose skeleton + (no) sockets fixed.

### Room sets (target T2)

**interior** — furniture: lathe/turned legs (tables, chairs, stools), raised-
panel doors + drawer fronts + pulls, table aprons, chair back slats + crest
rail, cornices/moldings on cabinets, bed head/foot boards, 16-gon on any round.
**tavern** — barrels/kegs: curved 16-gon staves + iron hoops + bung + tap;
mugs/tankards: handle + rim + 16-gon body; bottles: neck + shoulder + punt;
bar: edge molding + panel front; stools: turned legs.
**workshop** — workbench: apron + vise jaw screw + dog holes; tools: shaped
heads (hammer/chisel/saw teeth) + handles; anvil: horn + hardy hole + waist;
grindstone: 16-gon wheel + frame + crank + treadle; tool racks: pegs + hung tools.
**guard** — weapon rack: turned uprights + pegs + hung weapons; armor stand:
shoulder yoke + helm + tabard folds; shields: boss + rim; braziers: 16-gon bowl
+ legs + coals; banners: fold undulation; barricades: plank + iron strap detail.
**dungeon** — cell bars: cross-ties + rivets + 16-gon bars; restraints: real
chain links + shackle hinge; walls: stone-block coursing + mortar recess; torch
bracket, straw pallet lumps, rubble + bone variation.

### Nature (target T2, heavy on Lever E variation)

**grove / woodland** — trees: layered foliage clumps (3–5 balls, jittered) + 2–3
large boughs + branch forks, trunk taper + root flare + bark ridge boxes, 16-gon
trunk; understory: leaf clusters, fallen-branch + mushroom variation.
**crag / rock_cliff / cave** — rocks: irregular multi-facet (jittered vertices/
sub-blocks) not smooth balls, strata banding on cliffs, stalactite/stalagmite
taper, moss patches; boulders get 2–3 size/rotation variants.
**farm** — crops: more/varied stalks + heads (jitter), furrow ridging, fence
post + rail joinery, cart wheel spokes + 16-gon rim, hay-bale binding + texture.
**marsh** — reeds/cattails: more blades per clump (jitter lean/height), water-
plant leaves + flowers, log bark + bracket-fungus shelves, boardwalk plank grain
+ nail heads + support posts, bank tuft variation.
**riverbank** — rocks (as crag), logs (bark + broken ends), reeds (as marsh),
water edge detail.

### New Batch-6 kits (target T2)

**rigging** — ladders: real rungs + tapered stiles; ropes/nets: sagged catenary
segments + knot lumps + 16-gon coil; scaffolds: lashings + plank grain; pulleys:
16-gon sheave + groove + axle + hook; grates: bar lattice + frame rivets.
**portal** — doors: raised panels + rails/stiles + iron strap hinges + clavos
(studs) + ring/knocker; windows: mullions + came grid + sill + shutter louvers;
gates: picket + ironwork scroll; locks: shackle + keyway + hasp staple detail.
**animal** — taper legs + necks (Lever D), shaped heads (snout/ears/beak mass),
segmented tails, hooves/paws, 16-gon where round; per-species proportion pass;
seed jitter for herd variants.
**market** — stalls: awning scallop + valance + tie ropes + turned posts; goods:
individual loaves/fruits/pots (jitter) not solid piles; barrels (as tavern);
scale: chains + pans + beam; cart: 16-gon spoked wheels + bed slats; signage:
bracket scroll + hanging chain + carved face.

### Structural / legacy (target T1, lower priority)

**architecture / infrastructure / settlement / simulation / homestead /
stealth_blockout** — 16-gon on any round members; chamfer beam/post edges; add
sill/lintel/cornice moldings, roof-tile courses, stair nosing, wall coursing,
bridge rail balusters (lathe), lamp-post fluting. These read acceptably as
architecture already; detail them after the hero + room-set kits.

---

## 6. Execution Order (recommended)

1. **Build Lever-A tooling** (`ngon.py`) + fold into `assemble_kit`'s shared
   block → the single biggest visual win, applies everywhere.
2. **Build `reintegrate_kit.py`** + the static pre-checkers + the socket-name
   linter rule → makes each kit a one-command, safe re-integration.
3. **Hero kits first** (heist, armory, lighting, mannequin) — T2/T3, full review.
4. **Room sets** (interior, tavern, workshop, guard, dungeon) — T2.
5. **Nature** (grove, crag, farm, marsh, +legacy woodland/cave/rock_cliff) — T2 +
   variation.
6. **New kits** (rigging, portal, animal, market) — T2.
7. **Structural/legacy** — T1.
8. Each phase: author (agents) → preflight → re-integrate → suite → before/after
   gallery → commit. One kit or coherent batch per commit.

Blocked on Ace: Lever F (materials) needs the Tier-1 seamless textures from
`docs/creative_assets/reference_requests.md`. Everything else is unblocked.
