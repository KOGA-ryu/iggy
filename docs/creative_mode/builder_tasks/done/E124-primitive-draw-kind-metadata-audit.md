# E124: ProductPrimitiveDrawKind Metadata Audit

## Objective

**Read-only, plus one optional guard-test.** Confirm the per-kind
color/size/layer drift across the render draw-kind switches, produce the exact
per-kind diff, and specify the `constexpr` metadata table that will replace them.
Then draft the implementation card. No behavior change in this card (the guard
test, if added, only pins current values).

## Why This Exists

From `docs/complexity_audit_v0_1.md` (finding #5 / bucket 4). `ProductPrimitiveDrawKind`
(~22 values) has its per-kind presentation data (color / marker size / layer)
exhaustively switched in **three** render files, and the copies have **drifted**:

- `view/PrimitiveDrawList.cpp`, `view/OpeningMenuView.cpp`, `view/RenderBridge.cpp`.
- Confirmed drift: `ElevatedFloorTile` color is `{92,126,102}` in one site and
  `{137,168,143}` in another.

Adding a draw kind today means editing 3 files and risks another silent drift.

## Required Work

1. **Enumerate every switch over `ProductPrimitiveDrawKind`** across the three
   files (and any others — verify with a grep). For each site, record what
   per-kind data it encodes: base color, marker/point size, layer/z-order,
   counting, or genuine render dispatch.
2. **Produce the drift table.** For all ~22 kinds, list the value each site uses
   for each attribute and mark every disagreement (start from the confirmed
   `ElevatedFloorTile` color). This is the core deliverable.
3. **Classify each switch:** data-carrying (→ becomes a table lookup) vs genuine
   render-dispatch/control-flow (→ stays a switch). Do not table-ify real
   dispatch.
4. **Specify the table.** A `constexpr` metadata array/table keyed by
   `ProductPrimitiveDrawKind` (base color / markerSize / layer), where it lives,
   and how each data-carrying site becomes a lookup. Model on the credited
   `ProductCreativeUiCommandKind` metadata pattern.
5. **(Optional, recommended) add one guard-test** that pins the current per-kind
   color/size/layer for all kinds at their *primary* site, so the implementation
   card can prove behavior-preservation and the drift resolution is explicit.
   This test must only pin existing values — it changes no render behavior.
6. **Draft the implementation card** (`E12x`) with the exact edits AND the
   resolved value for each drifted kind. **Do not pick the "correct" drifted
   value yourself** — flag each drift for the render owner to resolve; the
   implementation card records their decision.

## Acceptance Notes

Deliverable: the per-kind drift table + switch classification + the table shape
spec + the drafted implementation card. If the guard-test is added, it is green
and pins current values only. No render behavior changes.

## Do Not

- Do not change any render output or resolve a drift value yourself (flag it).
- Do not table-ify genuine render-dispatch switches.
- Do not create the metadata table in this card — only specify it.
- Do not stage, commit, or push.

## Suggested Verification

```sh
grep -rn 'ProductPrimitiveDrawKind' --include='*.cpp' /Users/kogaryu/iggy3d/src/app/iggy3d/view
# if a guard-test is added:
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_primitive_draw_list_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^product_primitive_draw_list_tests$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

## Completion Brief

Append:

- Switch sites (file → attributes encoded → dispatch-or-data):
- Per-kind drift table (all disagreements):
- Proposed metadata table shape + home:
- Guard-test added? (target + what it pins):
- Drafted implementation card + flagged drift decisions:
- Concerns/deferred:

## Completion Brief - E124

### Switch Sites

- `src/app/iggy3d/view/PrimitiveDrawList.cpp:61`:
  `floorKindForSurface(...)` maps traversal tags to floor draw kinds. This is
  semantic dispatch, not presentation metadata.
- `src/app/iggy3d/view/PrimitiveDrawList.cpp:74`:
  `colorForRoomKind(...)` is data-carrying base color metadata for room/editor
  kinds, with a fallback for other draw kinds. This should become a table
  lookup or disappear behind table helpers.
- `src/app/iggy3d/view/PrimitiveDrawList.cpp:163`:
  room/wall/prop/door item construction sets base color and marker size. This
  is data-carrying, with dynamic variants for ledge/reset-zone props and
  open/closed doors.
- `src/app/iggy3d/view/PrimitiveDrawList.cpp:232`:
  `itemFromSceneItem(...)` maps `SceneItemKind` to primitive kind and then sets
  color/size. The scene-kind mapping is dispatch; the color/size assignments
  should be table-backed defaults.
- `src/app/iggy3d/view/PrimitiveDrawList.cpp:299`:
  `updateCounts(...)` is receipt/count classification. It is not presentation
  metadata; keep it separate unless a later count-flag catalog is explicitly
  designed.
- `src/app/iggy3d/view/PrimitiveDrawList.cpp:511`:
  editor/map-maker/physics overlay construction sets color/size. Base defaults
  should be table-backed where static; dynamic variants remain local policy.
- `src/app/iggy3d/view/PrimitiveDrawList.cpp:696`:
  debug-projection dispatch routes debug payload kinds to append helpers. This
  is dispatch, not presentation metadata.
- `src/app/iggy3d/view/OpeningMenuView.cpp:216`:
  marker and physics debug draw helpers mostly consume `item.color` and
  `item.markerSize`; the hardcoded line widths/insets are render-shape policy.
- `src/app/iggy3d/view/OpeningMenuView.cpp:274`:
  `drawFocusIndicator(...)` hardcodes the same color/effective size as
  `PlayerFocusIndicator` construction. This is duplicated presentation data
  plus render-shape policy.
- `src/app/iggy3d/view/OpeningMenuView.cpp:282`:
  `drawRoomEditorCursor(...)` hardcodes the same primary color as construction
  and adds a render-local outline color.
- `src/app/iggy3d/view/OpeningMenuView.cpp:300`:
  `drawDoorMarker(...)` consumes item color/size but owns the knob/detail color.
- `src/app/iggy3d/view/OpeningMenuView.cpp:320`:
  `drawRoomTile(...)` consumes base item color/size and hardcodes decorative
  per-kind overlays for elevated/ramp/blocked/wall/prop. This is genuine render
  dispatch plus drifted decorative color data.
- `src/app/iggy3d/view/OpeningMenuView.cpp:653`:
  `drawPrimitiveItem(...)` is genuine render dispatch and should stay a switch.
- `src/app/iggy3d/view/OpeningMenuView.cpp:740`:
  top-down map anchor/marker scaling uses `PlayerMarker` and scales marker
  size. This is viewport policy, not base metadata.
- `src/app/iggy3d/view/RenderBridge.cpp:8`:
  target classification and physics/debug/counting logic are receipt/bridge
  dispatch, not presentation metadata.
- `src/app/iggy3d/view/ViewportFraming.cpp:16`:
  player anchor and perspective marker-size scaling are viewport policy.
- `src/app/iggy3d/window/FramePresenter.cpp:657`:
  `MapMakerCubePreview` UI overlay consumes item color/size and adds a
  render-local inner rectangle color.

### Per-Kind Drift Table

Current code has no per-kind layer/z-order field; draw order is append/vector
order.

| Kind | Primary construction color / size | Other presentation values found | Classification |
| --- | --- | --- | --- |
| `PlayerMarker` | `{80,170,236}` / `26` | Top-down uses it as camera anchor; no color drift. | Data + anchor dispatch |
| `NpcMarker` | `{210,78,76}` / `28` | No drift. | Data |
| `PickupMarker` | `{229,196,72}` / `20` | No drift. | Data |
| `InteractableMarker` | `{198,142,222}` / `22` | No drift. | Data |
| `ObjectiveMarker` | `{126,201,176}` / `18` | No drift. | Data |
| `TacticalMarker` | `{126,201,176}` / `18` | No drift. | Data |
| `DebugMarker` | `{112,118,120}` / `14` | Same as fallback. | Data |
| `PlayerFocusIndicator` | `{226,230,211}` / `36` | Renderer hardcodes `{226,230,211}` and `36`-wide cross. | Duplicate, no value drift |
| `DoorMarker` | Closed `{220,178,86}` / `24`; open `{126,201,176}` / `18` | Renderer consumes item color/size and adds knob `{94,74,54}`. | Dynamic data + render detail |
| `FloorTile` | `{54,78,68}` / `58` | No render overlay. | Data |
| `ElevatedFloorTile` | `{92,126,102}` / `58` | Renderer inset `{137,168,143}`. | Drifted decorative color |
| `RampTile` | `{82,139,156}` / `58` | Renderer stripe `{183,213,210}`. | Drifted decorative color |
| `BlockedSlopeTile` | `{184,82,74}` / `58` | Renderer stripe `{246,184,130}`. | Drifted decorative color |
| `WallTile` | `{76,86,92}` / `62` | Renderer border `{116,128,132}`. | Drifted decorative color |
| `PropTile` | `{151,102,58}` / `42` | Ledge `{76,132,178}` / `52`; reset-zone `{214,74,92}` / `34`; renderer inner/detail `{198,142,82}` and `{88,58,34}`. | Dynamic variants + drifted decoration |
| `RoomEditorCursor` | `{245,214,96}` / `30` | Renderer hardcodes `{245,214,96}` plus outline `{32,42,44}`. | Duplicate primary + render detail |
| `RoomEditorPlacementPreview` | `{105,205,228}` / default `52` | Wall `58`; object `46`; renderer consumes item color/size. | Dynamic size policy |
| `PhysicsAabbDebug` | Solid `{105,205,228}` / `34`; sensor `{245,214,96}` / `28` | Renderer consumes item color/size. | Dynamic debug policy |
| `PhysicsContactNormalDebug` | Solid `{236,118,86}` / `18`; sensor `{245,214,96}` / `18` | Renderer consumes item color/size. | Dynamic debug policy |
| `PhysicsBroadphasePairDebug` | Solid `{166,184,177}` / `14`; sensor `{245,214,96}` / `14` | Renderer consumes item color/size. | Dynamic debug policy |
| `MapMakerGridDot` | Major `{136,184,226}` / `13`; minor `{86,130,172}` / `8` | Main primitive renderer skips it; bridge infers major from `markerSize > 10`. | Dynamic map-maker policy |
| `MapMakerCubePreview` | `{126,221,186}` / `54` | Frame presenter adds inner rect `{48,65,72}`. | Data + render detail |

### Proposed Metadata Table Shape And Home

- Home: add a small view catalog such as
  `src/app/iggy3d/view/PrimitiveDrawMetadata.hpp`, modeled on
  `src/app/iggy3d/creative/bridge/UiCommandCatalog.hpp`.
- Suggested row:

  ```cpp
  struct ProductPrimitiveDrawKindMetadata {
    ProductPrimitiveDrawKind kind = ProductPrimitiveDrawKind::DebugMarker;
    ProductPrimitiveColor baseColor{};
    float markerSize = 14.0F;
  };
  ```

- Suggested helpers:
  `productPrimitiveDrawKindMetadataCatalog()`,
  `findProductPrimitiveDrawKindMetadata(...)`,
  `baseColorForProductPrimitiveDrawKind(...)`, and
  `markerSizeForProductPrimitiveDrawKind(...)`.
- Do not add behavior-owning layer/z-order in the first implementation. There
  is no existing per-kind layer field; ordering currently comes from item append
  order.
- Keep dynamic variants local unless the owner explicitly broadens the table:
  door open/closed, prop ledge/reset-zone, placement preview tool size, physics
  sensor styles, and map-maker grid major/minor.

### Guard Test

No guard test was added in this audit slice. The drifted decorative colors need
render-owner decisions before a single metadata table test can honestly pin
centralized intended values.

### Drafted Implementation Card

Drafted follow-up card:
`docs/creative_mode/builder_tasks/ready/E127-product-primitive-draw-kind-metadata-table.md`.

Flagged drift decisions in that card:

- Focus/cursor duplicate primary colors should likely route through metadata.
- Elevated/ramp/blocked/wall/prop decorative overlay colors should either stay
  render-local or be explicitly added as secondary metadata.
- Door knob and map-maker cube inner rect should stay render-local unless the
  renderer owner wants secondary/detail colors in the table.

### Concerns / Deferred

- A metadata table should centralize base color/size only. Trying to absorb
  render-shape decoration in the same pass would mix presentation defaults with
  draw-function behavior.
- `RenderBridge` and `updateCounts` still contain kind switches, but those are
  diagnostics/counting dispatch rather than the color/size drift targeted by
  this card.
