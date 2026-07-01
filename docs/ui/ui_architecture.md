# iggy3d UI Architecture — Widget Primitives

> The durable design for how UI is built in iggy3d. **The reframe**: UI is not a
> collection of *screens*, it is a small collection of reusable *bricks*. You
> invent the bricks once (doors, windows, walls); every screen — this game's, the
> next game's — is assembled from them. This doc locks the brick set, grounds it
> in the engine's real draw-list code, and names the two engine gaps that must be
> filled. Written 2026-07-01. Companion: the L0 draw-list lives in
> `src/app/iggy3d/menu/DrawList.hpp`.

## The one leverage question

> *"What do I need to invent so every future UI becomes easy?"*

The answer is **~14 widget primitives + a theme system + ~10 composed components + a
few data catalogs.** Nail those and every interface across the thief game, the
knight/priest game, and future projects is assembled, not authored. The screens
change; the bricks don't. That is where the leverage is.

## The stack

```
L4  Screens        starter, save, pause, camp, notebook, HUD  ← assembled last, change freely, per game
L3  Components      Slot, Card, Page, BarGroup, Toast, …       ← data compositions of L1 (collapses the taxonomy)
L2  Theme / skin    tone tokens → colors + type + metrics      ← the "worn-leather" material system, as DATA
L1  Widgets         Panel, Text, Button, List, Viewport …      ← THE 14 BRICKS. game-agnostic. the invention.
L0  Draw primitives ProductUiDrawList / ProductUiPrimitive     ← EXISTS. what the renderer eats + receipts capture
```

Each layer only knows the one below it. A widget knows nothing of a screen; a screen
knows nothing of Vulkan. That is the whole discipline.

---

## L0 — the render primitive (EXISTS)

The bottom of the stack is already built and already receipt-tested. This is the
foundation the bricks emit into — we do not reinvent it, we build on it.

**The primitive** (`src/app/iggy3d/menu/DrawList.hpp:16-75`):

```cpp
enum class ProductUiPrimitiveKind { Panel, Rect, Text, Border, Highlight };

enum class ProductUiTone { Surface, SurfaceRaised, TextPrimary, TextMuted,
                           Accent, Selected, Disabled, Border, Status };

struct ProductUiPrimitive {
  ProductUiPrimitiveKind kind;
  ProductUiTone          tone;       // semantic color ROLE, not raw RGB
  ProductUiRect          rect;       // x, y, width, height (float, virtual px)
  std::string            semanticId; // stable id — tests find primitives by this
  std::string            text;       // Text primitives
  FrontendAction         action;     // interaction this primitive triggers
  bool                   selected;
  bool                   enabled;
};

struct ProductUiDrawList {           // 1280×720 virtual canvas
  std::vector<ProductUiPrimitive> primitives;
  std::uint64_t primitiveCount, textCount, rectCount, rowCount, disabledRowCount;
  // …status/reason/selectedAction
};
```

Built by a single pure function: `buildProductStarterUiDrawList(request) → ProductUiDrawList`
(`DrawList.hpp:101`). Request in, primitives out. **The pure-function model we want
is already the house style** — it just isn't componentized yet.

**The render path** (confirmed by scout, Vulkan backend):
`ProductUiDrawList` → `buildProductVulkanStarterMenuFrame` (`window/FramePresenter.cpp:840-878`)
→ `ProductVulkanMenuFrame {rects, textGlyphQuads}` → `RenderUiFrame` → `OverlayRect`
→ **`vkCmdClearAttachments`** (`CommandRecording.cpp:89-106`). Text →
`layoutDebugHudTextAt()` glyph quads. Vector order = draw order (no z field).

**What L0 gives us for free:** solid-color rectangles (Panel/Rect/Border/Highlight),
glyph text (Text), a semantic tone system, and — already on every primitive — a
`semanticId`, an `action`, and `selected`/`enabled`. Interaction and identity are
*already* first-class. That's more than most engines start with.

### Two things L0 does NOT have — the real engine gaps

Both confirmed by direct scout of the render path. These are not widget concerns;
they are **new L0 capabilities** the widget set forces us to add.

1. **No clipping / scissor at the draw-list level.** The Vulkan scissor is set to
   the full viewport once per frame and never modified; no primitive carries a clip
   rect. `ScrollView` (Tier A) and `Viewport` (Tier B) both need to bound children to
   a rectangle. **This must be added.** Proposed model below.

2. **No textured-quad path.** UI rects are `vkCmdClearAttachments` solid-color
   *clears*; there is no sampler/texture draw for UI. `Image`, `Icon`, and `Viewport`
   all need to sample a texture. **This is a new UI pipeline**, bigger than clipping,
   and is why those three are Tier B / built last.

### The two draw lists (don't confuse them)

- `menu/DrawList` — `ProductUiDrawList` / `ProductUiPrimitive` — **2D screen-space UI.
  This is our L0.**
- `view/PrimitiveDrawList` — `ProductPrimitiveDrawList` / `ProductPrimitiveDrawItem`
  (21 kinds: PlayerMarker, FloorTile, RoomEditorCursor, PhysicsAabbDebug, …) —
  3D-scene-projected markers/tiles/editor/physics-debug. **Not ours. Leave it alone.**

### The per-backend duplication (extra leverage)

Vulkan renders menus from `ProductUiDrawList` primitives. The SDL path
(`view/OpeningMenuView.cpp::drawLoadSavePanel`, ~lines 1100-1161) hand-draws the same
screens a *second* time with direct `setColor`/`fillRect`/`drawText`. Adding a screen
today means coding it twice, once per backend. The widget layer collapses per-screen
layout **and** this per-backend duplication: widgets emit `ProductUiDrawList` once;
each backend consumes the primitive list. Migrating the SDL path onto primitives is a
follow-on win, not a prerequisite.

---

## L1 — the 14 widget bricks (the invention)

Locked v1 set. Game-agnostic. Everything else is these, restyled or composed.

```
Panel   Stack   Grid    ScrollView     ← containers / layout
Text    Image   Icon    Viewport       ← content
Button  Toggle  Slider  ProgressBar    ← interactive / indicator
List    Popup                          ← collection / overlay
```

**The pure-function decision.** A widget is **not a stateful object**. It is a pure
function of `(props, theme, layout context)` that appends to a `WidgetOutput`:

```cpp
struct WidgetOutput {
  std::vector<ProductUiPrimitive> primitives;   // renderer eats this  (→ ProductUiDrawList)
  std::vector<UiHitRegion>        hitRegions;    // input eats this     (see "lanes" below)
};
```

This matches the engine's determinism: the same state deterministically produces the
same draw list, so **every widget is receipt-testable** exactly like the rest of the
engine. Retained/stateful widget trees would fight that — we don't build them.

**Emit visuals AND hit regions — the split.** L0 currently welds an `action` onto the
draw primitive. Widgets instead emit a *parallel* `UiHitRegion` list so draw and
input stay in separate lanes (see "Cross-cutting systems"). Layout is computed
**once** and feeds both — killing the draw/hit-test duplication that exists today
(the save browser computes `slotY = 318; slotY += 38` in *both* its draw loop and its
hit-test at `OpeningMenuView.cpp:1350`; they drift the moment one is edited).

### Two build tiers (forced by the L0 gaps)

**Tier A — buildable on today's rect+text L0 (+ clipping for ScrollView):**
`Panel · Stack · Grid · ScrollView · Text · Button · Toggle · Slider · ProgressBar · List · Popup`

**Tier B — need the new textured-quad pipeline:** `Image · Icon · Viewport`

| Brick | Props (sketch) | Emits | Notes |
|---|---|---|---|
| `Panel` | rect, tone, border?, padding | Panel/Border | the container; "border styles / frames / backgrounds" are Panel *skin*, not widgets |
| `Stack` | dir(row/col), gap, children | (layout only) | row/column layout; no pixels of its own |
| `Grid` | cols, rows, cell, gap | (layout only) | fixed-cell layout |
| `ScrollView` | rect, contentH, offset | ClipBegin/…/ClipEnd | **needs clipping** |
| `Text` | text, tone, scale | Text | glyph quads via existing path |
| `Button` | rect, label, action, enabled | Panel+Text (+Highlight) + **hit region** | the interactive atom |
| `Toggle` | on, label, action | Panel+Text/Highlight + hit region | absorbs checkbox/radio as variants |
| `Slider` | value, min, max, action | Panel+Rect + hit region | |
| `ProgressBar` | value 0..1, tone | Panel+Rect | absorbs all meters (health/mana/stamina/detection/noise) |
| `List` | items, selectedIndex, action | rows of Panel+Text + hit regions | selectable rows |
| `Popup` | rect, tone, children | Panel + children (overlay order) | overlay; foundation for Tooltip/Toast/Dropdown |
| `Image` | rect, textureId | **TexturedQuad** | Tier B — needs textured pipeline |
| `Icon` | rect, iconId | **TexturedQuad** | Tier B — Image + icon catalog lookup |
| `Viewport` | see below | ClipBegin/Texture/ClipEnd + hit region | Tier B — live render target |

### Viewport — the first-class brick (narrow by design)

`Image` is static texture display (icon, sketch PNG, portrait, panel art). `Viewport`
is a **live or semi-live render target** (sketch canvas, map table, minimap,
inspection window, scrying panel) — it needs clipping, zoom/pan, coordinate
conversion, input capture, overlays, possibly its own render pass. Calling that
"Image" is how you create a monster wearing a nametag. Kept narrow:

```cpp
struct Viewport {
  UiId          id;
  Rect          rect;
  RenderTargetId target;
  ViewportMode  mode;         // Static, PanZoom, Sketch, Map
  bool          acceptsInput;
  bool          drawBorder;
};
// emits:  ClipBegin(rect) · Texture(rect, targetTexture) · ClipEnd()
// + hit:  UiHitRegion{ id, rect, kind=Viewport, inputMode=PanZoom|Draw|Inspect }
```

---

## L2 — theme / skin (generalize the tone system)

`ProductUiTone` is the seed of this layer — semantic color *roles* resolved by
`productUiToneColor(tone)`. Today it's a hardcoded enum→color switch. L2 **generalizes
it into data**: a `Theme` mapping each tone slot to a color, plus type roles (sizes)
and metrics (padding, row height, border width). One theme per game/skin; swap the
theme and the worn-leather thief look becomes the knight/priest look with zero widget
changes. **Widgets reference tones, never raw RGB** — already true at L0; keep it.

---

## L3 — components (the collapse)

Components are data compositions of L1. This is where the ~100-item UI taxonomy
collapses into a handful of things × states × data:

```
Slot      = Panel + Icon + Text/overlays        ← one Slot × state enum = empty/occupied/selected/disabled/broken/equipped
Card      = Panel + Text + Icon/Image           ← one Card × data = workshop/chapel/cache/board/map-table/storage/portrait
Page      = Panel + Text/Image/Viewport body    ← one Page × body-mode = blank/lined/sketch/bestiary/contract/spell/blueprint
BarGroup  = ProgressBar × N                      ← health/mana/stamina from one bar + theme + value
Toast     = Popup + Card                          ← notification/warning/confirm/ribbon as variants
Tooltip   = Popup + Text/Card                     ← hover trigger; NOT an L1 widget
TabBar    = Stack + Toggle group                  ← NOT an L1 widget
Dropdown  = Button + Popup + List                 ← NOT an L1 widget
Hotbar    = Grid + Slot
Notebook  = Page + TabBar + ScrollView
```

The demotions are load-bearing: `Tooltip`, `TabBar`, `Dropdown`, `Checkbox`, `Radio`
are **compositions, not primitives.** Keeping them out of L1 is what stops the brick
set from rotting into a widget zoo.

## L4 — screens

Screens assemble components. The current bespoke emitters (starter, save browser,
pause, settings, world-setup) become compositions. Adding a screen becomes
configuration, not new UI technology — and it renders on every backend at once.

---

## Cross-cutting systems (NOT inside widgets — the lanes)

Widgets emit **visuals + hit regions only.** Everything else is its own lane, so no
system reaches into another:

```
Theme tokens          data: tone → color/type/metrics
Icon catalog          data: iconId → texture
Cursor catalog        data: cursorId → texture/hotspot
Hit regions           widgets emit → input consumes
Focus / navigation    keyboard/gamepad traversal over hit regions
Action queue          input produces → runtime consumes (FrontendAction)
Sound event queue      widgets/interactions request → audio consumes
Draw-list receipts     the test/verification lane
```

Renderer consumes primitives. Runtime consumes actions. Audio consumes sound
requests. Input consumes hit regions. Everyone stays in their lane.

---

## The two engine gaps — how we fill them

**1. Clipping.** Proposed as a **CPU-resolved clip stack at L0→L1 build time**, not a
per-frame GPU scissor dance. `ScrollView`/`Viewport` push a clip rect; the frame
builder (`buildProductVulkanStarterMenuFrame`) intersects each primitive's rect with
the active clip, drops fully-clipped primitives, clamps partial ones. This fits the
clear-attachment model perfectly — `vkCmdClearAttachments` already takes a
`VkClearRect`, so clipping a rect is a cheap CPU rectangle intersection. Expose it as
`ClipBegin(rect)` / `ClipEnd()` (a stack) rather than a per-primitive field.
*Needed by ScrollView (Tier A) — so this lands before ScrollView.*

**2. Textured quads.** A new `ProductUiPrimitiveKind::TexturedQuad` (rect + textureId
+ uv), a sampler path in the Vulkan frame builder, and an SDL equivalent. This is the
bigger lift and gates `Image`/`Icon`/`Viewport` — deferred behind all of Tier A.

---

## Receipt-testability (the engine fit / the moat)

A widget tree deterministically emits a `ProductUiDrawList`. Serialize it to canonical
`key=value` text → a golden receipt. This extends the engine's determinism +
receipt discipline to the entire UI: a widget's output is as verifiable as a physics
step. Proposed helper: `serializeProductUiDrawList(list) → std::string`.

`product_ui_draw_list_tests` already asserts the shape we must preserve — for the
starter/save draw list: `rectCount==11`, `textCount==11`, `primitiveCount ==
primitives.size()`, and per-`semanticId` field values (e.g. `header.text=="IGGY3D"`,
highlight `rect.x==50, rect.y==194`). **A widget rebuild is proven correct when these
stay byte-identical.**

---

## Implementation order

1. **This doc.** ✅
2. **`Theme`** — generalize `ProductUiTone`→color into a data struct (+ type roles, metrics).
3. **`WidgetOutput` + layout context + the pure-emit contract** — the spine.
4. **`Panel`, `Text`, `Stack`** — the irreducible three.
5. **`Button`** (+ hit regions) — the first interactive atom.
6. **`ProgressBar`** — the first indicator.
7. **PROOF: rebuild the starter/save `ProductUiDrawList` from L1 widgets** — keep
   `product_ui_draw_list_tests` byte-identical (`rectCount==11`, `textCount==11`,
   coords unchanged). This single output-preserving rebuild de-risks the whole
   migration. *(Same discipline as the ProductAppWindowState decoupling cuts.)*
8. **`Grid`, `List`.**
9. **Clipping at L0** (clip stack) → **`ScrollView`.**
10. **`Popup`.**
11. **Textured-quad L0 pipeline** → **`Image`, `Icon`.**
12. **`Viewport`.**

## First target — the proof

The save browser exists in the **primitive** path already (`buildProductStarterUiDrawList`
consumes `saves`/`selectedSaveId`; `product_ui_draw_list_tests` covers its rows —
"disabledSaveRowsAreRepresentedWithoutCompatibleSaves",
"selectedNewWorldGetsHighlightAndStableCoordinates"). Rebuild *that* emission from L1
widgets, hold the receipt identical. Then the SDL duplicate
(`OpeningMenuView::drawLoadSavePanel`) becomes deletable by pointing the SDL backend
at the same primitives.

## Discipline (unchanged from the rest of the engine)

Build + full suite gate every change. Output-preserving rebuilds proven by
byte-identical draw-list receipts. Small commits, `claude:` prefix. `TextInput` stays
out of v1 (caret/selection/clipboard/IME/focus is a basement of snakes); add it only
when a real screen needs typing.
