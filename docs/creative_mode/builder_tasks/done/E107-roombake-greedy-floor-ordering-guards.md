# E107: RoomBake Greedy Floor Ordering And Sidecar Guard Tests

## Objective

Add focused tests that pin greedy-floor mesh ordering and sidecar behavior before
any larger greedy-floor extraction.

E103 proved a 5x5 floor region collapses to one mesh. It does not fully pin
interleaved document-order behavior, nonaligned fallback, or mixed floor/nonfloor
sidecar ordering. Those are the likely regressions if the greedy-floor block is
extracted.

## Scope

Expected file:

- `tests/unit/creative_document_room_bake_tests.cpp`

Only touch source if a tiny test helper is needed inside the test file.

## Required Test Coverage

Add narrow tests for:

1. **Interleaved source ordering**
   - Document order includes a safe Line/Beam, two adjacent Floors that merge,
     then Wall/Crate.
   - Assert `staticMeshSources` order remains deterministic and still lets the
     merged floor appear at the first contributing Floor's document position.

2. **Nonaligned floor fallback**
   - A structural Floor with valid bounds that are not 1m grid-line aligned
     should bake as the normal per-object floor mesh, not disappear or merge.
   - Assert mesh id remains `creative_object_<id>` and surface source points at
     that mesh id.

3. **Separated aligned islands**
   - Two aligned but separated Floor regions should not collapse into one mesh
     that covers the gap.
   - Assert the number/extents of greedy floor meshes preserve the gap.

Keep expected counts exact and small.

## Do Not

- Do not change RoomBake implementation unless a test exposes an actual defect.
- Do not change RoomBake policy, descriptors, renderer, save/load, input, or
  standalone app.
- Do not stage, commit, push, launch a window, or run broad CTest.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target creative_document_room_bake_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^creative_document_room_bake_tests$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Tests added:
- Behavior pinned:
- Checks run:
- Concerns/deferred:

## Completed

- Files changed:
  - `tests/unit/creative_document_room_bake_tests.cpp`
  - `docs/creative_mode/builder_tasks/claimed/E107-roombake-greedy-floor-ordering-guards.md`
  - `docs/creative_mode/builder_tasks/PRIORITY.md`
- Tests added:
  - `interleavedGreedyFloorSourcesKeepDocumentOrder()`
  - `nonAlignedFloorFallsBackToPerObjectMesh()`
  - `separatedAlignedFloorIslandsPreserveGap()`
- Behavior pinned:
  - A bounds-backed `Beam`, two adjacent `Floor` objects, a `Wall`, and a
    `Crate` preserve deterministic static mesh order and source sidecar order.
    The merged floor mesh appears at the first contributing Floor's document
    position.
  - A valid but non-1m-grid-aligned `Floor` falls back to the per-object floor
    mesh path, keeps `creative_object_<id>` as the mesh id, and keeps its
    walkable surface source pointed at that same mesh id.
  - Two aligned Floor islands separated by a gap produce two floor meshes with
    exact extents around each island instead of one mesh covering the gap.
- Checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target creative_document_room_bake_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^creative_document_room_bake_tests$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - focused trailing-whitespace scan over touched files
- Concerns/deferred:
  - No RoomBake source changes were needed; the new guards passed against the
    current implementation.
  - Greedy-floor extraction should now be able to proceed with stronger
    ordering and sidecar coverage.
