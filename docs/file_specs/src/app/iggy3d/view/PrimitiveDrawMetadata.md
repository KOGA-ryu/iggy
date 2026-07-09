# File Spec

Files: `src/app/iggy3d/view/PrimitiveDrawMetadata.hpp`

Verified at: `b50008a5`

## Owns

- Static metadata catalog for `ProductPrimitiveDrawKind`.
- Default base color and marker size lookup for primitive draw items.
- Inline fallback values for unknown or unmapped primitive draw kinds.

## Does Not Own

- Primitive draw item construction or draw-list aggregation.
- Viewport projection, render bridge aggregation, SDL drawing, or Vulkan rendering.
- Runtime entity, room, physics, or map-maker truth.

## Reads

- `ProductPrimitiveDrawKind` and `ProductPrimitiveColor` from `PrimitiveDrawList.hpp`.

## Writes / Mutates

- No runtime state.
- Returns catalog spans, metadata pointers, colors, and marker sizes.

## Calls Out To / Wires Out To

- `PrimitiveDrawList.cpp` uses the lookup helpers when creating and normalizing primitive draw items.
- Unit tests validate catalog coverage and default lookup behavior.

## Called By / Entry Points

- `productPrimitiveDrawKindMetadataCatalog()`.
- `findProductPrimitiveDrawKindMetadata(...)`.
- `baseColorForProductPrimitiveDrawKind(...)`.
- `markerSizeForProductPrimitiveDrawKind(...)`.
- Focused proof: `rg -n "ProductPrimitiveDrawKindMetadata|baseColorForProductPrimitiveDrawKind|markerSizeForProductPrimitiveDrawKind" src tests`.

## Invariants

- Catalog entries are cold presentation metadata, not gameplay data.
- Each visible primitive kind should have an intentional color and marker size row.
- Missing metadata falls back to the debug-marker color and marker size.
- Keep this header inline-only unless callers need a stable compiled boundary.

## Tests / Proof Commands

- `rg -n "productPrimitiveDrawKindMetadataCatalog|findProductPrimitiveDrawKindMetadata" tests/unit/product_primitive_draw_list_tests.cpp`.
- `rg -n "product_primitive_draw_list_tests" cmake/iggy3d_tests.cmake tests/unit`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/view/PrimitiveDrawList.*` unless item construction or kind definitions change.
- `src/app/iggy3d/view/ViewportFraming.*` unless marker size affects projection contract.
- `src/app/iggy3d/view/RenderBridge.*` unless primitive kind summary policy changes.

## Update When

- A primitive draw kind is added, removed, renamed, or needs different default color or marker size.

## Do Not Update When

- Only runtime simulation, receipt formatting, or backend renderer behavior changes without changing primitive draw metadata.
