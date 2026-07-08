# E266: iggy3d_creative Main Split Preflight

## Objective

Perform a read-only preflight for `docs/refactor_targets.md` target #3:
extracting `apps/iggy3d_creative/main.cpp` out of one large `main()` into
stable standalone-editor seams.

The goal is to map the current `iggy3d_creative` entrypoint, decide the safest
first implementation slice, and define focused verification. Do not edit
production source in this card.

## Current Context

The two larger backlog targets ahead of this are closed:

- target #2, `Controller.cpp` split, is complete through E249;
- target #1, receipt field boilerplate, is complete through E265.

`docs/refactor_targets.md` now points at target #3:

- `apps/iggy3d_creative/main.cpp`
- current size: 1651 lines
- `main()` starts at line 230
- frame loop starts at line 488 with `while (window.isOpen())`
- the current per-frame body runs until shutdown/capture handling at the end of
  the file
- existing standalone helper siblings already own several domains:
  `CreativeRendererBootstrap`, `StandaloneCaptureScenario`,
  `StandaloneBrushPalette`, `StandaloneFrustumCull`, `StandaloneGizmo`,
  `StandalonePathEditing`, `StandalonePersistenceProof`, `StandalonePicking`,
  `StandalonePlacement`, `StandalonePreviewProxies`,
  `StandaloneRoomBakePreview`, and `StandaloneUndo`

The target text suggests eventually shaping the app around:

- a `CreativeEditorState` struct owning current free locals;
- named frame stages in `EditorFrame.{hpp,cpp}`;
- `main()` reduced to parse, build, seed, and
  `while (open) runCreativeEditorFrame(state, deps)`;
- the existing `--capture` path as the deterministic verification seam.

This card is only the preflight. It should not introduce `CreativeEditorState`
or `EditorFrame` yet.

## Scope

Read and classify:

- `docs/refactor_targets.md`
- `docs/creative_mode/builder_tasks/PRIORITY.md`
- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/*.hpp`
- `apps/iggy3d_creative/*.cpp`
- `CMakeLists.txt`
- `cmake/iggy3d_tests.cmake`
- standalone creative unit tests:
  - `tests/unit/standalone_picking_tests.cpp`
  - `tests/unit/standalone_placement_tests.cpp`
  - `tests/unit/standalone_frustum_cull_tests.cpp`
- prior done cards only if needed to identify already-extracted standalone
  helper ownership, especially E66, E83, E101, and current E250-E265 context

Classify `apps/iggy3d_creative/main.cpp` by line ranges into:

- argument parsing and capture-frame defaults;
- SDL window and Vulkan renderer bootstrap;
- initial camera/grid/document seed/setup;
- save-root and standalone persistence setup;
- editor state locals and latches;
- event polling, drawable/resize handling, and camera input;
- interactive tool-switch/save/load/delete/undo key handling;
- frame scene/RoomBake preview construction;
- aim-ground-cell resolution;
- object pick-candidate construction and capture pick-proof logging;
- click selection;
- placement behavior;
- capture scenario dispatch;
- selection/gizmo/path-handle geometry and hit-test;
- capture move script;
- interactive move/path/path-point drag handling;
- inspector UI projection;
- document wireframe, point/line/path/ghost/gizmo overlay construction;
- dimension label/glyph merge;
- frustum cull, submit, frame logging, and frame-limit exit;
- capture PNG write, renderer idle/shutdown, and return code.

For each range, record:

- line span;
- dominant owned state;
- external helper dependencies already available;
- whether the range is pure enough for extraction;
- whether it needs SDL/window/Vulkan dependencies;
- whether it mutates `CreativeAppState`, `StandaloneUndoStack`, camera state,
  capture script, selection/gizmo latches, or render-frame local buffers;
- whether the range is safe as a first implementation slice.

## Questions To Answer

1. Is `apps/iggy3d_creative/main.cpp` still the correct target #3 file and what
   is its current line/loop shape?
2. Which current line ranges are genuine frame-stage seams, and which are
   still bootstrap/setup/shutdown seams?
3. What first extraction should be implemented in E267, and why is it safer
   than starting with a full `CreativeEditorState`?
4. Should E267 introduce only a small helper/TU, or should it introduce the
   first minimal state struct?
5. Which existing standalone helper modules and tests already cover the
   candidate extraction area?
6. What exact CMake source-list changes would the first implementation require,
   if any?
7. What focused verification is enough for the first implementation slice?
8. Should `--capture` be run in the implementation lane, or should it be
   listed as optional/manual because it creates a Vulkan window?

## Helper Shapes To Evaluate

Evaluate, but do not implement, these possible first-slice shapes:

### Option A: Tiny App-Local Helper

Move one self-contained frame-loop utility out of `main.cpp` first, such as:

```cpp
void appendStandaloneWireframeBoxEdges(
    std::vector<RenderCreativeWireframeDebugLine>& out,
    Vec3 boxMin,
    Vec3 boxMax,
    RenderLineColor color,
    float thickness);
```

This proves CMake/source-list mechanics but does not reduce the main frame
loop meaningfully.

### Option B: Frame Input Stage

Extract camera/event/resize/fly input into a narrow helper that receives
window/backend/camera state explicitly and returns whether the loop should
continue. This is higher value but touches SDL and renderer resize behavior.

### Option C: Overlay Build Stage

Extract selected-object, gizmo, wireframe, ghost, and label construction into a
standalone frame-overlay helper. This is likely the highest line-count payoff,
but it has many transient buffers that must outlive `submitFrame(...)`, so the
state/lifetime design must be explicit before implementation.

### Option D: State Struct First

Introduce `CreativeEditorState` with current free locals, then move stages
later. This may be the right end state, but it is broad for a first slice and
should be recommended only if the preflight proves smaller extractions would
create churn.

Be conservative. The first implementation card should prove one seam without
changing behavior, capture schedule, frame numbers, object ids, visual output,
or editor input policy.

## Non-Goals

Do not edit:

- `apps/iggy3d_creative/main.cpp`
- any `apps/iggy3d_creative/*.{hpp,cpp}` file
- `CMakeLists.txt`
- `cmake/iggy3d_tests.cmake`
- tests
- receipt fields or golden files
- fixture/package data
- production docs other than this task card if the builder workflow appends to
  it

Do not introduce `CreativeEditorState`.
Do not create `EditorFrame.{hpp,cpp}`.
Do not move `appendWireframeBoxEdges(...)` or any frame-stage code yet.
Do not change the capture script, capture frame numbers, save root, seeded
objects, object ids, selection behavior, place/move behavior, RoomBake preview,
frustum culling, UI projection, render submit behavior, or shutdown behavior.
Do not run broad CTest.
Do not launch a window.
Do not run `iggy3d_creative --capture` in this preflight.
Do not stage, commit, or push.

## Required Commands

Run current size and entrypoint inventory:

```sh
wc -l /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp
rg -n "int main|while \\(window\\.isOpen\\(\\)\\)|--capture|capture|frameIndex|pollEvents|isDrawable|drawableExtent|resize|SDL_GetKeyboardState|SDL_GetRelativeMouseState|applyProductCreativeFlyInput|buildStandaloneRoomBakePreviewScene|makeProductVulkanFrame|snapGroundToCellCenter|pickNearestVisualBoundsObject|runStandaloneCaptureScenarioStep|pickGizmoAxisFromProjectedShafts|dispatchToolInput|buildProductCreativeUiProjection|buildCreativeDocumentWireframeSegments|appendWireframeBoxEdges|cullStandaloneSceneRoomMeshesByFrustum|submitFrame|captureFrameToPng|shutdown" \
  /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp
```

Run focused source-list and standalone-test inventory:

```sh
rg -n "iggy3d_creative|StandalonePicking|StandalonePlacement|StandaloneFrustumCull|standalone_picking_tests|standalone_placement_tests|standalone_frustum_cull_tests|--capture" \
  /Users/kogaryu/iggy3d/CMakeLists.txt \
  /Users/kogaryu/iggy3d/cmake/iggy3d_tests.cmake \
  /Users/kogaryu/iggy3d/tests/unit \
  /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/done \
  --glob '*.cmake' --glob 'CMakeLists.txt' --glob '*.cpp' --glob '*.hpp' --glob '*.md'
```

Read focused line ranges from `main.cpp`:

```sh
sed -n '1,230p' /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp
sed -n '230,520p' /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp
sed -n '520,930p' /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp
sed -n '930,1320p' /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp
sed -n '1320,1660p' /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp
```

Run:

```sh
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over this done card after moving it.

## Expected Deliverable

Move this card to `done/` and append a completion brief with:

- files inspected;
- current `main.cpp` line count and frame-loop line span;
- current CMake source-list and standalone-test inventory;
- line-range classification table;
- list of existing standalone helper ownership seams;
- recommendation for the first implementation slice;
- whether the first slice should introduce `CreativeEditorState`,
  `EditorFrame.{hpp,cpp}`, a smaller helper, or no code yet;
- exact files to touch in E267;
- exact non-goals for E267;
- focused verification commands for E267;
- whether `--capture` should be required, optional, or deferred for E267;
- self-blockers.

## Candidate Follow-Up Shape

If a safe first implementation slice is identified, draft the next card as:

`E267: iggy3d_creative Main Split G1 - <Chosen Seam>`

Likely implementation constraints:

- preserve `main(int, char**)` behavior and return codes;
- preserve `--frames` and `--capture` parsing/default behavior;
- preserve capture script frame numbers and object ids;
- preserve seeded Floor and Crate setup;
- preserve render-submit reason and final capture proof strings;
- touch only `apps/iggy3d_creative/main.cpp`, any newly introduced
  `apps/iggy3d_creative/<ChosenSeam>.{hpp,cpp}`, `CMakeLists.txt` if needed,
  and the task card;
- run targeted build/tests only;
- do not run broad CTest;
- do not stage, commit, push, or launch an interactive window.

Focused verification for E267 should be chosen by this preflight, but likely
starts with:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative standalone_picking_tests standalone_placement_tests standalone_frustum_cull_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(standalone_picking_tests|standalone_placement_tests|standalone_frustum_cull_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Only require a deterministic capture command if the preflight explicitly
decides it is acceptable for the implementation lane:

```sh
(cd /Users/kogaryu/iggy3d && ./build/iggy3d_creative --capture /tmp/iggy3d_creative_e267_final.png --frames 32 > /tmp/iggy3d_creative_e267_final.log 2>&1)
rg "ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e267_final.log
```

## Completion Brief - E266

### Files inspected

- `docs/refactor_targets.md`
- `docs/creative_mode/builder_tasks/PRIORITY.md`
- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/*.{hpp,cpp}`
- `CMakeLists.txt`
- `cmake/iggy3d_tests.cmake`
- `tests/unit/standalone_picking_tests.cpp`
- `tests/unit/standalone_placement_tests.cpp`
- `tests/unit/standalone_frustum_cull_tests.cpp`
- Prior done-card grep context for standalone helper ownership, especially E66,
  E83, and E101.

### Current entrypoint shape

- `apps/iggy3d_creative/main.cpp`: 1651 lines.
- `main(int, char**)`: starts at line 230.
- Frame loop: starts at line 488 with `while (window.isOpen())`; loop body runs
  through line 1641.
- Shutdown/capture return tail: lines 1644-1650.
- The file still contains one small pure-ish file-local geometry helper,
  `appendWireframeBoxEdges(...)`, at lines 202-226.

### Current CMake and standalone-test inventory

- `iggy3d_creative` is an executable in `CMakeLists.txt:385` with these source
  files:
  - `apps/iggy3d_creative/main.cpp`
  - `CreativeRendererBootstrap.cpp`
  - `StandaloneBrushPalette.cpp`
  - `StandaloneCaptureScenario.cpp`
  - `StandaloneFrustumCull.cpp`
  - `StandaloneGizmo.cpp`
  - `StandalonePathEditing.cpp`
  - `StandalonePersistenceProof.cpp`
  - `StandalonePicking.cpp`
  - `StandalonePlacement.cpp`
  - `StandalonePreviewProxies.cpp`
  - `StandaloneRoomBakePreview.cpp`
- Focused standalone tests currently registered in `cmake/iggy3d_tests.cmake`:
  - `standalone_picking_tests`: `StandalonePicking.cpp` plus
    `StandalonePreviewProxies.cpp`
  - `standalone_placement_tests`: `StandalonePlacement.cpp` plus
    `StandaloneBrushPalette.cpp`
  - `standalone_frustum_cull_tests`: `StandaloneFrustumCull.cpp`

### Existing standalone helper ownership seams

- `CreativeRendererBootstrap.*`: renderer/bootstrap and capture PNG helper.
- `StandaloneCaptureScript.hpp`: fixed capture schedule and proof roles.
- `StandaloneCaptureScenario.*`: deterministic capture scenario step dispatch.
- `StandaloneBrushPalette.*`: descriptor-driven palette, footprints, and path
  seed points.
- `StandaloneFrustumCull.*`: room mesh frustum cull and receipt.
- `StandaloneGizmo.*`: gizmo axes, shafts, names, placement/move logging.
- `StandalonePathEditing.*`: path polyline and path/path-point move helpers.
- `StandalonePersistenceProof.*`: save/load/clear and persistence proof data.
- `StandalonePicking.*`: screen/world ray helpers, object picking, gizmo axis
  picking, and path-handle picking.
- `StandalonePlacement.*`: brush placement with undo.
- `StandalonePreviewProxies.*`: point/line/path/object visual bounds.
- `StandaloneRoomBakePreview.*`: document-to-room-bake preview scene.
- `StandaloneUndo.hpp`: app-local undo stack facade.

### Line-range classification

| Lines | Bucket | Dominant state/deps | Extraction read |
| --- | --- | --- | --- |
| 230-248 | argument parsing and capture defaults | `maxFrames`, `capturePath` | Bootstrap seam; not a frame stage. |
| 250-276 | SDL window and Vulkan renderer bootstrap | `SdlWindow`, `VulkanBackend`, renderer factory | Existing bootstrap helper owns part of this; changing first would touch launch behavior. |
| 279-288 | initial camera setup | `ProductCreativeFlyCamera` | Simple setup, but tied to later frame input state. |
| 290-308 | grid setup | map-maker grid constants | Setup seam, not first implementation value. |
| 310-367 | document seed/setup | `CreativeAppState`, seeded Floor/Crate, object ids | Behavior-sensitive; keep in `main()` until state struct exists. |
| 369-386 | save-root and persistence setup | save path, persistence proof helpers | Existing persistence helper owns operations; setup values stay local. |
| 389-486 | editor locals and latches | camera, undo, selection, capture script, drag latches, proof latches | This is the future `CreativeEditorState` candidate, but too broad for G1. |
| 488-511 | event polling, drawable/resize handling | `SdlWindow`, drawable extent, renderer resize | Genuine frame input/bootstrap stage; SDL/Vulkan dependent, higher risk. |
| 514-548 | camera input | SDL keyboard/mouse, relative mouse, fly camera | Real frame input seam; touches SDL and camera mutation. |
| 550-625 | tool-switch/save/load/delete/undo key handling | keyboard state, app state, undo, persistence | Interactive policy seam; mutates document and proof state. |
| 628-642 | frame scene and RoomBake preview construction | RoomBake preview, scene projection, `RenderFrameInput` | Useful stage but owns render-frame lifetimes. |
| 645-664 | aim-ground-cell resolution | camera ray, grid snapping | Small computation seam, but coupled to placement/capture paths. |
| 665-789 | object pick candidates and capture pick-proof logging | visual bounds, world ray, capture proof latches | Existing picking helper covers core math; integration logging stays local. |
| 791-872 | click selection | mouse state, selection id, screen/world picking | Mutates selection and proof logging; not first slice. |
| 874-904 | placement behavior | place mode, brush, undo, app state | Existing placement helper owns core behavior; integration stays local. |
| 907-926 | capture scenario dispatch | `StandaloneCaptureScenarioStepRequest`, capture script, app state | Existing helper owns scenario step; call assembly stays local. |
| 929-1024 | selection/gizmo/path-handle geometry and hit-test | selected object, gizmo shafts, path handle hits | Real overlay/picking seam, but many transient frame locals. |
| 1026-1351 | capture move script and interactive move/path/path-point drag | drag latches, undo, path/gizmo helpers, app state | Behavior-heavy movement policy; not first slice. |
| 1353-1367 | inspector UI projection | creative UI projection, menu frame, glyphs | Extractable later, but tied to frame UI buffers. |
| 1369-1391 | document wireframe construction | document wireframe, debug frame, overlay line buffers | Part of overlay build stage; buffer lifetime matters. |
| 1393-1534 | document/point/line/path/ghost/gizmo overlay construction | `combinedWireLines`, visual bounds, path handles, ghost preview | Highest payoff overlay seam, but must be designed around buffers that outlive `submitFrame(...)`. |
| 1536-1564 | dimension label/glyph merge | selected bounds, label layout, glyph buffer | Overlay/UI seam; buffer lifetime tied to submit. |
| 1566-1641 | frustum cull, submit, frame logging, frame-limit exit | frame pointers, frustum result, submit result, final logs | Submit-stage seam; behavior/log string sensitive. |
| 1644-1650 | capture PNG write, idle/shutdown, return code | capture output, renderer idle/shutdown | Shutdown seam; not a frame stage. |

### Option assessment

- Option A, a tiny app-local helper, is the safest E267 slice. Move only
  `appendWireframeBoxEdges(...)` to a new standalone helper translation unit.
  It has no SDL/window/Vulkan dependency, no document mutation, no capture
  schedule ownership, and no frame-buffer lifetime ambiguity.
- Option B, frame input, is valuable but touches event polling, drawable checks,
  renderer resize, relative mouse mode, and camera mutation. It should wait
  until a minimal state/deps shape is chosen.
- Option C, overlay build, has the highest line-count payoff, but its transient
  vectors and frame pointers must remain alive through `submitFrame(...)`.
  Extracting it first would require a state/lifetime design.
- Option D, `CreativeEditorState` first, matches the likely end state but is
  broader than needed for G1 and would churn most current locals before proving
  the source-list seam.

### Recommended E267 slice

Draft `E267: iggy3d_creative Main Split G1 - Wireframe Box Edges`.

Recommended API:

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

Recommended files to touch in E267:

- `apps/iggy3d_creative/main.cpp`
- new `apps/iggy3d_creative/StandaloneWireframeBoxEdges.hpp`
- new `apps/iggy3d_creative/StandaloneWireframeBoxEdges.cpp`
- `CMakeLists.txt`
- the E267 task card

Recommended CMake change:

- Add `apps/iggy3d_creative/StandaloneWireframeBoxEdges.cpp` to the
  `iggy3d_creative` executable source list near the other standalone helper
  sources.

Do not introduce `CreativeEditorState` or `EditorFrame.{hpp,cpp}` in E267.

### E267 non-goals

- Do not change `main(int, char**)`, return codes, `--frames`, or `--capture`
  parsing/default behavior.
- Do not move frame stages, event handling, camera input, resize behavior,
  placement, selection, capture scenario dispatch, submit, shutdown, or logging.
- Do not change capture script frame numbers, object ids, seeded Floor/Crate
  setup, save root, final capture proof strings, or render-submit reason
  strings.
- Do not change line color, thickness, edge order, `objectId = 0`, vector
  reserve behavior, or call-site order for selection/path/ghost overlays.
- Do not add `CreativeEditorState`, `EditorFrame.{hpp,cpp}`, broad CTest,
  staging, commit, push, or an interactive window launch.

### E267 focused verification

Required:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d_creative standalone_picking_tests standalone_placement_tests standalone_frustum_cull_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(standalone_picking_tests|standalone_placement_tests|standalone_frustum_cull_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Also run a focused trailing-whitespace scan over the touched source/header files
and the E267 task card.

`--capture` should be optional/manual for E267, not required, because it creates
a Vulkan window. If explicitly allowed for implementation verification, use:

```sh
(cd /Users/kogaryu/iggy3d && ./build/iggy3d_creative --capture /tmp/iggy3d_creative_e267_final.png --frames 32 > /tmp/iggy3d_creative_e267_final.log 2>&1)
rg "ROUNDTRIP|ROOM_BAKE final|FINAL frame|submit outcome" /tmp/iggy3d_creative_e267_final.log
```

### E267 self-blockers

- Stop if moving the helper requires changing overlay edge order, object ids,
  colors, thickness, or frame buffer lifetime.
- Stop if compile fallout requires moving overlay construction, selection,
  gizmo/path policy, renderer submit, or capture behavior.
- Stop if CMake/source-list changes affect targets other than
  `iggy3d_creative`.

### Preflight command results

- Current size and entrypoint inventory completed.
- Focused source-list and standalone-test inventory completed.
- Focused `main.cpp` line-range reads completed.
- `git -C /Users/kogaryu/iggy3d diff --check`: passed.
- Focused trailing-whitespace scan over this done card: passed.
- No production source, CMake, tests, fixture data, receipt files, golden files,
  staging, commit, push, broad CTest, window launch, or `--capture` run was
  performed.
