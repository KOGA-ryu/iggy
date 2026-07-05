# iggy3d_creative — Standalone Creative App: Handoff

> Handoff for Codex to take over `iggy3d_creative`, the standalone, Claude-owned
> level editor built 2026-07-05. This doc is the single source of truth: what it
> is, how to build/run/verify it, where everything is wired, the seams it reuses,
> and the laws that keep it maintainable. Read it fully before changing code.

---

## 1. What this is and why it exists

`iggy3d_creative` is a **separate executable** that IS the creative level editor —
deliberately split out of the product app (`iggy3d`). It boots straight into a
blank stage and does the full authoring loop:

- **Build**: fly, aim at a ground cell, green ghost preview, click to drop objects.
- **Edit**: click-select, inspect metadata, ground-plane move, 3-axis gizmo.
- **Persist**: save the scene to disk, clear, load it back losslessly.

### Why separate (the root problem)
The product app's Vulkan present (`window/FramePresenter.cpp` `presentProduct
VulkanFrame`, ~line 780) gates the entire 3D render on `scene.room.loaded`. The
creative blank stage has **no room** (`Operations.cpp:236` clears `activeRoom`),
so nothing submitted and the window froze on the last menu frame while the title
flipped to "Creative". Rather than bend the product's room-centric pipeline, the
standalone app owns its own present path with **no room gate, no god-struct
(`ProductAppWindowState`), no `FrontendState`/menu**. It is a *pure consumer* of
the `iggy3d` library.

### The prime directive
The **`creative::` kernel owns all truth and decisions** (document, tools,
mutations, spatial). The app is a **thin shell**: it translates input → generic
kernel calls, and kernel state → a `FrameInput` for the renderer. **If a change
adds object-kind branching or per-tool logic to the app, it's wrong** — the fix
belongs in a kernel *system*, so every object/tool inherits it. This is the
doctrine the whole build followed and it MUST continue.

---

## 2. Build / Run / Verify — the loop that killed "building blind"

```
cd ~/iggy3d
cmake --build build --target iggy3d_creative -j10     # build (ccache-backed, fast)
./build/iggy3d_creative                               # run interactively (needs a display)
```

**The critical capability — headless capture.** The app renders + reads back the
swapchain to a PNG under MoltenVK **with no interactive display** (works from a
plain shell), so you (and CI) can *see* a render without a screen:

```
./build/iggy3d_creative --capture /tmp/shot.png --frames 12
# then open/Read /tmp/shot.png — that IS the verification
```

- `--frames N` auto-exits after N presented frames (scriptable/deterministic).
- `--capture <path>` renders a few frames, writes `<path>.png` (+ .rgba/.meta/.sha256).
- In `--capture` mode the app SYNTHESIZES input (select/move/gizmo/place/save-load
  on fixed frame indices) so the captured frame is self-documenting. When you add
  a feature, add a synthesized `--capture` script for it and **look at the PNG**.
- The reliable "did it render" signal is the submit **reason**, logged each run:
  `empty_frame_presented` (nothing) vs `package_room_meshes_presented` (drawing).
  Do NOT trust `nonBackgroundPixelCoverage` — it reads ~1.0 regardless (the stage
  clear color isn't the metric's "background").

**Discipline:** every visual change → build → `--capture` → read the PNG → fix,
before it's "done". Unit-green is necessary but not sufficient; the editor must be
seen. (This app is not in ctest; its proof is the capture. The kernel it uses IS
in ctest — 236 tests; a kernel change must keep them green via a full
`cmake --build build && ctest`.)

Build tooling installed 2026-07-05: **ccache** (fast rebuilds), **ninja**
(available; switch via `rm -rf build && cmake -G Ninja ...`), and
`compile_commands.json` (symlinked at repo root for clangd).

---

## 3. File map — everything is ONE app file + reused kernel

### The app (edit THIS; it's the whole app)
- `apps/iggy3d_creative/main.cpp` — ~1861 lines, one translation unit. Structure:
  - Setup helpers: `makeCreativeVulkanRendererConfig` (161), `createCreativeRenderer`
    (185, returns `VulkanBackend` directly so capture is reachable),
    `captureFrameToPng` (218), `appendGridDotsToScene` (251), `clipW` (293),
    `renderRoleForKind` (350, kind→render color, the ONLY kind switch),
    `brushFootprintFor` (373), `nextBrushKind` (385), `snapGroundToCellCenter`
    (394), `appendWireframeBoxEdges` (452), `heldAxisForGrabbedAxis` (542),
    `loadStandaloneScene` (704), `clearToBlankScene` (731).
  - `main()` (744): seed objects (824), save location (883), per-mode state
    (Move 913 / Gizmo 924 / Place 944 / Save-Load 976), then the frame loop (999):
    INPUT (1025) → tool switch (1061) → save/load keys (1091) → SCENE build (1115)
    → aim→cell (1154) → click-select over ALL objects (1174) → PLACE (1299) →
    save/load round-trip (1345) → resolve selection (1379) → gizmo geometry (1400)
    → handle hit-test (1423) → MOVE (1469) → inspector UI (1688) → bounds box
    (1704) → gizmo wireframe (1728) → ghost (1748) → dimension label (1775) →
    attach overlays to frame (1804) → submit + capture.
- `CMakeLists.txt` — the target block (~line 305): `add_executable(iggy3d_creative
  apps/iggy3d_creative/main.cpp)` + `target_link_libraries(... PRIVATE iggy3d)` +
  `iggy3d_apply_warnings`. That's the ENTIRE cmake footprint; all reused sources
  are already in the `iggy3d` library.
- `apps/iggy3d_creative/AGENTS.md` — the short rules (points here).

### The render seam (neutral, product-state-free)
- `src/render/FrameInput.hpp` — `FrameInput { viewport; clock; camera; projections
  (nullable scene/debug ptrs); ui (RenderUiRect[] + DebugHudGlyphQuad[]);
  creativeWireframeDebug (RenderCreativeWireframeDebugLine[]) }`. `validateFrameInput`
  accepts `scene==nullptr` IF there's UI content (BG-1080).
- `src/render/vulkan/VulkanBackend.hpp` — `submitFrame / resize / waitIdle /
  shutdown / lifecycleState / frameCaptureReady / readLastFrameCapture`.
- `src/render/vulkan/FrameCapture.hpp` — `writePacket7CaptureArtifacts` (RGBA→PNG,
  no libpng).
- `src/render/vulkan/BufferImageResources.cpp` — `colorForRoomRole` (158, role→color:
  "floor" gray-blue, "prop" brown, "grid", "wall", etc.); `creativeDebugLineBox`
  (406) — **only draws AXIS-ALIGNED line segments** (`movedAxisCount==1`); its
  `thickness` is in **WORLD METRES** (a tube of `|edge| × thickness × thickness`).

### The kernel seams the app calls (do NOT reinvent)
- Camera/frame: `gameplay/ProjectionRefresh.hpp` `makeProductVulkanFrame` (~118) —
  all camera math (degrees; adds +1.7 m eye height to the anchor override); returns
  a `FrameInput` with `ui`/`creativeWireframeDebug` **left empty for the app to fill**.
- Grid: `map_maker/Grid.hpp` `buildProductMapMakerGridSnapshot` (50) — **all extents
  must be > 0** or the config is invalid (0 dots). App filters to the planeY layer.
- Container: `creative/CreativeAppState.hpp` — `CreativeAppState { Facade facade;
  CreativeActiveIdentity identity; }`.
- Facade: `creative/Facade.hpp` — `installDocument` (173), `createDocumentObject`
  (165), `findObject` (175), `document()` (const, ~149), `setActiveTool` (156),
  `dispatchToolInput` (158), `selectionState()` (150), `toolState()` (149).
- Document/objects: `creative/document/Document.hpp` — `CreativeDocument::create` /
  `assignId` / `objects()` (187, returns `std::span<const CreativeObject>`);
  `CreativeDocumentCreateRequest` (61).
- Object model: `creative/document/Object.hpp` — `CreativeObject` (id/kind/name/
  transform/bounds/layerId/visible/locked/tags/parentId, ~164); `CreativeObjectKind`
  (35-162; e.g. `Floor` 41, `Crate`). NO per-instance skin — skin is descriptor data.
- Descriptor table: `creative/document/ObjectDescriptor.hpp` — `describeObject(kind)`
  (200) → per-kind metadata + defaults; rows in `.cpp` (110-228, Crate 185, Floor 115).
- Tools + Move: `creative/tools/Tools.hpp` — `CreativeToolInputPacket {kind, pointer}`,
  `CreativeToolPointerPacket {button, target, worldDestination, moveHeldAxis}`,
  `CreativeToolInputKind` (PointerPress/Move/Release), `Tool` (Select/Move/Measure/
  Navigate), `CreativeToolMoveHeldAxis` (X/Y/Z, default Z). The Move axis logic is
  `creative/Facade.cpp` `resolveMoveAnchor` + `holdMoveAxis` (view-agnostic; see §5).
- Inspector UI: `creative/ui/UiProjection.hpp` `buildProductCreativeUiProjection`
  (`ProductCreativeUiProjectionRequest{creative=&appState, virtualWidth, virtualHeight}`
  → `ProductUiDrawList`; `.model=nullptr` makes it call `facade.buildUiModel()`);
  `window/FramePresenter.hpp` `buildProductVulkanStarterMenuFrame` (70, `ProductUiDrawList`
  → owned `RenderUiRect[]` + `DebugHudGlyphQuad[]`).
- Wireframe: `creative/document/DocumentWireframe.hpp` `buildCreativeDocumentWireframeSegments`
  (190, 12 box edges/object); `creative/render/WireframeDebugLines.hpp`
  `buildProductCreativeWireframeDebugLines` (86); `window/FramePresenter.hpp`
  `buildProductCreativeWireframeDebugRenderFrame` (91).
- Text glyphs: `render/debug/DebugHudText.hpp` `layoutDebugHudTextAt` (36) → glyph quads.
- Math: `core/math/Mat4.hpp` `transformPoint` (does the perspective w-divide → NDC),
  `at(m,r,c)`.
- Save/Load: `creative/world/WorldService.hpp` — `saveCreativeWorld` (116,
  `CreativeWorldSaveRequest {saveRoot, saveId, document*, worldTitle, saveTitle, ...}`),
  `openCreativeWorld` (114, `CreativeWorldOpenRequest {saveRoot, saveId}` →
  `CreativeWorldOpenResult {accepted, document, objectCount, ...}`).

---

## 4. How the frame is wired (per iteration)

1. Poll SDL events; read keyboard/mouse; drive the fly camera
   (`applyProductCreativeFlyInput`, `creative/camera/Fly.hpp`).
2. Build `SceneProjectionResult scene{}`: grid dots (as `role="grid"` meshes) +
   **every** `document().objects()` as a `SceneRoomMeshItem` (role via
   `renderRoleForKind`). Skip `!visible`. Set `room.loaded=true`.
3. `FrameInput frame = makeProductVulkanFrame(scene, debug, ..., anchorOverride=flyPos)`
   — gives `frame.camera.clipFromWorld` (world→NDC, used for all picking/labels).
4. Resolve input by mode: click-select (hit-test all objects' projected AABBs, pick
   nearest, `dispatchToolInput` PointerPress with its `TargetRef`), Place (ghost +
   `createDocumentObject`), Move/Gizmo (`dispatchToolInput` PRESS/MOVE/RELEASE).
5. Build overlays into stack-local vectors that OUTLIVE submit:
   - `frame.ui` ← inspector (`buildProductCreativeUiProjection` →
     `buildProductVulkanStarterMenuFrame`) + dimension-label glyphs (merge into ONE
     glyph vector).
   - `frame.creativeWireframeDebug` ← one combined `RenderCreativeWireframeDebugLine`
     vector = document wireframe (selected object's edges recolored yellow) + gizmo
     axis shafts + ghost box. All segments AXIS-ALIGNED; thickness in metres (~0.03).
6. `backend->submitFrame(frame)`; in `--capture` grab the PNG.

Key: `makeProductVulkanFrame` leaves `frame.ui`/`frame.creativeWireframeDebug`
empty, so the app fills them after the call and before submit.

---

## 5. Laws & gotchas (learned the hard way this session)

- **Generic systems, never per-kind code.** Select/Inspect/Move/Gizmo/Place key off
  the *selection* + the *document*, not a hardcoded id or a kind switch. The only
  kind-specific code allowed: a color lookup (`renderRoleForKind`) and a footprint
  table (`brushFootprintFor`) — pure data. A new kind (e.g. Wall) should need only
  a descriptor row (kernel) + optionally those two data entries. **Proven with Floor.**
- **App edits ONLY `apps/iggy3d_creative/main.cpp`.** No product source, no
  CMakeLists. The one exception this session was a deliberate *kernel* change (below),
  gated by the full 236-test suite.
- **The one shared-kernel change made: view-agnostic Move.** `Facade.cpp`
  `resolveMoveAnchor`/`holdMoveAxis` + `Tools.hpp` `CreativeToolMoveHeldAxis`
  (default Z = product unchanged). The CALLER picks the held axis from its camera;
  the app uses **Y** (ground-plane slide). The held axis is re-applied AFTER
  document snap so it never rounds up a grid line (that was the "floating" bug).
  Superseded TD-7 (see `creative_tooling_v1.md`). Any kernel change: keep the
  default backward-compatible and run `cmake --build build && ctest` (236 green).
- **Renderer draws only AXIS-ALIGNED debug lines** (`creativeDebugLineBox`) — no
  diagonals (gizmo arrowheads must be axis-aligned segments or a tip box).
- **Debug-line `thickness` is WORLD METRES.** 3.0 = a 3 m tube (solid blob); use
  ~0.03 for a crisp wireframe on a 1 m object.
- **Grid config extents must be > 0** (`extentYMeters=0` → invalid → 0 dots).
- **The kernel viewport pick is TOP-DOWN grid-cell** (`ViewportPick`), wrong for a
  perspective fly camera — the app uses a CPU AABB raycast vs `clipFromWorld`
  (cull corners with `w<=0`).
- **Move/Place anchor = the anchor the FACADE uses** — `transform.position` for
  `hasTransform` kinds (Crate/Floor), NOT `bounds.min`. Pin non-moved axes to that
  so they're grid-aligned and snap to themselves.
- **`installDocument` clears the selection** (`resetTransientFacadeState`), so
  clear/load don't dangle. Overlay/tooling vectors must outlive `submitFrame`.
- **Known cosmetic:** the debug HUD font lacks `=` `(` `,` glyphs (render as `?`);
  gizmo shafts are short/no arrowheads; fly speed is frame-rate-coupled (no dt).

---

## 6. Status & next work

**Done (10 slices, each capture-verified):** blank stage + headless capture; Select
+ Inspect; ground-plane Move (+ view-agnostic kernel fix); 3-axis gizmo; Floor kind
+ multi-object generalization; in-world Place; Save/Load (lossless round-trip).

**Next candidates (in the app's generic style):**
- **Building power** — `delete` / `duplicate` / `rotate` tools (all via
  `dispatchToolInput` / generic mutations), a **Wall** kind (descriptor row + the
  two data entries → inherits everything).
- **Bake bridge** — the saved `CreativeDocument` → an optimized gameplay artifact;
  the reserved `creative/adapters/{Draw,RoomEd,ObjCat}` sockets + the optimization
  plans (`docs/creative_mode/optimization/`) are for exactly this. This is the seam
  to the game (creative is where the game is made; the saved doc bakes → the game
  runs it).
- **Generation pipeline** — terse tokens (`floor1:75x45;...`) → loops → ASCII →
  objects, per `creative_two_faces_vision.md`. Reuse/condense the ASCII authoring
  reference.
- **Polish** — gizmo arrowheads/length, grid-as-lines, dt-normalized fly, multi-slot
  save browser, the HUD font's missing punctuation glyphs.

**Related docs:** `creative_two_faces_vision.md` (the vision), `creative_tooling_v1.md`
(tooling laws/decisions, TD-7 superseded note), `optimization/` (bake program),
`world_foundation_plan_v0_1.md` (the CreativeDocument foundation contract).
