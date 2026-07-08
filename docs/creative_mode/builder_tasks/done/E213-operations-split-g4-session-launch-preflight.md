# E213 — Operations Split G4: Session Launch Preflight

## Status

Done.

## Context

E209 preflighted the `Operations.cpp` seam split. E210 extracted creative baked
active-room refresh, E211 extracted save-slot browser/delete/recover operations,
and E212 extracted creative world launch/open/save operations.

After E212, `Operations.cpp` is much smaller, but the remaining public API is
still mixed:

- world-template/package path resolution;
- current-session save/write receipt mirroring;
- product new-world creation and ASCII authoring launch;
- product continue/load save launch;
- package/session bootstrap;
- narrow creative blank-stage wrappers that were exposed for E212.

This card is read-only. Do not move source code yet. The goal is to decide the
next safe implementation slice.

## Objective

Audit the remaining `Operations.cpp`/`Operations.hpp` seams and produce a
specific next-card recommendation for product session/world launch extraction.

At the start of this card, record the current sizes of:

- `src/app/iggy3d/Operations.cpp`
- `src/app/iggy3d/Operations.hpp`

## Inspect

Inspect:

- `src/app/iggy3d/Operations.cpp`
- `src/app/iggy3d/Operations.hpp`
- `src/app/iggy3d/AppKernel.cpp`
- `src/app/iggy3d/menu/ActionHandlers.cpp`
- `src/app/iggy3d/save/Flow.hpp`
- `src/app/iggy3d/save/Flow.cpp`
- `src/app/iggy3d/save/SaveSlotOperations.cpp`
- `src/app/iggy3d/creative/CreativeWorldOperations.cpp`
- `src/app/iggy3d/ascii_room/Activation.cpp`
- `src/app/iggy3d/world/Creation.hpp/.cpp`
- `src/app/iggy3d/world/PackageSessionSeed.hpp/.cpp`
- focused callers/tests for the remaining public Operations APIs

## Classify Remaining Public APIs

Classify each remaining public API in `Operations.hpp`:

- `productWorldTemplateFromOptions(...)`
- `writeProductCurrentSessionSave(...)`
- `createCreativeBlankSession(...)`
- `frameCreativeStageCameraOnOrigin(...)`
- `clearProductGameplayLaunchState(...)`
- `launchProductNewWorld(...)`
- `launchProductContinueSave(...)`
- `launchProductLoadSaveSelection(...)`

For each one, report:

- owner candidate;
- direct production callers;
- direct test callers;
- private helpers it needs;
- whether it is safe for a next implementation card;
- why it should or should not remain in `Operations.cpp`.

## Classify Private Helper Clusters

Classify the remaining private helper clusters in `Operations.cpp`:

- package path/status helpers:
  - `packageLoadStatusName(...)`
  - `elapsedMicroseconds(...)`
  - `defaultProductPackagePath(...)`
- product package/session bootstrap:
  - `createProductSessionFromPackage(...)`
  - `createProductSession(...)`
- creative blank-stage wrappers:
  - `createCreativeBlankSessionImpl(...)`
  - `frameCreativeStageCameraOnOriginImpl(...)`
  - `clearProductGameplayLaunchStateImpl(...)`
- product new-world/ascii launch:
  - `productAsciiRoomAuthoringRequestFromWorldSetup(...)`
  - `prepareProductWorldCreationFromDraft(...)`
  - `recordProductWorldInitialSaveResult(...)`
  - `recordSavedRoomMarkerBindingResult(...)`
  - `launchProductNewWorld(...)`
- current-session save/write:
  - `recordProductSaveWriteResult(...)`
  - `writeProductCurrentSessionSave(...)`
- save load/continue launch:
  - `recordProductSaveLoadSelection(...)`
  - `recordProductSaveLoadResult(...)`
  - `saveSlotById(...)`
  - `launchProductSaveSlot(...)`
  - `launchProductContinueSave(...)`
  - `launchProductLoadSaveSelection(...)`

## Decision Questions

Answer these concretely:

1. Should the next implementation extract product save/load launch into a
   `ProductSessionLaunch` seam, or is a smaller pre-step needed?
2. Should `productWorldTemplateFromOptions(...)` move with save/load launch,
   with product new-world launch, or into a tiny `ProductWorldTemplateOperations`
   seam?
3. Should `writeProductCurrentSessionSave(...)` move alone before save/load
   launch, or stay until save-flow coupling is clearer?
4. Should the E212 creative blank-stage wrappers remain public in
   `Operations.hpp`, move to a creative/product session launch seam, or get a
   dedicated narrower header?
5. Is it safe to extract `launchProductNewWorld(...)` together with save/load
   launch, or should product new-world/ascii authoring stay separate?

## Required Greps

Run and summarize:

```sh
wc -l /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.hpp

rg -n "productWorldTemplateFromOptions|writeProductCurrentSessionSave|createCreativeBlankSession|frameCreativeStageCameraOnOrigin|clearProductGameplayLaunchState|launchProductNewWorld|launchProductContinueSave|launchProductLoadSaveSelection|launchProductSaveSlot|createProductSession|createProductSessionFromPackage|prepareProductWorldCreationFromDraft|recordProductSaveWriteResult|recordProductSaveLoadSelection|recordProductSaveLoadResult|defaultProductPackagePath" \
  /Users/kogaryu/iggy3d/src/app/iggy3d \
  /Users/kogaryu/iggy3d/tests/unit \
  --glob '*.cpp' --glob '*.hpp'

rg -n "#include \"app/iggy3d/Operations.hpp\"" \
  /Users/kogaryu/iggy3d/src/app/iggy3d \
  /Users/kogaryu/iggy3d/tests/unit \
  --glob '*.cpp' --glob '*.hpp'
```

Also inspect enough line ranges with `sed`/`nl` to classify call sites and
helper ownership.

## Deliverable

Append a completion brief to this card with:

- current `Operations.cpp/.hpp` sizes;
- remaining public API classification table;
- private helper cluster classification;
- direct caller counts and hottest caller files;
- recommended next implementation card title and scope;
- explicit non-scope for that next card;
- focused verification commands for that next card;
- self-blockers for that next card.

If the safest next step is another read-only preflight rather than an
implementation, say that directly and explain why.

## Verification

Because this is read-only:

```sh
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over this card.

No build or CTest is required unless source/test/CMake files are accidentally
edited; if that happens, stop and report.

## Scope Guard

Do not edit:

- source files;
- test files;
- CMake files;
- fixture files;
- receipt golden files;
- production docs outside this task card.

Do not stage, commit, push, or launch a window.

## Completion Brief

- Card moved to done: pending final move after this brief append.
- Files inspected:
  - `src/app/iggy3d/Operations.cpp`
  - `src/app/iggy3d/Operations.hpp`
  - `src/app/iggy3d/AppKernel.cpp`
  - `src/app/iggy3d/menu/ActionHandlers.cpp`
  - `src/app/iggy3d/save/Flow.hpp`
  - `src/app/iggy3d/save/Flow.cpp`
  - `src/app/iggy3d/save/SaveSlotOperations.hpp`
  - `src/app/iggy3d/save/SaveSlotOperations.cpp`
  - `src/app/iggy3d/creative/CreativeWorldOperations.hpp`
  - `src/app/iggy3d/creative/CreativeWorldOperations.cpp`
  - `src/app/iggy3d/ascii_room/Activation.cpp`
  - `src/app/iggy3d/world/Creation.hpp`
  - `src/app/iggy3d/world/Creation.cpp`
  - `src/app/iggy3d/world/PackageSessionSeed.hpp`
  - `src/app/iggy3d/world/PackageSessionSeed.cpp`
  - focused product tests and callers surfaced by the required greps.

### Current Sizes

- `src/app/iggy3d/Operations.cpp`: 705 lines.
- `src/app/iggy3d/Operations.hpp`: 53 lines.
- Current direct `#include "app/iggy3d/Operations.hpp"` count in
  `src/app/iggy3d` plus `tests/unit`: 15 files.

### Remaining Public API Classification

| API | Owner candidate | Direct production callers | Direct test callers | Private helpers needed | Safe next card? | Recommendation |
| --- | --- | --- | --- | --- | --- | --- |
| `productWorldTemplateFromOptions(...)` | New `world/ProductWorldTemplateOperations` seam | `AppKernel.cpp` 1, `ActionHandlers.cpp` 4, `SaveSlotOperations.cpp` 3, `Operations.cpp` 1 internal | `product_window_input_frame_tests.cpp` 1 | `defaultProductPackagePath(...)`, `loadPackage(...)`, `defaultProductWorldTemplate(...)`, `devOverrideProductWorldTemplate(...)` | Yes, safest first | Move first as a tiny pre-step; it is the fan-out keeping save-slot/menu/app startup tied to `Operations.hpp`. |
| `writeProductCurrentSessionSave(...)` | Save-owned `CurrentSessionSaveOperations` or `save/Flow` support seam | `save/Flow.cpp` 1 | No direct test callers found | `recordProductSaveWriteResult(...)`, `activeRoom(window)`, `productSaveTimestampNowUtc()`, `writeProductSessionSaveDurably(...)` | Not first | Leave until the template dependency is gone; then move alone or with pause-save flow cleanup. |
| `createCreativeBlankSession(...)` | Dedicated creative blank-stage/session seam | `CreativeWorldOperations.cpp` 2 | None | `createCreativeBlankSessionImpl(...)`, active-room clear, session seed creation, elapsed timing | Not first | Public in `Operations.hpp` is a temporary E212 bridge; move later to a narrower creative blank-stage header. |
| `frameCreativeStageCameraOnOrigin(...)` | Dedicated creative blank-stage/session seam | `CreativeWorldOperations.cpp` 2 | None | `frameCreativeStageCameraOnOriginImpl(...)`, creative fly anchor/epoch | Not first | Move with creative blank-stage helpers, not with product save/load launch. |
| `clearProductGameplayLaunchState(...)` | Product session launch seam or shared session-state helper | `CreativeWorldOperations.cpp` 2, `Operations.cpp` 3 internal | None | `clearProductGameplayLaunchStateImpl(...)`, active room/collision reset, active session reset | Later | This is shared failure cleanup for creative, product new-world, and save/load. Move only when the session launch seam is ready. |
| `launchProductNewWorld(...)` | Product world/new-world launch seam | `AppKernel.cpp` 1, `ActionHandlers.cpp` 1 | `product_creative_world_launch_tests.cpp` 2, `product_starter_menu_action_tests.cpp` 1, `product_window_input_frame_tests.cpp` 1 | world-template lookup, ASCII authoring/package, session bootstrap, world creation, initial save, marker binding/active-room collision | Not first | Keep separate from save/load launch; ASCII authoring and initial-save behavior make it a distinct slice. |
| `launchProductContinueSave(...)` | Product session/save-load launch seam | `ActionHandlers.cpp` 1 | None | `launchProductSaveSlot(...)`, `saveSlotById(...)`, `recordProductSaveLoadSelection(...)`, `createProductSession(...)`, save load result mirroring | After pre-step | Good second implementation candidate after template/path extraction. |
| `launchProductLoadSaveSelection(...)` | Product session/save-load launch seam | `ActionHandlers.cpp` 1 | None | `initializeSelectedProductSaveSlot(...)`, `launchProductSaveSlot(...)`, same save-load helpers as continue | After pre-step | Move with continue/save-slot launch, not with product new-world. |

### Private Helper Cluster Classification

| Cluster | Functions | Owner candidate | Classification |
| --- | --- | --- | --- |
| Package path/status helpers | `packageLoadStatusName(...)`, `elapsedMicroseconds(...)`, `defaultProductPackagePath(...)` | Split: `defaultProductPackagePath(...)` with new template/path seam; status/timing with session-launch seam | `defaultProductPackagePath(...)` is shared by template lookup and package session creation. `packageLoadStatusName(...)` and timing are only meaningful when recording launch/startup receipt fields. |
| Product package/session bootstrap | `createProductSessionFromPackage(...)`, `createProductSession(...)` | Product session launch seam | Cohesive runtime package-to-session bootstrap. Uses package loader, package seed, active-room install, collision freshness, and launch receipt fields. |
| Creative blank-stage wrappers | `createCreativeBlankSessionImpl(...)`, `frameCreativeStageCameraOnOriginImpl(...)`, `clearProductGameplayLaunchStateImpl(...)` | Creative blank-stage/session seam plus shared clear helper | Public wrappers are only needed by `CreativeWorldOperations.cpp` today. They should not remain in `Operations.hpp` long-term, but moving them together with product save/load would mix concerns. |
| Product new-world / ASCII launch | `productAsciiRoomAuthoringRequestFromWorldSetup(...)`, `prepareProductWorldCreationFromDraft(...)`, `recordProductWorldInitialSaveResult(...)`, `recordSavedRoomMarkerBindingResult(...)`, `launchProductNewWorld(...)` | Product world launch seam | Distinct from save/load: owns world setup receipts, ASCII authoring preview/package, initial durable save, and menu transition. |
| Current-session save/write | `recordProductSaveWriteResult(...)`, `writeProductCurrentSessionSave(...)` | Save-owned current-session write seam | Narrow and mostly save-flow owned. Only production caller is `save/Flow.cpp`. Do not fold into session launch. |
| Save load/continue launch | `recordProductSaveLoadSelection(...)`, `recordProductSaveLoadResult(...)`, `saveSlotById(...)`, `launchProductSaveSlot(...)`, `launchProductContinueSave(...)`, `launchProductLoadSaveSelection(...)` | Product session/save-load launch seam | Cohesive next large move after template/path pre-step. Needs product session bootstrap, active-room collision, saved-room marker binding, and menu transitions. |

### Caller Counts And Hotspots

- `productWorldTemplateFromOptions(...)`:
  - Production call expressions: 10 total including definition/internal use.
  - Hottest files: `ActionHandlers.cpp` 4 call expressions,
    `SaveSlotOperations.cpp` 3, `AppKernel.cpp` 1, `Operations.cpp` 2
    including definition/internal use.
  - Test call expressions: `product_window_input_frame_tests.cpp` 1.
- `writeProductCurrentSessionSave(...)`:
  - Production call expressions: `save/Flow.cpp` 1 plus declaration/definition.
  - Direct test call expressions: none found.
- `launchProductNewWorld(...)`:
  - Production call expressions: `AppKernel.cpp` 1, `ActionHandlers.cpp` 1.
  - Direct test call expressions: `product_creative_world_launch_tests.cpp` 2,
    `product_starter_menu_action_tests.cpp` 1,
    `product_window_input_frame_tests.cpp` 1.
- `launchProductContinueSave(...)` and
  `launchProductLoadSaveSelection(...)`:
  - Production call expressions: `ActionHandlers.cpp` 1 each.
  - Direct test call expressions: none found.
- Creative blank-stage wrappers:
  - `CreativeWorldOperations.cpp` calls `createCreativeBlankSession(...)` and
    `frameCreativeStageCameraOnOrigin(...)` in both creative new/open paths.
  - `clearProductGameplayLaunchState(...)` is called from
    `CreativeWorldOperations.cpp` and from product new-world/save-load failure
    paths in `Operations.cpp`.
- Remaining `Operations.hpp` includer hotspots:
  - Production: `AppKernel.cpp`, `ActionHandlers.cpp`, `InputFrame.cpp`,
    `save/Flow.cpp`, `save/SaveSlotOperations.cpp`,
    `creative/CreativeWorldOperations.cpp`, `Operations.cpp`.
  - Tests: `product_creative_world_launch_tests.cpp`,
    `product_creative_no_window_bake_scenario_tests.cpp`,
    `product_window_input_frame_tests.cpp`,
    `product_starter_menu_action_tests.cpp`,
    `product_creative_ui_input_frame_tests.cpp`,
    `product_creative_ui_frame_tests.cpp`,
    `product_creative_viewport_pick_frame_tests.cpp`,
    `product_frontend_router_tests.cpp`.

### Decision Questions

1. **Should the next implementation extract product save/load launch into a
   `ProductSessionLaunch` seam, or is a smaller pre-step needed?**
   - Smaller pre-step recommended. Extracting save/load launch now would still
     leave `SaveSlotOperations.cpp`, `ActionHandlers.cpp`, and `AppKernel.cpp`
     tied to `Operations.hpp` for `productWorldTemplateFromOptions(...)`.
     Move template/package-path resolution first.
2. **Should `productWorldTemplateFromOptions(...)` move with save/load launch,
   product new-world launch, or a tiny template seam?**
   - Move to a tiny `ProductWorldTemplateOperations` seam. It is shared by app
     startup, save-slot delete/recover, continue/load actions, creative menu
     catalog refresh, and product new-world. It should not belong to any one
     launch path.
3. **Should `writeProductCurrentSessionSave(...)` move alone before save/load
   launch, or stay until save-flow coupling is clearer?**
   - Stay for now. It has one production caller (`save/Flow.cpp`) and a clean
     later target: a save-owned current-session write seam. It is not blocking
     the session launch split as much as template/path ownership is.
4. **Should the E212 creative blank-stage wrappers remain public in
   `Operations.hpp`, move to a creative/product session launch seam, or get a
   dedicated narrower header?**
   - They should get a dedicated narrower creative blank-stage/session header
     in a later slice. `createCreativeBlankSession(...)` and
     `frameCreativeStageCameraOnOrigin(...)` are only used by
     `CreativeWorldOperations.cpp`; keeping them public in `Operations.hpp` is
     temporary. `clearProductGameplayLaunchState(...)` is shared failure
     cleanup and should move with the session-launch seam or a small shared
     cleanup helper when that seam is extracted.
5. **Is it safe to extract `launchProductNewWorld(...)` together with save/load
   launch, or should product new-world/ASCII authoring stay separate?**
   - Keep it separate. Product new-world owns world setup receipt fields,
     ASCII room authoring/package activation, initial durable save, and starter
     menu behavior. Save/load launch owns catalog slot selection and durable
     save restoration. Combining them would make the next card too broad.

### Recommended Next Implementation Card

Title:

`E214 — Operations Split G4a: Product World Template Resolution`

Objective:

Move product package path resolution and `productWorldTemplateFromOptions(...)`
out of `Operations.*` into a tiny world-owned seam, preserving package identity,
scenario identity, save catalog, and receipt behavior.

Proposed files:

- Add `src/app/iggy3d/world/ProductWorldTemplateOperations.hpp`
- Add `src/app/iggy3d/world/ProductWorldTemplateOperations.cpp`
- Edit `CMakeLists.txt`
- Edit `src/app/iggy3d/Operations.hpp`
- Edit `src/app/iggy3d/Operations.cpp`
- Edit direct callers/includes:
  - `src/app/iggy3d/AppKernel.cpp`
  - `src/app/iggy3d/menu/ActionHandlers.cpp`
  - `src/app/iggy3d/save/SaveSlotOperations.cpp`
  - `tests/unit/product_window_input_frame_tests.cpp`
  - any compiler-reported direct callers that use the moved API

Proposed API:

```cpp
std::filesystem::path productPackagePathFromOptions(
    const ProductAppOptions& options);
ProductWorldTemplate productWorldTemplateFromOptions(
    const ProductAppOptions& options);
```

Move:

- `productWorldTemplateFromOptions(...)`
- `defaultProductPackagePath(...)`, renamed or surfaced as
  `productPackagePathFromOptions(...)` so `Operations.cpp::createProductSession`
  can share the exact same path resolution without duplicating it.

Keep in `Operations.cpp`:

- `packageLoadStatusName(...)`
- `elapsedMicroseconds(...)`
- `createProductSessionFromPackage(...)`
- `createProductSession(...)`
- creative blank-stage wrappers
- product new-world launch
- save/load launch
- current-session save/write

Explicit non-scope for E214:

- Do not move product session bootstrap.
- Do not move `launchProductNewWorld(...)`.
- Do not move `launchProductContinueSave(...)` or
  `launchProductLoadSaveSelection(...)`.
- Do not move `writeProductCurrentSessionSave(...)`.
- Do not move creative blank-stage wrappers.
- Do not change package lookup policy, package loader validation, save catalog
  behavior, receipt keys/order/values, CMake test definitions, save formats,
  renderer/window code, staging, commit, push, or window launch.

Focused verification for E214:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_starter_menu_action_tests product_window_input_frame_tests product_save_delete_executor_tests product_automation_dispatch_tests product_creative_world_launch_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_starter_menu_action_tests|product_window_input_frame_tests|product_save_delete_executor_tests|product_automation_dispatch_tests|product_creative_world_launch_tests|product_receipt_key_order_tests)$' --output-on-failure
rg -n "productWorldTemplateFromOptions|productPackagePathFromOptions|defaultProductPackagePath" /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/world/ProductWorldTemplateOperations.hpp /Users/kogaryu/iggy3d/src/app/iggy3d/world/ProductWorldTemplateOperations.cpp
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over touched files and the card.

Self-blockers for E214:

- Stop if exposing/shared path resolution changes package path strings or
  package identity/scenario identity.
- Stop if save catalog or receipt golden output changes.
- Stop if the move forces session bootstrap, save/load launch, or product
  new-world launch into the new template file.
- Stop if compile fallout expands beyond direct include repairs.

### Follow-up After E214

After template/path resolution is out of `Operations.hpp`, the next likely
implementation is:

`E215 — Operations Split G4b: Product Save/Load Session Launch`

Move into a `ProductSessionLaunch` seam:

- `createProductSessionFromPackage(...)`
- `createProductSession(...)`
- `recordProductSaveLoadSelection(...)`
- `recordProductSaveLoadResult(...)`
- `recordSavedRoomMarkerBindingResult(...)`
- `saveSlotById(...)`
- `launchProductSaveSlot(...)`
- `launchProductContinueSave(...)`
- `launchProductLoadSaveSelection(...)`

Keep `launchProductNewWorld(...)`, current-session save/write, and creative
blank-stage wrappers out of that card unless the compiler proves a narrow
helper declaration is needed.

### Commands Run

```sh
wc -l /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.cpp /Users/kogaryu/iggy3d/src/app/iggy3d/Operations.hpp
rg -n "productWorldTemplateFromOptions|writeProductCurrentSessionSave|createCreativeBlankSession|frameCreativeStageCameraOnOrigin|clearProductGameplayLaunchState|launchProductNewWorld|launchProductContinueSave|launchProductLoadSaveSelection|launchProductSaveSlot|createProductSession|createProductSessionFromPackage|prepareProductWorldCreationFromDraft|recordProductSaveWriteResult|recordProductSaveLoadSelection|recordProductSaveLoadResult|defaultProductPackagePath" /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'
rg -n "#include \"app/iggy3d/Operations.hpp\"" /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'
rg -n "#include \"app/iggy3d/Operations.hpp\"" /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp' | wc -l
rg -n "productWorldTemplateFromOptions\\(" /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'
rg -n "writeProductCurrentSessionSave\\(" /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'
rg -n "launchProductNewWorld\\(" /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'
rg -n "launchProductContinueSave\\(|launchProductLoadSaveSelection\\(" /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'
rg -n "createCreativeBlankSession\\(|frameCreativeStageCameraOnOrigin\\(|clearProductGameplayLaunchState\\(" /Users/kogaryu/iggy3d/src/app/iggy3d /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'
git -C /Users/kogaryu/iggy3d diff --check
```

Additional `nl -ba` / `sed` reads were used for the inspected files listed
above.

### Verification

- `git -C /Users/kogaryu/iggy3d diff --check` passed.
- Focused trailing-whitespace scan over this card will be run after moving it
  to `done/`.
- No build or CTest was run because this was read-only.

### Confirmation

No source, test, CMake, fixture, receipt golden, production docs, staging,
commit, push, or window launch was performed.
