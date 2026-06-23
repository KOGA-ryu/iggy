# Product App Boot + Menu Structure v1

## Packet

**Name:** Product App Boot + Menu Structure v1
**Repo:** `/Users/kogaryu/iggy3d`
**Branch:** `iggy3d-main`
**Mode:** builder work order, source implementation deferred until dispatch
**User-facing goal:** boot the game app, not a visual demo, and do not require users to pass a package fixture path.

The current windowed entrypoint is still named `iggy3d_visual_demo` and the README still shows a fixture path:

```sh
./build/iggy3d_visual_demo --opening-menu --package fixtures/demos/movement_playground/package.iggy3d.toml
```

That is now the wrong product contract. The product app should boot as:

```sh
./build/iggy3d
```

or, if the executable rename is staged:

```sh
./build/iggy3d_app
```

The app starts at the starter menu, resolves its default product template internally, and creates/loads saves through menu actions. `--package` becomes a dev-only override, not the normal launch path.

## Current Source Truth

- `/Users/kogaryu/iggy3d/CMakeLists.txt` currently builds `iggy3d_visual_demo`, `iggy3d_headless_demo`, `iggy3d_replay_tool`, and `iggy3d_collision_probe`.
- `/Users/kogaryu/iggy3d/apps/iggy3d_visual_demo/main.cpp` currently parses `--package`, `--opening-menu`, and `--no-opening-menu`.
- `/Users/kogaryu/iggy3d/src/app/PackageRuntimeLookup.*` already exists and should be reused for app-owned package/resource lookup.
- `/Users/kogaryu/iggy3d/src/runtime/save/SaveFileStore.*` already owns `.iggy3d.save` discovery/read/write/delete.
- `/Users/kogaryu/iggy3d/src/app/frontend/*` already owns starter, pause, settings, dev tools, save-slot preview, and frontend receipts.
- The worktree may already be dirty from Menu Usefulness and editor work. Builder must preserve unrelated dirty files.

## Target File Layout

### App entrypoint

```text
/Users/kogaryu/iggy3d/apps/iggy3d/main.cpp
/Users/kogaryu/iggy3d/src/app/iggy3d/AppShell.hpp
/Users/kogaryu/iggy3d/src/app/iggy3d/AppShell.cpp
```

`apps/iggy3d/main.cpp` is thin:

```cpp
int main(int argc, char** argv) {
  return iggy3d::runIggy3dApp(argc, argv);
}
```

`AppShell` owns top-level app orchestration:

- parse product app flags;
- resolve app default template/package;
- create starter menu state;
- dispatch menu actions;
- own active session lifetime;
- coordinate save/load/new-world;
- call renderer/window path when a window is requested;
- emit app receipts.

### Frontend menu models

Keep reusable menu models under:

```text
/Users/kogaryu/iggy3d/src/app/frontend/MenuRow.hpp
/Users/kogaryu/iggy3d/src/app/frontend/MenuAction.hpp
/Users/kogaryu/iggy3d/src/app/frontend/MenuAction.cpp
/Users/kogaryu/iggy3d/src/app/frontend/MenuInput.hpp
/Users/kogaryu/iggy3d/src/app/frontend/MenuInput.cpp
/Users/kogaryu/iggy3d/src/app/frontend/StarterScreen.hpp
/Users/kogaryu/iggy3d/src/app/frontend/StarterScreen.cpp
/Users/kogaryu/iggy3d/src/app/frontend/PauseMenu.hpp
/Users/kogaryu/iggy3d/src/app/frontend/PauseMenu.cpp
/Users/kogaryu/iggy3d/src/app/frontend/SettingsMenu.hpp
/Users/kogaryu/iggy3d/src/app/frontend/SettingsMenu.cpp
/Users/kogaryu/iggy3d/src/app/frontend/DevToolsMenu.hpp
/Users/kogaryu/iggy3d/src/app/frontend/DevToolsMenu.cpp
/Users/kogaryu/iggy3d/src/app/frontend/SaveBrowser.hpp
/Users/kogaryu/iggy3d/src/app/frontend/SaveBrowser.cpp
/Users/kogaryu/iggy3d/src/app/frontend/ConfirmDialog.hpp
/Users/kogaryu/iggy3d/src/app/frontend/ConfirmDialog.cpp
/Users/kogaryu/iggy3d/src/app/frontend/FrontendController.hpp
/Users/kogaryu/iggy3d/src/app/frontend/FrontendController.cpp
/Users/kogaryu/iggy3d/src/app/frontend/FrontendReceipt.hpp
/Users/kogaryu/iggy3d/src/app/frontend/FrontendReceipt.cpp
```

Menu models produce rows and action requests. They do not load files, mutate runtime, or call renderer APIs directly.

### App bridges

Use app-owned bridges for side effects:

```text
/Users/kogaryu/iggy3d/src/app/iggy3d/DefaultWorldTemplate.hpp
/Users/kogaryu/iggy3d/src/app/iggy3d/DefaultWorldTemplate.cpp
/Users/kogaryu/iggy3d/src/app/iggy3d/SaveBridge.hpp
/Users/kogaryu/iggy3d/src/app/iggy3d/SaveBridge.cpp
/Users/kogaryu/iggy3d/src/app/iggy3d/FrontendActionExecutor.hpp
/Users/kogaryu/iggy3d/src/app/iggy3d/FrontendActionExecutor.cpp
```

`DefaultWorldTemplate` owns product default package/scenario resolution. It may use `PackageRuntimeLookup`, app install/resource roots, or a built-in package id. It must not require users to type a fixture path.

`SaveBridge` owns save read/write/create/delete orchestration through `SaveFileStore`.

`FrontendActionExecutor` maps menu actions to app operations.

## Product Boot Contract

Default product command:

```sh
./build/iggy3d
```

Allowed product flags:

```text
--save-root <path>
--renderer auto|null|vulkan
--window
--no-window
--input keyboard|gamepad|auto
--print-render-receipt
```

Dev-only override flags:

```text
--dev-package-override <path>
--dev-scenario <id>
--automation-control <path>
```

Compatibility:

- `--package` may remain temporarily as a deprecated alias for `--dev-package-override` only if existing tests require it.
- README and product docs must stop using `--package fixtures/...` for normal launch.
- `--no-opening-menu` should become dev-only or be replaced by `--dev-skip-starter`.

## Menu Structure

Shared row model:

```cpp
struct MenuRow {
  FrontendAction action;
  std::string label;
  bool enabled;
  std::string disabledReason;
  std::string command;
};
```

### Starter screen

Rows, in order:

1. Continue
2. New World
3. Load Save
4. Settings
5. Dev Tools
6. Exit

Wiring:

```text
Continue      -> SaveBrowser selects latest compatible save -> SaveBridge loads save -> AppShell enters gameplay
New World     -> DefaultWorldTemplate resolves template -> SaveBridge creates .iggy3d.save -> AppShell enters gameplay
Load Save     -> SaveBrowser -> SaveBridge loads selected save -> AppShell enters gameplay
Settings      -> SettingsMenu
Dev Tools     -> DevToolsMenu parent=starter
Exit          -> ConfirmDialog -> AppShell quits
```

Rules:

- Starter owns input while open.
- No demo world, package fixture, or gameplay scene is loaded behind the starter.
- Continue is disabled if no compatible save exists.
- Load Save shows save rows. Incompatible/corrupt saves are visible but disabled when metadata can be read.

### Pause menu

Rows, in order:

1. Resume
2. Save
3. Save And Exit
4. Load Save
5. Settings
6. Dev Tools
7. Return To Title
8. Exit Game

Wiring:

```text
Resume          -> FrontendController closes pause
Save            -> SaveBridge writes current save
Save And Exit   -> SaveBridge writes current save -> AppShell quits
Load Save       -> SaveBrowser -> SaveBridge loads selected save
Settings        -> SettingsMenu parent=pause
Dev Tools       -> DevToolsMenu parent=pause
Return To Title -> AppShell unloads active session -> StarterScreen
Exit Game       -> ConfirmDialog -> AppShell quits
```

### Settings

Tabs:

1. Input
2. Controls
3. Camera
4. Gameplay
5. Video/Display
6. Audio
7. Accessibility
8. Developer

Settings are app-owned. They do not mutate runtime save truth. Persistent settings are a later packet unless a non-JSON settings store has already landed.

### Dev tools

Tabs:

1. Runtime
2. Input
3. Movement
4. World
5. Editor
6. Renderer
7. Save
8. Combat
9. Performance

Dev tools are read-only by default. Commands must be explicitly whitelisted. No hidden gameplay authority.

## Data Ownership

- `AppShell` owns app lifetime and orchestration.
- Frontend menu modules own rows, selection, enabled/disabled reasons, and action requests.
- `SaveBridge` owns save side effects through `SaveFileStore`.
- `DefaultWorldTemplate` owns default product world/template resolution.
- Runtime/session owns gameplay truth.
- `EditableRoomDocument` owns authored room truth.
- Renderer/window owns visual output only.
- Receipt output is proof, not save truth.

## Receipt Fields

Product app boot:

```text
app=iggy3d
app_mode=product
starter_open=true|false
starter_world_suppressed=true
default_template_resolved=true|false
default_template_source=app_default|save_metadata|dev_override|unavailable
dev_package_override_used=true|false
frontend_screen=<screen>
frontend_selected_action=<action>
frontend_launch_requested=true|false
selected_save_id=<id|none>
save_root=<path>
window_launch_count=<integer>
result=pass|fail|skip
reason_code=<stable-lower-snake>
```

Menu rows:

```text
menu_owner=starter|pause|settings|dev_tools|gameplay|none
menu_row_count=<integer>
menu_selected_action=<action>
menu_selected_enabled=true|false
menu_selected_disabled_reason=<reason-or-none>
menu_selected_command=<command>
```

New World:

```text
new_world_requested=true|false
world_template_id=<id|none>
world_template_status=<status>
save_created=true|false
selected_save_id=<id|none>
```

## Approved Files

Builder may edit:

```text
/Users/kogaryu/iggy3d/CMakeLists.txt
/Users/kogaryu/iggy3d/cmake/iggy3d_tests.cmake
/Users/kogaryu/iggy3d/apps/iggy3d/main.cpp
/Users/kogaryu/iggy3d/apps/iggy3d_visual_demo/main.cpp
/Users/kogaryu/iggy3d/src/app/iggy3d/*
/Users/kogaryu/iggy3d/src/app/frontend/*
/Users/kogaryu/iggy3d/src/app/PackageRuntimeLookup.*
/Users/kogaryu/iggy3d/src/runtime/save/SaveFileStore.*
/Users/kogaryu/iggy3d/tests/unit/*frontend*tests.cpp
/Users/kogaryu/iggy3d/tests/unit/*menu*tests.cpp
/Users/kogaryu/iggy3d/tests/unit/*save_slot*tests.cpp
/Users/kogaryu/iggy3d/tests/smoke/package_app_boot_smoke.cpp
/Users/kogaryu/iggy3d/tests/smoke/package_visual_starter_screen_smoke.cpp
/Users/kogaryu/iggy3d/tests/smoke/package_visual_opening_menu_smoke.cpp
/Users/kogaryu/iggy3d/README.md
```

`iggy3d_visual_demo` may stay temporarily as a compatibility target, but product docs and new tests must use `iggy3d`.

## Forbidden Surfaces

- No runtime gameplay changes.
- No save format/schema changes unless required for product world metadata and explicitly tested.
- No renderer/Vulkan/screenshot/frame-hash work.
- No fixture-path launch in product docs.
- No JSON.
- No old `/Users/kogaryu/iggy` dependency.
- No deleting scripted/no-window automation control capability.

## Builder Phases

### Phase 1: product app target

- Add `apps/iggy3d/main.cpp`.
- Add `iggy3d` executable target.
- Reuse current visual app shell code path internally.
- Product command `./build/iggy3d` opens starter menu.
- Keep `iggy3d_visual_demo` as compatibility alias if tests still depend on it.

### Phase 2: default template resolution

- Add `DefaultWorldTemplate`.
- Resolve default package/scenario internally.
- Convert `--package` into `--dev-package-override` or deprecated alias.
- README no longer tells users to pass fixture paths.

### Phase 3: frontend action executor

- Move row action execution into `FrontendActionExecutor`.
- Starter New World creates save from default template.
- Continue/Load Save load via save metadata.

### Phase 4: tests and receipt migration

- Add `package_app_boot_smoke`.
- Assert `app=iggy3d`, `starter_world_suppressed=true`, and no fixture path requirement.
- Keep no-window/null smoke path for default verification.

## Verification Commands

From `/Users/kogaryu/iggy3d`:

```sh
cmake -S . -B build
cmake --build build --target iggy3d
./build/iggy3d --renderer null --no-window --print-render-receipt
ctest --test-dir build --output-on-failure -R 'app_boot|frontend|starter|opening|menu|save_slot'
```

Headless/replay stability:

```sh
./build/iggy3d_headless_demo \
  --package fixtures/demos/first_room/package.iggy3d.toml \
  --summary fixtures/demos/first_room/expected_summary.txt \
  --save /tmp/iggy3d_product_app_boot_runtime.save

./build/iggy3d_replay_tool \
  --package fixtures/demos/first_room/package.iggy3d.toml \
  --save /tmp/iggy3d_product_app_boot_runtime.save \
  --expect-summary fixtures/demos/first_room/expected_summary.txt
```

Werror:

```sh
cmake -S . -B build-werror -DIGGY3D_WARNINGS_AS_ERRORS=ON
cmake --build build-werror --target iggy3d
ctest --test-dir build-werror --output-on-failure -R 'app_boot|frontend|starter|opening|menu|save_slot'
rm -rf build-werror
```

Scans:

```sh
rg -n -- '--package fixtures|fixtures/demos/.+package\\.iggy3d\\.toml' README.md docs apps src tests cmake CMakeLists.txt
rg -n '/Users/kogaryu/iggy|namespace runtime3d|iggy::three_d|nlohmann|rapidjson|\\.json\\b|JSON|Json|StatusCode' apps src tests cmake CMakeLists.txt
git diff --check
```

Expected product docs scan result:

- No normal user-facing README command uses `--package fixtures/...`.
- Fixture paths may remain only in headless/replay tests, smoke tests, or explicit dev override docs.

## Expected Builder Final Report

1. Product app target added/updated.
2. Exact user-facing boot command.
3. Default template/package resolution path.
4. Deprecated dev override flags, if any.
5. Menu row wiring summary.
6. Receipt fields proven.
7. Tests and scans run.
8. Remaining `iggy3d_visual_demo` compatibility uses classified as temporary/test-only or unresolved.

## Open Decision

Executable name should be `iggy3d` unless the lead wants `iggy3d_app` as an interim target. Conservative default: create `iggy3d` now and leave `iggy3d_visual_demo` as a deprecated compatibility target until all tests are migrated.
