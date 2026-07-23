# Authoring Core Audit

Audit A1 traced the current state-changing routes from editor input through
history, persistence, and scene projection. The audit is evidence-only: no
production behavior was changed.

## Route Map

| Workflow | Current route | Ruling |
| --- | --- | --- |
| Desktop object edit | ImGui panel -> `CreativeDesktopCommandId` -> `EditorDesktopCommands.cpp` -> domain dispatcher -> `EditorEdits.cpp` `*WithUndo` adapter -> `DocumentMutation`/Facade -> history -> document revision -> scene cache | Connected, but the adapter reaches the document through a mutable Facade escape |
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
| AUT-A1-004 | `Facade::documentForPersistence()` | Contract Risk | Seventeen production calls in five editor files use it for object/path/asset/structural mutation; no production persistence path uses it; repair callers before deleting the escape | Repair | P0 |
| AUT-A1-005 | Staged-document publication | Duplicate Implementation | Twenty `document = std::move(staged)` publications coexist with only three uses of the documented one-revision `commitStagedMutation` primitive; Facade batch create publishes through full `installDocument` instead | Consolidate | P0 |
| AUT-A1-006 | Editor history transaction composition | Contract Risk | Fifty-three `beginEditTransaction` calls are distributed across 25 editor files; callers manually decide cancel/commit and whether a changed mutation requires a history record; preserve domain receipts during consolidation | Consolidate | P0 |
| AUT-A1-007 | Desktop semantic command dispatcher | Required Adapter | ImGui surfaces emit typed command ids into one headless dispatcher and then reuse domain kernels | Keep | P0 |
| AUT-A1-008 | Keyboard/controller command route | Contract Risk | `EditorCommandInput.cpp` and held-item paths invoke domain kernels directly rather than emitting the same semantic command ids as desktop UI; repair in Interaction and Controls after the mutation boundary is stable | Repair | P1 |
| AUT-A1-009 | Generic recipe apply APIs | Test-only Production | `applyCreativeRecipeWithHistory`, `applyCreativeTerrainRecipeWithHistory`, and `applyCreativeWorldLayoutPlanWithHistory` have no product-app callers; decide whether to integrate or remove the unused apply layers while retaining materialization and provenance | Investigate | P1 |
| AUT-A1-010 | Facade `State`, frame packet, and packet handler compatibility path | Test-only Production | AUT-007 removed `State.hpp`, the packet types, Facade stubs, mirror writes, and mirror assertions; tests now inspect the canonical typed states | Retired | P1 |
| AUT-A1-011 | `CreativeActiveIdentity` | Unreachable | AUT-007 removed the unread type and `CreativeAppState::identity` member | Retired | P1 |
| AUT-A1-012 | Facade `Stats` | Test-only Production | Counters have no product readers and are asserted only by tests; install, batch-create, and mutable escape routes do not form a complete product-command metric; decide whether deliberate diagnostics replace them | Investigate | P2 |
| AUT-A1-013 | World Layout source history and sidecar | Required Adapter | The app-specific wrapper records both document history and the 2D source snapshot, which the generic recipe helper cannot represent | Keep | P0 |
| AUT-A1-014 | `CreativeEditorSceneCache` revision key | Required Adapter | Document id/revision gates the room bake, terrain plans, placement clearance, and downstream preview caches; add regression pins against stale publication during repair | Keep | P0 |
| AUT-A1-015 | `creative/adapters/RoomBake*` ownership | Ownership Undecided | Thirteen adapter files do not mutate authored state; their consumers are preview, play, validation, and building traversal; rule their destination after Rendering and Playtest audits | Move | P2 |
| AUT-A1-016 | `WorldService` | Required Adapter | Sole create/open/save bridge for Creative documents and optional World Layout source; audit save atomicity in Persistence and Validation | Keep | P1 |
| AUT-A1-017 | `CreativeAppState.hpp` migration comments | Legacy Reachable | Comments still describe a retired `ProductAppWindowState` additive migration and mirror discipline that no longer exists; repair them with the identity deletion | Repair | P3 |

## Repair Order

1. **Retire dead compatibility state (complete).** AUT-007 removed
   `State.hpp`, the obsolete frame/packet types and Facade methods, the old
   mirror writes, the test-only assertions, and `CreativeActiveIdentity`.
   `Tool`, `TargetRef`, and the canonical typed state owners remain.
2. **Close the mutable document escape.** Add Facade entry points for generic
   single and atomic object mutation. Route the ordinary mutation callers
   through them. Give reattachment, hierarchy transform, and structural span
   editing narrow domain entry points rather than a callback that exposes the
   whole document. Delete `documentForPersistence()` once its tests use real
   setup/install paths.
3. **Make same-document publication singular.** Define
   `commitStagedMutation` as the only publication primitive for a staging copy
   of the current document. Convert the 20 direct move-assignments and the
   Facade batch-create path. Reserve `installDocument` for new/open/history or
   source-reconciliation replacement where transient-state reset is explicit.
4. **Resolve the recipe claim.** Either route real product actions through the
   three generic `*WithHistory` APIs, or remove those unused application
   wrappers and state honestly that shared recipes own planning,
   materialization, fingerprints, and provenance while each durable source
   owns its own apply transaction.
5. **Centralize transaction completion.** Keep explicit begin/apply/receipt
   phases, but move the repeated changed/cancel/commit decision and operation
   metadata into one Authoring Core helper. Preserve World Layout sidecars as a
   supported extension.
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
