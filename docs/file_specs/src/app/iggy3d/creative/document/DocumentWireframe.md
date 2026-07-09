# File Spec

Files: `src/app/iggy3d/creative/document/DocumentWireframe.hpp`, `src/app/iggy3d/creative/document/DocumentWireframe.cpp`

Verified at: `7b40370f`

## Owns

- CreativeDocument object wireframe item projection.
- Wireframe status, item kind, style, segment status, and segment kind enums.
- Conversion from spatial projection receipts into wireframe items.
- Conversion from wireframe items into line/box-edge segment lists.
- Wireframe receipts and segment receipts.

## Does Not Own

- Spatial projection cell math.
- Product frame routing or render debug-line conversion.
- Renderer/Vulkan geometry.
- Document mutation or object descriptor table contents.
- Editable room block overlays.

## Reads

- `CreativeDocument` or caller-provided object span.
- `CreativeSpatialProjectionRequest`.
- Object visibility, kind, bounds, transform, and path points.
- Projection and occupancy policies from spatial projection helpers.

## Writes / Mutates

- Builds `CreativeDocumentWireframeDrawList` and `CreativeDocumentWireframeSegmentList`.
- Writes wireframe and segment receipts.
- Does not mutate creative documents, objects, or projection requests.

## Calls Out To / Wires Out To

- Calls `projectionProfileForObject(...)` and `projectObjectToGrid(...)`.
- Consumed by product creative wireframe frame before debug-line rendering.
- Segment output is consumed by creative wireframe debug-line conversion.

## Called By / Entry Points

- `buildCreativeDocumentWireframeList(...)`.
- `buildCreativeObjectWireframeList(...)`.
- `buildCreativeDocumentWireframeSegments(...)`.
- `wireframeStyleForOccupancy(...)`, `wireframeItemKindForProjection(...)`, and `toString(...)` helpers.
- Grep proof: `rg -n "CreativeDocumentWireframe|buildCreativeDocumentWireframeList|buildCreativeDocumentWireframeSegments|wireframeStyleForOccupancy" src/app tests/unit cmake/iggy3d_tests.cmake --glob '*.{hpp,cpp,cmake}'`.

## Invariants

- Missing source, empty source, invalid projection, hidden-only source, and non-renderable source produce explicit statuses.
- Hidden objects count as hidden and emit no items.
- Item order follows source object order.
- Box items emit box-edge segments unless bounds are degenerate.
- Line/path/link items emit line segments and skip degenerate segments.
- Point items count but do not emit segments.

## Tests / Proof Commands

- `creative_document_wireframe_tests`.
- `product_creative_wireframe_frame_tests`.
- `product_creative_wireframe_debug_line_tests`.
- `render_room_mesh_geometry_tests`.
- `rg -n "creative_document_wireframe_tests|product_creative_wireframe_frame_tests|product_creative_wireframe_debug_line_tests|render_room_mesh_geometry_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/spatial/SpatialProjection.*` unless projection receipt semantics change.
- `src/app/iggy3d/creative/bridge/WireframeFrame.*` unless product frame routing changes.
- `src/app/iggy3d/creative/render/WireframeDebugLines.*` unless segment-to-debug-line conversion changes.
- `src/render/vulkan/BufferImageResources.cpp` unless renderer geometry consumption changes.

## Update When

- Wireframe item schema, status semantics, projection consumption, segment generation, or style mapping changes.

## Do Not Update When

- Only renderer geometry, product receipt recording, or object descriptor row values change without changing wireframe projection behavior.
