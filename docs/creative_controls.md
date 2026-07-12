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
11. Interactive panels use the shared bounded widget frame. Screens own durable
    state and interpret semantic widget-event receipts; widgets never own editor
    state or application callbacks.

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
| Ascend | `Space` | `R2` | Adapt |
| Descend | `Shift` | `L2` | Adapt |
| Fast movement | `Ctrl` | Sprint input | Adopt |
| Primary/destroy | Left mouse | Circle in the world | Adapt |
| Secondary/use/place | Right mouse | X in the world | Adapt |
| Pick/sample | Middle mouse | Square | Adapt |
| Select hotbar slot | Wheel or `1`-`9` | Bumpers/slot selector | Adopt |
| Open catalog | `E` | Inventory button | Adopt |
| Pause/menu | `Escape` | Start/menu | Adopt |
| Drop | `Q` | Drop action | Reserve until needed |
| Swap hand | `F` | Swap action | Reserve until needed |
| Change view | `F5` | Perspective action | Reserve until implemented |
| Save toolbar | `C` + `1`-`9` | Saved-hotbar UI | Reference |
| Load toolbar | `X` + `1`-`9` | Saved-hotbar UI | Reference |

The Creative application may stay permanently in flight initially. Keyboard
keeps `Space` up, `Shift` down, and `Ctrl` fast. PS5 deliberately uses `R2` to
raise and `L2` to lower so X and Circle can preserve one decision language
across the editor: X accepts/adds/advances, while Circle rejects/removes/backs
out. Tool and aim state persist between taps, so using a face button does not
discard the target or selected operation. This is an explicit controller
departure from Minecraft's trigger placement grammar.

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
| `CatalogAssignToolWheel` | Assign the highlighted catalog tool to a wheel sector |
| `ToggleToolWheel` | Open or close the radial creator-tool selector |
| `ToggleTransformControls` | Open or close controls for the active selection preview |
| `TransformControlPrevious` / `TransformControlNext` | Select a visible transform operation |
| `TransformConstraintX/Y/Z` | Toggle an axis lock for the active transform preview |
| `TransformNudgeNegative/Positive` | Move the preview one contextual snap step on its locked axis |
| `Confirm` / `Cancel` | Commit or abandon an explicit preview |
| `Undo` / `Redo` | Reverse or restore one committed gesture |

Initial tool grammar:

| Held tool | Mouse primary | Mouse secondary | PS5 X | PS5 Circle | Pick / Square |
|---|---|---|---|---|---|
| Block/material | Remove target | Place at target | Place at target | Remove target | Sample material |
| Material brush | Erase stamp | Paint stamp | Paint stamp | Erase stamp | Sample material |
| Connected fill | Erase connected region | Recolor connected region | Recolor connected region | Erase connected region | Sample replacement material |
| Surface extrude | Remove exposed layer | Extrude exposed face | Extrude exposed face | Remove exposed layer | Sample extrusion material |
| Object select | Select/toggle target | No action | Select/toggle target | Cancel active action | Sample material |
| Transform | Fast ground-plane drag | Begin transform preview | Fast ground-plane drag | Cancel active drag | Sample material |
| Selection wand | Set corner 1 | Set corner 2 | Advance corner 1/2 | Clear selection | Expand selection |
| Fill/Hollow | Start corner 1 | Set corner 2 and commit | Advance start/commit | Clear selection | Sample material |
| Replace/Erase/Clone | No action | Apply region | Apply region | Clear selection | Sample material |
| Linear array | Select target | Apply preview | Select if empty, otherwise apply | Cancel active action | Sample material |

Tool-specific operations such as fill, hollow, replace, clone, mirror, array,
rotate, and axis constraints belong in visible tool options or an active
preview's contextual wheel. They do not each earn a permanent global key.

## Current Implementation

- `WASD` moves, `Space` ascends, `Shift` descends, and `Ctrl` accelerates.
  On PS5, `R2` ascends, `L2` descends, and `L3` accelerates.
- Mouse and right stick look through the same frame input.
- Both controller sticks enter the editor through one canonical 2D primitive:
  negative/positive X means left/right and negative/positive Y means down/up.
  Movement, camera look, and radial selection consume the same radial-deadzone
  signal; consumer profiles own any explicit inversion or response curve.
- `Escape` or controller Options opens Controls from the viewport. Arrow keys,
  D-pad, wheel, pointer, `Enter`, and controller X operate the panel.
  Keyboard + Mouse and PS5 Controller are separate visible tabs; click a tab or
  focus it with vertical navigation and activate it with `Enter`/X.
  Selecting a binding listens for one keyboard/mouse or controller input;
  modifier keys are valid standalone bindings, while modifier-plus-key chords
  retain their modifier. Escape, Circle, or Options cancels capture.
- Rebinding is semantic and context-aware. Reject leaves a conflicting binding
  unchanged, Replace unbinds the displaced action, and Swap exchanges complete
  chords. The Controls open/close/navigation actions are reserved so a remap
  cannot remove the escape hatch. Backspace or controller Square restores the
  active control profile. The separate Reset Wheel button restores only the
  nine creator-tool favorites.
- The Keyboard + Mouse tab owns mouse sensitivity and keyboard/mouse bindings.
  The PS5 Controller tab owns controller sensitivity, movement/look stick
  deadzones and response curves, look X/Y inversion, and PS5-labelled bindings.
  Both tabs expose shared menu repeat timing and conflict policy. Changes save
  immediately to the versioned
  `creative_controls_v1.cfg` user setting beside Creative saves. Loading is
  atomic and fail-closed; settings never enter map documents or map history.
- Standard UI widgets live in `creative/ui/UiWidgets.*`. The fixed frame owns at
  most 192 widget records and 256 visuals with fixed text storage. It provides
  panel, label, button, list-row, stepper, toggle, tab, and text-field emitters;
  pointer/focus routing; scroll visibility; and deterministic menu repeat. The
  `creative/ui/UiTheme.*` palette and overlay renderer remain the sole draw
  path.
- The action ribbon above the hotbar is derived from semantic actions and the
  live remappable control profile. It shows at most six high-priority controls
  for the held tool and current context, uses compact PS5 labels (`X`, `Circle`,
  `R3`, `D-pad`, `L1/R1`, and `L2/R2`), and drops lower-priority hints on narrow
  viewports instead of overlapping text. Unbound or wrong-context actions are
  omitted. Catalog, Tool Options, and Controls retain their in-place widget
  commands rather than receiving a duplicate ribbon.
- The ribbon follows the last unambiguous physical device activity. Keyboard,
  mouse motion/wheel, controller buttons, and shaped controller sticks can
  switch it; simultaneous keyboard/mouse and controller activity preserves the
  previous device to avoid flicker. Remapping a command updates its displayed
  chord without a second label table.
- The retired passive inspector model, draw list, and projection pipeline have
  been removed. UI is now produced by live Creative screens and overlays from
  their owning state; `Facade` no longer builds a second mirrored UI model.
- Controls is the first migrated screen. Keyboard, controller, wheel, pointer,
  hidden-row focus traversal, Reset, Done, binding capture, and repeat all route
  through widget IDs and event receipts. Persistent profile and editor state
  remain owned by Controls rather than by the widget kernel.
- UI navigation enters through one pure input kernel. Wheel profiles explicitly
  choose natural or reversed polarity and rounded or unit steps; pointer samples
  map once from logical-window to drawable coordinates and expose centered Y-up
  coordinates; wrapped selection and clockwise radial sectors share one bounded
  implementation.
- Left/right/middle mouse and controller primary/secondary/pick map to semantic
  world actions with press, hold, and release state.
- Wheel, `1`-`9`, and controller bumpers select a nine-slot hotbar. Movement
  modifiers do not block slot selection or get consumed by it.
- Material Brush slots own independent fixed presets for shape, cylinder axis,
  size, Solid/Shell body, guide, symmetry, occupancy mask, and Replace source.
  Equipping Brush into a fresh slot clones the current brush; D-pad quick edit
  and Tool Options write back only to the selected slot. Switching slots
  restores that preset
  without changing array, volume, transform, or general snap settings. Compact
  labels identify the preset: `S3` is a 3-cell sphere, `C5` a 5-cell cube,
  `CX3` a 3-cell X-axis cylinder, `H` marks a Shell body, and `M` marks enabled
  symmetry. Replacing the slot with a different held-item kind clears its old
  brush preset. Presets are transient editor/hotbar state: they survive
  document New/Load for the current run but are not written into the world save.
- Default slots are material, object select, object move, selection wand, fill,
  hollow, replace, erase, and clone.
- `E` or the controller inventory button opens the searchable material/tool
  catalog. Typing filters results; arrow keys, D-pad, and wheel move selection;
  a result click selects without equipping. `Enter`, controller confirm, or the
  visible Equip command equips the selected result into the active hotbar slot.
  While open, `1`-`9` assigns the selected result directly to that slot without
  closing the catalog. Highlighting any tool and pressing controller Square or
  clicking Assign Wheel opens sector assignment without equipping it.
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
- `R` or controller `R3` opens the nine-sector creator-tool wheel.
  Mouse direction or right stick selects a sector; arrow keys, D-pad up/down,
  and wheel cycle it; `Enter`, controller confirm, or click equips the tool into
  the active hotbar slot. `Escape`, controller cancel, or the toggle closes it.
  Brush owns the first default sector, Connected Fill owns the seventh, Surface
  Extrude owns the eighth, and Array owns the ninth. During assignment, mouse,
  right stick, D-pad, and wheel select the destination; controller X or click
  replaces it and Circle returns to the catalog. Assigning a tool already on the
  wheel swaps the two sectors instead of creating a duplicate. Erase, Region
  Select, and Object Select begin outside the default wheel but can be assigned
  from the catalog.
- Tool-wheel favorites save atomically to `creative_tool_wheel_v1.cfg` beside
  the control profile. Missing or invalid files preserve the default wheel.
  Favorites are editor preferences and never enter map documents, document
  revision, room geometry, or undo history.
- While the wheel is open, `O` or controller Square opens the highlighted
  tool's contextual options. Modal capture prevents X, Circle, and Square from
  mutating the world underneath the wheel.
  Up/down or wheel selects a row;
  left/right changes its value; `Enter`/controller confirm applies the draft;
  `Escape`/controller cancel discards it. Mouse rows, `-`/`+`, Apply, and Cancel
  expose the same semantic actions. Material Brush adds bounded Set/Clear
  Symmetry Pivot command rows; confirm runs the selected command, while a mouse
  click runs that command row directly.
- `R` is an explicit builder extension: it is outside Minecraft's reserved
  movement/world grammar and keeps `Left Alt` available for cursor capture.
  Catalog, tool-wheel, and tool-options contexts are isolated and block
  camera/document input during both opening and closing transitions.
- Contextual settings currently provide Free/X/Z fast-drag movement,
  X/Y/Z precision-preview constraints, 15/45/90-degree
  rotation, 0.25/0.5/1/2-meter grid increments, Material Brush Cube/Sphere/
  Cylinder shape, X/Y/Z cylinder axis, 1/3/5-cell size, Solid/Shell body,
  Free/Line X/Line Y/Line Z/Plane X/Plane Y/Plane Z brush guides,
  Off/Mirror X/Mirror Y/Mirror Z/Mirror XZ symmetry, and Add Only/Replace/
  Overwrite occupancy masks, Replace
  Brush source filtering by material or Any, volume
  Replace source filtering by material or Any, Clone offsets on X/Y/Z at
  1/2/4/8 cells, Connected Fill limits of 64/128/256/512 cells, Surface
  Extrude depths of 1/2/4 cells with 64/128/256/512 affected-cell limits,
  Linear Array
  direction on either world axis with 1/2/4/8/16/32 copies at 1/2/4/8-cell
  spacing, and Radial Array X/Y/Z rings or arcs with 2/4/8/16/32 total
  instances across 90/180/360 degrees.
  Settings edit a non-document draft; Apply changes the editor configuration,
  while the next world operation remains one previewed history transaction.
- In the viewport, D-pad up/down selects the previous/next bounded quick-edit
  channel and D-pad left/right decreases/increases its value. The held-item HUD
  names the active channel and value. Authored transform-backed materials expose
  cardinal placement orientation; voxel, point, and path brushes omit it. In a
  transform preview, D-pad left/right rotates by one quarter turn and D-pad
  up/down performs the existing constrained nudge.
- The Transform tool keeps two speeds of interaction. Primary drag is the fast
  one-object ground-plane move. Secondary (right mouse or controller left
  trigger) snapshots the ordered selection and starts a non-destructive Move
  preview at the crosshair. Platform-command `V` starts the same preview in Copy
  mode from the clipboard.
- During a transform preview, `X`, `Y`, or `Z` toggles a world-axis constraint.
  Arrow up/down, the mouse wheel, or controller D-pad up/down nudges one active
  snap increment along that axis. Shift or controller `L1` makes that nudge one
  quarter of the configured increment. A nudge with no axis fails closed. Camera
  movement and look remain available while the contextual wheel is closed;
  Shift suppresses camera descent only on a frame that actually requests a fine
  nudge.
- `R` or controller `R3` opens a nine-sector contextual wheel: rotate
  `+90`, mirror X, cycle Free/X/Y/Z axis, toggle Copy/Move, confirm, cancel,
  mirror Z, rotate `-90`, and reset. Mouse or right-stick direction selects;
  arrows or D-pad up/down cycle; click, `Enter`, or controller X applies the
  highlighted operation. `Escape` or controller Circle first closes the wheel,
  then cancels the preview when pressed again. Reset clears rotation, mirrors,
  axis constraint, and accumulated nudge offsets.
- Right mouse, `Enter`, or controller X confirms the
  positioned preview. Move changes the original ordered selection; Copy creates
  new objects and selects them. Either result is one atomic history record.
  Primary world input, hotbar changes, and unrelated commands are suppressed
  while the preview is active. Focus loss cancels the non-mutating preview.
- Move previews draw the source amber and the destination mint, with explicit
  pivot boxes and an axis-aligned connector. X, Y, and Z constraints add red,
  green, and blue guides. Invalid but positionable output is red. The compact
  status row names mode, constraint, base/fine step, signed XYZ displacement,
  quarter-turn angle, and mirror flags. Above 512 source objects, each side
  collapses to one aggregate wire box so aiming remains bounded.
- Fill and Hollow are self-contained held tools. Primary starts or replaces
  corner 1 at the crosshair; aiming updates the exact bounded preview; Secondary
  sets corner 2 and commits once. A second Secondary cannot recommit the finished
  region until another Primary starts a new gesture. Cylinders choose X/Y/Z
  extrusion. A one-cell cylinder is a disk and a one-cell-thick box is a wall
  plane. The selection wand remains available for Replace, Erase, Clone, and
  explicit region editing, but it is not a prerequisite for Fill or Hollow.
- The center ray resolves the nearest visible object, hit face, horizontal player
  facing, occupied grid cell, adjacent placement cell, and placement anchor.
- Directional authored objects use descriptor-owned orientation: side placement
  faces outward from the clicked surface, while top/bottom placement turns the
  object's front toward the player. Voxel-backed materials remain unrotated. No
  extra rotate key is required for normal placement.
- Material, select, move, and volume tools use the held-tool grammar above.
- Material Brush paints the selected voxel material with X/right mouse and
  erases with Circle/left mouse. Cube, sphere, and cylinder stamps are
  allocation-free fixed batches at 1, 3, or 5 cells across. Cylinder stamps
  can be oriented along X, Y, or Z without changing the gesture controls.
  Solid emits the complete stamp. Shell retains only cells with at least one
  six-neighbor outside the filled stamp, matching the existing Hollow volume
  boundary law; a 1-cell stamp remains one cell, and a one-cell-thick Plane
  guide remains a complete sheet. A
  Free retains the complete stamp and unconstrained path. Line X/Y/Z retains
  the complete stamp while holding the other two world coordinates at the
  gesture's first valid sample; a red, green, or blue anchor line exposes the
  constrained path. Plane X/Y/Z keeps only the center slice perpendicular to
  that world axis and anchors that coordinate at the first valid sample. The
  guide remains fixed until release even if aim moves across uneven geometry;
  losing the target still breaks path interpolation without moving the anchor.
  With no locked pivot, symmetry uses that same first valid sample as a
  gesture-local pivot. To mirror around empty space, aim at the desired pivot,
  open `R3`, highlight Brush, open Tool Options with Square, select Set Symmetry
  Pivot, and confirm with X. Clear Symmetry Pivot restores gesture-local mode.
  The aim snapshot is resolved on the document's fixed voxel grid before the
  modal opens. A locked pivot survives strokes and tool changes, remains visible
  as a small yellow wire box while Material Brush is held, and clears on New or
  Load. Mirror X, Y, or Z reflects across one world plane; Mirror XZ produces
  the unique four-way set. Direct cells preview green and mirrored cells preview
  cyan. Cells on a mirror plane are emitted once, and direct/mirrored overlap
  across the stroke is deduplicated.
  A held gesture repeats every 200 ms and fills every grid cell crossed between
  valid samples, so fast axial and diagonal sweeps do not leave holes. Losing
  the target breaks that interpolation chain instead of bridging empty space.
  The gesture deduplicates previously visited cells and commits one undo record
  on release.
  The 256-cell gesture budget rejects a whole interpolated segment before
  mutation rather than clipping its shape. The target preview draws one wire
  box for every planned voxel and the status row reports the exact stamp count,
  so a 3-cell sphere visibly contains 7 voxels while a 3-cell cube contains 27.
  `ADD ONLY` writes only empty cells, `REPLACE` recolors only occupied cells,
  and `OVERWRITE` preserves the previous write-anywhere behavior. Replace adds
  a contextual Source option: Any accepts every occupied voxel, while a named
  material accepts only matching voxels. The preview uses the same material-
  aware predicate: admitted cells are green and a wholly blocked stamp is red.
  Shape, cylinder axis, size, body, guide, symmetry, mask, and replace source
  are frozen at the first valid sample so a single held gesture cannot mix
  configurations inside one undo record. Symmetry expansion shares the
  256-cell planner budget and rejects oversized mirrored batches without
  partial mutation. Circle erase is independent of the paint mask and source
  filter. Green wireframes paint; cyan wireframes mirror; red wireframes erase
  or mark blocked/invalid state.
- Connected Fill is the bounded paint-bucket tool for voxel regions. Aim at an
  occupied voxel; right mouse or controller X recolors its complete connected
  same-material component to the material held in that hotbar slot. Left mouse
  or controller Circle erases the same component. Pick block/Square changes the
  slot's replacement material through the existing material-selection path;
  Connected Fill has no duplicate material setting.
  Connectivity is deterministic six-neighbor adjacency in
  `-X,+X,-Y,+Y,-Z,+Z` order, so diagonal contact alone does not join regions.
  The exact accepted component previews cyan. Choosing the source material as
  its replacement previews the exact component red and performs no mutation.
  A component larger than the selected 64/128/256/512-cell Tool Options limit
  fails closed, previews only one red seed cell, and cannot partially edit the
  document. One accepted action is one batched Facade mutation and one undo
  record.
  Preview planning is allocation-free and cached by document id, document
  revision, aimed seed cell, and selected limit. Idle frames reuse the plan;
  voxel edits, undo/redo, New/Load, aim movement, or limit changes force the
  next preview to resolve fresh document truth. This bounded planner is the
  shared region-discovery seam for later surface-selection and extrusion tools.
- Surface Extrude turns an aimed exposed voxel face into a bounded coplanar
  patch. Right mouse or controller X creates 1, 2, or 4 complete layers in the
  outward face direction using the material held in that hotbar slot. Left
  mouse or controller Circle removes the same number of validated inward
  layers. Pick block/Square changes the extrusion material; removal always
  erases the existing source material.
  Patch connectivity uses four tangent neighbors on one face plane. Every cell
  must have the seed material and an empty cell immediately outside the aimed
  face, so diagonal contact, covered cells, corners, and differently oriented
  faces do not join the patch. The app snaps the finite picked face normal to
  one signed world axis before the core planner runs.
  Tool Options and D-pad quick edit expose Depth (`1`, `2`, or `4` cells) and
  Affected Limit (`64`, `128`, `256`, or `512` cells). The limit counts every
  generated or removed layer, not only the visible face: a depth-4 operation
  with a 256-cell limit can therefore contain at most 64 surface cells. Every
  outward destination must be empty, and every inward removal cell must retain
  the source material. Any mismatch, coordinate overflow, or limit breach
  rejects the complete action before mutation.
  Cyan wire boxes show every destination that X/right mouse will create. Red
  shows an invalid operation as one seed box; modal screens and capture mode
  hide the preview. Accepted extrusion and removal each use one batched Facade
  mutation and one undo record. Preview plans are allocation-free and cached
  by document id/revision, seed, face direction, operation, depth, and limit.
- A material's thin green wireframe is only a placement preview. Right mouse or
  controller X attempts the placement. `Wall`, `Floor`, `Ceiling`,
  and `Roof` place one exact voxel cell; props, attachments, paths, lights, and
  other materials remain authored objects. A successful mutation turns the
  crosshair brackets green and draws a thick lime outline from the live object
  or exact voxel cell. Red brackets mean the input arrived but no mutation was
  accepted. An occupied voxel cell or identical authored-object plan stays red,
  and repeated taps add no geometry or undo record.
- Voxel-backed materials and region tools always use the map's fixed block grid.
  The 0.25/0.5/1/2 meter snap option remains available for authored objects and
  transform tools; it does not resize voxel blocks or volume selections.
- Array previews show copies of the current ordered selection without mutating the
  document. Tool Options switches between `LINEAR` and `RADIAL`. Linear owns
  direction, new-copy count, and spacing. Radial owns X/Y/Z axis, total instance
  count, and 90/180/360-degree sweep; the crosshair is its live pivot. A closed
  360-degree ring counts the original as one slot and never duplicates the
  endpoint. Partial sweeps count the original at zero and include the requested
  endpoint. Right mouse/controller Secondary or `Enter` commits the full batch
  atomically as one history transaction, preserves the originals, and selects
  only the final generated copy so repeated commits do not grow exponentially.
  A yellow pivot marker identifies the radial center. Cyan copies are valid;
  red copies indicate that the selection anchor lies on the chosen pivot axis.
- Platform-command `C`, `X`, and `V` copy, cut, and begin clipboard transform.
  Copy aligns the selection's deterministic lower-center placement anchor to the
  crosshair and uses the same quarter-turn/mirror plan as commit. `Escape`,
  Delete, Backspace, or controller cancel abandons the preview. A clipboard from
  the unchanged current document may switch to Move; stale or foreign clipboard
  content remains Copy-only.
- The crosshair, target cell, hotbar selection, placement ghost, placement
  receipt feedback, and volume outline provide visible state. Above the hotbar,
  the held-tool label names operation, shape, cylinder axis, material, and
  whether corner 1 or a completed region is active. Mutations use the existing
  history transactions.
- Manual structural materials and accepted Fill/Hollow cells share the same
  sparse voxel field. Interactive Fill/Hollow previews fail closed above 512
  candidate/generated cells. The red outline appears before commit so an
  accidental distant second corner cannot start unexpectedly large work.
  Accepted bulk cells live in 16-cubed voxel chunks and render through
  dirty-chunk greedy cuboids rather than one document object and mesh per cell.
- Save, new, and load use platform command `S`, `N`, and `O`. No `F`, `F5`, or
  Page Up/Down binding is required.

## Remaining Gaps

- Pause/menu UI is not implemented.
- Authored-object placement restrictions remain a future contextual setting.
  Material Brush already exposes gesture-local world-line and world-plane
  guides; neither form receives permanent global keys.
- Named profile presets, per-device reset, and import/export remain future work.
  The live Controls panel already persists semantic keyboard/mouse and
  controller bindings plus bounded stick, look, and repeat tuning.
- Catalog, Tool Options, and Transform Overlay still own older private panel
  layout code. Migrate them one screen at a time onto the standard widget frame;
  do not rewrite their interaction contracts as part of a mechanical migration.
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
- Randomized Surface Extrude patch parity for all six face directions,
  occluded checkerboards, coordinate boundaries, depth/limit products, and
  occupied late-layer destinations before raising the 512-cell ceiling.
- Randomized radial-array rigid-transform parity for X/Y/Z axes, pre-rotated
  objects near Euler gimbal configurations, parented groups, and 90/180/360
  endpoint laws before increasing the 32-instance or 512-object limits.
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
