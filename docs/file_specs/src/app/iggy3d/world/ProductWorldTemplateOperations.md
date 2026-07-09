# File Spec

Files: `src/app/iggy3d/world/ProductWorldTemplateOperations.hpp`, `src/app/iggy3d/world/ProductWorldTemplateOperations.cpp`

Verified at: `266066cb`

## Owns

- Product package path resolution from app options.
- Product world template resolution from app options and package metadata.
- Dev package override conversion into world template identity.

## Does Not Own

- Default template field definitions.
- Package loading implementation.
- Runtime session creation.
- Save catalog compatibility policy.

## Reads

- `ProductAppOptions.devPackageOverride` and `ProductAppOptions.devScenario`.
- Package runtime lookup result for build-tree product package location.
- Loaded package manifest and scenario ids when package load succeeds.

## Writes / Mutates

- Returns package paths and `ProductWorldTemplate` values.
- Does not mutate app/window/runtime state.

## Calls Out To / Wires Out To

- `resolvePackageRuntimeLookup(...)`.
- `defaultProductWorldTemplate(...)` and `devOverrideProductWorldTemplate(...)`.
- `loadPackage(...)` for package/scenario id refresh.

## Called By / Entry Points

- App kernel startup, product menu action handlers, save slot operations, product session launch, and new-world launch.
- Focused proof: `rg -n "productPackagePathFromOptions|productWorldTemplateFromOptions" src/app tests/unit`.

## Invariants

- Dev package override wins over runtime lookup and fixture fallback.
- Build-tree lookup must not require graphics runtime or shader root.
- Template package/scenario ids update from loaded package metadata only when package load succeeds.
- This file resolves product template facts; it must not create sessions or write saves.

## Tests / Proof Commands

- `rg -n "productWorldTemplateFromOptions|defaultProductWorldTemplate|devOverrideProductWorldTemplate" tests/unit src/app`.
- `rg -n "product_world_creation_tests|product_window_input_frame_tests|product_save_delete_executor_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/PackageRuntimeLookup.*` unless package lookup policy changes.
- `src/content/PackageLoader.*` unless package metadata loading contracts change.
- `src/app/iggy3d/world/DefaultWorldTemplate.*` unless template default fields change.

## Update When

- Package path lookup, dev override behavior, template metadata refresh, or app option inputs change.

## Do Not Update When

- Only session creation, save/load flow, or package asset contents change without changing template resolution.
