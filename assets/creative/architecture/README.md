# Architecture Building-Closure Kit (ASSET-CAL-1, Batch 1 seed)

The building-closure half of the ASSET-CAL-1 batch: the highest-value missing
construction pieces to prove door state families, full-storey traversal, a rail,
and roof-ridge closure (backlog sections D/E/F; Production Laws 1-6).

Stable ids (verbatim): `architecture/openings/door_frame_standard`,
`door_leaf_standard_closed`, `door_leaf_standard_open`,
`architecture/traversal/stair_straight_3m`, `stair_landing_2x2m`,
`architecture/structural/railing_straight_2m`,
`architecture/roof/ridge_cap_straight_4m`, `ridge_cap_end`.

## Regenerate (headless only)

```
blender --background --python assets/creative/architecture/tools/generate_kit.py
blender --background --python assets/creative/calibration/tools/render_proofs.py   # shared proofs
python3 assets/creative/calibration/tools/lint_contracts.py --write               # README tables
python3 assets/creative/calibration/tools/lint_contracts.py                       # gate
```

Generators are deterministic Python (no hand-saved `.blend`), following the
stealth_blockout `tools/` precedent.

## Metrics (mechanically generated — do not hand-edit)

Emitted by `lint_contracts.py`; measured from the exported GLB. Bounds in glTF
Y-up (X width, Y vertical, Z depth). `walkable` = walkable-part count (compound)
or 1/0 for a single walkable `bounds` asset.

<!-- METRICS-TABLE:BEGIN -->
| assetId | tris | prims | mats | nodes | bounds min..max (m, glTF Y-up) | collision | parts | walkable | sockets | tile |
|---|--:|--:|--:|--:|---|---|--:|--:|--:|---|
| `architecture/openings/door_frame_standard` | 36 | 3 | 1 | 4 | (-0.500,0.000,-0.075)..(0.500,2.250,0.075) | compound_bounds | 3 | 0 | 1 | oak_timber |
| `architecture/openings/door_leaf_standard_closed` | 12 | 1 | 1 | 2 | (0.000,0.000,-0.025)..(0.900,2.100,0.025) | bounds | 0 | 0 | 1 | oak_plank |
| `architecture/openings/door_leaf_standard_open` | 12 | 1 | 1 | 2 | (-0.025,0.000,0.000)..(0.025,2.100,0.900) | bounds | 0 | 0 | 1 | oak_plank |
| `architecture/traversal/stair_straight_3m` | 192 | 16 | 1 | 16 | (-0.500,0.000,-2.000)..(0.500,3.000,2.000) | compound_bounds | 16 | 16 | 0 | oak_timber |
| `architecture/traversal/stair_landing_2x2m` | 12 | 1 | 1 | 1 | (-1.000,0.000,-1.000)..(1.000,0.200,1.000) | bounds | 0 | 1 | 0 | oak_plank |
| `architecture/structural/railing_straight_2m` | 48 | 1 | 1 | 1 | (-1.000,0.000,-0.040)..(1.000,1.000,0.040) | bounds | 0 | 0 | 0 | oak_timber |
| `architecture/roof/ridge_cap_straight_4m` | 8 | 1 | 1 | 1 | (-2.000,0.000,-0.200)..(2.000,0.115,0.200) | bounds | 0 | 0 | 0 | shingle_oak |
| `architecture/roof/ridge_cap_end` | 8 | 1 | 1 | 1 | (-0.200,0.000,-0.200)..(0.200,0.115,0.200) | bounds | 0 | 0 | 0 | shingle_oak |
| `architecture/openings/door_frame_wide` | 36 | 3 | 1 | 4 | (-0.650,0.000,-0.075)..(0.650,2.250,0.075) | compound_bounds | 3 | 0 | 1 | oak_timber |
| `architecture/openings/door_leaf_wide_closed` | 12 | 1 | 1 | 2 | (0.000,0.000,-0.025)..(1.200,2.100,0.025) | bounds | 0 | 0 | 1 | oak_plank |
| `architecture/openings/door_leaf_wide_open` | 12 | 1 | 1 | 2 | (-0.025,0.000,0.000)..(0.025,2.100,1.200) | bounds | 0 | 0 | 1 | oak_plank |
| `architecture/openings/door_frame_double` | 36 | 3 | 1 | 5 | (-0.950,0.000,-0.075)..(0.950,2.250,0.075) | compound_bounds | 3 | 0 | 2 | oak_timber |
| `architecture/openings/door_leaf_double_left_closed` | 12 | 1 | 1 | 2 | (0.000,0.000,-0.025)..(0.900,2.100,0.025) | bounds | 0 | 0 | 1 | oak_plank |
| `architecture/openings/door_leaf_double_left_open` | 12 | 1 | 1 | 2 | (-0.025,0.000,0.000)..(0.025,2.100,0.900) | bounds | 0 | 0 | 1 | oak_plank |
| `architecture/openings/door_leaf_double_right_closed` | 12 | 1 | 1 | 2 | (-0.900,0.000,-0.025)..(0.000,2.100,0.025) | bounds | 0 | 0 | 1 | oak_plank |
| `architecture/openings/door_leaf_double_right_open` | 12 | 1 | 1 | 2 | (-0.025,0.000,0.000)..(0.025,2.100,0.900) | bounds | 0 | 0 | 1 | oak_plank |
| `architecture/openings/window_frame_standard` | 48 | 4 | 1 | 7 | (-0.480,-0.080,-0.075)..(0.480,1.280,0.075) | compound_bounds | 4 | 0 | 3 | oak_timber |
| `architecture/openings/window_frame_small` | 48 | 4 | 1 | 4 | (-0.330,-0.080,-0.075)..(0.330,0.680,0.075) | compound_bounds | 4 | 0 | 0 | oak_timber |
| `architecture/openings/window_shutter_left_closed` | 12 | 1 | 1 | 2 | (0.000,0.000,-0.020)..(0.400,1.200,0.020) | bounds | 0 | 0 | 1 | oak_plank |
| `architecture/openings/window_shutter_left_open` | 12 | 1 | 1 | 2 | (-0.020,0.000,0.000)..(0.020,1.200,0.400) | bounds | 0 | 0 | 1 | oak_plank |
| `architecture/openings/window_shutter_right_closed` | 12 | 1 | 1 | 2 | (-0.400,0.000,-0.020)..(0.000,1.200,0.020) | bounds | 0 | 0 | 1 | oak_plank |
| `architecture/openings/window_shutter_right_open` | 12 | 1 | 1 | 2 | (-0.020,0.000,0.000)..(0.020,1.200,0.400) | bounds | 0 | 0 | 1 | oak_plank |
| `architecture/openings/window_frame_wide` | 48 | 4 | 1 | 4 | (-0.780,-0.080,-0.075)..(0.780,1.280,0.075) | compound_bounds | 4 | 0 | 0 | oak_timber |
| `architecture/openings/window_frame_tall` | 48 | 4 | 1 | 4 | (-0.480,-0.080,-0.075)..(0.480,1.880,0.075) | compound_bounds | 4 | 0 | 0 | oak_timber |
| `architecture/openings/window_bars_standard` | 48 | 1 | 1 | 2 | (-0.400,0.000,-0.020)..(0.400,1.200,0.020) | bounds | 0 | 0 | 1 | iron_forged |
| `architecture/openings/window_mullion_cross` | 24 | 1 | 1 | 2 | (-0.400,0.000,-0.025)..(0.400,1.200,0.025) | none | 0 | 0 | 1 | oak_timber |
| `architecture/openings/window_sill_standard` | 12 | 1 | 1 | 1 | (-0.550,0.000,-0.125)..(0.550,0.100,0.125) | none | 0 | 0 | 0 | stone_rough |
| `architecture/openings/window_lintel_standard` | 12 | 1 | 1 | 1 | (-0.600,0.000,-0.100)..(0.600,0.120,0.100) | none | 0 | 0 | 0 | oak_timber |
| `architecture/traversal/stair_straight_3m_with_rails` | 288 | 24 | 1 | 24 | (-0.585,0.000,-2.000)..(0.585,4.050,2.000) | compound_bounds | 24 | 16 | 0 | oak_timber |
| `architecture/traversal/stair_rail_slope_3m` | 84 | 1 | 1 | 1 | (-0.040,0.000,-2.000)..(0.040,3.950,2.000) | bounds | 0 | 0 | 0 | oak_timber |
| `architecture/traversal/stair_newel_post` | 24 | 1 | 1 | 1 | (-0.080,0.000,-0.080)..(0.080,1.120,0.080) | bounds | 0 | 0 | 0 | oak_timber |
| `architecture/structural/foundation_plinth_straight_2m` | 24 | 1 | 1 | 1 | (-1.000,0.000,-0.150)..(1.000,0.500,0.150) | none | 0 | 0 | 0 | stone_rough |
| `architecture/structural/foundation_plinth_straight_4m` | 24 | 1 | 1 | 1 | (-2.000,0.000,-0.150)..(2.000,0.500,0.150) | none | 0 | 0 | 0 | stone_rough |
| `architecture/structural/foundation_plinth_inner_corner` | 48 | 1 | 1 | 1 | (-0.150,0.000,-0.500)..(0.500,0.500,0.150) | none | 0 | 0 | 0 | stone_rough |
| `architecture/structural/foundation_plinth_outer_corner` | 60 | 1 | 1 | 1 | (-0.150,0.000,-0.500)..(0.500,0.550,0.150) | none | 0 | 0 | 0 | stone_rough |
| `architecture/structural/foundation_plinth_end` | 24 | 1 | 1 | 1 | (0.000,0.000,-0.150)..(0.450,0.500,0.150) | none | 0 | 0 | 0 | stone_rough |
| `architecture/structural/post_square_0p3x3m` | 12 | 1 | 1 | 1 | (-0.150,0.000,-0.150)..(0.150,3.000,0.150) | bounds | 0 | 0 | 0 | oak_timber |
| `architecture/structural/post_square_0p5x3m` | 12 | 1 | 1 | 1 | (-0.250,0.000,-0.250)..(0.250,3.000,0.250) | bounds | 0 | 0 | 0 | oak_timber |
| `architecture/structural/column_round_0p5x3m` | 60 | 1 | 1 | 3 | (-0.250,0.000,-0.250)..(0.250,3.000,0.250) | bounds | 0 | 0 | 2 | stone_rough |
| `architecture/structural/column_round_base` | 132 | 1 | 1 | 2 | (-0.300,0.000,-0.300)..(0.300,0.250,0.300) | none | 0 | 0 | 1 | stone_rough |
| `architecture/structural/column_round_cap` | 72 | 1 | 1 | 2 | (-0.300,0.000,-0.300)..(0.300,0.250,0.300) | none | 0 | 0 | 1 | stone_rough |
| `architecture/structural/beam_2m` | 12 | 1 | 1 | 1 | (-1.000,0.000,-0.100)..(1.000,0.300,0.100) | bounds | 0 | 0 | 0 | oak_timber |
| `architecture/structural/beam_4m` | 12 | 1 | 1 | 1 | (-2.000,0.000,-0.100)..(2.000,0.300,0.100) | bounds | 0 | 0 | 0 | oak_timber |
| `architecture/structural/beam_6m` | 12 | 1 | 1 | 1 | (-3.000,0.000,-0.125)..(3.000,0.450,0.125) | bounds | 0 | 0 | 0 | oak_timber |
| `architecture/structural/beam_end_cap` | 24 | 1 | 1 | 1 | (0.000,0.000,-0.130)..(0.240,0.360,0.130) | none | 0 | 0 | 0 | oak_timber |
| `architecture/structural/brace_left` | 12 | 1 | 1 | 1 | (0.000,0.000,-0.060)..(0.800,0.850,0.060) | bounds | 0 | 0 | 0 | oak_timber |
| `architecture/structural/brace_right` | 12 | 1 | 1 | 1 | (-0.800,0.000,-0.060)..(0.000,0.850,0.060) | bounds | 0 | 0 | 0 | oak_timber |
| `architecture/roof/ridge_cap_straight_2m` | 8 | 1 | 1 | 1 | (-1.000,0.000,-0.200)..(1.000,0.115,0.200) | bounds | 0 | 0 | 0 | shingle_oak |
| `architecture/roof/eave_trim_2m` | 36 | 1 | 1 | 1 | (-1.000,0.000,-0.170)..(1.000,0.300,0.030) | none | 0 | 0 | 0 | oak_timber |
| `architecture/roof/eave_trim_4m` | 48 | 1 | 1 | 1 | (-2.000,0.000,-0.170)..(2.000,0.300,0.030) | none | 0 | 0 | 0 | oak_timber |
| `architecture/roof/eave_outer_corner` | 36 | 1 | 1 | 1 | (-0.030,0.000,-0.600)..(0.600,0.300,0.030) | none | 0 | 0 | 0 | oak_timber |
| `architecture/roof/fascia_end` | 24 | 1 | 1 | 1 | (-0.050,0.000,-0.050)..(0.300,0.300,0.050) | none | 0 | 0 | 0 | oak_timber |
| `architecture/roof/gable_cap_4m` | 24 | 1 | 1 | 1 | (-2.000,0.000,-0.030)..(2.000,1.335,0.030) | bounds | 0 | 0 | 0 | oak_timber |
| `architecture/roof/gutter_straight_2m` | 36 | 1 | 1 | 1 | (-1.000,0.000,-0.090)..(1.000,0.140,0.090) | none | 0 | 0 | 0 | oak_plank |
| `architecture/roof/downspout_3m` | 12 | 1 | 1 | 1 | (-0.050,0.000,-0.050)..(0.050,3.000,0.050) | none | 0 | 0 | 0 | iron_forged |
| `architecture/roof/downspout_outlet` | 24 | 1 | 1 | 1 | (-0.050,0.000,-0.050)..(0.050,0.500,0.300) | none | 0 | 0 | 0 | iron_forged |
| `architecture/roof/chimney_stack_short` | 24 | 1 | 1 | 2 | (-0.475,0.000,-0.475)..(0.475,2.200,0.475) | bounds | 0 | 0 | 1 | stone_rough |
| `architecture/roof/chimney_cap` | 36 | 1 | 1 | 2 | (-0.450,0.000,-0.450)..(0.450,0.330,0.450) | none | 0 | 0 | 1 | stone_rough |
<!-- METRICS-TABLE:END -->

Validator path & tool versions: pygltflib 1.16.5, Blender 4.5.9 LTS; see
`../calibration/README.md`. Importer oracle: `static_mesh_asset_tests.cpp`;
socket round trip: `calibration_bay_tests.cpp`.

## Door state family (Production Law 3)

`door_leaf_standard_closed` and `_open` are two static assets under one semantic
family sharing an **identical hinge pivot**: object origin at the hinge edge
(local x=0) and one `plug` socket at that origin. `_closed` fills the opening
(+X); `_open` is the same leaf statically swung 90° into −Y. The frame carries a
`receiver` socket `hinge.left` at its hinge line; compatibility family
`architecture.door_standard` pairs the leaf plug to the frame receiver. Every
frame uses receiver sockets; every leaf uses a compatible plug (backlog section
D). Socket keys are documented in `../calibration/README.md`.

## Stair (backlog section E)

`stair_straight_3m` fills the 3.0 m storey rise **exactly** (16 risers ×
0.1875 m; riser < 0.35 m auto-step). Compound collision has one walkable
`bounds` part per visible step (16 parts, all walkable) — the compound collision
matches the visible steps (Q3). `stair_landing_2x2m` is a single walkable
`bounds` slab that rests at the stair top.

## Roof (backlog section F)

Base roof planes remain **generated** (Law 1); these caps finish the ridge seam.
`ridge_cap_straight_4m` and `ridge_cap_end` rest on Y=0 as authored and place at
the ridge line with no offset. In `assembled_bay.png` the ridge cap sits at the
3.0 m eaves with open space beneath — that gap is the generated roof plane's
domain, **not** a Blender↔engine offset (see Q4 in the completion brief).

## Per-Asset Acceptance Records

```
assetId: architecture/openings/door_frame_standard
family: openings; status: REPLACE (0.9×2.1 m clear opening)
source: tools/generate_kit.py (build_door_frame_standard); glb: openings/door_frame_standard.glb
nominal bounds m: (-0.500,0,-0.075)..(0.500,2.250,0.075); clear opening 0.9×2.1
collision: compound_bounds; parts: 3 (2 jambs + lintel); walkable: 0
sockets: 1 receiver `hinge.left` (family architecture.door_standard) at engine (-0.45,0,0)
pivot: opening center on ground; forward/up: +Z / +Y
state family: standard door; recipes: opening insert on a generated wall cut
gallery: ../calibration/proofs/gallery_all16.png; assembled: proofs/assembled_bay.png
import: static_mesh_asset_tests Authored/compound; save-load/align: calibration_bay_tests (Q6)
known limitations: single width; wide/double frames are later batch rows
```
```
assetId: architecture/openings/door_leaf_standard_closed
family: openings; status: REPLACE
source: build_door_leaf(closed); glb: openings/door_leaf_standard_closed.glb
nominal bounds m: (0.000,0,-0.025)..(0.900,2.100,0.025)
pivot contract: HINGE at local origin (bounds.min.x==0); plug at origin
sockets: 1 plug `hinge` (family architecture.door_standard) at origin (0,0,0)
collision: bounds; parts/walk: 0/0; state family: standard door (closed)
import: Authored/bounds; align: leaf plug meets frame receiver within 1 mm (calibration_bay_tests)
gallery: gallery_all16.png; assembled: assembled_bay.png (socketed to frame)
known limitations: static state (animation replaces the pair later, same id)
```
```
assetId: architecture/openings/door_leaf_standard_open
family: openings; status: NEW (static open state)
source: build_door_leaf(open); glb: openings/door_leaf_standard_open.glb
nominal bounds m: (-0.025,0,0.000)..(0.025,2.100,0.900)
pivot contract: identical hinge to _closed (origin at hinge; plug `hinge` at origin)
sockets: 1 plug `hinge` (architecture.door_standard); collision: bounds; parts/walk: 0/0
state family: standard door (open); import: Authored/bounds
gallery: gallery_all16.png; known limitations: static 90° swing into −Y
```
```
assetId: architecture/traversal/stair_straight_3m
family: traversal; status: NEW
source: build_stair_straight_3m; glb: traversal/stair_straight_3m.glb
nominal bounds m: (-0.500,0,-2.000)..(0.500,3.000,2.000); rise EXACTLY 3.0 m
tread/riser: 16 × riser 0.1875 (< 0.35 auto-step), tread 0.25, width 1.0
collision: compound_bounds; parts: 16 (one per step); walkable parts: 16
pivot: base-center on ground; forward/up: +Z / +Y
gallery: gallery_all16.png; assembled/side: proofs/assembled_bay.png, assembled_bay_side.png
collision proof: ../calibration/proofs/collision_walkability.png
import: Authored/compound (walkable=true, Q3); known limitations: straight run only
```
```
assetId: architecture/traversal/stair_landing_2x2m
family: traversal; status: NEW
source: build_stair_landing_2x2m; glb: traversal/stair_landing_2x2m.glb
nominal bounds m: (-1.000,0,-1.000)..(1.000,0.200,1.000)
collision: bounds; iggy_walkable: true (single walkable top); parts/walk: 0/1(single)
pivot: base-center on ground; state family: landing
gallery: gallery_all16.png; assembled: rests on the stair top in assembled_bay.png
import: Authored/bounds walkable; known limitations: 2×2 m only
```
```
assetId: architecture/structural/railing_straight_2m
family: structural; status: NEW (traversal blocker)
source: build_railing_straight_2m; glb: structural/railing_straight_2m.glb
nominal bounds m: (-1.000,0,-0.040)..(1.000,1.000,0.040); 2.0 m run, 1.0 m tall
collision: bounds (aggregate blocker); parts/walk/sockets: 0/0/0
pivot: run-center on ground; gallery: gallery_all16.png; assembled: on the landing edge
import: Authored/bounds; known limitations: straight module; corner/end-post are later rows
```
```
assetId: architecture/roof/ridge_cap_straight_4m
family: roof; status: REPLACE
source: build_ridge_cap_straight_4m; glb: roof/ridge_cap_straight_4m.glb
nominal bounds m: (-2.000,0,-0.200)..(2.000,0.115,0.200); 4.0 m ridge trim
pivot contract: apex-line underside on the ground (Y=0), rest pose as-mounted
  (apex up, symmetric slopes); pitch/pose baked into verts, node TRS identity
collision: bounds; parts/walk/sockets: 0/0/0
gallery: gallery_all16.png (broadside); assembled: ridge line in assembled_bay.png
import: Authored/bounds
known limitations: caps a GENERATED roof plane (Law 1); dihedral is the roof
  recipe DEFAULT pitch only (30 deg, kDefaultCreativeStructuralRoofPitchDegrees);
  other pitches are later rows
```
```
assetId: architecture/roof/ridge_cap_end
family: roof; status: NEW (finished ridge end)
source: build_ridge_cap_end; glb: roof/ridge_cap_end.glb
nominal bounds m: (-0.200,0,-0.200)..(0.200,0.115,0.200); tapered finished end
pivot contract: apex-line underside on ground (Y=0), rest pose apex up; pitch
  baked into verts, node TRS identity
collision: bounds; parts/walk/sockets: 0/0/0
gallery: gallery_all16.png (broadside); assembled: ridge end in assembled_bay.png
import: Authored/bounds
known limitations: matches the 0.4 m ridge profile; roof recipe DEFAULT pitch
  only (30 deg); other pitches are later rows
```
