# Product App Smoke Migration v1

## Objective

Migrate product-equivalent frontend/menu no-window smoke coverage onto the product app path `IGGY3D_PRODUCT_APP_PATH` / `./build/iggy3d`, while keeping renderer/package visual coverage on `IGGY3D_VISUAL_DEMO_PATH` / `iggy3d_visual_demo`.

This packet is a narrow source/test migration packet. It must improve product-app smoke coverage without removing the compatibility visual shell or weakening existing renderer/package tests.

## Non-Goals

- Do not remove, rename, or broadly deprecate `iggy3d_visual_demo`.
- Do not migrate renderer/package-room visual smokes.
- Do not touch Vulkan docs or renderer/Vulkan source.
- Do not invent product `--opening-menu`, `--no-opening-menu`, `--dev-menu`, or `--codex-control`.
- Do not port broad visual `--codex-control` semantics into product app.
- Do not add app/window launch requirements.
- Do not change runtime/session/save gameplay logic.

## Current Source Truth

- `iggy3d_app` builds from `apps/iggy3d/main.cpp` and outputs `./build/iggy3d`.
- Product smokes use `IGGY3D_PRODUCT_APP_PATH`.
- Product app already supports:
  - `--no-window`;
  - `--window`;
  - `--renderer null|vulkan` as product settings/receipt choice, not Vulkan backend proof;
  - `--input keyboard|gamepad|auto`;
  - `--save-root <path>`;
  - `--frames <count>`;
  - `--hold-seconds <count>`;
  - `--auto-new-world`;
  - `--scripted-gameplay-smoke`;
  - `--dev-package-override <path>`;
  - `--package <path>` as a dev-package alias;
  - `--automation-control <path>` and `--script-control <path>` in option parsing only.
- Product `automationControlPath` is currently parsed into `ProductAppOptions`, but `AppShell` does not consume it yet. Tests must not rely on product automation behavior until this packet implements a minimal product-owned intake.
- `iggy3d_visual_demo` still owns compatibility/package visual shell behavior, including:
  - `--opening-menu`;
  - `--no-opening-menu`;
  - `--dev-menu`;
  - `--codex-control`;
  - package visual renderer receipts;
  - renderer/package-room Vulkan proof.
- `package_visual_*` smokes are registered under `if(TARGET iggy3d_visual_demo)` and use `IGGY3D_VISUAL_DEMO_PATH`.
- Product smokes currently include:
  - `product_gameplay_controls_smoke`;
  - `product_menu_transition_smoke`.

## Approved Files

Builder may edit only these files unless they stop and report a source-truth blocker:

- `cmake/iggy3d_tests.cmake`
  - Register product smoke targets and wire `IGGY3D_PRODUCT_APP_PATH`.
- `tests/smoke/product_menu_transition_smoke.cpp`
  - Extend only if existing product transition proof needs one more no-window case.
- `tests/smoke/product_menu_usefulness_smoke.cpp`
  - New preferred smoke if product no-window menu usefulness proof needs a distinct target.
- `tests/smoke/product_opening_menu_smoke.cpp`
  - New only if product starter behavior needs separate proof. It must reflect product starter semantics, not old visual `--opening-menu`.
- `src/app/iggy3d/AppShell.cpp`
  - Only if adding minimal product automation intake is necessary for frontend/menu actions.
- `src/app/iggy3d/ProductAppOptions.hpp`
- `src/app/iggy3d/ProductAppOptions.cpp`
  - Only for tiny option/receipt corrections. Do not add old visual shell flag names.
- `src/app/iggy3d/ReceiptBuilder.hpp`
- `src/app/iggy3d/ReceiptBuilder.cpp`
  - Only for honest receipt fields needed by product no-window smoke assertions.
- Product docs after source/test behavior exists:
  - `docs/build_packets/product_app_source_test_migration_scout_v1.md`;
  - `docs/roadmaps/complete_runtime_todo_roadmap.md`;
  - `fixtures/demos/movement_playground/README.md` only if command guidance changes.

## No-Go Files And Surfaces

- `docs/vulkan/**/*.md`
- renderer/Vulkan source
- runtime/session/save gameplay logic
- fixture/package schemas
- `tests/smoke/package_visual_room_asset_smoke.cpp`
- `tests/smoke/package_visual_window_smoke.cpp`
- `tests/smoke/package_visual_playable_proxy_smoke.cpp`
- renderer/package-room visual smokes
- broad rename/removal of `iggy3d_visual_demo`
- CMake install/package changes for the visual shell

## First Implementation Slice

Recommended first slice: `product_menu_usefulness_smoke` without product automation intake if possible.

The builder should first check whether the current product app no-window receipts already prove enough menu usefulness:

```sh
./build/iggy3d --no-window --print-render-receipt
./build/iggy3d --no-window --scripted-gameplay-smoke --print-render-receipt
```

If those two commands can prove starter and gameplay/menu transition fields, add `product_menu_usefulness_smoke.cpp` that only runs those command forms and asserts the receipts. Do not add automation intake in that case.

If menu-usefulness cases require scripted menu navigation that the current product app cannot express, add the smallest possible product automation intake in `AppShell.cpp`:

- consume `options.automationControlPath`;
- read a deterministic key-value control file;
- support only frontend/menu actions needed by this packet;
- route to existing product frontend/menu transition functions;
- keep runtime gameplay mutation out of the automation path unless an existing product transition already performs it;
- set receipt fields that honestly show automation was used.

Stop and split a separate `Product Automation Parity` packet if the work expands beyond frontend/menu routing.

## Product Command Rules

Allowed product commands:

```sh
./build/iggy3d --no-window --print-render-receipt
./build/iggy3d --no-window --scripted-gameplay-smoke --print-render-receipt
```

Allowed only if this packet implements product automation intake:

```sh
./build/iggy3d --no-window --automation-control <path> --print-render-receipt
```

Never use these on the product app path:

- `--opening-menu`;
- `--no-opening-menu`;
- `--dev-menu`;
- `--codex-control`.

Do not use window launches in this packet.

## Receipt Assertions

### Starter / No-World Receipt

Prefer these assertions for `./build/iggy3d --no-window --print-render-receipt`:

```text
app=iggy3d
frontend_screen=starter
starter_world_suppressed=true
gameplay_active=false
gameplay_view_visible=false
input_owner=starter
gameplay_input_suppressed=true
product_render_bridge_ready=false
product_view_frame_ready=false
product_view_frame_item_count=0
product_feedback_bridge_ready=false
result=pass
reason_code=<stable-reason>
```

`window_launch_count=0` is not a product app receipt field today. The smoke
binary should print it as its own proof line after running only `--no-window`
commands, matching the existing product smoke convention.

### Scripted Gameplay Receipt

Prefer these assertions for `./build/iggy3d --no-window --scripted-gameplay-smoke --print-render-receipt`:

```text
app=iggy3d
frontend_screen=gameplay
scripted_gameplay_smoke=true
gameplay_active=true
gameplay_view_visible=true
gameplay_input_source=scripted
gameplay_command_submitted=true
gameplay_command_status=accepted
target_discovered=true
gameplay_reach_gate=pass
product_draw_item_count=<positive-integer>
product_view_projection=primitive_first_person
product_render_bridge_ready=true
product_view_frame_ready=true
product_view_frame_item_count=<positive-integer>
product_feedback_bridge_ready=true
product_feedback_bridge_line_count=<positive-integer>
result=pass
reason_code=<stable-reason>
```

The smoke binary should print `window_launch_count=0` as its own proof line; do
not require that field inside the product app receipt unless `ReceiptBuilder`
explicitly adds it in a later packet.

### Menu Transition Receipt

`product_menu_transition_smoke` already proves starter and gameplay transition fields. Reuse or extend carefully:

```text
product_transition_last_action=<startup|launch_gameplay|...>
product_transition_status=<starter_ready|gameplay_active|...>
product_transition_returned_to_gameplay=true|false
product_transition_returned_to_title=true|false
product_transition_session_preserved=true|false
```

If product automation intake is added, add focused receipt fields only if they are honest:

```text
automation_control_loaded=true|false
automation_control_status=<not_requested|loaded|parse_error|unsupported_action|applied>
automation_control_scope=frontend_menu
```

Do not add Codex-branded receipt fields.

## Test Plan

### Preferred Minimal Product Smoke

Create `tests/smoke/product_menu_usefulness_smoke.cpp`.

It should:

1. Use `IGGY3D_PRODUCT_APP_PATH`.
2. Run the no-window starter receipt command.
3. Run the no-window scripted gameplay receipt command.
4. Parse key-value receipts and reject duplicate fields.
5. Print and assert its own `window_launch_count=0` proof line because every
   invoked product command uses `--no-window`.
6. Assert product app fields only; do not accept `app=iggy3d_visual_demo`.
7. Return `77` only if `IGGY3D_PRODUCT_APP_PATH` is unavailable.

If automation intake is not implemented, the smoke should not mention `--automation-control`.

### Optional Product Automation Menu Case

Only if needed, add a small control file case:

```text
menu.down=true
menu.confirm=true
```

or equivalent names already used by product frontend/menu code.

The product control file must be key-value text only, no JSON.

The smoke must prove the action affected frontend/menu state and did not launch a window.

## CMake Registration

In `cmake/iggy3d_tests.cmake`:

- add `product_menu_usefulness_smoke`;
- link against `iggy3d`;
- apply warnings;
- define `IGGY3D_PRODUCT_APP_PATH="$<TARGET_FILE:iggy3d_app>"`;
- add dependency on `iggy3d_app`;
- register CTest with working directory `${CMAKE_CURRENT_SOURCE_DIR}`;
- set `SKIP_RETURN_CODE 77`;
- labels should include:

```text
smoke;product;frontend;menu;usefulness;no_window;iggy3d
```

Do not move `package_visual_*` registrations in this packet unless a product-equivalent replacement is already green and explicitly scoped.

## Acceptance Commands

Run from `/Users/kogaryu/iggy3d`:

```sh
cmake --build build --target iggy3d_app
cmake --build build --target product_gameplay_controls_smoke
cmake --build build --target product_menu_transition_smoke
cmake --build build --target product_menu_usefulness_smoke
ctest --test-dir build --output-on-failure -R '^product_(gameplay_controls|menu_transition|menu_usefulness)_smoke$'
git diff --check
```

No app/window launch. No broad CTest loop.

If `product_menu_usefulness_smoke` is not added because the builder finds a stronger existing product smoke covers the required behavior, stop and report the exact existing target and receipt assertions instead of forcing a redundant test.

## Risks

- Losing renderer/package-room coverage by touching `package_visual_*` smokes too early.
- Accidentally treating product `--renderer vulkan` as Vulkan proof.
- Inventing product CLI names that duplicate visual shell behavior instead of using product semantics.
- Expanding `--automation-control` into a broad control-file port.
- Reintroducing `--codex-control` in product tests or receipts.
- Window-launch spam from migrating visual smokes into product smoke lanes without no-window separation.

## Rollback / Stop Rule

Stop and report instead of continuing if:

- product automation intake requires gameplay/editor mutation beyond frontend/menu routing;
- a package visual renderer smoke must be changed to make product smokes pass;
- a Vulkan/renderer source change appears necessary;
- a fixture/schema/runtime/save change appears necessary;
- the new product smoke would need a real window.

If any of those happen, split a separate packet:

```text
Product Automation Parity v1
```

and keep this packet limited to product no-window smoke registration and existing receipt proof.
