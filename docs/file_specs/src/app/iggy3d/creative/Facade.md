# File Spec

Files: `src/app/iggy3d/creative/Facade.hpp`, `src/app/iggy3d/creative/Facade.cpp`

Verified at: `d40a476b`

## Owns

- Editor-facing aggregate over creative document state and transient tool, selection, measurement, snap, ghost, stats, and move-drag state.
- Tool dispatch receipts and mutation receipts for selection visibility/lock toggles, object create/remove, document install, atomic batch create, and move-drag lifecycle.
- Document install/reset behavior that clears transient editor state.

## Does Not Own

- Filesystem paths, save ids, launch/open/save policy, or active creative identity.
- Window input ownership, hit testing, renderer overlay presentation, or receipt field serialization.
- Durable document schema; that lives in the document and save layers.

## Reads

- Frame/tool input packets, creative document requests, current document content, selection state, snap settings, and tool state.
- Caller-provided documents for install and persistence access.

## Writes / Mutates

- Internal `CreativeDocument`, `State`, stats, tool, selection, measurement, snap, ghost, and move-drag members.
- Returns receipt packets for all public mutations.

## Calls Out To / Wires Out To

- `CreativeDocument` create/remove/restore APIs.
- Creative tool dispatch helpers, snap helpers, mutation metrics, and UI model build helpers.

## Called By / Entry Points

- Creative world operations install documents after create/open.
- Creative UI command frame mutates facade state from semantic UI commands.
- Tests exercise facade state, document install, create/remove, dirty, and tool behavior.
- Grep proof: `rg -n "Facade::installDocument|Facade::createDocumentObject|Facade::removeDocumentObject|dispatchToolInput|buildUiModel" src tests cmake`.

## Invariants

- `installDocument(...)` rejects invalid documents and clears selection, measurement, ghost, tool pointer, and active tool state on accepted install.
- Facade remains the app/editor boundary over document mutation; callers should not duplicate document mutation policy.
- `documentForPersistence()` exposes document state for save only; filesystem ownership stays outside.
- Move-drag receipts preserve begin/preview/commit/cancel observability.

## Tests / Proof Commands

- `rg -n "creative_facade_tests|creative_facade_mutation_tests|creative_core_tests|creative_document_create_tests|creative_document_remove_tests" cmake tests`.
- `rg -n "installDocument|createDocumentObjectsAtomically|toggleSelectedObject" src tests`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/document/Document.*` unless durable document mutation rules change.
- `src/app/iggy3d/creative/bridge/UiCommandFrame.*` unless UI command dispatch changes.
- `src/app/iggy3d/creative/CreativeWorldOperations.*` unless launch/open install behavior changes.

## Update When

- Facade public state, receipts, tool dispatch, document install/reset, or document mutation wrapper contracts change.

## Do Not Update When

- Only save path or product launch state changes outside the facade.
