# Authoring Core TODO

Existing completion claims are inventory evidence, not acceptance. Each row
must be re-proved against the current checkout before it becomes Accepted.

| ID | Capability | Maturity | Delivery | Evidence | Next |
| --- | --- | --- | --- | --- | --- |
| AUT-001 | Shared semantic recipe boundary | Integrated | Accepted | Commit `96df5d86` retains shared planning, preview, fingerprints, provenance, and atomic publication while retiring three test-only generic `*WithHistory` wrappers; focused gates passed 4/4 and 10/10 | Re-audit only if a generic recipe API takes `CreativeAppState` or claims history ownership without the durable source sidecars |
| AUT-002 | Atomic document mutation and rollback | Integrated | Accepted | `57fe7800` routes staged publication through one document primitive; `aa0b7882` makes Facade replacement revisions branch-safe; clean focused gates passed 11/11 and 7/7 | Re-audit only if a whole-document publication or installation bypass is introduced |
| AUT-003 | Undo and redo transaction ownership | Integrated | Accepted | Commit `671d67c4` routes ordinary measurement and volume completion through the shared policy; 8 metadata-bearing starts and 2 sidecar/multi-phase starts remain explicit by design; clean focused gates passed 2/2 and 10/10 | Re-audit only if an ordinary edit bypasses `completeEditTransaction` or a new sidecar protocol is introduced |
| AUT-004 | Generated-source provenance | Integrated | Accepted | Commit `af45a7da` names the real durable source stores, records mixed World Layout reconciliation as `WorldLayout`, and proves regeneration, detach, delete, undo, and redo across World Layout, pattern, scatter, prefab, and terrain; focused gates passed 8/8 and 19/19 | Re-audit when a generated-source family or durable source store is added |
| AUT-005 | World service and document replacement | Prototype | Needs Audit | WorldService is live and Facade install is the replacement boundary | Pin new, open, save, replacement reset, and failure atomicity in Persistence audit P1 |
| AUT-006 | Department ownership observability | Support | Accepted | Registry checker covers every tracked file; Audit A1 records classified findings and dispositions | Keep the audit current when ownership routes change |
| AUT-007 | Retire obsolete Facade compatibility state | Support | Accepted | Removed `State.hpp`, packet stubs, Facade mirror state, and `CreativeActiveIdentity`; canonical-state assertions pass | Re-audit if a compatibility mirror is proposed again |
| AUT-008 | Close mutable Facade document escape | Integrated | Accepted | Commits `3bae6e4e`, `93f10bdf`, `7b7e1f9c`, and `a568f149` remove all 47 mutable-accessor calls, preserve attached hierarchy relationships, and pass the independent clean 10/10 gate | Re-audit only if a mutable document escape or generic Facade mutation policy is proposed |
| AUT-009 | Reassign RoomBake adapter ownership | Support | Blocked | Adapter is live across preview, play, validation, and building traversal | Rule ownership after Rendering and Playtest audits |
