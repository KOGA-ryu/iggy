# World Creation UX Contract v0.1

## Objective

Define product world creation as the durable product path behind `New World`.

World creation must create a playable saved world lineage. It is not just a
shortcut into a demo session.

Required product flow:

```text
Starter
  -> New World
  -> World Setup Draft
  -> Create
  -> Create runtime session
  -> Write initial manual save
  -> Enter gameplay
```

This contract started as a plan-only document. The current repo now implements
the save-gated New World path, durable initial save write, and optional
ASCII-room-backed world creation described below.

## Current Baseline

The repo already has a partial world creation surface:

```text
src/app/frontend/FrontendState.hpp
src/app/frontend/FrontendState.cpp
src/app/frontend/StarterScreen.hpp
src/app/frontend/StarterScreen.cpp
src/app/iggy3d/DefaultWorldTemplate.hpp
src/app/iggy3d/DefaultWorldTemplate.cpp
src/app/iggy3d/OpeningMenuView.hpp
src/app/iggy3d/OpeningMenuView.cpp
src/app/iggy3d/AppShell.cpp
src/app/iggy3d/FrontendActionExecutor.hpp
src/app/iggy3d/FrontendActionExecutor.cpp
```

Current implemented capabilities:

- `FrontendScreen::NewWorld` exists;
- `FrontendAction::NewWorld` exists;
- `FrontendAction::CreateAndEnter` exists;
- starter routing can open the `NewWorld` child screen;
- `WorldSetupModel` owns a deterministic draft, validation, and create/back
  result;
- `ProductWorldCreation` builds product creation and initial-save request
  values from a validated draft;
- `launchProductNewWorld` creates a runtime session and enters gameplay only
  after the durable initial save succeeds;
- optional ASCII room source can be supplied on the world setup draft and
  compiled into authored room data before the runtime session is created;
- ASCII-backed New World initial saves persist the generated authored-room
  section;
- Continue/Load Save rehydrates saved authored-room floors and walls back into
  `ProductActiveRoomState` and active-room collision;
- `ProductWorldTemplate` exposes package id, scenario id, display name,
  source, authored floor count, and authored wall count;
- `drawNewWorldPanel` draws a basic new-world panel;
- `--auto-new-world` can skip starter and launch directly;
- product receipts already report world/template facts through existing
  product receipt paths.

Current limitations:

- no persisted `world_id` exists;
- no product world metadata record exists;
- no persisted `worldTitle` metadata exists yet;
- no seed editing model exists;
- no difficulty model exists;
- no starting scenario choice model exists;
- saved authored-room rehydration now restores durable floors, walls, markers,
  anchors, door panels, and door blocker surfaces; game-specific NPC AI,
  treasure inventory behavior, and objective binding remain separate runtime
  gameplay work;
- current world creation still has AppShell composition code that should shrink
  as UI models become real widgets.

## Source-Fit Notes For Builders

These notes describe the current source behavior that a later implementation
packet must work with. Do not infer product world creation semantics from the
existing direct-launch helpers.

- `ProductWorldTemplate` is a package/scenario/template descriptor. It is not a
  world setup draft, not a product world record, and not durable world metadata.
- `drawNewWorldPanel` currently displays template package/scenario/save-count
  facts. It does not yet render full editable world title, seed, difficulty,
  starting scenario, validation, ASCII source, or save-gate state.
- `applyOpeningMenuAction` still composes selected `NewWorld` through
  `launchProductNewWorld`, but that path now prepares product world creation,
  creates a runtime session, writes the durable initial manual save, and enters
  gameplay only after the save succeeds.
- `launchProductNewWorld` currently does not allocate durable `world_id` or
  write product world metadata beyond the save envelope/authored-room section.
- `FrontendActionExecutor` currently treats `CreateAndEnter` as a generic launch
  request. Future product behavior must narrow that meaning to "create a saved
  world, then enter gameplay after the initial save succeeds."
- `--auto-new-world` skips the starter screen into the same save-gated creation
  route. It must not become a separate source of product world creation truth.
- Product automation drives frontend/menu semantics and a small set of world
  setup draft fields. It must not become a parallel world creation path.
- Current source has legacy title field names in `WorldSetupDraft` and
  `ProductWorldCreationRequest`. Future product save catalog work must either
  migrate those fields to `worldTitle` or map them at the boundary. Product
  semantics should use `worldTitle`.

## Relationship To Save Load Contract

This contract depends on:

```text
docs/plan_bucket/save_load_ux_contract_v0_1.md
docs/plan_bucket/product_save_catalog_contract_v0_1.md
```

Save/load locked the rule:

```text
New World creates an initial save immediately.
```

World creation must not create orphan worlds. Gameplay entry is allowed only
after the initial manual save succeeds.

If the product later supports intentionally unsaved sandbox sessions, that must
be a separate named mode. It is not a fallback inside New World.

## Product Definition

A world is a user-facing lineage:

```text
world_id
world_title
world_seed
difficulty
starting_scenario
created_at_utc
package_id
scenario_id
initial_save_id
```

A world setup draft is not a world.

A runtime session is not enough to be a product world.

A save file is the durable gameplay truth for a world at a point in time.

## User-Facing Flow

Starter row:

```text
New World
```

Selecting New World opens:

```text
World Setup
```

V0.1 world setup fields:

```text
World Title
Seed
Difficulty
Starting Scenario
ASCII Room Source
Create
Back
```

Default draft:

```text
world_title=New World
seed=generated_editable
difficulty=standard
starting_scenario=training_ground
ascii_room_enabled=false
ascii_room_id=world_setup_room
ascii_room_source_name=world_setup_ascii_room.iggyroom.txt
```

Back behavior:

```text
discard draft
write nothing
create no world id
create no save
return to starter
```

Create behavior:

```text
validate draft
create world id
create world metadata
create runtime session
write initial manual save
if save succeeds: enter gameplay
if save fails: stay on world setup or world error route
```

## World Setup Draft

Proposed draft structure:

```text
WorldSetupDraft
  world_title
  seed_text
  resolved_seed
  seed_generated
  difficulty
  starting_scenario
  ascii_room_enabled
  ascii_room_text
  ascii_room_id
  ascii_room_source_name
  selected_field
  valid
  reason_code
```

Draft ownership:

- frontend/product state owns the draft;
- runtime does not own the draft;
- save files do not own the draft until Create succeeds;
- Back discards the draft;
- Create validates and converts draft into a product world creation request.

Draft must be deterministic for tests.

## Field Semantics

World Title:

- visible display name;
- default `New World`;
- trimmed for validation;
- empty after trim is invalid;
- max display length should be finite and tested;
- v0.1 recommended max length: 64 characters;
- user-visible invalid reason: `invalid_world_title`.

Seed:

- visible editable text;
- default generated seed;
- empty seed means generate deterministic replacement at Create time;
- seed can be stored as text and resolved numeric seed;
- invalid seed reports `invalid_seed`;
- v0.1 may accept alphanumeric seed text and hash/resolve later, but the draft
  result must expose the resolved seed deterministically.

Difficulty:

- v0.1 values:

```text
standard
```

- additional difficulties can be added later without changing the draft shape.

Starting Scenario:

- v0.1 values:

```text
training_ground
```

- the scenario maps to package/scenario data through product world creation,
  not through the frontend draft itself.

ASCII Room Source:

- ASCII is for map making only. It may be used as optional world-setup
  authoring input, but it is not gameplay truth, AI truth, control input, or a
  runtime behavior format;
- optional v0.1 authoring input;
- disabled by default;
- when enabled, raw ASCII source text is preserved as authoring input and is not
  printed in receipts;
- `ascii_room_id` is trimmed and must be non-empty;
- `ascii_room_source_name` is trimmed and must be non-empty;
- empty enabled source text reports `invalid_ascii_room_text`;
- empty enabled room id reports `invalid_ascii_room_id`;
- empty enabled source name reports `invalid_ascii_room_source_name`;
- successful Create compiles the ASCII source into authored room data before
  runtime session creation; after that point gameplay consumes generated
  authored room/package/save/session data;
- the initial save writes the generated authored-room section so the save owns
  durable room truth.

Create:

- enabled only when the draft validates;
- returns a route/result request, not direct runtime mutation.

Back:

- always enabled;
- discards draft.

## Product World Creation Request

Proposed request shape:

```text
ProductWorldCreationRequest
  world_title
  world_seed
  difficulty
  starting_scenario
  ascii_room_requested
  ascii_room_id
  ascii_room_source_name
  package_id
  scenario_id
  save_root
  requested_at_utc
```

The request is built from:

- `WorldSetupDraft`;
- selected/default `ProductWorldTemplate`;
- product options such as save root;
- clock/time provider for UTC timestamp.

The request must not contain renderer resources.

## Product World Creation Result

Proposed result shape:

```text
ProductWorldCreationResult
  status
  reason_code
  world_id
  world_title
  world_seed
  difficulty
  starting_scenario
  package_id
  scenario_id
  session_created
  initial_save_requested
  initial_save_written
  initial_save_id
  route_after_create
```

Allowed result statuses:

```text
not_requested
draft_invalid
session_create_failed
initial_save_failed
created
```

Gameplay is allowed only for:

```text
status=created
session_created=true
initial_save_written=true
route_after_create=gameplay
```

## Initial Save Semantics

Initial save:

```text
save_type=initial
save_title_present=false
default_title=<worldTitle>
world_id=<created world id>
authored_room.present=true when ASCII room source was used
```

Initial save must be written before gameplay.

If initial save fails:

- do not enter gameplay;
- keep user on world setup or world error route;
- preserve draft values;
- show recoverable failure reason;
- emit receipt;
- do not create an active world in the selector.

The first implementation may model this without full product save metadata, but
the route/result semantics must already reflect the hard gate.

On load:

- product save load decodes the save envelope and exposes the authored-room
  section in `ProductSaveLoadResult`;
- if the load succeeds and authored room data is present, AppShell replaces the
  package/default active room with `saved_authored_room`;
- collision is rebuilt from the loaded session state and durable authored-room
  floors/walls;
- absent authored-room data leaves the package/default active room in place.

## World Id Semantics

`world_id` must be stable, deterministic enough for tests, and safe for file
paths if reused in storage.

Recommended v0.1 format:

```text
world_<timestamp_or_counter>_<slug>
```

For unit tests, a fake id provider should produce deterministic ids:

```text
world_0001
world_0002
```

Do not derive long-term world identity only from display name. Display names can
change later.

Do not derive long-term world identity only from save filename. A world can own
many saves.

## Data Ownership

| Domain | Owns | Must not own |
| --- | --- | --- |
| `WorldSetupModel.*` | draft fields, validation, field navigation, create/back result | session creation, save writes |
| `ProductWorldCreation.*` | orchestration request/result, world id allocation, initial save request | menu row drawing |
| `ProductAsciiRoomPackage.*` | transient in-memory package for ASCII-authored room sessions | save catalog policy |
| `ProductActiveRoomState.*` | active-room summary and saved authored-room reconstruction | save-file scanning |
| `ProductWorldTemplate.*` | default package/scenario/display source | user draft state |
| `Session` / runtime | gameplay state after creation | starter UI draft |
| `ProductSaveBridge.*` | initial save write request/result | field validation UI |
| Runtime save files | durable gameplay truth | frontend selected field |
| `ProductFrontendRouter.*` | route requests and parent returns | runtime save encoding |
| `AppShell.cpp` | lifecycle composition only | per-field world setup logic |
| Receipts | deterministic proof | gameplay mutation |

## Proposed Durable Files

Likely new files:

```text
src/app/frontend/WorldSetupModel.hpp
src/app/frontend/WorldSetupModel.cpp
src/app/iggy3d/ProductWorldCreation.hpp
src/app/iggy3d/ProductWorldCreation.cpp
src/app/iggy3d/ProductAsciiRoomPackage.hpp
src/app/iggy3d/ProductAsciiRoomPackage.cpp
src/app/iggy3d/ProductActiveRoomState.hpp
src/app/iggy3d/ProductActiveRoomState.cpp
```

Likely existing files to use or extend later:

```text
src/app/iggy3d/DefaultWorldTemplate.hpp
src/app/iggy3d/DefaultWorldTemplate.cpp
src/app/frontend/FrontendRoute.hpp
src/app/frontend/FrontendRoute.cpp
src/app/frontend/StarterScreen.hpp
src/app/frontend/StarterScreen.cpp
src/app/iggy3d/ProductFrontendRouter.hpp
src/app/iggy3d/ProductFrontendRouter.cpp
src/app/iggy3d/SaveBridge.hpp
src/app/iggy3d/SaveBridge.cpp
src/app/iggy3d/AppShell.cpp
src/app/iggy3d/ReceiptBuilder.hpp
src/app/iggy3d/ReceiptBuilder.cpp
```

`AppShell.cpp` should be touched only after model/router tests prove the route
contract. Do not start world creation by expanding the existing direct branch.

## No-Go Files

Do not touch these for v0.1 planning or first implementation slices:

```text
src/render/**
src/render/vulkan/**
docs/vulkan/**
fixtures/** package schemas
apps/iggy3d_visual_demo/**
```

Do not add JSON or another new machine-contract format.

## Route Semantics

Starter New World route:

```text
screen=starter
child_screen=new_world
selected_action=new_world
status=starter_new_world_opened
```

World setup Back:

```text
screen=starter
child_screen=gameplay
selected_action=back
status=world_setup_back
draft_discarded=true
```

World setup Create with invalid draft:

```text
screen=starter
child_screen=new_world
selected_action=create_and_enter
accepted=false
status=world_setup_invalid
route_after_create=world_setup
```

World setup Create with save failure:

```text
screen=starter
child_screen=new_world
selected_action=create_and_enter
accepted=false
status=initial_save_failed
route_after_create=world_error
```

World setup Create success:

```text
screen=gameplay
child_screen=gameplay
selected_action=create_and_enter
accepted=true
status=world_created
route_after_create=gameplay
```

`CreateAndEnter` must eventually mean create saved world, not merely create a
runtime session.

## Input Semantics

All devices and automation must feed semantic actions.

World setup consumes:

```text
menu.up
menu.down
menu.left
menu.right
menu.confirm
menu.back
text.insert
text.delete
field.next
field.previous
```

V0.1 can begin with model-level semantic inputs only. Physical key/controller
binding changes are not required for Slice 1.

Automation should use the same semantic route/action names. Do not create a
separate automation-only world creation path.

## Validation Semantics

Validation result fields:

```text
valid=true|false
reason_code=<reason>
field=<field-or-none>
```

Reason codes:

```text
ok
invalid_world_title
invalid_seed
invalid_ascii_room_text
invalid_ascii_room_id
invalid_ascii_room_source_name
unsupported_difficulty
unsupported_scenario
session_create_failed
initial_save_failed
storage_full
permission_denied
unknown_error
```

Invalid draft does not call runtime session creation or save writing.

## Receipt Fields

Receipts must remain deterministic key-value text.

World setup:

```text
world_setup_open=true|false
world_setup_selected_field=<field-or-none>
world_setup_valid=true|false
world_setup_reason_code=<reason>
world_setup_draft_title=<title>
world_setup_seed_text=<text>
world_setup_seed_generated=true|false
world_setup_difficulty=<difficulty>
world_setup_starting_scenario=<scenario>
world_setup_ascii_room_enabled=true|false
world_setup_ascii_room_text_present=true|false
world_setup_ascii_room_id=<id>
world_setup_ascii_room_source_name=<source-name>
```

Create request:

```text
world_create_requested=true|false
world_create_accepted=true|false
world_create_status=<status>
world_create_reason_code=<reason>
world_id=<id-or-none>
world_title=<title-or-none>
world_seed=<seed-or-none>
world_difficulty=<difficulty-or-none>
world_scenario=<scenario-or-none>
world_creation_ascii_room_requested=true|false
world_creation_ascii_room_id=<id-or-none>
world_creation_ascii_room_source_name=<source-name-or-none>
```

Initial save:

```text
initial_save_requested=true|false
initial_save_written=true|false
initial_save_id=<id-or-none>
initial_save_type=initial
initial_save_default_title=<worldTitle-or-none>
initial_save_title_present=true|false
product_save_load_authored_room_present=true|false
product_save_load_authored_room_id=<id-or-none>
product_save_load_authored_floor_count=<count>
product_save_load_authored_wall_count=<count>
product_save_load_authored_marker_count=<count>
active_room_source=saved_authored_room when loaded from save-authored geometry
active_room_authored_marker_count=<count>
```

Routing:

```text
route_before_create=<route>
route_after_create=gameplay|world_setup|world_error
gameplay_entered_after_initial_save=true|false
```

## Test Plan

Unit tests for `WorldSetupModel` should prove:

- default draft values;
- generated seed state;
- world title validation;
- seed validation;
- difficulty validation;
- starting scenario validation;
- Back discards draft;
- invalid Create returns rejected result;
- valid Create returns creation request;
- route/status/reason fields are deterministic.
- optional ASCII room fields default disabled;
- invalid enabled ASCII fields reject with stable reasons;
- valid ASCII room create request carries source text, room id, and source name.

Unit tests for `ProductWorldCreation` should prove later:

- world id provider is deterministic under test;
- valid request creates session request;
- session creation failure blocks gameplay;
- initial save failure blocks gameplay;
- initial save success returns gameplay route;
- initial save default title is exactly `worldTitle`;
- product result includes world id and initial save id.
- ASCII room selection facts are preserved without filesystem IO.

Current and future no-window smokes should prove:

- starter New World opens world setup;
- default setup can create a saved world;
- initial save appears in save browser;
- ASCII setup can create a saved world whose initial save contains an authored
  room section;
- Continue can load that save and rebuild active-room collision from the saved
  authored-room floors/walls;
- failed initial save keeps the app out of gameplay;
- receipts prove `gameplay_entered_after_initial_save=true` only on success.

Window proof is not required by default.

## First Implementation Slice

Builder-safe first slice:

```text
World Creation Foundation / Slice 1 - WorldSetupModel
```

Allowed files:

```text
src/app/frontend/WorldSetupModel.hpp
src/app/frontend/WorldSetupModel.cpp
tests/unit/world_setup_model_tests.cpp
CMakeLists.txt
cmake/iggy3d_tests.cmake
```

Allowed behavior:

- create draft type;
- create validation type;
- create create/back result type;
- default values;
- deterministic fake seed provider for tests;
- unit tests only.

Slice 1 output must be a pure frontend model contract. It may define a creation
request value, but it must not execute that request.

Explicitly not allowed in Slice 1:

- `AppShell.cpp` migration;
- session creation changes;
- save writing;
- save schema changes;
- `SaveFileStore` changes;
- renderer/Vulkan changes;
- automation control changes;
- window launches;
- product smoke migration.

Slice 1 acceptance:

```text
cmake --build build --target iggy3d_app
cmake --build build --target world_setup_model_tests
ctest --test-dir build --output-on-failure -R '^world_setup_model_tests$'
git diff --check
```

## Later Implementation Order

1. `WorldSetupModel` draft and validation.
2. Starter route result uses world setup create/back semantics.
3. `ProductWorldCreation` request/result without AppShell migration.
4. Initial save request/result seam using product save bridge.
5. Safe initial save write once save bridge has durable write semantics.
6. AppShell migration from direct `launchProductNewWorld` to orchestrated
   world creation.
7. No-window receipt proof.
8. Visual UI polish for world setup fields.

Do not combine model creation, save writing, and AppShell migration in one
slice.

## Acceptance Gate

This plan is builder-slice ready when:

- current direct launch baseline is named;
- world setup draft is separate from a world;
- initial save gate is explicit;
- no orphan world fallback exists;
- data ownership is split between frontend model, product orchestration,
  runtime session, and save bridge;
- first implementation slice is model-only;
- receipts prove create/save/gameplay ordering;
- no JSON, renderer, Vulkan, or window proof is required.

## Stop Rules

Stop before implementation if a slice requires:

- gameplay entry before initial save succeeds;
- hardcoding demo-only world creation as product behavior;
- expanding `AppShell.cpp` branches before model/router tests exist;
- changing renderer/Vulkan;
- changing package fixture schemas;
- changing save schema before product identity plan is approved;
- writing image bytes into save files;
- launching a window by default;
- adding JSON or another external machine-contract format.

## Open Questions

No blocking user decisions remain for `WorldSetupModel` Slice 1.

Deferred product flavor decisions:

- final difficulty list;
- final starting scenario list;
- exact generated seed display format;
- whether world title can be renamed after creation;
- whether later world creation has an Advanced page;
- whether later character/profile setup belongs before or after world creation.

## Long-Term Fit

This contract turns New World from a direct session launch into a durable,
save-backed product flow.

The stable split is:

- world setup draft is frontend/product state;
- product world creation orchestrates session and initial save;
- runtime owns gameplay state;
- save files own durable truth;
- AppShell composes the flow but does not own field behavior;
- receipts prove the ordering without a window.
