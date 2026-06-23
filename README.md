# iggy3d

Standalone 3D runtime/gameplay repo for the `iggy3d` build.

Current status: product gameplay view in progress. The movement playground is
the current first-person test arena for movement, traversal, spells, debug
telemetry, room editing, and procedural bean model placeholders.

Startup script:

```sh
#!/usr/bin/env sh
set -eu

cd ~/iggy3d
cmake --build build --target iggy3d_app -j 8

./build/iggy3d \
  --window \
  --input auto \
  --save-root "$HOME/.iggy3d/saves" \
  --print-render-receipt
```

Use `--no-window --print-render-receipt` for routine receipt-only validation.
Windowed launches are for manual inspection only.

Normal interactive window launches open the starter menu first and suppress the
demo world until a save is created or loaded. Use W/S or Up/Down to move between
`Continue`, `New World`, `Load Save`, `Settings`, `Dev Tools`, and `Exit`; use
Enter/Space or controller Cross to execute. Save files are plain `.iggy3d.save`
files under `--save-root` and contain the runtime save envelope plus any authored
room floors/walls from the in-game editor. Use `--auto-new-world` only for
explicit scripted or developer launch paths that should enter gameplay without
manual starter-menu selection.

In gameplay, Esc opens the visible Pause Menu HUD: `Resume`, `Save`,
`Save And Exit`, `Load Save`, `Settings`, `Dev Tools`, `Return To Title`, and
`Exit Game`. F1 opens the in-game Dev Tools overlay from gameplay. On a
PS5/DualSense or SDL gamepad, Options opens the Pause Menu; pressing Options
again resumes from Pause, Dev Tools are reached through Pause Menu for now, and
Create+Options remains the hard-quit chord. Settings are app/frontend-owned in
this packet and are not persisted to a settings file.

In-game room editing is available from the current dev/editor controls. F2
opens the editor; use tools for select, floor placement, wall placement,
semantics, and delete. Authored floors and walls keep stable ids such as
`edit_floor_1` and `edit_wall_1`, can be selected, moved, resized, stretched,
rotated, height/thickness adjusted, deleted, undone, and redone. These edits are
saved into `.iggy3d.save` through the existing Save / Save And Exit flow.
Loading a save restores the authored room into the editor, resets selection,
continues id counters from the highest loaded suffix, and rebuilds the runtime
room/collision/traversal surfaces from the saved authored-room section.
Codex-control editor transform keys are frame-gated with
`editor.transform_frames` so scripted no-window runs do not mutate repeatedly.

For automated verification, prefer no-window/null-renderer receipt smokes and
frontend unit tests. Windowed Vulkan launches are for manual visual inspection,
not routine packet iteration.

Product View v1 source truth:

- `docs/product_view_v1.md`
- runtime/session remains gameplay truth;
- product draw/framing/feedback/bridge data is derived view proof;
- `ProductRenderBridgeFrame` is a product-owned renderer-safe handoff summary
  and is not consumed by Vulkan yet.

Start here:

- `docs/architecture.md`
- `docs/ownership.md`
- `docs/product_view_v1.md`
- `docs/acceptance_demo.md`
- `docs/roadmap.md`
- `docs/file_plans/INDEX.md`
- `docs/file_plans/PRIORITY.md`
- `docs/file_plans/COMPLETE_BUILD_SURFACE.md`

Hard rule: this repo must not include, link, or build against old
`/Users/kogaryu/iggy` code.
