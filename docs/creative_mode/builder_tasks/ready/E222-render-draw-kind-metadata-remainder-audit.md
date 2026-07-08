# E222 - Render Draw-Kind Metadata Remainder Audit

## Status

Ready.

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
