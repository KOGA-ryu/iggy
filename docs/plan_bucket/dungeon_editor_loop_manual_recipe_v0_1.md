# Dungeon Editor Loop Manual Recipe v0.1

## Objective

This is the user-run acceptance recipe for the current product dungeon
authoring loop:

```text
launch -> starter -> create dungeon -> see room -> explore in Player mode
-> enter Edit Room / Creative mode -> preview/place floor or wall
-> Leave Editor -> explicit Save -> exit -> relaunch -> Continue
```

The deterministic builder proof for the same loop is headless and receipt
driven. This document is for a real/manual window pass; it does not make the
builder launch a window.

Hard rules for this lane:

- ASCII is map/layout authoring only, not behavior/profile/NPC truth.
- Leave Editor does not autosave.
- Save is explicit through Pause -> Save or Pause -> Save And Exit.
- Receipts prove product state; visual acceptance still requires user/manual
  observation in a window.

## Build

From `/Users/kogaryu/iggy3d`:

```sh
cmake --build build --target iggy3d_app -j 8
```

The app binary is:

```sh
build/iggy3d
```

## Isolated Save Root

Use a fresh save root for each manual pass:

```sh
export IGGY3D_EDITOR_SAVE_ROOT="$(mktemp -d /tmp/iggy3d-editor-loop.XXXXXX)"
```

The first save created by the current loop is:

```sh
"$IGGY3D_EDITOR_SAVE_ROOT/save_001.iggy3d.save"
```

## Window Launch

Use this command for the main manual pass:

```sh
build/iggy3d \
  --window \
  --renderer null \
  --input auto \
  --save-root "$IGGY3D_EDITOR_SAVE_ROOT" \
  --print-render-receipt
```

Notes:

- `--window` opens the SDL product window.
- `--renderer null` uses the current SDL fallback product drawing path and is
  the stable manual recipe default.
- Use `--renderer vulkan` only when intentionally checking the display-dependent
  Vulkan backend path.
- `--input auto` enables keyboard/gamepad auto input selection. It does not
  create a world or run the editor by itself.
- `--print-render-receipt` prints state proof when the app exits.
- Do not add `--frames` for an interactive manual run.

For a short receipt-only window sanity check, add `--frames 1`; that is not a
replacement for the interactive pass.

## Full Manual Loop

### 1. Starter To New World

1. Launch the app with the command above.
2. On the Starter screen, select `New World` and confirm.
   On an empty save root, New World is the current practical create path.

### 2. Create A Custom Draft Dungeon

The New World screen shows the selected dungeon, ASCII room id/source, draft
status, cursor, and ASCII preview.

Current manual title behavior:

- The visible title comes from the selected New World draft/template.
- Free-text title editing in the live window is not currently wired.
- The no-window proof can set `world.title=Custom Draft`; manual window testing
  should not depend on typing a custom title yet.

To make a custom draft manually:

1. Press `Tab` to enter draft edit mode.
2. Move the draft cursor to row `1`, column `2`:
   - press `S` or Down once;
   - press `D` or Right twice.
3. Press `1` to paint `#` at the cursor.
4. Press `Enter` or `Space` to create the world.

New World draft controls:

| Control | Behavior |
| --- | --- |
| `Tab` | Toggle dungeon draft edit mode |
| `W/A/S/D` or arrows | Move draft cursor while edit mode owns input |
| `1` | Paint `#` wall glyph |
| `2` | Paint `.` floor glyph |
| `3` | Paint `P` player-start glyph |
| `4` | Paint `K` key glyph |
| `5` | Paint `$` treasure glyph |
| `6` | Paint `E` exit glyph |
| `7` | Paint `+` door/opening glyph |
| `Enter` or `Space` | Create the world |
| `Escape` | Back out of New World |

When draft edit mode is off, `W/A/S/D` and arrows select built-in dungeon
presets instead of moving the draft cursor. The number-key glyphs are New World
draft paint inputs only.

### 3. See And Explore The Room

After Create, gameplay should open. Visually check that the room is drawn and
the compact mode HUD shows `MODE player`.

In Player mode:

- keyboard movement uses the existing gameplay controls;
- controller left stick maps to player movement;
- room editor cursor, preview, and editor HUD should not be visible.

### 4. Enter Creative/Edit Mode

1. Open Pause.
2. Select `Edit Room`.

Entering Edit Room sets `interaction_mode=creative`. The mode HUD should show
`MODE creative`, and editor cursor/overlay feedback should be visible.

### 5. Preview And Place Geometry

Keyboard room-editor controls:

| Control | Behavior |
| --- | --- |
| `W/A/S/D` | Move editor cursor |
| `1` | Select Floor tool |
| `2` | Select Wall tool |
| `R` | Rotate wall direction clockwise: Up, Right, Down, Left, Up |
| `Q` / `E` | Cycle previous / next tool |
| Left mouse click | Move cursor to clicked grid cell and build/update phantom preview |
| `F` | Build/update phantom preview at current cursor |
| `Enter` | Confirm active phantom preview |
| `C` | Cancel active phantom preview |
| `Space` | Immediate place with active tool |
| `Delete` | Delete primitive under the cursor/tool target |
| `Z` / `Y` | Undo / redo |

Controller creative controls:

| Control | Behavior |
| --- | --- |
| Left stick or d-pad | Move editor cursor |
| West button | Build/update phantom preview |
| South button | Confirm active phantom preview |
| East button | Cancel active phantom preview |
| North button | Rotate wall direction clockwise |
| Left shoulder / right shoulder | Cycle previous / next tool |

The deliberate mode chord is:

```text
LT + RT + L3 + R3
```

The chord toggles `player <-> creative` on gameplay/editor-capable surfaces and
is consumed before ordinary controller actions. Edit Room entry already sets
Creative mode, so the chord is optional for the main editor path.

Practical wall proof path:

1. Move the editor cursor right once.
2. Press `2` for Wall.
3. Press `F` to preview.
4. Press `Enter` to confirm, or press `Space` for immediate placement.

Practical floor proof path:

1. Move the cursor to a cell where no floor exists.
2. Press `1` for Floor.
3. Press `F`.
4. Press `Enter`.

Mouse click policy:

- A creative-mode left click is not placement.
- It moves the editor cursor and builds a phantom preview.
- Real authored room and collision counts change only after explicit confirm.
- `C` cancels the preview without changing room geometry.

### 6. Leave Editor

1. Open Pause.
2. Select `Leave Editor`.

Expected result:

- gameplay remains active;
- `interaction_mode` returns to `player`;
- room editing becomes not ready;
- editor cursor, overlay, phantom preview, and editor HUD hide;
- the active room keeps already-applied edits;
- no save is written by Leave Editor.

Pause Resume is not Leave Editor. Resume only closes the pause overlay.

### 7. Save Explicitly

1. Open Pause.
2. Select `Save`.

Expected result:

- the current active session writes `save_001`;
- the save source is `pause_save`;
- the app remains in the product flow after saving.

After Save, either close the window manually for relaunch testing or use the
product menu's Save And Exit path if you want a product-controlled return to
Starter. Save And Exit writes through the existing save-and-exit path; it is not
required for the Leave Editor -> Save proof.

### 8. Relaunch And Continue

1. Relaunch with the same `--save-root`.
2. On Starter, verify one compatible save is present.
3. Select `Continue`.

Expected result:

- gameplay resumes;
- interaction mode is `player`;
- Edit Room is not opened automatically;
- the loaded room source is `saved_authored_room`;
- the saved floor/wall edits are present.

## Receipt Proof Map

Use these as the minimal fields to inspect from `--print-render-receipt`. The
exact values below match the current custom-draft one-wall proof unless marked
with `<...>`.

After creating the custom draft:

```text
frontend_screen=gameplay
gameplay_active=true
interaction_mode=player
interaction_mode_hud_visible=true
interaction_mode_hud_mode=player
world_setup_dungeon_draft_edit_mode=true
world_setup_dungeon_draft_modified=true
world_setup_dungeon_draft_cursor_row=1
world_setup_dungeon_draft_cursor_column=2
world_setup_dungeon_draft_status=dungeon_draft_cell_painted
world_setup_dungeon_draft_last_glyph=#
world_setup_ascii_room_id=custom_dungeon_draft
active_room_source=ascii_room
active_room_id=custom_dungeon_draft
active_room_authored_floor_count=58
active_room_authored_wall_count=61
product_vulkan_room_mesh_cpu_ready=true
product_vulkan_room_mesh_source=scene_room_projection
product_vulkan_room_asset_id=custom_dungeon_draft
product_vulkan_room_geometry_signature=<positive integer>
```

While editing with a phantom preview:

```text
interaction_mode=creative
interaction_mode_hud_visible=true
interaction_mode_hud_mode=creative
room_editing_ready=true
room_editor_overlay_visible=true
room_editor_preview_visible=true
room_editor_preview_status=room_editor_preview_ready
room_editor_preview_candidate_id=<candidate primitive id>
room_editor_preview_tool=floor|wall
room_editor_preview_grid_x=<grid x>
room_editor_preview_grid_z=<grid z>
room_editor_preview_optimized_draw_delta=<integer>
room_editor_preview_optimized_triangle_delta=<integer>
product_draw_room_editor_preview_count=1
product_render_bridge_room_editor_preview_count=1
```

Before confirm, authored room and collision counts should still match the
pre-preview state. After confirm, the normal room-editing operation fields and
active room counts should reflect the committed primitive.

After Leave Editor:

```text
frontend_screen=gameplay
gameplay_active=true
interaction_mode=player
interaction_mode_hud_visible=true
interaction_mode_hud_mode=player
input_owner=gameplay
gameplay_input_suppressed=false
room_editing_ready=false
room_editing_status=product_room_editing_left
room_editing_last_operation=pause_leave_editor
room_editor_cursor_ready=false
room_editor_overlay_visible=false
room_editor_preview_visible=false
room_editor_hud_visible=false
active_room_id=custom_dungeon_draft
active_room_authored_floor_count=58
active_room_authored_wall_count=62
active_room_collision_ready=true
active_room_collision_query_surface_count=182
active_room_collision_walkable_surface_count=58
active_room_collision_actor_blocker_count=62
active_room_collision_projectile_blocker_count=62
product_vulkan_room_mesh_cpu_ready=true
product_vulkan_room_wall_draw_count=22
```

After explicit Pause -> Save:

```text
frontend_screen=pause
frontend_selected_action=save
gameplay_active=true
interaction_mode=player
room_editing_ready=false
product_save_status=product_save_written
product_save_reason_code=product_save_written
product_save_durable_reason=durable_save_file_written
product_save_source=pause_save
product_save_save_id=save_001
product_save_session_saved=true
active_product_save_id=save_001
```

The saved file should contain:

```text
authoredRoom.id=custom_dungeon_draft
authoredRoom.floor.count=58
authoredRoom.wall.count=62
edit_wall_1
```

On a fresh Starter boot with the same save root:

```text
frontend_screen=starter
gameplay_active=false
interaction_mode=player
interaction_mode_hud_visible=false
save_count=1
compatible_save_count=1
product_save_status=not_requested
product_save_load_status=not_requested
```

After Continue:

```text
frontend_screen=gameplay
gameplay_active=true
interaction_mode=player
interaction_mode_hud_visible=true
interaction_mode_hud_mode=player
input_owner=gameplay
room_editing_ready=false
room_editor_overlay_visible=false
room_editor_preview_visible=false
room_editor_hud_visible=false
product_save_load_status=product_save_loaded
product_save_load_source=continue
product_save_load_save_id=save_001
product_save_load_authored_room_id=custom_dungeon_draft
product_save_load_authored_floor_count=58
product_save_load_authored_wall_count=62
active_room_source=saved_authored_room
active_room_id=custom_dungeon_draft
active_room_authored_floor_count=58
active_room_authored_wall_count=62
active_room_collision_query_surface_count=182
active_room_collision_walkable_surface_count=58
active_room_collision_actor_blocker_count=62
active_room_collision_projectile_blocker_count=62
product_vulkan_room_mesh_cpu_ready=true
product_vulkan_room_asset_id=custom_dungeon_draft
product_vulkan_room_wall_draw_count=22
```

Controller mode/action receipts:

```text
interaction_mode=player|creative
controller_mode_toggle_requested=true|false
controller_mode_toggle_accepted=true|false
controller_mode_toggle_status=interaction_mode_toggled|interaction_mode_surface_blocked|interaction_mode_chord_partial
controller_mode_toggle_surface=gameplay|room_editor|starter
controller_action_status=controller_action_mapped|controller_action_chord_consumed
controller_action_control=left_stick_up|dpad_right|none
controller_action_mode=player|creative
controller_action_surface=gameplay|room_editor|starter
controller_action_input_action=game.move_y|editor.nudge_x|none
```

## Deterministic Proof Targets

| Target | What it proves |
| --- | --- |
| `product_new_world_menu_action_tests` | New World draft edit-mode movement, paint, and create semantics |
| `room_editor_input_tests` | Keyboard editor mappings, including `1`, `2`, `R`, `F`, `Enter`, and `C` |
| `product_room_editor_action_controller_tests` | Editor actions mutate only through the room-editing controller path |
| `product_window_input_frame_tests` | Creative mouse click picks cursor and builds phantom preview without mutation |
| `product_controller_input_smoke` | No-window controller sample injection, mode chord, and creative editor routing |
| `product_ascii_map_smoke` | New World draft, editor input, Leave Editor, explicit Save, fresh Starter, and Continue |
| `product_editor_wall_direction_hotkey_smoke` | Wall direction hotkey persists distinct wall orientations |
| `product_editor_combined_save_continue_smoke` | Floor+wall edits persist together through Save And Exit and Continue |
| `product_continued_room_movement_smoke` | Continued edited room uses restored collision for exploration |
| `product_room_visual_proof_smoke` | Continued edited room has deterministic headless PPM visual artifact |

The Slice 11 leave/save/continue parity case is in `product_ascii_map_smoke`:

```text
ascii_map_custom_draft_leave_editor_pause_save
ascii_map_custom_draft_leave_editor_pause_save_reboot_starter
ascii_map_custom_draft_leave_editor_pause_save_continue
```

It uses real editor input for the wall edit, then the explicit Leave Editor
path, then Pause -> Save, then fresh Continue.

The headless visual artifact proof writes:

```text
custom_dungeon_draft_continue_visual_proof.ppm
```

under that smoke run's isolated temporary save root. The PPM metadata ties the
artifact to `custom_dungeon_draft`, `saved_authored_room`, authored counts,
optimized wall draw count, and geometry signature.

## Headless Control-File Shape

Control files reject duplicate keys. The focused proof uses distinct keys and
aliases to express multiple menu phases in one no-window invocation. The current
shape is:

```text
frontend.execute=true
world.title=Custom Draft
world.draft_cell=1,2,#
world.create=true
system.pause=true
menu.down=true
pause.execute=true
editor.input=editor.nudge_x_pos,editor.next_tool,editor.place
menu.back=true
frontend.select=leave_editor
menu.confirm=true
settings.back=true
pause.select=save
dev_tools.execute=true
```

This is not the preferred manual UX; it is the deterministic automation shape
used to prove the same product path without launching a window.

## Current Gaps

- Manual free-text New World title entry is not wired in the live window. The
  control-file path can set `world.title=Custom Draft`.
- The New World dungeon authoring UI is draft/cursor based, not a full visual
  level editor.
- Mouse click in Creative mode only moves the editor cursor and creates a
  phantom preview. It does not place geometry until `Enter`, controller south,
  or `room_editor.preview_confirm=true`.
- Keyboard and gamepad editor actions are edge-triggered. Hold-to-repeat is not
  a documented guarantee for this loop.
- Builder proof is deterministic and no-window. Manual controller/window
  validation is still user/controller-run hardware and display acceptance.
- Vulkan screenshot/window proof remains separate and display-dependent.
- ASCII remains map/layout authoring only; it does not define behavior,
  profiles, or NPC truth.

## Verification Gate For This Recipe

Docs-only changes should run:

```sh
git diff --check
rg -n "<unresolved-marker-pattern>" <docs checked by this slice>
tools/check_branch_gate.py
```

If a proof target or test helper is changed, also run that exact focused build
and ctest target.
