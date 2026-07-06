# E78: Standalone Section Map Dependency Audit

## Objective

Produce a concrete extraction map for `apps/iggy3d_creative/main.cpp` before
more standalone refactor slices move code.

## Problem

`apps/iggy3d_creative/main.cpp` is still about 3,685 lines after the first
helper extractions. The next refactor lane needs to stay organized by ownership
boundary, not by whichever helper is easiest to cut.

Without a section/dependency map, standalone cleanup can turn into another pile
of generic utility headers. The goal is to keep `main.cpp` as orchestration:
argument parsing, app/render initialization, frame loop/capture, and calls into
named standalone systems.

## Required Reads

- `apps/iggy3d_creative/main.cpp`
- `apps/iggy3d_creative/StandaloneUndo.hpp`
- `apps/iggy3d_creative/StandaloneCaptureScript.hpp`
- `apps/iggy3d_creative/AGENTS.md`
- `docs/creative_mode/standalone_app_handoff.md`
- `docs/creative_mode/builder_tasks/ready/E77-standalone-visual-proxy-extraction.md`

## Scope

- Read-only audit. Do not edit source files.
- Append a completion brief to this card with:
  - current `main.cpp` line count,
  - function/type groups and line ranges,
  - dependencies between groups,
  - proposed destination modules,
  - recommended extraction order after this audit.
- Identify whether E77 should extract `.hpp/.cpp` files with CMake edits or a
  header-only helper, and explain why.

## Acceptance

- The completion brief gives exact line ranges and destination files for at
  least these groups:
  - Vulkan/capture frame plumbing,
  - grid/preview scene rendering,
  - brush/palette/placement policy,
  - visual proxy rendering,
  - picking/hit testing,
  - gizmo/move/path-handle interaction,
  - undo wrappers,
  - RoomBake preview consumption,
  - save/open round-trip proof,
  - scripted capture scenario,
  - logging/proof receipts,
  - `main(...)` orchestration.
- The audit distinguishes behavior-preserving extraction candidates from
  feature/policy changes that must not be folded into this refactor lane.
- No source behavior changes.

## Suggested Checks

- `wc -l /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp`
- `rg -n "^(struct|enum class|bool|void|auto|std::|int main|constexpr|static)" /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp`
- `git -C /Users/kogaryu/iggy3d diff --check`

## Do Not

- Do not move code in this card.
- Do not change capture behavior, RoomBake policy, brush policy, save/load, or
  creative object semantics.
- Do not create a generic utility dump as the proposed direction.

## Completion Brief

### Current size

- `apps/iggy3d_creative/main.cpp`: 3,685 lines.
- Existing extracted helpers:
  - `apps/iggy3d_creative/StandaloneUndo.hpp`: app-local wrapper around
    `creative::CreativeDocumentUndoStack`, logging, push/pop/discard/clear.
  - `apps/iggy3d_creative/StandaloneCaptureScript.hpp`: deterministic capture
    schedule, proof roles, placement script, proof target ids, and round-trip
    snapshot state.

### Current function/type groups and ranges

- File banner/includes/standalone aliases: `main.cpp:1-87`.
- Vulkan/capture frame plumbing:
  - renderer config/create: `main.cpp:88-143`.
  - PNG capture: `main.cpp:145-168`.
  - Destination: keep in `main.cpp` for now or later extract to
    `StandaloneRuntime.hpp/.cpp` only if app bootstrap grows further.
- Grid and projection primitives:
  - grid mesh append: `main.cpp:170-207`.
  - screen projection helpers: `ScreenAabb`, `clipW`,
    `projectBoxToScreen`, `toVec3`: `main.cpp:209-273`.
  - Destination: grid append belongs with future
    `StandaloneRoomBakePreview` or `StandaloneSceneBuild`; projection helpers
    should move with picking in E79 because picking and handle hit tests share
    them.
- Brush/palette/placement policy:
  - `BrushFootprint`, placement constants, path-point utilities, descriptor
    eligibility, palette construction, brush cycling, render-role policy:
    `main.cpp:275-573`.
  - place/drop/delete/select helpers: `main.cpp:946-1131`.
  - Destination: `StandaloneBrushPalette.hpp/.cpp` for descriptor eligibility,
    footprint, palette, brush cycling, and `placeBrushObject*`; delete/select
    helpers should stay in an interaction/command helper unless E80 chooses to
    include them with placement.
- Visual proxy rendering:
  - `VisualBounds`, point/line/path visual bounds, axis selection, path segment
    proxy generation, baked-static-mesh source suppression, standalone preview
    proxy append: `main.cpp:575-944`.
  - path polyline/debug helper: `main.cpp:1164-1188`.
  - point/line/path marker wireframe append in frame loop:
    `main.cpp:3426-3492`.
  - ghost preview bounds/wireframe: `main.cpp:3505-3555`.
  - Destination for E77:
    `apps/iggy3d_creative/StandalonePreviewProxies.hpp/.cpp`.
- Picking/hit testing:
  - screen/clip primitive dependencies: `main.cpp:209-273`.
  - `WorldRay`, object visual pick bounds/result, vector math, world ray,
    ray/AABB pick: `main.cpp:708-858`.
  - object candidate build + hit-proxy proof logs: `main.cpp:2112-2239`.
  - click routing: `main.cpp:2241-2323`.
  - path-handle hit structs/projection/picking: `main.cpp:2950-3054`.
  - Destination for E79:
    `apps/iggy3d_creative/StandalonePicking.hpp/.cpp`.
- Gizmo/move/path-handle interaction:
  - `ScreenPoint`, point projection, segment distance, `GizmoAxis`,
    `GizmoAxisShaft`, held-axis/name helpers: `main.cpp:1190-1280`.
  - move logging and generic move-with-undo helpers:
    `main.cpp:1282-1372`.
  - whole-path/path-point mutation helpers: `main.cpp:1374-1530`.
  - selected object/gizmo state and interactive move/path handle handling:
    `main.cpp:2905-3377`.
  - Destination after E79:
    `StandaloneMoveInteraction.hpp/.cpp`, but only after visual bounds and
    picking are extracted.
- Undo wrappers:
  - App-local undo implementation already lives in `StandaloneUndo.hpp`.
  - `main.cpp` remaining undo coupling is command-site push/discard around
    create/delete/move/path edits: `main.cpp:1024-1091`, `1344-1530`,
    `2484-2861`, and interactive commit sites `main.cpp:3221-3255`,
    `3343-3355`.
  - Destination: leave command-site calls near the owning interaction helper;
    do not centralize into a generic undo utility dump.
- RoomBake preview consumption:
  - per-frame `CreativeRoomBakeRequest`, `buildSceneProjection(...)`,
    grid append, standalone preview proxy append:
    `main.cpp:2059-2083`.
  - final RoomBake receipt log: `main.cpp:3619-3647`.
  - Destination:
    `StandaloneRoomBakePreview.hpp/.cpp` after E77, because it should call the
    extracted preview-proxy suppression API instead of owning duplicate policy.
- Save/open round-trip proof:
  - snapshot/log/compare helpers: `main.cpp:1532-1593`.
  - save/open/clear helpers: `main.cpp:1595-1658`.
  - scripted save/clear/load proof: `main.cpp:2863-2903`.
  - Destination:
    `StandalonePersistenceProof.hpp/.cpp` once capture scenario code is
    separated from core frame orchestration.
- Scripted capture scenario:
  - schedule/state is already in `StandaloneCaptureScript.hpp`.
  - main-loop execution blocks still live in `main.cpp`:
    placements `2325-2482`, delete/create-undo `2484-2520`,
    move proof `2522-2585`, Point proof `2587-2674`, Line proof
    `2676-2782`, Path/path-handle proof `2784-2861`,
    save/load proof `2863-2903`.
  - Destination:
    `StandaloneCaptureScenario.hpp/.cpp`, but only after placement, picking,
    move, and persistence helpers expose clean APIs.
- Logging/proof receipts:
  - placement/delete/select/move/path/snapshot/final logs are distributed
    through the owning code: `main.cpp:1002-1020`, `1057-1107`,
    `1110-1131`, `1282-1530`, `1553-1568`, `2141-2239`,
    `3600-3672`.
  - Destination: keep logs with the behavior they prove; do not make a global
    `StandaloneLogging` module.
- `main(...)` orchestration:
  - args/window/renderer/fly/grid/document seed/save root/state:
    `main.cpp:1662-1918`.
  - event/drawable/resize/input/fly/tool/save keys:
    `main.cpp:1919-2057`.
  - scene build/frame creation/aim/select/place/script/move/UI/wire/submit:
    `main.cpp:2059-3676`.
  - capture/shutdown return: `main.cpp:3678-3685`.
  - Target end state: `main.cpp` should retain argument parsing, SDL window and
    renderer lifetime, frame loop ordering, and calls into named standalone
    systems.

### Dependency graph

- `RoomBake preview consumption` depends on:
  - RoomBake adapter output,
  - product scene projection,
  - grid append,
  - preview proxy suppression by `CreativeRoomBakeResult.staticMeshSources`.
- `Visual proxy rendering` depends on:
  - descriptor shape facts,
  - path-point validation/helpers,
  - render role policy,
  - `VisualBounds`.
  It is consumed by RoomBake preview suppression, selection bounds,
  hit-proxy proof logs, path handle rendering, selected-object box sizing, and
  ghost preview.
- `Picking/hit testing` depends on:
  - `VisualBounds`,
  - screen projection helpers,
  - `RenderCameraFrame`,
  - current document objects.
  It is consumed by click-to-select, capture pick proofs, interactive gizmo
  grabs, and path-point handle selection.
- `Brush/palette/placement` depends on:
  - descriptor facts,
  - path initial-point policy,
  - undo push/discard wrappers.
  It is consumed by interactive Place and scripted placements.
- `Gizmo/move/path-handle interaction` depends on:
  - picking projection helpers,
  - `VisualBounds`,
  - undo wrappers,
  - creative facade tool dispatch and `SetPatrolRoute` mutation.
- `Capture scenario` depends on nearly every domain; it should be extracted
  late or kept as orchestration until the lower-level APIs are narrow.

### Proposed destination modules

- `StandalonePreviewProxies.hpp/.cpp`:
  - marker/proxy constants,
  - `VisualBounds`, `VisualMajorAxis`,
  - point/line/path visual bounds,
  - path segment proxy mesh append,
  - static-mesh-source suppression,
  - standalone preview proxy append,
  - path polyline debug helper if E77 wants all proxy visualization together.
- `StandalonePicking.hpp/.cpp`:
  - `ScreenAabb`, `ScreenPoint`, projection helpers,
  - `WorldRay`, visual pick candidate/result,
  - world ray construction, AABB ray entry, nearest visual pick,
  - path-point handle hit helpers if not owned by move interaction.
- `StandaloneBrushPalette.hpp/.cpp`:
  - `BrushFootprint`,
  - descriptor placement eligibility,
  - descriptor-derived footprint,
  - brush palette construction/cycling,
  - initial Path route points,
  - `placeBrushObject(...)` and `placeBrushObjectWithUndo(...)` if E80 chooses.
- `StandaloneRoomBakePreview.hpp/.cpp`:
  - build `CreativeRoomBakeRequest`,
  - project baked `RoomAsset`,
  - append grid and standalone preview proxies,
  - return scene plus receipt/proxy counts for final logging.
- `StandaloneMoveInteraction.hpp/.cpp`:
  - gizmo axis helpers,
  - move release with undo,
  - path whole-route and path-point mutation helpers,
  - interactive move state execution.
- `StandalonePersistenceProof.hpp/.cpp`:
  - document snapshot/log/compare,
  - standalone save/open/clear,
  - round-trip proof driver.
- `StandaloneCaptureScenario.hpp/.cpp`:
  - scripted proof execution once the lower-level modules are extracted.

### Recommended extraction order

1. E77: extract `StandalonePreviewProxies.hpp/.cpp`.
   - Do this as `.hpp/.cpp` with a small CMake source addition, not header-only.
   - Reason: the helper needs `SceneProjectionResult`, RoomBake source sidecars,
     descriptor APIs, path-point helpers, and render roles. Header-only would
     spread those dependencies into every includer and make `main.cpp` cleanup
     look smaller while compile ownership stays tangled.
2. E79: extract `StandalonePicking.hpp/.cpp`.
   - It should consume `VisualBounds` from E77 instead of duplicating bounds
     policy.
3. E80: extract brush palette and placement.
   - It should remain descriptor-driven and should not import picking.
4. Extract `StandaloneRoomBakePreview` after E77 so scene build owns only
   assembly, not preview-suppression policy.
5. Extract move/gizmo/path-handle interaction after E79 because it needs screen
   projection, path-handle hit tests, and selected visual bounds.
6. Extract persistence and capture scenario last; they are proof orchestration,
   not foundational policy.

### Behavior-preserving vs policy changes

- Behavior-preserving candidates:
  - Move visual proxy helpers, pick helpers, brush eligibility helpers,
    RoomBake preview assembly, save/open proof helpers, and capture script
    execution into named standalone modules without changing constants, logs,
    frame count, or RoomBake counts.
- Do not fold these policy changes into extraction:
  - new object/shape affordances,
  - new RoomBake bake policy,
  - changed brush eligibility or palette order,
  - changed path point placement defaults,
  - changed capture script frame schedule,
  - changed UI/save/load semantics,
  - renderer/Vulkan changes.

### Checks

- `wc -l /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp`
- `rg -n "^(struct|enum class|class|bool|void|auto|std::|int main|constexpr|static)" /Users/kogaryu/iggy3d/apps/iggy3d_creative/main.cpp`
- Targeted `nl -ba ... | sed -n` reads over the mapped ranges above.
