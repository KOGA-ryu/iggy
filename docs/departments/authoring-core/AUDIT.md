# Authoring Core Audit

Audit A1 traced the current state-changing routes from editor input through
history, persistence, and scene projection. The audit is evidence-only: no
production behavior was changed.

## Route Map

| Workflow | Current route | Ruling |
| --- | --- | --- |
| Desktop object edit | ImGui panel -> `CreativeDesktopCommandId` -> `EditorDesktopCommands.cpp` -> domain dispatcher -> `EditorEdits.cpp` `*WithUndo` adapter -> explicit Facade/domain mutation contract -> history -> document revision -> scene cache | Connected through explicit single, batch, hierarchy-transform, and reattachment contracts; no mutable Facade document escape remains |
| Keyboard or controller edit | `InputRouter` -> `EditorCommandInput.cpp` or held-item dispatcher -> the same `*WithUndo`/tool kernels -> history -> document revision -> scene cache | Reuses many kernels, but bypasses the desktop semantic command dispatcher |
| World Layout edit | drafting UI -> World Layout command dispatcher -> source-store edit -> compile/reconcile -> `applyCreativeEditorWorldLayoutPlanWithHistory` -> Facade document install -> history sidecar -> scene cache | Valid dedicated source-authoring route; it must remain distinct because undo also restores the 2D source |
| Terrain generation edit | terrain UI/command -> `EditorTerrainGeneration.cpp` -> Facade terrain-operation mutation -> manually composed history transaction -> scene cache | Connected, but the advertised `applyCreativeTerrainRecipeWithHistory` route is not used by the product |
| Generic recipe apply | recipe plan -> `applyCreativeRecipeWithHistory` -> Facade atomic create -> history | Test-only in the product build; no `apps/iggy3d_creative` caller |
| Save/open/new | desktop or keyboard command -> `EditorPersistence.cpp` -> `WorldService` -> SaveBridge -> Facade install on open/new | Required persistence adapter; detailed save-contract review belongs to Persistence and Validation |
| 3D projection | document id/revision -> `CreativeEditorSceneCache` in `EditorScenePreview.cpp` -> RoomBake/render plans -> frame submission | Canonical downstream invalidation route |

## Findings

A low include count alone is not deletion evidence. Deletion findings below
require zero production callers or a superseded contract with a named
replacement.

| ID | Surface | Classification | Evidence | Disposition | Priority |
| --- | --- | --- | --- | --- | --- |
| AUT-A1-001 | `CreativeDocument` | Canonical Owner | Owns durable identity, objects, revision, dirty domains, and validated restore | Keep | P0 |
| AUT-A1-002 | `MutationApply` + `DocumentMutation` | Canonical Owner | Object mutation policy is separated from document lookup, hierarchy validation, rollback, dirty flags, and revision publication | Keep | P0 |
| AUT-A1-003 | `Facade` document/editor boundary | Canonical Owner | Owns the live document and canonical tool, selection, measurement, snap, and ghost state; narrow its mutable surface during AUT-008 | Keep | P0 |
| AUT-A1-004 | `Facade::documentForPersistence()` | Contract Risk | The 17 production and 30 test callers are migrated to explicit mutation, locked asset-bounds refresh, hierarchy-transform, and reattachment contracts; the mutable accessor is deleted and the final repository search is empty | Retired | P0 |
| AUT-A1-005 | Staged-document publication | Duplicate Implementation | Twenty direct `document = std::move(staged...)` publications coexist with five call sites of the documented one-revision `commitStagedMutation` primitive; Facade batch create also publishes a same-document edit through full `installDocument`; AUT-002A owns the repair | Consolidate | P0 |
| AUT-A1-006 | Editor history transaction composition | Required Adapter | Fifty-three ordinary app transaction call sites use `beginEditTransaction`; 8 direct starts attach required authoring-operation records and 2 own sidecar/multi-phase protocols. AUT-003 removed the plain measurement duplicate and routed volume completion through `completeEditTransaction` | Keep | P1 |
| AUT-A1-007 | Desktop semantic command dispatcher | Required Adapter | ImGui surfaces emit typed command ids into one headless dispatcher and then reuse domain kernels | Keep | P0 |
| AUT-A1-008 | Keyboard/controller command route | Contract Risk | `EditorCommandInput.cpp` and held-item paths invoke domain kernels directly rather than emitting the same semantic command ids as desktop UI; repair in Interaction and Controls after the mutation boundary is stable | Repair | P1 |
| AUT-A1-009 | Generic recipe apply APIs | Test-only Production | `applyCreativeRecipeWithHistory`, `applyCreativeTerrainRecipeWithHistory`, and `applyCreativeWorldLayoutPlanWithHistory` have no product-app callers; decide whether to integrate or remove the unused apply layers while retaining materialization and provenance | Investigate | P1 |
| AUT-A1-010 | Facade `State`, frame packet, and packet handler compatibility path | Test-only Production | AUT-007 removed `State.hpp`, the packet types, Facade stubs, mirror writes, and mirror assertions; tests now inspect the canonical typed states | Retired | P1 |
| AUT-A1-011 | `CreativeActiveIdentity` | Unreachable | AUT-007 removed the unread type and `CreativeAppState::identity` member | Retired | P1 |
| AUT-A1-012 | Facade `Stats` | Test-only Production | Counters have no product readers and are asserted only by tests; install, batch-create, and mutable escape routes do not form a complete product-command metric; decide whether deliberate diagnostics replace them | Investigate | P2 |
| AUT-A1-013 | World Layout source history and sidecar | Required Adapter | The app-specific wrapper records both document history and the 2D source snapshot, which the generic recipe helper cannot represent | Keep | P0 |
| AUT-A1-014 | `CreativeEditorSceneCache` revision key | Contract Risk | Document id/revision gates the room bake, terrain plans, placement clearance, and downstream preview caches, but history currently reinstalls old revisions and permits an alternate branch to reuse the same key; AUT-002B owns the revision-lineage repair | Repair | P0 |
| AUT-A1-015 | `creative/adapters/RoomBake*` ownership | Ownership Undecided | Thirteen adapter files do not mutate authored state; their consumers are preview, play, validation, and building traversal; rule their destination after Rendering and Playtest audits | Move | P2 |
| AUT-A1-016 | `WorldService` | Required Adapter | Sole create/open/save bridge for Creative documents and optional World Layout source; audit save atomicity in Persistence and Validation | Keep | P1 |
| AUT-A1-017 | `CreativeAppState.hpp` migration comments | Legacy Reachable | AUT-007 removed the stale `ProductAppWindowState` migration and mirror-discipline comments together with the retired identity member | Retired | P3 |

## Repair Order

1. **Retire dead compatibility state (complete).** AUT-007 removed
   `State.hpp`, the obsolete frame/packet types and Facade methods, the old
   mirror writes, the test-only assertions, and `CreativeActiveIdentity`.
   `Tool`, `TargetRef`, and the canonical typed state owners remain.
2. **Close the mutable document escape (complete).** AUT-008 added Facade
   entry points for generic single and atomic object mutation, a
   locked-object-safe asset-bounds refresh, and narrow hierarchy
   transform/reattachment operations. All 47 callers were migrated and
   `documentForPersistence()` was deleted. The accepted repair preserves an
   attached root's parent/socket during ordinary transform while reattachment
   still replaces the relationship atomically.
3. **Make same-document publication singular.** Execute
   [AUT-002A](AUT-002A-LUNA.md): make `commitStagedMutation` the only
   publication primitive for an edited copy of the current document, convert
   the 20 direct assignments and Facade batch-create path, and normalize every
   atomic operation to one live revision. Then execute
   [AUT-002B](AUT-002B-LUNA.md): preserve a monotonic live revision lineage
   across replacement, undo, redo, reset, and reinstall without adding a
   second publication stamp.
4. **Resolve the recipe claim.** Either route real product actions through the
   three generic `*WithHistory` APIs, or remove those unused application
   wrappers and state honestly that shared recipes own planning,
   materialization, fingerprints, and provenance while each durable source
   owns its own apply transaction.
5. **Audit transaction exceptions (complete).** AUT-003 keeps the existing
   shared completion helper, routes ordinary measurement and volume completion
   through it, and leaves 10 justified direct starts: 8 attach authoring
   operation metadata, World Layout owns its document/source sidecar, and map
   regeneration owns rollback if history recording fails. Authored asset
   cancellation remains explicit before durable-write, source, provenance, or
   operation-record failure. No generic helper was added over these distinct
   protocols.
6. **Converge interaction routes.** Once the mutation contract is stable, make
   desktop, keyboard, and controller actions resolve to the same semantic
   operation ids. Input-specific gesture state remains in Interaction and
   Controls.

## Deletion Ruling

Retired by AUT-007:

- `src/app/iggy3d/creative/State.hpp`.
- `PacketKind`, `FrameRef`, `Flags`, `FramePacket`, and `Packet` from
  `Core.hpp`.
- `Facade::beginFrame`, `Facade::handle`, `Facade::state`, and the duplicate
  `State state_` mirror.
- `CreativeActiveIdentity` and `CreativeAppState::identity`.

## AUT-008 Evidence

AUT-008 accepted across commits `3bae6e4e`, `93f10bdf`, `7b7e1f9c`, and
`a568f149`:

- `Facade` owns explicit single-object mutation, atomic batch mutation,
  locked-object-safe asset-bounds refresh, hierarchy transform, and hierarchy
  reattachment entry points.
- `HierarchyTransform` owns resolve-once validation, staged hierarchy edits,
  deterministic transform ordering, one-revision publication, and
  phase-specific failure receipts.
- Ordinary absolute transform temporarily stages an externally attached root
  as detached only because the placement kernel rejects external parents, then
  restores the original parent/socket before publication. Reattachment retains
  detach-before-transform behavior and installs the requested relationship.
- All 47 mutable-accessor callers were migrated: 17 production calls and 30
  test calls. No app-facing mutation options or local staged publication remain
  in the retired editor implementations.
- Tests pin single/batch mutation, rollback, no-change, missing-object
  rejection, locked refresh, hierarchy position/scale/three-axis orientation,
  attached parent/socket preservation, stale reattachment, target-inside-source
  rejection, one undo record, one live revision, and Facade editor-state
  preservation.
- Sol independently configured a clean build at
  `/tmp/iggy3d-aut008-review-sol`, built `i3dc` and all ten targeted tests, and
  passed the exact 10/10 CTest gate. The earlier empty-archive blocker claim was
  stale build-tree state, not a source or contract blocker.

Not deletion candidates:

- `CreativeDocument`, mutation, history, Facade, or WorldService.
- Creative recipe materialization, fingerprints, and provenance.
- RoomBake adapters; their department assignment is questionable, but their
  output is live.

Deferred decision:

- Facade `Stats`. It is currently test-only and incomplete as a command metric,
  but should be removed only after replacing any tests that use it as indirect
  behavioral evidence.

## Verification

Audit A1 completed on 2026-07-23.

- `python3 tools/repo_departments.py check` passed with 10 departments, 1,481
  governed files, 90 work items, 10 manual tests, and 17 audit findings.
- The Audit A1 baseline target set built with `CCACHE_DISABLE=1` and passed
  9/9 focused tests.
- No production code or runtime behavior changed during this audit.

AUT-007 completed on 2026-07-23.

- `i3dc` built with `CCACHE_DISABLE=1`; no window was launched.
- The ten targets currently listed in `TESTING.md` built successfully.
- The matching focused CTest gate passed 10/10.
- Repository searches found no remaining `State.hpp`, Facade compatibility
  accessor, frame/packet stub, mirror member, or `CreativeActiveIdentity`
  reference.

AUT-008 completed on 2026-07-23.

- A clean build produced `i3dc`, `creative_facade_mutation_tests`,
  `creative_facade_tests`, `creative_document_create_tests`,
  `creative_editor_attachment_tests`, `creative_editor_asset_reload_tests`,
  `creative_editor_placement_tests`, `creative_authored_asset_tests`,
  `creative_desktop_ui_command_tests`, `creative_world_layout_tests`, and
  `creative_world_layout_diagnostics_tests`.
- The exact focused CTest expression passed 10/10.
- Repository gates found no `documentForPersistence`, no app-facing
  `CreativeDocumentMutationOptions`, and no local staged assignment in the
  retired editor hierarchy implementations.
- `tools/repo_departments.py check` passed with 10 departments, 1,482 governed
  files, 93 work items, 10 manual tests, and 37 audit findings.

AUT-002 completed on 2026-07-23.

- AUT-002A commit `57fe7800` routes all staged same-document publication
  through `CreativeDocument::commitStagedMutation`; the clean focused gate
  passed 11/11.
- AUT-002B commit `aa0b7882` gives `Facade` a transient live-revision
  high-water mark and narrowly rebases later whole-document replacements.
  Initial install remains unchanged, while undo, redo, reset/reinstall,
  alternate branches, copied/moved Facades, rejection atomicity, frozen
  reattachment plans, and map-validation cache staleness are pinned.
- The cache and stale-plan proofs use the existing document revision. No
  publication id, generation, epoch, persistence field, or source-layout
  revision contract was added.
- A clean cache-disabled build passed the exact 7/7 AUT-002B CTest gate.

AUT-003 completed on 2026-07-23.

- Commit `671d67c4` replaces the plain measurement transaction duplicate with
  `beginEditTransaction` plus `completeEditTransaction`, while retaining its
  user-visible history failure reason.
- Volume operations retain their exact destructive authoring-operation record
  but now use the shared changed/rejected completion policy.
- The remaining direct starts are classified and intentional: 8 operation
  metadata owners and 2 sidecar/multi-phase owners. Direct cancellation outside
  the shared helper is limited to authored-asset preconditions, World Layout
  source history, and map-regeneration rollback.
- Focused placement and desktop command tests passed 2/2. The full Authoring
  Core automated gate passed 10/10.
- Five older assertions discovered by the gate now pin the AUT-002B law:
  undo/redo restore exact content while advancing, rather than rewinding, the
  live document revision.
