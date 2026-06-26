# Map Generation Slices With DoD v0.1

## Objective

Define the next map-generation work as reviewable slices with clear Definition
of Done gates.

The product goal:

```text
Starter
-> New World
-> choose or generate a dungeon room
-> see floor and wall geometry in 3D
-> explore it
-> enter room edit mode
-> save
-> exit
-> reboot
-> Continue into the same edited room
```

This document focuses on floors and walls first. Doors, treasure, markers,
lighting, materials, and authored objects can layer on later, but this slice
ladder must first make map shape generation boring, deterministic, testable,
and fast.

DoD means Definition of Done. Each DoD below also includes the required
data-oriented design rule for the slice.

## Hard Rules

- ASCII is map making only. It may be used as input/proof for room layout, but
  raw glyphs do not own gameplay behavior.
- No NPC behavior work.
- No visual-demo or `package_visual_*` restoration.
- No JSON.
- No new AppShell branch corridors.
- No new source branch statements unless branch-gate approval exists.
- Builder orders must omit model and thinking overrides unless explicitly
  approved by the user.
- Source slices must run `tools/check_branch_gate.py`.
- Performance proof is required once generation changes geometry size or render
  counts.

## Current Source Truth

Existing usable pipeline:

```text
ProductDungeonDraft
-> AsciiRoomSource
-> AsciiRoomGrid
-> AsciiRoomToAuthoredRoom
-> AsciiRoomToEditableRoom
-> EditableRoomDocument
-> AsciiRoomToRoomAsset
-> SceneProjection
-> BufferImageResources::buildRoomMeshCpuGeometry
```

Existing proof surfaces:

- `product_ascii_map_smoke`;
- `product_ascii_authoring_smoke`;
- `product_room_editing_state_tests`;
- `product_room_editor_cursor_tests`;
- `product_room_geometry_optimization_tests`;
- `render_room_mesh_geometry_tests`;
- render receipt fields such as:
  - `product_vulkan_room_mesh_cpu_ready`;
  - `product_vulkan_room_wall_draw_count`;
  - `product_vulkan_room_vertex_count`;
  - `product_vulkan_room_index_count`;
  - `product_vulkan_room_geometry_signature`.

Existing optimization baseline:

- floor rectangles can merge;
- wall segments preserve orientation;
- wall runs can merge;
- editable-room wall bakes now carry segment metadata into room assets.

## Data Ownership

Map generation must keep these layers separate:

| Layer | Owns | Does Not Own |
| --- | --- | --- |
| Generation request | seed, dimensions, room style, constraints | runtime session mutation |
| Generated grid | floor/wall cell facts | gameplay behavior |
| ASCII proof | human-readable layout display | durable runtime truth |
| Authored room | floors, walls, markers, source refs | renderer allocation |
| Editable room | in-game edit document and commands | save file IO |
| Room asset | render/collision-ready room data | product flow policy |
| Scene projection | scene facts for draw/build | authoring policy |
| Vulkan CPU geometry | vertices, indices, draw counts | source truth |
| AppShell | composition only | map generation policy |

## Coding Methods

Use the method that matches the slice:

| Work Type | Method |
| --- | --- |
| generator settings | config struct and validation rule table |
| deterministic room shape | pure request/result model |
| shape templates | descriptor table |
| cell classification | data table or dense grid pass |
| grid to walls/floors | pipeline stage |
| optimization | precomputed merge plans |
| editor commands | command descriptors |
| render bake | bake pass/emitter descriptors |
| receipts | descriptor fields |

Branches that protect invalid input or backend failure are allowed as guards.
Branches that map key/name/type to behavior should become tables or dispatchers.

## Definitions

Generated map:

- a deterministic layout result from a request;
- contains only layout facts at v0.1: floors and walls;
- may emit ASCII proof text;
- must be rebuildable from seed/config.

Generated room:

- authored-room records created from generated map output;
- can be edited like any other room;
- can be saved and loaded through existing authored-room save path.

Generated room is not:

- an NPC behavior source;
- a save file by itself;
- a renderer-only mesh;
- an AppShell command corridor.

## Slice 0 - Planning And Governance Baseline

Purpose:

Freeze the work shape before source work starts.

Allowed files:

- `docs/plan_bucket/map_generation_slices_dod_v0_1.md`;
- `docs/plan_bucket/README.md`.

No-go files:

- all source;
- all tests;
- all build files.

DoD:

- plan lists every slice through starter/create/edit/save/continue proof;
- each slice has allowed files, no-go files, and explicit proof commands;
- ASCII hard rule is present;
- branch gate is required for source slices;
- no placeholder decision markers remain.

## Slice 1 - Map Generation Request And Result Model

Purpose:

Add a pure product-owned model for requesting a generated floor/wall map.

Suggested files:

```text
src/app/iggy3d/ProductMapGeneration.hpp
src/app/iggy3d/ProductMapGeneration.cpp
tests/unit/product_map_generation_tests.cpp
CMakeLists.txt
cmake/iggy3d_tests.cmake
```

Model shape:

```text
ProductMapGenerationRequest
  seedText
  widthCells
  heightCells
  styleId
  wallPolicy

ProductGeneratedMapCell
  row
  column
  kind = floor | wall

ProductMapGenerationResult
  ok
  status
  reasonCode
  normalizedSeed
  widthCells
  heightCells
  cells
  asciiPreview
```

Coding method:

- pure request/result model;
- validation rule table for dimensions/style/seed;
- no AppShell;
- no runtime session;
- no renderer.

DoD:

- missing/invalid dimensions reject with stable reason codes;
- unknown style rejects with stable reason code;
- same request produces same normalized result;
- result contains only floor/wall cells;
- ASCII proof exists but is not source of gameplay behavior;
- branch gate passes;
- focused unit test passes.

Proof commands:

```bash
cmake --build build --target iggy3d_app
cmake --build build --target product_map_generation_tests
ctest --test-dir build --output-on-failure -R '^product_map_generation_tests$'
git diff --check
tools/check_branch_gate.py
```

## Slice 2 - First Deterministic Room Shape

Purpose:

Implement the first real generator: a rectangular room with surrounding walls
and interior floors.

Scope:

- floors and walls only;
- no doors;
- no NPCs;
- no treasure;
- no pathfinding;
- no AppShell wiring.

Algorithm:

```text
for each cell:
  border cell -> wall
  interior cell -> floor
```

Data-oriented method:

- generate into dense row-major cell buffer;
- use style descriptor row for wall/floor glyph/proof chars;
- keep validation separate from generation.

DoD:

- `8x8`, `16x16`, and non-square rooms generate deterministic counts;
- wall count equals border cell count;
- floor count equals interior cell count;
- ASCII preview dimensions match request;
- no behavior semantics are attached to glyphs;
- branch gate passes;
- focused tests pass.

Proof commands:

```bash
cmake --build build --target product_map_generation_tests
ctest --test-dir build --output-on-failure -R '^product_map_generation_tests$'
git diff --check
tools/check_branch_gate.py
```

## Slice 3 - Generated Map To Authored Room Adapter

Purpose:

Convert generated floor/wall cells into existing authored-room records.

Suggested files:

```text
src/app/iggy3d/ProductGeneratedMapToAuthoredRoom.hpp
src/app/iggy3d/ProductGeneratedMapToAuthoredRoom.cpp
tests/unit/product_generated_map_to_authored_room_tests.cpp
```

Coding method:

- pipeline stage;
- input is `ProductMapGenerationResult`;
- output is existing authored-room section;
- source refs preserve row/column;
- no AppShell and no save IO.

DoD:

- generated floors become authored floor records;
- generated walls become authored wall records with oriented segment metadata;
- floor/wall counts match generator result;
- source row/column refs are deterministic;
- invalid generation result rejects without partial authored room;
- authored room can bake into `EditableRoomDocument`;
- branch gate passes.

Proof commands:

```bash
cmake --build build --target product_generated_map_to_authored_room_tests
ctest --test-dir build --output-on-failure -R '^product_generated_map_to_authored_room_tests$'
cmake --build build --target product_room_editing_state_tests
ctest --test-dir build --output-on-failure -R '^product_room_editing_state_tests$'
git diff --check
tools/check_branch_gate.py
```

## Slice 4 - Generated Map To Room Asset And Geometry Proof

Purpose:

Prove generated rooms become renderable 3D floors and walls through the existing
room asset and Vulkan CPU geometry path.

Allowed files:

- generated-map adapter files;
- render geometry tests if needed;
- no AppShell.

Coding method:

- pipeline proof from generated map to authored room to room asset to CPU
  geometry;
- no renderer window;
- no Vulkan upload changes.

DoD:

- generated room asset is ready;
- CPU mesh geometry is ready;
- floor draw count and wall draw count are positive;
- optimized wall/floor draw counts are lower than naive counts for mergeable
  rooms;
- vertex/index counts are positive and deterministic;
- geometry signature changes when dimensions or seed/style changes;
- branch gate passes.

Proof commands:

```bash
cmake --build build --target render_room_mesh_geometry_tests
ctest --test-dir build --output-on-failure -R '^render_room_mesh_geometry_tests$'
cmake --build build --target product_room_geometry_optimization_tests
ctest --test-dir build --output-on-failure -R '^product_room_geometry_optimization_tests$'
git diff --check
tools/check_branch_gate.py
```

## Slice 5 - World Setup Generated Map Selection Model

Purpose:

Let world setup carry a generated-map request without AppShell owning field
logic.

Suggested files:

```text
src/app/frontend/WorldSetupModel.*
src/app/iggy3d/ProductMapGeneration.*
tests/unit/world_setup_model_tests.cpp
tests/unit/product_map_generation_tests.cpp
```

Coding method:

- world setup draft owns chosen generation settings;
- generated map request is value data;
- world setup validates request before Create;
- no AppShell execution yet.

DoD:

- default New World draft can carry a generated room request;
- invalid generation settings reject at model level;
- valid Create request includes generated map request;
- no runtime session or save write occurs in this slice;
- branch gate passes.

Proof commands:

```bash
cmake --build build --target world_setup_model_tests
ctest --test-dir build --output-on-failure -R '^world_setup_model_tests$'
cmake --build build --target product_map_generation_tests
ctest --test-dir build --output-on-failure -R '^product_map_generation_tests$'
git diff --check
tools/check_branch_gate.py
```

## Slice 6 - Product World Creation Generated Map Request

Purpose:

Thread generated-map request data into product world creation without creating a
session or writing a save in this slice.

Suggested files:

```text
src/app/iggy3d/ProductWorldCreation.*
tests/unit/product_world_creation_tests.cpp
```

Coding method:

- pure request/result extension;
- no AppShell;
- no save IO.

DoD:

- prepared product world creation result carries generated map request;
- invalid request status is stable;
- initial save plan remains requested but unwritten;
- no generated room is activated yet;
- branch gate passes.

Proof commands:

```bash
cmake --build build --target product_world_creation_tests
ctest --test-dir build --output-on-failure -R '^product_world_creation_tests$'
git diff --check
tools/check_branch_gate.py
```

## Slice 7 - Generated Map Session Seed Bridge

Purpose:

Create the generated authored room and feed it into the existing product session
seed path.

Allowed files:

```text
src/app/iggy3d/ProductPackageSessionSeed.*
src/app/iggy3d/ProductGeneratedMapToAuthoredRoom.*
tests/unit/product_package_session_seed_tests.cpp
tests/unit/product_generated_map_to_authored_room_tests.cpp
```

Coding method:

- bridge model;
- no AppShell;
- no save/write;
- no renderer backend.

DoD:

- generated map request produces authored room data for session seed;
- session seed contains floor and wall room anchors/surfaces as existing paths
  expect;
- invalid generation fails closed with stable reason;
- no raw ASCII glyph behavior semantics are introduced;
- branch gate passes.

Proof commands:

```bash
cmake --build build --target product_package_session_seed_tests
ctest --test-dir build --output-on-failure -R '^product_package_session_seed_tests$'
cmake --build build --target product_generated_map_to_authored_room_tests
ctest --test-dir build --output-on-failure -R '^product_generated_map_to_authored_room_tests$'
git diff --check
tools/check_branch_gate.py
```

## Slice 8 - AppShell Integration Through Refactored Controller Only

Purpose:

Wire generated map creation into the product New World flow only after the
automation/refactor spine has a suitable controller seam.

Precondition:

- AppShell automation/world setup command handling has moved behind controller
  or state-owned handlers;
- no active dirty user refactor conflicts.

Allowed files:

- integration controller files;
- minimal AppShell composition call only if the refactor seam already exists;
- focused product smoke tests.

No-go:

- no new AppShell command corridor;
- no field-specific AppShell logic;
- no renderer backend changes;
- no NPC behavior.

DoD:

- New World can create a generated floor/wall room;
- initial durable save is written only after generated room creation succeeds;
- gameplay enters only after save succeeds;
- receipts prove active room source, room id, floor count, wall count,
  collision readiness, and Vulkan CPU mesh readiness;
- branch gate passes;
- no-window smoke passes.

Proof commands:

```bash
cmake --build build --target iggy3d_app
cmake --build build --target product_ascii_map_smoke
ctest --test-dir build --output-on-failure -R '^product_ascii_map_smoke$'
cmake --build build --target product_world_setup_smoke
ctest --test-dir build --output-on-failure -R '^product_world_setup_smoke$'
git diff --check
tools/check_branch_gate.py
```

## Slice 9 - Generated Room Edit, Save, Exit, Continue Proof

Purpose:

Prove the full user loop after generated map integration.

Flow:

```text
fresh app
-> New World
-> generated dungeon room
-> gameplay
-> pause Edit Room
-> place one wall with room editor
-> Save And Exit
-> fresh app boot
-> Continue
-> restored generated-and-edited room
```

Coding method:

- smoke proof only unless a real source defect is found;
- no behavior changes if current source already supports the flow.

DoD:

- generated original room has deterministic floor/wall counts;
- editor placement changes authored wall count by one;
- save file contains edited authored room;
- fresh starter sees one compatible save;
- Continue loads the edited room from disk;
- active room source and room id are correct;
- active room collision is ready;
- CPU room mesh is ready with positive vertex/index/draw counts;
- optimized wall/floor draw counts are deterministic;
- branch gate passes if source changes were required.

Proof commands:

```bash
cmake --build build --target iggy3d_app
cmake --build build --target product_ascii_map_smoke
ctest --test-dir build --output-on-failure -R '^product_ascii_map_smoke$'
git diff --check
tools/check_branch_gate.py
```

## Slice 10 - Map Generation Performance Harness

Purpose:

Measure generated room pipeline costs before larger maps or richer generation.

Suggested file:

```text
tools/measure_room_pipeline.py
```

Harness behavior:

- generate `8x8`, `16x16`, `32x32`, `64x64` rooms;
- run no-window proof or focused unit targets;
- collect floor/wall source counts;
- collect optimized draw counts;
- collect vertex/index counts;
- collect geometry signature;
- emit TSV.

DoD:

- script runs from repo root;
- output is deterministic TSV;
- no source behavior changes;
- metrics show naive vs optimized draw counts;
- documentation says which fields are performance signals and which are proof
  signals.

Proof commands:

```bash
python3 tools/measure_room_pipeline.py --sizes 8,16,32,64
git diff --check
```

## Slice 11 - Generation Styles As Data

Purpose:

Add room style descriptors after the rectangle generator is proven.

Examples:

- rectangular room;
- two-room connector;
- loop corridor;
- courtyard.

Coding method:

- style descriptor table;
- generator functions registered by id;
- no AppShell field branches.

DoD:

- each style has id, display name, min/max dimensions, and generator id;
- unknown style rejects;
- each style has deterministic count proof;
- generated geometry proof remains valid;
- branch gate passes.

## Slice 12 - Visual Proof Gate

Purpose:

Add explicit visual proof only after no-window generated room metrics are
stable.

Allowed proof:

- `--window` launch only when explicitly approved;
- screenshot or frame-hash smoke;
- receipt proof must still accompany image proof.

DoD:

- visual proof shows generated floors and walls, not just an empty frame;
- screenshot/frame hash links to same room id and geometry signature as receipt;
- no new renderer feature work is hidden inside the proof slice.

## Global DoD For Any Map Generation Slice

Every source slice is done only when:

- it names the coding method used;
- it preserves ASCII as map/layout only;
- it avoids new AppShell branch corridors;
- it passes branch gate;
- it has focused tests;
- it reports exact files changed;
- it reports behavior changed or confirms no behavior change;
- it reports receipt fields changed or confirms none changed;
- it reports performance/proof fields when geometry changes;
- it keeps save/load and renderer changes out unless explicitly scoped.

## Review Questions Before Dispatch

- Is the slice model-only, adapter-only, integration-only, proof-only, or
  performance-only?
- Does it need AppShell? If yes, why can it not wait for the refactored
  controller seam?
- Does it add any command key, receipt key, save field, or render path?
- Does it rely on raw ASCII glyphs for behavior?
- Does it have a DoD that proves user-visible progress?
- Does it have a DoD that prevents branch-debt growth?

## Stop Rules

Stop immediately if:

- the branch gate fails;
- the slice requires a new feature not named in its scope;
- AppShell becomes the owner of generation policy;
- ASCII glyphs are treated as behavior;
- save/load schema changes appear unexpectedly;
- renderer/Vulkan backend changes appear before CPU geometry proof;
- a worker order would need a model or thinking override;
- unrelated dirty source files overlap the planned edit.
