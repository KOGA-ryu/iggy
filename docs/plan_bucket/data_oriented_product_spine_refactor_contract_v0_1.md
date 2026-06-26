# Data-Oriented Product Spine Refactor Contract v0.1

## Objective

Map the full product-spine refactor before more feature work lands.

The user-facing goal is still narrow:

- boot to the starter window;
- create a titled world;
- author a dungeon room quickly from ASCII/draft/editor controls;
- see floors and walls as real 3D room geometry;
- explore, edit, save, exit, reboot, and Continue back into that room.

The engineering goal is to stop growing product behavior through ad hoc
`AppShell.cpp` branch corridors. New behavior must enter through data-owned
registries, route tables, validators, pipeline passes, or backend adapters.

This contract is for review. It is not a worker implementation order by itself.

## Hard Rules

- No feature expansion while a refactor slice is active unless the slice says so
  explicitly.
- No new source branch statements under `src/` or `apps/` without branch-gate
  approval.
- Builder orders must omit model and thinking overrides unless the user
  explicitly approves them.
- `AppShell.cpp` must shrink toward lifecycle composition only.
- ASCII is map making only. ASCII source, glyphs, and layout symbols do not own
  behavior, profile ids, guard policy, or gameplay rules.
- No NPC behavior work is part of this refactor bucket.
- The retired visual demo must stay retired. Do not restore
  `apps/iggy3d_visual_demo` or `package_visual_*`.
- Tests passing is not enough. A slice that grows branch debt without approval
  is rejected.

## Current Baseline

Governance already exists:

- `tools/check_branch_gate.py` rejects new source `if`, `else if`, `switch`,
  and ternary expressions without a branch-gate id;
- `docs/branch_gate_policy.md` documents the approval rule;
- `docs/branch_gate_approvals.tsv` is currently the approval ledger.

Completed data-oriented cleanup baseline:

- visual demo retired;
- AppShell automation parser ladders were converted to mapping tables;
- product automation command registry model exists;
- `room_editor.*` automation aliases canonicalize through the registry;
- `room_edit.*` automation aliases canonicalize through the registry;
- menu shortcut automation uses a key-to-`InputAction` table.

Current branch-debt audit result:

| Area | Main File | Debt Type | First Replacement Shape |
| --- | --- | --- | --- |
| Product orchestration | `src/app/iggy3d/AppShell.cpp` | mixed ownership, automation corridor, route branches | command registry and state handlers |
| Product operations | `src/app/iggy3d/ProductAppOperations.cpp` | save/load/world flow branching | request/result executors |
| Receipts | `src/app/iggy3d/ReceiptBuilder.cpp` | manual field append surface | receipt descriptor table |
| Frontend routing | `src/app/iggy3d/ProductFrontendRouter.cpp` | screen/action route branching | route table and owner handlers |
| Starter/world setup | `src/app/frontend/StarterScreen.cpp`, `src/app/frontend/WorldSetupModel.cpp` | row enablement and field routing | menu/action descriptors |
| ASCII pipeline | `src/app/iggy3d/AsciiRoom*.cpp` | glyph/layout conversion branches | glyph schema and pipeline stages |
| Editable room | `src/content/authoring/EditableRoomDocument.cpp` | command validation and bake branching | edit command descriptors and validation rules |
| Render geometry | `src/render/vulkan/BufferImageResources.cpp` | bake pass and backend safety branches | bake emitter table plus backend guards |
| Input | `src/app/input/KeyboardInput.cpp`, `src/app/input/GamepadInput.cpp` | binding branches | input binding tables |

## Coding Method Rules

Use the method that matches the problem:

| Problem Shape | Required Method | Example |
| --- | --- | --- |
| string to enum/action | mapping table | `menu.up -> InputAction::MenuUp` |
| enum to handler | dispatch table | `RoomEditorCommandId -> apply function` |
| screen mode behavior | owned state handler | `applyPauseInput(...)` |
| field or invariant validation | validator or guard function | `validateWorldSetupDraft(...)` |
| parser table | schema table | required keys and value kinds |
| render geometry emission | bake pass and emitter table | floor pass, wall run pass, grid pass |
| platform/API failure | backend adapter guard | Vulkan allocation or index overflow |
| hot loop | precomputed buckets/plans | merged wall runs, floor rectangles |

Mapping branches are debt. Validation guards are allowed when named and owned by
the validating module. Backend safety checks are allowed inside backend
boundaries. Product flow conditions do not belong in `AppShell.cpp`.

## Product Spine Ownership

### AppShell

Owns:

- process options;
- outer lifecycle;
- active session presence;
- calling product controllers;
- calling render/projection builders;
- writing receipts.

Must not own:

- per-command automation policy;
- menu row behavior;
- world setup field logic;
- save browser selection policy;
- room editor command semantics;
- geometry bake policy;
- receipt field lists.

### Automation

Source truth:

- `ProductAutomationCommandRegistry` owns command keys, aliases, categories,
  value kinds, and metadata.

Next ownership target:

- `ProductAutomationCommandController` should own lookup, parse, command gating,
  and dispatch into category-specific handlers.

AppShell should eventually call:

```text
applyProductAutomationCommand(controllerContext, command)
```

and copy the result.

### Frontend And Starter Window

Source truth:

- starter rows belong in starter/menu definitions;
- route results belong in frontend router helpers;
- disabled reasons belong with row definitions and selectors.

AppShell should only:

- ask the frontend router for a route result;
- apply route transitions;
- call product operations for runtime side effects.

### World Setup And Dungeon Draft

Source truth:

- `WorldSetupDraft` owns title, seed, selected field, and create request facts;
- `ProductDungeonDraft` owns cursor movement and paint results;
- ASCII source is only map authoring input.

Draft command ownership should move toward:

```text
WorldSetupCommandSpec -> parse value -> validate mode -> apply draft operation
```

### ASCII To Room Pipeline

Source truth:

```text
AsciiRoomSource
-> AsciiRoomGrid
-> SaveAuthoredRoomSection
-> EditableRoomDocument
-> RoomAsset
-> SceneRoomProjection
-> Vulkan CPU room mesh
```

Each stage must return value/result structs. No stage may reach back into
AppShell for policy.

### Room Editing

Source truth:

- `ProductRoomEditorCursor` owns cursor/tool direction;
- `ProductRoomEditorActionController` owns input action to edit command;
- `EditableRoomDocument` owns document validation and bake;
- room editor overlay is derived presentation only.

Next ownership target:

```text
RoomEditorCommandSpec -> parse value -> apply cursor/edit operation
```

### Save/Load

Source truth:

- runtime save store owns file durability;
- product save bridge owns product-facing write/load/delete/recover adapters;
- product save catalog owns newest compatible Continue policy;
- AppShell should not own catalog sorting or selection policy.

### Rendering And Vulkan

Source truth:

- projection owns scene facts;
- CPU geometry bake owns floor/wall/grid emit decisions;
- Vulkan backend owns device, buffer, image, and API safety checks;
- receipt/proof fields report metrics, not behavior truth.

Next ownership target:

```text
RoomGeometryBakePass {
  role;
  compatibility key;
  merge plan;
  emitter;
}
```

Backend safety branches remain direct guards inside Vulkan boundaries.

### Receipts

Source truth:

- receipt field names and value extraction should live in descriptor tables.

Target shape:

```text
ReceiptFieldSpec {
  key;
  source group;
  read function;
  value kind;
}
```

Receipts must remain deterministic key-value lines. No JSON.

## Full Refactor Ladder

### Phase 0 - Governance And Dead Surface Removal

Status: implemented.

Completed scope:

- retire visual demo;
- delete package visual smoke surface;
- add branch-gate script and policy;
- require builder orders to avoid reasoning/model overrides unless approved.

Acceptance:

- no visual-demo references remain;
- branch gate passes on committed slices;
- no dirty source before dispatching implementation slices.

### Phase 1 - AppShell Automation Intake

Purpose:

Remove the automation command corridor from AppShell without changing behavior.

Completed early slices:

- parser mapping tables;
- registry model;
- registry canonicalization for `room_editor.*`;
- registry canonicalization for `room_edit.*`;
- table-driven menu shortcut automation.

Remaining slices:

1. table-drive gameplay input automation;
2. canonicalize save browser and deleted-save browser automation;
3. canonicalize world setup and dungeon draft automation;
4. canonicalize ASCII preview/activation automation;
5. canonicalize frontend/system/settings/dev-tools automation;
6. add `ProductAutomationCommandController` model only;
7. migrate one category at a time from AppShell to controller handlers;
8. delete old AppShell key corridor after parity proof.

Stop rules:

- if a migration changes receipt keys or command semantics, stop;
- if branch gate fails, stop;
- if a category requires new behavior, defer it to a separate feature bucket.

### Phase 2 - Receipt Descriptor Tables

Purpose:

Stop hand-appending new receipt fields as product features grow.

Slices:

1. add descriptor model for product receipt fields;
2. table-drive stable window-state fields with parity tests;
3. table-drive room editor/render proof fields;
4. table-drive save/load fields;
5. leave uncommon ad hoc fields only when they are backend diagnostics.

Acceptance:

- receipt output matches previous key set and values;
- tests prove stable ordering;
- no feature keys are added during migration.

### Phase 3 - Frontend Route And Starter Window Ownership

Purpose:

Make the main starter window a state-owned product surface rather than an
AppShell branch set.

Slices:

1. define starter action descriptor table;
2. define world setup field/action descriptors;
3. route starter, load-save, settings, dev tools, and pause through owned
   handlers;
4. replace `applyOpeningMenuAction` branches with router calls;
5. prove starter to New World to gameplay and Continue paths by no-window smoke.

Acceptance:

- starter window rows and disabled reasons are table-owned;
- AppShell no longer branches on selected starter rows;
- existing starter/menu smokes pass.

### Phase 4 - World Setup And Dungeon Draft Commands

Purpose:

Keep map creation fast while preventing world setup from becoming another
branch corridor.

Slices:

1. add `WorldSetupCommandSpec` and `DungeonDraftCommandSpec`;
2. migrate title/dungeon id/draft edit mode/draft move/draft paint/draft cell
   automation through command specs;
3. keep ASCII source as map-authoring input only;
4. add command parity tests for cursor paint and create flow;
5. prove New World creates a saved authored room with floors/walls.

Acceptance:

- no raw ASCII glyph behavior semantics;
- no AppShell-owned world setup field logic;
- create flow and save gate remain unchanged.

### Phase 5 - Room Editor Command Ownership

Purpose:

Make in-game room editing extensible without AppShell feature branches.

Slices:

1. define room editor command descriptors for move, tool, cycle, wall
   direction, place;
2. define room edit command descriptors for add/delete/undo/redo;
3. route automation and physical editor input through the same controller;
4. keep cursor overlay derived from editor state;
5. prove edit, save, exit, reboot, and Continue restore the edited room.

Acceptance:

- editor actions and automation use the same command path;
- AppShell only supplies context and copies result summaries;
- collision/render proof remains current after edits.

### Phase 6 - Save/Load Product Flow Ownership

Purpose:

Keep the save loop reliable while removing AppShell-owned save policy.

Slices:

1. introduce save command descriptors for select, delete, show deleted,
   deleted select, recover;
2. move selected-row policy to save browser/catalog helpers;
3. move Continue selection proof to product save catalog;
4. keep durable save writes in product save bridge/runtime save store;
5. prove Save, Save And Exit, reboot starter, and Continue.

Acceptance:

- AppShell does not sort or choose save rows;
- deleted/recover surfaces remain separate;
- save file truth remains `.iggy3d.save`.

### Phase 7 - Render Geometry Bake Tables And Performance Proof

Purpose:

Make floor/wall geometry efficient without asking the user to think about
triangles while placing objects.

Existing baseline:

- floor merge metrics exist;
- floor CPU mesh merge exists;
- wall segment orientation source truth exists;
- wall run merge exists;
- live editable-room bake now carries wall segment metadata.

Next slices:

1. introduce `RoomGeometryBakePass` descriptors for floor, wall, grid, overlay;
2. isolate merge compatibility keys from emitters;
3. add room-size performance harness;
4. prove vertex/index/draw count reductions for generated maps;
5. keep Vulkan allocation/upload guards inside backend code.

Performance tools:

- `render_room_mesh_geometry_tests`;
- `product_room_geometry_optimization_tests`;
- `product_ascii_map_smoke`;
- `--print-render-receipt`;
- future `tools/measure_room_pipeline.py` for `8x8`, `16x16`, `32x32`, and
  `64x64` generated rooms.

Acceptance:

- optimized draw counts are lower than naive counts for mergeable rooms;
- invalid geometry still fails closed;
- screenshot/window proof is optional and explicit, not required for every
  no-window slice.

### Phase 8 - Input Binding Tables

Purpose:

Stop keyboard/gamepad input mapping from becoming per-key branch code.

Slices:

1. add keyboard binding descriptors for menu/gameplay/editor actions;
2. add gamepad binding descriptors;
3. keep edge/hold policy as binding metadata;
4. prove physical editor input still maps to cursor movement/tool/place.

Acceptance:

- adding a key is adding a row;
- SDL sampling guards stay local;
- gameplay input is suppressed while editor owner is active.

### Phase 9 - AppShell Shrink Passes

Purpose:

Make AppShell visibly small enough to read.

Slices:

1. extract automation control read/apply/report orchestration;
2. extract projection refresh pipeline;
3. extract room editor state copy/report helpers;
4. extract startup/load/save flow coordinator calls;
5. keep only lifecycle composition in AppShell.

Acceptance:

- AppShell branch count trends down each slice;
- source behavior remains proven by existing smokes;
- no broad rewrite lands without focused parity proof.

### Phase 10 - Branch Debt Reporting

Purpose:

Make branch growth visible before review.

Slices:

1. add `tools/report_branch_debt.py`;
2. report branch count by touched file;
3. report categories by simple heuristics;
4. require worker briefs to include branch delta for source slices.

Acceptance:

- every source slice says branch statements added, removed, and net change;
- branch debt reports are proof artifacts, not subjective impressions.

## Review Checklist

Before approving any implementation slice:

- Does it name the coding method?
- Does it say which ownership boundary is being moved?
- Does it avoid feature expansion?
- Does it avoid new source branch statements?
- Does it preserve receipt keys and behavior unless explicitly scoped?
- Does it run `tools/check_branch_gate.py`?
- Does it list exact focused tests?
- Does it avoid model and thinking overrides in worker dispatch?

## Test Plan

Refactor slices should prefer focused proof:

- `cmake --build build --target iggy3d_app`
- `tools/check_branch_gate.py`
- `tools/check_branch_gate.py --diff HEAD~1..HEAD`
- `git diff --check`
- targeted unit tests for the moved model/controller;
- targeted no-window smokes for starter, world setup, room editor, save/load,
  and room render receipts.

Broad CTest loops are not required for every slice and should not replace
focused proof.

## Acceptance Gate

The product-spine refactor is acceptable when:

- `AppShell.cpp` is primarily lifecycle composition;
- automation commands are registry/controller driven;
- starter/menu/world setup behavior is state-owned;
- receipt fields are descriptor driven;
- room editor commands are controller driven;
- save/load policy lives in save catalog/bridge modules;
- room geometry bake policy lives in bake pass/emitter modules;
- branch gate is standard verification for source slices;
- no worker has to ask where to add another AppShell `if` for routine work.

## Open Questions

None for v0.1 review.

Deferred feature work remains outside this refactor bucket:

- richer level editor UI;
- mouse placement;
- material palette tools;
- custom room asset libraries;
- pathfinding or behavior systems;
- advanced Vulkan profiling overlays.

## Implementation Stop Rules

Stop immediately if:

- branch gate fails;
- a slice needs a new feature to complete;
- a slice needs to change receipt keys for parity;
- a source file is dirty with unrelated user/controller work;
- a worker order would require model or thinking overrides;
- a builder attempts to restore visual-demo/package-visual files;
- a builder routes behavior through raw ASCII glyphs;
- a builder adds NPC behavior work under this bucket.

