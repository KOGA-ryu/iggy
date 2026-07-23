# AUT-002A Luna Work Order: Singular Same-Document Publication

## Status

Blocked until AUT-008 is accepted.

This is one coherent Authoring Core batch. It replaces every live
same-document staging-copy assignment with one validated publication
primitive. Do not combine it with history-branch revision lineage,
persistence cleanliness, recipe integration, or input routing.

## Problem

The repository currently has competing ways to publish a staging copy of the
live Creative document:

- 20 direct `document = std::move(staged...)` assignments;
- five call sites of the documented
  `CreativeDocument::commitStagedMutation` primitive;
- `Facade::createDocumentObjectsAtomically` stages the current document and
  then calls `installDocument`, resetting editor-owned transient state;
- several outward-facing receipts expose the staging copy's intermediate
  revision jumps as though they were live atomic publication facts.

This makes "atomic" mean different things by tool. Pattern tests currently pin
single user commands that advance the live revision by four, five, or seven.
Facade batch create advances by the number of staged creates and clears
selection, tool, measurement, snap, ghost, and command-stat state through the
replacement path.

`installDocument` is a replacement boundary. It must not be the publication
mechanism for an edited copy of the current document.

## Target Ownership

`CreativeDocument::commitStagedMutation` is the sole primitive allowed to
publish a staging copy of the current document.

Direct assignment remains legal only for:

- construction of a value that is not yet live;
- moving an already-published result into an output receipt;
- whole-document replacement inside `Facade::installDocument`.

The post-edit repository search must have no direct same-document staging
assignment under `src/app/iggy3d/creative`, excluding the explicitly reviewed
`WorldLayoutApply.cpp` result-value construction.

## Typed Publication Contract

Replace the boolean `commitStagedMutation` result with a fixed-layout receipt
declared beside `CreativeDocument`:

```cpp
enum class CreativeDocumentPublicationStatus : std::uint8_t {
  NotRequested,
  InvalidLiveDocument,
  InvalidStagedDocument,
  MissingDocumentId,
  DocumentIdMismatch,
  StagedRevisionNotAdvanced,
  RevisionExhausted,
  Published,
};

struct CreativeDocumentPublicationReceipt {
  bool requested = false;
  bool accepted = false;
  bool changed = false;
  CreativeDocumentPublicationStatus status =
      CreativeDocumentPublicationStatus::NotRequested;
  CreativeDocumentId documentId = kInvalidDocumentId;
  std::uint64_t revisionBefore = 0U;
  std::uint64_t stagedRevision = 0U;
  std::uint64_t revisionAfter = 0U;
  std::uint64_t objectCountBefore = 0U;
  std::uint64_t objectCountAfter = 0U;
  CreativeObjectDirtyFlags dirtyFlagsBefore = 0U;
  CreativeObjectDirtyFlags dirtyFlagsAfter = 0U;
  std::string_view reasonCode =
      "creative_document_publication_not_requested";
};
```

Exact names may change only to match established repository vocabulary. Keep
the fields fixed-layout and trivially copyable.

Publication laws:

- Validate the full live and staged `isValid()` contracts, not only the private
  activation bit.
- Reject a missing live id or a staged document with a different id.
- Reject a staging copy whose revision did not advance; callers must not
  publish no-change work.
- Reject revision overflow.
- A successful publication forces the installed staging copy to
  `revisionBefore + 1`, regardless of how many staged suboperations ran.
- A rejected publication leaves the live document and revision unchanged.
- `revisionBefore`, `stagedRevision`, and `revisionAfter` distinguish live
  state from staging diagnostics.
- Preserve the staged copy's accumulated dirty flags and all validated durable
  content on success.
- Add `toString(CreativeDocumentPublicationStatus)` if status strings are
  exposed elsewhere; do not add dynamic receipt strings.

## Core Mutation Migration

### `document/DocumentMutation.*`

Change `applyDocumentMutationsAtomically` to publish through
`commitStagedMutation`.

Add the typed publication receipt to
`CreativeDocumentBatchMutationReceipt`. On publication rejection:

- `committed` is false;
- `changed` is false for the live document;
- `revisionAfter == revisionBefore`;
- the status remains an existing rejection/failure status;
- no partially staged mutation becomes visible.

Do not change the non-atomic `applyDocumentMutations` contract.

### Existing canonical callers

Migrate the existing five boolean call sites:

- `tools/RecipeTransform.cpp`;
- both `tools/VolumeObjects.cpp` call sites;
- both `tools/HierarchyTransform.cpp` call sites.

Map publication failure into each owner's existing rejected status and phase.
Do not weaken their no-change checks or add a second publication layer.

## Direct Publication Migration

Replace all direct same-document staging assignments in:

- `assets/AuthoredAssetRefresh.cpp`;
- `assets/AuthoredAssetPlacement.cpp`;
- `tools/Group.cpp`;
- `tools/Clipboard.cpp`;
- `tools/VolumeFill.cpp`;
- `tools/Pattern.cpp`;
- `tools/AssetScatter.cpp`;
- `document/DocumentMutation.cpp`.

For every owner:

- call `commitStagedMutation` exactly once after all staging checks pass;
- map publication rejection to the owner's existing rejected status;
- clear output ids or counts that the existing failure path promises are not
  published;
- set the outer receipt's revision from the live publication receipt;
- preserve existing semantic status, object ordering, id remaps, source
  provenance, dirty flags, and rollback behavior;
- do not split one command into multiple live publications.

## Receipt Truth

The outer command receipt is the authority for live publication.

Any nested create, remove, paste, mutation, or pattern receipt returned to a
caller must not report a staging-only revision as a live revision:

- on accepted changed publication, normalize contributing nested receipt
  revision ranges to the outer live `revisionBefore` and `revisionAfter`;
- on rejected publication, nested receipt revision ranges must end at the
  unchanged live revision;
- preserve nested semantic statuses, ids, counts, dirty flags, and reason
  codes;
- do not add another revision counter to high-level tool receipts.

At minimum, review and normalize:

- group create, mutation, remove, and batch-remove receipts;
- clipboard paste and cut remove receipts;
- linear/radial array paste and pattern-mutation receipts;
- asset-scatter pattern-mutation and hierarchy-removal receipts;
- authored-asset root creation, content paste, parenting, and refresh mutation
  receipts.

If an existing nested receipt cannot truthfully represent participation in one
atomic live publication, stop and report the exact type rather than preserving
an intermediate live-revision claim.

## Facade Batch Create

Convert `Facade::createDocumentObjectsAtomically` from same-document
`installDocument` to `commitStagedMutation`.

Rename the replacement vocabulary:

- `InstallRejected` -> `PublicationRejected`;
- `installAttempted` -> `publicationAttempted`;
- `installReceipt` -> `publicationReceipt`.

Laws:

- Empty input rejects without publication.
- A late create failure leaves the live document byte-equivalent.
- Any successful nonempty batch advances the live revision exactly once.
- Selection, active tool, tool pointer, measurement, snap settings, ghost
  state, move-drag state, and Facade stats are preserved.
- Do not route this path through `installDocument`.
- Do not redesign or remove Facade stats in this batch.
- Update consumers that inspect the renamed receipt fields; do not retain
  compatibility aliases.

`installDocument` remains the explicit replacement boundary for new, open,
history restoration, and source-reconciliation paths. Do not modify its
revision-lineage behavior in AUT-002A.

## Tests

### Publication primitive

Extend `creative_document_mutation_tests` or the closest existing document
owner to pin every publication status:

- invalid live document;
- invalid staged document;
- missing id;
- id mismatch;
- staging revision not advanced;
- revision overflow if it can be reached without a test-only backdoor;
- successful publication;
- success normalizes multiple staged increments to exactly `+1`;
- rejection leaves durable content, dirty flags, and revision unchanged.

Do not add a mutable test accessor solely to force private revision state. If
overflow cannot be reached through a supported contract, test the status mapper
and leave the runtime guard source-reviewed.

### Atomic owners

Pin exactly one live revision and rollback for:

- atomic document mutation;
- group and ungroup;
- hierarchy removal;
- clipboard paste and cut;
- fill/replace volume operations;
- linear and radial array create/update;
- asset-scatter create/update/extend/exclude/remove;
- authored-asset instantiate and refresh;
- recipe transform, volume-object, and hierarchy-transform existing callers.

Replace the existing pattern expectations of `+4`, `+5`, and `+7` with `+1`.
Add assertions that outward nested receipt revision ranges match the outer live
range.

### Facade batch create

Update `creative_document_create_tests` and `creative_facade_tests` to prove:

- two or more creates publish at exactly `+1`;
- a late failure publishes none;
- the typed publication receipt is exposed;
- selection, active tool, measurement, snap, ghost, move-drag, and stats survive
  a successful same-document batch;
- `installDocument` is not observed through replacement-reset facts.

## Allowed Files

Core:

- `src/app/iggy3d/creative/document/Document.hpp`
- `src/app/iggy3d/creative/document/Document.cpp`
- `src/app/iggy3d/creative/document/DocumentMutation.hpp`
- `src/app/iggy3d/creative/document/DocumentMutation.cpp`
- `src/app/iggy3d/creative/Facade.hpp`
- `src/app/iggy3d/creative/Facade.cpp`
- `src/app/iggy3d/creative/tools/RecipeTransform.cpp`
- `src/app/iggy3d/creative/tools/VolumeObjects.cpp`
- `src/app/iggy3d/creative/tools/HierarchyTransform.cpp`
- `src/app/iggy3d/creative/tools/Group.cpp`
- `src/app/iggy3d/creative/tools/Clipboard.cpp`
- `src/app/iggy3d/creative/tools/VolumeFill.cpp`
- `src/app/iggy3d/creative/tools/Pattern.cpp`
- `src/app/iggy3d/creative/tools/AssetScatter.cpp`
- `src/app/iggy3d/creative/assets/AuthoredAssetRefresh.cpp`
- `src/app/iggy3d/creative/assets/AuthoredAssetPlacement.cpp`

Headers may be changed only when required to expose the typed publication
receipt or to remove a stale receipt field:

- `src/app/iggy3d/creative/tools/Group.hpp`
- `src/app/iggy3d/creative/tools/Clipboard.hpp`
- `src/app/iggy3d/creative/tools/Pattern.hpp`
- `src/app/iggy3d/creative/tools/AssetScatter.hpp`
- `src/app/iggy3d/creative/assets/AuthoredAsset.hpp`

Tests:

- `tests/unit/creative_document_mutation_tests.cpp`
- `tests/unit/creative_document_create_tests.cpp`
- `tests/unit/creative_facade_tests.cpp`
- `tests/unit/creative_group_tests.cpp`
- `tests/unit/creative_tools_tests.cpp`
- `tests/unit/creative_volume_tests.cpp`
- `tests/unit/creative_pattern_tests.cpp`
- `tests/unit/creative_asset_scatter_tests.cpp`
- `tests/unit/creative_authored_asset_tests.cpp`
- focused tests already owning RecipeTransform, VolumeObjects, and
  HierarchyTransform behavior if they are not in the files above.

Governance files are Sol-owned and excluded from Luna's implementation commit.

## Scope Firewall

Do not:

- change save formats, codecs, hashes, compatibility, golden files, or dirty
  draining;
- add a serialized revision or publication field;
- add a second publication id, generation, epoch, or cache stamp;
- repair undo/redo branch revision aliasing; AUT-002B owns the Facade revision
  high-water;
- change World Layout source revisions or source-reconciliation installation;
- integrate or delete generic recipe apply APIs; AUT-001 owns that;
- centralize history transaction predicates; AUT-003 follows publication
  repair;
- expose a public staged-document publisher on Facade;
- add callbacks, mutable proxies, public mutation options, friend access, or
  test-only document backdoors;
- change renderer, cache, UI, input, controller, or playtest behavior;
- launch a window or run broad CTest.

## Stop Conditions

Stop and report before widening scope if:

- a direct staging assignment is not publication of an edited copy of the
  current document;
- converting an owner to one publication would change object ids, ordering,
  provenance, dirty domains, or rollback semantics;
- a nested receipt cannot be made truthful without changing its public shape;
- any product caller requires Facade batch create to reset editor transient
  state;
- the focused baseline fails before edits for a reason unrelated to AUT-002A;
- an allowed-file omission is required for compilation or an exact focused
  behavior test.

## Verification

Before edits:

```bash
rg -n 'document\s*=\s*std::move\(staged(Document)?\)' \
  src/app/iggy3d/creative
rg -n 'commitStagedMutation\(' src/app/iggy3d/creative
rg -n 'installAttempted|installReceipt|InstallRejected' \
  src/app/iggy3d/creative apps/iggy3d_creative tests/unit
```

After edits:

```bash
rg -n 'document\s*=\s*std::move\(staged(Document)?\)' \
  src/app/iggy3d/creative
rg -n 'commitStagedMutation\(' src/app/iggy3d/creative
rg -n 'installAttempted|installReceipt|InstallRejected' \
  src/app/iggy3d/creative/Facade.* \
  src/app/iggy3d/creative/world/MapTemplate.cpp \
  src/app/iggy3d/creative/world/MapDemoTemplate.cpp \
  src/app/iggy3d/creative/recipes/CreativeRecipe.* \
  tests/unit/creative_document_create_tests.cpp \
  tests/unit/creative_facade_tests.cpp
```

The first post-edit search may return
`WorldLayoutApply.cpp: result.document = std::move(staged.document)` only. It
constructs an output value and is not live publication. The second search must
show the single primitive definition and every reviewed staged owner calling
it. The third search must be empty for the listed Facade batch-create surfaces;
do not remove unrelated replacement-path `InstallRejected` vocabulary.

Configure and run the focused gate from a clean build tree:

```bash
cmake -S . -B /tmp/iggy3d-aut002a-review
CCACHE_DISABLE=1 cmake --build /tmp/iggy3d-aut002a-review --target \
  creative_document_mutation_tests \
  creative_document_create_tests \
  creative_facade_tests \
  creative_group_tests \
  creative_tools_tests \
  creative_volume_tests \
  creative_pattern_tests \
  creative_asset_scatter_tests \
  creative_authored_asset_tests \
  creative_facade_mutation_tests \
  creative_editor_attachment_tests
ctest --test-dir /tmp/iggy3d-aut002a-review -R \
  '^(creative_document_mutation_tests|creative_document_create_tests|creative_facade_tests|creative_group_tests|creative_tools_tests|creative_volume_tests|creative_pattern_tests|creative_asset_scatter_tests|creative_authored_asset_tests|creative_facade_mutation_tests|creative_editor_attachment_tests)$' \
  --output-on-failure
python3 tools/repo_departments.py check
git diff --check
```

Do not use the existing `build/` archive as acceptance evidence.

## Completion Brief

Report:

- commit hash and exact changed files;
- the typed publication statuses and fields;
- final direct-publication and primitive-call counts;
- every migrated owner;
- exact one-revision, rollback, nested-receipt, and Facade-state pins;
- focused clean build, CTest, department checker, and whitespace results;
- any deviation or stop condition;
- confirmation that AUT-002B revision lineage, persistence, history policy,
  recipes, UI, input, and rendering were not touched.
