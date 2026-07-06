# E52: Spatial Projection Profile Context

## Objective

Reduce per-projection boilerplate in `SpatialProjection.cpp` by extracting the
shared profile/occupancy/request/object/visibility validation context.

## Problem

`projectObjectToGrid(...)` dispatches by projection profile, then each concrete
projector repeats the same setup:

- determine profile and occupancy;
- validate request;
- validate object;
- reject hidden objects;
- construct a receipt with profile and occupancy.

That repetition is not just cosmetic. Adding a new projection profile requires
copying the same guard and receipt scaffolding, while wireframe mapping carries
another profile-to-item switch in `DocumentWireframe.cpp`.

## Required Reads

- `src/app/iggy3d/creative/spatial/SpatialProjection.hpp`
- `src/app/iggy3d/creative/spatial/SpatialProjection.cpp`
- `src/app/iggy3d/creative/document/DocumentWireframe.cpp`
- `tests/unit/creative_spatial_projection_tests.cpp`
- `tests/unit/creative_document_wireframe_tests.cpp`

## Scope

- Add a small internal projection context/helper that computes profile,
  occupancy, and common rejection receipts once.
- Keep profile-specific geometry logic unchanged.
- Preserve all status/reason codes and cell output.
- If practical, add a local helper for profile-to-wireframe item mapping only
  when it reduces duplication without mixing wireframe policy into projection.

## Acceptance

- New projection profiles have one clear place for common validation and receipt
  setup.
- Existing Point, Box, Volume, Line, Path, Link, hidden, out-of-bounds, and
  authoring-excluded tests remain behavior-identical.
- No projection semantics change.

## Suggested Checks

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_spatial_projection_tests creative_document_wireframe_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_spatial_projection_tests|creative_document_wireframe_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not change Path off-grid no-clamp behavior.
- Do not add spatial indexes or picking in this slice.
- Do not move wireframe styling into projection output.

## Completion Brief

- Files modified:
  - `src/app/iggy3d/creative/spatial/SpatialProjection.cpp`
- Added a private `CreativeSpatialProjectionContext` helper that centralizes:
  - profile
  - occupancy kind
  - invalid-grid rejection
  - invalid-object rejection
  - hidden-object rejection
- Routed top-level dispatch and concrete Point/Box/Volume/Line/Path projectors
  through the shared context.
- Preserved direct `projectLinkObjectToGrid(...)` behavior by explicitly
  disabling request validation in its context, matching the prior direct helper
  semantics while top-level dispatch still validates requests.
- Did not change projection geometry, Path off-grid/no-clamp behavior,
  wireframe styling, or wireframe policy.

## Verification

- `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d creative_spatial_projection_tests creative_document_wireframe_tests -j10`
- `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(creative_spatial_projection_tests|creative_document_wireframe_tests)$' --output-on-failure`
- `git -C /Users/kogaryu/iggy3d diff --check`
- Focused trailing whitespace scan over touched files.

All verification passed.
