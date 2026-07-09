# File Spec

Files: `src/app/iggy3d/creative/world/DocumentSection.hpp`, `src/app/iggy3d/creative/world/DocumentSection.cpp`

Verified at: `7c9e712f`

## Owns

- Conversion between `creative::CreativeDocument` and `SaveCreativeDocumentSection`.
- `ProductCreativeDocumentSectionStatus`, receipt packets, build results, and restore results.
- Save/restore validation mapping for document id, units, grid, snap, world bounds, objects, object ids, parent ids, path points, and next object id.

## Does Not Own

- Save file encoding/decoding.
- Creative document mutation rules outside restore validation.
- Product save-slot/catalog flow.
- Runtime session or active-room creation from restored documents.

## Reads

- Creative document settings, object records, transforms, bounds, tags, path points, parent ids, and next object id.
- `SaveCreativeDocumentSection` fields from `runtime/save/SaveEnvelope.hpp`.
- Creative document restore status from document-layer validation.

## Writes / Mutates

- Builds `SaveCreativeDocumentSection` and conversion receipts.
- Builds restored `creative::CreativeDocument` and restore receipts.
- Does not mutate the input document or save section.

## Calls Out To / Wires Out To

- Uses creative object-kind serialization/parsing helpers.
- Uses creative document snap and restore validation helpers.
- Called by app save bridge when saving/loading CreativeDocument saves.

## Called By / Entry Points

- `toString(ProductCreativeDocumentSectionStatus)`.
- `buildSaveCreativeDocumentSection`.
- `restoreCreativeDocumentFromSaveSection`.
- `src/app/iggy3d/save/SaveBridge.cpp` for save/load integration.
- Focused proof: `rg -n "buildSaveCreativeDocumentSection|restoreCreativeDocumentFromSaveSection|ProductCreativeDocumentSection|SaveCreativeDocumentSection|creative_document_section" src/app/iggy3d src/runtime tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Converted sections use `kSaveCreativeDocumentSectionVersion`.
- Missing or invalid section data produces explicit failure statuses and reason codes.
- Restore rejects invalid object ids, duplicate object ids, and next-object ids that do not advance beyond existing ids.
- Snap/grid/world-bounds failures stay distinguishable in receipts.
- Conversion code stays app-save adapter code, not creative document source truth.

## Tests / Proof Commands

- `creative_document_save_section_tests` covers build/restore conversion and invalid section cases.
- `save_creative_document_section_tests` covers save-codec section persistence.
- `product_save_bridge_tests`, `creative_world_service_tests`, and `product_creative_world_launch_tests` cover product save/open consumers.
- `rg -n "creative_document_save_section_tests|save_creative_document_section_tests|product_save_bridge_tests|creative_world_service_tests|product_creative_world_launch_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/runtime/save/SaveEnvelope.hpp` and `src/runtime/save/SaveCodec.*` unless save schema changes.
- `src/app/iggy3d/creative/document/Document.*` unless restore validation status changes.
- `src/app/iggy3d/save/SaveBridge.*` unless save/load adapter flow changes.

## Update When

- Creative save-section schema, conversion validation, receipt statuses, or save-bridge ownership changes.

## Do Not Update When

- Only UI, active-room rebuild, or non-persistent creative authoring behavior changes.
