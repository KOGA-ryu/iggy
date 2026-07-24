# Persistence and Validation Audit

Audit P1 traced the Creative save lifecycle from editor commands through
`EditorPersistence`, `WorldService`, `SaveBridge`, the envelope codec, durable
file replacement, document restore, and the map-validation presentation. It
also compared the intended clean/dirty and compatibility laws with history
branching and the actual app call sites.

## Route Map

| Workflow | Current route | Ruling |
| --- | --- | --- |
| Desktop save/save-as | menu command -> desktop dispatcher -> `saveStandaloneScene` -> `saveCreativeWorld` -> durable envelope write -> exact live Facade acknowledgement -> durable-content checkpoint | Durable success drains the exact live document's dirty domains, preserves both history rings, and marks document/source fingerprints |
| Keyboard save | semantic input action -> `EditorCommandInput` -> the same save adapter and shared checkpoint | I/O, history preservation, and clean-state semantics match desktop |
| Open/startup load | `loadStandaloneScene` -> `openCreativeWorld` -> section/source decode -> Facade install -> World Layout publication | Document installation now precedes source publication; failure leaves both live owners unchanged |
| New document | desktop/keyboard command -> `clearToBlankScene` -> Facade install receipt -> editor reset | Callers clear history/reset only after accepted installation and establish an unsaved checkpoint |
| Creative section persistence | `CreativeDocument` -> `buildSaveCreativeDocumentSection` -> `SaveEnvelope` -> strict ordered codec -> `restoreCreativeDocumentFromSaveSection` | Active durable document fields are covered and legacy migrations are substantial; section-version refusal is missing |
| World Layout source persistence | versioned World Layout codec -> optional section in the same envelope -> decode before world open is accepted | Strong atomic transport and explicit unsupported-version refusal; keep |
| Map validation | document/catalog -> bounded typed validation -> cached result -> descriptor table -> actionable panel row/focus command | The diagnostic contract is strong; AUT-002B makes the document id/revision cache key branch-safe |

## Findings

| ID | Surface | Classification | Evidence | Disposition | Priority |
| --- | --- | --- | --- | --- | --- |
| PER-A1-001 | `WorldService` + `SaveBridgeCreative` + `DocumentSection` + `SaveCodec` | Canonical Owner | These are the only Creative world create/open/save, durable section conversion, and envelope-codec routes | Keep | P0 |
| PER-A1-002 | `saveStandaloneScene` live clean-state acknowledgement | Canonical Owner | AUT-005 writes a copy durably, then drains live dirty domains only through an exact Facade document-id/revision acknowledgement; mismatches reject atomically | Keep | P0 |
| PER-A1-003 | Save and Save As history behavior | Canonical Owner | PER-001 removes successful-save history clearing from desktop, keyboard, and capture routes. Focused command tests preserve simultaneous undo/redo rings across Save As and prove navigation away from and back to the checkpoint | Keep | P0 |
| PER-A1-004 | Shared editor document checkpoint | Canonical Owner | `CreativeEditorPersistenceState` owns one document id plus canonical durable-section fingerprint. A document id/revision memoization key avoids per-frame re-encoding without making revisions the clean-state truth | Keep | P0 |
| PER-A1-005 | World Layout clean-state identity | Canonical Owner | PER-001 compares a cached canonical World Layout fingerprint with the fixed saved fingerprint. The save checkpoint remains outside source/document history, so alternate content reusing the saved numeric revision stays dirty | Keep | P0 |
| PER-A1-006 | Document cache and stale-plan keys | Canonical Owner | AUT-002B gives every Facade replacement, undo, redo, reset, and reinstall a monotonic live revision; document id/revision cache keys no longer alias alternate branches | Keep | P0 |
| PER-A1-007 | Creative compatibility enforcement | Contract Risk | `loadCreativeDocumentSave` decodes and restores directly. Unlike runtime `SaveLoad`, it never calls `checkSaveCompatibility`, so schema/runtime/package/scenario policy is not enforced by the Creative open path | Repair | P0 |
| PER-A1-008 | Creative save integrity hash | Contract Risk | `SaveBridgeCreative` writes `savedStateHash=0` and `0000000000000000`; Creative load never recomputes a content hash. A syntactically valid field edit is accepted without integrity evidence | Repair | P0 |
| PER-A1-009 | Creative document section version | Contract Risk | `restoreCreativeDocumentFromSaveSection` checks presence and field validity but has no zero/future-version refusal status. World Layout already applies the correct `0 < version <= current` law | Repair | P0 |
| PER-A1-010 | Load publication ordering | Required Adapter | AUT-005 installs the decoded document before publishing optional World Layout output; rejected open/install paths leave both live owners unchanged | Keep | P1 |
| PER-A1-011 | `clearToBlankScene` result | Required Adapter | AUT-005 returns the Facade install receipt; New callers clear history and reset editor state only after accepted installation | Keep | P1 |
| PER-A1-012 | Capture `ObjectSnapshotEntry` round-trip proof | Legacy Reachable | The comment claims a lossless round trip, but comparison covers only kind, transform position, bounds, path points, and moving-platform settings. It omits document settings, most object payloads, hierarchy, logic, voxels, terrain, recipes, annotations, materials, assets, and World Layout source | Repair | P1 |
| PER-A1-013 | World Layout codec and envelope transport | Canonical Owner | Source bytes are versioned, bounded, migrated from version 1, stored in the same durable envelope, rejected on corruption/future versions, and guarded against unsynchronized save | Keep | P0 |
| PER-A1-014 | Map diagnostic model and descriptor table | Canonical Owner | Every code has stable severity/code/object/fact data plus one tested area/title/remediation descriptor; the panel can focus valid object ids and reports truncation explicitly | Keep | P0 |
| PER-A1-015 | Creative document field coverage | Canonical Owner | Section build/restore covers all currently active `CreativeDocument` durable stores and resets loaded revision/dirty state; focused tests cover current and legacy object, terrain, recipe, annotation, and source records | Keep | P0 |
| PER-A1-016 | `readSaveFile` decode ownership | Canonical Owner | `readSaveFile` owns one decode, builds its record from that envelope, and returns the same envelope to catalog, identity, Creative load, and list consumers | Keep | P2 |
| PER-A1-017 | Controls persistence ownership | Ownership Undecided | `EditorControlsPersistence.cpp` is assigned here but owns input profile and playtest-window preferences; its behavioral owner is Interaction and Controls | Move | P2 |
| PER-A1-018 | Validation panel ownership | Ownership Undecided | The validation kernel and descriptor model are durable diagnostic contracts; `EditorMapValidationPanel.*` is an ImGui renderer and belongs to Editor Shell and Drafting UI | Move | P2 |
| PER-A1-019 | Package/import validation claim | Legacy Reachable | No package validator exists in the current Creative-only checkout. Static-mesh reference/collision/metadata checks are map validation; importer/catalog validation belongs to Assets and Object Composition | Move | P1 |
| PER-A1-020 | Department automated gate | Contract Risk | The listed four-test gate omits WorldService, document build/restore, dirty-state, World Layout codec/persistence, editor diagnostics, and desktop command tests that exercise this contract | Repair | P1 |

## Repair Order

1. **Keep Authoring publication identity singular.** AUT-002A makes
   same-document staging publish through one primitive, and AUT-002B makes the
   existing live document revision monotonic across replacement and history.
   Durable dirty state remains separate from publication freshness.
2. **Keep honest editor save semantics.** AUT-005 acknowledges only exact
   durable success; PER-001 preserves undo/redo and compares canonical durable
   document/source fingerprints rather than revision numbers. New/Open remain
   the only persistence commands that replace the history epoch.
3. **Keep load/new publication atomic and observable.** AUT-005 publishes
   decoded source only after document installation succeeds and makes New
   callers observe the installation receipt.
4. **Enforce compatibility and integrity.** Reject zero/future Creative section
   versions, run envelope compatibility in the Creative load path, hash the
   canonical Creative envelope, and reject valid-syntax tampering.
5. **Replace the capture proof.** Compare the canonical durable document
   section and encoded World Layout source, not a hand-selected object subset.
6. **Consolidate secondary plumbing.** Reuse one decoded envelope per file read
   and move controls/panel files to their behavioral departments.

## Compatibility Laws

- Current writes emit the current schema, Creative document section, and World
  Layout versions.
- Readers may migrate explicitly supported older versions.
- Version zero and versions newer than the current owner are rejected before
  publication.
- Unknown keys remain rejected by the strict ordered envelope codec. Evolution
  therefore requires a versioned reader branch; silent field loss is forbidden.
- A failed read, decode, compatibility check, hash check, section restore,
  source decode, or Facade install leaves the live document, source, history,
  active save id, and clean state unchanged.
- Save is not New/Open. It may establish a clean checkpoint but may not delete
  undo or redo history.

## Verification

Audit P1 completed on 2026-07-23.

- Source searches found four successful-save history clears, one production
  `lastSavedRevision` writer, no Creative-load compatibility call, a hardcoded
  zero Creative hash, and no Creative document section version guard.
- The active document field list was compared with section build/restore.
- World Layout persistence and map-diagnostic contracts were traced through
  their focused tests and production consumers.
- Production behavior was not changed during this audit.

AUT-005 implementation evidence recorded on 2026-07-23.

- Commit `7594b078` resolves PER-A1-002, PER-A1-004, PER-A1-010, and
  PER-A1-011.
- The 16/16 focused gate covers exact live acknowledgement, desktop/keyboard
  checkpoint parity, New/Open ordering, and rejected-operation atomicity.
- PER-A1-003 remained open at this checkpoint: successful save still cleared
  undo/redo.

PER-001 implementation evidence recorded on 2026-07-23.

- Commit `3c55c35a` resolves PER-A1-003 and PER-A1-005 and strengthens
  PER-A1-004.
- Canonical document identity reuses the complete durable
  `SaveCreativeDocumentSection`; dirty flags and live revision counters do not
  enter the fingerprint.
- Canonical World Layout identity reuses the versioned source codec. The saved
  fingerprint is session checkpoint state and is intentionally not restored by
  undo/redo snapshots.
- The 12/12 Persistence and Validation gate passes, including simultaneous
  undo/redo preservation, undo-away/redo-back cleanliness, alternate-branch
  revision alias rejection, desktop/keyboard parity, and all prior
  save/load/source/diagnostic tests.

PER-A1-016 implementation evidence recorded on 2026-07-24.

- `SaveFileReadResult` exposes the successful `SaveEnvelope`; encoded file bytes
  remain local to `readSaveFile`, which decodes once and builds the record before
  moving that same envelope into the result.
- Catalog projection, existing-save identity, and Creative load consume the
  returned envelope. `listSaveFiles` continues to consume the record from the
  same read result.
- The focused 4/4 save gate passes, including direct envelope field coverage and
  the existing malformed-input decode reason and codec status.
