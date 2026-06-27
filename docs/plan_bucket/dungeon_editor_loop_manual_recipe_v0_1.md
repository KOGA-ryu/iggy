# Dungeon Editor Loop Manual Recipe v0.1

## Objective

Give a practical user-facing recipe for the current dungeon editor loop without
guessing flags or receipt fields. This document connects the live window path
to the deterministic no-window proof that already covers:

- New World from a custom draft;
- Pause to Edit Room;
- real editor input for direct floor/wall tool selection, wall-direction
  rotation, place/delete/undo/redo;
- Save And Exit;
- fresh starter boot;
- Continue into the saved edited room;
- headless PPM visual proof for the continued room;
- creative-mode mouse click cursor picking and phantom placement preview;
- controller player/creative mode switching and creative editor control
  semantics through no-window receipt proof.

This recipe does not require a builder to open a real window. The live window
steps are for a user-run manual check.

## Build

From `/Users/kogaryu/iggy3d`:

```sh
cmake --build build --target iggy3d_app -j 8
```

The app binary is:

```sh
build/iggy3d
```

## Save Root

Use an isolated save root while testing:

```sh
export IGGY3D_EDITOR_SAVE_ROOT="$(mktemp -d /tmp/iggy3d-editor-loop.XXXXXX)"
```

The current save file created by this loop is expected at:

```sh
"$IGGY3D_EDITOR_SAVE_ROOT/save_001.iggy3d.save"
```

## Manual Window Launch

Normal interactive launch:

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
- `--renderer null` uses the current product SDL fallback drawing path. Use
  `--renderer vulkan` only when intentionally checking the Vulkan backend.
- `--print-render-receipt` prints the receipt after the app exits.
- Do not add `--frames` for an interactive manual run, because that exits after
  the requested frame count.

## Manual Flow

1. On the Starter screen, select `New World`.
2. On the New World screen, press `Tab` to enter dungeon draft edit mode.
3. Move the draft cursor to row `1`, column `2`:
   - press `S` or Down once;
   - press `D` or Right twice.
4. Press `1` to paint `#` as a wall glyph at the cursor.
5. Press `Enter` or `Space` to create the world from the edited draft.
6. Enter gameplay.
7. Open Pause.
8. Select `Edit Room`.
9. Use editor controls:
   - `W/A/S/D` or d-pad: move editor cursor.
   - `1`: select Floor tool.
   - `2`: select Wall tool.
   - `R`: rotate wall direction clockwise while the Wall tool is active.
   - `Q/E` or shoulders: cycle editor tools when direct selection is not desired.
   - `Space` or gamepad south: place.
   - `Delete`: delete.
   - `Z`: undo.
   - `Y`: redo.
10. Place at least one wall or floor so the edited room changes. A compact manual
    proof path is: move right once, press `2`, press `Space`, press `R`, press
    `Space` again. That creates two wall edits with distinct directions.
11. Open Pause again.
12. Select `Save And Exit`.
13. Relaunch with the same `--save-root`.
14. Select `Continue`.

Expected user-visible state after Continue:

- gameplay resumes instead of opening the editor automatically;
- the restored room is the saved edited room;
- the room editor HUD is not visible until Edit Room is opened again;
- when Edit Room is opened again, the editor HUD and cursor/tool overlay should
  be visible.

## Manual Controller Acceptance Flow

Use the same build, save root, and window launch above:

```sh
cmake --build build --target iggy3d_app -j 8

build/iggy3d \
  --window \
  --renderer null \
  --input auto \
  --save-root "$IGGY3D_EDITOR_SAVE_ROOT" \
  --print-render-receipt
```

Then run this controller-focused manual path:

1. Create a New World/custom dungeon using the manual New World draft steps
   above.
2. Enter gameplay.
3. In player mode, use the controller left stick to move the player. Player
   mode routes controller movement to gameplay movement.
4. Open Pause and select `Edit Room`. Entering room editing sets
   `interaction_mode=creative`.
5. In creative mode, controller input routes to room-editor actions:

| Control | Creative room-editor behavior |
| --- | --- |
| Left stick or d-pad | Move editor cursor |
| West button | Build/update phantom placement preview |
| South button | Confirm active phantom preview |
| East button | Cancel active phantom preview |
| North button | Rotate wall direction clockwise |
| Left shoulder | Cycle to previous tool |
| Right shoulder | Cycle to next tool |

6. Use the creative controls to preview and confirm at least one floor or wall
   edit.
7. Press and hold `LT + RT + L3 + R3` once to return to player mode.
8. In player mode, controller movement should control the player again instead
   of the editor cursor.
9. Press and release the chord again if you want to return to creative mode
   during the same gameplay session.
10. Save And Exit.
11. Relaunch with the same `--save-root`.
12. Select `Continue`.

The mode chord is deliberately handled before ordinary controller actions. When
the full chord is active, normal controller action routing is consumed for that
frame so the chord does not also place, preview, move, or interact.

Mode lifecycle is intentionally narrow today: entering `Edit Room` sets
`interaction_mode=creative`, while Save And Exit and Return To Title reset the
starter/title state to `interaction_mode=player`. There is not yet a separate
"leave editor but stay in gameplay" command; closing a pause overlay while room
editing remains ready does not by itself leave the editor.

Current mode feedback is receipt/proof based:

```text
interaction_mode=player|creative
controller_mode_toggle_requested=true|false
controller_mode_toggle_accepted=true|false
controller_mode_toggle_status=interaction_mode_toggled|interaction_mode_surface_blocked|...
controller_mode_toggle_surface=gameplay|room_editor|starter|...
controller_action_status=controller_action_mapped|controller_action_chord_consumed|...
controller_action_control=<controller control name>
controller_action_mode=player|creative
controller_action_surface=gameplay|room_editor|...
controller_action_input_action=<mapped InputAction name>
```

The live UI currently does not add controller shortcut/tutorial text. A compact
visible mode indicator can be added later if it fits an existing gameplay HUD
state area without becoming instructions.

## Window Mouse Creative Placement Acceptance

Use this user-run path to check the live mouse flow. The builder verification
for this recipe remains headless; do not treat this section as a CI window gate.

1. Build and launch the product app with the same isolated save root:

```sh
cmake --build build --target iggy3d_app -j 8

build/iggy3d \
  --window \
  --renderer null \
  --input auto \
  --save-root "$IGGY3D_EDITOR_SAVE_ROOT" \
  --print-render-receipt
```

2. Create and enter a dungeon world using the New World draft steps above.
3. Open Pause and select `Edit Room`.
4. Edit Room entry sets `interaction_mode=creative`, so creative/editor mouse
   control is active immediately. The controller chord `LT + RT + L3 + R3`
   remains available if you intentionally switch back to player mode and later
   return to creative mode.
5. Select the intended tool:
   - press `1` for Floor;
   - press `2` for Wall;
   - press `R` to rotate wall direction before previewing a wall.
6. Left mouse click in the room view.

Expected after the click:

- the editor cursor moves to the clicked grid cell;
- a phantom placement preview appears for the current tool and cursor;
- the active authored floor/wall counts do not change yet;
- collision counts do not change yet;
- no save/write occurs from the click.

7. Confirm or cancel explicitly:
   - press `Enter`, controller south, or run `room_editor.preview_confirm=true`
     to commit the active phantom preview;
   - press `C`, controller east, or run `room_editor.preview_cancel=true` to
     clear it without changing the room.
8. If confirmed, Save And Exit, relaunch with the same `--save-root`, then
   Continue. The confirmed edit should persist only after that explicit confirm.

A click is intentionally not placement. It updates cursor plus phantom preview;
the existing confirm/cancel path decides whether real geometry changes.

If using `--print-render-receipt`, the relevant proof fields after a creative
mouse preview are:

```text
interaction_mode=creative
room_editor_grid_x=<clicked grid x>
room_editor_grid_z=<clicked grid z>
room_editor_last_operation=room_editor.mouse_pick
room_editor_last_operation_accepted=true
room_editor_preview_visible=true
room_editor_preview_status=room_editor_preview_ready
room_editor_preview_candidate_id=<candidate primitive id>
room_editor_preview_tool=floor|wall
room_editor_preview_grid_x=<clicked grid x>
room_editor_preview_grid_z=<clicked grid z>
room_editor_preview_optimized_draw_delta=<integer>
room_editor_preview_optimized_triangle_delta=<integer>
product_draw_room_editor_preview_count=1
product_render_bridge_room_editor_preview_count=1
```

After confirm, expect the preview counts to return to zero and the normal room
editing/authored/collision receipts to reflect the committed primitive. After
cancel, expect the preview to be cleared and the authored/collision counts to
remain unchanged.

## New World Draft Controls

The New World draft editor is controlled through the existing menu input layer.
Press `Tab` before painting. When draft edit mode is off, `W/A/S/D` and the
arrow keys select built-in dungeon presets instead of moving the draft cursor.

| Control | New World draft behavior |
| --- | --- |
| `Tab` | Toggle dungeon draft edit mode |
| `W` or Up | Move draft cursor up while edit mode is on |
| `S` or Down | Move draft cursor down while edit mode is on |
| `A` or Left | Move draft cursor left while edit mode is on |
| `D` or Right | Move draft cursor right while edit mode is on |
| `1` | Paint `#` wall glyph |
| `2` | Paint `.` floor glyph |
| `3` | Paint `P` player-start glyph |
| `4` | Paint `K` key glyph |
| `5` | Paint `$` treasure glyph |
| `6` | Paint `E` exit glyph |
| `7` | Paint `+` door/opening glyph |
| `Enter` or `Space` | Create the world |
| `Escape` | Back out of New World |

Gamepad d-pad, south, and east buttons can navigate, confirm, and back out of
New World. Glyph painting is currently keyboard number-key driven.

ASCII glyphs remain map/layout authoring only. The New World number keys are
only draft paint inputs while the New World draft editor owns input; they are
separate from the in-game room editor tool hotkeys below.

## In-Game Room Editor Controls

Open Pause, select `Edit Room`, then use these controls while room editing is
active:

| Control | In-game room editor behavior |
| --- | --- |
| `W` or d-pad Up | Move editor cursor up |
| `S` or d-pad Down | Move editor cursor down |
| `A` or d-pad Left | Move editor cursor left |
| `D` or d-pad Right | Move editor cursor right |
| `1` | Select Floor tool |
| `2` | Select Wall tool |
| `R` | Rotate wall direction clockwise: Up, Right, Down, Left, Up |
| `Q` or left shoulder | Cycle to previous tool |
| `E` or right shoulder | Cycle to next tool |
| Left mouse click in creative mode | Move editor cursor and build/update phantom preview |
| `F` or gamepad west | Build/update phantom preview at current cursor |
| `Enter` or gamepad south | Confirm active phantom preview |
| `C` or gamepad east | Cancel active phantom preview |
| `Space` | Place immediately with the active tool |
| `Delete` | Delete the primitive under the cursor/tool target |
| `Z` | Undo |
| `Y` | Redo |

Direct `1`/`2` selection is the preferred manual way to choose Floor or Wall.
`Q/E` still work for cycling and remain covered by the input/controller tests.

## Bounded Window Receipt Check

For a short user-run receipt sanity check, use a finite frame count:

```sh
build/iggy3d \
  --window \
  --renderer null \
  --input auto \
  --save-root "$IGGY3D_EDITOR_SAVE_ROOT" \
  --frames 1 \
  --print-render-receipt
```

This opens a window only briefly and prints a receipt. It is not a replacement
for the interactive editor test above.

## Deterministic No-Window Parity Proof

The focused proof map for the current editor loop is:

| Target | What it proves |
| --- | --- |
| `product_new_world_menu_action_tests` | New World draft edit-mode movement, paint, and create semantics |
| `product_ascii_map_smoke` | End-to-end New World draft receipt parity, including cursor paint create |
| `room_editor_input_tests` | Physical room-editor keyboard mappings, including `1`, `2`, and `R` |
| `product_room_editor_action_controller_tests` | Editor actions change cursor/tool/direction state and edit documents correctly |
| `product_window_input_frame_tests` | Live product mouse click path headlessly: creative click picks cursor and builds phantom preview without mutation |
| `product_controller_input_smoke` | No-window controller sample injection, player/creative mode toggle receipts, and creative editor routing |
| `product_editor_wall_direction_hotkey_smoke` | Wall direction hotkey persists distinct Up and Right wall geometry |
| `product_editor_combined_save_continue_smoke` | Direct floor+wall edits persist together through Save And Exit and Continue |
| `product_continued_room_movement_smoke` | Continued edited room uses restored collision for exploration |
| `product_room_visual_proof_smoke` | Continued edited room has deterministic headless PPM visual artifact |

The stable headless visual parity gate is:

```sh
cmake --build build --target product_room_visual_proof_smoke
ctest --test-dir build --output-on-failure -R '^product_room_visual_proof_smoke$'
```

That smoke runs the editor-loop spine without a window:

1. creates a custom draft through the New World flow;
2. enters gameplay;
3. opens Pause and Edit Room;
4. applies real editor input through the room editor action path:

```text
editor.input=editor.nudge_x_pos,editor.next_tool,editor.place
```

5. Save And Exit;
6. fresh starter boot on the same save root;
7. Continue;
8. decodes the saved authored room;
9. rebuilds product active-room, scene projection, primitive draw-list, and CPU
   room mesh proof;
10. writes a deterministic PPM visual proof artifact.

The direct floor/wall persistence proof uses:

```text
editor.input=editor.nudge_x_pos,editor.select_wall_tool,editor.place,editor.select_floor_tool,editor.place
```

The wall-direction hotkey persistence proof uses:

```text
editor.input=editor.nudge_x_pos,editor.select_wall_tool,editor.place,editor.rotate_wall_direction,editor.place
```

That second proof decodes the saved room and checks `edit_wall_1` is the
default Up edge while `edit_wall_2` is the rotated Right edge.

The focused controller mode parity gate is:

```sh
cmake --build build --target product_controller_input_smoke
ctest --test-dir build --output-on-failure -R '^product_controller_input_smoke$'
```

That smoke injects controller samples without SDL hardware or a real window:

```text
controller.input=left_stick_up
controller.input=mode_chord
controller.input=mode_chord,release,mode_chord
room_edit.start_active=true
controller.input=dpad_right
```

It proves:

- player-mode gameplay surface maps controller movement to `game.move_y`;
- `mode_chord` toggles `player -> creative` and records
  `controller_action_chord_consumed`;
- release plus another `mode_chord` toggles `creative -> player`;
- `room_edit.start_active=true` enters room editing in creative mode without a
  controller chord;
- creative room-editor surface maps controller d-pad movement to
  `editor.nudge_x` and moves the editor cursor;
- starter surface blocks the chord with
  `interaction_mode_surface_blocked`.

The focused creative mouse input-frame parity gate is:

```sh
cmake --build build --target product_window_input_frame_tests
ctest --test-dir build --output-on-failure -R '^product_window_input_frame_tests$'
```

That test calls `processProductWindowEditorMousePickPreview(...)` directly with
synthetic clicks. It proves:

- creative mode plus room editing ready moves the editor cursor, builds a
  visible phantom preview, and leaves real floor/wall/collision counts
  unchanged;
- player mode with the same click does not run the editor pick/preview path;
- room editing not ready does not mutate editor state;
- invalid click coordinates propagate the mouse-pick rejection without preview
  or geometry mutation.

The smoke emits:

```text
room_visual_proof_save_exit=true
room_visual_proof_reboot_starter=true
room_visual_proof_continue=true
room_visual_proof_artifact=true
room_visual_proof_artifact_path=<temp save root>/custom_dungeon_draft_continue_visual_proof.ppm
room_visual_proof_floor_pixels=<positive integer>
room_visual_proof_wall_pixels=<positive integer>
room_visual_proof_geometry_signature=<positive integer>
result=pass
```

The focused draft-control parity case is in `product_ascii_map_smoke` as
`ascii_map_custom_draft_cursor_paint_create`. It proves the New World draft
cursor path without using direct hidden room setup:

```text
frontend.select=new_world
frontend.execute=true
world.title=Cursor Draft
menu.next_tab=true
menu.input=down
menu.right=true
world.draft_move=right
world.draft_paint=#
world.create=true
```

This maps to the manual controls as:

- `menu.next_tab=true`: press `Tab`, entering draft edit mode.
- `menu.input=down`: press Down or `S` once.
- `menu.right=true` and `world.draft_move=right`: move right twice without
  repeating the same automation key in one control file.
- `world.draft_paint=#`: headless equivalent of pressing `1`.
- `world.create=true`: press `Enter` or `Space`.

`world.draft_cell=1,2,#` is still useful for direct model setup tests, but the
manual recipe and parity proof use cursor movement plus paint because that is
the user-facing flow.

The PPM path policy is stable within each smoke run: it is written under that
run's isolated temporary save root as:

```text
custom_dungeon_draft_continue_visual_proof.ppm
```

The exact temp root prefix is intentionally unique per run.

## Control File Example

If a reproducible control file is needed, use unique command keys. Repeating a
key in one control file is rejected as `duplicate_key`.

Create `/tmp/iggy3d-editor-loop-create-edit-save.in`:

```text
frontend.select=new_world
frontend.execute=true
world.title=Custom Draft
menu.next_tab=true
menu.input=down
menu.right=true
world.draft_move=right
world.draft_paint=#
world.create=true
system.pause=true
menu.down=true
pause.execute=true
editor.input=editor.nudge_x_pos,editor.select_wall_tool,editor.place,editor.rotate_wall_direction,editor.place
menu.back=true
pause.select=save_and_exit
menu.confirm=true
```

Run it headlessly:

```sh
build/iggy3d \
  --no-window \
  --save-root "$IGGY3D_EDITOR_SAVE_ROOT" \
  --automation-control /tmp/iggy3d-editor-loop-create-edit-save.in \
  --print-render-receipt
```

Then verify a fresh starter sees the save:

```sh
build/iggy3d \
  --no-window \
  --save-root "$IGGY3D_EDITOR_SAVE_ROOT" \
  --print-render-receipt
```

Create `/tmp/iggy3d-editor-loop-continue.in`:

```text
frontend.select=continue
frontend.execute=true
```

Continue headlessly:

```sh
build/iggy3d \
  --no-window \
  --save-root "$IGGY3D_EDITOR_SAVE_ROOT" \
  --automation-control /tmp/iggy3d-editor-loop-continue.in \
  --print-render-receipt
```

The equivalent window launch uses the same `--save-root`, but do not use
automation for an interactive manual run unless deliberately reproducing a
scripted setup:

```sh
build/iggy3d \
  --window \
  --renderer null \
  --input auto \
  --save-root "$IGGY3D_EDITOR_SAVE_ROOT" \
  --print-render-receipt
```

## Receipt Fields

After creating the world from the cursor-painted draft, expect:

```text
frontend_screen=gameplay
gameplay_active=true
world_setup_dungeon_draft_edit_mode=true
world_setup_dungeon_draft_modified=true
world_setup_dungeon_draft_cursor_row=1
world_setup_dungeon_draft_cursor_column=2
world_setup_dungeon_draft_status=dungeon_draft_cell_painted
world_setup_dungeon_draft_reason_code=dungeon_draft_cell_painted
world_setup_dungeon_draft_last_glyph=#
world_setup_ascii_room_id=custom_dungeon_draft
world_creation_ascii_room_id=custom_dungeon_draft
active_room_source=ascii_room
active_room_id=custom_dungeon_draft
active_room_authored_floor_count=58
active_room_authored_wall_count=61
```

After Save And Exit, expect the save path receipt to include:

```text
frontend_screen=starter
gameplay_active=false
interaction_mode=player
product_save_status=product_save_written
product_save_source=pause_save_and_exit
active_product_save_id=save_001
room_editor_hud_visible=false
room_editor_hud_tool=wall
room_editor_hud_wall_direction=right
room_editor_hud_last_operation=editor.place
room_editor_overlay_visible=false
```

After a fresh Continue, expect:

```text
frontend_screen=gameplay
gameplay_active=true
active_room_source=saved_authored_room
active_room_id=custom_dungeon_draft
active_room_authored_floor_count=58
active_room_authored_wall_count=63
room_editor_hud_visible=false
product_save_load_status=product_save_loaded
product_save_load_source=continue
product_vulkan_room_mesh_cpu_ready=true
product_vulkan_room_wall_draw_count=<positive integer>
```

After controller mode checks, expect:

```text
interaction_mode=player|creative
controller_mode_toggle_requested=true|false
controller_mode_toggle_accepted=true|false
controller_mode_toggle_status=interaction_mode_toggled|interaction_mode_surface_blocked|interaction_mode_chord_partial
controller_mode_toggle_reason_code=<same stable status>
controller_mode_toggle_surface=gameplay|room_editor|starter
controller_action_mapped=true|false
controller_action_status=controller_action_mapped|controller_action_chord_consumed
controller_action_reason_code=<same stable status>
controller_action_control=left_stick_up|dpad_right|none
controller_action_mode=player|creative
controller_action_surface=gameplay|room_editor|starter
controller_action_input_action=game.move_y|editor.nudge_x|none
```

After creative mouse preview checks, expect:

```text
interaction_mode=creative
room_editor_grid_x=<clicked grid x>
room_editor_grid_z=<clicked grid z>
room_editor_status=room_editor_mouse_pick_mapped
room_editor_reason_code=room_editor_mouse_pick_mapped
room_editor_last_operation=room_editor.mouse_pick
room_editor_last_operation_accepted=true
room_editor_preview_visible=true
room_editor_preview_status=room_editor_preview_ready
room_editor_preview_reason_code=room_editor_preview_ready
room_editor_preview_candidate_id=<candidate primitive id>
room_editor_preview_tool=floor|wall
room_editor_preview_grid_x=<clicked grid x>
room_editor_preview_grid_z=<clicked grid z>
room_editor_preview_optimized_draw_delta=<integer>
room_editor_preview_optimized_triangle_delta=<integer>
product_draw_room_editor_preview_count=1
product_render_bridge_room_editor_preview_count=1
```

Before explicit confirm, authored room and collision counts should match the
pre-click state. After explicit confirm, those counts should update through the
normal room-editing operation receipts. After cancel, they should remain
unchanged and preview counts should return to zero.

The visual proof smoke additionally ties the PPM artifact to:

```text
room_visual_proof_artifact=true
room_visual_proof_floor_pixels=<positive integer>
room_visual_proof_wall_pixels=<positive integer>
room_visual_proof_geometry_signature=<positive integer>
```

The PPM comments include:

```text
room_id=custom_dungeon_draft
active_room_source=saved_authored_room
authored_floor_count=58
authored_wall_count=62
optimized_wall_draw_count=22
room_geometry_signature=<positive integer>
```

## Honest Current Limits

- This recipe does not claim Vulkan screenshot proof. `vulkan_screenshot_smoke`
  remains the later display-dependent visual gate.
- The builder verification for this slice stays headless.
- ASCII remains map/layout authoring only.
- NPC behavior is outside this lane.
- `--input auto` only selects input backend. It does not create a dungeon or
  run the editor loop by itself.

## Verification Gate

For this recipe and receipt parity:

```sh
cmake --build build --target iggy3d_app
cmake --build build --target room_editor_input_tests
ctest --test-dir build --output-on-failure -R '^room_editor_input_tests$'
cmake --build build --target product_room_editor_action_controller_tests
ctest --test-dir build --output-on-failure -R '^product_room_editor_action_controller_tests$'
cmake --build build --target product_editor_wall_direction_hotkey_smoke
ctest --test-dir build --output-on-failure -R '^product_editor_wall_direction_hotkey_smoke$'
cmake --build build --target product_window_input_frame_tests
ctest --test-dir build --output-on-failure -R '^product_window_input_frame_tests$'
cmake --build build --target product_controller_input_smoke
ctest --test-dir build --output-on-failure -R '^product_controller_input_smoke$'
cmake --build build --target product_pause_save_smoke
ctest --test-dir build --output-on-failure -R '^product_pause_save_smoke$'
cmake --build build --target product_new_world_menu_action_tests
ctest --test-dir build --output-on-failure -R '^product_new_world_menu_action_tests$'
tools/check_branch_gate.py
git diff --check
```

`product_ascii_map_smoke`, `product_editor_combined_save_continue_smoke`,
`product_continued_room_movement_smoke`, and `product_room_visual_proof_smoke`
remain the broader end-to-end proof targets listed in the proof map above.

## Stop Rules

Stop and create a source slice instead of editing this recipe if any of these
become true:

- the manual flow cannot open New World, Edit Room, Save And Exit, or Continue;
- the receipt fields above disappear or change meaning;
- the visual proof artifact can only be produced by launching a window;
- fixing the recipe requires AppShell to own new behavior.
