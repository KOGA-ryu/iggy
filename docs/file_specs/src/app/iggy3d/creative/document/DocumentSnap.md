# File Spec

Files: `src/app/iggy3d/creative/document/DocumentSnap.hpp`, `src/app/iggy3d/creative/document/DocumentSnap.cpp`

Verified at: `1ffca70b`

## Owns

- Creative document snap settings and snapping receipts.
- Snap mode, axis mask constants, point/bounds packets, settings packet, and receipt packet.
- Settings validation for disabled/grid mode, known axes, and positive finite per-axis steps.
- Point and bounds snapping through the core snap scalar kernel.

## Does Not Own

- Core scalar snap math.
- Creative tool ghost projection or placement policy.
- Document storage of snap settings.
- Save/load section encoding of snap settings.
- UI controls for changing snap settings.

## Reads

- Snap settings mode, axes, steps, and origins.
- Input point or bounds packets.
- Core `snapScalarToGrid(...)` result.

## Writes / Mutates

- Writes `CreativeDocumentSnapReceipt` with requested/accepted/settings/snapped/changed/status fields and original/snapped data.
- Normalizes snapped bounds min/max after snapping.
- Does not mutate `CreativeDocument`; callers store settings separately.

## Calls Out To / Wires Out To

- Calls `iggy3d::snapScalarToGrid(...)`.
- Used by `CreativeDocument` for snap setting validation and by creative placement/tooling for snapped positions and bounds.
- Save/load document sections serialize and restore these settings.

## Called By / Entry Points

- `makeDefaultCreativeDocumentSnapSettings()`.
- `isValidCreativeDocumentSnapSettings(...)`.
- `snapCreativeDocumentScalar(...)`.
- `snapCreativeDocumentPoint(...)`.
- `snapCreativeDocumentBounds(...)`.
- Grep proof: `rg -n "CreativeDocumentSnap|makeDefaultCreativeDocumentSnapSettings|snapCreativeDocumentPoint|snapCreativeDocumentBounds|isValidCreativeDocumentSnapSettings" src/app tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp}'`.

## Invariants

- Disabled mode is valid and accepted but does not snap.
- Unknown axis bits make settings invalid.
- No-axis grid mode is valid and accepted but does not snap.
- Only active axes require valid positive finite steps.
- Receipt status, reason code, and message stay synchronized.
- Bounds snapping must normalize inverted min/max after independent point snapping.

## Tests / Proof Commands

- `creative_document_snap_tests`.
- `creative_document_mutation_tests`.
- `creative_document_save_section_tests`.
- `product_save_bridge_tests`.
- `rg -n "creative_document_snap_tests|creative_document_mutation_tests|creative_document_save_section_tests|product_save_bridge_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/core/math/Snap.*` unless scalar snap math changes.
- `src/app/iggy3d/creative/document/Document.*` unless document-level setting storage changes.
- `src/app/iggy3d/creative/world/DocumentSection.*` unless save/load encoding changes.
- `src/app/iggy3d/creative/tools/Placement.*` unless placement consumption changes.

## Update When

- Snap modes, axis masks, settings validation, receipt semantics, point/bounds snapping, or core snap wiring changes.

## Do Not Update When

- Only UI presentation or save catalog behavior changes without altering snap settings or snap results.
