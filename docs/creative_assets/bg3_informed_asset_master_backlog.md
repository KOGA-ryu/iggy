# BG3-Informed Creative Asset Master Backlog

## Purpose

This document is the exhaustive asset-production map for the first complete
iggy3d Creative world-building library. It covers the assets needed to author
fantasy estates, settlements, roads, rivers, woodland, cliffs, caves, interiors,
stealth spaces, and the first settlement-defense gameplay loop.

It is exhaustive within that product boundary. It is not an attempt to list
every object that could exist in every future biome, culture, or art style.

The list separates four things that must not be conflated:

- generated geometry owned by Creative recipes;
- reusable Blender resources;
- semantic definitions and state variants built around those resources;
- features blocked on engine support.

The current 133 checked-in GLBs provide broad placeholder coverage. They should
not be thrown away. They are calibration and interaction proofs until a named
production replacement reaches parity.

## Research Basis: How BG3 Organizes Assets

The official Baldur's Gate 3 Toolkit documentation exposes a layered asset
model rather than a mesh-only catalog:

1. Models, textures, materials, animations, and other resources are managed as
   distinct resources. A Visual Resource selects materials, animations, and
   attachments.
2. A reusable Root Template points at resources and carries the object's
   reusable identity and behavior. Root templates are the building blocks used
   for trees, rocks, items, chests, and characters.
3. A placed object becomes a Local Template. It inherits from its root while
   allowing local overrides. Editing the root updates inheriting instances.
4. UUID identity is preferred over names because names are not unique.
5. Visual and physics resources can be assigned separately. Equipment and
   character content add composition, slot, variation, and fallback layers
   rather than duplicating one complete asset for every combination.
6. Larian's published level-design workflow begins with whitebox areas, then
   hands them through environment art, scripting, combat, and cinematics before
   finalization.

Primary references:

- [Toolkit navigation and root/local templates](https://docs.baldursgate3.game/Editor%3A_Navigation)
- [Toolkit project/resource organization](https://docs.baldursgate3.game/Getting_Started%3A_Creating_a_New_Mod)
- [Resource override and package model](https://docs.baldursgate3.game/index.php?title=Editor%3A_Overriding_Resources)
- [Model, material, texture, physics, root-template, and equipment composition](https://docs.baldursgate3.game/Adding_Armour)
- [Larian level-design whitebox and handoff responsibilities](https://larian.com/careers/87954a51-6677-4c7a-8b5b-a40210d653e0)
- [Larian BG3 environment-art survey](https://magazine.artstation.com/2023/09/larian-studios-baldurs-gate-3-art-blast/)
- [BG3 character customization production overview](https://gdcvault.com/play/1034742/Adobe-Developer-Summit-The-Art)
- [BG3 environment artist portfolio showing asset and modular-set ownership](https://www.artstation.com/timwilmsen)

The iggy3d translation is:

```text
Blender source + GLB metadata
    -> StaticMeshAsset resource
    -> stable catalog/authored-asset definition
    -> semantic recipe or state family
    -> placed CreativeObject instance with local transform/hierarchy overrides
    -> building/map template composition
```

This is an architectural reference, not a request to copy BG3 art, meshes,
textures, names, or proprietary content.

## Production Laws

### Law 1: Generated surfaces stay generated

Do not create a separate Blender mesh for every wall length, room size, floor
rectangle, roof plane, terrain patch, or contour. Creative's semantic recipes
own those continuous surfaces. Blender owns inserts, junctions, trim,
traversal pieces, dressing, landmarks, and hero geometry.

### Law 2: One family, one reusable identity

Each reusable asset has one stable `assetId`. Visual revisions replace the GLB
under that ID when bounds and pivot contracts remain compatible. Do not make
timestamped or artist-named runtime IDs.

### Law 3: State is explicit

Until animation is supported, open/closed, intact/broken, raised/lowered, and
construction/damage states are separate static resources under one documented
semantic family. Do not hide state by moving mesh nodes at runtime without a
contract.

### Law 4: Scale is physical

- Blender units are meters.
- Z is vertical in Blender; export through the standard Y-up glTF conversion.
- Grounded assets rest on Z = 0.
- A normal storey is 3.0 m floor-to-floor unless a recipe says otherwise.
- A normal interior door clear opening is approximately 0.9 x 2.1 m.
- Pivots, openings, tread/riser dimensions, collision, and sockets are checked
  against calibration assets before visual polish.
- Never compensate in art for a camera, recipe, or unit-conversion bug.

### Law 5: Metadata is part of the asset

Every GLB declares the relevant current contract:

- `iggy_category`;
- `iggy_collision` as `bounds`, `compound_bounds`, or `none`;
- `iggy_walkable=true` only for valid aggregate walkable tops;
- compound collision parts and per-part walkability;
- named receiver/plug sockets with compatibility families.

Convex and mesh collision, skins, morphs, animation, alpha, advanced PBR maps,
decals, particles, water, and LOD selection are not silently approximated.
Their assets remain gated until the engine contract exists.

### Law 6: A kit ships as a proof, not a folder

Every kit delivery includes:

- editable Blender source;
- exported GLBs at stable IDs;
- one gallery render;
- one assembled use-case render;
- one scale render with the 1.8 m gauge;
- collision and walkability visualization;
- socket visualization when applicable;
- triangle, primitive, material, node, bounds, collision-part, and socket counts;
- importer verification and one in-engine composition fixture.

## Status Vocabulary

| Status | Meaning |
|---|---|
| `KEEP` | Existing asset remains a valid proof or placeholder. |
| `REPLACE` | Existing stable ID is needed, but geometry/art needs a production pass. |
| `NEW` | Missing static asset that the current importer can support. |
| `COMPOSE` | A reusable composition/template made from assets, not another monolithic GLB. |
| `GENERATED` | Creative recipe output; explicitly not a Blender deliverable. |
| `GATED` | Do not produce final content until named engine support exists. |

## Current Baseline

The checked-in library has 133 GLBs:

| Family | Count | Current role |
|---|---:|---|
| Cave | 16 | Modular proof kit |
| Homestead modular | 20 | Building and traversal proof kit |
| Homestead interior | 10 | Furniture/socket proof kit |
| Homestead yard | 11 | Yard/gate/socket proof kit |
| Homestead loose proofs | 2 | Table and wall-bay proofs |
| Infrastructure | 15 | Path, retaining, stair, and culvert proof kit |
| Rock/cliff | 16 | Natural boundary proof kit |
| Riverbank | 2 | Early natural props |
| Woodland | 13 | Solid-geometry vegetation proof kit |
| Stealth blockout | 26 | Scale, cover, climb, and movement proofs |
| Root fixtures | 2 | Import and texture fixtures |

These prove import, instancing, bounds, compound collision, walkability,
sockets, placement, and replacement. They do not yet form a coherent final
production library.

## Master Asset Registry

### A. Calibration and Pipeline Proofs

These are developer-facing and should be hidden from the normal creator catalog.

| Target asset ID | Status | Required contract |
|---|---|---|
| `calibration/grid_1m_10x10` | NEW | Non-colliding one-meter reference grid |
| `calibration/cube_1m` | NEW | Exact 1 x 1 x 1 m bounds |
| `calibration/human_gauge_1p8m` | NEW | Replace/adopt the current player gauge as the canonical scale figure |
| `calibration/door_clearance_0p9x2p1` | NEW | Exact clear opening gauge |
| `calibration/storey_3m` | NEW | Floor, eye-height, lintel, ceiling, and next-floor markers |
| `calibration/stair_3m_standard` | NEW | Standard tread/riser and full-storey rise proof |
| `calibration/ramp_walkable` | NEW | Compound walkable incline approximation proof |
| `calibration/pivot_center` | NEW | Center-pivot rotation proof |
| `calibration/pivot_bottom_center` | NEW | Ground-placement pivot proof |
| `calibration/pivot_hinge` | NEW | Door/gate hinge-pivot proof |
| `calibration/socket_receiver` | NEW | Visible receiver frame and axis markers |
| `calibration/socket_plug` | NEW | Compatible plug alignment proof |
| `calibration/collision_bounds` | NEW | Visual mesh plus aggregate bounds overlay proof |
| `calibration/collision_compound` | NEW | Multi-part collision and walkable-top proof |
| `calibration/collision_none` | NEW | Visible non-colliding proof |
| `calibration/material_base_color` | NEW | Current supported material/color proof |
| `calibration/material_texture_uv` | NEW | UV, sampler, and base-color texture proof |
| `calibration/material_missing_texture` | NEW | Required visible fallback proof |
| `calibration/scale_gallery` | COMPOSE | One scene containing every physical reference |

### B. Generated Structural Surfaces

These are required capabilities but are not Blender assets.

| Semantic output | Status | Owner |
|---|---|---|
| Arbitrary rectangular floor slab | GENERATED | Building/floor recipe |
| Arbitrary room floor plan | GENERATED | Building blockout recipe |
| Continuous exterior wall per facade/storey stack | GENERATED | Building shell recipe |
| Interior partition per floor plan | GENERATED | Building shell recipe |
| Opening cuts in generated walls | GENERATED | Opening recipe |
| Flat/gable base roof planes | GENERATED | Roof recipe |
| Terrain height field and contour surface | GENERATED | Terrain recipe |
| Road/path spline base surface | GENERATED | Infrastructure recipe |
| River bed and bank spline base surface | GENERATED | River recipe |
| 2D plan symbols and thumbnails | GENERATED | Drafting projection and thumbnail renderer |

Blender production begins around these surfaces, not in competition with them.

### C. Structural Detail Kit

The generated shell needs readable structure, edges, and transitions.

| Target asset ID | Status | Notes |
|---|---|---|
| `architecture/structural/foundation_plinth_straight_2m` | NEW | Scaleable/repeatable straight trim |
| `architecture/structural/foundation_plinth_straight_4m` | NEW | Four-meter module |
| `architecture/structural/foundation_plinth_inner_corner` | NEW | 90-degree concave junction |
| `architecture/structural/foundation_plinth_outer_corner` | NEW | 90-degree convex junction |
| `architecture/structural/foundation_plinth_end` | NEW | Finished exposed end |
| `architecture/structural/post_square_0p3x3m` | NEW | Storey-height post |
| `architecture/structural/post_square_0p5x3m` | REPLACE | Production replacement for current pillar role |
| `architecture/structural/column_round_0p5x3m` | NEW | Round column |
| `architecture/structural/column_round_base` | NEW | Separate base/socketable trim |
| `architecture/structural/column_round_cap` | NEW | Separate cap/socketable trim |
| `architecture/structural/beam_2m` | NEW | Short beam |
| `architecture/structural/beam_4m` | REPLACE | Current beam role, production pass |
| `architecture/structural/beam_6m` | NEW | Long span with credible depth |
| `architecture/structural/beam_end_cap` | NEW | Exposed end treatment |
| `architecture/structural/brace_left` | NEW | Diagonal support |
| `architecture/structural/brace_right` | NEW | Mirrored support, explicit pivot |
| `architecture/structural/arch_1p2m` | NEW | Narrow passage/window arch |
| `architecture/structural/arch_2m` | NEW | Standard passage arch |
| `architecture/structural/arch_4m` | NEW | Wide hall arch |
| `architecture/structural/buttress_low` | NEW | One-storey shallow buttress |
| `architecture/structural/buttress_tall` | NEW | Multi-storey facade buttress |
| `architecture/structural/wall_pier_0p5x3m` | REPLACE | Production replacement for narrow wall pier |
| `architecture/structural/wall_pier_1x3m` | REPLACE | Current wall-pier role, production pass |
| `architecture/structural/parapet_straight_2m` | NEW | Roof/terrace edge |
| `architecture/structural/parapet_straight_4m` | NEW | Roof/terrace edge |
| `architecture/structural/parapet_inner_corner` | NEW | Concave corner |
| `architecture/structural/parapet_outer_corner` | NEW | Convex corner |
| `architecture/structural/parapet_end` | NEW | Finished end |
| `architecture/structural/balcony_deck_2x1p5m` | NEW | Walkable compound asset |
| `architecture/structural/balcony_deck_4x1p5m` | NEW | Walkable compound asset |
| `architecture/structural/balcony_bracket` | NEW | Non-walkable support |
| `architecture/structural/railing_straight_1m` | NEW | Traversal blocker |
| `architecture/structural/railing_straight_2m` | NEW | Traversal blocker |
| `architecture/structural/railing_corner` | NEW | Traversal blocker |
| `architecture/structural/railing_end_post` | NEW | Finished endpoint |

### D. Door, Gate, Window, and Hatch Families

Every frame uses receiver sockets. Every leaf uses a compatible plug and a
hinge-correct pivot. Static state pairs are required now; animation replaces
the pair later without changing semantic identity.

| Target asset ID | Status | Notes |
|---|---|---|
| `architecture/openings/door_frame_standard` | REPLACE | 0.9 x 2.1 m clear opening |
| `architecture/openings/door_leaf_standard_closed` | REPLACE | Hinge pivot, closed state |
| `architecture/openings/door_leaf_standard_open` | NEW | Static open state |
| `architecture/openings/door_frame_wide` | NEW | 1.2 m clear opening |
| `architecture/openings/door_leaf_wide_closed` | NEW | Hinge pivot |
| `architecture/openings/door_leaf_wide_open` | NEW | Static open state |
| `architecture/openings/door_frame_double` | NEW | 1.8 m clear opening, two receivers |
| `architecture/openings/door_leaf_double_left_closed` | NEW | Left hinge |
| `architecture/openings/door_leaf_double_left_open` | NEW | Left hinge open state |
| `architecture/openings/door_leaf_double_right_closed` | NEW | Right hinge |
| `architecture/openings/door_leaf_double_right_open` | NEW | Right hinge open state |
| `architecture/openings/door_frame_arched` | NEW | Arched lintel and compound collision |
| `architecture/openings/door_leaf_arched_closed` | NEW | Matching leaf |
| `architecture/openings/door_leaf_arched_open` | NEW | Matching open state |
| `architecture/openings/gate_frame_yard` | REPLACE | Existing yard gate role, production pass |
| `architecture/openings/gate_leaf_yard_closed` | REPLACE | Existing yard leaf role, hinge pivot |
| `architecture/openings/gate_leaf_yard_open` | NEW | Static open state |
| `architecture/openings/gate_frame_large` | NEW | Cart/fortification gate |
| `architecture/openings/gate_leaf_large_left_closed` | NEW | Left half |
| `architecture/openings/gate_leaf_large_left_open` | NEW | Left open state |
| `architecture/openings/gate_leaf_large_right_closed` | NEW | Right half |
| `architecture/openings/gate_leaf_large_right_open` | NEW | Right open state |
| `architecture/openings/window_frame_small` | NEW | Small service opening |
| `architecture/openings/window_frame_standard` | REPLACE | Current window-frame role, production pass |
| `architecture/openings/window_frame_wide` | NEW | Wide room opening |
| `architecture/openings/window_frame_tall` | NEW | Tall facade opening |
| `architecture/openings/window_frame_arched` | NEW | Arched opening |
| `architecture/openings/window_mullion_cross` | NEW | Socketable insert |
| `architecture/openings/window_shutter_left_closed` | NEW | Hinge pivot |
| `architecture/openings/window_shutter_left_open` | NEW | Static open state |
| `architecture/openings/window_shutter_right_closed` | NEW | Hinge pivot |
| `architecture/openings/window_shutter_right_open` | NEW | Static open state |
| `architecture/openings/window_bars_standard` | NEW | Solid opaque bars |
| `architecture/openings/window_sill_standard` | NEW | Wall-aligned trim |
| `architecture/openings/window_lintel_standard` | NEW | Wall-aligned trim |
| `architecture/openings/cellar_hatch_frame` | NEW | Horizontal receiver frame |
| `architecture/openings/cellar_hatch_closed` | NEW | Hinge pivot, closed state |
| `architecture/openings/cellar_hatch_open` | NEW | Static open state |
| `architecture/openings/trapdoor_frame` | NEW | Floor insert receiver |
| `architecture/openings/trapdoor_closed` | NEW | Walkable when closed |
| `architecture/openings/trapdoor_open` | NEW | Non-walkable opening state |
| `architecture/openings/glass_clear` | GATED | Requires alpha/material contract |

### E. Vertical Circulation and Traversal

All walkable pieces require per-tread/deck compound collision. Rail collision
must not block the walkable path.

| Target asset ID | Status | Notes |
|---|---|---|
| `architecture/traversal/stair_straight_1p5m` | REPLACE | Existing half-storey proof role |
| `architecture/traversal/stair_straight_3m` | NEW | Standard full-storey stair |
| `architecture/traversal/stair_straight_3m_with_rails` | NEW | Full assembly, compound collision |
| `architecture/traversal/stair_quarter_turn_3m` | NEW | L stair with landing |
| `architecture/traversal/stair_half_turn_3m` | NEW | U stair with landing |
| `architecture/traversal/stair_winder_3m` | NEW | Winder geometry, tread proof required |
| `architecture/traversal/stair_spiral_3m` | NEW | Compact stair, path-width proof required |
| `architecture/traversal/stair_landing_2x2m` | NEW | Walkable landing |
| `architecture/traversal/stair_landing_2x4m` | NEW | Walkable landing |
| `architecture/traversal/stair_rail_straight_2m` | NEW | Separate rail module |
| `architecture/traversal/stair_rail_slope_3m` | NEW | Matches standard stair pitch |
| `architecture/traversal/stair_newel_post` | NEW | Rail endpoint/junction |
| `architecture/traversal/ramp_2x3x1p5m` | REPLACE | Existing ramp role, production pass |
| `architecture/traversal/ramp_2x6x3m` | NEW | Full-storey ramp |
| `architecture/traversal/ramp_landing_2x2m` | NEW | Walkable landing |
| `architecture/traversal/ladder_2m` | NEW | Static climb marker and blocker contract |
| `architecture/traversal/ladder_3m` | NEW | One-storey ladder |
| `architecture/traversal/ladder_6m` | NEW | Two-storey ladder |
| `architecture/traversal/ladder_top_transition` | NEW | Readable exit geometry |
| `architecture/traversal/scaffold_platform_2x2m` | NEW | Walkable platform |
| `architecture/traversal/scaffold_platform_4x2m` | NEW | Walkable platform |
| `architecture/traversal/scaffold_stair_3m` | NEW | Construction-state traversal |
| `architecture/traversal/scaffold_ladder_3m` | NEW | Construction-state traversal |

### F. Roof, Eave, Drainage, and Chimney Kit

Base roof planes remain generated. These assets finish their seams and provide
readable features.

| Target asset ID | Status | Notes |
|---|---|---|
| `architecture/roof/ridge_cap_straight_2m` | NEW | Repeatable ridge trim |
| `architecture/roof/ridge_cap_straight_4m` | REPLACE | Existing ridge role, production pass |
| `architecture/roof/ridge_cap_end` | NEW | Finished ridge end |
| `architecture/roof/ridge_cap_t_junction` | NEW | Roof intersection |
| `architecture/roof/hip_cap_2m` | NEW | Diagonal hip trim |
| `architecture/roof/hip_cap_end` | NEW | Finished lower hip |
| `architecture/roof/valley_channel_2m` | NEW | Valley seam, no water simulation |
| `architecture/roof/eave_trim_2m` | NEW | Straight eave |
| `architecture/roof/eave_trim_4m` | NEW | Straight eave |
| `architecture/roof/eave_inner_corner` | NEW | Concave eave junction |
| `architecture/roof/eave_outer_corner` | NEW | Convex eave junction |
| `architecture/roof/fascia_end` | NEW | Finished exposed end |
| `architecture/roof/gable_cap_4m` | REPLACE | Existing gable-cap role |
| `architecture/roof/gutter_straight_2m` | NEW | Opaque static gutter |
| `architecture/roof/gutter_straight_4m` | NEW | Opaque static gutter |
| `architecture/roof/gutter_inner_corner` | NEW | Drainage junction |
| `architecture/roof/gutter_outer_corner` | NEW | Drainage junction |
| `architecture/roof/downspout_3m` | NEW | One storey |
| `architecture/roof/downspout_outlet` | NEW | Ground outlet |
| `architecture/roof/chimney_stack_short` | NEW | One roof-plane crossing |
| `architecture/roof/chimney_stack_tall` | NEW | Tall/steep roof variant |
| `architecture/roof/chimney_cap` | NEW | Socketable cap |
| `architecture/roof/dormer_frame_small` | NEW | Roof insert shell |
| `architecture/roof/dormer_roof_small` | NEW | Matching roof cap |
| `architecture/roof/skylight_frame` | NEW | Opaque frame only |
| `architecture/roof/skylight_glass` | GATED | Requires alpha/material contract |
| `architecture/roof/broken_edge_small` | NEW | Damage dressing, non-colliding |
| `architecture/roof/broken_edge_large` | NEW | Damage dressing, non-colliding |

### G. Roads, Paths, Retaining Structures, and Bridges

The existing infrastructure kit stays as the blockout proof. Production needs
transition and bridge closure more than more disconnected straight pieces.

| Target asset ID | Status | Notes |
|---|---|---|
| `infrastructure/path_stone_straight_2x2m` | REPLACE | Current path role |
| `infrastructure/path_stone_straight_4x2m` | REPLACE | Current path role |
| `infrastructure/path_stone_turn_4x4m` | REPLACE | Current path role |
| `infrastructure/path_stone_t_4x4m` | REPLACE | Current path role |
| `infrastructure/path_stone_cross_4x4m` | REPLACE | Current path role |
| `infrastructure/path_stone_end_2x2m` | REPLACE | Current path role |
| `infrastructure/path_stone_to_dirt_transition` | NEW | Generated-road transition |
| `infrastructure/path_stone_to_floor_transition` | NEW | Building threshold transition |
| `infrastructure/path_cobble_straight_4x4m` | NEW | Settlement street dressing |
| `infrastructure/path_cobble_turn_4x4m` | NEW | Settlement street dressing |
| `infrastructure/path_cobble_t_4x4m` | NEW | Settlement street dressing |
| `infrastructure/path_cobble_cross_4x4m` | NEW | Settlement street dressing |
| `infrastructure/curb_straight_2m` | NEW | Road edge trim |
| `infrastructure/curb_inner_corner` | NEW | Concave curb |
| `infrastructure/curb_outer_corner` | NEW | Convex curb |
| `infrastructure/curb_ramp` | NEW | Cart/access transition |
| `infrastructure/drain_grate` | NEW | Opaque inset prop |
| `infrastructure/drainage_culvert_2x2m` | REPLACE | Current compound culvert role |
| `infrastructure/retaining_wall_2m` | REPLACE | Current role |
| `infrastructure/retaining_wall_4m` | REPLACE | Current role |
| `infrastructure/retaining_wall_inner_corner` | NEW | Concave terrain junction |
| `infrastructure/retaining_wall_outer_corner` | REPLACE | Current corner role |
| `infrastructure/retaining_wall_end` | REPLACE | Current end role |
| `infrastructure/retaining_wall_stair_transition` | NEW | Retaining wall to steps |
| `infrastructure/bridge_timber_deck_4x2m` | REPLACE | Current bridge role |
| `infrastructure/bridge_timber_deck_8x2m` | NEW | Long deck module |
| `infrastructure/bridge_timber_rail_4m` | NEW | Side rail module |
| `infrastructure/bridge_timber_end` | NEW | Approach transition |
| `infrastructure/bridge_stone_deck_4x3m` | NEW | Walkable deck |
| `infrastructure/bridge_stone_arch_4m` | NEW | Compound opening collision |
| `infrastructure/bridge_stone_pier` | NEW | Reusable support |
| `infrastructure/bridge_stone_parapet_4m` | NEW | Side blocker |
| `infrastructure/bridge_abutment_left` | NEW | Terrain connection |
| `infrastructure/bridge_abutment_right` | NEW | Terrain connection |
| `infrastructure/stepping_stones_4m` | REPLACE | Current role |
| `infrastructure/ford_stones_6m` | NEW | Shallow-river crossing proof |

### H. Riverbank, Wetland, and Water-Edge Kit

The river surface itself is generated and visually gated. Banks, crossings,
and solid dressing can ship now.

| Target asset ID | Status | Notes |
|---|---|---|
| `riverbank/bank_straight_low_4m` | NEW | Low generated-bank transition |
| `riverbank/bank_straight_high_4m` | NEW | High cut bank |
| `riverbank/bank_inner_turn_4m` | NEW | Concave bend |
| `riverbank/bank_outer_turn_4m` | NEW | Convex bend |
| `riverbank/bank_end_left` | NEW | River-to-terrain closure |
| `riverbank/bank_end_right` | NEW | River-to-terrain closure |
| `riverbank/bank_rocky_straight_4m` | NEW | Rock dressing variant |
| `riverbank/bank_muddy_straight_4m` | NEW | Opaque sculpted variant |
| `riverbank/riverbed_gravel_4x4m` | NEW | Non-colliding dressing |
| `riverbank/riverbed_rock_cluster_small` | NEW | Non-colliding dressing |
| `riverbank/riverbed_rock_cluster_large` | NEW | Bounds collision |
| `riverbank/fallen_log_crossing_4m` | REPLACE | Current fallen-log role with walkability proof |
| `riverbank/driftwood_cluster_small` | NEW | Non-colliding dressing |
| `riverbank/driftwood_cluster_large` | NEW | Non-colliding dressing |
| `riverbank/reed_cluster_short` | REPLACE | Current reed role, opaque geometry |
| `riverbank/reed_cluster_tall` | NEW | Opaque geometry |
| `riverbank/cattail_cluster` | NEW | Opaque geometry |
| `riverbank/waterfall_ledge_4m` | NEW | Solid ledge only |
| `riverbank/waterfall_foam_mesh` | GATED | Requires alpha/VFX contract |
| `riverbank/water_surface_calm` | GATED | Requires water/material system |
| `riverbank/water_surface_flowing` | GATED | Requires flow/material system |

### I. Rock, Cliff, Cave, and Ruin-Natural Transitions

Current rock and cave kits prove the assembly language. Production requires
variation sets and explicit seams so repeated maps do not tile visibly.

| Target family | Status | Required variants |
|---|---|---|
| `rock_cliff/boulder_small` | NEW | 5 silhouettes, 0.4-0.8 m |
| `rock_cliff/boulder_medium` | REPLACE | 5 silhouettes, 1.2-2.0 m |
| `rock_cliff/boulder_large` | REPLACE | 5 silhouettes, 2.5-4.0 m |
| `rock_cliff/boulder_hero` | NEW | 3 climbable landmark silhouettes |
| `rock_cliff/rock_cluster_small` | REPLACE | 4 clusters |
| `rock_cliff/rock_cluster_medium` | NEW | 4 clusters |
| `rock_cliff/rock_cluster_large` | NEW | 4 clusters |
| `rock_cliff/outcrop_low` | REPLACE | 4 yaw-safe variants |
| `rock_cliff/outcrop_tall` | REPLACE | 4 yaw-safe variants |
| `rock_cliff/cliff_face_2x2m` | REPLACE | 4 tile variants |
| `rock_cliff/cliff_face_4x3m` | REPLACE | 4 tile variants |
| `rock_cliff/cliff_inner_corner` | REPLACE | 2 variants |
| `rock_cliff/cliff_outer_corner` | REPLACE | 2 variants |
| `rock_cliff/cliff_cap_walkable` | REPLACE | 4 cap variants |
| `rock_cliff/cliff_base_transition` | NEW | 4 transition variants |
| `rock_cliff/cliff_top_transition` | NEW | 4 transition variants |
| `rock_cliff/scree_pile` | REPLACE | 4 non-colliding variants |
| `rock_cliff/rubble_cluster` | REPLACE | 4 non-colliding variants |
| `rock_cliff/natural_steps` | REPLACE | 2 traversal variants |
| `rock_cliff/cave_mouth_small` | NEW | 3 m opening |
| `rock_cliff/cave_mouth_large` | REPLACE | 4-6 m opening |
| `cave/floor_4x4m` | REPLACE | 4 tile variants |
| `cave/wall_straight_4x3m` | REPLACE | 4 tile variants |
| `cave/wall_inner_corner` | REPLACE | 2 variants |
| `cave/wall_outer_corner` | REPLACE | 2 variants |
| `cave/ceiling_4x4m` | REPLACE | 3 tile variants |
| `cave/tunnel_straight` | REPLACE | 2 silhouettes |
| `cave/tunnel_turn` | REPLACE | 2 silhouettes |
| `cave/tunnel_t_junction` | REPLACE | 1 production pass |
| `cave/tunnel_cross_junction` | NEW | 1 production asset |
| `cave/tunnel_dead_end` | REPLACE | 2 silhouettes |
| `cave/chamber_small` | NEW | 6 x 6 m |
| `cave/chamber_medium` | REPLACE | 8 x 8 m |
| `cave/chamber_large` | NEW | 12 x 12 m |
| `cave/column` | REPLACE | 4 variants |
| `cave/stalactite_cluster` | REPLACE | 5 non-colliding variants |
| `cave/stalagmite_cluster` | REPLACE | 5 non-colliding variants |
| `cave/collapsed_passage` | REPLACE | 3 variants |
| `cave/ledge_walkable` | REPLACE | 3 variants |
| `cave/ramp_walkable` | REPLACE | 2 variants |

### J. Woodland, Meadow, Farm, and Ground Dressing

Vegetation remains opaque solid geometry until alpha and foliage rendering are
supported. Trunk collision is separate from canopy geometry.

| Target family | Status | Required variants |
|---|---|---|
| `woodland/broadleaf_young` | REPLACE | 4 silhouettes |
| `woodland/broadleaf_mature` | REPLACE | 6 silhouettes |
| `woodland/broadleaf_ancient` | REPLACE | 3 landmark silhouettes |
| `woodland/pine_young` | NEW | 3 silhouettes |
| `woodland/pine_medium` | REPLACE | 4 silhouettes |
| `woodland/pine_tall` | REPLACE | 4 silhouettes |
| `woodland/birch_young` | NEW | 3 silhouettes |
| `woodland/birch_mature` | NEW | 3 silhouettes |
| `woodland/dead_tree_snag` | REPLACE | 4 silhouettes |
| `woodland/tree_stump` | REPLACE | 4 silhouettes |
| `woodland/fallen_tree` | NEW | 3 sizes, walkability documented |
| `woodland/fallen_branch_pile` | REPLACE | 4 variants |
| `woodland/shrub_low` | REPLACE | 6 variants |
| `woodland/shrub_dense` | REPLACE | 6 variants |
| `woodland/fern_cluster` | REPLACE | 5 variants |
| `woodland/grass_clump_short` | NEW | 5 opaque variants |
| `woodland/grass_clump_tall` | REPLACE | 5 opaque variants |
| `woodland/wildflower_cluster` | NEW | 5 opaque variants |
| `woodland/mushroom_cluster` | NEW | 5 variants |
| `woodland/forest_floor_leaves` | NEW | 4 non-colliding opaque clusters |
| `woodland/forest_floor_twigs` | NEW | 4 non-colliding clusters |
| `farm/crop_wheat_row_2m` | NEW | Opaque row segment |
| `farm/crop_wheat_row_4m` | NEW | Opaque row segment |
| `farm/crop_vegetable_patch_2x2m` | NEW | 4 visual variants |
| `farm/crop_dead_patch_2x2m` | NEW | 2 visual variants |
| `farm/field_furrow_4x4m` | NEW | Non-colliding ground dressing |
| `farm/orchard_tree_young` | NEW | 3 silhouettes |
| `farm/orchard_tree_mature` | NEW | 3 silhouettes |
| `farm/scarecrow` | NEW | Landmark prop |

### K. Exterior Settlement, Yard, and Market Props

| Target family or asset ID | Status | Required variants |
|---|---|---|
| `settlement/fence_straight_2m` | REPLACE | Existing role, production pass |
| `settlement/fence_straight_4m` | REPLACE | Existing role, production pass |
| `settlement/fence_inner_corner` | NEW | Concave junction |
| `settlement/fence_outer_corner` | REPLACE | Existing corner role |
| `settlement/fence_end` | NEW | Finished end |
| `settlement/fence_broken` | NEW | 3 damage variants |
| `settlement/palisade_straight_2m` | NEW | Fortification module |
| `settlement/palisade_straight_4m` | NEW | Fortification module |
| `settlement/palisade_corner` | NEW | Fortification junction |
| `settlement/palisade_end` | NEW | Fortification end |
| `settlement/palisade_spike_cluster` | NEW | Non-colliding dressing |
| `settlement/well_open` | NEW | Simple well |
| `settlement/well_roofed` | REPLACE | Existing role, production pass |
| `settlement/handcart_empty` | REPLACE | Existing role, production pass |
| `settlement/handcart_loaded` | NEW | Composed load variant |
| `settlement/wagon_small` | NEW | Static small wagon |
| `settlement/wagon_large` | NEW | Static freight wagon |
| `settlement/signpost_single` | REPLACE | Existing role, production pass |
| `settlement/signpost_multi` | NEW | Multi-direction sign |
| `settlement/notice_board` | NEW | Settlement landmark |
| `settlement/woodpile_small` | REPLACE | Existing role |
| `settlement/woodpile_large` | NEW | Large resource pile |
| `settlement/trough` | REPLACE | Existing role |
| `settlement/hay_bale_rect` | REPLACE | Existing role |
| `settlement/hay_bale_round` | NEW | Round variant |
| `settlement/hay_stack` | NEW | Large resource pile |
| `settlement/market_stall_frame` | NEW | Socket receivers for canopy/sign/wares |
| `settlement/market_stall_canopy_opaque` | NEW | Opaque temporary canopy |
| `settlement/market_table` | NEW | Stall surface |
| `settlement/market_crate_display` | NEW | 4 composed variants |
| `settlement/market_basket_display` | NEW | 4 composed variants |
| `settlement/hitching_post` | NEW | Yard prop |
| `settlement/feeding_rack` | NEW | Farmyard prop |
| `settlement/clothesline_frame` | NEW | Static frame only |
| `settlement/clothesline_cloth` | GATED | Requires alpha/cloth expectations |
| `settlement/latrine` | NEW | Utility prop |
| `settlement/outhouse` | NEW | Small structure composition or asset |
| `settlement/shrine_roadside` | NEW | Landmark prop |
| `settlement/grave_marker_wood` | NEW | 4 variants |
| `settlement/grave_marker_stone` | NEW | 4 variants |
| `settlement/grave_open` | NEW | Terrain-aligned static dressing |

### L. Interior Architectural Trim

| Target asset ID | Status | Notes |
|---|---|---|
| `interior/trim/baseboard_straight_2m` | NEW | Wall base trim |
| `interior/trim/baseboard_inner_corner` | NEW | Concave corner |
| `interior/trim/baseboard_outer_corner` | NEW | Convex corner |
| `interior/trim/baseboard_end` | NEW | Finished end |
| `interior/trim/crown_straight_2m` | NEW | Ceiling trim |
| `interior/trim/crown_inner_corner` | NEW | Concave corner |
| `interior/trim/crown_outer_corner` | NEW | Convex corner |
| `interior/trim/crown_end` | NEW | Finished end |
| `interior/trim/wainscot_panel_1m` | NEW | Repeatable wall dressing |
| `interior/trim/wainscot_corner` | NEW | Junction |
| `interior/trim/floor_transition_1m` | NEW | Material boundary strip |
| `interior/trim/ceiling_beam_2m` | NEW | Interior beam |
| `interior/trim/ceiling_beam_4m` | NEW | Interior beam |
| `interior/trim/hearth_arch` | NEW | Fireplace insert |
| `interior/trim/hearth_mantel` | NEW | Decor receiver sockets |
| `interior/trim/chimney_breast` | NEW | Wall-aligned shell insert |

### M. Interior Furniture and Room Function Sets

Furniture should be authored by room function so the estate generator can
compose credible interiors rather than drawing from one undifferentiated prop
pile.

| Room set | Status | Required assets/variants |
|---|---|---|
| Common seating | REPLACE/NEW | chair ladderback x4, stool x3, bench short x2, bench long x2, armchair x2 |
| Common tables | REPLACE/NEW | side table x2, dining table small x2, dining table large x2, trestle table x2, round table x2 |
| Bedroom | REPLACE/NEW | single bed x3, double bed x3, cot x2, bedside table x2, wardrobe x2, dresser x3, washstand x2, privacy screen x2 |
| Storage | REPLACE/NEW | crate small/medium/large x3 each, barrel closed/open x3 each, sack single x4, sack pile x4, chest small/large x3 each, shelf x3, cabinet x3 |
| Kitchen | REPLACE/NEW | hearth x3, oven x2, prep table x2, cupboard x2, hanging rack x2, cauldron x3, bucket x3, water barrel x2, dish cluster x4 |
| Dining | NEW | place setting x4, serving tray x3, tankard cluster x4, bottle cluster x4, bowl cluster x4, candleholder x3 |
| Study/library | REPLACE/NEW | bookshelf short/tall x3 each, writing desk x3, lectern x2, book stack x5, scroll rack x2, map table x2 |
| Workshop | NEW | workbench x3, anvil x2, forge x2, tool rack x3, saw horse x2, grindstone x2, material bin x3 |
| Tavern | NEW | bar straight 2m/4m, bar corner, backbar x2, keg rack x2, tap barrel x2, booth x2, fireplace x2, game table x2 |
| Guard room | NEW | weapon rack x3, armor stand x2, bunk x2, footlocker x3, briefing table x2, wall shield x4 |
| Cell/dungeon | NEW | cell bed x2, shackles wall/floor, cage small/large, barred partition 2m, barred door states, torture table placeholder, drain grate |
| Temple/shrine | NEW | altar x3, pew short/long x2 each, offering table x2, brazier x3, reliquary x2, kneeling bench x2 |
| Lighting fixtures | REPLACE/NEW | lantern table/hook x3 each, candle single/triple x3, wall sconce x3, standing brazier x2, chandelier x3 |
| Rugs and hangings | GATED | Final versions require alpha/material support; opaque blockout variants may ship |

Every horizontal furniture surface that should accept decor needs a
`decor.surface` receiver. Stackable crates need compatible stack plug/receiver
sockets. Hanging lights need `decor.hook` plugs.

### N. Construction, Resource, and Settlement-Simulation Assets

These make the proposed town/outpost loop readable. Buildings remain
compositions; these assets visualize construction, production, storage, and
defense state.

| Target family | Status | Required states/variants |
|---|---|---|
| `simulation/construction/foundation_markers` | NEW | corner, edge, doorway, post socket |
| `simulation/construction/timber_stack` | NEW | small/medium/large |
| `simulation/construction/stone_stack` | NEW | small/medium/large |
| `simulation/construction/plank_stack` | NEW | small/medium/large |
| `simulation/construction/scaffold_set` | NEW | post, brace, platform, ladder, stair |
| `simulation/construction/crane_small` | NEW | Static construction landmark |
| `simulation/construction/building_stage_foundation` | COMPOSE | Recipe output plus marker assets |
| `simulation/construction/building_stage_frame` | COMPOSE | Recipe output plus scaffold assets |
| `simulation/construction/building_stage_shell` | COMPOSE | Recipe output plus openings/roof |
| `simulation/construction/building_stage_complete` | COMPOSE | Final recipe composition |
| `simulation/resource/firewood` | NEW | 3 quantity silhouettes |
| `simulation/resource/timber` | NEW | 3 quantity silhouettes |
| `simulation/resource/stone` | NEW | 3 quantity silhouettes |
| `simulation/resource/ore` | NEW | 3 quantity silhouettes |
| `simulation/resource/food_crates` | NEW | 3 quantity silhouettes |
| `simulation/resource/grain_sacks` | NEW | 3 quantity silhouettes |
| `simulation/resource/tools` | NEW | 3 quantity silhouettes |
| `simulation/production/sawmill_station` | NEW | Static station plus log/plank sockets |
| `simulation/production/stonecutter_station` | NEW | Static station plus stone sockets |
| `simulation/production/blacksmith_station` | NEW | Forge, anvil, quench, rack composition |
| `simulation/production/carpenter_station` | NEW | Bench, saw, timber composition |
| `simulation/production/kitchen_station` | NEW | Hearth, prep, storage composition |
| `simulation/production/farm_station` | NEW | Tool rack and seed storage composition |
| `simulation/production/storage_shed` | COMPOSE | Generated building plus storage assets |
| `simulation/defense/watchtower_small` | COMPOSE | Generated structure plus stair/rail assets |
| `simulation/defense/watchtower_large` | COMPOSE | Generated structure plus stair/rail assets |
| `simulation/defense/barricade_light` | NEW | Compound blocker |
| `simulation/defense/barricade_heavy` | NEW | Compound blocker |
| `simulation/defense/spike_barrier` | NEW | Compound blocker |
| `simulation/defense/arrow_slit_insert` | NEW | Opening insert |
| `simulation/defense/bell_alarm` | NEW | Static interactable prop |
| `simulation/defense/beacon_unlit` | NEW | Static state |
| `simulation/defense/beacon_lit` | GATED | Requires emissive/VFX support |
| `simulation/civic/town_board` | NEW | Notice/assignment landmark |
| `simulation/civic/market_marker` | NEW | Market center landmark |
| `simulation/civic/shrine_marker` | NEW | Settlement service landmark |
| `simulation/civic/objective_cache` | NEW | Readable objective container |
| `simulation/damage/rubble_small` | NEW | 5 variants |
| `simulation/damage/rubble_large` | NEW | 5 variants |
| `simulation/damage/burned_beam` | NEW | 4 variants |
| `simulation/damage/broken_wall_edge` | NEW | 4 variants |
| `simulation/damage/broken_roof_edge` | NEW | 4 variants |

### O. Gameplay Interactables and State Families

Static visual states can be authored now. Their behavior belongs to semantic
systems, not to mesh names.

| Semantic family | Status | Required static resources |
|---|---|---|
| Door | NEW/REPLACE | closed, open, broken; frame separate |
| Double door | NEW | both closed, left open, right open, both open, broken |
| Yard gate | NEW/REPLACE | closed, open, broken; frame separate |
| Portcullis | NEW | lowered, raised, broken; frame separate |
| Chest | NEW/REPLACE | closed, open, broken; small and large |
| Crate | NEW/REPLACE | intact, broken; small/medium/large |
| Barrel | NEW/REPLACE | closed, open, broken; normal and explosive visual family |
| Lever | NEW | up, down, broken; wall and floor mounts |
| Button | NEW | released, pressed; wall and pedestal mounts |
| Pressure plate | NEW | released, pressed; floor-aligned |
| Floor trap | NEW | hidden/covered, revealed, triggered, spent |
| Wall trap | NEW | idle, triggered, spent |
| Lock | NEW | intact, open, broken; door/chest compatible |
| Key | NEW | iron, brass, ornate, color-coded placeholder variants |
| Ladder | NEW | intact, broken, extended/retracted if applicable |
| Moving platform | NEW | platform, guide rail, end-stop, mechanism housing |
| Lift | NEW | platform, frame, crank, counterweight, stops |
| Drawbridge | NEW | bridge deck, hinge frame, raised/lowered static proofs |
| Destructible wall | NEW | intact, cracked, breached, rubble composition |
| Destructible barricade | NEW | intact, damaged, destroyed |
| Loot pickup | NEW | pouch, coin pile, resource bundle, quest-object placeholder |
| Objective marker object | NEW | cache, standard, altar, beacon, capture point |
| Spawn/patrol/cover markers | GENERATED | Editor-only semantic visualization, not Blender art |

Animation, transition timing, sounds, particles, and behavior graphs are
separate engine work. Static state assets should not imply those systems exist.

### P. Weapons, Equipment, Characters, and Creatures

Static world props can ship now. Equipped/skinned character content is gated.

| Target family | Status | Required variants |
|---|---|---|
| `equipment/weapon/sword` | NEW | short, arming, longsword, greatsword static props |
| `equipment/weapon/axe` | NEW | hand axe, battle axe, great axe static props |
| `equipment/weapon/mace` | NEW | club, mace, warhammer static props |
| `equipment/weapon/polearm` | NEW | spear, halberd, staff static props |
| `equipment/weapon/bow` | NEW | shortbow, longbow, crossbow static props |
| `equipment/shield` | NEW | round, heater, kite, tower static props |
| `equipment/quiver` | NEW | full and empty static props |
| `equipment/armor_display` | NEW | helmet x6, breastplate x4, gauntlet x3, boot x3 as props |
| `character/calibration/player` | KEEP | Current gauge until a static mannequin replaces it |
| `character/mannequin/player` | NEW | 1.8 m neutral static pose |
| `character/mannequin/guard` | NEW | 1.8 m readable guard static pose |
| `character/mannequin/civilian` | NEW | 3 body silhouettes, static poses |
| `character/mannequin/raider` | NEW | 3 body silhouettes, static poses |
| `character/mannequin/child` | NEW | Scale-only static reference, not gameplay-ready |
| Rigged player | GATED | Requires skinning, armature, animation, and graph support |
| Rigged NPC | GATED | Requires skinning, armature, animation, equipment slots, and graph support |
| Animals | GATED | Requires rigging/animation; static farm proofs optional |
| Monsters | GATED | Requires rigging, animation, hit volumes, and gameplay contracts |

BG3's character pipeline is relevant here as a future structural reference:
one body is not duplicated for every outfit and NPC. Body/race variants,
equipment slots, visual resources, material customization, and fallbacks are
composed. Iggy3d should not mass-produce final character meshes before it has
equivalent composition seams.

### Q. Materials, Surface Variation, Decals, VFX, and Lighting Assets

| Family | Status | Reason |
|---|---|---|
| Reusable base-color material palettes | NEW | Supported now; keep material count bounded |
| UV trim-sheet proofs | NEW | Supported through base-color textures |
| Stone/wood/plaster/metal/cloth base-color sets | NEW | Supported now |
| Normal maps | GATED | Imported contract is not rendered yet |
| Metallic-roughness maps | GATED | Imported contract is not rendered yet |
| Occlusion maps | GATED | Not evaluated yet |
| Emissive maps | GATED | Not evaluated yet |
| Alpha-cut foliage | GATED | Alpha material contract deferred |
| Transparent glass | GATED | Alpha blend/mask deferred |
| Dirt/moss/soot/wetness decals | GATED | Decal projection/rendering absent |
| Damage/blood/footprint decals | GATED | Decal and gameplay ownership absent |
| Fire/smoke/sparks/dust | GATED | Particle/VFX system absent |
| Water/foam/waterfall | GATED | Water and VFX systems absent |
| Light-emitting fixtures | GATED | Mesh can ship; authored light attachment contract does not exist |
| Cloth motion | GATED | Skinning/cloth/animation absent |

### R. Performance and Distance Variants

Do not ask artists to create final LOD chains until the runtime can select them.
The following are future requirements, not current Blender deliverables:

| Family | Status | Engine prerequisite |
|---|---|---|
| LOD0/LOD1/LOD2 mesh groups | GATED | LOD import and runtime selection |
| Vegetation impostors | GATED | Billboard/impostor renderer |
| Hierarchical cluster proxies | GATED | Clustered visibility/streaming |
| Mesh collision hulls | GATED | Convex/mesh collision cooker |
| Offline cooked mesh packages | GATED | Derived-data cache/cooker |
| Per-instance material variants | GATED | Instance material override contract |

Current production should still keep primitive counts, material counts, and
collision-part counts disciplined because repeated assets are instanced.

## Required Compositions and Proof Maps

These are not additional monolithic meshes. They prove the library closes into
useful authored spaces.

| Composition | Required proof |
|---|---|
| `proof/calibration_lab` | Scale, pivot, collision, walkability, sockets, materials |
| `proof/one_storey_house` | Generated shell plus openings, roof trim, stair/porch, interior set |
| `proof/two_storey_estate` | Exterior facade, per-floor partitions, stairs, roof, balconies |
| `proof/market_square` | Roads, stalls, carts, signs, civic marker, clutter |
| `proof/farmstead` | House, yard, fence/gate, field, well, storage, crops |
| `proof/guard_outpost` | Palisade, gate, watchtower, barracks, cover, alarm landmark |
| `proof/river_crossing` | Generated river bed/banks, ford, timber and stone bridge options |
| `proof/woodland_edge` | Trees, shrubs, ground dressing, rocks, path transition |
| `proof/cliff_cave_route` | Cliffs, climb route, cave mouth, tunnel, chamber, alternate path |
| `proof/settlement_growth` | Foundation, frame, shell, complete building states and resources |
| `proof/raid_damage` | Intact, damaged, breached, rubble, blocked/alternate traversal |
| `proof/stealth_encounter` | Cover heights, sight blockers, vertical route, objective prop |

Each proof map must be buildable through the same catalog, placement, socket,
recipe, save/load, and replacement paths available to the user.

## Production Order

### Batch 0: Contract Calibration

Create section A first. Nothing else is accepted until meter scale, ground,
pivots, compound collision, walkability, sockets, and current material support
are visible and repeatable.

### Batch 1: Building Closure

Create the high-value missing construction pieces:

- standard/wide/double door state families;
- standard/wide/tall window frames and shutters;
- full-storey straight and quarter-turn stairs with landings/rails;
- ridge/eave/gutter/chimney closure;
- balcony/railing/parapet modules;
- structural posts, beams, braces, arches, and foundation trim.

Acceptance composition: `proof/two_storey_estate`.

### Batch 2: Ground-to-Building Closure

Create roads, curb/threshold transitions, retaining junctions, bridge systems,
riverbank turns/ends, and generated-terrain transition rocks.

Acceptance compositions: `proof/market_square` and `proof/river_crossing`.

### Batch 3: Settlement Function

Create resource piles, construction markers/scaffolds, production stations,
palisades, barricades, watchtower components, civic landmarks, market props,
and damage states.

Acceptance compositions: `proof/settlement_growth`, `proof/guard_outpost`, and
`proof/raid_damage`.

### Batch 4: Interior Function

Finish furniture by room set, with surface/stack/hook sockets and explicit
state families for storage and interactables.

Acceptance compositions: furnished house, tavern, workshop, guard room, and
dungeon room.

### Batch 5: Natural Variation

Expand rocks, cliffs, caves, trees, shrubs, farm vegetation, wetland dressing,
and terrain seams. Variation is accepted by silhouette and tiling proof, not by
raw asset count.

Acceptance compositions: `proof/woodland_edge` and `proof/cliff_cave_route`.

### Batch 6: Gameplay Objects

Create static interactable states, weapons/equipment props, objective props,
and readable placeholder mannequins. Bind them only to existing semantic
systems; otherwise retain them as catalog assets.

### Batch 7: Engine-Gated Art

After the corresponding renderer/runtime milestones, produce final alpha
foliage, glass, water, decals, VFX, lights, LODs, rigged characters, equipment
composition, animals, and creatures.

## First Linux/Blender Work Order

The first box batch should be deliberately small enough to validate the entire
contract but large enough to expose bad assumptions:

1. `calibration/grid_1m_10x10`
2. `calibration/human_gauge_1p8m`
3. `calibration/door_clearance_0p9x2p1`
4. `calibration/storey_3m`
5. `calibration/pivot_hinge`
6. `calibration/socket_receiver`
7. `calibration/socket_plug`
8. `calibration/collision_compound`
9. `architecture/openings/door_frame_standard`
10. `architecture/openings/door_leaf_standard_closed`
11. `architecture/openings/door_leaf_standard_open`
12. `architecture/traversal/stair_straight_3m`
13. `architecture/traversal/stair_landing_2x2m`
14. `architecture/structural/railing_straight_2m`
15. `architecture/roof/ridge_cap_straight_4m`
16. `architecture/roof/ridge_cap_end`

The assembled proof is one 4 x 4 m bay with a 3 m storey, working door socket,
full-storey stair and landing, rail, and finished roof ridge. This batch answers
the most expensive questions before mass production:

- Is the actual engine meter scale correct?
- Are pivots and socket frames correct after Blender-to-glTF conversion?
- Does compound collision match visible stair geometry?
- Can the semantic building output and Blender trim align without offsets?
- Can an existing stable asset ID be replaced without breaking saved objects?

## Per-Asset Acceptance Record

Every completed row records:

```text
assetId:
family:
status replaced:
source blend:
runtime glb:
nominal bounds meters:
pivot contract:
forward/up axes:
materials/primitives/triangles:
collision mode:
collision parts:
walkable parts:
sockets:
state family:
compatible semantic recipes:
gallery proof:
assembled proof:
import result:
placement result:
save/load result:
replacement result:
known limitations:
```

## Completion Definition

The asset program is not complete because every row has a mesh. It is complete
when:

- all `NEW` and `REPLACE` rows required by the first product boundary have an
  accepted resource or an explicit scope ruling;
- all proof compositions can be authored through normal Creative workflows;
- generated geometry and Blender geometry align at meter scale without manual
  offsets;
- state families have semantic owners;
- no final asset depends on an unsupported renderer/runtime feature;
- collision, walkability, sockets, placement, save/load, and replacement are
  verified;
- placeholder assets remain available until their replacements pass those
  checks;
- art direction can replace visuals without forcing building, terrain,
  gameplay, or editor architecture to change.
