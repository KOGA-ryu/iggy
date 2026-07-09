# File Spec

Files: `src/app/iggy3d/creative/bridge/WireframeFrame.hpp`, `src/app/iggy3d/creative/bridge/WireframeFrame.cpp`

Verified at: `5ad31d28`

## Owns

- Product bridge request/result for building a creative document wireframe frame.
- Active creative editor gate for wireframe generation.
- Product receipt that combines document wireframe, wireframe segments, and debug-line build facts.
- Debug-line list handoff for renderer/projection consumers.

## Does Not Own

- Creative document wireframe item generation.
- Wireframe segment generation internals.
- Debug line geometry kernel internals.
- Creative document storage, mutation, or visibility policy.
- Receipt field emission or renderer backend drawing.

## Reads

- Product window active creative editor state.
- `creative::CreativeAppState` and `Facade` document.
- Creative spatial projection request.
- Document wireframe, segment, and debug-line build receipts.

## Writes / Mutates

- Returns `ProductCreativeWireframeFrameBuildResult` or receipt-only route result.
- Does not mutate window state, creative app state, document objects, or renderer state.

## Calls Out To / Wires Out To

- Calls `productCreativeDocumentEditorActiveForSource(...)`.
- Calls `creative::buildCreativeDocumentWireframeList(...)`.
- Calls `creative::buildCreativeDocumentWireframeSegments(...)`.
- Calls `buildProductCreativeWireframeDebugLines(...)`.
- Receipt recording and render projection paths consume the frame receipt and debug lines.

## Called By / Entry Points

- `productCreativeWireframeFrameActiveForWindow(...)`.
- `buildProductCreativeWireframeFrame(...)`.
- `routeProductCreativeWireframeFrame(...)`.
- Focused proof: `rg -n "ProductCreativeWireframeFrame|buildProductCreativeWireframeFrame|creative_wireframe" src/app tests`.

## Invariants

- Missing window, inactive surface, missing facade, invalid document, invalid projection, empty source, no segments, and built frame each produce explicit statuses.
- Wireframe receipt and segment receipt fields are copied separately so failures stay diagnosable.
- Debug-line receipt is copied into the product frame even when no renderable lines result.
- This bridge is a product frame builder; reusable document/geometry kernels stay under creative document/render surfaces.

## Tests / Proof Commands

- `rg -n "product_creative_wireframe_frame_tests|product_creative_wireframe_debug_line_tests|creative_document_wireframe_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "buildProductCreativeWireframeFrame|product_creative_wireframe_frame_built|creative_wireframe_debug_line_count" tests/unit src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/document/DocumentWireframe.*` unless document wireframe output changes.
- `src/app/iggy3d/creative/render/WireframeDebugLines.*` unless debug-line output changes.
- `src/app/iggy3d/receipt/CreativePickWireframeFields.*` unless receipt keys change.
- `src/render/*` unless renderer consumption changes.

## Update When

- Wireframe frame request/result fields, active gating, document/segment/debug-line handoff, status mapping, or receipt-facing semantics change.

## Do Not Update When

- Only document wireframe internals, debug-line geometry, or renderer backend behavior changes without changing this bridge contract.
