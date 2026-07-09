# File Spec

Files: `src/app/iggy3d/creative/bridge/WindowCoordinateSpace.hpp`, `src/app/iggy3d/creative/bridge/WindowCoordinateSpace.cpp`

Verified at: `5ad31d28`

## Owns

- Product creative virtual coordinate-space resolution for window-backed UI and bridge surfaces.
- Logical, fallback, and guard extent selection.
- Receipt-friendly coordinate-space status and reason strings.

## Does Not Own

- SDL window event polling.
- Drawable-to-logical mouse conversion.
- Creative UI layout, hit-region generation, or viewport picking.
- Renderer viewport/scissor state.

## Reads

- Logical window extent.
- Drawable extent.
- Fallback virtual extent.
- Guard virtual extent.

## Writes / Mutates

- Returns `ProductCreativeWindowCoordinateSpace`.
- Does not mutate window, input, renderer, or creative state.

## Calls Out To / Wires Out To

- Creative UI/window frame paths use the resolved virtual size.
- Unit tests call the resolver directly.

## Called By / Entry Points

- `resolveProductCreativeWindowCoordinateSpace(...)`.
- Focused proof: `rg -n "ProductCreativeWindowCoordinateSpace|resolveProductCreativeWindowCoordinateSpace|creative_coordinate_space" src/app tests`.

## Invariants

- Valid logical extent wins over fallback and guard.
- Valid fallback extent is used when logical extent is absent.
- Guard extent is used when logical and fallback extents are invalid.
- If the provided guard is invalid, the hard default is `1280x720`.
- Drawable extent is preserved as an observation, not used as the virtual size selector.

## Tests / Proof Commands

- `rg -n "product_creative_window_coordinate_space_tests" cmake/iggy3d_tests.cmake tests/unit`.
- `rg -n "creative_coordinate_space_logical|creative_coordinate_space_fallback|creative_coordinate_space_guard" tests/unit/product_creative_window_coordinate_space_tests.cpp src/app`.

## Nearby Files Usually Not Touched

- `src/app/iggy3d/creative/bridge/UiWindowFrame.*` unless coordinate-space consumption changes.
- `src/app/iggy3d/window/FramePresenter.*` unless drawable/logical window facts change.
- `src/app/platform/SdlWindow.*` unless platform extent reporting changes.

## Update When

- Virtual coordinate-space selection, default extents, request/result fields, or status strings change.

## Do Not Update When

- Only creative UI layout, renderer viewport, or mouse-hit algorithms change without changing coordinate-space resolution.
