# Menu Usefulness v1 Builder Packet

## Packet

**Name:** Menu Usefulness v1
**Repo:** `/Users/kogaryu/iggy3d`
**Branch:** `iggy3d-main`
**Mode:** Builder implementation packet after liaison review
**Hard verification preference:** no-window/null/receipt tests; expected `window_launch_count=0` for packet smokes.

This packet makes the starter, pause, settings, dev tools, save rows, and menu input paths useful rather than decorative. The goal is not visual polish. The goal is durable row behavior, shared input semantics, stable enabled/disabled reasons, real action wiring where safe, and receipt proof that gameplay does not receive input while a menu owns control.

Do not revert existing dirty work. Do not touch old `/Users/kogaryu/iggy`. Do not introduce JSON or JSON libraries. Receipts and any data files remain deterministic key-value/TOML-style text.

## Current Source Truth

Current frontend/application surfaces already exist:

- `/Users/kogaryu/iggy3d/src/app/frontend/FrontendState.hpp/.cpp`
  - Owns `FrontendScreen`, `FrontendAction`, starter/pause/dev overlay state, action order helpers, input ownership, and gameplay-blocking checks.
  - Current pause order is already `Resume`, `Save`, `Save And Exit`, `Load Save`, `Settings`, `Dev Tools`, `Return To Title`, `Exit Game`.
- `/Users/kogaryu/iggy3d/src/app/frontend/StarterScreen.hpp/.cpp`
  - Owns starter row order and Continue disabled when no compatible save exists.
- `/Users/kogaryu/iggy3d/src/app/frontend/SaveSlotModel.hpp/.cpp`
  - Owns save preview rows, compatibility state, corrupt counts, authored floor/wall counts, package/scenario/hash/tick previews.
- `/Users/kogaryu/iggy3d/src/app/frontend/SettingsMenu.hpp/.cpp`
  - Owns settings tabs and defaults for input backend, look sensitivity, invert look, controller sensitivity, camera, renderer/window, audio/accessibility/dev toggles.
- `/Users/kogaryu/iggy3d/src/app/frontend/DevToolsMenu.hpp/.cpp`
  - Owns dev tool categories and labels.
- `/Users/kogaryu/iggy3d/src/app/frontend/FrontendReceipt.hpp/.cpp`
  - Already emits frontend screen/action/status, selected save metadata, starter visibility, pause/settings/dev flags, gamepad system control fields, and `window_launch_count`.
- `/Users/kogaryu/iggy3d/src/app/input/GamepadSystemControls.hpp/.cpp`
  - Existing PS5/DualSense system mapping seam: Options toggles pause, Create+Options hard quit, Create+East/Circle toggles editor, direct dev overlay requires explicit opt-in.
- `/Users/kogaryu/iggy3d/apps/iggy3d/main.cpp`
- `/Users/kogaryu/iggy3d/src/app/iggy3d/**`
  - Integrates opening/starter, pause, settings, dev overlay, editor, save file store, Codex control parser, null renderer receipt smokes, and gameplay suppression while menus own input.
- Existing tests include frontend unit tests, gamepad system control tests, starter/save/settings tests, no-window opening/starter/in-game menu smokes.

Current gaps this packet must close:

- Menu navigation/action mapping is still spread through app code. Add a shared menu input/action spine so keyboard, PS5 controller, and Codex controls route through the same semantic actions.
- Every visible row needs an enabled/disabled state, disabled reason, command name, and receipt field. Continue has partial coverage; pause/settings/dev tools need complete row semantics.
- Settings tabs exist, but rows need useful behavior contracts, runtime-only vs persisted classification, and apply/restore/back proofs.
- Dev tools categories exist, but v1 needs explicit readout/action permissions by category and a direct receipt model.
- Starter screen save hub must keep proving no demo world auto-load unless New World/Continue/Load explicitly launches.
- Verification must avoid opening windows unless explicitly approved; all new smokes should use `--renderer null`, `--no-window`/default no-window, Codex control, and receipt assertions.

## Approved Files

Builder may edit these files:

- `/Users/kogaryu/iggy3d/apps/iggy3d/main.cpp`
- `/Users/kogaryu/iggy3d/src/app/iggy3d/**`
  - Route shared menu input actions, execute menu commands, update receipts, keep no-window smokes deterministic.
- `/Users/kogaryu/iggy3d/src/app/frontend/FrontendState.hpp`
- `/Users/kogaryu/iggy3d/src/app/frontend/FrontendState.cpp`
  - Add shared row/action state, priority owner names, disabled reason naming, and input ownership helpers.
- `/Users/kogaryu/iggy3d/src/app/frontend/MenuInput.hpp`
- `/Users/kogaryu/iggy3d/src/app/frontend/MenuInput.cpp`
  - New focused module for normalized menu input actions.
- `/Users/kogaryu/iggy3d/src/app/frontend/PauseMenu.hpp`
- `/Users/kogaryu/iggy3d/src/app/frontend/PauseMenu.cpp`
  - New focused module if cleaner than expanding `FrontendState`; owns pause row model and action enablement.
- `/Users/kogaryu/iggy3d/src/app/frontend/StarterScreen.hpp`
- `/Users/kogaryu/iggy3d/src/app/frontend/StarterScreen.cpp`
  - Harden starter/save-hub row enablement and disabled reasons.
- `/Users/kogaryu/iggy3d/src/app/frontend/SaveSlotModel.hpp`
- `/Users/kogaryu/iggy3d/src/app/frontend/SaveSlotModel.cpp`
  - Add row display metadata only if needed; do not change save truth.
- `/Users/kogaryu/iggy3d/src/app/frontend/SettingsMenu.hpp`
- `/Users/kogaryu/iggy3d/src/app/frontend/SettingsMenu.cpp`
  - Add settings row model, row actions, runtime-only/persisted classification, and defaults.
- `/Users/kogaryu/iggy3d/src/app/frontend/DevToolsMenu.hpp`
- `/Users/kogaryu/iggy3d/src/app/frontend/DevToolsMenu.cpp`
  - Add v1 category readouts/actions and, if needed, an `Input` category.
- `/Users/kogaryu/iggy3d/src/app/frontend/FrontendReceipt.hpp`
- `/Users/kogaryu/iggy3d/src/app/frontend/FrontendReceipt.cpp`
  - Add receipt fields listed below.
- `/Users/kogaryu/iggy3d/src/app/input/GamepadSystemControls.hpp`
- `/Users/kogaryu/iggy3d/src/app/input/GamepadSystemControls.cpp`
  - Extend only if shared menu navigation needs PS5 confirm/back/navigation state. Preserve existing system action behavior.
- `/Users/kogaryu/iggy3d/tests/unit/frontend_state_tests.cpp`
- `/Users/kogaryu/iggy3d/tests/unit/starter_screen_tests.cpp`
- `/Users/kogaryu/iggy3d/tests/unit/settings_menu_tests.cpp`
- `/Users/kogaryu/iggy3d/tests/unit/save_slot_model_tests.cpp`
- `/Users/kogaryu/iggy3d/tests/unit/gamepad_system_controls_tests.cpp`
- `/Users/kogaryu/iggy3d/tests/unit/menu_input_tests.cpp`
- `/Users/kogaryu/iggy3d/tests/unit/pause_menu_tests.cpp`
- `/Users/kogaryu/iggy3d/tests/unit/dev_tools_menu_tests.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_ingame_menu_smoke.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_starter_screen_smoke.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_opening_menu_smoke.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_menu_usefulness_smoke.cpp`
- `/Users/kogaryu/iggy3d/cmake/iggy3d_tests.cmake`
- `/Users/kogaryu/iggy3d/README.md` only for user-facing control/menu documentation.

May edit only if a test proves a direct save/menu seam is missing:

- `/Users/kogaryu/iggy3d/src/runtime/save/SaveFileStore.hpp`
- `/Users/kogaryu/iggy3d/src/runtime/save/SaveFileStore.cpp`

## Forbidden Files and Surfaces

- No runtime gameplay, combat, movement, session, save codec, replay, projection, renderer, Vulkan, shader, screenshot, frame-hash, fixture schema, or package schema changes unless explicitly required by a failing approved test and called out in Builder's final report.
- No old `/Users/kogaryu/iggy` dependency.
- No JSON, JSON libraries, or JSON-like receipt formats.
- No repeated app window testing. Do not add window-opening smokes for this packet.
- Do not remove or weaken starter-world suppression.
- Do not regress editor/input priority already present in current app code.

## Data Ownership

- Frontend menu modules own menu row lists, selected row/category/tab, enabled/disabled state, disabled reason, command names, and UI/navigation semantics.
- Visual app owns orchestration: reading keyboard/gamepad/Codex input, selecting the current input owner, invoking menu actions, save/load commands, and emitting receipts.
- `SaveFileStore` owns save file discovery, read/write/delete, and save metadata. Menus display save previews and request actions; they do not parse save files directly.
- Runtime session owns gameplay state. Menus may request save/load/reset/return-to-title, but menu models never mutate runtime state directly.
- Settings v1 is app/frontend-owned runtime state. No persistent settings file is required for green. If Builder adds persistence, it must be non-JSON TOML/key-value, app-owned, and separately reported.
- Dev tools v1 is mostly read-only. Allowed commands are explicitly listed below. Dev tools must not become a second gameplay authority.
- Renderer/HUD displays menu/dev readouts only; it does not own menu truth.

## Shared Menu Input Spine

Add normalized semantic menu input actions, preferably in `src/app/frontend/MenuInput.*`:

```text
menu_input=up|down|left|right|confirm|back|next_tab|previous_tab|none
menu_input_source=keyboard|gamepad|codex|scripted|none
menu_owner=starter|pause|settings|dev_tools|editor|gameplay|none
```

Keyboard mapping:

- Up: `W` or Arrow Up
- Down: `S` or Arrow Down
- Left: `A` or Arrow Left
- Right: `D` or Arrow Right
- Confirm: Enter or Space
- Back: Esc
- Next/previous tab: Tab / Shift+Tab where available
- F1 may open dev overlay only if developer tools are enabled and no higher-priority owner blocks it.

PS5/DualSense semantics:

- D-pad or left stick: up/down/left/right menu navigation
- Cross: confirm
- Circle: back
- Options: pause when gameplay owns input
- Create+Options: hard quit
- Create+Circle/East: editor toggle only when editor is enabled and higher-priority menu does not own input
- Direct controller dev overlay shortcut remains deferred unless current `GamepadSystemControls` explicit opt-in is enabled.

Codex-control semantics:

Keep current keys stable and add shared aliases where useful:

```text
menu.owner=starter|pause|settings|dev_tools
menu.input=up|down|left|right|confirm|back|next_tab|previous_tab
menu.input_frames=<csv-frames>
pause.open=true|false
pause.select=<action>
pause.next=true
pause.previous=true
pause.execute=true
settings.tab=<tab>
settings.apply=true
settings.restore_defaults=true
settings.back=true
dev_tools.category=<category>
opening_menu.select=<action>
opening_menu.execute=true
```

Input priority stack:

1. Starter/opening menu
2. Pause menu
3. Settings menu
4. Dev tools overlay/menu
5. Editor
6. Gameplay

Only the active owner consumes menu actions. When a menu owns input, gameplay movement/look/attack/interact must be suppressed and receipts must show `gameplay_input_suppressed=true`.

## Pause Menu Rows

Pause menu row order is fixed:

1. Resume
2. Save
3. Save And Exit
4. Load Save
5. Settings
6. Dev Tools
7. Return To Title
8. Exit Game

For every row provide:

```text
pause_row.<index>.action=<action>
pause_row.<index>.enabled=true|false
pause_row.<index>.disabled_reason=<reason-or-none>
pause_row.<index>.command=<command-name>
```

Minimum row semantics:

- Resume: enabled when pause is open. Command closes pause and returns to gameplay.
- Save: enabled when a runtime session exists and save root is writable. Command writes current save, including authored room if present.
- Save And Exit: enabled when Save is enabled. Command writes save then requests process exit; no gameplay command record.
- Load Save: enabled when at least one compatible save exists. Command opens/uses load-save child flow; if no compatible save, disabled reason `no_compatible_save`.
- Settings: enabled. Command opens settings with parent `pause`.
- Dev Tools: enabled only when developer tools are enabled. Command opens dev overlay with parent `pause`; disabled reason `developer_tools_disabled`.
- Return To Title: enabled. Command closes runtime gameplay path, returns to starter, does not auto-load a demo world.
- Exit Game: enabled. Command requests exit; no implicit save unless Save And Exit was selected.

Required pause receipts:

```text
frontend_screen=pause|settings|dev_overlay|starter|gameplay
menu_owner=pause|settings|dev_tools|starter|gameplay
pause_menu_open=true|false
pause_selected_action=<action>
pause_selected_enabled=true|false
pause_selected_disabled_reason=<reason-or-none>
pause_action_command=<command-name>
pause_action_executed=true|false
pause_action_status=<status>
pause_row_count=8
pause_enabled_row_count=<integer>
gameplay_input_suppressed=true|false
opening_menu_saved_current=true|false
opening_menu_saved_and_exit=true|false
frontend_return_to_title_requested=true|false
opening_menu_exit_requested=true|false
window_launch_count=0
```

## Starter Screen as Save Hub

Starter row order:

1. Continue
2. New World
3. Existing Saves / Load Save
4. Delete Save
5. Settings
6. Dev Tools
7. Exit

If retaining current visible labels, `Load Save` may remain the row label but the model must expose the save list as Existing Saves behavior. Continue remains disabled if no compatible save exists.

Starter semantics:

- Starter owns input until explicit launch request.
- No demo world auto-loads while starter is open.
- Continue loads most recent compatible save.
- New World creates a new `.iggy3d.save`, then launches runtime.
- Existing Saves/Load Save displays rows and metadata.
- Delete Save requires a confirmation state; direct delete without confirmation is not allowed for menu input. Existing Codex smoke can use explicit delete-confirm control if needed.
- Settings opens settings with parent `starter`.
- Dev Tools opens starter dev tools/readout with parent `starter`.
- Exit opens exit confirmation or requests exit with explicit receipt.

Required starter/save receipts:

```text
frontend_screen=starter|new_world|load_save|settings|starter_dev_tools|exit_confirm
starter_world_suppressed=true
starter_header_visible=true
starter_action_list_visible=true
starter_detail_panel_visible=true
starter_status_strip_visible=true
starter_row_count=<integer>
starter_selected_action=<action>
starter_selected_enabled=true|false
starter_selected_disabled_reason=<reason-or-none>
frontend_launch_requested=true|false
selected_save_id=<id|none>
selected_package_id=<id|none>
selected_scenario_id=<id|none>
selected_save_tick=<integer>
selected_save_hash=<hash|none>
selected_save_authored_floor_count=<integer>
selected_save_authored_wall_count=<integer>
selected_save_compatible=true|false|unavailable
save_count=<integer>
compatible_save_count=<integer>
corrupt_save_count=<integer>
window_launch_count=0
```

## Settings Menu

Settings tab order is fixed:

1. Input
2. Controls
3. Camera
4. Gameplay
5. Video/Display
6. Audio
7. Accessibility
8. Developer

Rows and v1 behavior:

- Input: `input_backend=keyboard|gamepad|auto|scripted`; runtime-only.
- Controls: `look_sensitivity`, `invert_look`, `controller_look_sensitivity`; runtime-only.
- Camera: `camera_mode=first_person|third_person|tactical`; runtime-only. Do not force tactical gameplay implementation here.
- Gameplay: `difficulty=normal`; read-only v1 unless code already supports more.
- Video/Display: `renderer=auto|null|vulkan`, `window_mode=no_window|window`; changing these is pending-only unless safe to apply without restart. Do not launch a window during packet verification.
- Audio: `master_volume`; row visible even if audio unavailable; disabled reason `audio_unavailable`.
- Accessibility: `high_contrast`, `reduced_motion`; runtime-only UI flags.
- Developer: `dev_tools_enabled`, `debug_overlay_enabled`; runtime-only.

Actions:

- Apply: copies draft settings to active frontend settings.
- Restore Defaults: resets draft and active settings to defaults only when confirmed by action; default input backend returns to keyboard.
- Back: returns to parent (`starter` or `pause`) without launching gameplay.

No persistent settings file is required for v1. If Builder chooses persistence, it must be app-owned, non-JSON, and optional; default build/tests must not depend on it.

Required settings receipts:

```text
settings_open=true|false
settings_parent=starter|pause|none
settings_tab=<tab>
settings_selected_row=<row>
settings_selected_enabled=true|false
settings_selected_disabled_reason=<reason-or-none>
settings_apply_requested=true|false
settings_restore_defaults_requested=true|false
settings_back_requested=true|false
settings_input_backend=keyboard|gamepad|auto|scripted
settings_look_sensitivity=<float>
settings_invert_look=true|false
settings_controller_look_sensitivity=<float>
settings_renderer=auto|null|vulkan
settings_window_mode=no_window|window
settings_audio_available=true|false
settings_accessibility_high_contrast=true|false
settings_accessibility_reduced_motion=true|false
settings_developer_tools_enabled=true|false
settings_persistence=deferred|runtime_only|saved
window_launch_count=0
```

## Dev Tools Menu

Dev tools v1 is an access shell and readout surface. It must be reachable from starter and pause, not forced on launch.

Category order:

1. Session
2. Input
3. Player
4. Movement
5. World Editor
6. Collision
7. Spells
8. Camera
9. Renderer
10. Performance

If adding `Input` is too invasive for the existing enum, Builder may keep the old order and represent input under Session for this packet, but must report that limitation.

V1 permissions:

- Session: read-only current lifecycle, tick, save id/path, package/scenario, state hash. Allowed command: none.
- Input: read-only active input backend, gamepad availability/name, menu owner, last menu action. Allowed command: none.
- Player: read-only player position, camera mode, combat/objective status. Allowed command: none.
- Movement: read-only movement/traversal state. Allowed command: none.
- World Editor: read-only selected editor primitive/counts if editor exists. Allowed command: open editor only if current app already supports it safely.
- Collision: read-only runtime surface count and traversal slot count.
- Spells: read-only ability/spell state. No new gameplay commands.
- Camera: read-only mode/yaw/pitch/FOV fields available in app.
- Renderer: read-only backend, frame count, draw count, window mode, diagnostics dir.
- Performance: read-only frame budget/frame count; no profiler dependency.

Required dev tools receipts:

```text
dev_tools_open=true|false
dev_tools_parent=starter|pause|direct|none
dev_tools_category=<category>
dev_tools_input_blocking=true
dev_tools_readout_visible=true
dev_tools_selected_action=<action-or-none>
dev_tools_selected_enabled=true|false
dev_tools_selected_disabled_reason=<reason-or-none>
dev_tools_command_status=<status>
dev_tools_runtime_readout_count=<integer>
window_launch_count=0
```

## Persistence and Save/Load Interaction

- Save rows display `SaveSlotPreview`; do not parse save files in UI code outside `SaveSlotModel`/`SaveFileStore` paths.
- Save, Save And Exit, Continue, Load Save, New World, Delete Save must all produce receipts showing selected save id, status, and whether a runtime launch/return/exit was requested.
- Authored room/player/runtime save truth remains in `.iggy3d.save` through existing save flow.
- Settings are runtime-only for v1 unless Builder adds a clearly scoped non-JSON app settings file. Do not make persistent settings a blocker.

## Implementation Phases

1. **Model hardening**
   - Add row models for starter/pause/settings/dev tools.
   - Add enabled/disabled state, disabled reason, and command name.
   - Add unit tests for row order and disabled reasons.

2. **Shared input spine**
   - Add `MenuInput` semantic actions and owner priority selection.
   - Route keyboard, PS5/controller, and Codex controls through this shared action model.
   - Preserve existing direct Codex keys for compatibility.

3. **Action execution**
   - Wire pause/starter/settings/dev tools actions to existing app/save/frontend functions.
   - Keep runtime mutation behind existing save/load/session/editor APIs.
   - Ensure menu-owned input suppresses gameplay input.

4. **Receipts**
   - Extend `FrontendReceipt` and app receipt fields.
   - Every selected row must report enabled/disabled reason/action command/status.
   - Every smoke must assert `window_launch_count=0`.

5. **Focused tests and docs**
   - Add/update unit tests and no-window smokes.
   - Update README only for user-facing controls and menu paths.

## Tests to Add or Update

Unit tests:

- `menu_input_tests.cpp`
  - keyboard up/down/left/right/confirm/back/tab normalize correctly.
  - PS5/DualSense Cross/Circle/D-pad/Options/Create+Options normalize or route correctly.
  - owner priority chooses starter over pause, pause over settings/dev/editor/gameplay, settings over gameplay.
- `pause_menu_tests.cpp`
  - row order exactly matches packet.
  - Save/Load disabled reasons are deterministic.
  - Dev Tools disabled when developer tools disabled.
- `frontend_state_tests.cpp`
  - `frontendBlocksGameplayInput` remains true for starter/pause/settings/dev tools and false for gameplay.
  - return-to-title clears runtime launch ownership and returns to starter.
- `starter_screen_tests.cpp`
  - Continue disabled without compatible save, enabled with compatible save.
  - selected row metadata and disabled reason are stable.
- `settings_menu_tests.cpp`
  - tab order, row defaults, apply, restore defaults, back parent behavior.
  - runtime-only classification for v1 settings.
- `dev_tools_menu_tests.cpp`
  - category order, read-only/action permissions, readout visibility.
- `gamepad_system_controls_tests.cpp`
  - preserve Options pause, Create+Options quit, Create+East editor, no repeated held action.

Smoke tests:

- Extend `package_visual_ingame_menu_smoke.cpp` or add `package_visual_menu_usefulness_smoke.cpp`.
- Prefer product app no-window receipts:
  `./build/iggy3d --no-window --print-render-receipt` and
  `./build/iggy3d --no-window --scripted-gameplay-smoke --print-render-receipt`.
- Compatibility/Test Shell package visual smokes may still run this until the
  source/test migration replaces those smoke names and flags:
  ```sh
  ./build/iggy3d_visual_demo --renderer null --interactive --frames 1 --no-opening-menu --codex-control ... --print-render-receipt
  ```
- Cases:
  - Pause blocks movement/attack/look input and reports row metadata.
  - Pause Save writes current save.
  - Pause Save And Exit writes save and requests exit.
  - Pause Load Save disabled reason with no compatible save.
  - Pause Settings opens settings and applies input/backend/look changes.
  - Restore Defaults resets settings.
  - Dev Tools opens from pause and reports category/readout.
  - Return To Title returns to starter with no auto world reload.
  - Starter Continue disabled with no save.
  - Starter New World creates save and launches only after explicit action.
  - Load Save selected row reports metadata.
  - Delete Save requires confirmation.

## Verification Commands

Run from `/Users/kogaryu/iggy3d`.

Default no-window/focused lane:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure -R 'menu_input|pause_menu|frontend|starter|settings|dev_tools|gamepad|save_slot|opening|ingame_menu|menu_usefulness'
```

Headless/replay stability:

```sh
./build/iggy3d_headless_demo \
  --package fixtures/demos/first_room/package.iggy3d.toml \
  --summary fixtures/demos/first_room/expected_summary.txt \
  --save /tmp/iggy3d_menu_usefulness_runtime.save

./build/iggy3d_replay_tool \
  --package fixtures/demos/first_room/package.iggy3d.toml \
  --save /tmp/iggy3d_menu_usefulness_runtime.save \
  --expect-summary fixtures/demos/first_room/expected_summary.txt
```

Warnings-as-errors focused lane:

```sh
cmake -S . -B build-werror -DIGGY3D_WARNINGS_AS_ERRORS=ON
cmake --build build-werror
ctest --test-dir build-werror --output-on-failure -R 'menu_input|pause_menu|frontend|starter|settings|dev_tools|gamepad|save_slot|opening|ingame_menu|menu_usefulness'
rm -rf build-werror
```

No-go scans:

```sh
rg -n '#include[ <"](SDL3/|SDL\.h|SDL_vulkan|vulkan/)|\bVk[A-Z][A-Za-z0-9_]*|\bVK_[A-Z0-9_]+' src/runtime src/content src/projection src/runtime/save

rg -n '/Users/kogaryu/iggy|namespace runtime3d|iggy::three_d|nlohmann|rapidjson|\.json\b|JSON|Json|StatusCode' apps src tests cmake CMakeLists.txt

git diff --check
```

Do not run a real app window command for green unless the lead explicitly approves it. New packet smokes must report `window_launch_count=0`.

## No-Go List

- No decorative-only menu rows. Every visible row needs enabled/disabled reason, action command, receipt, and at least one test.
- No per-menu bespoke input handling if shared `MenuInput` can own it.
- No gameplay input reaching runtime while starter/pause/settings/dev tools owns input.
- No new save format for settings in v1 unless explicitly non-JSON and optional.
- No renderer/Vulkan/window/screenshot work.
- No fixture/schema churn.
- No multiplayer or split-screen menu design.
- No old repo dependency.

## Expected Builder Dex Final Report

1. Files changed.
2. Row models added/updated by menu surface.
3. Shared input action spine summary and PS5/controller mapping proof.
4. Pause row actions: enabled/disabled reasons and command results.
5. Starter save hub behavior and no-world-at-menu proof.
6. Settings rows, runtime-only/persistence decision, apply/restore/back proof.
7. Dev tools category/readout/action permission proof.
8. Receipt fields added with sample no-window receipt excerpts.
9. Tests and exact commands run.
10. Confirmation: `window_launch_count=0` for new smokes.
11. Remaining blockers, or `No remaining blocker in Menu Usefulness v1`.
