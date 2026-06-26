# Dungeon Editor Loop Manual Recipe v0.1

## Objective

Give a practical user-facing recipe for the current dungeon editor loop without
guessing flags or receipt fields. This document connects the live window path
to the deterministic no-window proof that already covers:

- New World from a custom draft;
- Pause to Edit Room;
- real editor input for place/delete/undo/redo;
- Save And Exit;
- fresh starter boot;
- Continue into the saved edited room;
- headless PPM visual proof for the continued room.

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
   - `Q/E` or shoulders: cycle editor tool.
   - `Space` or gamepad south: place.
   - `Delete`: delete.
   - `Z`: undo.
   - `Y`: redo.
10. Place at least one wall or floor so the edited room changes.
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

The stable no-window parity gate is:

```sh
cmake --build build --target product_room_visual_proof_smoke
ctest --test-dir build --output-on-failure -R '^product_room_visual_proof_smoke$'
```

That smoke runs the same editor-loop spine without a window:

1. creates a custom draft through the New World flow;
2. enters gameplay;
3. opens Pause and Edit Room;
4. applies real editor input:

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
editor.input=editor.nudge_x_pos,editor.next_tool,editor.place
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
product_save_status=product_save_written
product_save_source=pause_save_and_exit
active_product_save_id=save_001
room_editor_hud_visible=false
room_editor_hud_tool=wall
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
active_room_authored_wall_count=62
room_editor_hud_visible=false
product_save_load_status=product_save_loaded
product_save_load_source=continue
product_vulkan_room_mesh_cpu_ready=true
product_vulkan_room_wall_draw_count=22
```

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
cmake --build build --target product_ascii_map_smoke
ctest --test-dir build --output-on-failure -R '^product_ascii_map_smoke$'
cmake --build build --target product_room_visual_proof_smoke
ctest --test-dir build --output-on-failure -R '^product_room_visual_proof_smoke$'
tools/check_branch_gate.py
git diff --check
```

`product_ascii_map_smoke` proves the New World draft controls and receipt
fields through `ascii_map_custom_draft_cursor_paint_create`.
`product_room_visual_proof_smoke` proves Save And Exit, fresh Continue, and the
headless PPM visual artifact for the edited room.

## Stop Rules

Stop and create a source slice instead of editing this recipe if any of these
become true:

- the manual flow cannot open New World, Edit Room, Save And Exit, or Continue;
- the receipt fields above disappear or change meaning;
- the visual proof artifact can only be produced by launching a window;
- fixing the recipe requires AppShell to own new behavior.
