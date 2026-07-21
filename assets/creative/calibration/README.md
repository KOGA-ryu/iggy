# Calibration & Pipeline-Proof Kit (ASSET-CAL-1, Batch 0)

Developer-facing calibration assets — the first half of the ASSET-CAL-1 batch.
These prove meter scale, ground plane, pivots, compound collision, walkability,
and sockets before any visual polish (backlog section A; Production Laws 1-6).
Per backlog section A they are hidden from the normal creator catalog.

Stable ids (verbatim from the backlog "First Linux/Blender Work Order"):
`calibration/grid_1m_10x10`, `human_gauge_1p8m`, `door_clearance_0p9x2p1`,
`storey_3m`, `pivot_hinge`, `socket_receiver`, `socket_plug`,
`collision_compound`.

## Regenerate (headless only — no window is ever opened)

```
blender --background --python assets/creative/calibration/tools/generate_kit.py     # 8 GLBs
blender --background --python assets/creative/calibration/tools/render_proofs.py     # proofs/
python3 assets/creative/calibration/tools/lint_contracts.py --write                 # README tables
python3 assets/creative/calibration/tools/lint_contracts.py                         # gate: validate
```

The Python generator is the deterministic source (Production Law: scripted,
re-runnable builds — no hand-saved `.blend`, matching the stealth_blockout kit
precedent of a `tools/` generator rather than a `source/` subdirectory).

## Metrics (mechanically generated — do not hand-edit)

Counts are emitted by `lint_contracts.py`; every value below is measured from
the exported GLB, never typed. Bounds are in the engine's glTF Y-up space
(X = width, Y = vertical, Z = depth). `walkable` is the walkable-part count for
compounds, or 1/0 for a single `bounds` asset that declares `iggy_walkable`.

<!-- METRICS-TABLE:BEGIN -->
| assetId | tris | prims | mats | nodes | bounds min..max (m, glTF Y-up) | collision | parts | walkable | sockets |
|---|--:|--:|--:|--:|---|---|--:|--:|--:|
| `calibration/grid_1m_10x10` | 264 | 1 | 1 | 1 | (-5.010,0.000,-5.010)..(5.010,0.020,5.010) | none | 0 | 0 | 0 |
| `calibration/human_gauge_1p8m` | 72 | 1 | 1 | 1 | (-0.250,0.000,-0.150)..(0.250,1.800,0.150) | none | 0 | 0 | 0 |
| `calibration/door_clearance_0p9x2p1` | 48 | 1 | 1 | 1 | (-0.500,0.000,-0.025)..(0.500,2.150,0.025) | none | 0 | 0 | 0 |
| `calibration/storey_3m` | 72 | 1 | 1 | 1 | (-0.300,0.000,-0.300)..(0.300,3.000,0.300) | none | 0 | 0 | 0 |
| `calibration/pivot_hinge` | 12 | 1 | 1 | 1 | (0.000,0.000,-0.025)..(0.900,2.100,0.025) | bounds | 0 | 0 | 0 |
| `calibration/socket_receiver` | 60 | 2 | 2 | 2 | (-0.200,0.000,-0.200)..(0.200,0.900,0.200) | none | 0 | 0 | 1 |
| `calibration/socket_plug` | 60 | 2 | 2 | 2 | (-0.100,0.000,-0.200)..(0.200,0.520,0.100) | none | 0 | 0 | 1 |
| `calibration/collision_compound` | 24 | 2 | 2 | 2 | (-0.700,0.000,-0.700)..(0.700,0.700,0.700) | compound_bounds | 2 | 1 | 0 |
<!-- METRICS-TABLE:END -->

## Validator path & tool versions

This box has no Khronos `gltf-validator` binary, so the linter validates with
`pygltflib` structural parse + explicit structural/bounds/extras checks:

- pygltflib 1.16.5
- Blender 4.5.9 LTS (glTF 2.0 GLB exporter, `export_yup=True`, `export_extras=True`)
- The engine importer is the ultimate oracle: `tests/unit/static_mesh_asset_tests.cpp`
  imports every GLB and pins each contract; `tests/unit/calibration_bay_tests.cpp`
  proves the socket round trip in a loaded world.

## Metadata contract (Production Law 5)

Matched to `assets/creative/stealth_blockout` and to the importer
(`src/content/assets/StaticMeshAuthoringMetadata.cpp`):

- single collidable box: `iggy_category`, `iggy_collision:"bounds"`, optional
  `iggy_walkable:true` (only legal on `bounds`);
- non-colliding proof: `iggy_category`, `iggy_collision:"none"`;
- compound: every node `iggy_collision:"compound_bounds"`; collidable nodes add
  `iggy_collision_part:"bounds"` + `iggy_collision_part_walkable`.

### Socket contract (recorded key names)

The stealth_blockout kit has **no** socket precedent, but sockets are already
established engine contract — the homestead modular kit uses them
(`homestead/modular/door_frame_1p5x2p46` receiver, `door_leaf_1p1x2p2` plug) and
the importer defines them in `parseStaticMeshAttachmentSocket`. These calibration
assets are the first *calibration* socket proofs; they use the existing keys:

| key | value |
|---|---|
| `iggy_socket` | socket name, e.g. `receiver.main`, `plug.main` (`[A-Za-z0-9_.-]`) |
| `iggy_socket_role` | `receiver` or `plug` |
| `iggy_socket_compatibility` | family string, here `calibration.socket` |

A socket is an **unrotated (or yaw-only) empty node**. The importer reads the
node world transform: position = node origin, forward = glTF +Z (= Blender −Y),
up = glTF +Y (= Blender +Z). The attachment snap requires up.y > 0.999, so
sockets must not tilt. Compatibility family `calibration.socket` pairs
`socket_receiver`'s `receiver.main` with `socket_plug`'s `plug.main`.

## Proofs (Production Law 6)

- `proofs/gallery_all16.png` — contact sheet of all 16 batch assets;
- `proofs/scale_human_gauge.png` — scale row beside the 1.8 m gauge;
- `proofs/collision_walkability.png` — walkable (green) vs non-walkable support (brown);
- `proofs/socket_alignment.png` — receiver+plug and door frame+leaf aligned;
- the assembled 4×4 m bay: `../architecture/proofs/assembled_bay.png` (+ `_side`).

## Per-Asset Acceptance Records

```
assetId: calibration/grid_1m_10x10
family: calibration; status: NEW
source blend: tools/generate_kit.py (build_grid); runtime glb: grid_1m_10x10.glb
nominal bounds m (glTF Y-up): (-5.010,0,-5.010)..(5.010,0.020,5.010)
pivot contract: center of the 10x10 m grid, on the ground (Y=0)
forward/up axes: glTF +Z forward / +Y up (Blender Z-up exported yup)
collision mode: none; collision parts: 0; walkable parts: 0; sockets: 0
state family: -; compatible recipes: reference overlay (non-colliding)
gallery proof: proofs/gallery_all16.png; assembled proof: n/a (reference)
import result: static_mesh_asset_tests Authored/none
placement/save-load: loaded as "Bay Grid" in calibration_bay_tests
replacement result: id path scheme proven (Q5); known limitations: 1 m grid only
```
```
assetId: calibration/human_gauge_1p8m
family: calibration; status: NEW (adopts the player-gauge role as scale figure)
source blend: tools/generate_kit.py (build_human_gauge); runtime glb: human_gauge_1p8m.glb
nominal bounds m: (-0.250,0,-0.150)..(0.250,1.800,0.150)  (exact 1.8 m height)
pivot contract: base-center on the ground (Y=0); forward/up: +Z / +Y
collision mode: none; parts: 0; walkable: 0; sockets: 0
datum plates: stand/guard eye 1.6 m, sneak eye 0.9 m
gallery proof: gallery_all16.png; assembled proof: assembled_bay.png (scale)
import result: Authored/none; save-load: "Bay Scale Gauge" in calibration_bay_tests
known limitations: blockout silhouette, not a final mannequin
```
```
assetId: calibration/door_clearance_0p9x2p1
family: calibration; status: NEW
source: generate_kit.py (build_door_clearance); glb: door_clearance_0p9x2p1.glb
nominal bounds m: (-0.500,0,-0.025)..(0.500,2.150,0.025)
clear opening (inner faces): 0.900 m wide × 2.100 m tall (Law 4)
pivot: opening center on the ground; collision: none; parts/walk/sockets: 0/0/0
gallery proof: gallery_all16.png; import: Authored/none
known limitations: gauge only (outer posts 0.05 m add to bounds, opening is exact)
```
```
assetId: calibration/storey_3m
family: calibration; status: NEW
source: generate_kit.py (build_storey); glb: storey_3m.glb
nominal bounds m: (-0.300,0,-0.300)..(0.300,3.000,0.300)  (exact 3.0 m storey)
marker plates: floor 0.0, eye 1.6, lintel 2.1, ceiling 2.7, next-floor 3.0
pivot: base-center on ground; collision: none; parts/walk/sockets: 0/0/0
gallery proof: gallery_all16.png; assembled: assembled_bay.png (storey gauge)
import: Authored/none; known limitations: single storey height
```
```
assetId: calibration/pivot_hinge
family: calibration; status: NEW
source: generate_kit.py (build_pivot_hinge); glb: pivot_hinge.glb
nominal bounds m: (0.000,0,-0.025)..(0.900,2.100,0.025)
pivot contract: HINGE at local origin -> bounds.min.x == 0.000 (proves the
  hinge pivot survives Blender->glTF conversion, Q2)
forward/up: +Z / +Y; collision: bounds; parts/walk/sockets: 0/0/0
gallery proof: gallery_all16.png; import: Authored/bounds
known limitations: proof panel; production leaves live under architecture/openings
```
```
assetId: calibration/socket_receiver
family: calibration; status: NEW (defines the calibration socket proof)
source: generate_kit.py (build_socket_receiver); glb: socket_receiver.glb
nominal bounds m: (-0.200,0,-0.200)..(0.200,0.900,0.200)
socket: receiver.main (receiver, family calibration.socket) at engine (0,0.5,0)
pivot: base-center on ground; collision: none; parts/walk: 0/0; sockets: 1 (1 receiver)
gallery proof: gallery_all16.png; socket proof: socket_alignment.png
save-load/align: "Bay Socket Receiver" — plug aligns within 1 mm (calibration_bay_tests, Q6)
known limitations: visualization frame only
```
```
assetId: calibration/socket_plug
family: calibration; status: NEW
source: generate_kit.py (build_socket_plug); glb: socket_plug.glb
nominal bounds m: (-0.100,0,-0.200)..(0.200,0.520,0.100)
socket: plug.main (plug, family calibration.socket) at engine (0,0.3,0)
pivot: base-center on ground; collision: none; parts/walk: 0/0; sockets: 1 (1 plug)
gallery proof: gallery_all16.png; socket proof: socket_alignment.png
align: plug socket coincides with receiver socket in engine space (calibration_bay_tests, Q6)
known limitations: proof plug only
```
```
assetId: calibration/collision_compound
family: calibration; status: NEW
source: generate_kit.py (build_collision_compound); glb: collision_compound.glb
nominal bounds m: (-0.700,0,-0.700)..(0.700,0.700,0.700)
parts: 2 (support 1.0×1.0×0.5 non-walkable; cap 1.4×1.4×0.2 walkable)
collision: compound_bounds; walkable parts: 1; sockets: 0
pivot: base-center on ground; forward/up: +Z / +Y
gallery proof: gallery_all16.png; collision proof: collision_walkability.png
import: Authored/compound (aggregate walkable=true); known limitations: 2-part proof
```
