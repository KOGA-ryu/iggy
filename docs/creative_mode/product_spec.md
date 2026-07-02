# Creative Mode Product Spec

## 1. Product Target

Creative Mode is the in-game level and object editor for product rooms. It lets the user enter an authored room, fly/edit from inside the running product, place and validate objects, inspect existing objects, commit edits through the existing authoring command system, playtest, and save the authored room changes.

The mode relationship is:

- Player mode: normal gameplay control, movement, collisions, HUDs, and playtest output.
- Creative mode: gameplay-world editing control, creative fly/grid visual support, object palette, toolbelt, inspector, preview, validation, and save/playtest commands.
- RoomEditor surface: existing authoring/edit kernel for cursor state, preview, `RoomEditCommand`, dry-run validation, undo/redo, and document bake.
- Playtest: a temporary return from Creative mode to Player control using the current edited active room/collision/projection state.

Ownership lock:

- `src/app/iggy3d/room_editor/` remains the command/preview/edit kernel.
- `src/app/iggy3d/map_maker/` remains the grid/fly/cube-preview support layer for now and becomes a supporting visual/control layer under Creative later.
- Future `src/app/iggy3d/creative/` is a facade/orchestrator only. It coordinates palette, UI packets, placement request packets, selection packets, and proof packets. It must not duplicate `EditableRoomSession`, `RoomEditCommand`, or room-editor preview logic.
- `src/runtime/object/ObjectTraits.*` is the source of palette/object metadata.
- `src/app/iggy3d/ascii_room/*` remains a seed/import path, not the live Creative UI.
- `src/app/iggy3d/world/MovementTestLab.*` remains fixture/reference generation, not the product palette authority.

## 2. Finished UI Layout at 1280x720

The Creative UI uses the existing 1280x720 virtual coordinate model used by `ProductUiDrawList`.

| Region | Rect x/y/w/h | Contents |
| --- | --- | --- |
| Top bar | `0/0/1280/44` | mode badge `CREATIVE`, room title/id, save state, playtest state, object budget, primitive/collision budget, undo/redo availability |
| Left palette | `0/44/236/604` | category tabs, search/filter placeholder, palette rows, recent slots, favorites slots, hotbar slots 1-8 |
| Center viewport | `236/44/808/604` | game viewport, ghost preview, snap point markers, invalid placement tint, selected-object outline, drag handles, grid/fly overlay |
| Bottom toolbelt | `0/648/1280/72` | Select, Place, Move, Rotate, Resize, Duplicate, Delete, Undo, Redo, Playtest, Save |
| Right inspector | `1044/44/236/604` | selected object id/type, transform, size, yaw, snap settings, collision flags, gameplay tags, lock/hidden flags, validation/status |
| Context hints | `244/612/792/28` | minimal current-tool hints, current validation reason, confirm/cancel hints |

Top bar fields:

- `x=12 y=10 w=104 h=24`: mode badge.
- `x=128 y=8 w=280 h=28`: room title and room id.
- `x=420 y=8 w=160 h=28`: save dirty/clean state.
- `x=592 y=8 w=148 h=28`: playtest state.
- `x=752 y=8 w=220 h=28`: object count and budget.
- `x=984 y=8 w=284 h=28`: collision/draw/UI budget summary.

Left palette fields:

- `x=12 y=56 w=212 h=32`: search/filter placeholder.
- `x=12 y=96 w=212 h=32`: category segmented row.
- `x=12 y=136 w=212 h=360`: palette rows from `CreativePaletteGroup`.
- `x=12 y=508 w=212 h=52`: recent/favorites.
- `x=12 y=568 w=212 h=68`: hotbar slots.

Center viewport overlay:

- Ghost preview is drawn in world space through the existing product primitive path.
- Valid preview uses normal ghost tone.
- Warning preview uses amber tint and a visible reason.
- Invalid preview uses red tint and disables confirm.
- Selected object outline uses a stable selected-object tone and object id proof.
- Snap candidates use small point/line overlays, capped and counted.

Right inspector fields:

- `x=1056 y=56 w=212 h=28`: selected object title.
- `x=1056 y=92 w=212 h=96`: transform group.
- `x=1056 y=196 w=212 h=72`: size/yaw group.
- `x=1056 y=276 w=212 h=92`: snap/collision group.
- `x=1056 y=376 w=212 h=108`: gameplay tags/material group.
- `x=1056 y=492 w=212 h=84`: lock/hidden/visibility group.
- `x=1056 y=584 w=212 h=52`: validation/status group.

Bottom toolbelt slots:

- Buttons are 76x44, starting at `x=16 y=662`, with 8px gaps.
- Tool order: Select, Place, Move, Rotate, Resize, Duplicate, Delete, Undo, Redo, Playtest, Save.
- Tool buttons emit `UiHitRegion` rows with stable semantic ids such as `creative.tool.place`, `creative.command.undo`, and `creative.command.playtest`.

## 3. Controls And Interaction

Keyboard and mouse:

- `M` / existing `MapMakerToggle`: enter/exit Creative mode where gameplay owns input.
- Mouse move: move ghost/cursor over floor/surface under the current snap mode.
- Left click: preview/select depending on current tool.
- Enter/Space / `EditorApply`: confirm current preview or apply current inspector field.
- Escape / `EditorCancelPreview`: cancel active preview, then back out of tool focus.
- `EditorPlace`: commit the current place command when preview is valid.
- Delete / `EditorDelete`: delete selected authored object/floor/wall where supported.
- `Ctrl+Z` / `EditorUndo`: undo last room edit command.
- `Ctrl+Y` or `Ctrl+Shift+Z` / `EditorRedo`: redo.
- Tab / `EditorNextTool`: next tool.
- Shift+Tab / `EditorPreviousTool`: previous tool.
- `R` / `EditorRotateWallDirection`: rotate placement or wall direction by configured increment.
- Arrow keys/WASD: existing room-editor cursor/fly movement according to active surface.

Controller:

- Left stick: move cursor/ghost or creative fly depending on focus.
- Right stick: look/rotate camera.
- D-pad: palette/tool/inspector navigation.
- South button: confirm/apply/place through `EditorApply` or `EditorPlace`.
- East button: cancel preview through `EditorCancelPreview`.
- Shoulder buttons: previous/next tool or palette category.
- Trigger/bumper combinations: rotate/resize when the current tool supports it.
- Start/options: pause/menu, not a Creative command.

Existing action names to reuse first:

- `MapMakerToggle`
- `EditorPlace`
- `EditorApply`
- `EditorCancelPreview`
- `EditorPreviewPlacement`
- `EditorDelete`
- `EditorUndo`
- `EditorRedo`
- `EditorNextTool`
- `EditorPreviousTool`
- `EditorRotateWallDirection`

Future actions, not first implementation requirements:

- `CreativeDuplicate`
- `CreativeTogglePlaytest`
- `CreativeSaveRoom`
- `CreativePaletteSearch`
- `CreativeFavoriteObject`
- `CreativeSnapModeNext`
- `CreativeInspectorFocusNext`

## 4. Object Palette Categories

Palette source is `src/runtime/object/ObjectTraits.*`. The Creative facade projects `ObjectAssetDefinition.editor.paletteGroup`, display name, placeable/rotatable/scalable flags, collision traits, physics traits, material traits, and interaction traits into Creative palette rows. UI strings should be projection labels from object traits or descriptor tables, not hardcoded at the widget site.

Initial categories:

- Structure: floor, wall, platform.
- Movement: wall-run surface, ledge/mantle block, climb pipe placeholder, swing bar placeholder, tightrope beam placeholder.
- Physics: crate, pushable block placeholder, moving platform placeholder.
- Markers: player spawn, exit, target marker, checkpoint, debug trigger volume.
- Test Lab: movement-lane markers when useful for test-room authoring.

Current catalog support confirmed by headers:

- `ObjectAssetDefinition` carries asset id, kind, primitive shape, render profile, collision template, physics profile, material traits, interaction traits, save flags, and editor metadata.
- `ObjectEditorMetadata` carries palette group, display name, placeable, rotatable, and scalable flags.

Known gaps to represent as locked/placeholder rows until catalog entries exist:

- climb pipe placeholder
- swing bar placeholder
- tightrope beam placeholder
- moving platform placeholder
- debug trigger volume if no current collision/interaction trait maps cleanly
- movement-lane markers if they only exist as Movement Test Lab descriptors

## 5. Placement Preview And Validation

Ghost states:

- `valid`: candidate can be committed now.
- `warning`: candidate can be committed but has a warning reason.
- `invalid`: candidate cannot be committed.
- `selected`: existing authored object/floor/wall is selected, not a new placement ghost.

Validation reasons:

- `overlap`
- `out_of_bounds`
- `no_support`
- `locked_target`
- `missing_surface`
- `budget_exceeded`
- `unknown_asset`
- `not_placeable`
- `invalid_transform`
- `invalid_size`
- `unsupported_tool`
- `room_editor_not_ready`

Confirm/cancel/continuous placement:

- Preview is built continuously from the current cursor/ghost request.
- Confirm commits only when preview status is valid or warning.
- Cancel clears preview and leaves the tool selected.
- Continuous placement keeps the selected palette object active after a successful place command.
- Select/move/resize/rotate tools operate on a selected object and emit command requests only on confirm/apply, not every hover tick.

Snap modes:

- Grid snap: first slice.
- Surface snap: first slice if existing cursor-to-surface data is available through room-editor cursor/pick.
- Slot/socket snap: future.
- Vertical story/layer snap: first slice should expose story/layer value only if already present in `ProductRoomEditorCursorState`; deeper story tools are future.
- Rotate increment: first slice, using the current wall/object rotate semantics where present.

First-slice scope:

- Palette selection.
- Place preview request adapter.
- Valid/invalid/warning state.
- Grid snap.
- Confirm/cancel.
- Existing room-editor dry-run validation.
- Receipt proof.

Future scope:

- Drag handles.
- Object sockets.
- Multi-select.
- Duplication.
- Inspector editing of every field.
- Search indexing.
- Favorite/hotbar persistence.
- Advanced budget editor.

## 6. Data-Oriented Architecture

Future structs and target ownership:

- `CreativeObjectSpec`: `src/app/iggy3d/creative/Palette.hpp`; projected cold view of `ObjectAssetDefinition`.
- `CreativePaletteGroup`: `src/app/iggy3d/creative/Palette.hpp`; category id, label, slot range.
- `CreativeToolSpec`: `src/app/iggy3d/creative/Tools.hpp`; tool id, label, required selection kind, input actions.
- `CreativeSnapRule`: `src/app/iggy3d/creative/Snap.hpp`; snap id, enabled flag, priority, grid/surface/layer config.
- `CreativeInspectorFieldSpec`: `src/app/iggy3d/creative/Inspector.hpp`; field id, label, type, read/write policy.

Hot/cold packets:

- `CreativeFrameInputPacket`: `src/app/iggy3d/creative/Input.hpp`; one frame of resolved editor/creative actions, pointer/cursor sample, controller sample, active tool, active palette slot.
- `CreativePlacementRequest`: `src/app/iggy3d/creative/Placement.hpp`; selected object spec id, cursor/snap data, transform, tool.
- `CreativePlacementPreviewPacket`: `src/app/iggy3d/creative/Placement.hpp`; validation result, candidate `RoomEditCommand`, ghost draw facts, counts.
- `CreativeSnapCandidatePacket`: `src/app/iggy3d/creative/Snap.hpp`; contiguous snap candidates for current frame.
- `CreativeSelectionPacket`: `src/app/iggy3d/creative/Selection.hpp`; contiguous selected refs and selected bounds.
- `CreativeUiModelPacket`: `src/app/iggy3d/creative/UiModel.hpp`; top bar, palette slots, toolbelt items, inspector rows, context hints.
- `CreativeMetricsPacket`: `src/app/iggy3d/creative/Metrics.hpp`; preview ns, snap count, validation failures, command commit count, rebuild count, draw/hit counts.

Contiguous arrays:

- palette slots
- palette groups
- tool specs
- snap candidates
- selected refs
- inspector field rows
- UI primitives
- UI hit regions
- preview draw items
- validation reasons for current frame

Cold metadata:

- labels
- descriptions
- material names
- object category names
- inspector display text
- long reason strings
- help text
- search tokens

The hot path should pass ids, enum values, indices, counts, transforms, and compact packets. It should not repeatedly copy labels or scan arbitrary strings during cursor motion.

## 7. Repo Wiring Plan

Enter Creative mode:

1. Input samples enter through `src/app/input/KeyboardInput.*`, `src/app/input/GamepadInput.*`, and `src/app/input/InputBindings.cpp`.
2. `src/app/iggy3d/window/InputFrame.cpp` resolves `MapMakerToggle` and editor actions through existing active-surface/input-owner rules.
3. `src/app/iggy3d/menu/ActionHandlers.*` keeps existing map-maker/creative gating policy.
4. `src/app/iggy3d/input/InteractionModeState.*` and `src/app/iggy3d/input/InteractionMode.*` continue to represent `ProductInteractionMode::Creative` and `ProductInteractionMode::RoomEditor`.
5. Future `src/app/iggy3d/creative/Mode.hpp/.cpp` derives the Creative facade state from interaction mode plus resolved active surface.

Palette object selected:

1. `src/runtime/object/ObjectTraits.*` builds or exposes `ObjectAssetCatalog`.
2. Future `src/app/iggy3d/creative/Palette.*` projects placeable catalog assets into `CreativeObjectSpec` and `CreativePaletteGroup`.
3. Future `src/app/iggy3d/creative/UiModel.*` stores selected palette slot in `CreativeUiModelPacket`.
4. `src/app/iggy3d/ui/Widget.*` and `src/app/iggy3d/menu/DrawList.*` emit palette primitives and hit regions.
5. Future input routing maps palette hit semantic ids to selected palette slot updates.

Mouse/controller moves ghost:

1. Raw mouse/controller input enters through `src/app/input/*`.
2. `src/app/iggy3d/window/InputFrame.cpp` builds frame actions only when Creative/RoomEditor owns input.
3. `src/app/iggy3d/room_editor/Cursor.*` remains the cursor/grid/wall-direction primitive.
4. Future `src/app/iggy3d/creative/Snap.*` builds snap candidates from cursor, active room, selected object spec, and snap rules.
5. Future `src/app/iggy3d/creative/Placement.*` converts current object spec plus snap result into `CreativePlacementRequest`.

Preview validates:

1. `CreativePlacementRequest` adapts into `ProductRoomEditorPlacementPreviewRequest`.
2. `src/app/iggy3d/room_editor/Preview.*` builds `ProductRoomEditorPlacementPreviewResult`.
3. `src/content/authoring/EditableRoomDocument.*` applies dry-run validation through `RoomEditCommand` and document copy.
4. Future `CreativePlacementPreviewPacket` copies validation, candidate command, object id, ghost transform, counts, and reason code.
5. `src/app/iggy3d/gameplay/ProjectionRefresh.*` copies proof to `ProductAppWindowState` only through packet summaries.

Confirm commits `RoomEditCommand`:

1. `EditorPlace` or `EditorApply` arrives through existing input action paths.
2. Future `CreativePlacementPreviewPacket.candidateCommand` is passed to `src/app/iggy3d/room_editor/AuthoringController.*`.
3. `ProductRoomAuthoringController::submit(...)` submits to `EditableRoomSession`.
4. `EditableRoomSession::submit(...)` applies command and updates undo/redo history.
5. `buildProductRoomAuthoringSnapshot(...)` bakes the edited document into room/projection data.

Active room/collision/render refreshes:

1. `src/app/iggy3d/room_editor/EditableRoomToAuthoredRoom.*` converts editable document state.
2. `src/app/iggy3d/gameplay/ActiveRoomState.*` receives active authored room state.
3. `src/app/iggy3d/gameplay/ActiveRoomCollision.*` rebuilds collision.
4. `src/app/iggy3d/gameplay/ProjectionRefresh.*` refreshes presentation packets.
5. `src/app/iggy3d/view/PrimitiveDrawList.*` builds room/editor/preview primitives.
6. `src/app/iggy3d/window/FramePresenter.*` presents the frame.

Select existing object and inspect:

1. Future `src/app/iggy3d/creative/Selection.*` resolves pick/hit to object/floor/wall references.
2. `CreativeSelectionPacket` stores selected refs contiguously.
3. Future `src/app/iggy3d/creative/Inspector.*` projects selected object fields and object trait metadata into `CreativeInspectorFieldSpec` rows.
4. `src/app/iggy3d/ui/Widget.*` emits inspector rows and hit regions.

Delete/undo/redo:

1. `EditorDelete`, `EditorUndo`, and `EditorRedo` come from existing input actions.
2. Delete produces a `RoomEditCommand` against the selected ref.
3. Undo/redo call `ProductRoomAuthoringController::undo(...)` / `redo(...)`.
4. Snapshot refresh follows the same active room/collision/projection path.

Toggle playtest:

1. Future `CreativeTogglePlaytest` or a toolbelt hit region switches from Creative facade focus to Player mode while preserving current edited active room state.
2. Playtest uses `ProductInteractionMode::Player`.
3. Returning to Creative restores Creative facade UI state, not stale map-maker-only state.

Save authored room edits:

1. Edited authored room data is held by `ProductRoomAuthoringSnapshot`.
2. Existing save path uses `src/runtime/save/SaveEnvelope.hpp` and `src/runtime/save/SaveCodec.*`, which already carry authored room save records for floors/walls/objects/markers.
3. App save bridge remains under `src/app/iggy3d/save/`.
4. Save UI receives result/proof through `src/app/iggy3d/ReceiptBuilder.*`.

## 8. Consolidation Policy

- `room_editor`: keep as authoring kernel. It owns cursor primitives, preview dry-run, `RoomEditCommand`, `EditableRoomSession`, undo/redo, and document bake.
- `map_maker`: keep grid/fly/cube preview for now. It becomes Creative support only after the Creative facade works and tests prove behavior parity.
- `creative`: future facade/orchestrator. It owns palette projection, selected palette slot, tool model, placement request packets, selection packets, UI model packets, and metrics packets. It does not own a second command system.
- UI moves toward `ProductUiDrawList` and `UiHitRegion`. Creative UI should not add a second hand-drawn rectangle/hit-test path.
- Receipts move toward packet summaries: `CreativeUiModelPacket`, `CreativePlacementPreviewPacket`, `CreativeSelectionPacket`, and `CreativeMetricsPacket`, not hundreds of unrelated scalar fields.
- File/folder renames wait until the facade exists and tests prove the old `room_editor` and `map_maker` behaviors are preserved.

## 9. Hot Paths And Metrics

Likely hot paths:

- cursor-to-world pick
- snap candidate scan
- placement preview dry run
- active room rebuild after confirmed edit
- active collision rebuild after confirmed edit
- primitive draw list build
- UI primitive and hit-region build
- inspector row build when selection changes

Metrics needed:

- `creative_preview_build_ns`
- `creative_snap_candidate_count`
- `creative_validation_failure_count`
- `creative_command_commit_count`
- `creative_active_room_rebuild_count`
- `creative_collision_surface_count`
- `creative_primitive_draw_item_count`
- `creative_ui_primitive_count`
- `creative_ui_hit_region_count`
- `creative_selected_ref_count`
- `creative_palette_slot_count`
- `creative_inspector_field_count`

Do not optimize yet:

- renderer/Vulkan backend
- physics solver
- save codec/schema
- object trait catalog storage
- full text search
- object sockets
- collision rebuild internals before metrics prove rebuild cost
- primitive draw internals before draw item counts and scenario metrics prove the pressure

## 10. First 5 Implementation Slices

### 1. Creative palette descriptor facade

Likely files:

- `src/app/iggy3d/creative/Palette.hpp`
- `src/app/iggy3d/creative/Palette.cpp`
- `src/runtime/object/ObjectTraits.hpp`
- `src/runtime/object/ObjectTraits.cpp`
- `tests/unit/product_creative_palette_tests.cpp`
- `CMakeLists.txt`

DOD:

- `CreativeObjectSpec` and `CreativePaletteGroup` are projected from `ObjectAssetCatalog`.
- Placeable object assets appear in stable group/slot order.
- Missing placeholder categories are represented as disabled rows only if backed by descriptor rows.
- No UI drawing or placement behavior changes.

No-go:

- no source-of-truth strings in UI
- no save schema changes
- no object placement behavior changes

### 2. Creative UI draw-list shell

Likely files:

- `src/app/iggy3d/creative/UiModel.hpp`
- `src/app/iggy3d/creative/UiModel.cpp`
- `src/app/iggy3d/menu/DrawList.hpp`
- `src/app/iggy3d/menu/DrawList.cpp`
- `src/app/iggy3d/ui/Widget.hpp`
- `src/app/iggy3d/ui/Widget.cpp`
- `tests/unit/product_creative_ui_model_tests.cpp`
- `tests/unit/product_ui_draw_list_tests.cpp`

DOD:

- Builds `CreativeUiModelPacket` for 1280x720.
- Emits top bar, left palette shell, bottom toolbelt, right inspector shell, context hint region.
- Emits `ProductUiDrawList` primitives and `UiHitRegion` rows with stable semantic ids.
- Does not route hits yet.

No-go:

- no renderer backend changes
- no duplicated hit-test geometry outside draw-list/hit-region model
- no behavior changes

### 3. Placement packet adapter to existing room editor preview

Likely files:

- `src/app/iggy3d/creative/Placement.hpp`
- `src/app/iggy3d/creative/Placement.cpp`
- `src/app/iggy3d/creative/Snap.hpp`
- `src/app/iggy3d/creative/Snap.cpp`
- `src/app/iggy3d/room_editor/Preview.hpp`
- `src/app/iggy3d/room_editor/Preview.cpp`
- `tests/unit/product_creative_placement_tests.cpp`
- `tests/unit/product_room_editor_preview_tests.cpp`

DOD:

- Converts selected `CreativeObjectSpec` plus cursor/snap data into `CreativePlacementRequest`.
- Calls existing `buildProductRoomEditorPlacementPreview(...)`.
- Builds `CreativePlacementPreviewPacket`.
- Valid/warning/invalid proof is stable.
- No commit/place mutation yet.

No-go:

- no new command system
- no active room/collision mutation
- no movement/physics behavior changes

### 4. Visual creative overlay integration into projection/presenter/receipts

Likely files:

- `src/app/iggy3d/creative/Overlay.hpp`
- `src/app/iggy3d/creative/Overlay.cpp`
- `src/app/iggy3d/gameplay/ProjectionRefresh.hpp`
- `src/app/iggy3d/gameplay/ProjectionRefresh.cpp`
- `src/app/iggy3d/view/PrimitiveDrawList.hpp`
- `src/app/iggy3d/view/PrimitiveDrawList.cpp`
- `src/app/iggy3d/window/FramePresenter.cpp`
- `src/app/iggy3d/ReceiptBuilder.hpp`
- `src/app/iggy3d/ReceiptBuilder.cpp`
- `tests/unit/product_creative_ui_model_tests.cpp`
- `tests/unit/product_primitive_draw_list_tests.cpp`

DOD:

- Valid/warning/invalid ghost preview appears through existing primitive draw/proof path.
- Selected-object outline proof exists.
- Creative UI draw-list appears only on Creative/RoomEditor surface.
- Receipt exposes packet summaries and counts.

No-go:

- no Vulkan backend allocation/submission changes
- no hand-built second overlay renderer
- no broad receipt scalar expansion

### 5. Confirm/place/save persistence proof

Likely files:

- `src/app/iggy3d/creative/Placement.cpp`
- `src/app/iggy3d/room_editor/AuthoringController.hpp`
- `src/app/iggy3d/room_editor/AuthoringController.cpp`
- `src/app/iggy3d/gameplay/ActiveRoomState.cpp`
- `src/app/iggy3d/gameplay/ActiveRoomCollision.cpp`
- `src/app/iggy3d/save/SaveBridge.*`
- `src/app/iggy3d/window/InputFrame.cpp`
- `tests/unit/product_creative_placement_tests.cpp`
- `tests/smoke/product_creative_placement_save_smoke.cpp`

DOD:

- Confirm commits candidate `RoomEditCommand`.
- Active room/collision/projection refresh.
- Undo/redo still work.
- Save writes authored object/floor/wall/marker data through existing save envelope/codec.
- Continue/load shows the placed object after save.

No-go:

- no save schema change
- no renderer backend change
- no duplicate authoring session

## 11. Acceptance And Test Map

Existing likely targets to reuse:

- `product_room_editor_action_controller_tests`
- `product_room_editor_cursor_tests`
- `product_room_editor_preview_tests`
- `product_room_editor_overlay_tests`
- `product_room_editor_hud_tests`
- `product_map_maker_grid_tests`
- `product_creative_fly_tests`
- `product_map_maker_presentation_tests`
- `object_traits_tests`
- `product_active_room_state_tests`
- `product_active_room_collision_tests`
- `product_primitive_draw_list_tests`
- `product_ui_draw_list_tests`
- `product_ui_widget_tests`
- `product_controller_action_map_tests`
- `product_controller_action_routing_tests`
- `product_interaction_mode_tests`
- `product_interaction_mode_state_tests`
- `product_window_input_frame_tests`
- `product_ascii_map_smoke`
- `product_editor_floor_save_continue_smoke`
- `product_editor_combined_save_continue_smoke`
- `product_editor_wall_direction_hotkey_smoke`

Missing planned targets:

- `product_creative_palette_tests`
- `product_creative_tools_tests`
- `product_creative_placement_tests`
- `product_creative_selection_tests`
- `product_creative_ui_model_tests`
- `product_creative_placement_save_smoke`

Acceptance map:

- Use the existing movement lab room as the first authored-room test surface when it already has floors/walls/object anchors.
- Creative placement/save smoke should start from a deterministic authored room, enter Creative, select a placeable object from `ObjectTraits`, preview placement, confirm, save, reload/continue, and assert authored object count plus visible/proof object id.

Acceptance proof:

- palette projection is catalog-driven
- UI shell rects match this spec
- preview packets adapt to room-editor preview
- confirm path commits `RoomEditCommand`
- active room/collision/projection rebuild after edit
- save/load preserves authored objects
- receipts expose packet summaries and counts

## Unresolved Decisions

- Exact first placeholder asset ids for climb pipe, swing bar, tightrope beam, moving platform, checkpoint, debug trigger volume.
- Whether surface snap can be first-slice or must wait for a dedicated pick/snap candidate packet.
- Whether hotbar/favorites are runtime-only or persisted later.
- Whether playtest toggle needs a new input action immediately or can be driven by UI hit region first.
- Whether inspector edits are direct command commits or staged preview commits per field group.
