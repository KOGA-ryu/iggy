# E222 - Render Draw-Kind Metadata Remainder Audit

## Status

Done.

## Context

`docs/complexity_audit_v0_1.md` finding #5 originally recommended replacing
duplicated `ProductPrimitiveDrawKind` presentation facts with one metadata
table. E124 audited the seam and E127 implemented the base metadata catalog in
`src/app/iggy3d/view/PrimitiveDrawMetadata.hpp`.

Current known state:

- `ProductPrimitiveDrawKindMetadata` already owns base color and marker size
  for all current draw kinds.
- `PrimitiveDrawList.cpp` routes default/base construction through that catalog.
- E127 intentionally kept dynamic variants and decorative renderer-only colors
  local.
- The older complexity roadmap still mentions replacing the `RenderBridge`
  counting switch, while E127 classified bridge counting/target classification
  as genuine dispatch. That conflict needs a fresh read-only decision before an
  implementation card.

## Objective

Read-only audit the remaining draw-kind metadata/render duplication after E127
and decide whether any safe implementation slice remains.

Produce one of:

- a narrow implementation card recommendation, or
- a clear "no implementation card" recommendation if the remaining literals are
  render-local decoration/dispatch.

Do not edit source, tests, CMake, receipt golden, fixtures, or production docs.

## Files To Inspect

Production:

- `src/app/iggy3d/view/PrimitiveDrawList.hpp`
- `src/app/iggy3d/view/PrimitiveDrawList.cpp`
- `src/app/iggy3d/view/PrimitiveDrawMetadata.hpp`
- `src/app/iggy3d/view/OpeningMenuView.cpp`
- `src/app/iggy3d/view/RenderBridge.hpp`
- `src/app/iggy3d/view/RenderBridge.cpp`
- `src/app/iggy3d/view/ViewportFraming.cpp`
- any focused render/presenter file found by grep for `ProductPrimitiveDrawKind`

Tests:

- `tests/unit/product_primitive_draw_list_tests.cpp`
- `tests/unit/product_render_bridge_tests.cpp`
- any focused view/render tests found by grep for `ProductPrimitiveDrawKind`

Reference docs:

- `docs/complexity_audit_v0_1.md` finding #5 only
- `docs/creative_mode/builder_tasks/done/E124-primitive-draw-kind-metadata-audit.md`
- `docs/creative_mode/builder_tasks/done/E127-product-primitive-draw-kind-metadata-table.md`

## Questions To Answer

1. Which `ProductPrimitiveDrawKind` facts remain duplicated outside
   `PrimitiveDrawMetadata.hpp`?
2. Which remaining literals are primary/base presentation data that should move
   into metadata?
3. Which remaining literals are secondary/decorative renderer policy that
   should stay local unless a future secondary-color catalog is explicitly
   designed?
4. Which remaining switches over `ProductPrimitiveDrawKind` are data lookup,
   and which are genuine render/bridge dispatch?
5. Is `RenderBridge.cpp::countPhysicsDebugKind(...)` still genuine counting
   dispatch, or is there a small safe metadata flag/table slice?
6. Is the `MapMakerGridDot` major/minor distinction still dynamic policy based
   on `markerSize > 10.0F`, or should it become metadata/state instead?
7. Do existing tests pin the current deferred decisions well enough, or should
   the next card add guard tests before moving anything?

## Required Classification

In the completion brief, include a grouped classification with:

- file/function or block
- draw kind(s)
- literal/fact type: base color, marker size, secondary/decorative color,
  dynamic variant, render shape dispatch, bridge/count dispatch, target
  classification, or unknown
- whether it duplicates `PrimitiveDrawMetadata`
- current test coverage
- recommendation: move to metadata, keep local, needs owner decision, or no-op

Pay special attention to:

- `PlayerFocusIndicator`
- `RoomEditorCursor`
- `RoomEditorPlacementPreview`
- `ElevatedFloorTile`
- `RampTile`
- `BlockedSlopeTile`
- `WallTile`
- `PropTile`
- `DoorMarker`
- `MapMakerGridDot`
- `MapMakerCubePreview`
- the three physics debug draw kinds

## Non-Scope

Do not edit:

- source code
- test code
- CMake
- receipt golden
- fixtures
- production docs

Do not change:

- render output
- draw ordering
- draw-list counts
- render-bridge counts
- `OpeningMenuView` split or file layout
- receipt fields/order/values
- renderer/Vulkan code outside focused inspection

No staging, commit, push, or window launch.

## Required Commands

Run and report:

```sh
rg -n "ProductPrimitiveDrawKind|baseColorForProductPrimitiveDrawKind|markerSizeForProductPrimitiveDrawKind|ProductPrimitiveDrawKindMetadata|setColor\\(|markerSize|countPhysicsDebugKind|targetItem|MapMakerGridDot|MapMakerCubePreview" /Users/kogaryu/iggy3d/src/app/iggy3d/view /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'
rg -n "ProductPrimitiveDrawKind|ElevatedFloorTile|RenderBridge|PrimitiveDrawMetadata|metadata" /Users/kogaryu/iggy3d/docs/complexity_audit_v0_1.md /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/done/E124-primitive-draw-kind-metadata-audit.md /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/done/E127-product-primitive-draw-kind-metadata-table.md
git -C /Users/kogaryu/iggy3d diff --check
```

No build or CTest is required unless files are unexpectedly edited.

Run a focused trailing-whitespace scan over this card.

## Self-Blockers

Stop and report "no implementation card yet" instead of drafting code if:

- remaining literals are all secondary/decorative renderer policy
- routing them would require expanding metadata beyond base color/marker size
  without an owner decision
- table-driving `RenderBridge` would hide clearer explicit counter semantics
- implementation would require splitting `OpeningMenuView.cpp`
- tests do not currently pin a proposed owner decision

## Completion Brief

When done, report:

- files inspected
- grouped classification
- exact stale roadmap assumptions, if any
- safe implementation recommendation, or no-card rationale
- tests/checks run
- confirmation that no source/test/CMake/receipt files were edited

## Completion Brief - E222

- Files inspected:
  - `src/app/iggy3d/view/PrimitiveDrawList.hpp`
  - `src/app/iggy3d/view/PrimitiveDrawList.cpp`
  - `src/app/iggy3d/view/PrimitiveDrawMetadata.hpp`
  - `src/app/iggy3d/view/OpeningMenuView.cpp`
  - `src/app/iggy3d/view/RenderBridge.hpp`
  - `src/app/iggy3d/view/RenderBridge.cpp`
  - `src/app/iggy3d/view/ViewportFraming.cpp`
  - `src/app/iggy3d/window/FramePresenter.cpp` because a broader grep still finds `MapMakerCubePreview` presentation there
  - `tests/unit/product_primitive_draw_list_tests.cpp`
  - `tests/unit/product_render_bridge_tests.cpp`
  - `tests/unit/product_viewport_framing_tests.cpp`
  - `tests/unit/product_room_editor_overlay_tests.cpp`
  - `tests/unit/product_map_maker_presentation_tests.cpp`
  - `docs/complexity_audit_v0_1.md` finding #5 only
  - `docs/creative_mode/builder_tasks/done/E124-primitive-draw-kind-metadata-audit.md`
  - `docs/creative_mode/builder_tasks/done/E127-product-primitive-draw-kind-metadata-table.md`

### Grouped Classification

| File / area | Draw kind(s) | Literal/fact type | Duplicates metadata? | Current coverage | Recommendation |
| --- | --- | --- | --- | --- | --- |
| `PrimitiveDrawMetadata.hpp` catalog | all 22 kinds | base color and marker size owner | N/A, this is the owner | `product_primitive_draw_list_tests.cpp` pins all rows and lookup helpers | No-op |
| `PrimitiveDrawList.cpp` construction defaults | scene markers, focus, room geometry defaults, room editor defaults, solid physics defaults, map-maker minor dot/cube | base color and marker size lookups | No, already routes through metadata | `product_primitive_draw_list_tests.cpp` pins row values plus constructed colors/sizes for representative paths | No-op |
| `PrimitiveDrawList.cpp::floorKindForSurface(...)` | `ElevatedFloorTile`, `RampTile`, `BlockedSlopeTile`, `FloorTile` | render-kind semantic dispatch from traversal tags | No | room geometry tests pin kind mapping/counts | Keep local |
| `PrimitiveDrawList.cpp::updateCounts(...)` | all kinds, including the three physics debug kinds | draw-list count dispatch | No | `product_primitive_draw_list_tests.cpp` pins scene, room, physics, editor, map-maker counts | Keep local |
| `PrimitiveDrawList.cpp::itemFromDoorMesh(...)` | `DoorMarker` | dynamic open/closed variant color and marker size | Partially overlaps closed base row, but state owns the variant | door tests pin open/closed counts and marker state | Keep local |
| `PrimitiveDrawList.cpp::itemFromPropMesh(...)` | `PropTile` | dynamic ledge/reset-zone variants | No, variants depend on mesh role/material | tests pin default prop and ledge color/size | Keep local |
| `PrimitiveDrawList.cpp::appendRoomEditorPlacementPreview(...)` | `RoomEditorPlacementPreview` | dynamic marker size by editor tool | Default size comes from metadata first; wall/object overrides are local policy | room editor overlay tests pin object `46.0F`; primitive draw-list tests pin preview count/kind | Keep local |
| `PrimitiveDrawList.cpp` physics style helpers | `PhysicsAabbDebug`, `PhysicsContactNormalDebug`, `PhysicsBroadphasePairDebug` | dynamic sensor style plus base metadata fallback | Base defaults route through metadata; sensor color remains local dynamic debug policy | primitive draw-list tests pin solid AABB/pair and sensor contact style | Keep local |
| `PrimitiveDrawList.cpp::appendMapMakerGridOverlay(...)` | `MapMakerGridDot` | dynamic major/minor map-maker variant | Minor dot uses metadata; major dot is local `dot.major` policy | primitive draw-list tests pin major larger than minor and count 1 major | Keep local |
| `PrimitiveDrawList.cpp::appendMapMakerCubePreview(...)` | `MapMakerCubePreview` | base color and marker size lookup | No | primitive draw-list tests pin kind, stable name, bounds/counts | No-op |
| `OpeningMenuView.cpp::drawFocusIndicator(...)` | `PlayerFocusIndicator` | primary color and fixed effective size in SDL renderer | Yes: `{226,230,211}` and 36-pixel cross duplicate catalog row intent | metadata and draw-list item values are pinned; no SDL pixel test pins renderer output | Move in a tiny renderer handoff card |
| `OpeningMenuView.cpp::drawRoomEditorCursor(...)` | `RoomEditorCursor` | primary color plus outline color | Primary `{245,214,96}` duplicates metadata; outline `{32,42,44}` is renderer decoration | metadata row and draw-list item values are pinned; no SDL pixel test pins renderer output | Move only primary color handoff; keep outline local |
| `OpeningMenuView.cpp::drawDoorMarker(...)` | `DoorMarker` | renderer shape and knob/detail color | No base duplication; consumes item color/size | no direct SDL pixel test; door item state is pinned upstream | Keep local |
| `OpeningMenuView.cpp::drawRoomTile(...)` | `ElevatedFloorTile`, `RampTile`, `BlockedSlopeTile`, `WallTile`, `PropTile`, `FloorTile` | secondary/decorative tile colors and shape dispatch | No base duplication after E127; overlay colors are distinct secondary facts | no direct SDL pixel test; draw-list base colors/kinds are pinned upstream | Keep local unless a future secondary-color catalog is explicitly designed |
| `OpeningMenuView.cpp::drawPrimitiveItem(...)` | all kinds | render shape dispatch | No | indirectly compiled; no pixel test | Keep switch local |
| `RenderBridge.cpp::isTargetItem(...)` | targetable kinds plus item flags | target classification | No | `product_render_bridge_tests.cpp` pins target count and physics/map-maker non-target behavior | Keep local |
| `RenderBridge.cpp::countPhysicsDebugKind(...)` | three physics debug kinds | bridge/count dispatch | No | `product_render_bridge_tests.cpp` pins copied and framed-only physics counts | Keep local; table-driving would hide explicit counter semantics |
| `RenderBridge.cpp` framed-item loop | `RoomEditorCursor`, `RoomEditorPlacementPreview`, `PropTile`, `MapMakerGridDot`, `MapMakerCubePreview` | bridge/count dispatch and framed fallback counts | No | `product_render_bridge_tests.cpp` pins preview/prop/map-maker counts | Keep local |
| `RenderBridge.cpp` map-maker major logic | `MapMakerGridDot` | dynamic variant inferred by `markerSize > 10.0F` | No; this is bridge fallback policy, not base metadata | `product_render_bridge_tests.cpp` pins minor `8.0F`, major `13.0F`, and one major count | Keep local unless map-maker adds explicit item state |
| `ViewportFraming.cpp` | `PlayerMarker` | camera anchor and perspective marker-size scaling | No | `product_viewport_framing_tests.cpp` pins anchor/framing/scaling behavior | Keep local |
| `FramePresenter.cpp::appendMapMakerCubePreviewUi(...)` | `MapMakerCubePreview` | Vulkan/UI overlay draw detail and inner color | No; consumes item color/size and adds local inner rect `{48,65,72}` | product Vulkan/viewport tests cover visibility/counts, not the inner color | Keep local |

### Answers To Card Questions

1. Remaining duplicated `ProductPrimitiveDrawKind` facts outside metadata are narrow: `OpeningMenuView.cpp::drawFocusIndicator(...)` still hardcodes the `PlayerFocusIndicator` primary color and fixed 36-pixel span, and `OpeningMenuView.cpp::drawRoomEditorCursor(...)` still hardcodes the `RoomEditorCursor` primary color. Other repeated-looking literals are dynamic variants or renderer decoration.
2. The only remaining primary/base presentation data that should move is the focus/cursor primary renderer handoff above.
3. Secondary/decorative renderer policy should stay local: tile insets/stripes/borders, prop inner/detail colors, door knob color, room-editor cursor outline, and map-maker cube inner rect.
4. Remaining switches over `ProductPrimitiveDrawKind` are dispatch:
   - `OpeningMenuView.cpp::drawPrimitiveItem(...)` is render shape dispatch.
   - `OpeningMenuView.cpp::drawRoomTile(...)` is room-tile shape/decorative dispatch.
   - `PrimitiveDrawList.cpp::updateCounts(...)` is draw-list count dispatch.
   - `RenderBridge.cpp` switches/if-chains are bridge count, target, and framed fallback dispatch.
5. `RenderBridge.cpp::countPhysicsDebugKind(...)` is still genuine counting dispatch. A metadata flag/table slice would mix bridge receipt counters into presentation metadata and make the explicit per-counter increments less clear.
6. `MapMakerGridDot` major/minor is still dynamic policy: draw-list construction uses `dot.major`, while RenderBridge fallback infers major from `markerSize > 10.0F`. Existing tests pin minor `8.0F`, major `13.0F`, and one major count. This should not become draw-kind metadata unless map-maker introduces explicit item state.
7. Existing tests pin base metadata rows, draw-list construction/counts, bridge physics/map-maker counts, and viewport marker scaling. They do not pin SDL pixel colors, so a renderer-primary handoff card should stay extremely narrow and behavior-preserving, or else wait for a renderer pixel harness.

### Stale Roadmap Assumptions

- `docs/complexity_audit_v0_1.md` finding #5 still says to replace the `RenderBridge` counting switch with lookups. Current E124/E127 evidence plus live `product_render_bridge_tests.cpp` show that is stale: this is bridge/count dispatch, not presentation metadata.
- The original roadmap says to have `OpeningMenuView` read shared color. Current state narrows that to two primary renderer duplicates (`PlayerFocusIndicator`, `RoomEditorCursor`). The room-tile colors in `drawRoomTile(...)` are secondary decoration, not base color drift.
- The original "base color / markerSize / layer" wording is partly stale. Current metadata has no layer field, and draw order remains append order.
- The confirmed `ElevatedFloorTile` base-color drift is fixed by E127. The remaining `{137,168,143}` literal is the renderer inset color and should not be silently treated as the base color.

### Recommendation

Safe narrow implementation card:

`E223 - OpeningMenuView Primitive Primary Metadata Handoff`

Scope:

- Edit only `src/app/iggy3d/view/OpeningMenuView.cpp` and the task card.
- Include `app/iggy3d/view/PrimitiveDrawMetadata.hpp` only if needed.
- In `drawFocusIndicator(...)`, use the framed/item primary color instead of hardcoded `{226,230,211}` and preserve the current fixed 36-pixel cross span. If the implementation uses metadata for size, use `markerSizeForProductPrimitiveDrawKind(ProductPrimitiveDrawKind::PlayerFocusIndicator)`, not perspective-scaled `framed.item.markerSize`, unless the owner explicitly wants a behavior change.
- In `drawRoomEditorCursor(...)`, use `item.color` for the primary cursor color and keep the outline `{32,42,44}` local.
- Do not touch `drawRoomTile(...)`, `drawDoorMarker(...)`, map-maker cube inner rect, `RenderBridge`, `PrimitiveDrawList::updateCounts(...)`, metadata shape, CMake, or tests.

Suggested verification:

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_primitive_draw_list_tests product_render_bridge_tests product_receipt_key_order_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_primitive_draw_list_tests|product_render_bridge_tests|product_receipt_key_order_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff -- tests/golden/product_receipt_key_order.golden
git -C /Users/kogaryu/iggy3d diff --check
```

Self-blockers:

- Stop if preserving the fixed focus-indicator size cannot be done without changing SDL output.
- Stop if the owner wants focus indicator perspective scaling; that is a behavior card, not metadata cleanup.
- Stop if the implementation starts expanding metadata to secondary colors or bridge count flags.

No broader metadata implementation card is recommended. `RenderBridge` counting, tile decoration, dynamic variants, and map-maker major/minor policy should stay local.

### Checks Run

- Required current-code grep:
  - `rg -n "ProductPrimitiveDrawKind|baseColorForProductPrimitiveDrawKind|markerSizeForProductPrimitiveDrawKind|ProductPrimitiveDrawKindMetadata|setColor\\(|markerSize|countPhysicsDebugKind|targetItem|MapMakerGridDot|MapMakerCubePreview" /Users/kogaryu/iggy3d/src/app/iggy3d/view /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp' --glob '*.hpp'`
- Required doc grep:
  - `rg -n "ProductPrimitiveDrawKind|ElevatedFloorTile|RenderBridge|PrimitiveDrawMetadata|metadata" /Users/kogaryu/iggy3d/docs/complexity_audit_v0_1.md /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/done/E124-primitive-draw-kind-metadata-audit.md /Users/kogaryu/iggy3d/docs/creative_mode/builder_tasks/done/E127-product-primitive-draw-kind-metadata-table.md`
- Additional focused greps for presenter/test coverage:
  - `rg -n "ProductPrimitiveDrawKind|MapMakerCubePreview|markerSize|setColor\\(" /Users/kogaryu/iggy3d/src/app/iggy3d --glob '*.cpp' --glob '*.hpp' | rg -v '/view/'`
  - `rg -n "drawPrimitiveItem|drawFocusIndicator|drawRoomEditorCursor|drawRoomTile|ProductPrimitiveDrawKind::" /Users/kogaryu/iggy3d/tests/unit --glob '*.cpp'`
- `git -C /Users/kogaryu/iggy3d diff --check` passed.
- Focused trailing-whitespace scan over this card returned no hits.
- No build or CTest was run because this card is read-only.

### Confirmation

- No source, test, CMake, receipt golden, fixture, or production-doc files were edited.
- No render output, draw ordering, draw-list counts, render-bridge counts, receipt fields/order/values, staging, commit, push, or window launch was performed.
