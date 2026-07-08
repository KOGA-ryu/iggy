# E209: Operations Seam Split Preflight

## Status

Done.

## Context

`src/app/iggy3d/Operations.cpp` is still a broad product-app catch-all after
the god-struct and identity-mirror cleanup. The complexity audit calls out
`Operations.cpp` as a mixed-domain file and says to split it only after the
active-creative identity mirror work was removed. That prerequisite is now
complete.

This is a read-only preflight. Do not move code yet. The goal is to produce the
small implementation cards that can safely split `Operations.cpp` by seam.

Current rough size:

- `src/app/iggy3d/Operations.cpp`: about 1.6k lines
- `src/app/iggy3d/Operations.hpp`: about 200 lines

Known candidate seams visible from the public API and current implementation:

- save slot / save browser / delete / recover flow
- package/session bootstrap and gameplay launch
- creative new/open/save world operations
- creative baked active-room refresh service
- world-template / ASCII package helpers

## Objective

Audit `Operations.cpp` and `Operations.hpp`, classify their functions into
cohesive seams, and draft small follow-up implementation cards. The output
should let reviewer release one narrow move at a time instead of giving builder
a large refactor.

## Scope

Read-only. Do not edit source, tests, CMake, fixtures, receipt golden, or
production docs.

Allowed file move/edit:

- this task card only

Inspect at minimum:

- `src/app/iggy3d/Operations.cpp`
- `src/app/iggy3d/Operations.hpp`
- direct call sites for public Operations APIs under `src/app/iggy3d` and
  `tests/unit`
- `docs/complexity_audit_v0_1.md`
- `docs/creative_mode/builder_tasks/PRIORITY.md`

## Required Work

1. Inventory public declarations in `Operations.hpp` and top-level helper /
   function definitions in `Operations.cpp`.
2. Group each public API and major internal helper into one proposed owner
   seam. Use existing naming and behavior, not aspirational architecture.
3. For each seam, report:
   - candidate new file/header name;
   - public APIs it would own;
   - private helpers it would move with those APIs;
   - expected direct callers;
   - focused tests that should guard the move.
4. Identify the safest first implementation slice. Prefer a seam with:
   - cohesive public API;
   - small caller set;
   - low risk of circular includes;
   - focused tests already present.
5. Identify explicit non-slices that should not move yet.
6. Draft follow-up implementation cards, each small enough to be reviewed
   independently. At minimum, draft the first card in full.

## Suggested Commands

Run current inventories such as:

```sh
wc -l /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.hpp
rg -n "^[A-Za-z_][A-Za-z0-9_:<>]*[&*[:space:]]+[A-Za-z_][A-Za-z0-9_:]*\\(" /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.hpp
rg -n "writeProductCurrentSessionSave|openDeletedProductSaveBrowser|executeProductSaveRecover|openProductSaveDeleteConfirmation|executeProductSaveSoftDelete|launchProductNewWorld|launchProductCreativeNewWorld|launchProductCreativeOpenWorld|saveProductCurrentCreativeWorld|refreshProductCreativeBakedActiveRoom|launchProductContinueSave|launchProductLoadSaveSelection" /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'
git -C /Users/kogaryu/iggy3d diff --check
```

Add narrower `nl -ba`, `sed`, and `rg` reads as needed to map helper clusters
and call sites.

## Decision Requirements

Report one of:

- **Slice plan ready:** include ordered implementation cards with file names,
  moved APIs/helpers, expected tests, and self-blockers.
- **Blocked:** explain the exact coupling or missing tests that prevent a safe
  first move.

## Completion Brief Requirements

Report:

- card moved to done;
- files inspected;
- function/API inventory summary;
- seam grouping table;
- direct call-site hotspots by seam;
- recommended first implementation slice;
- follow-up card drafts;
- non-slices/deferred work;
- `diff --check` result;
- confirmation that no source, test, CMake, fixture, receipt golden,
  production docs, staging, commit, push, or window launch was performed.

## Completion Brief

- Card moved to done:
  `/Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/done/E209-operations-seam-split-preflight.md`
- Files inspected:
  - `src/app/iggy3d/Operations.cpp`
  - `src/app/iggy3d/Operations.hpp`
  - `src/app/iggy3d/AppKernel.cpp`
  - `src/app/iggy3d/menu/ActionHandlers.cpp`
  - `src/app/iggy3d/automation/AutomationSaveBrowser.cpp`
  - `src/app/iggy3d/window/InputFrame.cpp`
  - `src/app/iggy3d/save/Flow.hpp`
  - `src/app/iggy3d/save/Flow.cpp`
  - focused callers under `tests/unit/product_*`
  - `docs/complexity_audit_v0_1.md`
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
  - `cmake/iggy3d_tests.cmake`
- Current size:
  - `Operations.cpp`: 1684 lines
  - `Operations.hpp`: 205 lines

### Function/API Inventory Summary

- Public result/request types in `Operations.hpp`:
  - `ProductSaveFlowOperation`, `ProductSaveFlowRequest`,
    `ProductSaveFlowResult`
  - `ProductCreativeNewWorldLaunchRequest`,
    `ProductCreativeNewWorldLaunchResult`
  - `ProductCreativeOpenWorldLaunchRequest`,
    `ProductCreativeOpenWorldLaunchResult`
  - `ProductCreativeCurrentWorldSaveResult`
- Public APIs in `Operations.hpp`:
  - Save/browser/delete/recover:
    `productSaveFlowOperationName`, `writeProductCurrentSessionSave`,
    `initializeSelectedProductSaveSlot`, `moveSelectedProductSaveSlot`,
    `selectProductSaveSlotById`, `scanDeletedProductSavesForOptions`,
    `recordDeletedProductSaveSlots`, `selectDeletedProductSaveSlotById`,
    `openDeletedProductSaveBrowser`, `executeProductSaveRecover`,
    `openProductSaveDeleteConfirmation`,
    `cancelProductSaveDeleteConfirmation`,
    `executeProductSaveSoftDelete`
  - World template/session/gameplay launch:
    `productWorldTemplateFromOptions`, `launchProductNewWorld`,
    `launchProductContinueSave`, `launchProductLoadSaveSelection`
  - Creative world lifecycle:
    `launchProductCreativeNewWorld`, `launchProductCreativeOpenWorld`,
    `saveProductCurrentCreativeWorld`
  - Creative baked-room refresh:
    `refreshProductCreativeBakedActiveRoom`
- Major private helper clusters in `Operations.cpp`:
  - Package/session bootstrap: `packageLoadStatusName`,
    `elapsedMicroseconds`, `defaultProductPackagePath`,
    `createProductSessionFromPackage`, `createProductSession`,
    `createCreativeBlankSession`, `clearProductGameplayLaunchState`
  - World/ascii creation: `productAsciiRoomAuthoringRequestFromWorldSetup`,
    `frameCreativeStageCameraOnOrigin`,
    `prepareProductWorldCreationFromDraft`,
    `recordProductWorldInitialSaveResult`
  - Save-slot/save-flow recording: `recordProductSaveWriteResult`,
    `recordProductSaveLoadSelection`, `saveSlotById`,
    `firstSelectableSaveSlot`, `recordSelectedProductSaveSlot`,
    `recordProductSaveSlotAction`, `recordProductSaveFlowRequest`,
    `recordProductSaveFlowResult`,
    `recordSelectedDeletedProductSaveSlot`,
    `initializeSelectedDeletedProductSaveSlot`
  - Save load/bind recording: `recordProductSaveLoadResult`,
    `recordSavedRoomMarkerBindingResult`
  - Creative world result mirroring/identity:
    `setCreativeNewWorldLaunchStatus`, `setCreativeOpenWorldLaunchStatus`,
    `setCurrentCreativeSaveStatus`, `idOrNone`, `pathOrNone`,
    `missingWindowIdentity`, `recordActiveCreativeSaveIdentity`,
    `recordActiveCreativeSaveResult`,
    `mirrorCreativeWorldCreateResult`, `mirrorCreativeWorldOpenResult`,
    `mirrorCreativeDocumentInstallResult`,
    `mirrorCreativeBakedActiveRoomRefreshResult`
  - Baked room refresh service:
    `setCreativeBakedActiveRoomRefreshStatus`, `fallbackString`,
    `clearedCreativeBakedActiveRoom`,
    `ProductCreativeBakedRoomRefreshService`
  - Private load launcher:
    `launchProductSaveSlot`

### Seam Grouping Table

| Seam | Candidate files | Public APIs owned | Private helpers moved with it | Expected direct callers | Focused tests |
| --- | --- | --- | --- | --- | --- |
| Creative baked active-room refresh | `src/app/iggy3d/creative/BakedActiveRoomRefresh.hpp/.cpp` or `src/app/iggy3d/creative/BakedActiveRoomRefreshOperations.hpp/.cpp` | `refreshProductCreativeBakedActiveRoom` | `kCreativeRoomBakeNoRenderableObjects`, `kProductCreativeBakedRoomClearedNoRenderableObjects`, `setCreativeBakedActiveRoomRefreshStatus`, `fallbackString`, `clearedCreativeBakedActiveRoom`, `ProductCreativeBakedRoomRefreshService`; add a file-local elapsed-microseconds helper or extract a tiny timing helper only if needed | `Operations.cpp` launch/open paths, `window/InputFrame.cpp`, `tests/unit/product_creative_world_launch_tests.cpp`, `tests/unit/product_creative_no_window_bake_scenario_tests.cpp` | `product_creative_world_launch_tests`, `product_creative_no_window_bake_scenario_tests`, `product_creative_ui_input_frame_tests`, `product_receipt_key_order_tests` |
| Creative world lifecycle | `src/app/iggy3d/creative/CreativeWorldOperations.hpp/.cpp` | `ProductCreativeNewWorldLaunchRequest/Result`, `ProductCreativeOpenWorldLaunchRequest/Result`, `ProductCreativeCurrentWorldSaveResult`, `launchProductCreativeNewWorld`, `launchProductCreativeOpenWorld`, `saveProductCurrentCreativeWorld` | creative status setters, `frameCreativeStageCameraOnOrigin`, active-creative identity/save result helpers, create/open/install mirror helpers, baked refresh mirror helpers; depends on creative blank session helper until session seam is split | `menu/ActionHandlers.cpp`, `save/Flow.cpp`, `tests/unit/product_creative_world_launch_tests.cpp`, `tests/unit/product_creative_no_window_bake_scenario_tests.cpp`, automation creative save/new/open paths indirectly through `ActionHandlers` | `product_creative_world_launch_tests`, `product_creative_no_window_bake_scenario_tests`, `product_starter_menu_action_tests`, `product_automation_dispatch_tests`, `product_receipt_key_order_tests` |
| Save-slot browser/delete/recover | `src/app/iggy3d/save/SaveSlotOperations.hpp/.cpp` | `ProductSaveFlowOperation/Request/Result`, `productSaveFlowOperationName`, `initializeSelectedProductSaveSlot`, `moveSelectedProductSaveSlot`, `selectProductSaveSlotById`, `scanDeletedProductSavesForOptions`, `recordDeletedProductSaveSlots`, `selectDeletedProductSaveSlotById`, `openDeletedProductSaveBrowser`, `executeProductSaveRecover`, `openProductSaveDeleteConfirmation`, `cancelProductSaveDeleteConfirmation`, `executeProductSaveSoftDelete` | `saveSlotById`, `firstSelectableSaveSlot`, `recordSelectedProductSaveSlot`, `recordProductSaveSlotAction`, `recordProductSaveFlowRequest`, `recordProductSaveFlowResult`, `recordSelectedDeletedProductSaveSlot`, `initializeSelectedDeletedProductSaveSlot`; either keep calling `productWorldTemplateFromOptions` through a small include or split world-template first | `menu/ActionHandlers.cpp`, `automation/AutomationSaveBrowser.cpp`, `window/InputFrame.cpp`, `tests/unit/product_save_delete_executor_tests.cpp` | `product_save_delete_executor_tests`, `product_starter_menu_action_tests`, `product_window_input_frame_tests`, `product_automation_dispatch_tests`, `product_save_catalog_tests`, `product_save_bridge_tests`, `product_receipt_key_order_tests` |
| Current session save/write | `src/app/iggy3d/save/CurrentSessionSave.hpp/.cpp` or merge into `save/Flow.cpp` later | `writeProductCurrentSessionSave` | `recordProductSaveWriteResult`; can move only after deciding whether save flow should depend on this helper directly or call lower-level durable writer | `save/Flow.cpp` only for production; tests indirectly through starter/world/save paths | `product_starter_menu_action_tests`, `product_creative_world_launch_tests`, `product_save_bridge_tests`, `product_receipt_key_order_tests` |
| Package/session bootstrap + load-save launch | `src/app/iggy3d/world/ProductSessionLaunch.hpp/.cpp` or `src/app/iggy3d/gameplay/ProductSessionLaunch.hpp/.cpp` | `launchProductContinueSave`, `launchProductLoadSaveSelection`; possibly `productWorldTemplateFromOptions` | `packageLoadStatusName`, `elapsedMicroseconds`, `defaultProductPackagePath`, `createProductSessionFromPackage`, `createProductSession`, `recordProductSaveLoadSelection`, `recordProductSaveLoadResult`, `recordSavedRoomMarkerBindingResult`, `clearProductGameplayLaunchState`, `launchProductSaveSlot` | `AppKernel.cpp`, `menu/ActionHandlers.cpp`, `tests/unit/product_window_input_frame_tests.cpp`, `tests/unit/product_creative_world_launch_tests.cpp` | `product_starter_menu_action_tests`, `product_window_input_frame_tests`, `product_creative_world_launch_tests`, `product_frontend_router_tests`, `product_receipt_key_order_tests` |
| Product new-world + ASCII authored-room launch | `src/app/iggy3d/world/ProductNewWorldLaunch.hpp/.cpp` or `src/app/iggy3d/ascii_room/ProductAsciiWorldLaunch.hpp/.cpp` | `launchProductNewWorld` | `productAsciiRoomAuthoringRequestFromWorldSetup`, `prepareProductWorldCreationFromDraft`, `recordProductWorldInitialSaveResult`; depends on package/session bootstrap helpers and save write helpers | `AppKernel.cpp`, `menu/ActionHandlers.cpp`, `tests/unit/product_window_input_frame_tests.cpp`, `tests/unit/product_creative_world_launch_tests.cpp`, `tests/unit/product_starter_menu_action_tests.cpp` | `product_starter_menu_action_tests`, `product_window_input_frame_tests`, `product_creative_world_launch_tests`, `product_new_world_menu_action_tests`, `product_ascii_room_activation_tests`, `product_ascii_package_smoke`, `product_receipt_key_order_tests` |
| World-template/package path helper | `src/app/iggy3d/world/ProductWorldTemplateOperations.hpp/.cpp` | `productWorldTemplateFromOptions` | `defaultProductPackagePath`; consider later whether `packageLoadStatusName` belongs to session launch, not this helper | `AppKernel.cpp`, `menu/ActionHandlers.cpp`, `window/InputFrame.cpp`, `Operations.cpp` save and launch internals, `tests/unit/product_window_input_frame_tests.cpp`, `tests/unit/product_save_delete_executor_tests.cpp` | `product_save_delete_executor_tests`, `product_starter_menu_action_tests`, `product_window_input_frame_tests`, `product_creative_world_launch_tests`, `product_receipt_key_order_tests` |

### Direct Call-Site Hotspots By Seam

- `Operations.hpp` direct include users today:
  - Production: `AppKernel.cpp`, `menu/ActionHandlers.cpp`,
    `automation/AutomationSaveBrowser.cpp`, `window/InputFrame.cpp`,
    `save/Flow.hpp`, `save/Flow.cpp`
  - Tests: `product_creative_world_launch_tests.cpp`,
    `product_creative_no_window_bake_scenario_tests.cpp`,
    `product_creative_ui_frame_tests.cpp`,
    `product_creative_ui_input_frame_tests.cpp`,
    `product_creative_viewport_pick_frame_tests.cpp`,
    `product_frontend_router_tests.cpp`,
    `product_save_delete_executor_tests.cpp`,
    `product_starter_menu_action_tests.cpp`,
    `product_window_input_frame_tests.cpp`
- Save-slot/delete/recover hot spots:
  - `menu/ActionHandlers.cpp`: continue/load/new/open/save/delete command
    routing; calls `initializeSelectedProductSaveSlot`,
    `productWorldTemplateFromOptions`, `launchProductContinueSave`,
    `launchProductCreativeNewWorld`, `launchProductCreativeOpenWorld`,
    `executeProductSaveSoftDelete`,
    `openProductSaveDeleteConfirmation`,
    `cancelProductSaveDeleteConfirmation`,
    `moveSelectedProductSaveSlot`, `launchProductLoadSaveSelection`,
    `launchProductNewWorld`
  - `automation/AutomationSaveBrowser.cpp`: calls
    `selectProductSaveSlotById`, `openProductSaveDeleteConfirmation`,
    `openDeletedProductSaveBrowser`, `scanDeletedProductSavesForOptions`,
    `recordDeletedProductSaveSlots`,
    `selectDeletedProductSaveSlotById`, `executeProductSaveRecover`
  - `window/InputFrame.cpp`: calls `selectProductSaveSlotById` and
    `refreshProductCreativeBakedActiveRoom`
  - `save/Flow.cpp`: calls `saveProductCurrentCreativeWorld` and
    `writeProductCurrentSessionSave`
- Creative baked-room refresh hot spots:
  - `window/InputFrame.cpp`: manual rebuild and mutation-time auto-refresh
  - `Operations.cpp`: creative launch/open accepted refresh
  - `product_creative_world_launch_tests.cpp`: direct helper tests for
    accepted, rejected, no-renderable clear, activation hook, and failure paths
  - `product_creative_no_window_bake_scenario_tests.cpp`: direct no-window
    scenario refresh
- Creative world lifecycle hot spots:
  - `menu/ActionHandlers.cpp`: creative new/open actions
  - `save/Flow.cpp`: creative save path
  - `product_creative_world_launch_tests.cpp`: primary direct coverage
  - `product_creative_no_window_bake_scenario_tests.cpp`: launch + bake
    scenario coverage
- Product new/load/continue session launch hot spots:
  - `AppKernel.cpp`: startup world template and auto-new-world launch
  - `menu/ActionHandlers.cpp`: continue/load/create routes
  - `product_window_input_frame_tests.cpp`, `product_starter_menu_action_tests.cpp`,
    `product_creative_world_launch_tests.cpp`

### Recommended First Implementation Slice

**Slice plan ready.** Start with the creative baked active-room refresh service
extraction.

Reasoning:

- The refresh service is already a cohesive private class in
  `Operations.cpp:584-741` plus a single public wrapper at
  `Operations.cpp:1544-1562`.
- It does not own save browser, package loader, frontend transition, or save
  format policy.
- It already has focused direct tests in
  `product_creative_world_launch_tests` and
  `product_creative_no_window_bake_scenario_tests`, plus input-frame tests that
  exercise auto/manual refresh through production routing.
- It can move without deciding the larger `ProductWorldTemplate` /
  `createProductSession` coupling.
- It removes one of the most self-contained behavior services from the mixed
  Operations file while preserving all current launch/open/manual/auto refresh
  behavior.

### Follow-Up Card Drafts

#### E210 Draft: Extract Creative Baked Active-Room Refresh Service

Goal:

- Move `refreshProductCreativeBakedActiveRoom(...)` and its private service
  implementation out of `Operations.cpp` into a creative-owned service file,
  without changing refresh behavior, receipt fields, or callers.

Scope:

- Add:
  - `src/app/iggy3d/creative/BakedActiveRoomRefresh.hpp`
  - `src/app/iggy3d/creative/BakedActiveRoomRefresh.cpp`
- Edit only as needed:
  - `src/app/iggy3d/Operations.hpp`
  - `src/app/iggy3d/Operations.cpp`
  - direct production/test include fallout for callers of
    `refreshProductCreativeBakedActiveRoom(...)`
  - focused tests only if include fallout requires it
- Do not move creative world launch/open/save, package/session bootstrap, save
  slot operations, RoomBake policy, receipt fields, CMake unless required for
  new source registration, renderer/window behavior, save/load format, or
  ProductAppWindowState storage.

Implementation:

- Move from `Operations.cpp`:
  - `kCreativeRoomBakeNoRenderableObjects`
  - `kProductCreativeBakedRoomClearedNoRenderableObjects`
  - `setCreativeBakedActiveRoomRefreshStatus`
  - `fallbackString`
  - `clearedCreativeBakedActiveRoom`
  - `ProductCreativeBakedRoomRefreshService`
  - `refreshProductCreativeBakedActiveRoom`
- Preserve activation hook defaulting to `activateCreativeReasoningGraph`.
- Preserve `clearOnNoRenderable` behavior, stale-clearing behavior, collision
  refresh behavior, timing diagnostics, and accepted/rejected status strings.
- Prefer updating direct callers (`InputFrame.cpp`, `Operations.cpp`, focused
  tests) to include the new header. If too many callers still include
  `Operations.hpp` only, a transitional include from `Operations.hpp` is
  acceptable but should be reported.

Self-blockers:

- Moving the service requires changing refresh semantics, RoomBake policy,
  receipt/golden fields, or session lifecycle policy.
- CMake/source registration fallout expands beyond adding the one new `.cpp`.
- Tests reveal launch/open/manual/auto refresh drift.

Verification:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_world_launch_tests product_creative_no_window_bake_scenario_tests product_creative_ui_input_frame_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_creative_no_window_bake_scenario_tests|product_creative_ui_input_frame_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

#### E211 Draft: Extract Save-Slot Browser/Delete/Recover Operations

Goal:

- Move save-slot selection, deleted-save browser, recover, and soft-delete flow
  operations into `src/app/iggy3d/save/SaveSlotOperations.hpp/.cpp`.

Scope:

- Move public types/APIs:
  - `ProductSaveFlowOperation`, `ProductSaveFlowRequest`,
    `ProductSaveFlowResult`
  - `productSaveFlowOperationName`
  - `initializeSelectedProductSaveSlot`, `moveSelectedProductSaveSlot`,
    `selectProductSaveSlotById`
  - `scanDeletedProductSavesForOptions`, `recordDeletedProductSaveSlots`,
    `selectDeletedProductSaveSlotById`, `openDeletedProductSaveBrowser`,
    `executeProductSaveRecover`
  - `openProductSaveDeleteConfirmation`,
    `cancelProductSaveDeleteConfirmation`,
    `executeProductSaveSoftDelete`
- Move private helpers:
  - `saveSlotById`, `firstSelectableSaveSlot`,
    `recordSelectedProductSaveSlot`, `recordProductSaveSlotAction`,
    `recordProductSaveFlowRequest`, `recordProductSaveFlowResult`,
    `recordSelectedDeletedProductSaveSlot`,
    `initializeSelectedDeletedProductSaveSlot`
- Keep `writeProductCurrentSessionSave`, load/continue launch, and world
  creation in place.
- If `productWorldTemplateFromOptions` dependency makes this non-local, stop
  and split the world-template helper first.

Tests:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_save_delete_executor_tests product_starter_menu_action_tests product_window_input_frame_tests product_automation_dispatch_tests product_save_catalog_tests product_save_bridge_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_save_delete_executor_tests|product_starter_menu_action_tests|product_window_input_frame_tests|product_automation_dispatch_tests|product_save_catalog_tests|product_save_bridge_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

#### E212 Draft: Extract Creative World Launch/Open/Save Operations

Goal:

- Move creative world new/open/save result types and operations into
  `src/app/iggy3d/creative/CreativeWorldOperations.hpp/.cpp`, after the baked
  refresh service is already extracted.

Scope:

- Move public types/APIs:
  - `ProductCreativeNewWorldLaunchRequest/Result`
  - `ProductCreativeOpenWorldLaunchRequest/Result`
  - `ProductCreativeCurrentWorldSaveResult`
  - `launchProductCreativeNewWorld`, `launchProductCreativeOpenWorld`,
    `saveProductCurrentCreativeWorld`
- Move private helpers:
  - creative status setters, active creative identity/save result helpers,
    create/open/install/refresh mirror helpers,
    `frameCreativeStageCameraOnOrigin`
- Keep `createCreativeBlankSession` in Operations/session launch for this slice
  unless moving it is proven non-circular.

Tests:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_world_launch_tests product_creative_no_window_bake_scenario_tests product_starter_menu_action_tests product_automation_dispatch_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_world_launch_tests|product_creative_no_window_bake_scenario_tests|product_starter_menu_action_tests|product_automation_dispatch_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

#### E213 Draft: Preflight Product Session Launch / World Template Coupling

Goal:

- Do a read-only preflight before moving package/session bootstrap,
  `productWorldTemplateFromOptions`, `launchProductContinueSave`,
  `launchProductLoadSaveSelection`, or `launchProductNewWorld`.

Reason:

- These functions are coupled through `defaultProductPackagePath`,
  package load status recording, active-room/collision freshness, saved-room
  marker binding, and frontend transitions.
- A read-only preflight should decide whether `productWorldTemplateFromOptions`
  moves first or stays with session launch.

### Non-Slices / Deferred Work

- Do not extract `CreativeIdentityMirror`: that complexity-audit slice is stale
  because the active-creative identity mirror work has already been removed.
- Do not move `AppKernel.hpp`, `window/Loop.hpp`, or app-lifetime ownership
  while splitting `Operations.cpp`.
- Do not split package/session bootstrap and new-world launch in the same
  implementation card; their shared helpers and frontend state writes make the
  combined change too hard to review.
- Do not move save/load format, save bridge durable writer/parser, RoomBake
  policy, renderer/Vulkan/projection, or ProductAppWindowState storage as part
  of this lane.
- Do not use this seam split to rename status/reason strings or receipt keys.

### Decision

- **Slice plan ready.** Release E210 first: extract the creative baked
  active-room refresh service.

### Checks

- Passed: `git -C /Users/kogaryu/iggy3d diff --check`
- No build/CTest was run because this was a read-only audit.
- Confirmation:
  - No source, test, CMake, fixture, receipt golden, production docs, staging,
    commit, push, or window launch was performed.
