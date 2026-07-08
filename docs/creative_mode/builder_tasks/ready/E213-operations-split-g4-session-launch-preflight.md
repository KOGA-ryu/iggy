# E213 — Operations Split G4: Session Launch Preflight

## Status

Ready.

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
