# File Spec

Files: `src/app/iggy3d/creative/render/WireframeDebugLines.hpp`, `src/app/iggy3d/creative/render/WireframeDebugLines.cpp`

Verified at: `116e4a9e`

## Owns

- Conversion from creative document wireframe segments into product debug line packets.
- Wireframe debug line status enum, string conversion, color mapping, request/result packets, and receipt counters.
- Degenerate segment skipping and line-thickness propagation.

## Does Not Own

- Document wireframe item generation.
- Wireframe segment generation.
- Product wireframe frame active gating.
- Renderer geometry generation or backend drawing.
- Receipt field emission.

## Reads

- `CreativeDocumentWireframeSegment` spans or segment lists.
- Segment start/end points, object id, object kind, wireframe style, and segment kind.
- Requested source availability and line thickness.

## Writes / Mutates

- Returns `ProductCreativeWireframeDebugLineBuildResult`.
- Does not mutate segment lists, document state, window state, or renderer state.

## Calls Out To / Wires Out To

- `WireframeFrame.cpp` builds debug lines after document wireframe segmentation.
- `FramePresenter.cpp`, render projection validation, and renderer tests consume debug line lists.
- Unit tests call debug line builders directly.

## Called By / Entry Points

- `toString(ProductCreativeWireframeDebugLineStatus)`.
- `productCreativeWireframeDebugLineColorForStyle(...)`.
- `buildProductCreativeWireframeDebugLines(...)` overloads.
- Focused proof: `rg -n "ProductCreativeWireframeDebugLine|buildProductCreativeWireframeDebugLines|product_creative_wireframe_debug_lines" src/app tests`.

## Invariants

- Missing source returns missing-source status.
- Empty source returns no-lines source-empty status.
- Degenerate segments are skipped and counted.
- Built status requires at least one output line.
- Style-to-color mapping is presentation metadata and not creative document truth.

## Tests / Proof Commands

- `rg -n "product_creative_wireframe_debug_line_tests|product_creative_wireframe_frame_tests|render_projection_input_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "product_creative_wireframe_debug_lines_built|skippedDegenerateCount|ProductCreativeWireframeDebugLineStatus" tests/unit src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/document/DocumentWireframe.*` unless segment output changes.
- `src/app/iggy3d/creative/bridge/WireframeFrame.*` unless frame handoff changes.
- `src/app/iggy3d/window/FramePresenter.*` unless presenter consumption changes.
- `src/render/*` unless backend debug-line geometry changes.

## Update When

- Debug line packets, color mapping, degenerate filtering, status strings, builder overloads, or receipt counters change.

## Do Not Update When

- Only document wireframe generation, bridge active gating, or renderer backend internals change without changing debug line output.
