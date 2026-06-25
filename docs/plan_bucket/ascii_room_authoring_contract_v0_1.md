# ASCII Room Authoring Contract v0.1

## Objective

Define a durable ASCII-to-3D room authoring pipeline for Iggy3D.

The goal is to let a user write a compact ASCII room, validate it, and compile
it into Iggy3D-owned semantic room data that can later become a playable 3D
room with walls, floors, doors, NPCs, treasure, traps, exits, and other tagged
markers.

This is not renderer work first. This is authoring and geometry compilation
first. The renderer should consume generated room data later.

Target pipeline:

```text
ASCII room source
-> parsed source document
-> semantic grid
-> generated authored room
-> deterministic proof output
-> runtime package/fixture
-> renderer/backend consumption later
```

This contract started as a plan-only document. The current repo now implements
the parser/model/compiler path and the first product runtime binding described
below.

## Scout Inputs

Two EDI-like repos were scouted read-only before this contract:

```text
/Users/kogaryu/edi-dungeon-map
/Users/kogaryu/edi-blender-lab
```

Useful lessons from `/Users/kogaryu/edi-dungeon-map`:

- raw ASCII can be parsed into a semantic grid before geometry generation;
- raw glyphs should be preserved for proof and roundtrip display;
- glyph vocabulary should be data-owned and testable;
- row 0 is north/top, column increases east/right;
- parser and geometry generation should stay separate;
- wall runs can be compressed deterministically;
- marker glyphs should become neutral tagged semantics first, not runtime
  behavior immediately.

Useful lessons from `/Users/kogaryu/edi-blender-lab`:

```text
Recipe is truth.
ASCII preview is proof.
Blender script is execution.
```

Iggy3D should apply the same discipline:

```text
ASCII room source is truth.
Semantic grid is validated meaning.
Generated authored room is Iggy3D game data.
ASCII/primitive proof is inspection output.
Renderer execution is downstream.
```

## Source Truth And Proof Tiers

The ASCII source is authoring truth for this pipeline. Generated files and
rendered views are derived proof until a later editor makes changes and writes a
new source document.

Tiers:

```text
Tier 0: ASCII source text
Tier 1: parsed source document
Tier 2: semantic grid
Tier 3: generated authored room data
Tier 4: deterministic proof text / receipt fields
Tier 5: runtime package / fixture
Tier 6: renderer display
```

Rules:

- each tier must be rebuildable from the tier before it;
- failures must point back to source line/column when possible;
- renderer output must not become source truth;
- generated geometry must carry source references for debugging;
- no JSON;
- no AppShell compilation.

## Source File Extension

Engine-owned ASCII room sources should use:

```text
.iggyroom.txt
```

Examples:

```text
fixtures/rooms/ascii/training_room.iggyroom.txt
fixtures/rooms/ascii/treasure_room.iggyroom.txt
```

Why:

- `.txt` keeps the source easy to open in any editor;
- `.iggyroom` marks the file as an Iggy3D-authored room source;
- the extension does not imply runtime package truth;
- future EDI/gameguy exports can target the same extension without importing
  their repo code.

## Current Iggy3D Fit

Existing Iggy3D targets this work must inspect and compile into later:

```text
src/runtime/save/SaveEnvelope.hpp
src/runtime/save/SaveFileStore.hpp
src/app/iggy3d/ProductWorldCreation.*
src/app/iggy3d/SaveBridge.*
src/app/iggy3d/ProductSaveCatalog.*
src/app/frontend/SaveBrowser.*
```

Current authored-room output types:

```text
SaveAuthoredRoomSection
SaveAuthoredRoomFloorRecord
SaveAuthoredRoomWallRecord
SaveAuthoredRoomSemanticsRecord
```

Important boundary:

- ASCII room authoring should produce authored room data;
- runtime save/load remains the durable gameplay truth once the generated room
  is saved;
- product save catalog remains save browser/index policy;
- renderer/Vulkan should only consume generated room/draw data later.

## Owned Files

First model/parser slices should own new files only:

```text
src/app/iggy3d/AsciiRoomSource.hpp
src/app/iggy3d/AsciiRoomSource.cpp
src/app/iggy3d/AsciiRoomGrid.hpp
src/app/iggy3d/AsciiRoomGrid.cpp
tests/unit/ascii_room_source_tests.cpp
tests/unit/ascii_room_grid_tests.cpp
CMakeLists.txt
cmake/iggy3d_tests.cmake
```

Second compile slice should own:

```text
src/app/iggy3d/AsciiRoomToAuthoredRoom.hpp
src/app/iggy3d/AsciiRoomToAuthoredRoom.cpp
tests/unit/ascii_room_to_authored_room_tests.cpp
```

Later fixture/package slices may own:

```text
fixtures/rooms/ascii/*.iggyroom.txt
fixtures/demos/ascii_room/package.iggy3d.toml
docs/fixtures/ascii_room_authoring.md
```

## No-Go Files

Do not touch these in v0.1 parser/model slices:

```text
src/app/iggy3d/AppShell.cpp
src/app/iggy3d/ReceiptBuilder.*
src/render/**
src/render/vulkan/**
apps/iggy3d_visual_demo/**
docs/vulkan/**
```

Do not add:

- JSON;
- renderer/Vulkan behavior;
- window proof;
- game-specific NPC AI;
- combat behavior;
- save/load schema changes;
- AppShell compilation branches;
- EDI repo imports;
- Blender/Python execution;
- generated OBJ as gameplay truth.

## Glyph Vocabulary

V0.1 should use a small fixed vocabulary that is easy to extend later.

Locked v0.1 glyphs:

```text
#  wall
.  floor
space  floor
+  door
s  secret door
P  player spawn
N  NPC spawn
M  monster spawn
$  treasure
K  key or pickup
T  trap
E  exit or objective
?  inspect marker
```

Initial behavior:

| Glyph | Semantic | Walkable | Blocks Actor | Marker |
| --- | --- | --- | --- | --- |
| `#` | wall | false | true | false |
| `.` | floor | true | false | false |
| space | floor | true | false | false |
| `+` | door | true | closed blocker surface | true |
| `s` | secret door | true | closed blocker surface | true |
| `P` | player spawn on floor | true | false | true |
| `N` | NPC spawn on floor | true | false | true |
| `M` | monster spawn on floor | true | false | true |
| `$` | treasure marker on floor | true | false | true |
| `K` | key/pickup marker on floor | true | false | true |
| `T` | trap marker on floor | true | false | true |
| `E` | exit/objective marker on floor | true | false | true |
| `?` | inspect marker on floor | true | false | true |

Unknown glyph policy:

- default v0.1 should reject unknown glyphs;
- error must include row, column, glyph, and reason;
- a later permissive preview mode may preserve unknown glyphs as tagged floor,
  but gameplay compile should be strict.

Canonical v0.1 test map:

```text
#######
#P..N.#
#.+.$.#
#..E..#
#######
```

Expected high-level facts:

```text
width=7
height=5
player_spawn_count=1
npc_count=1
door_count=1
treasure_count=1
exit_count=1
```

## Source Model

Planned value types:

```text
AsciiRoomSource
  sourceName
  rawText
  rows
  width
  height
  status
  reasonCode
  diagnostics

AsciiRoomDiagnostic
  severity
  reasonCode
  row
  column
  glyph
  message
```

Parsing policy:

- normalize CRLF to LF;
- ignore one trailing empty line;
- reject completely empty source;
- reject ragged rows;
- ragged rows fail with `ascii_room_ragged_rows`;
- store row/column as zero-based internally;
- receipts/tests print one-based line/column for user-facing diagnostics.

Reason codes:

```text
ascii_room_ok
ascii_room_empty
ascii_room_ragged_rows
ascii_room_unknown_glyph
ascii_room_too_large
ascii_room_no_floor
ascii_room_missing_player_spawn
ascii_room_multiple_player_spawns
ascii_room_marker_without_floor
```

## Semantic Grid Model

Planned value types:

```text
AsciiRoomCellKind
  Floor
  Wall
  Door
  SecretDoor
  PlayerSpawn
  NpcSpawn
  MonsterSpawn
  Treasure
  Key
  Trap
  Exit
  Inspect

AsciiRoomCell
  row
  column
  glyph
  kind
  walkable
  blocksActor
  blocksProjectile
  markerTag
  sourceOffset

AsciiRoomGrid
  width
  height
  cells
  playerSpawnCount
  markerCount
  floorCount
  wallCount
```

Data ownership:

- `AsciiRoomSource` owns raw text and parse diagnostics;
- `AsciiRoomGrid` owns cell semantics;
- `AsciiRoomToAuthoredRoom` owns generated floors, walls, and
  `AsciiRoomMarker` sidecar records;
- `SaveAuthoredRoomSection` owns generated floor/wall geometry only in v0.1;
- `AsciiRoomMarker` owns spawn/object/door/treasure/NPC/trap semantics until a
  later gameplay binding slice converts those markers into package/runtime
  entities;
- runtime session does not own source parsing;
- renderer does not own semantic interpretation.

## Coordinate Convention

Iggy3D must use 3D world axes consistently:

```text
grid column -> +X
grid row -> +Z
world Y -> height/elevation
row 0 -> north/top
column 0 -> west/left
```

Room-local direction rule:

```text
lower row index = north
higher row index = south
world +Z = row-increase / south
world -Z = north
```

Default centering policy:

```text
world_x = (column - (width - 1) / 2.0) * tile_size
world_z = (row - (height - 1) / 2.0) * tile_size
world_y = elevation
```

This makes row values increase south/down in +Z. A camera or compass layer can
label negative Z as north later if desired.

Alternative top-left origin is useful for import/export, but v0.1 game compile
should center generated rooms around world origin.

Defaults:

```text
tile_size_meters = 1.0
floor_thickness_meters = 0.10
wall_height_meters = 2.50
wall_thickness_meters = 1.0 for block walls in slice 2
marker_y_meters = 0.05
```

Cell center:

```text
center_x = (column - (width - 1) / 2.0) * tile_size
center_z = (row - (height - 1) / 2.0) * tile_size
```

Floor record:

```text
center = (center_x, -floor_thickness / 2, center_z)
size = (tile_size, floor_thickness, tile_size)
```

Block wall record:

```text
center = (center_x, wall_height / 2, center_z)
size = (tile_size, wall_height, tile_size)
```

Marker position:

```text
position = (center_x, marker_y, center_z)
```

## Geometry Generation Algorithms

V0.1 should separate simple correctness from optimization.

Slice 1:

- parse ASCII;
- build semantic grid;
- no authored room generation yet.

Slice 2 simple geometry:

- every walkable cell emits one floor tile;
- every wall cell emits one wall block;
- every marker cell emits a marker/spawn record at tile center;
- doors emit floor plus door marker plus a runtime-owned closed-door blocker
  surface;
- secret doors emit floor plus secret-door marker plus a runtime-owned
  closed-door blocker surface.

Slice 3 wall-run optimization:

- merge horizontal wall runs of length >= 2 into one wall segment;
- merge vertical wall runs of length >= 2 into one wall segment only for wall
  cells not already consumed by horizontal runs;
- isolated walls remain one block;
- doors break wall runs;
- generated segment ids must be deterministic.

Why simple first:

- easier source diagnostics;
- clearer unit tests;
- no geometry optimization bugs mixed with parser bugs;
- renderer can consume a correct room before optimization.

## Door And Opening Semantics

Door glyphs are not walls in v0.1. They are walkable marker cells with a
separate runtime-owned blocker surface.

Rules:

- `+` compiles as floor plus marker tag `door` plus a `door_panel` mesh and
  `door_blocker` spatial surface;
- `s` compiles as floor plus marker tag `secret_door` plus a `door_panel` mesh
  and `door_blocker` spatial surface;
- the blocker surface uses `runtime_owner_stable_name` equal to the door marker
  id, for example `marker_door_r2_c2`;
- generated product door entities use `OpenDoor + EmitEventOnly +
  deactivateTargetOnSuccess`;
- closed door collision is represented by the active runtime owner;
- opening a door deactivates the runtime door entity and product active-room
  collision filters out the owned blocker surface;
- `+` opens without an item requirement;
- `s` requires the first `K` key pickup when the room contains a key, and
  rejects interaction with `required_item_missing` until the player has it;
- `+` and `s` do not cut authored wall openings in v0.1;
- v0.1 does not infer hinges or animated door hardware.

Door validation:

- door should be adjacent to at least one wall cell;
- v0.1 emits warning `ascii_room_door_not_adjacent_to_wall` when a door is not
  adjacent to at least one wall cell;
- v0.1 does not reject non-adjacent doors.

## Entity And Object Marker Semantics

Markers are semantic placeholders, not full gameplay behavior.

V0.1 marker ownership:

```text
AsciiRoomMarker
```

Markers do not live in `SaveAuthoredRoomSection` yet. That save section is the
floor/wall authored-room geometry target. Marker records travel beside the
authored room compile result until a later entity-binding or package-binding
slice consumes them.

Locked v0.1 marker tags:

```text
player_spawn
npc_spawn
monster_spawn
treasure
key
trap
exit
inspect
door
secret_door
```

Generated marker record should include:

```text
AsciiRoomMarker
id
tag
glyph
row
column
worldPosition
sourceLine
sourceColumn
```

Required deterministic marker id format:

```text
marker_<tag>_r<row>_c<column>
```

Current implemented runtime marker binding:

- `K` and `$` become pickup entities with item ids matching their marker ids;
- every pickup gets a `collect_<marker id>` inventory objective;
- `s` doors require the first `K` item when one exists;
- `E` becomes an objective-trigger marker with `CompleteObjective`;
- `E` requires the first `$` treasure item when one exists;
- completing an `E` exit objective sets runtime `SessionOutcome::Victory`;
- required-item failures reject command admission before tick execution;
- required-item facts are saved, loaded, hashed, and replay-visible.

Deferred entity behavior:

- NPC AI;
- trap effects;
- monster-specific behavior beyond neutral marker binding;
- authored hinge, animation, and door hardware metadata.

The compile result should preserve enough tags for a later gameplay binding
slice to turn markers into actual runtime entities.

Current full-loop proof:

- `product_ascii_gameplay_loop_smoke` runs no-window and no-AppShell;
- source path is ASCII text to authored room to room asset to product package
  session seed to runtime session;
- proven loop is key pickup, secret door open, treasure pickup, exit objective,
  and final `Victory`;
- the smoke also proves the exit rejects with `required_item_missing` before
  treasure is collected.

Current product-app tape proof:

- `--gameplay-tape <path>` accepts a plain ordered line tape, not JSON;
- supported v0.1 commands are `move <stable_name>`, `interact <stable_name>`,
  `wait`, `expect_reject <reason> interact <stable_name>`, and
  `expect_blocked <movement_reason> move <stable_name>`;
- tape target ownership is stable runtime entity name, usually the deterministic
  ASCII marker id;
- unexpected failure stops the tape immediately;
- expected rejection steps are recorded and do not tick the runtime;
- expected movement-block steps must execute the command, tick runtime movement,
  and then match `SessionState::transient.lastMovementResult`;
- `product_gameplay_tape_smoke` generates an ASCII package, launches
  `./build/iggy3d --no-window --auto-new-world --gameplay-tape`, and proves the
  same key, secret door, treasure, exit, `Victory` loop through the product app;
- receipt proof fields are prefixed with `gameplay_tape_` and include loaded
  state, step counts, expected rejection count, expected blocked count, failed
  step, failed movement block, final action/target, last movement block,
  key/door/treasure/exit booleans, and loop completion.

Current active-room runtime proof:

- package-backed ASCII rooms now become `ProductActiveRoomState` with
  `active_room_source=package_room`;
- product runtime session creation builds `ProductActiveRoomCollisionState` from
  the package room and active `SessionState`;
- the gameplay tape runner uses active-room collision surfaces when present;
- active-room collision refreshes after each tape tick so runtime-owned blockers
  such as opened doors are filtered before the next movement command;
- exported room text preserves full 8-point `box` spatial surfaces so package
  collision has the same height as the in-memory generated room;
- the room asset loader accepts legacy 4-point boxes and full 8-point boxes;
- product no-window smokes prove both positive traversal through an opened
  secret door and negative collision against a package-loaded wall.

## Validation Rules

Hard errors for v0.1:

- empty source;
- unknown glyph;
- ragged rows;
- no floor/walkable cells;
- no player spawn;
- more than one player spawn;
- map exceeds configured max width or height.

Player spawn policy:

- exactly one `P` is required;
- there is no default spawn fallback;
- missing spawn fails with `ascii_room_missing_player_spawn`;
- multiple spawns fail with `ascii_room_multiple_player_spawns`.

Warnings for v0.1:

- no exit marker;
- no treasure/objective marker;
- door not adjacent to wall;
- marker appears on an otherwise blocked cell after future custom glyph rules;
- disconnected walkable region.

Connectivity:

- not required for first parser slice;
- later semantic-grid slice may use 4-connected flood fill from player spawn;
- disconnected walkable regions should warn first, then optionally fail when
  package compile requires it.

Flood fill policy:

```text
neighbors = north, east, south, west
walkable = floor, door, secret door, markers on floor
walls are not walkable
```

## Compile Result Model

Planned value types:

```text
AsciiRoomCompileConfig
  tileSizeMeters
  floorThicknessMeters
  wallHeightMeters
  centerOnOrigin
  strictUnknownGlyphs
  strictRectangularRows
  requirePlayerSpawn

AsciiRoomCompileResult
  ok
  status
  reasonCode
  grid
  authoredRoom
  markers
  diagnostics
  proof
```

Generated authored room ids should be deterministic:

```text
floor_r<row>_c<column>
wall_r<row>_c<column>
wall_h_r<row>_c<start>_len<count>
wall_v_c<column>_r<start>_len<count>
marker_<tag>_r<row>_c<column>
```

Slice 2 authored-room boundary:

- floors and walls compile into `SaveAuthoredRoomSection`;
- markers remain `AsciiRoomMarker` records in the compile result;
- no marker should become a runtime entity in Slice 2;
- no marker should be written into `SaveAuthoredRoomSection` unless that type is
  explicitly expanded in a later approved packet.

Generated semantics:

```text
floor:
  materialId=debug_floor
  traversalTags=walkable
  gameplayTags=floor
  walkable=true
  blocksActor=false
  blocksProjectile=false

wall:
  materialId=debug_wall
  traversalTags=clamber_candidate later
  gameplayTags=wall
  walkable=false
  blocksActor=true
  blocksProjectile=true
```

## Proof Fields

No-window proof should be deterministic.

Receipt/proof fields:

```text
ascii_room_status
ascii_room_reason_code
ascii_room_source_name
ascii_room_width
ascii_room_height
ascii_room_cell_count
ascii_room_floor_count
ascii_room_wall_count
ascii_room_marker_count
ascii_room_player_spawn_count
ascii_room_door_count
ascii_room_unknown_glyph_count
ascii_room_warning_count
ascii_room_tile_size_meters
ascii_room_centered_on_origin
ascii_room_authored_floor_count
ascii_room_authored_wall_count
ascii_room_spawn_count
ascii_room_treasure_count
ascii_room_npc_count
```

Proof text:

- source echo with normalized rows;
- semantic grid echo with canonical glyphs;
- diagnostic list sorted by source row/column;
- generated authored room summary.

Do not require renderer proof for parser/model slices.

## Compute Costs

Parser:

```text
O(width * height)
```

Semantic grid:

```text
O(cell_count)
```

Simple geometry:

```text
O(cell_count)
```

Wall-run compression:

```text
O(cell_count)
```

Connectivity flood fill:

```text
O(walkable_cell_count + edge_count)
```

These costs are trivial for hand-authored rooms. For generated maps, enforce a
maximum width/height before allocating cell vectors.

Default v0.1 limits:

```text
max_width = 256
max_height = 256
max_cells = 65536
```

## Coding Methods

Use typed model-first code:

- parser returns value structs, not AppShell side effects;
- semantic grid is immutable after construction where practical;
- compile config is explicit;
- every failure has a stable reason code;
- tests assert reason codes and exact generated ids;
- no renderer dependency;
- no runtime session mutation;
- no JSON.

Use table-driven glyph semantics:

```text
glyph -> cell kind -> behavior flags -> marker tag
```

Do not scatter glyph checks across parser, compiler, and tests. Keep one source
of glyph truth.

Use structured generation, not ad hoc string parsing:

- parse source into cells;
- compile cells into typed records;
- only proof rendering emits text.

Why:

- lets gameguy/EDI-style map ideas feed Iggy3D without merging code;
- makes the room compiler unit-testable;
- keeps renderer work downstream;
- preserves future ability to add TOML/EDI imports into the same semantic grid.

## Builder Slices

### Slice 1 - ASCII Source And Semantic Grid

Files:

```text
src/app/iggy3d/AsciiRoomSource.hpp
src/app/iggy3d/AsciiRoomSource.cpp
src/app/iggy3d/AsciiRoomGrid.hpp
src/app/iggy3d/AsciiRoomGrid.cpp
tests/unit/ascii_room_source_tests.cpp
tests/unit/ascii_room_grid_tests.cpp
CMakeLists.txt
cmake/iggy3d_tests.cmake
```

Scope:

- glyph table;
- source parser;
- semantic grid model;
- diagnostics;
- coordinate conversion helper;
- no authored room output yet.

### Slice 2 - Compile Grid To Authored Room

Files:

```text
src/app/iggy3d/AsciiRoomToAuthoredRoom.hpp
src/app/iggy3d/AsciiRoomToAuthoredRoom.cpp
tests/unit/ascii_room_to_authored_room_tests.cpp
```

Scope:

- floor records;
- wall records;
- `AsciiRoomMarker` sidecar records;
- deterministic ids;
- source row/column tracking;
- markers stay sidecar records and do not become runtime entities;
- no runtime package wiring.

### Slice 3 - ASCII Fixture Proof

Files:

```text
fixtures/rooms/ascii/training_room.iggyroom.txt
tests/unit/ascii_room_fixture_tests.cpp
docs/fixtures/ascii_room_authoring.md
```

Scope:

- first authored ASCII room fixture;
- proof summary;
- no renderer requirement.

### Slice 4 - Runtime Package Bridge

Scope:

- convert compiled authored room into the existing package/demo path;
- make it loadable as an authored room fixture;
- no Vulkan-specific work.

### Slice 5 - Visual Primitive Proof

Scope:

- renderer/product view consumes generated floors/walls/markers;
- primitive cubes/markers are acceptable;
- one bounded window proof only if explicitly approved.

## Test Plan

Parser tests:

- empty source rejects;
- trailing newline accepted;
- unknown glyph rejects with row/column;
- ragged rows reject with `ascii_room_ragged_rows`;
- valid map reports width/height/cell counts;
- raw glyphs are preserved.

Grid tests:

- each glyph maps to expected kind and flags;
- player spawn count;
- marker counts by tag;
- row/column indexing;
- coordinate conversion centers map correctly;
- tile size changes world positions predictably.

Compile tests:

- floors generated for walkable cells;
- walls generated for wall cells;
- doors generate floor plus `AsciiRoomMarker`;
- `+` and `s` do not cut wall openings in v0.1;
- player spawn marker generated at center;
- treasure/NPC/trap markers generated with tags;
- markers remain sidecar records, not runtime entities;
- generated ids are deterministic;
- generated wall/floor sizes use config values.

Validation tests:

- missing player spawn fails;
- multiple player spawns fail;
- exactly one `P` succeeds;
- no default spawn fallback exists;
- no floor fails;
- too large map fails;
- disconnected walkable region warns when connectivity check is enabled.

Regression tests:

- no AppShell dependency;
- no renderer dependency;
- no JSON;
- no window launch required.

## Acceptance Gate

The v0.1 plan is builder-ready when it states:

- ASCII source is truth;
- semantic grid is the first durable handoff;
- renderer is downstream;
- row/column maps to X/Z and Y is height;
- glyph vocabulary is explicit;
- parser and geometry generation are separate;
- no JSON;
- no AppShell compilation;
- first builder slice is parser/model/tests only.

## Stop Rules

Stop and ask before implementing if a slice requires:

- renderer or Vulkan changes;
- AppShell changes;
- save/load schema changes;
- game-specific NPC AI/combat behavior;
- a new external format besides ASCII text;
- JSON;
- importing EDI code directly;
- Blender/Python execution;
- broad fixture/package rewrites;
- window proof.

## Blocking Decisions

None blocking v0.1 parser/model work.

Deferred decisions:

- whether permissive unknown glyphs are useful for rough sketch mode;
- final tile size for production rooms;
- whether row-positive south should later flip to world `-Z`;
- whether doors later become authored openings, animated meshes, locked doors,
  interactable entities, or a combination;
- how ASCII room fixtures attach to world creation UI;
- how EDI/gameguy exports should target this semantic grid later.
