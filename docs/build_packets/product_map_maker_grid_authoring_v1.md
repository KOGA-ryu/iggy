# Product Map Maker Grid Authoring v1

Purpose: make player-authored 3D asset/map creation visible and usable in the
app. The first working layer is creative fly navigation plus a 3D drafting grid
made of snap dots. Later slices add point selection, connecting lines, radius
tools, bake/export, and save durability.

This packet is implementation-facing. Use it as the first reference before
building map-maker features.

## Product Direction

- Normal gameplay remains player movement with gravity, collision, jump, dash,
  clamber, etc.
- Map-maker mode is an authoring/navigation mode. It should feel like creative
  fly: camera-relative movement, vertical up/down controls, no gravity, no
  floor-collision dependency.
- The map maker owns drafting data. It should not be a renderer-only effect.
- The visible grid is a 3D authoring surface:
  - base world unit: `1.0` meter;
  - default grid pitch: `1.0` meter;
  - supported pitch values: `1.0`, `0.5`, `0.25`, `0.1` meters;
  - major dot/mark step: `5.0` meters;
  - grid plane starts at `y=0.0` and can later move by story/elevation.
- The first grid should draw dots/snap points, not full grid lines. Lines are a
  later authored primitive.

## Existing Files To Reuse

Input and action routing:

- `src/app/input/InputAction.hpp`
  - Add new actions here when map-maker controls become real input, such as
    `MapMakerToggle`, `MapMakerMoveUp`, `MapMakerMoveDown`,
    `MapMakerGridPitchNext`, `MapMakerGridPitchPrevious`,
    `MapMakerSelectPoint`, and later line-tool actions.
- `src/app/input/KeyboardInput.hpp`
- `src/app/input/KeyboardInput.cpp`
  - Add keyboard edge guards and polling for map-maker keys.
  - Keep edge-triggered toggles guarded with `WasDown` fields, like F3 and
    editor controls.
- `src/app/input/GamepadInput.hpp`
- `src/app/input/GamepadInput.cpp`
  - Add controller samples only after keyboard/no-window proof exists.
- `src/app/iggy3d/window/InputFrame.hpp`
- `src/app/iggy3d/window/InputFrame.cpp`
  - Central product input frame. This is where app/window input should dispatch
    map-maker actions, similar to editor and gameplay actions.
  - Do not bypass this with direct SDL calls inside map-maker files.
- `tests/unit/product_window_input_frame_tests.cpp`
- `tests/unit/room_editor_input_tests.cpp`
  - Extend or mirror these for map-maker input edge/toggle proof.

Gameplay and camera/state routing:

- `src/app/iggy3d/gameplay/Controller.cpp`
  - Current product gameplay action routing reads `PlayerMoveX`, `PlayerMoveY`,
    `PlayerJump`, `PlayerSprint`, `PlayerDash`, etc.
  - Creative fly should branch before normal movement submission when map-maker
    mode owns input. Do not send creative fly movement through `Session`
    movement commands.
- `src/app/iggy3d/gameplay/MovementTuning.hpp`
  - Existing product movement feel constants live here.
  - Add creative fly tuning here only if the tuning is product-wide and simple,
    for example `creativeFlySpeedMetersPerSecond`,
    `creativeFlySprintMultiplier`, and `creativeFlyInputStepSeconds`.
- `src/app/iggy3d/input/InteractionMode.hpp`
  - Existing product interaction modes should be checked before adding a new
    mode string. If `Creative` or editor ownership exists, reuse it; otherwise
    add a map-maker/authoring mode there.
- `tests/unit/product_gameplay_controller_tests.cpp`
  - Prove normal gameplay movement is unchanged and map-maker mode uses creative
    fly without submitting runtime movement commands.

Room-editor and authoring model:

- `src/content/authoring/EditableRoomDocument.hpp`
  - Current durable editable primitives are floors, walls, and objects.
  - Do not put transient grid dots here.
  - Later point/line/stroke primitives can either become new editable-room
    authoring primitives or bake into floors/walls/objects. Decide when lines
    become persistent.
- `src/app/iggy3d/room_editor/Cursor.hpp`
- `src/app/iggy3d/room_editor/Cursor.cpp`
  - Current room editor cursor has `gridX`, `gridZ`, `storyIndex`, and
    `cellSizeMeters`.
  - Reuse its coordinate assumptions when translating grid snap points to
    room-edit commands.
- `src/app/iggy3d/room_editor/Preview.hpp`
- `src/app/iggy3d/room_editor/Preview.cpp`
  - Existing phantom preview/dry-run pattern. Use this style for future line or
    shape preview.
- `src/app/iggy3d/room_editor/Presentation.hpp`
- `src/app/iggy3d/room_editor/Presentation.cpp`
  - Existing HUD/overlay model pattern. Map-maker presentation should follow
    this shape instead of writing text directly in the renderer.
- `src/app/iggy3d/room_editor/EditingState.hpp`
- `src/app/iggy3d/room_editor/EditingState.cpp`
  - Existing product editing state and active-room rebuild path.
- `src/app/iggy3d/room_editor/AuthoringController.hpp`
- `src/app/iggy3d/room_editor/AuthoringController.cpp`
  - Existing command -> editable document -> bake -> projection path.
  - Future line/radius tools should eventually route through this style of
    command path once they mutate authored geometry.

Projection, draw-list, and render bridge:

- `src/app/iggy3d/gameplay/ProjectionRefresh.hpp`
- `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`
  - Product frame assembly owner. Add map-maker overlay/HUD models to
    `ProductGameplayProjectionFrame` here.
  - Copy receipt/window proof fields here, following room-editor HUD/overlay
    copy helpers.
- `src/app/iggy3d/view/PrimitiveDrawList.hpp`
- `src/app/iggy3d/view/PrimitiveDrawList.cpp`
  - Add product primitive kinds for visible map-maker items:
    `MapMakerGridDot`, later `MapMakerSelectedPoint`,
    `MapMakerDraftLine`, and `MapMakerLineRadiusPreview`.
  - Add counts:
    `mapMakerGridVisible`, `mapMakerGridDotCount`,
    `mapMakerMajorGridDotCount`, later `mapMakerLineCount`.
  - Build draw items from a map-maker overlay/snapshot, not from renderer state.
- `src/app/iggy3d/view/ViewportFraming.hpp`
- `src/app/iggy3d/view/ViewportFraming.cpp`
  - Existing first-person primitive projection.
  - Grid dots should use the same projection path as room geometry so they
    appear in the world and respect camera yaw/pitch/depth.
- `src/app/iggy3d/view/RenderBridge.hpp`
- `src/app/iggy3d/view/RenderBridge.cpp`
  - Mirror map-maker visible/count fields after draw-list support lands.
- `src/app/iggy3d/view/OpeningMenuView.cpp`
  - SDL fallback draw path. Add compact drawing for map-maker grid dots after
    draw-list support exists.
  - Keep HUD text compact and out of existing top-left position/debug text.
- `src/app/iggy3d/window/FramePresenter.hpp`
- `src/app/iggy3d/window/FramePresenter.cpp`
  - Vulkan product presentation bridge. If map-maker HUD text is added, it
    needs a Vulkan UI bridge just like `PositionHud`.
  - Grid dots that are primitive draw items should flow through the existing
    room/product primitive path where possible.
- `tests/unit/product_primitive_draw_list_tests.cpp`
- `tests/unit/product_viewport_framing_tests.cpp`
- `tests/unit/product_render_bridge_tests.cpp`
- `tests/unit/product_vulkan_room_frame_tests.cpp`
  - Focused projection/draw/render proof targets.

Receipts and no-window proof:

- `src/app/iggy3d/ReceiptBuilder.hpp`
- `src/app/iggy3d/ReceiptBuilder.cpp`
  - `ProductAppWindowState` is the current receipt source.
  - Add narrow mirror fields only for state that is useful to prove no-window
    behavior. Do not dump full grid points or full authored documents.
- `tests/smoke/product_ascii_map_smoke.cpp`
- `tests/smoke/product_gameplay_controls_smoke.cpp`
  - Add smoke proof only after unit proof is stable.

CMake/test registration:

- `CMakeLists.txt`
  - Register any new `src/app/iggy3d/map_maker/*.cpp` files in the `iggy3d`
    library source list.
- `cmake/iggy3d_tests.cmake`
  - Register new unit tests with `iggy3d_add_unit_test(...)`.

## New Files To Add

Create a new product-owned folder:

- `src/app/iggy3d/map_maker/Grid.hpp`
- `src/app/iggy3d/map_maker/Grid.cpp`
- `src/app/iggy3d/map_maker/CreativeFly.hpp`
- `src/app/iggy3d/map_maker/CreativeFly.cpp`
- `src/app/iggy3d/map_maker/Presentation.hpp`
- `src/app/iggy3d/map_maker/Presentation.cpp`

Add tests:

- `tests/unit/product_map_maker_grid_tests.cpp`
- `tests/unit/product_creative_fly_tests.cpp`
- `tests/unit/product_map_maker_presentation_tests.cpp`

Add these later, not in the first visible slice:

- `src/app/iggy3d/map_maker/Document.hpp`
- `src/app/iggy3d/map_maker/Document.cpp`
- `src/app/iggy3d/map_maker/LineTool.hpp`
- `src/app/iggy3d/map_maker/LineTool.cpp`
- `src/app/iggy3d/map_maker/Bake.hpp`
- `src/app/iggy3d/map_maker/Bake.cpp`
- `tests/unit/product_map_maker_document_tests.cpp`
- `tests/unit/product_map_maker_line_tool_tests.cpp`
- `tests/unit/product_map_maker_bake_tests.cpp`

## First Slice: Creative Fly And Visible Dot Grid

Name: `Product Map Maker / Slice 1 - Creative Fly And Grid Preview`

Behavior:

- Add explicit product map-maker mode, default off.
- Toggle map-maker mode through an input action and/or automation key.
- In map-maker mode:
  - WASD moves camera/player anchor on the camera plane;
  - Space moves up;
  - Ctrl or Shift moves down if free; if sprint already uses Shift, prefer Ctrl
    for down and keep Shift as speed multiplier;
  - sprint modifier increases fly speed;
  - no gravity;
  - no jump/dash/clamber commands;
  - no runtime physics movement command submission.
- Show a 3D dot grid on the current authoring plane:
  - default pitch `1.0m`;
  - major dots every `5.0m`;
  - default extent around the camera/player anchor, for example `40m x 40m`;
  - dots should be visible in the first-person viewport and fade by size/color,
    not by alpha.
- Add a compact map-maker HUD line in the lower-left or lower-center gameplay
  area:
  - `MAP grid=1m major=5m y=0`
  - keep it tight, one or two rows max;
  - do not overlap the top-left position HUD or top-right debug/NPC panels.

Suggested receipt fields:

- `map_maker_active`
- `map_maker_status`
- `map_maker_reason_code`
- `creative_fly_active`
- `creative_fly_speed_mps`
- `map_maker_grid_visible`
- `map_maker_grid_pitch_meters`
- `map_maker_grid_major_step_meters`
- `map_maker_grid_dot_count`
- `map_maker_grid_major_dot_count`
- `product_draw_map_maker_grid_visible`
- `product_draw_map_maker_grid_dot_count`
- `product_render_bridge_map_maker_grid_visible`
- `product_render_bridge_map_maker_grid_dot_count`

Do not add save schema fields in Slice 1. The grid is a view/model aid until
the user creates points or lines.

## Grid Model Methods

In `src/app/iggy3d/map_maker/Grid.hpp`:

```cpp
enum class ProductMapMakerGridStatus : std::uint8_t {
  Ready,
  Disabled,
  InvalidConfig,
};

struct ProductMapMakerGridConfig {
  bool enabled = false;
  float pitchMeters = 1.0F;
  float majorStepMeters = 5.0F;
  float extentXMeters = 40.0F;
  float extentZMeters = 40.0F;
  float planeY = 0.0F;
  Vec3 anchorWorld;
};

struct ProductMapMakerGridDot {
  Vec3 worldPosition;
  bool major = false;
};

struct ProductMapMakerGridSnapshot {
  bool ok = false;
  bool visible = false;
  ProductMapMakerGridStatus status = ProductMapMakerGridStatus::Disabled;
  std::string reasonCode = "map_maker_grid_disabled";
  float pitchMeters = 1.0F;
  float majorStepMeters = 5.0F;
  float planeY = 0.0F;
  std::uint64_t dotCount = 0;
  std::uint64_t majorDotCount = 0;
  std::vector<ProductMapMakerGridDot> dots;
};

bool isValidProductMapMakerGridConfig(const ProductMapMakerGridConfig& config);
std::string_view productMapMakerGridStatusName(ProductMapMakerGridStatus status);
ProductMapMakerGridSnapshot buildProductMapMakerGridSnapshot(
    const ProductMapMakerGridConfig& config);
float nextProductMapMakerGridPitch(float currentPitchMeters);
float previousProductMapMakerGridPitch(float currentPitchMeters);
```

Method policy:

- Pure builder only; no renderer calls and no window mutation.
- Validate finite positive pitch, major step, and extents.
- Snap the anchor to the nearest pitch cell before generating dots.
- Generate bounded dots only. Do not create a world-sized grid.
- Major dot if both X and Z are on a major-step multiple.
- Keep output deterministic: stable row-major order by Z then X.

## Creative Fly Methods

In `src/app/iggy3d/map_maker/CreativeFly.hpp`:

```cpp
struct ProductCreativeFlyConfig {
  bool enabled = false;
  float speedMetersPerSecond = 8.0F;
  float sprintMultiplier = 3.0F;
  float inputStepSeconds = 1.0F / 60.0F;
};

struct ProductCreativeFlyInput {
  float moveX = 0.0F;
  float moveY = 0.0F;      // forward/back on camera plane
  float moveZ = 0.0F;      // vertical up/down
  bool sprinting = false;
  float cameraYawDegrees = 0.0F;
  float cameraPitchDegrees = 0.0F;
};

struct ProductCreativeFlyResult {
  bool applied = false;
  std::string reasonCode = "creative_fly_not_requested";
  Vec3 deltaMeters;
  Vec3 finalPositionMeters;
  float speedMetersPerSecond = 0.0F;
};

bool isValidProductCreativeFlyConfig(const ProductCreativeFlyConfig& config);
ProductCreativeFlyResult applyProductCreativeFlyInput(
    const ProductCreativeFlyConfig& config,
    const ProductCreativeFlyInput& input,
    Vec3 startPositionMeters);
```

Method policy:

- Product-only navigation. Do not call `Session::tick(...)` or
  `executeMovement(...)`.
- Use yaw to build camera-forward and camera-right vectors on the horizontal
  plane.
- Vertical movement is direct world Y.
- Normalize combined input so diagonal fly does not move faster.
- Clamp/ignore nonfinite input.
- Store the resulting position on product/player camera state only after the
  product controller accepts map-maker mode.

## Presentation Methods

In `src/app/iggy3d/map_maker/Presentation.hpp`:

```cpp
struct ProductMapMakerGridOverlay {
  bool visible = false;
  std::string status = "map_maker_grid_disabled";
  std::string reasonCode = "map_maker_grid_disabled";
  float pitchMeters = 1.0F;
  float majorStepMeters = 5.0F;
  float planeY = 0.0F;
  std::uint64_t dotCount = 0;
  std::uint64_t majorDotCount = 0;
  std::vector<ProductMapMakerGridDot> dots;
};

struct ProductMapMakerHudLine {
  std::string text;
  bool visible = false;
};

struct ProductMapMakerHud {
  bool visible = false;
  std::uint64_t lineCount = 0;
  std::vector<ProductMapMakerHudLine> lines;
};

ProductMapMakerGridOverlay buildProductMapMakerGridOverlay(
    const ProductMapMakerGridSnapshot& snapshot);
ProductMapMakerHud buildProductMapMakerHud(
    bool mapMakerActive,
    const ProductMapMakerGridSnapshot& grid);
```

Presentation policy:

- HUD text stays compact.
- Do not store full HUD text in receipts.
- Overlay carries dot data for draw-list conversion.

## Draw List Mapping

Add to `ProductPrimitiveDrawKind`:

- `MapMakerGridDot`
- later: `MapMakerSelectedPoint`
- later: `MapMakerDraftLine`
- later: `MapMakerLineRadiusPreview`

Add to `ProductPrimitiveDrawList`:

- `bool mapMakerGridVisible = false;`
- `std::uint64_t mapMakerGridDotCount = 0;`
- `std::uint64_t mapMakerMajorGridDotCount = 0;`

Add `appendMapMakerGridOverlay(...)` in `PrimitiveDrawList.cpp`:

- input: `const ProductMapMakerGridOverlay* overlay`;
- skip null, hidden, or zero-dot overlays;
- emit one `ProductPrimitiveDrawItem` per dot;
- `stableName = "map_maker.grid_dot"` or `"map_maker.grid_major_dot"`;
- `worldPosition = dot.worldPosition`;
- `worldBounds = small AABB around dot`, for example extents
  `{0.035F, 0.035F, 0.035F}` for minor and `{0.060F, 0.060F, 0.060F}` for
  major;
- color minor `{86, 130, 172}`;
- color major `{136, 184, 226}`;
- marker size minor `8.0F`;
- marker size major `13.0F`;
- update counts without treating grid dots as room geometry, targets, or
  physics debug.

## Projection Refresh Wiring

Extend `ProductGameplayProjectionFrame` with:

- `ProductMapMakerGridSnapshot mapMakerGrid;`
- `ProductMapMakerGridOverlay mapMakerGridOverlay;`
- `ProductMapMakerHud mapMakerHud;`

In `ProjectionRefresh.cpp`:

- build grid only when map-maker mode is active;
- anchor it to player/camera position;
- pass overlay to `buildProductPrimitiveDrawList(...)`;
- copy grid/HUD proof fields to `ProductAppWindowState`;
- keep normal gameplay projection working when map-maker mode is off.

## Receipt Builder Fields

Add receipt fields only as narrow proof mirrors:

- `map_maker_active`
- `map_maker_status`
- `map_maker_reason_code`
- `creative_fly_active`
- `creative_fly_speed_mps`
- `map_maker_grid_visible`
- `map_maker_grid_pitch_meters`
- `map_maker_grid_major_step_meters`
- `map_maker_grid_dot_count`
- `map_maker_grid_major_dot_count`
- `product_draw_map_maker_grid_visible`
- `product_draw_map_maker_grid_dot_count`
- `product_render_bridge_map_maker_grid_visible`
- `product_render_bridge_map_maker_grid_dot_count`

Do not add full dot lists, selected points, line lists, or document blobs to
receipts.

## Future Slice: Point Selection

New state:

- `ProductMapMakerSelectedPoint`
- `ProductMapMakerCursor`

Methods:

- `pickNearestProductMapMakerGridDot(...)`
- `buildProductMapMakerSelectedPointOverlay(...)`

Rules:

- selection must use the same grid pitch and plane as the visible grid;
- show the selected dot with a distinct primitive kind;
- receipt mirrors selected world X/Y/Z and grid index only.

## Future Slice: Lines Between Dots

New document file:

- `src/app/iggy3d/map_maker/Document.hpp`
- `src/app/iggy3d/map_maker/Document.cpp`

Suggested model:

```cpp
struct ProductMapMakerPoint {
  std::string id;
  Vec3 positionMeters;
};

struct ProductMapMakerLine {
  std::string id;
  std::string startPointId;
  std::string endPointId;
  float radiusMeters = 0.05F;
  std::string materialId = "debug_line";
};

struct ProductMapMakerDocument {
  std::string id = "map_maker_document";
  std::uint32_t version = 1;
  std::vector<ProductMapMakerPoint> points;
  std::vector<ProductMapMakerLine> lines;
};
```

Line tool methods:

- `beginProductMapMakerLine(...)`
- `previewProductMapMakerLine(...)`
- `confirmProductMapMakerLine(...)`
- `cancelProductMapMakerLine(...)`

Rules:

- two selected dots create one line;
- preview never mutates the document;
- confirm mutates the document;
- undo/redo should follow the existing room-editor command pattern if lines
  become real editable primitives.

## Future Slice: Radius Lines And Bake

Add:

- `src/app/iggy3d/map_maker/Bake.hpp`
- `src/app/iggy3d/map_maker/Bake.cpp`

Bake options:

1. Visual-only draft lines:
   - render as debug/authoring primitives only;
   - no collision.
2. Room geometry lines:
   - bake radius lines into `EditableRoomWall`, `EditableRoomObject`, or a new
     authoring primitive;
   - use `EditableRoomDocument` and `ProductRoomAuthoringController` for
     mutation.
3. Exportable asset lines:
   - export to authored room/save only after line semantics are stable.

Do not add save schema for lines until there is a document that can be roundtrip
tested.

## Verification Commands

First visible slice:

```sh
cmake --build build --target iggy3d_app
cmake --build build --target product_creative_fly_tests product_map_maker_grid_tests product_map_maker_presentation_tests
ctest --test-dir build --output-on-failure -R '^(product_creative_fly|product_map_maker_grid|product_map_maker_presentation)_tests$'
cmake --build build --target product_primitive_draw_list_tests product_render_bridge_tests product_vulkan_room_frame_tests product_window_input_frame_tests
ctest --test-dir build --output-on-failure -R '^(product_primitive_draw_list|product_render_bridge|product_vulkan_room_frame|product_window_input_frame)_tests$'
git diff --check
tools/check_branch_gate.py
```

Before commit:

```sh
git diff --cached --check
tools/check_branch_gate.py --cached
```

After commit:

```sh
tools/check_branch_gate.py --diff HEAD~1..HEAD
```

## Hard Boundaries

- Do not change physics solver math for creative fly.
- Do not send creative fly through runtime movement commands.
- Do not add save/load schema for grid preview.
- Do not make map-maker grid depend on debug overlay. It is an authoring tool,
  not a dev-only debug panel.
- Do not create a second room format for final geometry. Long-term authored
  geometry should bake into the existing editable-room/authored-room path.
- Do not let grid dots count as room geometry, targets, or physics debug items.
