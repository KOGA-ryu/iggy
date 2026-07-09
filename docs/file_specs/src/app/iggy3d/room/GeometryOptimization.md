# File Spec

Files: `src/app/iggy3d/room/GeometryOptimization.hpp`, `src/app/iggy3d/room/GeometryOptimization.cpp`

Verified at: `ac496769`

## Owns

- App-side room geometry optimization estimate report for authored room documents.
- `ProductRoomGeometryOptimizationReport`.
- Floor-rectangle merge estimation and wall-run merge estimation used by room-editor previews.
- Naive versus optimized draw/triangle count and avoided-count summaries.

## Does Not Own

- Actual renderer mesh generation or Vulkan draw submission.
- Editable room mutation.
- Runtime collision, physics, or save persistence.
- Creative document object descriptors.

## Reads

- `EditableRoomDocument` floors, walls, objects, semantics, locked/hidden flags, story index, sizes, and endpoints.
- Quantized floor/wall geometry facts for grouping comparable merge candidates.

## Writes / Mutates

- Builds and returns `ProductRoomGeometryOptimizationReport`.
- Does not mutate the authored room document.

## Calls Out To / Wires Out To

- Consumed by room-editor preview to compare before/after optimization metrics.
- Uses local estimate helpers only; does not call render backend code.

## Called By / Entry Points

- `buildProductRoomGeometryOptimizationReport`.
- `src/app/iggy3d/room_editor/Preview.cpp` computes before/after preview reports.
- Focused proof: `rg -n "buildProductRoomGeometryOptimizationReport|ProductRoomGeometryOptimizationReport|product_room_geometry|optimizedFloorRectCount|optimizedWallRunCount" src/app/iggy3d tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Null document returns `product_room_geometry_document_missing`.
- Valid document returns `product_room_geometry_optimization_ready`.
- Non-grid-aligned or invalid floors/walls are counted as unmerged, not dropped.
- Avoided counts are saturating and never underflow.
- This is an estimate/report surface; it must not become renderer truth.

## Tests / Proof Commands

- `product_room_geometry_optimization_tests` covers null input, floor rectangle merging, wall run merging, non-mergeable cases, and invalid geometry handling.
- `product_room_editor_preview_tests` covers the preview consumer path.
- `rg -n "product_room_geometry_optimization_tests|product_room_editor_preview_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/render/vulkan/BufferImageResources.cpp` unless actual renderer mesh emission changes.
- `src/app/iggy3d/room_editor/Preview.*` unless preview metrics change.
- `src/content/authoring/EditableRoomDocument.*` unless document geometry schema changes.

## Update When

- Optimization report fields, floor/wall merge estimation rules, triangle/draw-count assumptions, or room-editor preview metric ownership changes.

## Do Not Update When

- Only actual renderer optimization code changes without changing this app-side estimate contract.
