# Product App Source/Test Target Migration Scout v1

## Objective

Map the source and test work required if `iggy3d_visual_demo` is reduced or deprecated in favor of the product app path `./build/iggy3d`, without losing current renderer/package visual coverage.

This is a scout packet only. Do not edit source, CMake, tests, fixtures, schemas, or Vulkan docs from this document alone.

## Current Target Map

| Target / test family | Current executable | Output / path macro | Current role |
| --- | --- | --- | --- |
| `iggy3d_app` | `apps/iggy3d/main.cpp` | output name `./build/iggy3d` | Product app. Starts at product frontend, owns product view receipts and no-window scripted product proof. |
| `iggy3d_visual_demo` | `apps/iggy3d_visual_demo/main.cpp` | `./build/iggy3d_visual_demo` | Compatibility/package visual shell. Still owns several package-renderer, editor, dev-menu, and old control-file smokes. |
| `product_*` smokes | product app | `IGGY3D_PRODUCT_APP_PATH` | Product no-window proof. Currently `product_gameplay_controls_smoke` and `product_menu_transition_smoke`. |
| `package_visual_*` smokes | visual shell | `IGGY3D_VISUAL_DEMO_PATH` | Package visual shell and renderer/package coverage. Registered only when `iggy3d_visual_demo` target exists. |

Source facts:

- `CMakeLists.txt` always builds `iggy3d_app` and sets its output name to `iggy3d`.
- `CMakeLists.txt` builds `iggy3d_visual_demo` when `IGGY3D_BUILD_TOOLS OR IGGY3D_ENABLE_VISUAL_DEMO`.
- `cmake/iggy3d_tests.cmake` registers `package_visual_*` smokes inside `if(TARGET iggy3d_visual_demo)` and injects `IGGY3D_VISUAL_DEMO_PATH`.
- `cmake/iggy3d_tests.cmake` registers product smokes against `iggy3d_app` and injects `IGGY3D_PRODUCT_APP_PATH`.

## Current Smoke Dependency Map

Package visual shell smokes:

- `package_visual_startup_smoke`
- `package_visual_window_smoke`
- `package_visual_playable_proxy_smoke`
- `package_visual_room_asset_smoke`
- `package_visual_movement_playground_smoke`
- `package_visual_codex_control_smoke`
- `package_visual_bean_models_smoke`
- `package_visual_editor_control_smoke`
- `package_visual_editor_save_load_smoke`
- `package_visual_editor_manipulation_smoke`
- `package_visual_opening_menu_smoke`
- `package_visual_starter_screen_smoke`
- `package_visual_ingame_menu_smoke`
- `package_visual_menu_usefulness_smoke`

Product app smokes:

- `product_gameplay_controls_smoke`
- `product_menu_transition_smoke`

Unit tests already using product-owned seams:

- `product_primitive_draw_list_tests`
- `product_viewport_framing_tests`
- `product_render_bridge_tests`
- `product_gameplay_feedback_tests`
- `product_menu_transitions_tests`

## Feature Gap Table

| Feature | Product app `./build/iggy3d` | Visual shell `iggy3d_visual_demo` | Migration note |
| --- | --- | --- | --- |
| `--renderer null|vulkan` | Parsed and reported through product settings/receipts; not renderer-backend proof. | Supported plus `auto`, `--require-renderer`, `--strict-vulkan`. | Keep renderer stress/Vulkan strict behavior on visual shell until product app has an explicit renderer-test mode. |
| `--window` / `--no-window` | Supported. | Supported. | Product no-window should be preferred for CI proof. |
| `--input keyboard|gamepad|auto` | Supported. | Supported through visual input backend. | Product input path is current product truth. |
| `--package` | Supported as alias for `--dev-package-override`. | Primary package/fixture input. | Product docs should prefer `--dev-package-override` when a fixture override is necessary. |
| `--dev-package-override` | Supported. | Not supported. | Product-only name for explicit package override. |
| `--frames` / `--hold-seconds` | Supported. | Supported. | Safe parity candidate for bounded no-window/window runs. |
| `--dev-menu` | Not a product CLI flag. | Supported; enables old visual dev menu. | Do not invent this on product path. Product dev access should route through product frontend/dev tools. |
| `--codex-control` | Not supported. | Supported; used by many `package_visual_*` smokes. | Migrate to product `--automation-control` / `--script-control` only where the product app supports equivalent behavior. |
| `--automation-control` / `--script-control` | Parsed into `ProductAppOptions::automationControlPath`, but not consumed by `AppShell` yet. | Not supported. | Product path is the neutral future name; first migration work must add product automation semantics before tests rely on this flag. |
| `--opening-menu` / `--no-opening-menu` | Not supported. Product app starts at starter unless scripted/auto flags explicitly launch. | Supported. | Product app should not inherit these names unless a dev-only equivalent is deliberately added. |
| `--scripted-gameplay-smoke` | Supported and auto-launches product gameplay. | Not supported. | Current product gameplay proof path. |
| Startup receipt | `app=iggy3d`, product/frontend fields. | `app=iggy3d_visual_demo`, package visual fields. | Tests must assert the correct app family. |
| Package runtime lookup proof | Partially through product default package/dev override. | Strongly covered by package visual smokes. | Keep visual shell until product package-mode receipts are equivalent. |
| Renderer package-room Vulkan proof | Not current product proof. | Covered by `package_visual_room_asset_smoke` and related visual smokes. | Do not migrate until product renderer bridge consumes package-room rendering or a product renderer-test mode exists. |
| Editor/dev-menu controls | Product app has product frontend/dev tools, but product automation control is not wired to behavior yet. | Broad `--codex-control` smokes cover editor, traversal, movement playground, dev menu, save/load. | Migrate in small parity packets by behavior area after product automation intake exists. |

## Receipts And Proof Surfaces

Product app receipt strengths:

- product frontend state: `frontend_screen`, `starter_world_suppressed`, `gameplay_active`;
- product view and controls: `gameplay_view_visible`, `product_draw_*`, `product_view_*`, `product_feedback_*`;
- product transition proof: `product_transition_*`;
- product renderer bridge summary: `product_render_bridge_ready`, `product_view_frame_ready`, `product_feedback_bridge_ready`;
- scripted no-window gameplay proof through `--scripted-gameplay-smoke`.

Visual shell receipt strengths:

- package visual startup and build-tree package lookup;
- renderer-specific visual/package fields;
- Vulkan/package-room proof such as `rendering_path=package_room_meshes`, `record_mode=room_mesh_draws`, `room_asset_loaded=true`;
- movement playground/traversal/dev-menu/editor automation via `--codex-control`;
- explicit `opening-menu` and `no-opening-menu` compatibility states.

## Migration Options

### Option 1: Keep Visual Shell Permanently As Package Renderer Harness

Keep `iggy3d_visual_demo` and all `package_visual_*` tests as renderer/package harnesses. Product app remains the game surface.

Pros:

- Lowest risk to Vulkan/package-room/editor coverage.
- Avoids inventing product flags just to satisfy old smokes.
- Keeps window-heavy renderer proof isolated from product no-window proof.

Cons:

- Continued naming debt.
- Two app paths must be understood by builders.

### Option 2: Migrate Selected `package_visual_*` Smokes To Product Equivalents

Migrate only tests where product app already has equivalent semantics:

- starter/menu transition cases to product app smokes;
- no-window frontend/menu usefulness cases after adding minimal product automation intake, or with direct product no-window receipts when no automation is needed;
- selected editor/save/load cases after product app automation parity is proven.

Keep renderer/package-room/Vulkan visual smokes on visual shell.

Pros:

- Reduces old visual shell use without losing renderer coverage.
- Moves product/menu/editor behavior toward current product ownership.
- Keeps CI no-window-first.

Cons:

- Requires careful receipt parity design.
- Product `--automation-control` is currently parsed but not executed by `AppShell`.
- Some old `--codex-control` commands may not have product automation equivalents.

### Option 3: Rename Or Deprecate Visual Shell After Full Parity

Only after product app covers package renderer proofs or a new renderer harness replaces visual shell:

- rename visual test harness to an explicit name such as `iggy3d_package_visual_harness`;
- or remove it after tests no longer need it.

Pros:

- Cleans product naming fully.

Cons:

- Highest risk now.
- Would touch CMake, install rules, many smoke tests, docs, and possibly Vulkan/package lookup expectations.
- Not appropriate until parity is proven.

## Recommendation

Use Option 2 next: migrate selected no-window product-equivalent smokes first, while keeping `iggy3d_visual_demo` as the package renderer harness.

Do not remove or rename `iggy3d_visual_demo` yet.

Do not migrate these yet:

- `package_visual_room_asset_smoke`;
- `package_visual_window_smoke`;
- `package_visual_playable_proxy_smoke`;
- renderer/Vulkan/package-room smokes;
- movement playground cases that rely on `--dev-menu` and old `--codex-control` until product automation has implemented equivalent commands.

## Recommended Next Builder Packet

Name: `Product App Smoke Migration v1 - Frontend/Menu No-Window Parity`

Goal: move product-equivalent frontend/menu no-window coverage away from `IGGY3D_VISUAL_DEMO_PATH` and onto `IGGY3D_PRODUCT_APP_PATH`, without touching renderer/package-room visual smokes.

Approved files:

- `cmake/iggy3d_tests.cmake`
- `tests/smoke/product_menu_transition_smoke.cpp`
- new `tests/smoke/product_menu_usefulness_smoke.cpp` if needed
- new `tests/smoke/product_opening_menu_smoke.cpp` only if it reflects product starter behavior rather than old `--opening-menu`
- `src/app/iggy3d/ProductAppOptions.*` only if option parsing needs a small compatibility fix
- `src/app/iggy3d/AppShell.cpp` only for minimal automation intake/routing needed by product no-window smokes
- relevant product docs after source/test behavior exists

No-go files:

- `docs/vulkan/**/*.md`
- renderer/Vulkan source
- runtime/session/save gameplay logic
- fixtures/package schemas
- `package_visual_room_asset_smoke.cpp` and other renderer/package-room visual smokes unless explicitly scoped
- broad rename of `iggy3d_visual_demo`

Implementation semantics:

- Prefer product no-window commands:

```sh
./build/iggy3d --no-window --print-render-receipt
./build/iggy3d --no-window --scripted-gameplay-smoke --print-render-receipt
```

- Product automation cases require implementation first. The flag currently parses
  but does not execute control-file behavior. After the packet adds explicit
  product automation intake, use:

```sh
./build/iggy3d --no-window --automation-control <path> --print-render-receipt
```

- Do not add product `--opening-menu` / `--no-opening-menu`; product startup semantics already define starter-vs-gameplay through default, `--auto-new-world`, and scripted smoke flags.
- Do not add product `--dev-menu`; dev access belongs to product frontend/dev tools, not the old visual shell CLI.
- Do not use `--codex-control` in new product tests.

Acceptance checks:

```sh
cmake --build build --target iggy3d_app
cmake --build build --target product_gameplay_controls_smoke
cmake --build build --target product_menu_transition_smoke
cmake --build build --target product_menu_usefulness_smoke
ctest --test-dir build --output-on-failure -R '^product_(gameplay_controls|menu_transition|menu_usefulness)_smoke$'
git diff --check
```

No app/window launch is required. `window_launch_count=0` should remain expected for product no-window smokes.

## Later Migration Packets

### Product Automation Parity v1

Implement and then port selected old visual control-file behaviors to product
`--automation-control` only where product gameplay/editor/frontend owns the
behavior. Parsing already exists; behavior execution does not.

Candidate old tests to split:

- `package_visual_menu_usefulness_smoke` -> product no-window menu usefulness smoke;
- `package_visual_opening_menu_smoke` and `package_visual_starter_screen_smoke` -> product starter/menu transition smokes;
- `package_visual_ingame_menu_smoke` -> product pause/settings/dev menu smoke.

Keep old tests until product equivalents are green.

### Product Editor Automation Parity v1

Migrate editor save/load/manipulation no-window cases only after product app automation exposes equivalent editor commands.

Candidate old tests:

- `package_visual_editor_control_smoke`;
- `package_visual_editor_save_load_smoke`;
- `package_visual_editor_manipulation_smoke`.

### Renderer Harness Naming v1

If the visual shell remains, rename it intentionally as a renderer/package harness instead of pretending it is the product app. This should be a separate source/CMake/test/docs packet.

Potential names:

- `iggy3d_package_visual_harness`;
- `iggy3d_renderer_package_harness`.

Do not start this until product migration and renderer coverage strategy are accepted.

## Risks

- Losing Vulkan/package-room coverage if `package_visual_*` tests are moved before product renderer parity exists.
- Inventing product flags to mirror old visual-shell behavior instead of using product semantics.
- Mixing product gameplay ownership with renderer test harness ownership.
- Window-launch spam if renderer/package smokes are moved into product smoke lanes without no-window separation.
- Treating `--codex-control` as product-facing instead of implementing the
  neutral `--automation-control` / `--script-control` path.
- Removing `iggy3d_visual_demo` while package runtime lookup tests and install/package docs still model visual-shell layouts.

## Verification For This Scout

Scouting commands:

```sh
rg -n "iggy3d_visual_demo|IGGY3D_VISUAL_DEMO_PATH|package_visual_|product_" CMakeLists.txt cmake tests apps src -g '*.*'
rg -n --glob '*.*' -- "--dev-menu|--codex-control|--automation-control|--script-control|--opening-menu|--no-opening-menu|--package|--dev-package-override|--scripted-gameplay-smoke" apps src tests docs
git diff --check
```

No app/window launch, compile, or CTest is required for this scout.
