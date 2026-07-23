# Authoring Core TODO

Existing completion claims are inventory evidence, not acceptance. Each row
must be re-proved against the current checkout before it becomes Accepted.

| ID | Capability | Maturity | Delivery | Evidence | Next |
| --- | --- | --- | --- | --- | --- |
| AUT-001 | Shared semantic recipe boundary | Stable Recipe | Planned | Audit A1 found materialization/provenance live but all three generic `*WithHistory` apply APIs absent from product callers | Integrate the apply boundary or remove the unused application wrappers |
| AUT-002 | Atomic document mutation and rollback | Integrated | Planned | `DocumentMutation` is canonical; 20 direct staged assignments compete with three `commitStagedMutation` calls | Make same-document publication singular and pin one revision per atomic operation |
| AUT-003 | Undo and redo transaction ownership | Integrated | Planned | History is live; 53 editor transaction starts across 25 files manually compose completion | Add one explicit transaction-completion owner, preserving sidecar support |
| AUT-004 | Generated-source provenance | Stable Recipe | Needs Audit | Operation records are live in pattern, asset, terrain, and World Layout paths | Prove edit, regenerate, detach, delete, undo, and redo for every source family |
| AUT-005 | World service and document replacement | Prototype | Needs Audit | WorldService is live and Facade install is the replacement boundary | Pin new, open, save, replacement reset, and failure atomicity in Persistence audit P1 |
| AUT-006 | Department ownership observability | Support | Accepted | Registry checker covers every tracked file; Audit A1 records classified findings and dispositions | Keep the audit current when ownership routes change |
| AUT-007 | Retire obsolete Facade compatibility state | Support | Accepted | Removed `State.hpp`, packet stubs, Facade mirror state, and `CreativeActiveIdentity`; canonical-state assertions pass | Re-audit if a compatibility mirror is proposed again |
| AUT-008 | Close mutable Facade document escape | Integrated | Planned | Seventeen mutating app calls across five files use `documentForPersistence` | Add narrow Facade/domain operations and delete the escape |
| AUT-009 | Reassign RoomBake adapter ownership | Support | Blocked | Adapter is live across preview, play, validation, and building traversal | Rule ownership after Rendering and Playtest audits |
