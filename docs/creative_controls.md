# Creative Interaction Contract

This is the control-design authority for the Creative application. It records
the Minecraft inputs we preserve, useful creator interactions proven by mods,
and the iggy3d action vocabulary. New input behavior must be recorded here
before or in the same change that implements it.

The implementation pipeline, placement policy, failure taxonomy, history
boundaries, and official Mojang source anchors live in
[`creative_placement_contract.md`](creative_placement_contract.md).

## Laws

1. Minecraft's movement and world-interaction grammar is reserved.
2. Bind semantic actions, never implementation details such as "increase maxY."
3. The crosshair supplies the target and the held tool supplies context.
4. A tool consumes a vanilla action only after it handles that action.
5. Every world action needs keyboard/mouse and controller semantics.
6. Do not require keys absent from compact keyboards. Page Up/Down may be
   optional nudges, never the only route to an operation.
7. Prefer aim, click, wheel, hotbar, and radial interaction over new chords.
8. Every mutation previews before commit and becomes one undo transaction.
9. Text, menus, viewport tools, and capture mode are separate input contexts.
10. Any deviation from Minecraft defaults needs an explicit reason in this file.

Statuses used below:

- **Adopt**: target behavior for iggy3d.
- **Adapt**: retain the interaction idea, not necessarily the physical key.
- **Reference**: useful evidence, not a committed control.
- **Reject**: deliberately excluded from the default scheme.

## Minecraft-Reserved Inputs

| Semantic action | Keyboard and mouse | Controller meaning | iggy3d status |
|---|---|---|---|
| Move | `WASD` | Left stick | Adopt |
| Look/target | Mouse | Right stick | Adopt |
| Ascend | `Space` | Jump/fly-up button | Adopt |
| Descend | `Shift` | Sneak/fly-down button | Adopt |
| Fast movement | `Ctrl` | Sprint input | Adopt |
| Primary/destroy | Left mouse | Right trigger | Adopt |
| Secondary/use/place | Right mouse | Left trigger | Adopt |
| Pick/sample | Middle mouse | Pick-block action | Adopt |
| Select hotbar slot | Wheel or `1`-`9` | Bumpers/slot selector | Adopt |
| Open catalog | `E` | Inventory button | Adopt |
| Pause/menu | `Escape` | Start/menu | Adopt |
| Drop | `Q` | Drop action | Reserve until needed |
| Swap hand | `F` | Swap action | Reserve until needed |
| Change view | `F5` | Perspective action | Reserve until implemented |
| Save toolbar | `C` + `1`-`9` | Saved-hotbar UI | Reference |
| Load toolbar | `X` + `1`-`9` | Saved-hotbar UI | Reference |

The Creative application may stay permanently in flight initially, but the
movement inputs keep their Minecraft meanings: `Space` up, `Shift` down, and
`Ctrl` fast. Double-tap flight toggling can wait for a grounded movement mode.

Sources: [Minecraft controls](https://www.minecraft.net/article/minecraft-controls),
[Java hotkeys](https://help.minecraft.net/hc/en-us/articles/360059148111).

## Creator Extension Record

### WorldEdit

| Creator pain | Proven interaction | Decision |
|---|---|---|
| Define a 3D region quickly | Hold a selection wand; left-click corner 1 and right-click corner 2 | Adopt |
| Preserve ordinary building controls | Cancel the vanilla click only when the held tool handles it | Adopt |
| Reuse the same motor grammar | Held items bind primary and secondary tool actions | Adopt |
| Repeat large edits safely | Session-level undo/redo records one edit session per operation | Adopt concept |

Evidence: WorldEdit `c9884dd09de0bf38f77555f1cad9f0f90174d00d`,
`SelectionWand`, `WorldEditListener`, and `PlatformManager` in
[EngineHub/WorldEdit](https://github.com/EngineHub/WorldEdit). WorldEdit is GPL;
use behavior as reference and do not copy implementation.

### Litematica and Forgematica

| Creator pain | Proven interaction | Decision |
|---|---|---|
| Select corners without leaving the world | Left mouse corner 1, right mouse corner 2, middle mouse select element | Adopt |
| Keep many related screens discoverable | `M` opens the main menu; `M+C`, `M+S`, `M+L`, `M+P`, and `M+V` open related screens | Adapt as one namespaced menu/radial family |
| Change operation without another toolbar | Hold `Ctrl` and scroll to cycle tool mode | Adapt through a visible radial/tool strip |
| Nudge a selection relative to the view | Hold `Alt` and scroll | Adapt; physical modifier remains undecided |
| Adjust layers | Page Up/Down | Reject as a required default; compact keyboards lack these keys |
| Avoid accidental edits | Tool input requires an active world, no GUI, tool enabled, and tool item held | Adopt |
| Keep previews non-destructive | Selection and schematic overlays remain separate from committed world state | Adopt |

Evidence: Litematica `26d7023f548360fb9a785767ddbef6aafafed60a`,
`Hotkeys`, `InputHandler`, `KeyCallbacks`, and `ToolMode` in
[sakura-ryoko/litematica](https://github.com/sakura-ryoko/litematica). Forgematica
uses the same interaction family. These projects are LGPL; reimplement concepts.

### Effortless Building

| Creator pain | Proven interaction | Decision |
|---|---|---|
| Too many build modes for dedicated keys | Hold `Left Alt` to expose a radial build-mode menu | Adapt as a semantic radial action |
| Correct a whole placement gesture | `Ctrl+Z` undo and `Ctrl+Y` redo | Adopt with platform command modifier |
| Build repeated or symmetric structures | Mirror, array, and radial-mirror settings generate a preview batch before placement | Adopt concept |
| Preserve ordinary placement | Build modes extend vanilla use/attack actions only in the in-game context | Adopt |

Evidence: Effortless Building `911d4b5605ef566586b80f35946270c337382663`,
`ClientProxy`, `RadialMenu`, and `BuildModes` in
[Requios/effortless-building](https://bitbucket.org/Requios/effortless-building).
Its repository and metadata disagree on the exact LGPL version; use concepts only.

### Axiom

Axiom is closed source. These are public behavior references, not architecture
or implementation claims.

| Creator pain | Public interaction | Decision |
|---|---|---|
| Access tools without replacing the nine-slot hotbar | Slot 10/`0` enters Builder Tools; hold `Left Alt` and scroll to change tool | Adapt; retain a visible tool lane |
| Select faces as well as corners | Left/right choose corners; middle-click expands/selects faces | Adopt |
| Move selections precisely | Wheel nudges relative to view; hold `X`, `Y`, or `Z` to constrain axis | Adapt |
| Transform without a modal dialog | `Ctrl+R` rotate and `Ctrl+F` flip, with visible preview | Adapt |
| Copy exact block state | `Ctrl+C` while targeting a block | Adapt through Pick/Sample metadata |
| Fast inventory search | `Enter` takes the first search result into the current hotbar slot | Adopt concept |
| Manipulate placements safely | `Ctrl+V` creates a preview; `Enter` confirms; Delete/Backspace cancels | Adopt concept |
| Make tools discoverable | Keybind window, tool palette, options panel, and visible key hints | Adopt concept |
| Scale tool radius | Hold `Ctrl+Middle Mouse` and adjust | Reference |

Sources: [Builder Mode](https://axiomdocs.moulberry.com/builder/intro.html),
[Move](https://axiomdocs.moulberry.com/builder/buildertools/move.html),
[Editor keybinds](https://axiomdocs.moulberry.com/editor/windows/keybinds.html),
and [Clipboard](https://axiomdocs.moulberry.com/editor/windows/clipboard.html).

### MiniHUD and Tweakeroo

| Creator pain | Proven interaction | Decision |
|---|---|---|
| Inspect spatial facts without mutating | MiniHUD uses `H` for overlays and `H+C` for configuration | Adapt as a read-only overlay family |
| Place oriented blocks accurately | Tweakeroo uses held modifiers for placement offset and rotation | Adopt semantic placement modifiers; do not reserve its exact keys |
| Restrict placement to face/line/plane/layer | Tweakeroo uses a namespaced `Z+number` family | Adapt through tool options/radial UI |
| Avoid default-key overload | Most specialized Tweakeroo operations ship unbound and configurable | Adopt principle |

Evidence: MiniHUD `f50ebe5715a6e4d4bb55d7aeb9afb9743bc7e9ea`
and Tweakeroo current source, `Configs`/`Hotkeys`, from
[maruohon/minihud](https://github.com/maruohon/minihud) and
[maruohon/tweakeroo](https://github.com/maruohon/tweakeroo). Both are LGPL;
reimplement concepts.

### Controlify

| Controller pain | Proven interaction | Decision |
|---|---|---|
| Mods assume keyboard keys | Register semantic action IDs with allowed contexts instead of spoofing keys | Adopt |
| Too many actions for available buttons | Expose eligible actions in a radial menu | Adopt |
| Preserve Minecraft muscle memory | Sticks move/look, triggers attack/use, bumpers cycle hotbar, face buttons handle jump/inventory | Adopt |
| Make hidden controls visible | Supply action glyphs and context-sensitive button guides | Adopt |
| Support hold, press, and analogue behavior | Bindings expose digital, just-pressed, just-tapped, and analogue outputs | Adopt concept |

Evidence: Controlify `f1bb11b190fef91a7dad4b89a293131f0bbb68d2`,
`ControlifyBindings`, `BindContext`, binding outputs, and
`controllers/default_bind/default.json` in
[isXander/Controlify](https://github.com/isXander/Controlify). Controlify is LGPL;
reimplement concepts.

## iggy3d Semantic Actions

Physical input is translated into these stable actions before a tool sees it:

| Action | World meaning |
|---|---|
| `Move` / `Look` | Navigate and aim |
| `Ascend` / `Descend` / `FastMove` | Creative flight |
| `Primary` | Destroy or active tool's primary operation |
| `Secondary` | Place/use or active tool's secondary operation |
| `Pick` | Sample target or active tool's tertiary operation |
| `HotbarSlot` / `HotbarNext` / `HotbarPrevious` | Select held material/tool |
| `ToggleCatalog` | Open or close searchable object and tool inventory |
| `ToggleToolWheel` | Open or close the radial creator-tool selector |
| `Confirm` / `Cancel` | Commit or abandon an explicit preview |
| `Undo` / `Redo` | Reverse or restore one committed gesture |

Initial tool grammar:

| Held tool | Primary | Secondary | Pick |
|---|---|---|---|
| Block/material | Remove targeted object | Place against targeted face | Sample object/material |
| Object select | Select/toggle targeted object | No action | Replace slot with sampled material |
| Object move | Begin/preview/commit ground-plane move | No action | Replace slot with sampled material |
| Selection wand | Set corner 1 | Set corner 2 | Expand selection to targeted cell |
| Fill/Hollow shape tool | Start or replace corner 1 | Set corner 2 and commit one history transaction | Sample operation material |
| Replace/Erase/Clone | No action | Apply the existing region as one history transaction | Sample operation material |
| Linear array | Select/toggle targeted object | Commit the previewed copies as one history transaction | Replace slot with sampled material |

Tool-specific operations such as fill, hollow, replace, clone, mirror, array,
rotate, and axis constraints belong in visible tool options. They do not each
earn a permanent global key.

## Current Implementation

- `WASD` moves, `Space` ascends, `Shift` descends, and `Ctrl` accelerates.
- Mouse and right stick look through the same frame input.
- Left/right/middle mouse and controller primary/secondary/pick map to semantic
  world actions with press, hold, and release state.
- Wheel, `1`-`9`, and controller bumpers select a nine-slot hotbar. Movement
  modifiers do not block slot selection or get consumed by it.
- Default slots are material, object select, object move, selection wand, fill,
  hollow, replace, erase, and clone.
- `E` or the controller inventory button opens the searchable material/tool
  catalog. Typing filters results; arrow keys, D-pad, and wheel move selection;
  a result click selects without equipping. `Enter`, controller confirm, or the
  visible Equip command equips the selected result into the active hotbar slot.
  While open, `1`-`9` assigns the selected result directly to that slot without
  closing the catalog.
- Fill and Hollow expose Shape, Axis, and Material beside the selected catalog
  row. Left/right or controller D-pad left/right cycles the bounded presets:
  Box, Line, Ellipsoid, and Cylinder X/Y/Z. The values are a draft until Equip;
  closing the catalog does not silently change the held tool.
- The catalog has Build and Actions pages. Brackets or controller L1/R1 change
  pages, and either tab can be clicked. Actions exposes Undo, Redo,
  Copy, Cut, Paste, Duplicate, Save, New, and Load through the same semantic
  command dispatcher used by keyboard input. Unavailable history, selection,
  and clipboard actions remain visible but dimmed and cannot emit a command.
  New and Load require a second confirm on the same selected row; moving the
  selection, changing pages, closing the catalog, or canceling clears the
  pending confirmation.
- `Escape`, the controller cancel button, or a second inventory press closes
  the catalog. Catalog input is modal and cannot mutate the world underneath it.
- `R` or controller D-pad right opens the eight-sector creator-tool wheel.
  Mouse direction or right stick selects a sector; arrow keys, D-pad up/down,
  and wheel cycle it; `Enter`, controller confirm, or click equips the tool into
  the active hotbar slot. `Escape`, controller cancel, or the toggle closes it.
  Array owns the eighth sector; Erase remains available from the default hotbar
  and searchable catalog instead of occupying a radial sector.
- While the wheel is open, right mouse or controller left trigger opens the
  highlighted tool's contextual options. Up/down or wheel selects a row;
  left/right changes its value; `Enter`/controller confirm applies the draft;
  `Escape`/controller cancel discards it. Mouse rows, `-`/`+`, Apply, and Cancel
  expose the same semantic actions.
- `R` is an explicit builder extension: it is outside Minecraft's reserved
  movement/world grammar and keeps `Left Alt` available for cursor capture.
  Catalog, tool-wheel, and tool-options contexts are isolated and block
  camera/document input during both opening and closing transitions.
- Contextual settings currently provide Free/X/Z movement, 15/45/90-degree
  rotation, 0.25/0.5/1/2-meter grid increments, Replace source filtering by
  material or Any, Clone offsets on X/Y/Z at 1/2/4/8 cells, and Array direction
  on either world axis with 1/2/4/8/16/32 copies at 1/2/4/8-cell spacing.
  Settings edit a non-document draft; Apply changes the editor configuration,
  while the next world operation remains one previewed history transaction.
- Fill and Hollow are self-contained held tools. Primary starts or replaces
  corner 1 at the crosshair; aiming updates the exact bounded preview; Secondary
  sets corner 2 and commits once. A second Secondary cannot recommit the finished
  region until another Primary starts a new gesture. Cylinders choose X/Y/Z
  extrusion. A one-cell cylinder is a disk and a one-cell-thick box is a wall
  plane. The selection wand remains available for Replace, Erase, Clone, and
  explicit region editing, but it is not a prerequisite for Fill or Hollow.
- The center ray resolves the nearest visible object, hit face, horizontal player
  facing, occupied grid cell, adjacent placement cell, and placement anchor.
- Directional materials use descriptor-owned orientation: side placement faces
  outward from the clicked surface, while floor/ceiling placement turns the
  object's front toward the player. No extra rotate key is required for normal
  placement.
- Material, select, move, and volume tools use the held-tool grammar above.
- A material's thin green wireframe is only a placement preview. Right mouse or
  controller left trigger attempts the placement. A successful create receipt
  turns the crosshair brackets green and draws a thick lime outline from the
  new live document object's bounds; red brackets mean the input arrived but
  no object was created. An identical object at the exact planned transform,
  bounds, and path is treated as occupied: its preview is red and repeated taps
  do not add another document object or undo record.
- Array previews copies of the current ordered selection without mutating the
  document. Right mouse/controller Secondary or `Enter` commits the full batch
  atomically as one history transaction, preserves the originals, and selects
  only the final generated copy so repeated commits do not grow exponentially.
- Platform-command `C`, `X`, and `V` copy, cut, and begin clipboard placement.
  Paste aligns the copied selection's deterministic lower-center placement
  anchor to the crosshair cell and displays a mint wireframe without mutating the
  document. Right mouse/controller Secondary, `Enter`, or controller confirm
  commits one atomic paste and one undo step. `Escape`, Delete, Backspace, or
  controller cancel abandons the preview. Camera movement remains available
  while all unrelated document commands and hotbar changes are isolated.
- The crosshair, target cell, hotbar selection, placement ghost, placement
  receipt feedback, and volume outline provide visible state. Above the hotbar,
  the held-tool label names operation, shape, cylinder axis, material, and
  whether corner 1 or a completed region is active. Mutations use the existing
  history transactions.
- Interactive Fill/Hollow previews fail closed above 512 candidate/generated
  cells. The red outline appears before commit so an accidental distant second
  corner cannot create thousands of per-cell objects and stall scene rebaking.
- Save, new, and load use platform command `S`, `N`, and `O`. No `F`, `F5`, or
  Page Up/Down binding is required.

## Remaining Gaps

- Pause/menu UI is not implemented.
- Mirror, radial array, and placement restrictions remain future previewable tools or
  contextual settings. They do not receive permanent global keys.
- Controller glyph hints, remapping, and accessibility settings remain future
  work; the current controller map is fixed.
- A real-controller and live-window interaction pass remains required. Headless
  tests pin routing, hotbar, target-grid, and binding behavior without launching
  the application.

## Algorithm Validation Queue

- Randomized parity between indexed center-ray picking and the retained
  brute-force picker before raising scene-size targets.
- Randomized rotated/non-uniform object face-normal parity, including edge and
  corner ties and rays that start inside a visual bound.
- Large-volume latency and preview-density thresholds near the 16,384-object
  operation limit.
- Randomized shape-brush symmetry and endpoint-reversal parity, especially even
  extents, diagonal 3D lines, thin ellipsoids, cylinder-axis permutations, and
  selections immediately above both planner limits.
- Linear-array commit latency and preview density at the hard 512-generated-
  object limit (up to 6,144 transient box edges), including multi-object parent
  remapping and rollback on a late staged-paste failure.
- Clipboard preview latency at 512 detailed objects and above; larger
  clipboards intentionally collapse to one aggregate extent box while commit
  still applies the complete clipboard atomically.
- Path-point/line sub-element targeting under the center-ray grammar.
- Controller deadzone, trigger threshold, disconnect, and reconnect tests on
  real hardware.

## Input Change Checklist

Any implementation that adds or changes an input must include:

1. A semantic action and owning context.
2. Keyboard/mouse and controller mappings, or a recorded controller blocker.
3. Consume behavior: pass through, consume on handle, or modal capture.
4. Visible feedback or a button/key hint.
5. Binding-conflict and context-routing tests.
6. One manual interaction check in addition to headless logic tests.
7. An update to this document's adopted control or extension record.
