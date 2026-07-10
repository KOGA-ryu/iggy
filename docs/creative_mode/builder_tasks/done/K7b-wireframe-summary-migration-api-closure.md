# K7b - Wireframe Summary Migration And Projection API Closure

## Status

READY. K7 batch step 2 of 2. Claim only after K7a is committed and in `done/`,
with its seven-member summary parity suite and graph 703/317/16/16, app fan-out
188. Authority:
`docs/creative_mode/builder_tasks/blocked/K7-spatial-projection-summary-plan.md`.

This is the K7 closeout. Reviewer receives one aggregate two-commit brief only
after this card is committed, unless K7a or K7b STOPs.

## Goal

Route per-frame document wireframe construction through the allocation-free
summary and remove the six unused public per-shape projector APIs.

## Exact Files

Update only:

- `src/app/iggy3d/creative/document/DocumentWireframe.cpp`
- `src/app/iggy3d/creative/spatial/SpatialProjection.hpp`
- `src/app/iggy3d/creative/spatial/SpatialProjection.cpp`
- `tests/unit/creative_document_wireframe_tests.cpp`
- `tests/unit/creative_spatial_projection_tests.cpp`

Do not edit DocumentWireframe.hpp, CMake, bridge/window/render code, descriptors,
room bake, viewport picking, receipts/goldens, policy, or other tests/source.

## Wireframe Migration

Change `makeWireframeItem` to accept
`const CreativeSpatialProjectionSummary&`. Preserve all item fields and geometry
logic; set projected cell count from `summary.cellCount`.

In the build loop:

- retain current hidden/visible/projectable/non-projectable counter order;
- call `projectObjectToGridSummary` only after the same prechecks;
- require `Projected` and nonzero `cellCount` where the old code required
  `Projected` and nonempty cells;
- accumulate `receipt.projectionCellCount` from `summary.cellCount`;
- preserve unknown item-kind handling, path-point copies, styles, final status,
  item order, and segment order.

No `CreativeSpatialProjectionReceipt`, `projectObjectToGrid`, or `.cells` read
may remain in DocumentWireframe.cpp.

Extend wireframe tests with a path that revisits cells and assert item and
aggregate projected counts match both the summary and existing expected value.
Do not alter existing receipt expectations.

## Public API Closure

Remove public declarations for:

- `projectPointObjectToGrid`
- `projectBoxObjectToGrid`
- `projectVolumeObjectToGrid`
- `projectLineObjectToGrid`
- `projectPathObjectToGrid`
- `projectLinkObjectToGrid`

Delete the six named wrappers. Required private per-profile helpers use
plan/materialization names; the old public API names must have no source/test
hit. Do not duplicate function bodies or add an internal header.

Retarget the three direct link tests to `projectObjectToGrid`; keep their status,
message, profile, occupancy, cell, and off-grid assertions unchanged.

The final public header has exactly three function declarations whose names
start with `project` and end with `Object(s)ToGrid` or `ObjectToGridSummary`:
summary single-object, full single-object, and full aggregate.

## Final Checkpoint

- DocumentWireframe has one positive summary call and zero full projection/cell
  payload references.
- The six per-shape names have no source/test hit.
- Full and aggregate projection tests remain unchanged and green.
- SpatialProjection.cpp remains no more than 1,150 lines.
- Source/graph remain 703 files, 317 direct edges, 16/16 pairs, app fan-out 188,
  zero SCCs and violations.
- No CMake, receipt, golden, policy, or persisted schema change.

## Focused Verification

    cmake --build build --target iggy3d_app creative_spatial_projection_tests creative_document_wireframe_tests product_creative_wireframe_frame_tests product_creative_wireframe_debug_line_tests product_creative_pick_flow_tests standalone_picking_tests
    ctest --test-dir build -R '^(creative_spatial_projection_tests|creative_document_wireframe_tests|product_creative_wireframe_frame_tests|product_creative_wireframe_debug_line_tests|product_creative_pick_flow_tests|standalone_picking_tests|dependency_direction_tests)$' --output-on-failure
    python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json
    rg -n 'projectObjectToGridSummary\(' src/app/iggy3d/creative/document/DocumentWireframe.cpp
    rg -n 'projectObjectToGrid\(|CreativeSpatialProjectionReceipt|\.cells' src/app/iggy3d/creative/document/DocumentWireframe.cpp
    rg -n 'project(Point|Box|Volume|Line|Path|Link)ObjectToGrid' src tests
    rg -n '^\[\[nodiscard\]\] CreativeSpatialProjection(Receipt|Summary) project' src/app/iggy3d/creative/spatial/SpatialProjection.hpp
    wc -l src/app/iggy3d/creative/spatial/SpatialProjection.cpp
    git diff -- tests/golden
    git diff --check

The first grep has one production call. The next two greps return no output.
The public-header grep reports exactly summary, full single, and aggregate.
Golden diff is empty.

## Stop Conditions

- Wireframe consumes any full cell coordinate/index not found in the live read
  set.
- Summary changes wireframe counters, item geometry/order, style, or status.
- A production per-shape caller appears or a compatibility declaration is
  requested.
- Closing the API requires a new file/header or duplicate projection policy.
- SpatialProjection.cpp exceeds 1,150 lines, graph differs from
  703/317/188/16/16, or a focused test regresses.

## Completion Brief

Append wireframe call/read-set evidence, revisiting-path count pin, deleted API
surface, exact public operation list, line/graph metrics, focused tests, and
protected receipt/golden/policy evidence. Commit, then send Reviewer one
aggregate K7 brief listing both commits and request milestone code review.
