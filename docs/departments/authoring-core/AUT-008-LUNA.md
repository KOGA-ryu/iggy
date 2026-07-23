# AUT-008 Luna Work Order: Close The Mutable Facade Escape

## Status

Ready for implementation after accepted baseline `0dcfbd1a`.

This is one coherent Authoring Core batch. Do not split the accessor deletion
from its caller migrations: the batch is complete only when no production or
test code can obtain a mutable document from `Facade`.

## Problem

`Facade::documentForPersistence()` is misnamed and unrestricted. It has no
persistence caller. Seventeen production calls across five editor files use it
to mutate objects, paths, assets, or structural spans, while 30 test calls
across nine files use it as a fixture shortcut. Those mutations bypass the
Facade boundary, command diagnostics, and any future invariant attached to
document publication.

Two callers are not ordinary object mutations:

- absolute hierarchy transform stages several placement operations so failure
  cannot partially move descendants;
- reattachment stages hierarchy transform plus relationship assignment so the
  entire operation is atomic.

Do not replace the mutable reference with a callback, writable proxy, staged
document publisher, or public mutation options. Those are equivalent escapes
under a different name.

## Target Contract

### Facade mutation boundary

Add these public operations to `Facade`:

```cpp
[[nodiscard]] CreativeDocumentMutationReceipt mutateObject(
    const CreativeMutationRequest& request);

[[nodiscard]] CreativeDocumentMutationReceipt mutateObject(
    CreativeObjectId objectId,
    CreativeMutationKind kind,
    CreativeMutationPayload payload);

[[nodiscard]] CreativeDocumentBatchMutationReceipt mutateObjectsAtomically(
    std::span<const CreativeMutationRequest> requests);

[[nodiscard]] CreativeDocumentBatchMutationReceipt
refreshAssetBoundsAtomically(
    std::span<const CreativeMutationRequest> requests);
```

Laws:

- The public methods never expose `CreativeDocumentMutationOptions`.
- `mutateObject` and `mutateObjectsAtomically` use normal locked-object policy.
- `refreshAssetBoundsAtomically` accepts only `SetBounds` requests and uses
  `rejectLockedObjects = false`; any other mutation kind rejects the entire
  request before touching the document.
- A changed call advances the live document revision exactly once.
- A rejected atomic call publishes nothing.
- A no-change call is accepted where the underlying mutation contract accepts
  no change and does not advance revision.
- One Facade call records one command attempt and exactly one success or
  failure. Do not remove or redesign `Stats` in this batch.
- Selection, tool, measurement, snap, and ghost state remain unchanged because
  these operations do not create or remove objects.

### Hierarchy edit kernel

Create:

- `src/app/iggy3d/creative/tools/HierarchyTransform.hpp`
- `src/app/iggy3d/creative/tools/HierarchyTransform.cpp`

The owner provides two explicit atomic kernels and their fixed request/status/
receipt types:

```cpp
applyCreativeHierarchyTransformAtomically(
    CreativeDocument& document,
    const CreativeHierarchyTransformRequest& request);

reattachCreativeObjectHierarchyAtomically(
    CreativeDocument& document,
    const CreativeHierarchyReattachmentRequest& request);
```

`CreativeHierarchyTransformRequest` carries:

- root object id;
- target absolute `CreativeTransform`;
- `setPosition`, `setRotation`, and `setScale` component flags.

`CreativeHierarchyReattachmentRequest` carries:

- expected document id and revision from the preview plan;
- source root object id;
- target object id and target socket;
- target absolute transform.

Kernel laws:

- Resolve the source root and full descendant hierarchy once.
- Validate finite position/rotation, positive scale, component selection,
  missing objects, effective locks, and target-not-inside-source-hierarchy.
- Preserve the current world-space hierarchy behavior: scale around the source
  root, replace Euler orientation in deterministic Z/Y/X removal then X/Y/Z
  application order, and move to the requested root position.
- Reattachment applies the same absolute transform and then assigns the target
  relationship/socket.
- Stage all component operations on a document copy. Publish only through
  `CreativeDocument::commitStagedMutation`.
- Normalize every changed command to one live revision increment regardless of
  the number of staged component mutations.
- Failure at any phase leaves the live document byte-equivalent and revision
  unchanged.
- No-change is accepted without publication.
- Receipts identify status, failed object or phase, changed/accepted state,
  hierarchy object count, and live revision before/after.

Add matching Facade operations:

```cpp
transformObjectHierarchyAtomically(
    const CreativeHierarchyTransformRequest& request);

reattachObjectHierarchyAtomically(
    const CreativeHierarchyReattachmentRequest& request);
```

They wrap the kernels with the same one-attempt/one-result diagnostic law.

## Production Migration Map

### `apps/iggy3d_creative/EditorEdits.cpp`

- `detachObjectWithUndo`, rename, visibility/lock batches, moving-platform
  settings, player spawn settings, NPC spawn settings, loot settings, and exit
  settings use `Facade::mutateObject` or `mutateObjectsAtomically`.
- `setObjectTransformWithUndo` delegates leaf and hierarchy cases to
  `Facade::transformObjectHierarchyAtomically`; delete its local staged
  hierarchy transform implementation.
- `reattachObjectWithUndo` passes the preview plan's frozen document identity,
  revision, snap transform, target id, and socket to the Facade reattachment
  operation.
- Preserve existing history source strings, log fields, receipt mapping, and
  changed predicates.

### `apps/iggy3d_creative/EditorPathEditing.cpp`

Route all four `SetPatrolRoute` mutations through `Facade::mutateObject`.
Preserve moving-platform status and no-change behavior.

### `apps/iggy3d_creative/EditorStructuralPlacement.cpp`

Use `Facade::document()` for the frozen-plan comparison and
`Facade::mutateObjectsAtomically` for transform plus bounds publication.
Preserve stale-preview rejection and one history record.

### `apps/iggy3d_creative/EditorAssetReplacement.cpp`

Use `Facade::mutateObjectsAtomically`. Preserve the preview document, stale
revision gate, and replacement receipt.

### `apps/iggy3d_creative/EditorAssets.cpp`

Use `Facade::refreshAssetBoundsAtomically`. Do not broaden the unlocked policy
beyond `SetBounds`.

### `apps/iggy3d_creative/EditorAttachmentPlacement.hpp/.cpp`

Keep asset-catalog snap and clearance planning in the editor layer. Change the
commit boundary to accept `Facade&`, map the frozen plan into
`CreativeHierarchyReattachmentRequest`, invoke the Facade operation, and map
the core receipt back to the existing editor receipt. During planning, run the
shared hierarchy-transform kernel against the planner's private document copy
to derive clearance candidates without touching the live Facade. Remove the
duplicate absolute hierarchy transform implementation from this file.

### `src/app/iggy3d/creative/Facade.hpp/.cpp`

Delete `documentForPersistence()` declaration and definition only after all
callers are migrated. Keep `document()` as the sole document accessor.

## Test Migration

Remove all 30 test uses of `documentForPersistence()` from:

- `tests/unit/creative_authored_asset_tests.cpp`
- `tests/unit/creative_desktop_ui_command_tests.cpp`
- `tests/unit/creative_document_create_tests.cpp`
- `tests/unit/creative_editor_asset_reload_tests.cpp`
- `tests/unit/creative_editor_attachment_tests.cpp`
- `tests/unit/creative_editor_placement_tests.cpp`
- `tests/unit/creative_facade_tests.cpp`
- `tests/unit/creative_world_layout_diagnostics_tests.cpp`
- `tests/unit/creative_world_layout_tests.cpp`

Use Facade mutation operations when the test models a live edit. For fixture
identity or bulk setup, construct a `CreativeDocument`, mutate it before
publication, then call `Facade::installDocument`. Do not add a test-only mutable
accessor or `const_cast`.

Extend focused tests to prove:

- single mutation success, no-change, missing-object rejection, revision, and
  Stats behavior;
- atomic batch success advances one revision;
- a late batch failure rolls back earlier staged mutations;
- asset-bounds refresh can update a locked object, rejects any non-`SetBounds`
  request atomically, and advances one revision;
- absolute transform of a leaf and a hierarchy preserves current position,
  scale, and deterministic three-axis rotation behavior;
- invalid, missing, or effectively locked hierarchy requests publish nothing;
- reattachment transform plus relationship is one revision and one undo entry;
- stale reattachment preview and target-inside-source-hierarchy publish
  nothing;
- successful mutations do not disturb selection or other editor-owned Facade
  state.

Prefer the existing `creative_facade_mutation_tests`,
`creative_editor_attachment_tests`, `creative_desktop_ui_command_tests`, and
`creative_editor_placement_tests`. Add a dedicated hierarchy-transform test
target only if those owners cannot express the pure kernel without app setup.

## Allowed Files

Production:

- `CMakeLists.txt`
- `src/app/iggy3d/creative/Facade.hpp`
- `src/app/iggy3d/creative/Facade.cpp`
- `src/app/iggy3d/creative/FacadeDocumentMutations.cpp`
- `src/app/iggy3d/creative/FacadeObjectCommands.cpp`
- `src/app/iggy3d/creative/tools/HierarchyTransform.hpp` (new)
- `src/app/iggy3d/creative/tools/HierarchyTransform.cpp` (new)
- `apps/iggy3d_creative/EditorEdits.cpp`
- `apps/iggy3d_creative/EditorPathEditing.cpp`
- `apps/iggy3d_creative/EditorStructuralPlacement.cpp`
- `apps/iggy3d_creative/EditorAssetReplacement.cpp`
- `apps/iggy3d_creative/EditorAssets.cpp`
- `apps/iggy3d_creative/EditorAttachmentPlacement.hpp`
- `apps/iggy3d_creative/EditorAttachmentPlacement.cpp`

Tests:

- the nine migration files listed under **Test Migration**;
- `tests/unit/creative_facade_mutation_tests.cpp`;
- one new hierarchy-transform test and `cmake/iggy3d_tests.cmake` only if the
  existing focused owners are insufficient.

Governance after verification:

- `docs/departments/authoring-core/AUDIT.md`
- `docs/departments/authoring-core/TODO.md`
- `docs/departments/authoring-core/TESTING.md`
- generated department files through `tools/repo_departments.py generate`
- `docs/departments/ownership.tsv` only if a new test file is added.

## Scope Firewall

Do not:

- change persistence formats, hashes, save versions, or golden files;
- change renderer, scene-cache, ImGui, input, controller, or binding code;
- change World Layout source or sidecar semantics;
- remove or redesign Facade `Stats`;
- sweep the other 22 direct staged-document publications; AUT-002 owns that;
- integrate or delete generic recipe application APIs; AUT-001 owns that;
- centralize history transaction policy; AUT-003 follows publication repair;
- add callbacks, public mutation options, mutable proxies, friend access, or
  test-only backdoors to the document;
- launch a window or run broad CTest.

## Stop Conditions

Stop and report before widening scope if:

- exact hierarchy transform or reattachment behavior cannot be represented by
  the two explicit request contracts;
- the asset reload needs unlocked mutation kinds other than `SetBounds`;
- preserving stale-preview rejection requires exposing mutable document state;
- a test requires a mutable live document for behavior rather than fixture
  setup;
- the focused baseline fails before edits for a reason unrelated to this batch.

## Verification

Before edits:

```bash
rg -n "documentForPersistence\(" apps src tests
cmake -S . -B build
```

After edits:

```bash
rg -n "documentForPersistence\(" apps src tests
rg -n "CreativeDocumentMutationOptions" apps/iggy3d_creative
rg -n "document\s*=\s*std::move\(staged" \
  apps/iggy3d_creative/EditorEdits.cpp \
  apps/iggy3d_creative/EditorAttachmentPlacement.cpp
CCACHE_DISABLE=1 cmake --build build --target \
  i3dc \
  creative_facade_mutation_tests \
  creative_facade_tests \
  creative_document_create_tests \
  creative_editor_attachment_tests \
  creative_editor_asset_reload_tests \
  creative_editor_placement_tests \
  creative_authored_asset_tests \
  creative_desktop_ui_command_tests \
  creative_world_layout_tests \
  creative_world_layout_diagnostics_tests
ctest --test-dir build -R \
  '^(creative_facade_mutation_tests|creative_facade_tests|creative_document_create_tests|creative_editor_attachment_tests|creative_editor_asset_reload_tests|creative_editor_placement_tests|creative_authored_asset_tests|creative_desktop_ui_command_tests|creative_world_layout_tests|creative_world_layout_diagnostics_tests)$' \
  --output-on-failure
python3 tools/repo_departments.py generate
python3 tools/repo_departments.py check
git diff --check
```

The first three post-edit searches must return no mutable-accessor hit, no app
use of public mutation options, and no local staged-document assignment in the
two retired duplicate hierarchy implementations.

## Completion Brief

Report:

- commit hash and exact changed files;
- final Facade API and hierarchy-kernel ownership;
- all 47 accessor-call migrations;
- revision, rollback, locked-refresh, stale-plan, history, and state-preservation
  pins;
- focused build/CTest and department checker results;
- any deviation or stop condition;
- remaining AUT-002 staged-publication count, without repairing it here.
