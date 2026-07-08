# E267: iggy3d_creative Main Split G1 - Wireframe Box Edges

## Objective

Perform the first narrow implementation slice for `docs/refactor_targets.md`
target #3 by moving only the file-local `appendWireframeBoxEdges(...)` helper
out of `apps/iggy3d_creative/main.cpp`.

This is a mechanical extraction only. Preserve behavior, edge order, overlay
line values, call-site order, CMake target behavior, and focused test results.

## Current Context

E266 completed the read-only preflight for extracting
`apps/iggy3d_creative/main.cpp`:

- `main.cpp` is 1651 lines.
- `main(int, char**)` starts at line 230.
- the frame loop starts at line 488 and runs through line 1641.
- the capture/shutdown tail is lines 1644-1650.
- the safest first slice is not a frame-stage move or state-object
  introduction.

E266 selected a tiny app-local helper extraction because it proves the
standalone helper/CMake seam without touching SDL, Vulkan, frame-buffer
lifetime, editor input, capture timing, object ids, or document mutation.

Current helper shape in `main.cpp`:

- file-local `appendWireframeBoxEdges(...)` at lines 202-226;
- appends the 12 axis-aligned edges of a world box into a
  `std::vector<RenderCreativeWireframeDebugLine>`;
- preserves edge order:
  - X edges: `{0,1}`, `{2,3}`, `{4,5}`, `{6,7}`
  - Y edges: `{0,2}`, `{1,3}`, `{4,6}`, `{5,7}`
  - Z edges: `{0,4}`, `{1,5}`, `{2,6}`, `{3,7}`
- reserves `out.size() + 12`;
- sets `line.color = color`;
- sets `line.objectId = 0`;
- sets `line.thickness = thickness`;
- currently has three call sites in `main.cpp`:
  - point/line marker boxes;
  - path-point handle boxes;
  - non-path placement ghost box.

## Scope

Edit only:

- `apps/iggy3d_creative/main.cpp`
- new `apps/iggy3d_creative/StandaloneWireframeBoxEdges.hpp`
- new `apps/iggy3d_creative/StandaloneWireframeBoxEdges.cpp`
- `CMakeLists.txt`
- this task card when moving it to `done/`

Add the new helper source to the `iggy3d_creative` executable source list near
the other `apps/iggy3d_creative/Standalone*.cpp` helper files.

## Required Helper API

Create a new app-local helper in namespace `iggy3d_creative_app`:

```cpp
namespace iggy3d_creative_app {

void appendStandaloneWireframeBoxEdges(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& out,
    iggy3d::Vec3 boxMin,
    iggy3d::Vec3 boxMax,
    iggy3d::RenderLineColor color,
    float thickness);

}  // namespace iggy3d_creative_app
```

Expected header dependencies:

- `<vector>`
- `core/math/Vec3.hpp`
- `render/FrameInput.hpp`

The implementation should move the current function body exactly, adjusted only
for namespace qualification and the new function name.

`main.cpp` should include `StandaloneWireframeBoxEdges.hpp`, add a local
`using iggy3d_creative_app::appendStandaloneWireframeBoxEdges;`, delete the
file-local helper definition, and update the three call sites to the new helper
name.

## Required Behavior Preservation

Preserve:

- edge order exactly;
- corner calculation exactly;
- `out.reserve(out.size() + 12)`;
- `line.start` and `line.end` assignment;
- `line.color = color`;
- `line.objectId = 0`;
- `line.thickness = thickness`;
- `out.push_back(line)`;
- all three existing call sites and their order;
- all passed colors, thicknesses, `ghostMin`/`ghostMax`, marker bounds, and
  path-handle bounds;
- `main(int, char**)`, `--frames`, `--capture`, frame loop, submit, shutdown,
  and return-code behavior.

Do not change any comments beyond what is needed to keep the moved helper
understandable in its new file.

## Non-Goals

Do not edit:

- any other `apps/iggy3d_creative/*.{hpp,cpp}` file;
- `cmake/iggy3d_tests.cmake`;
- tests;
- receipt fields or golden files;
- fixture/package data;
- production docs outside this task card and `PRIORITY.md` if the workflow
  requires priority bookkeeping.

Do not introduce `CreativeEditorState`.
Do not create `EditorFrame.{hpp,cpp}`.
Do not move frame stages.
Do not move or change event handling, camera input, resize behavior, placement,
selection, capture scenario dispatch, gizmo/path policy, overlay construction
beyond the renamed helper calls, frustum culling, submit, shutdown, logging,
capture script frame numbers, object ids, seeded Floor/Crate setup, save root,
final capture proof strings, or render-submit reason strings.
Do not merge this helper into `StandalonePreviewProxies.*` or move
`appendPathPolylineLines(...)`.
Do not run broad CTest.
Do not launch an interactive window.
Do not require `iggy3d_creative --capture` unless an owner explicitly allows a
windowed/Vulkan capture check.
Do not stage, commit, or push.

## Required Grep Classification

Run:

```sh
rg -n "appendWireframeBoxEdges|appendStandaloneWireframeBoxEdges|StandaloneWireframeBoxEdges" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/StandaloneWireframeBoxEdges.hpp \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/StandaloneWireframeBoxEdges.cpp \
  /Users/kogaryu/iggy3d/CMakeLists.txt
```

Expected:

- no old `appendWireframeBoxEdges(...)` definition or call sites remain;
- `appendStandaloneWireframeBoxEdges(...)` is declared in
  `StandaloneWireframeBoxEdges.hpp`;
- `appendStandaloneWireframeBoxEdges(...)` is defined in
  `StandaloneWireframeBoxEdges.cpp`;
- `main.cpp` includes `StandaloneWireframeBoxEdges.hpp`;
- `main.cpp` has the `using` declaration and exactly three call sites for
  `appendStandaloneWireframeBoxEdges(...)`;
- `CMakeLists.txt` includes
  `apps/iggy3d_creative/StandaloneWireframeBoxEdges.cpp` in the
  `iggy3d_creative` executable source list.

Run:

```sh
rg -n "CreativeEditorState|EditorFrame|runCreativeEditorFrame|appendPathPolylineLines" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative \
  --glob '*.hpp' --glob '*.cpp'
```

Expected:

- no `CreativeEditorState`, `EditorFrame`, or `runCreativeEditorFrame` was
  introduced;
- `appendPathPolylineLines(...)` remains owned by the existing file that owned
  it before this card.

## Verification

Run:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative standalone_picking_tests standalone_placement_tests standalone_frustum_cull_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(standalone_picking_tests|standalone_placement_tests|standalone_frustum_cull_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over:

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/StandaloneWireframeBoxEdges.hpp`
- `apps/iggy3d_creative/StandaloneWireframeBoxEdges.cpp`
- `CMakeLists.txt`
- this task card after moving it to `done/`

Optional only if a windowed/Vulkan capture check is explicitly allowed:

```sh
(cd /Users/kogaryu/iggy3d && ./build/iggy3d_creative --capture /tmp/iggy3d_creative_e267_final.png --frames 32 > /tmp/iggy3d_creative_e267_final.log 2>&1)
rg "ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e267_final.log
```

## Self-Blockers

Stop and report instead of widening scope if:

- the helper move requires changing edge order, colors, thicknesses, object ids,
  vector reserve behavior, or call-site order;
- compile fallout requires moving overlay construction, selection, gizmo/path
  policy, renderer submit, capture behavior, or any frame-stage code;
- CMake/source-list changes affect targets other than `iggy3d_creative`;
- a capture/window launch appears necessary to prove correctness.

## Completion Brief Checklist

Report:

- files changed;
- exact helper API shape;
- CMake source-list placement;
- number of old helper definitions/call sites removed;
- number of new helper call sites in `main.cpp`;
- required grep classifications;
- focused build/CTest results;
- diff/whitespace check results;
- whether optional capture was skipped or run;
- confirmation that no `CreativeEditorState`, `EditorFrame`, frame-stage move,
  tests, receipt/golden files, broad CTest, interactive window launch, staging,
  commit, or push was performed.

## Completion Brief - E267

### Files changed

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/StandaloneWireframeBoxEdges.hpp`
- `apps/iggy3d_creative/StandaloneWireframeBoxEdges.cpp`
- `CMakeLists.txt`
- `docs/creative_mode/builder_tasks/done/E267-iggy3d-creative-main-split-g1-wireframe-box-edges.md`

### Helper API shape

Added app-local helper in namespace `iggy3d_creative_app`:

```cpp
void appendStandaloneWireframeBoxEdges(
    std::vector<iggy3d::RenderCreativeWireframeDebugLine>& out,
    iggy3d::Vec3 boxMin,
    iggy3d::Vec3 boxMax,
    iggy3d::RenderLineColor color,
    float thickness);
```

Header dependencies are `vector`, `core/math/Vec3.hpp`, and
`render/FrameInput.hpp`.

The implementation preserves the moved helper body behavior: same corner
calculation, same edge order, same `out.reserve(out.size() + 12)`, same
`line.start` / `line.end`, same `line.color`, same `line.objectId = 0`, same
`line.thickness`, and same `out.push_back(line)`.

### CMake source-list placement

Added `apps/iggy3d_creative/StandaloneWireframeBoxEdges.cpp` to the
`iggy3d_creative` executable source list in `CMakeLists.txt`, directly after
`apps/iggy3d_creative/StandaloneRoomBakePreview.cpp` and alongside the other
standalone helper sources.

### Helper move counts

- Old `appendWireframeBoxEdges(...)` definitions removed from `main.cpp`: 1.
- Old `appendWireframeBoxEdges(...)` call sites removed from `main.cpp`: 3.
- New `appendStandaloneWireframeBoxEdges(...)` call sites in `main.cpp`: 3.

### Required grep classifications

`rg -n "appendWireframeBoxEdges|appendStandaloneWireframeBoxEdges|StandaloneWireframeBoxEdges" ...`

- No old `appendWireframeBoxEdges(...)` definition or call site remains.
- `StandaloneWireframeBoxEdges.hpp` declares
  `appendStandaloneWireframeBoxEdges(...)`.
- `StandaloneWireframeBoxEdges.cpp` defines
  `appendStandaloneWireframeBoxEdges(...)`.
- `main.cpp` includes `StandaloneWireframeBoxEdges.hpp`.
- `main.cpp` has the local `using` declaration and exactly three call sites for
  `appendStandaloneWireframeBoxEdges(...)`.
- `CMakeLists.txt` includes
  `apps/iggy3d_creative/StandaloneWireframeBoxEdges.cpp` in the
  `iggy3d_creative` executable source list.

`rg -n "CreativeEditorState|EditorFrame|runCreativeEditorFrame|appendPathPolylineLines" ...`

- No `CreativeEditorState`, `EditorFrame`, or `runCreativeEditorFrame` hits.
- `appendPathPolylineLines(...)` remains owned by
  `StandalonePreviewProxies.hpp/.cpp`, with only the existing `main.cpp` using
  declaration and call site.

### Focused build and CTest

Passed:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative standalone_picking_tests standalone_placement_tests standalone_frustum_cull_tests -j10
```

Passed:

```sh
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(standalone_picking_tests|standalone_placement_tests|standalone_frustum_cull_tests)$' --output-on-failure
```

CTest result:

- `standalone_picking_tests`: passed
- `standalone_placement_tests`: passed
- `standalone_frustum_cull_tests`: passed

### Diff and whitespace checks

Passed:

```sh
git -C /Users/kogaryu/iggy3d diff --check
```

Focused trailing-whitespace scan over touched source/CMake files and this task
card: passed.

### Optional capture

Skipped. The card made `iggy3d_creative --capture` optional only with explicit
owner approval for a windowed/Vulkan capture check, and no such approval was
given for this slice.

### Scope confirmation

No `CreativeEditorState`, `EditorFrame`, `runCreativeEditorFrame`, frame-stage
move, event handling change, camera input change, resize behavior change,
placement change, selection change, capture scenario dispatch change,
gizmo/path policy change, frustum culling change, submit change, shutdown
change, logging change, capture script frame number change, object id change,
seeded Floor/Crate setup change, save-root change, tests, receipt/golden files,
broad CTest, interactive window launch, staging, commit, or push was performed.
