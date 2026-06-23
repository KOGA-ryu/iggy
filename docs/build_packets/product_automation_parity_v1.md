# Product Automation Parity v1

## Objective

Implement the first product-owned automation intake for `./build/iggy3d` so
no-window product smokes can drive starter, pause, settings, and dev-tools menu
flows through `--automation-control` / `--script-control`.

This packet is frontend/menu parity only. It must not port the old visual
`--codex-control` surface and must not add editor or gameplay automation.

## Current Source Truth

- `apps/iggy3d/main.cpp` runs the product app through
  `iggy3d::runProductApp`.
- `ProductAppOptions` already parses both `--automation-control <path>` and
  `--script-control <path>` into `ProductAppOptions::automationControlPath`.
- `AppShell.cpp` does not consume `automationControlPath` yet.
- `AppShell.cpp` already owns product starter/pause/settings/dev-tools routing:
  - `routeOpeningMenuInput`
  - `applyOpeningMenuAction`
  - `nextStarterSelection`
  - `nextPauseSelection`
  - `nextSettingsSelection`
  - `nextDevToolsSelection`
- Product frontend receipt proof already includes:
  - `frontend_screen`
  - `frontend_child_screen`
  - `frontend_selected_action`
  - `frontend_status`
  - `pause_menu_open`
  - `dev_tools_open`
  - `dev_tools_category`
  - `settings_selected_tab`
  - `input_owner`
  - `input_action_last`
  - `input_action_accepted`
  - `gameplay_input_suppressed`
  - `product_transition_*`
- `MenuInput.*` owns neutral menu owner names.
- `InputAction.*` owns product input action names such as
  `menu.down`, `menu.confirm`, `system.pause`, and `dev.toggle`.
- `product_menu_usefulness_smoke` proves product starter and scripted gameplay
  no-window receipts without automation intake.

## Scope

Add product automation intake for frontend/menu actions only:

- load one key-value text file from `ProductAppOptions::automationControlPath`;
- parse deterministic line-oriented controls;
- apply controls through existing product frontend/menu transition functions;
- emit honest automation receipt fields;
- add no-window product smoke proof.

The control file format is deterministic key-value text:

```text
key=value
key=value
```

Blank lines may be ignored. Comments are not required for v1. Duplicate keys
should be rejected unless a key is explicitly list-like in this packet. v1 does
not require list-like keys.

## First Automation Scope

Supported owners:

```text
automation.owner=starter|pause|settings|dev_tools
```

Supported menu actions:

```text
menu.input=up|down|left|right|confirm|back|next_tab|previous_tab|none
menu.up=true
menu.down=true
menu.left=true
menu.right=true
menu.confirm=true
menu.back=true
menu.next_tab=true
menu.previous_tab=true
```

`menu.input=<action>` is the canonical compact form. Boolean forms are
compatibility aliases for simple smoke files. If multiple action keys are
present in one file, the implementation may either apply them in file order or
reject the file with `automation_control_ambiguous`; choose one behavior and
test it. Preferred v1 behavior: apply in file order by reading the file into an
ordered command list.

Supported direct selection controls:

```text
frontend.select=continue|new_world|load_save|settings|dev_tools|exit|resume|save|save_and_exit|return_to_title|exit_game
pause.select=resume|save|save_and_exit|load_save|settings|dev_tools|return_to_title|exit_game
settings.tab=input|controls|camera|gameplay|video_display|audio|accessibility|developer
dev_tools.category=session|input|player|movement|world_editor|collision|spells|camera|renderer|performance
```

Supported execution controls:

```text
frontend.execute=true|false
pause.execute=true|false
settings.apply=true|false
settings.restore_defaults=true|false
settings.back=true|false
dev_tools.execute=true|false
```

Supported product transition controls:

```text
frontend.return_to_title=true|false
system.pause=true|false
system.back=true|false
system.quit=true|false
```

`system.pause=true` should map to `InputAction::SystemPause`.
`system.back=true` should map to `InputAction::MenuBack`.
`system.quit=true` may request app close only through existing product close
state; it must not write save data or mutate gameplay.

## Unsupported In v1

Do not implement these in this packet:

- `--codex-control`;
- editor keys such as `editor.open`, `editor.apply`, `editor.cursor`;
- gameplay keys such as `move.forward`, `attack`, `look.yaw_delta`;
- save-file mutation controls beyond existing product menu actions;
- fixture/package schema controls;
- renderer/Vulkan controls;
- multi-frame automation such as `menu.input_frames`;
- old visual `--opening-menu`, `--no-opening-menu`, or `--dev-menu` CLI
  compatibility.

If a smoke needs any unsupported behavior, stop and split a later packet.

## Ownership

- `ProductAppOptions` continues to parse the automation path only.
- `AppShell` owns loading, parsing, applying, and reporting product automation.
- Product automation translates keys into existing `InputAction` /
  `FrontendAction` / `FrontendSettingsTab` / `FrontendDevToolsCategory` values.
- `AppShell` applies automation before or inside product frontend transition
  handling by reusing existing transition functions.
- Runtime/session mutation remains possible only through existing product
  transition/gameplay functions such as `launchProductNewWorld`. Automation
  must not directly mutate runtime state.
- Save files remain owned by `SaveBridge` / `SaveFileStore`; automation does not
  parse or write save files directly.
- Renderer/Vulkan/package visual shell does not participate.

## Approved Files For Implementation

Implementation may edit:

- `src/app/iggy3d/AppShell.cpp`
- `src/app/iggy3d/ProductAppOptions.hpp`
- `src/app/iggy3d/ProductAppOptions.cpp`
- `src/app/iggy3d/ReceiptBuilder.hpp`
- `src/app/iggy3d/ReceiptBuilder.cpp`
- `tests/smoke/product_menu_transition_smoke.cpp`
- new `tests/smoke/product_automation_menu_smoke.cpp`
- `cmake/iggy3d_tests.cmake`

Optional only if the implementation would otherwise bloat `AppShell.cpp`:

- new `src/app/iggy3d/ProductAutomationControl.hpp`
- new `src/app/iggy3d/ProductAutomationControl.cpp`
- new `tests/unit/product_automation_control_tests.cpp`
- `CMakeLists.txt` only to register the optional product automation source

If a new source file is added, register it in `CMakeLists.txt`.

## No-Go Files And Surfaces

Do not edit:

- renderer/Vulkan source;
- `docs/vulkan/**`;
- runtime/session/save gameplay logic;
- fixture/package schemas;
- package-room visual smokes;
- visual renderer smokes;
- install/package visual shell rules.

Do not remove, rename, or broadly change `iggy3d_visual_demo`.

Do not introduce JSON or JSON libraries.

Do not add a broad product port of visual `--codex-control`.

## Automation Result Semantics

Recommended internal status strings:

```text
not_requested
loaded
applied
parse_error
read_failed
duplicate_key
unknown_key
invalid_value
unsupported_action
ambiguous_action
owner_unavailable
```

Stable reason codes for smoke failures should be lower snake case, for example:

```text
product_automation_menu_pass
product_automation_menu_failed
product_app_unavailable
```

## Receipt Fields

Add receipt fields only when they are truthful:

```text
automation_control_requested=true|false
automation_control_loaded=true|false
automation_control_path=<path-or-empty>
automation_control_status=not_requested|loaded|applied|parse_error|read_failed|duplicate_key|unknown_key|invalid_value|unsupported_action|ambiguous_action|owner_unavailable
automation_control_scope=frontend_menu|none
automation_control_line_count=<integer>
automation_control_applied_count=<integer>
automation_control_last_key=<key-or-none>
automation_control_last_action=<action-or-none>
automation_control_last_owner=starter|pause|settings|dev_tools|gameplay|none
automation_control_last_result=applied|ignored|failed|none
```

Reuse existing product receipt fields to prove behavior:

```text
frontend_screen=<screen>
frontend_child_screen=<screen>
frontend_selected_action=<action>
frontend_status=<status>
pause_menu_open=true|false
dev_tools_open=true|false
dev_tools_category=<category>
settings_selected_tab=<tab>
input_owner=<owner>
input_action_last=<action>
input_action_accepted=true|false
gameplay_input_suppressed=true|false
product_transition_last_action=<action>
product_transition_status=<status>
product_transition_returned_to_gameplay=true|false
product_transition_returned_to_title=true|false
product_transition_session_preserved=true|false
```

Do not add Codex-branded receipt fields.

## Implementation Notes

Recommended shape:

1. Add a small product automation parse helper that reads the file into ordered
   commands.
2. Convert command values into existing enums:
   - `InputAction`
   - `FrontendAction`
   - `FrontendSettingsTab`
   - `FrontendDevToolsCategory`
3. In `runProductApp`, after starter initialization and before optional window
   handling, apply the automation once.
4. When an automation command represents menu navigation, call
   `routeOpeningMenuInput`.
5. When an automation command represents a direct selected row/category/tab,
   set the same frontend fields that the current product app already changes
   for keyboard/mouse/gamepad interaction.
6. For confirm/back/system actions, reuse `applyOpeningMenuAction` through
   `routeOpeningMenuInput` instead of duplicating transition logic.
7. Update `ProductAppWindowState` or a narrow automation state member so
   `ReceiptBuilder` can report the automation fields.

No-window mode still matters. `runOpeningMenuWindow` currently returns early
when `window.requested` is false, so automation must be applied outside that
window-only event loop.

## Tests

Add `tests/smoke/product_automation_menu_smoke.cpp`.

It should:

1. Use `IGGY3D_PRODUCT_APP_PATH`.
2. Write key-value control files under `/tmp`.
3. Run only no-window product commands:

```sh
./build/iggy3d --no-window --automation-control <path> --print-render-receipt
```

4. Parse key-value receipts and reject duplicate fields.
5. Assert `app=iggy3d` and never accept `app=iggy3d_visual_demo`.
6. Print `window_launch_count=0` from the smoke binary.
7. Return `77` only if product app path is unavailable.

Minimum smoke cases:

### Starter Settings Case

Control file:

```text
frontend.select=settings
frontend.execute=true
settings.tab=audio
```

Expected receipt:

```text
automation_control_loaded=true
automation_control_status=applied
automation_control_scope=frontend_menu
frontend_screen=starter
frontend_child_screen=settings
frontend_selected_action=settings
settings_selected_tab=audio
input_owner=settings
gameplay_input_suppressed=true
result=pass
```

### Starter Dev Tools Case

Control file:

```text
frontend.select=dev_tools
frontend.execute=true
dev_tools.category=input
```

Expected receipt:

```text
frontend_screen=starter
frontend_child_screen=starter_dev_tools
dev_tools_open=true
dev_tools_category=input
input_owner=dev_tools
gameplay_input_suppressed=true
```

### New World Launch Case

Control file:

```text
frontend.select=new_world
frontend.execute=true
```

Expected receipt:

```text
frontend_screen=gameplay
frontend_selected_action=create_and_enter
frontend_launch_requested=true
gameplay_active=true
product_transition_last_action=launch_gameplay
product_transition_status=gameplay_active
```

### Pause From Gameplay Case

Command:

```sh
./build/iggy3d --no-window --auto-new-world --automation-control <path> --print-render-receipt
```

Control file:

```text
system.pause=true
```

Expected receipt:

```text
frontend_screen=pause
pause_menu_open=true
input_owner=pause
input_action_last=system.pause
input_action_accepted=true
gameplay_input_suppressed=true
```

### Return To Title Case

Command:

```sh
./build/iggy3d --no-window --auto-new-world --automation-control <path> --print-render-receipt
```

Control file:

```text
system.pause=true
pause.select=return_to_title
pause.execute=true
```

Expected receipt:

```text
frontend_screen=starter
frontend_return_to_title_requested=true
product_transition_returned_to_title=true
gameplay_active=false
```

### Parse Error Case

Control file:

```text
menu.input=teleport
```

Expected receipt:

```text
automation_control_loaded=false
automation_control_status=invalid_value
automation_control_scope=frontend_menu
result=pass
```

The app may still exit `0` for parse errors if the receipt honestly reports
the parse failure. The smoke should assert the receipt, not require process
failure, unless implementation deliberately chooses nonzero for invalid
automation.

## CMake Registration

In `cmake/iggy3d_tests.cmake`:

- add `product_automation_menu_smoke`;
- link against `iggy3d`;
- apply warnings;
- define `IGGY3D_PRODUCT_APP_PATH="$<TARGET_FILE:iggy3d_app>"`;
- add dependency on `iggy3d_app`;
- register CTest with working directory `${CMAKE_CURRENT_SOURCE_DIR}`;
- set `SKIP_RETURN_CODE 77`;
- labels:

```text
smoke;product;frontend;menu;automation;no_window;iggy3d
```

## Acceptance Commands

Run from `/Users/kogaryu/iggy3d`:

```sh
cmake --build build --target iggy3d_app
cmake --build build --target product_menu_usefulness_smoke
cmake --build build --target product_automation_menu_smoke
ctest --test-dir build --output-on-failure -R '^product_(menu_usefulness|automation_menu)_smoke$'
git diff --check
```

Do not run a broad CTest loop for this packet.

Do not run any command with `--window`.

## Stop Rules

Stop and report instead of continuing if:

- implementation requires editor automation;
- implementation requires gameplay input automation beyond existing product
  transition functions;
- implementation requires runtime/session/save gameplay changes;
- implementation requires renderer/Vulkan changes;
- implementation requires fixture/package schema changes;
- a visual `package_visual_*` smoke must be changed;
- a real window is needed for proof;
- product automation starts resembling a broad `--codex-control` port.

If any stop rule triggers, split a narrower packet before implementation.
