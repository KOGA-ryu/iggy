# Stealth Blockout Asset Kit

Placeholder/starter kit for large-map stealth testing. Blockout-grade,
dimensioned to the engine's real movement/perception constants (table below).
Regenerate everything headlessly with:

```
cd assets/creative/stealth_blockout/tools
blender --background --python generate_kit.py      # rebuild all .glb
blender --background --python render_previews.py   # previews -> ~/stealth_blockout_review/
```

## Measured metrics (extracted from code — the kit's single source of truth)

| Metric | Value | Source |
|---|---|---|
| Player capsule height | 1.80 m | `src/runtime/movement/MovementParams.hpp:8` |
| Player capsule radius | 0.30 m | `src/runtime/movement/MovementParams.hpp:7` |
| Auto-step height | 0.35 m | `src/runtime/movement/MovementParams.hpp:10` |
| Max walkable slope | 40° | `src/runtime/movement/MovementParams.hpp:9` |
| Ground snap distance | 0.60 m | `src/runtime/movement/MovementParams.hpp:11` |
| Player max speed | 3.0 m/s | `src/runtime/movement/MovementParams.hpp:6` |
| Editor fly-camera eye | 1.7 m | `src/app/iggy3d/creative/render/CreativeSceneFrame.cpp:67` |
| Reasoning occlusion eye | 1.6 m | `src/runtime/ai/ReasoningGraph.hpp:66` |
| Occlusion margin | 0.05 m | `src/runtime/ai/ReasoningGraph.hpp:67` |
| Guard eye height | 1.6 m | `src/runtime/ai/NpcBehaviorProfile.hpp:25` |
| Target stand eye | 1.6 m | `src/runtime/ai/NpcBehaviorProfile.hpp:26` |
| Target sneak eye | 0.9 m | `src/runtime/ai/NpcBehaviorProfile.hpp:27` |
| Guard perception radius | 6.0 m | `src/runtime/ai/NpcBehaviorProfile.hpp:19` |
| Guard vision half-angle (horiz) | 60° | `src/runtime/ai/NpcBehaviorProfile.hpp:23` |
| Guard vision half-angle (vert) | 30° | `src/runtime/ai/NpcBehaviorProfile.hpp:24` |
| Guard attack range | 1.5 m | `src/runtime/ai/NpcBehaviorProfile.hpp:21` |
| Guard chase stop distance | 1.25 m | `src/runtime/ai/NpcBehaviorProfile.hpp:20` |
| Snap increments | 0.25 / 0.5 / 1 / 2 m | `src/app/iggy3d/creative/tools/Tools.hpp:91-96`, values in `ToolSettingLabels.cpp:36-44` |
| Grid cell / place pitch | 1.0 m | `src/app/iggy3d/map_maker/Grid.hpp:20`, `apps/iggy3d_creative/EditorBootstrap.cpp:144` |
| Crouch height | **not in code** — only `bool crouched` (`src/runtime/player/PlayerMotor.hpp:73`); sneak eye 0.9 m (`clamber` motor untouched by crouch v1) is the working proxy | — |
| Clamber band | (0.35, 1.80] m — bottom exclusive (`clamberBandBottomMeters`), top inclusive (`clamberBandTopMeters`); reach ≤ 0.55 m (`clamberMaxReachMeters`), 8-tick phase, 30 dB engage noise | `src/runtime/movement/MovementParams.hpp` clamber block |

**Clamber is now code truth:** the clamber motor (flow feat v1) reads the
band from `MovementParams` — band top = player height (1.80 m), so the
ladder's 1.8 block engages and the 2.00 m fail block refuses with
`clamber_top_above_band`. The ladder heights are pinned by
`tests/unit/clamber_motor_tests.cpp`.

## The kit (26 pieces, base at y=0, XZ centered, one flat color per family)

### Cover — blue
| Asset | Dims (W×D×H m) | Note |
|---|---|---|
| `cover_low_wall_2x1p1` | 2.0×0.4×1.1 | above sneak eye 0.9+0.05 margin, below stand/guard eye 1.6 |
| `cover_high_wall_2x2` | 2.0×0.4×2.0 | above player 1.80 |
| `cover_crate_1m` | 1.0×1.0×1.0 | one grid cell |
| `cover_crate_0p5` | 0.5×0.5×0.5 | half snap |
| `cover_barrel_0p6x1p1` | Ø0.6×1.1 | 16-side cylinder |
| `cover_sandbag_run_2x0p9` | 2.0×0.6×0.9 | crest exactly at sneak eye — peek-over cover |

### Traversal test ladder — orange
| Asset | Dims | Note |
|---|---|---|
| `clamber_block_0p6` / `_1p0` / `_1p4` / `_1p8` | 1×1×H, walkable tops | ladder rungs; 1.8 = assumed band top |
| `clamber_fail_2p0` | 1×1×2.0 | band+0.2 — motor MUST refuse |
| `vault_rail_2x1p0` | 2.0×0.15×1.0 | |
| `stair_run_2x3x1p5` | 2 W, 6× riser 0.25 (one snap step, < auto-step 0.35), tread 0.5 | walkable compound treads |
| `ramp_2x2x1` | 2×2 run×1 rise (26.6° < 40° limit) | 8 fine treads, walkable |
| `ledge_shelf_1x1p8` | slab top at 1.8, 0.2 m overhang | hang/clamber target |
| `platform_2x2x0p5` | 2×2×0.5 | walkable |

### Structure — grey
| Asset | Dims | Note |
|---|---|---|
| `wall_seg_2x2` / `wall_seg_4x2` | 2 or 4 ×0.2×2.0 | |
| `wall_window_2x2` | 2×0.2×2.0, slot 1.4–1.9 | slot spans stand/guard/editor eye band (1.6/1.7); sneak eye 0.9 occluded — LOS play |
| `doorway_2x1` | 1.0×2.0 opening, 2.0×0.2×2.5 overall | player 1.80 clears |
| `pillar_0p5x3` | 0.5×0.5×3.0 | |
| `corner_l_2x2` | two 2 m legs, 0.2 thick, 2.0 h | compound so the inner corner stays open |

### Reference — green
| Asset | Dims | Note |
|---|---|---|
| `guard_post_1p8` | 0.4×0.4×1.80 body, eye fin at 1.6 | guard HEIGHT is not a code constant; body borrows player height, the 1.6 eye line is the verified datum |
| `player_gauge_1p8` | 0.3×0.3×1.80, band plate at 0.9 | crouch/sneak-eye band |

### Giant scale (liminal) — orange
| Asset | Dims | Note |
|---|---|---|
| `giant_stair_4x6x3` | 2× stair: riser 0.5, tread 1.0, 4 W | riser > auto-step 0.35 — each step is a climb |
| `giant_shelf_2x3p6` | 2× ledge shelf, top at 3.6 | |

## Adjustments from the original plan
- Clamber band unverified (see above) — ladder kept at 0.6/1.0/1.4/1.8 + fail 2.0
  but flagged as assumption, not measurement.
- `sandbag_run` kept at 0.9 h: with the 0.05 occlusion margin the sneak eye ray
  sits at 0.95, so it reads as peek-over concealment, not hard cover.
- `wall_window` slot set to 1.4–1.9 (not mid-wall) so it brackets the measured
  1.6/1.7 eye band while hiding the 0.9 sneak eye.
- `ramp` widened to 2×2 (from 2×1 run) and built as stepped treads because
  engine collision is AABB per part — a true sloped box would collide as its
  full bounding box (`cave/ramp_4x4x1p5.glb` uses the same tread pattern).
- `stair_run` riser 0.25 = QuarterMeter snap per order; total 6 risers → 1.5 h.
- `guard_post` body height assumed 1.80 (player height) — no guard body height
  constant exists; only the 1.6 m eye is measured.

## Conventions (matched to existing kit)
- glTF 2.0 `.glb`, Blender 4.5 exporter, one material per asset, flat-shaded.
- Single-box pieces: node extras `iggy_category: "stealth_blockout"`,
  `iggy_collision: "bounds"`, plus `iggy_walkable: true` on walkable tops
  (`walkway_stone_01.glb` pattern).
- Multi-box pieces: every node carries `iggy_category` +
  `iggy_collision: "compound_bounds"`; collidable nodes add
  `iggy_collision_part: "bounds"` + `iggy_collision_part_walkable`
  (`cave/ramp_4x4x1p5.glb`, `infrastructure/retaining_wall_2x1p2.glb` pattern).
