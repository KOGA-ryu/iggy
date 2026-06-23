# Visual Demo Decomposition v1 Builder Packet

## Packet

**Name:** Visual Demo Decomposition v1
**Repo:** `/Users/kogaryu/iggy3d`
**Branch:** `iggy3d-main`
**Mode:** docs-only builder packet for a later source migration
**Default verification:** no-window/null/receipt tests; no app-window spam.

`apps/iggy3d_visual_demo/main.cpp` is now a 7k+ LOC integration pile. This packet defines a behavior-preserving migration plan before builders split it. The first implementation packet must reduce coupling without changing gameplay, renderer, save, editor, or menu behavior.

User direction: “im deleting the codex thing also.” Treat this as removing Codex-branded runtime/control naming from the product-facing visual demo surface. Preserve the underlying automated/no-window scripted control capability under neutral naming such as `AutomationControl` unless a later lead instruction explicitly requires hard deletion of all aliases in one pass.

Do not implement source from this document until dispatched. Do not commit. Do not introduce JSON. Do not use old `/Users/kogaryu/iggy` code. Do not launch windows for this packet.

## Current Source Truth

Current file:

- `/Users/kogaryu/iggy3d/apps/iggy3d_visual_demo/main.cpp`
- Current size observed during liaison pass: `7535` lines.
- It already includes frontend modules, `MenuInput`, `PauseMenu`, save store, package lookup, SDL window/platform, Vulkan backend bridge, projection, renderer API, bean mesh, authoring document, runtime movement, traversal, ability, save/load, session, and debug snapshot headers.

Major internal regions currently mixed in `main.cpp`:

- **CLI/options parsing**
  - `VisualOptions`, `ParseResult`, `parseVisualOptions`, frame limit helpers, failure printing.
  - Owns renderer choice, window/no-window, package path, opening menu, control file, dev menu, scripted modes.
- **Window/render/backend setup**
  - `WindowReceiptFields`, SDL unavailable/window unavailable receipt helpers, Vulkan boot field appending, `printReceiptAndReturn`.
  - Owns immediate visual app boot behavior.
- **Playable receipt aggregation**
  - `PlayableReceiptFields` contains frontend, editor, runtime, movement, traversal, combat, beans, gamepad, debug, and control-file receipt state.
  - `appendPlayableReceiptFields` emits the final key-value proof surface.
- **Opening/starter save menu bridge**
  - `OpeningMenuState`, opening menu action parsing, save list refresh, `writeOpeningMenuSaveFile`, `loadOpeningMenuSelectedSave`, `executeOpeningMenuAction`, `recordOpeningMenuFields`, `recordFrontendFields`.
- **Frontend/pause/settings/dev tools integration**
  - Frontend action/category/tab parsing, pause action navigation, menu owner and gameplay suppression wiring, settings apply/restore/back, dev overlay parent and category proof.
- **Editor integration**
  - `EditorTool`, `EditorPreset`, `EditorModeState`, editor tool/preset parsing, cursor/probe/ghost, selection, apply/delete/add-basic-room/transform/undo/redo, runtime room rebuild, authored room load/save receipts, debug HUD lines.
- **Control-file parser currently branded Codex**
  - `CodexControlFrame`, `readCodexControlFile`, `setControlStatus`, frame-list parsing, current `--codex-control` CLI option, `codex_*` receipt fields, `codex_probe.*` keys, `runCodexAcceptanceDemoStep`.
  - This must become neutral `AutomationControl` / `automation_*` naming while preserving test automation capability.
- **Debug HUD and projections**
  - Runtime debug snapshot recording, editor/opening/pause/dev debug HUD line builders, bean model projection, editor ghost projection, ability projectile projection.
- **Runtime/session/gameplay loop orchestration**
  - Session creation, scripted playable step, control commands, save/load roundtrip, reset recording, runtime debug snapshot, movement/traversal/ability/combat interaction, player motor reset.
- **Input/gamepad routing**
  - `GamepadSession`, DualSense name detection, gamepad open/close, pressed-edge helpers, mouse look, keyboard/controller/app event routing.
- **Receipt output and app loop**
  - Main loop owns frame advancement, input owner checks, menu/editor/runtime command dispatch, render submit, receipt finalization, exit code.

Current tests proving behavior include:

- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_codex_control_smoke.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_menu_usefulness_smoke.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_ingame_menu_smoke.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_opening_menu_smoke.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_starter_screen_smoke.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_editor_control_smoke.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_editor_manipulation_smoke.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_editor_save_load_smoke.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_movement_playground_smoke.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_playable_proxy_smoke.cpp`
- `/Users/kogaryu/iggy3d/tests/smoke/package_visual_bean_models_smoke.cpp`
- frontend/menu/input/editor unit tests under `/Users/kogaryu/iggy3d/tests/unit/`.

## Migration Target File Layout

Target state after the decomposition series:

- `/Users/kogaryu/iggy3d/apps/iggy3d_visual_demo/main.cpp`
  - Thin entrypoint only: call `iggy3d::runVisualDemoApp(argc, argv)` and return its exit code.
- `/Users/kogaryu/iggy3d/src/app/visual_demo/VisualDemoApp.hpp`
- `/Users/kogaryu/iggy3d/src/app/visual_demo/VisualDemoApp.cpp`
  - Owns high-level app orchestration and lifetime. This is the only place that should coordinate runtime session, frontend controller, editor controller, save bridge, input router, renderer submit, and receipt builder.
- `/Users/kogaryu/iggy3d/src/app/visual_demo/VisualDemoOptions.hpp`
- `/Users/kogaryu/iggy3d/src/app/visual_demo/VisualDemoOptions.cpp`
  - Owns CLI parse and app options. It may keep app-local parsing to avoid leaking app flags into generic app config.
- `/Users/kogaryu/iggy3d/src/app/visual_demo/InputRouter.hpp`
- `/Users/kogaryu/iggy3d/src/app/visual_demo/InputRouter.cpp`
  - Owns event/input collection normalization and per-frame input state. It consumes keyboard, mouse, gamepad, and automation control output and produces semantic app/game/menu/editor actions.
- `/Users/kogaryu/iggy3d/src/app/visual_demo/FrontendController.hpp`
- `/Users/kogaryu/iggy3d/src/app/visual_demo/FrontendController.cpp`
  - Owns starter/opening/pause/settings/dev tools orchestration against existing `src/app/frontend` models.
- `/Users/kogaryu/iggy3d/src/app/visual_demo/EditorController.hpp`
- `/Users/kogaryu/iggy3d/src/app/visual_demo/EditorController.cpp`
  - Owns app-level editor state, cursor/probe/ghost, selection routing, and calls into `EditableRoomSession` commands.
- `/Users/kogaryu/iggy3d/src/app/visual_demo/SaveBridge.hpp`
- `/Users/kogaryu/iggy3d/src/app/visual_demo/SaveBridge.cpp`
  - Owns visual-demo save/load orchestration around `SaveFileStore`, `Session`, and optional authored room. It must not change save format.
- `/Users/kogaryu/iggy3d/src/app/visual_demo/ReceiptBuilder.hpp`
- `/Users/kogaryu/iggy3d/src/app/visual_demo/ReceiptBuilder.cpp`
  - Owns app-specific receipt aggregation and appends fields through existing `RenderReceipt` helpers.
- `/Users/kogaryu/iggy3d/src/app/visual_demo/AutomationControl.hpp`
- `/Users/kogaryu/iggy3d/src/app/visual_demo/AutomationControl.cpp`
  - Neutral replacement for Codex-branded control-file parser and frame-gated test/dev input adapter.
- `/Users/kogaryu/iggy3d/src/app/visual_demo/DebugHudController.hpp`
- `/Users/kogaryu/iggy3d/src/app/visual_demo/DebugHudController.cpp`
  - Optional but recommended once receipt/controller extraction is underway. Owns app debug HUD line assembly; projection/rendering remain in their existing modules.
- `/Users/kogaryu/iggy3d/src/app/visual_demo/BeanProbeController.hpp`
- `/Users/kogaryu/iggy3d/src/app/visual_demo/BeanProbeController.cpp`
  - Optional if separating the Codex probe/bean visual rename would otherwise keep product-facing Codex naming alive in `main.cpp`.

CMake:

- Update `/Users/kogaryu/iggy3d/CMakeLists.txt` and/or `/Users/kogaryu/iggy3d/cmake/iggy3d_tests.cmake` only to add new app source files and renamed tests.

## Delete Codex Thing Policy

### Current Codex-Branded Surfaces

Current source/test/docs references include:

- CLI: `--codex-control`.
- Struct/functions: `CodexControlFrame`, `readCodexControlFile`, `setControlStatus`, `runCodexAcceptanceDemoStep`, `codexEditorTransformConfigured`, `CodexProbeState`.
- Control keys: `codex_probe.visible`, `codex_probe.position`, `codex_probe.position_meters`, `codex_probe.position_ft`.
- Receipt fields: `codex_control_configured`, `codex_control_read`, `codex_control_applied`, `codex_control_status`, `codex_control_path`, `bean_codex_probe_*`.
- Test binary and CMake target: `package_visual_codex_control_smoke`.
- Test file names/temp paths containing `codex`.
- Bean model id/kind: `bean_codex_probe` and `BeanModelKind::CodexProbe` in tests/source.
- Recent docs and README mentions of Codex-control.

### Neutral Replacement Names

Use these replacements unless source truth during implementation suggests a better neutral name:

```text
--codex-control                  -> --automation-control
CodexControlFrame                -> AutomationControlFrame
readCodexControlFile             -> readAutomationControlFile
runCodexAcceptanceDemoStep       -> runAutomationAcceptanceDemoStep
CodexProbeState                  -> AutomationProbeState
codex_probe.visible              -> automation_probe.visible
codex_probe.position             -> automation_probe.position
tests/...codex_control...        -> tests/...automation_control...
package_visual_codex_control     -> package_visual_automation_control
codex_control_configured         -> automation_control_configured
codex_control_read               -> automation_control_read
codex_control_applied            -> automation_control_applied
codex_control_status             -> automation_control_status
codex_control_path               -> automation_control_path
bean_codex_probe_*               -> bean_automation_probe_* or automation_probe_bean_*
bean_codex_probe                 -> bean_automation_probe
BeanModelKind::CodexProbe        -> BeanModelKind::AutomationProbe
```

### Compatibility Policy

Preferred migration:

1. Add neutral `--automation-control` and neutral receipt fields.
2. Temporarily accept `--codex-control` as a deprecated alias in tests only, with no product-facing docs using the old name.
3. Temporarily accept old `codex_probe.*` keys as aliases for automation control files while all tests are renamed.
4. During the final phase, remove old receipt fields from product-facing output. If compatibility is still needed for a short time, keep old fields only in a dedicated compatibility smoke and mark them deprecated.
5. Do not remove no-window scripted test-control capability.

Hard deletion alternative:

If lead explicitly says old aliases must be deleted in the same packet, Builder must update all affected tests, CMake target names, README/docs, temp file names, control keys, and receipt assertions in the same packet. That is higher blast radius and should not be combined with broad behavior refactors.

## Data Ownership and Semantics

- Visual demo app owns orchestration only.
- Runtime/session remains runtime-owned.
- Editable room truth remains `EditableRoomDocument` / `EditableRoomSession` owned.
- Frontend menu models remain `src/app/frontend` owned.
- Save format/store remains `src/runtime/save` owned.
- Renderer/window remains render/platform owned.
- Automation control is a test/dev input adapter. It is not gameplay truth, not a runtime command log, and not save/replay state.
- Receipts prove app behavior. Receipts are not save truth.
- Debug HUD is user-visible diagnostics; it should consume existing debug/projection data and app state, not own runtime state.

## Phased Builder Plan

### Phase 1: Pure Data / Options / Automation Parsing Extraction

Behavior-preserving first packet.

Approved files:

- `apps/iggy3d_visual_demo/main.cpp`
- `src/app/visual_demo/VisualDemoOptions.hpp/.cpp`
- `src/app/visual_demo/AutomationControl.hpp/.cpp`
- `tests/unit/visual_demo_options_tests.cpp`
- `tests/unit/automation_control_tests.cpp`
- `tests/smoke/package_visual_codex_control_smoke.cpp` only if compatibility aliases are asserted
- `cmake/iggy3d_tests.cmake`
- `CMakeLists.txt`

Expected diff shape:

- Move `VisualOptions`, `ParseResult`, CLI parsing, frame-limit helpers into `VisualDemoOptions`.
- Move `CodexControlFrame` parser into `AutomationControl` with neutral names.
- Add `--automation-control`; keep `--codex-control` as deprecated alias unless hard deletion is dispatched.
- Add neutral receipt field values inside data structs, but do not remove old receipt fields in phase 1 unless all dependent tests are updated.
- Main app behavior and receipt values remain byte-equivalent except added neutral fields.

Forbidden in phase 1:

- No gameplay changes.
- No renderer/window changes.
- No save format changes.
- No editor behavior changes.
- No menu redesign.

Tests:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure -R 'visual_demo_options|automation_control|codex_control|menu_usefulness|ingame_menu|opening|starter|editor|movement_playground'
```

### Phase 2: Receipt and Debug HUD Builders

Approved files:

- `src/app/visual_demo/ReceiptBuilder.hpp/.cpp`
- `src/app/visual_demo/DebugHudController.hpp/.cpp`
- `apps/iggy3d_visual_demo/main.cpp`
- frontend/editor/menu smoke tests that assert receipts

Expected diff shape:

- Move `PlayableReceiptFields`, `WindowReceiptFields`, receipt append helpers, and app-specific receipt finalization into `ReceiptBuilder`.
- Move editor/opening/pause/dev debug HUD line builders into `DebugHudController`.
- Preserve field order and field values for no-window receipts, except explicitly renamed automation fields.

Receipt parity gate:

- For identical no-window automation controls, old and new code paths must emit identical key-value receipts except for planned neutral field additions/renames.
- Tests must reject duplicate receipt keys.

### Phase 3: Frontend Menu Controller

Approved files:

- `src/app/visual_demo/FrontendController.hpp/.cpp`
- `src/app/frontend/*` only if a model seam is missing
- `apps/iggy3d_visual_demo/main.cpp`
- menu/frontend tests

Expected diff shape:

- Move `OpeningMenuState`, frontend field recording, starter/pause/settings/dev overlay orchestration, save-list refresh calls, and menu owner logic out of `main.cpp`.
- Preserve menu owner priority and gameplay input suppression.
- No save format changes.

Tests:

```sh
ctest --test-dir build --output-on-failure -R 'frontend|starter|settings|dev_tools|pause_menu|menu_input|opening|ingame_menu|menu_usefulness'
```

### Phase 4: Editor Controller and Save Bridge

Approved files:

- `src/app/visual_demo/EditorController.hpp/.cpp`
- `src/app/visual_demo/SaveBridge.hpp/.cpp`
- `apps/iggy3d_visual_demo/main.cpp`
- editor/save smoke tests

Expected diff shape:

- Move `EditorModeState`, tool/preset parsing, cursor/probe/ghost, selection, apply/delete/transform/undo/redo orchestration into `EditorController`.
- Move visual app save/load orchestration around `SaveFileStore` and authored room into `SaveBridge`.
- `EditableRoomDocument` remains the authoring truth.
- Runtime room/collision/traversal rebuild remains an output of editor/save bridge operations.

Tests:

```sh
ctest --test-dir build --output-on-failure -R 'editable_room|save_load|save_file_store|visual_editor|editor_manipulation|opening|starter'
```

### Phase 5: Thin Main and Remove / Rename Codex-Branded Surfaces

Approved files:

- `apps/iggy3d_visual_demo/main.cpp`
- `src/app/visual_demo/*`
- `tests/smoke/package_visual_automation_control_smoke.cpp`
- `tests/smoke/package_visual_codex_control_smoke.cpp` only if kept as compatibility alias temporarily
- `tests/unit/bean_mesh_tests.cpp`
- `src/render/mesh/BeanMesh.*` only for `CodexProbe` to `AutomationProbe` rename
- `README.md`
- docs/build packet references that mention Codex-control
- CMake test registration

Expected diff shape:

- `main.cpp` becomes thin entrypoint.
- Product-facing CLI/docs/tests use `automation_control`, `automation_probe`, and `--automation-control`.
- Old `--codex-control` accepted only if compatibility alias is still explicitly needed.
- No product-facing receipt field starts with `codex_` after final migration.
- Rename test target labels away from `codex_control`.

Tests:

```sh
ctest --test-dir build --output-on-failure -R 'automation_control|visual|menu_usefulness|editor|movement_playground|bean|opening|starter'
rg -n 'codex|Codex|--codex|codex_' apps src tests docs README.md cmake CMakeLists.txt
```

The final `rg` should report only intentionally deprecated aliases if the phase keeps compatibility. Builder must list every remaining hit.

## Verification Strategy

No-window defaults:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure -R 'visual|automation|codex_control|menu_usefulness|ingame_menu|opening|starter|editor|movement_playground|frontend|settings|dev_tools|gamepad'
```

Headless/replay stability:

```sh
./build/iggy3d_headless_demo \
  --package fixtures/demos/first_room/package.iggy3d.toml \
  --summary fixtures/demos/first_room/expected_summary.txt \
  --save /tmp/iggy3d_visual_demo_decomposition_runtime.save

./build/iggy3d_replay_tool \
  --package fixtures/demos/first_room/package.iggy3d.toml \
  --save /tmp/iggy3d_visual_demo_decomposition_runtime.save \
  --expect-summary fixtures/demos/first_room/expected_summary.txt
```

Warnings-as-errors:

```sh
cmake -S . -B build-werror -DIGGY3D_WARNINGS_AS_ERRORS=ON
cmake --build build-werror
ctest --test-dir build-werror --output-on-failure -R 'visual|automation|codex_control|menu_usefulness|ingame_menu|opening|starter|editor|movement_playground|frontend|settings|dev_tools|gamepad'
rm -rf build-werror
```

Scans:

```sh
rg -n '#include[ <"](SDL3/|SDL\.h|SDL_vulkan|vulkan/)|\bVk[A-Z][A-Za-z0-9_]*|\bVK_[A-Z0-9_]+' src/runtime src/content src/projection src/runtime/save

rg -n '/Users/kogaryu/iggy|namespace runtime3d|iggy::three_d|nlohmann|rapidjson|\.json\b|JSON|Json|StatusCode' apps src tests cmake CMakeLists.txt

git diff --check
```

Receipt parity:

- Add or use a smoke helper that runs the same no-window control file before and after extraction.
- Expected result: identical key-value receipt fields except explicitly added/renamed automation fields.
- `window_launch_count=0` is required for decomposition packet smokes.

## Risk and No-Go Surfaces

- No gameplay behavior changes.
- No renderer/Vulkan/swapchain/shader/screenshot/frame-hash changes.
- No save format/schema changes.
- No JSON.
- No old `/Users/kogaryu/iggy` dependency.
- No full UI redesign.
- No physical controller remap unless isolated and covered by unit tests.
- No broad include leakage from app visual demo modules into runtime/content/projection/save.
- Do not remove scripted/no-window control-file capability.
- Do not combine hard Codex alias deletion with large behavior-preserving extraction unless lead explicitly accepts the blast radius.

## Recommended First Builder Packet

**Visual Demo Decomposition v1a: Options + AutomationControl Extraction**

Goal: extract CLI/options parsing and control-file parsing into `src/app/visual_demo/VisualDemoOptions.*` and `src/app/visual_demo/AutomationControl.*`, add `--automation-control`, preserve `--codex-control` as a deprecated alias, and prove no-window receipt parity.

This first packet is the safest start because it removes a major pure-data/parser region from `main.cpp` without changing runtime/session/editor/render behavior.

Approved files for v1a:

- `/Users/kogaryu/iggy3d/apps/iggy3d_visual_demo/main.cpp`
- `/Users/kogaryu/iggy3d/src/app/visual_demo/VisualDemoOptions.hpp`
- `/Users/kogaryu/iggy3d/src/app/visual_demo/VisualDemoOptions.cpp`
- `/Users/kogaryu/iggy3d/src/app/visual_demo/AutomationControl.hpp`
- `/Users/kogaryu/iggy3d/src/app/visual_demo/AutomationControl.cpp`
- `/Users/kogaryu/iggy3d/tests/unit/visual_demo_options_tests.cpp`
- `/Users/kogaryu/iggy3d/tests/unit/automation_control_tests.cpp`
- relevant visual smoke tests only for CLI flag/receipt alias updates
- `/Users/kogaryu/iggy3d/CMakeLists.txt`
- `/Users/kogaryu/iggy3d/cmake/iggy3d_tests.cmake`

Acceptance for v1a:

- Existing no-window scripted/control smokes still pass.
- New `--automation-control` path passes at least one existing control smoke.
- Deprecated `--codex-control` alias passes only if compatibility retained.
- Receipts include neutral `automation_control_*` fields.
- Product-facing README/docs prefer automation naming.
- Builder reports all remaining `codex` string hits and classifies them as deprecated alias, test compatibility, or unresolved.

## Open Questions

The phrase “im deleting the codex thing also” could mean either:

1. remove product-facing Codex naming while preserving test automation under neutral names, or
2. hard delete every Codex-branded alias immediately.

Conservative default for Builder: do option 1 first. It preserves the no-window scripted control capability that many tests rely on, avoids breaking receipt smokes, and creates a clean path to hard deletion in a final rename phase.
