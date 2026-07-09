# K7a - Spatial Projection Summary Kernel

## Status

READY. K7 batch step 1 of 2. Claim only after K6 is accepted; accepted baseline
is `fbba6e72`. Authority:
`docs/creative_mode/builder_tasks/blocked/K7-spatial-projection-summary-plan.md`.

On green completion, commit this card, move it to `done/`, and continue to K7b
without waiting for Reviewer. A STOP pauses the whole K7 batch.

## Goal

Add the allocation-free seven-field projection summary and make summary/full
single-object projection share one private policy plan without changing any
existing full receipt.

## Exact Files

Update only:

- `src/app/iggy3d/creative/spatial/SpatialProjection.hpp`
- `src/app/iggy3d/creative/spatial/SpatialProjection.cpp`
- `tests/unit/creative_spatial_projection_tests.cpp`

Do not edit DocumentWireframe, CMake, descriptors, object/document types,
aggregate callers, receipts, policy, goldens, or any other source/test.

## Required Public Additions

Add exactly the parent plan's seven-member
`CreativeSpatialProjectionSummary` and
`projectObjectToGridSummary(...)`. Do not add message/reason text, a cells
container, optional/variant payload, owning pointer, or aggregate summary API.

Keep all existing declarations in this slice; K7b closes per-shape APIs.

## Required Shared Kernel

Create one private allocation-free projection plan containing summary plus a
non-owning static message view. It is the sole owner of generic request/object
validation, descriptor profile/occupancy resolution, visibility and
authoring-only policy, projected bounds, status, and exact cell count.

- Summary returns the plan summary directly.
- Full single-object projection builds its existing receipt from the same plan
  and materializes cells only for `Projected`.
- Existing per-shape functions temporarily use the same plan/materializer in
  forced-profile mode and preserve their direct-call semantics.
- Aggregate projection continues through the full single-object API unchanged.

Do not call the full projector from summary, construct a full receipt on the
summary path, or maintain parallel profile/status switches.

## Allocation-Free Count Contract

- Summary plan/result contains no dynamic container or owning string.
- Bounds count uses half-open arithmetic.
- Sampled line/link use the existing rounded coordinate sequence.
- Path count preserves global first-seen deduplication across every segment.
- Summary path uses no vector/set/unordered_set/new and does not weaken
  deduplication to adjacent endpoints.
- Full materialization and summary count share the same sampled-coordinate
  enumerator.

Keep all existing full cell coordinates, indices, order, messages, and statuses.
SpatialProjection.cpp must remain at or below 1,150 lines.

## Required Tests

Add a reusable parity assertion comparing summary with full receipt for status,
object id/kind, profile, occupancy, projected bounds, and
`summary.cellCount == receipt.cells.size()`.

Exercise parity for:

- valid point, box, volume, line, path, and link;
- invalid grid/object, hidden object, authoring exclusion, no-projection,
  out-of-bounds, invalid path, and invalid link;
- a path that backtracks and crosses already sampled coordinates.

Add a summary-only large bounds case, for example 128 x 128 x 128, that pins the
arithmetic count without invoking the full projector. Add
`static_assert(std::is_trivially_copyable_v<CreativeSpatialProjectionSummary>)`.
Do not delete or weaken existing full receipt tests.

## Mechanical Checkpoint

- Summary has exactly seven direct members.
- There is one generic profile-policy switch and one private plan owner.
- Full receipt fields/cells/messages remain unchanged.
- SpatialProjection.cpp is no more than 1,150 lines.
- Source/graph remain 703 files, 317 direct edges, 16/16 pairs, app fan-out 188,
  zero SCCs and policy violations.

## Focused Verification

    cmake --build build --target iggy3d_app creative_spatial_projection_tests creative_document_wireframe_tests product_creative_wireframe_frame_tests product_creative_pick_flow_tests standalone_picking_tests
    ctest --test-dir build -R '^(creative_spatial_projection_tests|creative_document_wireframe_tests|product_creative_wireframe_frame_tests|product_creative_pick_flow_tests|standalone_picking_tests|dependency_direction_tests)$' --output-on-failure
    python3 tools/dependency_graph.py --repo-root /Users/kogaryu/iggy3d --policy docs/architecture_dependency_policy.json --check-policy --format json
    awk '/struct CreativeSpatialProjectionSummary/{inside=1; next} inside && /^};/{print count; exit} inside && /;[[:space:]]*$/{count++}' src/app/iggy3d/creative/spatial/SpatialProjection.hpp
    wc -l src/app/iggy3d/creative/spatial/SpatialProjection.cpp
    rg -n 'projectObjectToGrid\(' src/app/iggy3d/creative/document/DocumentWireframe.cpp
    git diff --check

The summary count command returns 7. The positive wireframe grep still returns
the existing call in K7a; report it as protected deferred work for K7b.

## Stop Conditions

- Exact summary parity requires an allocated path container or second policy
  implementation.
- Existing full receipt tests need changed expectations.
- DocumentWireframe or a non-listed file needs edits.
- SpatialProjection.cpp exceeds 1,150 lines.
- Summary member count differs from seven, graph differs from
  703/317/188/16/16, or a focused test regresses.

## Completion Brief

Append summary shape, shared-plan/materializer design, no-allocation path-count
evidence, parity matrix, large-bounds pin, exact line/graph metrics, focused
tests, and protected wireframe call evidence. Commit and continue to K7b.
