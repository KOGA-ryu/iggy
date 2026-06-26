# Product Module Consolidation Audit v0.1

## Objective

Reduce product-spine sprawl while making `AppShell.cpp` a shell.

This is not a cleanup campaign for its own sake. The goal is to stop two forms
of growth:

- `AppShell.cpp` taking product policy that belongs to domain modules;
- tiny one-shot product files accumulating until the project is hard to scan.

The target shape is fewer durable product modules, not one file per feature.

Prerequisite:

- `product_module_taxonomy_contract_v0_1.md` defines the folder and naming
  scheme that consolidation slices must use.
- Do not start moving source until the target folder/module taxonomy and the
  file migration table for the slice are explicit.

## Audit Tool

Use:

```bash
tools/audit_product_module_shape.py --top 20
```

Optional machine-readable output:

```bash
tools/audit_product_module_shape.py --format csv --output /tmp/product_module_shape.csv
```

The tool is read-only. It scans the product/frontend/content/render seams and
reports:

- file path;
- nonblank LOC;
- branch hits;
- include count;
- exported type/function count;
- likely domain;
- target consolidated module;
- recommendation;
- reason.

Current audit snapshot from this repo state:

- scanned files: 196;
- nonblank LOC: 31555;
- branch hits: 3837;
- merge candidates: 66;
- extraction roots: 2;
- split/table candidates: 8.

Top product pressure:

| File | Finding | Action |
| --- | --- | --- |
| `src/app/iggy3d/AppShell.cpp` | largest product policy sink | extract to domain modules |
| `src/app/iggy3d/ProductAppOperations.cpp` | mixed save/world/session orchestration | split by domain |
| `src/app/iggy3d/OpeningMenuView.cpp` | view/layout branch pressure | defer until flow state is cleaner |
| `src/content/authoring/EditableRoomDocument.cpp` | real domain validation/edit logic | keep guards, table command descriptors later |
| `src/render/vulkan/BufferImageResources.cpp` | geometry bake plus backend guards | split bake policy only, keep backend safety guards |

## Target Product Domains

The target is not `ProductHelpers` or another junk drawer. Each consolidated
module must have one durable domain.

| Target module | Owns | Does not own |
| --- | --- | --- |
| `product/Shell` (compatibility via `src/app/iggy3d/Shell.*` + temporary wrappers) | app lifecycle, option parse result handling, top-level sequencing | save policy, room editing policy, map generation |
| `product/Frontend` | starter/pause/settings/dev-tools/save-selector/world-setup screen flow | durable save writes, room geometry |
| `product/Automation` | command registry, parser, dispatch, command result recording | separate feature behavior |
| `product/Save` | Continue, Load Save, Save, Save And Exit, delete, recover, save-slot selection | filesystem primitives, runtime save codec |
| `product/World` | world setup draft, title/seed/dungeon selection, New World launch request | room mesh baking |
| `product/Room` | editable room state, edit commands, cursor, overlay, editor actions | ASCII parsing pipeline, Vulkan |
| `product/RoomPipeline` | ASCII/grid/authored/editable/room-asset conversion and generated-map adapters | editor input, save/load |
| `product/Projection` | scene/debug/draw-list/viewport/render-bridge proof assembly | Vulkan allocation/upload |
| `product/Receipt` | descriptor-driven receipt projection | domain behavior |
| `product/Gameplay` | gameplay command/controller/tape proof | menus, save flow |

## Consolidation Targets

### product/RoomPipeline

Likely fold together:

- `AsciiRoomSource.*`
- `AsciiRoomGrid.*`
- `AsciiRoomToAuthoredRoom.*`
- `AsciiRoomToEditableRoom.*`
- `AsciiRoomToRoomAsset.*`
- `AsciiRoomAssetText.*`
- `ProductAsciiRoomActivation.*`
- `ProductAsciiRoomAuthoring.*`
- `ProductAsciiRoomEditing.*`
- `ProductAsciiRoomPackage.*`
- `ProductAsciiRoomPreview.*`
- `ProductActiveRoomState.*`
- `ProductActiveRoomCollision.*`

Why:

- this is one pipeline family: map text/grid/source -> authored/editable/asset
  -> active room/collision;
- the current file count is high because each adapter became a separate
  feature file;
- map generation should plug into this domain, not create another chain of
  one-shot adapters.

DoD for a consolidation slice:

- reduces file count or clearly removes a wrapper-only file;
- keeps public data contracts intact or adds a compatibility include for one
  transition slice only;
- no ASCII behavior semantics are added;
- existing ASCII/map/room smoke tests pass;
- branch gate passes.

### product/Room

Likely fold together:

- `ProductRoomEditingState.*`
- `ProductRoomAuthoringController.*`
- `ProductRoomEditorCursor.*`
- `ProductRoomEditorActionController.*`
- `ProductRoomEditorOverlay.*`
- `EditableRoomToAuthoredRoom.*`

Why:

- these are all one user-facing room editing domain;
- separate files made sense during foundation slices, but now they create scan
  cost;
- the user should think "room authoring", not five product room editor modules.

DoD:

- source file count decreases;
- editor cursor/action/overlay behavior remains identical;
- edit/save/exit/Continue proof still passes;
- AppShell calls only room-authoring domain APIs for room editor operations.

### product/Projection

Likely fold together:

- `ProductPrimitiveDrawList.*`
- `ProductRenderBridge.*`
- `ProductViewportFraming.*`
- `ProductViewportState.hpp`
- `ProductMovementDebugHud.*`

Deferred or special-case:

- `ProductNpcBehaviorDebugHud.*` is projection/debug shaped, but no NPC behavior
  work should be done now. Leave it alone unless a projection-only move is
  explicitly approved.

Why:

- these files are one projection proof family;
- Vulkan backend files should not absorb product proof logic;
- AppShell should call one projection pipeline and get one proof result.

DoD:

- AppShell loses projection field-copy code;
- draw-list/render-bridge receipt fields remain stable;
- room mesh CPU proof remains stable;
- Vulkan backend allocation/upload code is not changed.

### product/Save

Keep as durable low-level owners:

- `SaveBridge.*`
- `ProductSaveCatalog.*`

Split or move out of:

- `ProductAppOperations.*`

Why:

- save/load primitives already exist;
- AppShell still owns too many save decisions;
- `ProductAppOperations` is mixed and should either shrink or split into
  durable domain modules.

DoD:

- AppShell no longer branches on Continue/Load/Save/Delete/Recover policy;
- no save schema changes;
- no runtime save codec changes;
- existing save/load/delete/recover smokes pass.

### product/Frontend

Candidate consolidation:

- `ProductFrontendRouter.*`
- `ProductMenuTransitions.*`
- `FrontendActionExecutor.*`
- small frontend model helpers where they are not reusable enough to justify
  their own file.

Keep separate if still genuinely reusable:

- `SaveBrowser.*`
- `WorldSetupModel.*`
- `SettingsMenu.*`
- `DevToolsMenu.*`

Why:

- menu/screen routing belongs together;
- not every small frontend model should be merged immediately if it is a real
  pure model with its own tests.

DoD:

- AppShell loses selected-action corridors;
- pure frontend model tests remain focused;
- no save/write/session side effects enter frontend model files.

### product/Automation

Likely fold together:

- `ProductAutomationCommandRegistry.*`
- new parser/dispatch code.

Why:

- automation keys are a proof/control API, not product behavior;
- AppShell should not know individual automation key strings.

DoD:

- AppShell loses individual automation key branches;
- valid command file parse failures versus command failures remain distinct;
- no-window smoke proofs remain stable.

## What Not To Consolidate Aggressively

Do not flatten these just to reduce file count:

- runtime save codec/store/load files;
- Vulkan backend allocation/upload/swapchain safety files;
- parser edge files with real syntax/error ownership;
- runtime movement/combat/session systems;
- test files that are already focused and readable.

File count reduction is not allowed to erase ownership boundaries.

## Aggressive Slice Order

### Slice 0 - Audit Tool And Consolidation Contract

Allowed files:

- `tools/audit_product_module_shape.py`;
- this plan document;
- `product_module_taxonomy_contract_v0_1.md`;
- plan-bucket README.

DoD:

- script runs without changing source;
- doc records target domains and first consolidation order;
- taxonomy contract records folder/name rules before code moves;
- no source/test/build edits.

### Slice 1 - product/Save Extraction Without New File Explosion

Allowed files:

- existing save/product operation files;
- one consolidated `product/Save.hpp/.cpp` if needed;
- AppShell call-site reduction;
- focused save smokes/tests.

DoD:

- AppShell loses pause Save/Save And Exit and Continue/Load Save policy;
- file count does not increase by more than one pair;
- `ProductAppOperations` shrinks or is split with a clear deletion path;
- no save schema/runtime changes.

### Slice 2 - product/Automation Consolidation

Allowed files:

- `product/Automation.*` or existing registry renamed/expanded;
- AppShell command dispatch removal;
- automation tests/smokes.

DoD:

- command registry/parser/dispatch live in one domain module;
- AppShell does not contain direct automation key corridors;
- no new one-shot automation controller/parser pair unless they merge into the
  domain before acceptance.

### Slice 3 - product/Room Consolidation

Allowed files:

- current room editor/authoring files;
- AppShell room editor call sites;
- focused room authoring tests/smokes.

DoD:

- cursor/action/overlay/edit-state files are consolidated or have a scheduled
  deletion path;
- AppShell no longer applies room edit commands directly;
- edit/save/continue proof remains stable.

### Slice 4 - product/RoomPipeline Consolidation

Allowed files:

- ASCII/source/grid/conversion/active-room files;
- map-generation adapter files only if they merge into the same domain;
- ASCII/map tests.

DoD:

- fewer adapter files;
- one pipeline API for map text/generated map -> authored/editable/room asset;
- ASCII remains layout only;
- map generation has a clear entry point without adding another adapter chain.

### Slice 5 - product/Projection Consolidation

Allowed files:

- product projection/draw/render proof files;
- AppShell projection refresh call sites;
- projection/render bridge tests.

DoD:

- AppShell loses projection metric copying;
- product projection module owns draw-list/frame/bridge proof assembly;
- Vulkan backend remains isolated.

### Slice 6 - product/Frontend Consolidation

Allowed files:

- frontend router/menu transition/action executor files;
- AppShell menu routing call sites;
- frontend/router tests/smokes.

DoD:

- AppShell does not own selected-action menu policy;
- pure models remain pure;
- Save/World/Room flows delegate to their domain modules.

### Slice 7 - AppShell File Budget Gate

Allowed files:

- branch gate tools/docs;
- AppShell shrink proof.

DoD:

- `AppShell.cpp` line-count budget is set;
- direct key corridors are blocked;
- direct save/load corridors are blocked;
- direct room edit command application is blocked;
- new one-shot product file creation requires explicit approval.

## No-New-One-Shot Rule

New product source file pairs are rejected unless at least one is true:

- they replace several existing files;
- they are a durable domain module named in this plan;
- they isolate backend/platform code;
- they are a pure model with independent reuse and a clear test target.

Rejected names by default:

- `ProductHelpers.*`
- `ProductUtils.*`
- `ProductCommon.*`
- `FlowManager.*`
- feature-specific controller files that do not replace or merge existing files.

## Token-Saving Workflow

Use the audit script to refresh evidence:

```bash
tools/audit_product_module_shape.py --top 20
```

Chat reports should include only:

- top changed recommendation;
- file-count/AppShell LOC delta;
- tests run;
- blocker if any.

Do not paste full audit tables into chat. Put full tables in generated files or
review docs when needed.

## Acceptance Gate

The consolidation is working when:

- `AppShell.cpp` is lifecycle sequencing, not product policy;
- product file count trends downward;
- no tiny product feature file is added without approval;
- save/load policy is not visible in AppShell;
- room authoring and room pipeline are separate durable domains;
- map generation plugs into room pipeline instead of creating parallel one-shot
  conversion files;
- branch gate catches new AppShell branch corridors;
- no source behavior drift is accepted without a named repair note.

## Stop Rules

Stop a consolidation slice if:

- it increases file count without deleting or scheduling deletion of older
  files;
- it hides multiple unrelated domains inside a junk drawer module;
- it changes save/load schema;
- it changes room geometry output without render proof;
- it touches NPC behavior;
- it restores visual-demo or package-visual work;
- it requires broad test loops to gain confidence.
