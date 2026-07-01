# ASCII Dungeon Authoring Reference (iggy3d)

> The complete, code-grounded spec for authoring iggy3d rooms as ASCII grids. Written for an AI author (and the human doing the vital 20% of fine-tuning). All facts are traced to `src/app/iggy3d/ascii_room/*` and verified against the source.

---

## 0. The One-Sentence Model

You author **one room per file/tape** as a rectangular grid of single-character glyphs. Each glyph maps through a fixed **26-entry table** (`kGlyphs`, `AsciiRoomGrid.cpp:11-70`) into a cell with collision flags, terrain/elevation, an optional marker tag, and an optional object proxy. A strict **5-stage compile cascade** turns the grid into geometry, collision surfaces, runtime entities, and (optionally) a live session. **It is ROOMS, not a connected dungeon** — there is no room-to-room linking anywhere.

Pipeline: `AsciiRoomSource (text parse) → AsciiRoomGrid (semantic cells) → AuthoredRoom (3D geometry) → RoomAsset (meshes + collision surfaces) → asset text / runtime Session`.

---

## 1. Glyph Table (the complete, closed vocabulary)

Source of truth: `kGlyphs` at `AsciiRoomGrid.cpp:11-70`. **Exactly 26 entries.** Any character not in this table (except `' '` inside layer mode) is a hard build failure (`ascii_room_unknown_glyph`). `kElevationStepMeters = 0.5` (`AsciiRoomGrid.cpp:9`).

Field order per entry (`AsciiRoomGlyphInfo`, `AsciiRoomGrid.hpp:40-53`): `glyph, kind, walkable, blocksActor, blocksProjectile, markerTag, terrainKind, elevationMeters, riseMeters, objectAssetId, objectSizeMeters(default {0.8,0.8,0.8}), traversalTag(default "")`.

| Glyph | Kind | Walkable | Blocks A/P | Terrain | Elev (m) | Rise (m) | Marker tag | Object / traversal |
|:-:|:--|:-:|:-:|:--|:-:|:-:|:--|:--|
| `#` | Wall | no | yes/yes | Flat | 0.0 | 0.0 | — | — |
| `J` | Wall | no | yes/yes | Flat | 0.0 | 0.0 | — | traversalTag `wall_jump` |
| `.` | Floor | yes | no/no | Flat | 0.0 | 0.0 | — | — |
| `' '` (space) | Floor | yes | no/no | Flat | 0.0 | 0.0 | — | **HOLE in layer mode** (see §3.3) |
| `0` | Floor | yes | no/no | Flat | 0.0 | 0.0 | — | — |
| `1` | Floor | yes | no/no | Flat | **0.5** | 0.0 | — | elevated_floor |
| `2` | Floor | yes | no/no | Flat | **1.0** | 0.0 | — | elevated_floor |
| `3` | Floor | yes | no/no | Flat | **1.5** | 0.0 | — | elevated_floor |
| `^` | Floor | yes | no/no | RampNorth | 0.25 | 0.5 | — | ramp up toward −Z |
| `v` | Floor | yes | no/no | RampSouth | 0.25 | 0.5 | — | ramp up toward +Z |
| `<` | Floor | yes | no/no | RampWest | 0.25 | 0.5 | — | ramp up toward −X |
| `>` | Floor | yes | no/no | RampEast | 0.25 | 0.5 | — | ramp up toward +X |
| `!` | Floor | yes | no/no | BlockedSteepEast | **0.5** | **1.0** | — | too-steep east slope (movement-blocked at runtime) |
| `+` | Door | yes | no/no | Flat | 0.0 | 0.0 | `door` | opens on interact |
| `s` | SecretDoor | yes | no/no | Flat | 0.0 | 0.0 | `secret_door` | key-gated (see §4) |
| `P` | PlayerSpawn | yes | no/no | Flat | 0.0 | 0.0 | `player_spawn` | **exactly one required** |
| `N` | NpcSpawn | yes | no/no | Flat | 0.0 | 0.0 | `npc_spawn` | hostile NPC |
| `M` | MonsterSpawn | yes | no/no | Flat | 0.0 | 0.0 | `monster_spawn` | **identical to N at runtime** |
| `$` | Treasure | yes | no/no | Flat | 0.0 | 0.0 | `treasure` | pickup + objective |
| `K` | Key | yes | no/no | Flat | 0.0 | 0.0 | `key` | pickup + objective |
| `T` | Trap | yes | no/no | Flat | 0.0 | 0.0 | `trap` | **inert marker at runtime** |
| `R` | ResetZone | yes | no/no | Flat | 0.0 | 0.0 | `reset_zone` | visual pad only; inert |
| `C` | Floor | yes | no/no | Flat | 0.0 | 0.0 | — | objectAssetId `wood_crate_proxy` (0.8³, blocks A+P) |
| `L` | Floor | yes | no/no | Flat | 0.0 | 0.0 | — | objectAssetId `movement_clamber_ledge_proxy`, size **{2.0,1.7,1.0}**, walkable top (`clamber`) |
| `E` | Exit | yes | no/no | Flat | 0.0 | 0.0 | `exit` | objective trigger (see §4) |
| `?` | Inspect | yes | no/no | Flat | 0.0 | 0.0 | `inspect` | **inert inspect-only marker** |

Notes:
- `J` and `C` both place a string in a positional slot but in **different fields**: `J` → `traversalTag="wall_jump"` (12th positional), `C` → `objectAssetId="wood_crate_proxy"` (10th positional). This is field-order aliasing; both are correct as written.
- Terrain enum (`AsciiRoomTerrainKind`, `AsciiRoomGrid.hpp:31-38`): `Flat, RampNorth, RampSouth, RampWest, RampEast, BlockedSteepEast`. `asciiRoomTerrainIsRamp()` returns true only for the four ramps — **not** `BlockedSteepEast` (`AsciiRoomGrid.cpp:115-120`).
- Cell kinds enum has 13 values but only **Floor / Wall / Object / Marker** materialize as authored primitives; Door/Spawn/Treasure/etc. are just markers on a walkable floor.

---

## 2. Coordinate & Dimension Model

**The key function:** `asciiRoomCellCenter` (`AsciiRoomGrid.cpp:285-298`):

```
world_x = (column - (width  - 1)/2) * tileSize
world_z = (row    - (height - 1)/2) * tileSize
world_y = elevation
```

Axis conventions (unusual — read carefully):

| ASCII direction | World axis |
|:--|:--|
| column increases (left → right) | **+X** (east) |
| row increases (top → bottom of text) | **+Z** (south) |
| row 0 (top line) | **most-negative Z** (north/far edge) |
| column 0 (first char) | **most-negative X** (west/left edge) |
| elevation | **+Y** (up) |

- **Centering:** `centerOnOrigin` defaults `true` (`AsciiRoomToAuthoredRoom.hpp`); the grid is centered on the origin. When `false`, top-left cell center sits at the origin (`x=col*tile`, `z=row*tile`).
- **Even width caveat:** centering uses `(dim-1)/2`, so an **even-width** room puts NO cell center on `x=0` (centers fall at half-tile offsets); odd width puts the middle column on `x=0`.
- `width` = length of the first row (after directives stripped); `height` = number of rows. Rows must be **rectangular** (all equal to first row width). Trailing spaces are significant columns.
- **Effective tile size** = `request.tileSizeMeters * source.tileScaleMeters`, applied **only** at the orchestration layer (`Authoring.cpp:25,45`). Default base tile = **1.0 m**.

Default geometry constants (`Authoring.hpp:19-23`): tile 1.0 m, floor thickness **0.10 m**, wall height **2.50 m**, wall thickness **1.0 m**, marker Y **0.05 m**.

---

## 3. Elevation / Multi-Height Model

Height comes from **two independent, additive sources** (`AsciiRoomGrid.cpp:181-183`):

```
cell.elevationMeters = glyph.elevationMeters + storyIndex * layerFloorSpacingMeters
```

### 3.1 Per-glyph intra-story steps (single layer)
The digit glyphs give **four discrete flat floor heights on ONE layer**, stored as **absolute meters** (level × 0.5 m), not abstract indices:
- `0`/`.`/`' '` → 0.0 m · `1` → 0.5 m · `2` → 1.0 m · `3` → 1.5 m.

There is **no flat glyph above 1.5 m** within a story — higher flat floors require stacking layers.

### 3.2 Ramps bridge one 0.5 m step
Ramps (`^ v < >`) sit at **0.25 m (step/2, the ramp midpoint)** with **riseMeters 0.5 m** — they span exactly one 0.5 m step, bridging a `0` floor to a `1` floor. `stored elevationMeters` is the **center**, not top or bottom.
- Corner heights (`terrainTopFacePoints`, `AsciiRoomToAuthoredRoom.cpp:129-178`): `lowY = elev − rise/2`, `highY = elev + rise/2`.
  - `^` RampNorth: NW/NE high, SW/SE low (up toward −Z).
  - `v` RampSouth: SW/SE high (up toward +Z).
  - `<` RampWest: NW/SW high (up toward −X).
  - `>` RampEast: NE/SE high (up toward +X, i.e. **rises west→east**).
- Corner order returned: **NW, NE, SE, SW**. `plane.front()` = **NW corner** (the plane's reference point for runtime height sampling).
- `!` BlockedSteepEast uses the **same corner layout as RampEast** but rise 1.0 m; geometrically identical to `>`, differs only in tags (`blocked_slope` vs `ramp`).
- Slope normal (`terrainNormal`, `:180-211`): `grade = rise/tileSize`; RampNorth `{0,1,grade}`, RampSouth `{0,1,−grade}`, RampWest `{grade,1,0}`, RampEast/Steep `{−grade,1,0}`, then normalized. Flat or `rise<=0` → `{0,1,0}`.

### 3.3 Per-story stacking (layers)
`floorN` directives (N≥1) create stacked stories; `storyIndex = N−1`. Every cell in a story is lifted by `storyIndex * layerFloorSpacingMeters` (default **4.0 m**). A `1` on `floor2` sits at `0.5 + 4.0 = 4.5 m`. **In layer mode, `' '` (space) is a HOLE — skipped, no cell emitted** (`AsciiRoomGrid.cpp:152-154`).

### 3.4 Floor mesh geometry
Each walkable cell → a thin floor box: **center Y = `elevationMeters − floorThickness/2`**, size `{tile, 0.10, tile}`. The floor's **top face sits exactly at `elevationMeters`** (the walkable surface Y). Do not read the box center as standing height.

Elevation tallies (`AsciiRoomGrid.cpp:200-214`, mutually exclusive if/else-if over walkable non-wall cells): `BlockedSteepEast → blockedSlopeCount`; ramp → `rampCount`; else `elevation>0 → elevatedFloorCount`. Ramps and steep tiles are **excluded** from `elevatedFloorCount` even though they're above 0 m.

---

## 4. Marker → Gameplay Semantics (at activation)

Markers never become entities directly. Flow: `glyph → cell.markerTag → AsciiRoomMarker.tag → RoomAnchorAsset.kind (anchorKindForMarkerTag) → ScenarioEntitySeed (entityFromAnchor) → live entity`. Gameplay is entirely determined by the anchor kind (`PackageSessionSeed.cpp`).

| Glyph | Anchor kind | Runtime entity | Behavior |
|:-:|:--|:--|:--|
| `P` | spawn | Player (Kind=Player, faction 1, HP 10/10) | **Required, exactly one.** Attack+Inspect targeting. |
| `N` / `M` | npc | Npc (faction 2, HP 3/3) | **N and M are identical at runtime.** Attack+Inspect, hostile combatant. |
| `$` | treasure | Pickup | AddItemToInventory; auto-objective `collect_<name>` (InventoryContains). |
| `K` | key | Pickup | Same as treasure; also gates secret doors. |
| `+` | door | Door | OpenDoor on interact, EmitEventOnly. No item required. |
| `s` | secret_door | Door | Requires the **first `K` in room order** (`required_item_missing` if absent from inventory). If no `K` exists, opens like `+`. |
| `E` | exit | Marker (ObjectiveTrigger) | CompleteObjective; auto-objective `exit_<name>`. If any `$` exists, requires the **first `$` treasure item**. Completing sets `SessionOutcome::Victory`. |
| `T` | trap | Marker | **Inert.** Inspect-only. No damage. |
| `R` | reset_zone | Marker (+ visual pad mesh) | **Inert.** No respawn/reset behavior. |
| `?` | inspect | Marker | **Inert.** Inspect-only. |

Additional build-time output:
- `+`/`s` → full-height door panel mesh + door blocker box (blocks actor AND projectile).
- `R` → thin reset-zone pad mesh, **no collision**.
- `T`/`?` → anchor only, no mesh, no surface.
- `C` → mesh + actor/projectile blocker box (rests on floor, center Y = `elev + size.y/2`).
- `L` → mesh + walkable-top surface at `y = elev + 1.7/2` tagged `clamber`, plus a movement traversal slot.

Gating uses **only the first** key/treasure in room-iteration order — every secret door shares `firstKeyItemId`, every exit shares `firstTreasureItemId`. There is no per-door gating.

---

## 5. Input Formats (EXACT)

### 5.1 Automation tape (`ascii_room.*` keys) — the AI author's primary surface

Registered keys (`Automation.cpp:451-460`, all `Category::AsciiRoom`, owner `frontend`):

| Key | Value type | Purpose |
|:--|:--|:--|
| `ascii_room.text` | Raw | The grid, inline. Rows separated by the **two-char escape `\n`**. |
| `ascii_room.room_id` | String | Stable room id (default `ascii_room`). Empty → `invalid_value`. |
| `ascii_room.source_name` | String | Receipt **label only** (default `inline_ascii_room`). Does NOT read a file on this path. |
| `ascii_room.build` | Bool | `true` → compile a **preview only** (no session). `false` → ignored. |
| `ascii_room.activate` | Bool | `true` → compile AND start a runtime session on the gameplay screen. |

**Row-separator encoding — CRITICAL.** The grid value is one physical line. Rows are separated by the two literal characters **backslash + `n`** (`\n`), NOT a real newline. A real newline in the tape ends the command. Decoder `decodeProductAsciiRoomAutomationText` (`Preview.cpp:15-37`): `\n`→LF, `\t`→TAB, `\\`→backslash; unknown escapes keep the backslash + char.

Author your tape with **physical newlines between commands** and **`\n` escapes inside the grid value**. In a raw C-string tape the grid backslashes are often doubled (`\\n`) because the tape file itself is a quoted string.

Receipt fields — **BUILD path** (`ascii_room_preview_*`): `status, reason_code, failed_stage, room_id, source_name, ready, width, height, floor_count, wall_count, object_count, marker_count, elevated_floor_count, ramp_count, blocked_slope_count, static_mesh_count, anchor_count, spatial_surface_count, asset_text_written, asset_text_bytes`.

Receipt fields — **ACTIVATE path** (`ascii_room_activation_*`): `status, reason_code, room_id, package_id (=iggy3d.ascii_room_authoring), scenario_id, session_created, player_spawned, player_count, entity_count, npc_count, pickup_count, door_count, marker_entity_count, objective_count, wall_count, marker_count, runtime_hash` + `active_room_*`/`gameplay_*`. **Reading the wrong prefix returns stale/empty fields.**

Post-activation gameplay can be driven in the same tape via `game.move_x`, `game.move_y`, `game.player_position=<x,y,z>`, `game.interact`.

**Separate key (NOT the inline path):** `world.ascii_room_file=<path>` reads a file from disk (`std::ifstream`, path relative to process CWD, no base dir) — but **only on the New-World screen** (`childScreen==NewWorld`, else `owner_unavailable`). Empty/unreadable → `invalid_value`.

### 5.2 `.iggyroom.txt` source file

The engine-owned source extension. It carries **essentially no metadata header** — just:
1. Optional first line **scale directive** `*<positive-float>` (e.g. `*5`) → multiplies tile size. Must be positive & finite (`ascii_room_invalid_scale` otherwise). Recognized **only on the first row**.
2. Optional **layer directives** `floorN` (N≥1 integer) → stacked stories. First content before any `floorN` in layered intent aborts layered parsing.
3. The rectangular grid.

Room id, units, materials, and mesh ids all live **downstream** in the generated `.room.iggy3d.toml`, not in the ASCII. Fixtures live under `fixtures/rooms/ascii/` (e.g. `training_room.iggyroom.txt`, `slope_gym.iggyroom.txt`, `layered_jump_gym.iggyroom.txt`, `loop_keep.iggyroom.txt`).

Newline handling (`AsciiRoomSource.cpp`): CRLF and lone CR normalized to LF; exactly one trailing empty row is dropped (a trailing newline does NOT create a phantom row). A trailing blank line **with spaces** IS a real row and can trigger ragged/empty errors.

### 5.3 `.room.iggy3d.toml` (generated asset — not authored by hand)

Emitted by the exporter (`AsciiRoomAssetText.cpp`); carries `room.id`, version, units, `source='iggy3d.ascii_room'`, `source_file` (path back to the `.iggyroom.txt`), `source_subset='ascii_room_authoring'`, a `[conversion] feet_to_meters` table, and generated `[[static_meshes]]`/`[[spatial_surfaces]]`/`[[anchors]]`. **All meters are converted to feet** (`feetToMeters = 0.3048`); positions/sizes use `*_ft` keys, normals stay unit vectors. Rejects empty required strings and strings containing `"`, `\`, or newline. No `[[openings]]` are emitted from the ASCII path.

### 5.4 `package.iggy3d.toml` (manifest — one room)

`makeProductAsciiRoomPackage` (`Package.cpp:26`) does exactly one `package.rooms.push_back(room)`. Schema: `[package] id/schema_version/required_runtime_schema/scenario` + `[[assets]] {id,path}` referencing **exactly one room**, one meshes lib, one materials lib.
- Default `packageId = "iggy3d.ascii_room_authoring"` (hardcoded; **not authorable from the grid**).
- `scenarioId = "<roomId>.runtime_loop"`, or `"ascii_room_preview.runtime_loop"` when roomId is empty (`Package.cpp:5-10`).
- **Only `package.rooms.front()` is ever used at runtime** (`PackageSessionSeed.cpp:309`, `Operations.cpp:112`). Extra `.room.*` assets are silently ignored.

---

## 6. Validation & Error Reason Codes

The compile is a strict **5-stage cascade** (`buildProductAsciiRoomAuthoring`, `Authoring.cpp:84-155`); any failure short-circuits and sets `failedStage` + `status` + `reasonCode`. Success status = `product_ascii_room_ready`; success reason = `ascii_room_ok` at the source/grid stages.

| Stage | `failedStage` | Reason code | Trigger |
|:--|:--|:--|:--|
| source | `source` | `ascii_room_empty` | empty / all-blank input |
| source | `source` | `ascii_room_ragged_rows` | rows unequal width (single-layer) |
| source | `source` | `ascii_room_unknown_glyph` | glyph not in table (diagnostic carries row/col/glyph) |
| source | `source` | `ascii_room_invalid_scale` | `*` directive not positive-finite (e.g. `*0`) |
| source | `source` | `ascii_room_empty_layer` | a `floorN` layer has no rows |
| source | `source` | `ascii_room_layer_size_mismatch` | layers differ in width OR height |
| grid | `grid` | `ascii_room_no_floor` | no walkable cell (all-wall) |
| grid | `grid` | `ascii_room_missing_player_spawn` | zero `P` |
| grid | `grid` | `ascii_room_multiple_player_spawns` | more than one `P` |
| grid | `grid` | `ascii_room_unknown_glyph` | redundant guard |
| asset_text | `asset_text` | `ascii_room_asset_text_invalid_conversion` | `feetToMeters == 0`, empty required field, or unsafe string char |

Seed/activation extras: `product_package_seed_missing_spawn_anchor` (no spawn anchor); activation success `ascii_room_activated`. Saved-room binding: `saved_marker_bind_applied` / `_noop` / `_no_markers` / `_player_missing` / `_invalid_combatant` / `_replace_failed`.

**Hard invariants (memorize):**
1. **Exactly one `P`.** No default spawn. This is the single most common failure.
2. **≥1 walkable cell.** Walls `#`/`J` are the only non-walkable glyphs.
3. Only the 26 glyphs (plus `' '` as a hole in layer mode).
4. Rectangular rows; layered mode requires uniform width AND height across all layers.
5. `*` scale must be positive & finite.

---

## 7. Capabilities vs. Limits

**Can do:**
- Single-layer terraced floors at four heights (`0`/`1`/`2`/`3`).
- Directional ramps (`^ v < >`) bridging one 0.5 m step; deliberately-blocked steep east slope (`!`).
- Multi-story stacking with `floorN`, 4.0 m default spacing, `' '` holes.
- Global tile scale via `*N`.
- Gameplay markers, doors (open + key-gated secret), pickups (`$`/`K`), exit objective, NPCs.
- Fetch-then-progress loops: `K`+`s` (key unlocks secret door), `$`+`E` (treasure gates exit).
- Object proxies: crate `C` (blocker), clamber ledge `L` (walkable top).
- Wall-jump surface `J`.
- Author inline (automation) or from disk (New-World `world.ascii_room_file`), then optionally hand off to the interactive editor (see §9).

**Cannot do:**
- No room-to-room connectivity. `E`/`+` are in-room anchors only; no portal/target-room field exists in `RoomAsset`. "Dungeon" here = a **catalog of single-room files** (`BuiltinDungeon.cpp`) or one painted grid (`DungeonDraft.cpp`), never a graph.
- No flat floor above 1.5 m within a story (must stack layers).
- Ramp slope is baked into the glyph (0.25 m elev / 0.5 m rise) — not tunable from ASCII.
- `packageId` is fixed (`iggy3d.ascii_room_authoring`); only `scenarioId` (from roomId) distinguishes rooms.
- One semantic per glyph: a cell is exactly one of {floor, wall, object, marker}. You cannot put a marker and an object on the same tile.
- The interactive editor exposes **only Floor/Wall/Object** tools — ramps, elevation, markers, doors are **ASCII-only**.

---

## 8. Proven by Tests vs. Unproven

**Proven end-to-end** (unit + fixture + product smokes booting a headless session):
- Canonical flat room, exact floor/wall/marker counts and geometry (positions, sizes, normals, surface-point counts).
- Terrain: elevation `1`/`2`/`3`, ramps `^v<>`, blocked-steep `!` (accepted moderate ramp **26.565° × 0.75 speed**; rejected steep **45° → `slope_rejected`, speed 0**).
- Objects `C`/`L`, wall-jump `J`, doors `+`, secret doors `s` (key-gated → `required_item_missing`).
- Multi-story `floor1`/`floor2` (4 m spacing, `' '` = hole), `*` scale directive.
- Marker binding to entities (npc/treasure/door/exit/key), collision surface roles (walkable / actor-blocker / projectile-blocker are **separate surfaces with disjoint masks**), door open/close collision filtering, NPC combat.
- Built-in presets via map smoke: `physics_flat_room`, `slope_gym` (20 elevated + 24 ramps), `layered_jump_gym` (178 elevated), `movement_gym`, `object_crate_room`, `large_flat_room`.
- Asset-text roundtrip and **byte-identical fixture parity** (`training_room.room.iggy3d.toml` must match exporter output — regenerate if the compiler changes).
- Exact Vulkan render counts (floors batched into **ONE draw call**, not one per tile).

**Unproven / weakly proven:**
- `T` (Trap), `M` (Monster), `?` (Inspect): only glyph-mapping/tag coverage. **No runtime test** proves a trap fires/damages, a monster differs from an NPC, or inspect triggers anything. Treat their gameplay as unproven — and per §4, `T`/`?`/`R` are in fact **inert** at the session-seed level.
- `R` and `L` are omitted from the `glyphMappingsMatchContract` array (covered instead in room-asset compile tests).
- Genuine multi-room dungeons: none. The `loop_keep` "multi-room" fixture is one 17×7 grid with interior wall dividers, not linked rooms.

---

## 9. Interactive Editor & Source-of-Truth Handoff

- Branch A (**authored/bake/export**): `compileAsciiRoomToAuthoredRoom` → `SaveAuthoredRoomSection` → `RoomAsset` → asset text.
- Branch B (**editable/interactive, the human 20%**): `buildEditableRoomFromAsciiRoom` compiles to the authored room **first**, then copies field-by-field into an `EditableRoomDocument`. So the editable doc inherits every quirk of the authored compile; it never sees raw grid cells.
- **There is NO editable/authored → ASCII serializer** (grep-confirmed). ASCII is a **write-only input language**. Once the editable document exists it is the **sole source of truth**; interactive edits **cannot be reflected back into the ASCII text** — the two representations diverge permanently.
- Editor: 16 `RoomEditCommandKind`s (Add/Delete/Move/Resize floor; Add/Delete/Move/Stretch/Rotate90/SetHeight/SetThickness/SetSemantics wall; Add/Delete/Move object; SetFloorSemantics), full undo/redo, transactional re-bake (a command that bakes/projects invalidly is rejected even if the edit validated). Only 3 place tools: Floor/Wall/Object. Cursor `Up`=−Z, `Down`=+Z. Author identity is tracked (`Ai`/`Hotkey`/`Mouse`/`Script`).
- **MovementTestLab objects** are injected into the authored room only on Branch A (`Authoring.cpp`), not Branch B — the editable doc can differ from the baked one for the same request.

---

## 10. Gotchas & Seams

1. **RAMP-TILE-CENTER "airborne" collision seam (confirmed).** At (near) the exact center-X of a RampEast `>` tile, a movement ground sample can read the **flat/contour band instead of the slope**. Ground normal is sampled from whichever walkable surface's XZ-AABB contains the point at the highest height (plane equation, `planePoint = NW corner`); at the ramp's own center the plane height equals the tile's mid elevation, so a tiny/zero horizontal step yields `|dy|<=0.001 m` → travelDirection **Contour**, registering as flat/airborne rather than uphill/downhill. **Slope facts are only reliable once the destination XZ moves DEEP into the ramp tile.** Seed the player onto the ramp with `game.player_position` before moving.
2. **Row separator is `\n` (backslash+n), not a real newline.** A grid pasted with real newlines silently truncates to its first row (the rest parse as separate/unknown commands).
3. **Automation `game.move_x=1` is a ~0.055 m velocity step, NOT a 1 m tile jump.** It cannot cross a 1 m gap to an adjacent ramp/steep tile; repeated moves overshoot to room center and the last no-ops. Teleport with `game.player_position` first, then a single move.
4. **`source_name` does NOT read a file** on the `ascii_room.*` path — the grid always comes from `ascii_room.text`. Only `world.ascii_room_file` reads disk (relative to CWD, no base dir → a wrong CWD fails as `invalid_value`, not "file not found").
5. **`!` is `walkable=true` at the glyph level** (emits a floor) but its 45° slope is **rejected by movement at runtime** (`slope_rejected`, speed 0). Authors expecting to walk up it will be blocked.
6. **`' '` (space) is overloaded:** a walkable flat floor in single-layer mode, a HOLE (no cell) in `floorN` layer mode. Same character, opposite meaning.
7. **`N` and `M` are gameplay-identical** (both → anchor kind `npc`). The distinction exists only in the glyph.
8. **`T`, `R`, `?` are inert** at activation (inspect-only markers). No trap damage, no reset, no special inspect behavior is wired.
9. **`E` and `+` do NOT connect rooms** — in-room anchors only. No target-room wiring exists anywhere.
10. **Final elevation is additive:** `glyph.elev + storyIndex*4.0 m`. A `1` on `floor2` is 4.5 m, not 0.5 m.
11. **Floor box center ≠ standing height:** center Y = `elev − 0.05` (half the 0.10 m thickness); the walkable top is at `elev`.
12. **Ramp `elevationMeters` is the center** (0.25 m), not top or bottom; `riseMeters` (0.5 m) is the span.
13. **`elevatedFloorCount` excludes ramps and steep tiles** (if/else-if chain), even though they're above 0 m.
14. **Even-width rooms have no cell on x=0** (half-tile centering offset).
15. **Editor default wall** is thickness 1.0 / height 2.5 (Cursor place), NOT the `EditableRoomWall` struct defaults (0.20 / 2.0).
16. **`training_room.room.iggy3d.toml` is byte-locked** to exporter output — any geometry-affecting compiler change breaks the parity test and must be regenerated.
17. **Doors leave a collision opening:** `+`/`s` produce a walkable FLOOR + marker (not a wall), so a door cell is passable geometry; the door blocker is a separate filtered surface.
18. **A leading `*` scale is recognized only on the first decoded row.** If any content precedes it, it becomes an unknown/normal glyph row.

---

## 11. Copy-Pasteable Examples

### 11.1 Minimal valid room (grid form)
```
#####
#.P.#
#####
```
5 wide × 3 tall, centered on origin. `P` at column 2, row 1 → world `(0, 0.05, 0)`. Satisfies: exactly one `P`, ≥1 walkable cell, rectangular.

### 11.2 Minimal automation tape (compile-only)
Physical newlines separate the 4 commands; `\n` inside the grid value separates rows:
```
ascii_room.room_id=my_room
ascii_room.source_name=my_room.iggyroom.txt
ascii_room.text=#####\n#.P.#\n#####\n
ascii_room.build=true
```
Yields `product_ascii_room_ready`, width 5, height 3, one player spawn. Swap the last line to `ascii_room.activate=true` to compile AND start a live session on the gameplay screen. (In a raw quoted-string tape, the in-grid `\n` are typically written `\\n`.)

### 11.3 Canonical 7×5 training room (proven everywhere)
```
#######
#P..N.#
#.+.$.#
#..E..#
#######
```
width 7, height 5, floor 15, wall 20, marker 5. Activation → player + NPC + treasure pickup + door + exit objective.

### 11.4 Richer multi-feature room (elevation + ramp + steep + objects + gameplay loop)
```
*2
#########
#P..C..E#
#.>>>.K.#
#..!.1s$#
#########
```
- `*2` → 2 m tiles (must be the first decoded row).
- `P` spawn, `C` crate (blocker), `E` exit (gated by the `$` treasure).
- `>>>` a 3-tile east ramp; `1` a 0.5 m elevated floor; `!` a blocked steep tile.
- `K` key → gates the `s` secret door.

As an automation `ascii_room.text` value (rows joined with `\n`, `*2` first):
```
ascii_room.text=*2\n#########\n#P..C..E#\n#.>>>.K.#\n#..!.1s$#\n#########\n
```

### 11.5 Multi-story room (layer directives)
```
floor1
#P..#
#...#
floor2
#..E#
#.  #
```
Story 2 lifted 4.0 m; `' '` on the upper layer is a hole. `E` on floor2 is an in-room marker (NOT a portal). All layers must share width AND height.

### 11.6 Ramp-move seed (to avoid the center seam)
Room `#######` / `#P>..$#` / `#######`; then `game.player_position=-1.3,0,0` (seed onto the ramp) + `game.move_x=1` → band `moderate`, angle 26.565°, speed 0.750, uphill.

