# Persistence and Validation Audit

Audit P1 traced the Creative save lifecycle from editor commands through
`EditorPersistence`, `WorldService`, `SaveBridge`, the envelope codec, durable
file replacement, document restore, and the map-validation presentation. It
also compared the intended clean/dirty and compatibility laws with history
branching and the actual app call sites.

## Route Map

| Workflow | Current route | Ruling |
| --- | --- | --- |
| Desktop save/save-as | menu command -> desktop dispatcher -> `saveStandaloneScene` -> `saveCreativeWorld` -> `writeCreativeDocumentSaveDurably` -> durable envelope write | Durable file replacement is real, but app lifecycle effects are wrong: the live document stays dirty and undo is erased |
| Keyboard save | semantic input action -> `EditorCommandInput` -> the same save adapter | Uses the same I/O kernel, but does not update the desktop revision checkpoint and also erases undo |
| Open/startup load | `loadStandaloneScene` -> `openCreativeWorld` -> `loadCreativeDocumentSave` -> section restore -> Facade install -> World Layout install | Document and source decode together, but the app publishes the source output before document installation succeeds |
| New document | desktop/keyboard command -> `clearToBlankScene` -> Facade install -> editor reset | The blank document is valid, but the adapter discards its install receipt and callers report success unconditionally |
| Creative section persistence | `CreativeDocument` -> `buildSaveCreativeDocumentSection` -> `SaveEnvelope` -> strict ordered codec -> `restoreCreativeDocumentFromSaveSection` | Active durable document fields are covered and legacy migrations are substantial; section-version refusal is missing |
| World Layout source persistence | versioned World Layout codec -> optional section in the same envelope -> decode before world open is accepted | Strong atomic transport and explicit unsupported-version refusal; keep |
| Map validation | document/catalog -> bounded typed validation -> cached result -> descriptor table -> actionable panel row/focus command | The diagnostic contract is strong; only its revision-only cache key is unsound after an undo branch |

## Findings

| ID | Surface | Classification | Evidence | Disposition | Priority |
| --- | --- | --- | --- | --- | --- |
| PER-A1-001 | `WorldService` + `SaveBridgeCreative` + `DocumentSection` + `SaveCodec` | Canonical Owner | These are the only Creative world create/open/save, durable section conversion, and envelope-codec routes | Keep | P0 |
| PER-A1-002 | `saveStandaloneScene` live clean-state acknowledgement | Contract Risk | The adapter copies `facade.document()` because `saveCreativeWorld` drains its mutable input. A successful app save therefore drains only the copy; the live document retains every dirty domain | Repair | P0 |
| PER-A1-003 | Save and Save As history behavior | Contract Risk | Desktop Save, Desktop Save As, keyboard Save, and capture Save all call `clearEditHistory` after success. `creative_desktop_ui_command_tests` explicitly pins this behavior even though save is a checkpoint, not a document replacement | Repair | P0 |
| PER-A1-004 | Desktop `lastSavedRevision` checkpoint | Duplicate Implementation | The field is UI-owned, has one production writer after desktop saves, is not updated by keyboard saves, and is compared every frame. Revision equality also aliases a different branch after undo -> alternate edit | Consolidate | P0 |
| PER-A1-005 | World Layout `savedRevision` dirty test | Contract Risk | Dirty is `revision != savedRevision`. Undo to an older revision followed by a different edit can recreate the saved revision number with different source content and report clean | Repair | P0 |
| PER-A1-006 | Revision-only cache and stale-plan keys | Contract Risk | Scene, clearance, terrain, desktop-model, validation, and several preview caches compare document id/revision. History restores old revisions, so an alternate branch can reuse a revision for different content and leave caches or plans falsely current | Repair | P0 |
| PER-A1-007 | Creative compatibility enforcement | Contract Risk | `loadCreativeDocumentSave` decodes and restores directly. Unlike runtime `SaveLoad`, it never calls `checkSaveCompatibility`, so schema/runtime/package/scenario policy is not enforced by the Creative open path | Repair | P0 |
| PER-A1-008 | Creative save integrity hash | Contract Risk | `SaveBridgeCreative` writes `savedStateHash=0` and `0000000000000000`; Creative load never recomputes a content hash. A syntactically valid field edit is accepted without integrity evidence | Repair | P0 |
| PER-A1-009 | Creative document section version | Contract Risk | `restoreCreativeDocumentFromSaveSection` checks presence and field validity but has no zero/future-version refusal status. World Layout already applies the correct `0 < version <= current` law | Repair | P0 |
| PER-A1-010 | Load publication ordering | Contract Risk | `loadStandaloneScene` writes the caller's World Layout output before `Facade::installDocument`; an install rejection can expose new source beside the old live document | Repair | P1 |
| PER-A1-011 | `clearToBlankScene` result | Contract Risk | The helper discards the Facade install receipt and returns `void`; New callers clear history and report success without observing installation | Repair | P1 |
| PER-A1-012 | Capture `ObjectSnapshotEntry` round-trip proof | Legacy Reachable | The comment claims a lossless round trip, but comparison covers only kind, transform position, bounds, path points, and moving-platform settings. It omits document settings, most object payloads, hierarchy, logic, voxels, terrain, recipes, annotations, materials, assets, and World Layout source | Repair | P1 |
| PER-A1-013 | World Layout codec and envelope transport | Canonical Owner | Source bytes are versioned, bounded, migrated from version 1, stored in the same durable envelope, rejected on corruption/future versions, and guarded against unsynchronized save | Keep | P0 |
| PER-A1-014 | Map diagnostic model and descriptor table | Canonical Owner | Every code has stable severity/code/object/fact data plus one tested area/title/remediation descriptor; the panel can focus valid object ids and reports truncation explicitly | Keep | P0 |
| PER-A1-015 | Creative document field coverage | Canonical Owner | Section build/restore covers all currently active `CreativeDocument` durable stores and resets loaded revision/dirty state; focused tests cover current and legacy object, terrain, recipe, annotation, and source records | Keep | P0 |
| PER-A1-016 | `readSaveFile` decode ownership | Duplicate Implementation | `readSaveFile` decodes for validation, then each SaveBridge load/identity path decodes the same bytes again because the read result does not expose the decoded envelope | Consolidate | P2 |
| PER-A1-017 | Controls persistence ownership | Ownership Undecided | `EditorControlsPersistence.cpp` is assigned here but owns input profile and playtest-window preferences; its behavioral owner is Interaction and Controls | Move | P2 |
| PER-A1-018 | Validation panel ownership | Ownership Undecided | The validation kernel and descriptor model are durable diagnostic contracts; `EditorMapValidationPanel.*` is an ImGui renderer and belongs to Editor Shell and Drafting UI | Move | P2 |
| PER-A1-019 | Package/import validation claim | Legacy Reachable | No package validator exists in the current Creative-only checkout. Static-mesh reference/collision/metadata checks are map validation; importer/catalog validation belongs to Assets and Object Composition | Move | P1 |
| PER-A1-020 | Department automated gate | Contract Risk | The listed four-test gate omits WorldService, document build/restore, dirty-state, World Layout codec/persistence, editor diagnostics, and desktop command tests that exercise this contract | Repair | P1 |

## Repair Order

1. **Establish unambiguous Authoring publication identity.** Add a
   non-serialized live publication stamp that cannot alias after undo and an
   alternate edit. Make same-document staging publish through one primitive,
   then migrate cache and stale-plan freshness checks. Durable dirty state is a
   separate concern and must not use the publication stamp.
2. **Make editor save semantics honest.** A successful save acknowledges the
   live document's dirty domains, preserves undo/redo, marks only the current
   World Layout source state clean, and makes desktop and keyboard paths observe
   the same clean state. New/Open still clear history.
3. **Make load/new publication atomic and observable.** Publish decoded source
   only after document installation succeeds and return receipts instead of
   unconditional success.
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
