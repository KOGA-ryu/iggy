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
2. Enter the New World flow and create the custom draft/world.
3. Enter gameplay.
4. Open Pause.
5. Select `Edit Room`.
6. Use editor controls:
   - `W/A/S/D` or d-pad: move editor cursor.
   - `Q/E` or shoulders: cycle editor tool.
   - `Space` or gamepad south: place.
   - `Delete`: delete.
   - `Z`: undo.
   - `Y`: redo.
7. Place at least one wall or floor so the edited room changes.
8. Open Pause again.
9. Select `Save And Exit`.
10. Relaunch with the same `--save-root`.
11. Select `Continue`.

Expected user-visible state after Continue:

- gameplay resumes instead of opening the editor automatically;
- the restored room is the saved edited room;
- the room editor HUD is not visible until Edit Room is opened again;
- when Edit Room is opened again, the editor HUD and cursor/tool overlay should
  be visible.

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

1. creates a custom draft;
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
world.draft_cell=1,2,#
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
cmake --build build --target product_room_visual_proof_smoke
ctest --test-dir build --output-on-failure -R '^product_room_visual_proof_smoke$'
tools/check_branch_gate.py
git diff --check
```

Run `product_ascii_map_smoke` only when changing the broader ASCII map smoke:

```sh
cmake --build build --target product_ascii_map_smoke
ctest --test-dir build --output-on-failure -R '^product_ascii_map_smoke$'
```

## Stop Rules

Stop and create a source slice instead of editing this recipe if any of these
become true:

- the manual flow cannot open New World, Edit Room, Save And Exit, or Continue;
- the receipt fields above disappear or change meaning;
- the visual proof artifact can only be produced by launching a window;
- fixing the recipe requires AppShell to own new behavior.
