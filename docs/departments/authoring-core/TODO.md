# Authoring Core TODO

Existing completion claims are inventory evidence, not acceptance. Each row
must be re-proved against the current checkout before it becomes Accepted.

| ID | Capability | Maturity | Delivery | Evidence | Next |
| --- | --- | --- | --- | --- | --- |
| AUT-001 | Shared semantic recipe boundary | Stable Recipe | Planned | Audit A1 found materialization/provenance live but all three generic `*WithHistory` apply APIs absent from product callers | Integrate the apply boundary or remove the unused application wrappers |
| AUT-002 | Atomic document mutation and rollback | Integrated | Planned | `DocumentMutation` is canonical; 20 direct staged assignments compete with five `commitStagedMutation` call sites, while Facade batch create uses same-document `installDocument` | Execute [AUT-002A](AUT-002A-LUNA.md), then [AUT-002B](AUT-002B-LUNA.md) |
| AUT-003 | Undo and redo transaction ownership | Integrated | Needs Audit | Fifty-three app starts already use the shared completion wrapper; 11 direct core starts cover sidecars or multi-phase operations | Compare predicates and consolidate only genuine policy differences after AUT-002 |
| AUT-004 | Generated-source provenance | Stable Recipe | Needs Audit | Operation records are live in pattern, asset, terrain, and World Layout paths | Prove edit, regenerate, detach, delete, undo, and redo for every source family |
| AUT-005 | World service and document replacement | Prototype | Needs Audit | WorldService is live and Facade install is the replacement boundary | Pin new, open, save, replacement reset, and failure atomicity in Persistence audit P1 |
| AUT-006 | Department ownership observability | Support | Accepted | Registry checker covers every tracked file; Audit A1 records classified findings and dispositions | Keep the audit current when ownership routes change |
| AUT-007 | Retire obsolete Facade compatibility state | Support | Accepted | Removed `State.hpp`, packet stubs, Facade mirror state, and `CreativeActiveIdentity`; canonical-state assertions pass | Re-audit if a compatibility mirror is proposed again |
| AUT-008 | Close mutable Facade document escape | Integrated | Accepted | Commits `3bae6e4e`, `93f10bdf`, `7b7e1f9c`, and `a568f149` remove all 47 mutable-accessor calls, preserve attached hierarchy relationships, and pass the independent clean 10/10 gate | Re-audit only if a mutable document escape or generic Facade mutation policy is proposed |
| AUT-009 | Reassign RoomBake adapter ownership | Support | Blocked | Adapter is live across preview, play, validation, and building traversal | Rule ownership after Rendering and Playtest audits |
