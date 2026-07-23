# AUT-002B Luna Work Order: Branch-Safe Live Revision Lineage

## Status

Blocked until AUT-002A is accepted.

This batch makes the existing nonserialized Creative document revision a
branch-safe live-state version across whole-document replacement, undo, redo,
reset, and reinstall. It does not add another publication stamp and does not
change save cleanliness.

## Problem

`CreativeDocument::revision()` is already the freshness key for scene,
clearance, terrain, validation, desktop-model, preview, stale-plan, and
play-preparation paths. The revision is intentionally omitted from durable
serialization and restored documents start at zero.

Same-document mutations advance it, but `Facade::installDocument` currently
installs an incoming historical revision verbatim. Undo can therefore replace
revision `N + 1` with an old snapshot at revision `N`; an alternate edit then
reuses `N + 1` for different content. Any cache or frozen plan keyed by
`{documentId, revision}` can mistake the alternate branch for the prior state.

A second publication id would duplicate the existing transient revision and
force every consumer to carry two freshness counters. Repair the lineage at
the replacement boundary instead.

## Target Contract

`Facade` owns a nonserialized live-revision high-water mark.

### Initial install

The first successful `installDocument` on a newly constructed Facade preserves
the incoming document revision. This keeps initial create/open behavior and
restore revision zero intact.

### Later replacement

Every later successful `installDocument`, including after `Facade::reset`,
installs at:

```text
max(highWaterBefore, currentLiveRevision, incomingRevision) + 1
```

Laws:

- The installed revision is strictly greater than every revision previously
  published by that Facade.
- The law applies to same-id and different-id replacement. Document id remains
  the durable identity; the monotonically increasing revision is the
  Facade-local live-state version.
- A failed install leaves the live document, high-water mark, and transient
  editor state unchanged.
- Revision overflow rejects installation before publication.
- `Facade::reset` clears the live document and editor state but preserves the
  high-water mark and the fact that an initial publication already occurred.
- Same-document mutations need no additional Facade call. A later replacement
  includes the current live revision in the maximum and catches up the
  high-water mark.
- Copy/move of a Facade must copy/move the lineage state with the rest of the
  owner. Do not add global or static revision state.

## Narrow Document Access

Do not add a public revision setter, mutable document accessor, callback,
friend access for the entire `Facade`, or test-only backdoor.

Add one narrowly named internal revision-rebase operation under
`document/DocumentInternal.hpp`, with only the access needed to increase the
transient revision of an already validated replacement document. A suggested
shape is an internal access struct friended by `CreativeDocument`; exact naming
may follow existing internal vocabulary.

The operation must:

- accept only a valid document with a valid id;
- only increase or preserve revision as directed by the Facade installation
  law;
- reject overflow or regression;
- leave all durable content and dirty flags unchanged.

`CreativeDocument::reset()` and `restoreForLoad()` retain their current
standalone contracts: revision zero. The Facade installation boundary performs
live rebasing.

## Install Receipt

Extend `CreativeFacadeDocumentInstallReceipt` with fixed revision facts:

- previous live revision;
- incoming revision;
- installed revision;
- high-water before and after;
- whether revision rebasing occurred;
- whether this was the initial successful install.

Keep the existing replacement-reset facts. A rejected receipt reports
unchanged installed/high-water values and a specific reason code. Do not
replace the existing receipt with an exception or boolean.

## History Behavior

`applyCreativeHistory` continues to restore the selected snapshot through
`Facade::installDocument`, but the restored durable content receives a new live
revision.

Pin this sequence:

1. install document at revision `N`;
2. make edit A at `N + 1`;
3. undo A: content equals the before snapshot and live revision is greater than
   `N + 1`;
4. redo A: content equals the after snapshot and live revision grows again;
5. undo, then make alternate edit B: content differs from A and live revision
   does not reuse any prior value;
6. a plan frozen on A's `{documentId, revision}` rejects on B;
7. a cache built on A cannot hit on B.

History snapshots may keep the revision captured with their content. The
install boundary rebases the copy being published; do not rewrite stored
snapshots in place.

## Cache and Stale-Plan Consumers

No production cache structure should require a new field. Existing
`{documentId, revision}` and revision-only comparisons become branch-safe once
the lineage law is enforced.

Add focused regression pins through existing public consumers:

- hierarchy reattachment or asset-replacement stale plan;
- scene/preview or placement-clearance cache rebuild;
- map-validation cache if it is cheaper than a render-owner test.

The pin must construct equal numeric historical revisions only through the old
undo/branch scenario and prove the repaired Facade no longer publishes the
alias. Do not manually edit private revisions in tests.

## Allowed Files

Production:

- `src/app/iggy3d/creative/document/Document.hpp`
- `src/app/iggy3d/creative/document/DocumentInternal.hpp`
- `src/app/iggy3d/creative/Facade.hpp`
- `src/app/iggy3d/creative/Facade.cpp`

Tests:

- `tests/unit/creative_facade_tests.cpp`
- `tests/unit/creative_facade_mutation_tests.cpp`
- `tests/unit/creative_world_layout_source_history_tests.cpp`
- one existing stale-plan test owner:
  - `tests/unit/creative_editor_attachment_tests.cpp`, or
  - `tests/unit/creative_editor_asset_reload_tests.cpp`
- one existing cache owner:
  - `tests/unit/creative_editor_placement_clearance_tests.cpp`,
  - `tests/unit/creative_editor_map_validation_diagnostics_tests.cpp`, or
  - the current scene-cache test file discovered during pre-edit survey.

Choose one stale-plan owner and one cache owner. Do not touch all candidates.
Governance files are Sol-owned and excluded from Luna's implementation commit.

## Scope Firewall

Do not:

- serialize or hash the document revision or Facade high-water mark;
- add a publication stamp, generation, epoch, content id, or cache-key field;
- change Creative document id semantics;
- change save dirty/clean state, `lastSavedRevision`, World Layout
  `savedRevision`, or history-clearing behavior;
- change source-side World Layout revision lineage;
- migrate cache structs or stale-plan request types;
- change the content of history snapshots;
- change save/load codecs, compatibility, integrity hashes, golden files, UI,
  input, controller, renderer behavior, or playtest behavior;
- launch a window or run broad CTest.

## Stop Conditions

Stop and report before widening scope if:

- a live cache or stale-plan consumer does not include document revision at
  all;
- branch safety requires serializing a transient counter;
- initial install cannot preserve the incoming revision without breaking the
  replacement law;
- Facade copy/move semantics require a new public lifecycle contract;
- a focused baseline fails before edits for a reason unrelated to AUT-002B;
- an allowed-file omission is required for compilation or the two selected
  consumer proofs.

## Verification

Before edits:

```bash
rg -n 'installDocument|revisionBefore|revisionAfter' \
  src/app/iggy3d/creative/Facade.* \
  src/app/iggy3d/creative/history \
  tests/unit/creative_facade_tests.cpp \
  tests/unit/creative_world_layout_source_history_tests.cpp
rg -n 'creativeDocument\.revision|documentRevision' \
  src/app/iggy3d/creative/world/DocumentSection* \
  tests/unit/creative_document_save_section_tests.cpp
```

After edits:

```bash
rg -n 'publicationStamp|publicationId|revisionEpoch|generationId' \
  src/app/iggy3d/creative apps/iggy3d_creative tests/unit
rg -n 'creativeDocument\.revision' \
  src/app/iggy3d/creative/world/DocumentSection*
rg -n 'encodedOmitsRevision|creativeDocument\.revision' \
  tests/unit/creative_document_save_section_tests.cpp
```

The first two post-edit searches must be empty. The final search must retain
the existing explicit test that the encoded section omits Creative document
revision.

Configure and run a clean focused gate:

```bash
cmake -S . -B /tmp/iggy3d-aut002b-review
CCACHE_DISABLE=1 cmake --build /tmp/iggy3d-aut002b-review --target \
  creative_facade_tests \
  creative_facade_mutation_tests \
  creative_world_layout_source_history_tests \
  <selected-stale-plan-target> \
  <selected-cache-target> \
  creative_document_save_section_tests \
  creative_document_persistence_state_tests
ctest --test-dir /tmp/iggy3d-aut002b-review -R \
  '^(creative_facade_tests|creative_facade_mutation_tests|creative_world_layout_source_history_tests|<selected-stale-plan-target>|<selected-cache-target>|creative_document_save_section_tests|creative_document_persistence_state_tests)$' \
  --output-on-failure
python3 tools/repo_departments.py check
git diff --check
```

Replace the two placeholders with the selected existing targets in the
completion brief. Do not silently omit them.

## Completion Brief

Report:

- commit hash and exact changed files;
- the high-water fields and narrow internal rebase owner;
- initial-install, replacement, reset/reinstall, overflow-source-review,
  undo/redo, alternate-branch, stale-plan, and cache pins;
- confirmation that document revision remains absent from serialized fields;
- selected stale-plan and cache targets;
- clean build, CTest, department checker, and whitespace results;
- confirmation that persistence cleanliness and World Layout source revision
  semantics remain deferred.
