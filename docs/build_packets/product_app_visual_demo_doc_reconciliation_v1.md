# Product App / Visual Demo Documentation Reconciliation v1

## Objective

Document the remaining places where repository docs still point builders or users at `iggy3d_visual_demo` or older visual-demo/product-surface language, then classify each reference before any broad edits are made.

This packet is an audit and migration map only. It must not delete historical Vulkan/file-plan context blindly, rename tests, change CMake targets, launch windows, or change source behavior.

## Current Product Launch Truth

The current product app root launch is `iggy3d`, built from `iggy3d_app`:

```sh
cmake --build build --target iggy3d_app -j 8
./build/iggy3d --window --input auto --save-root "$HOME/.iggy3d/saves" --print-render-receipt
```

No-window product receipt proof:

```sh
./build/iggy3d --no-window --print-render-receipt
```

Product gameplay scripted receipt proof:

```sh
./build/iggy3d --no-window --scripted-gameplay-smoke --print-render-receipt
```

The visual/package shell remains a compatibility and test shell while source/tests still reference it:

```sh
cmake --build build --target iggy3d_visual_demo -j 8
./build/iggy3d_visual_demo \
  --renderer vulkan \
  --window \
  --interactive \
  --dev-menu \
  --input auto \
  --package fixtures/demos/movement_playground/package.iggy3d.toml \
  --print-render-receipt
```

That command is not the current product root launch. It should be labeled compatibility/test shell wherever it remains.

## Inventory Snapshot

Pre-audit scan:

```sh
rg -n "iggy3d_visual_demo|./build/iggy3d|FIRST PERSON PROXY VIEW|proxy view|demo-only" README.md docs fixtures -g '*.md'
```

`iggy3d_visual_demo` appeared 86 times across 33 markdown files. The table below groups the remaining references by intended handling.

| Category | Files | Count | Handling |
| --- | --- | ---: | --- |
| Current product docs to migrate | `fixtures/demos/movement_playground/README.md`, `docs/roadmaps/complete_runtime_todo_roadmap.md`, `docs/build_packets/menu_usefulness_v1.md`, `docs/build_packets/menu_usefulness_adjacency/*.md` | 23 | Update in a docs-only migration packet so product app commands come first and visual shell commands are clearly labeled compatibility/test shell. |
| Historical accepted build packets | `docs/build_packets/product_app_boot_menu_structure_v1.md`, `docs/build_packets/visual_demo_decomposition_v1.md`, `docs/build_packets/runtime_packet_8_tactical_combat.md` | 21 | Leave as historical packet evidence unless adding a short status note. Do not rewrite accepted packet history into current-tense product docs. |
| Room/asset planning docs with stale launch examples | `docs/build_packets/room_asset_packet_1_map_toml_to_3d_room.md`, `docs/build_packets/room_asset_packet_2_loader_hardening_spatial_surface_contract.md` | 3 | Update in the same docs-only migration as movement playground docs. Use product app commands first; keep visual shell only if explicitly marked compatibility. |
| Vulkan/file-plan docs | `docs/vulkan/**/*.md` | 39 | Leave until an approved renderer docs cleanup or source/test target migration. These docs describe historical renderer packet plans and package/visual-shell compatibility. |
| Product view terminology docs | `docs/product_view_v1.md`, `docs/roadmap.md` | N/A in `iggy3d_visual_demo` count | Keep explicit stale-term explanations where they say old `proxy view` wording has been replaced. Remove only accidental user-facing proxy language. |

## Category Details

### Current Product Docs To Migrate

These docs can mislead a user or builder because they describe current product flows but still point to the old visual shell.

Recommended next docs-only edits:

- `fixtures/demos/movement_playground/README.md`
  - Put `./build/iggy3d` product commands first.
  - Keep no-window receipt command for fast verification.
  - Keep `iggy3d_visual_demo` only under a `Compatibility/Test Shell` heading if it still works.
  - Replace old `--codex-control` examples only when the source/test migration provides the neutral automation/script-control alias. Do not invent unsupported flags in docs.
- `docs/roadmaps/complete_runtime_todo_roadmap.md`
  - Mark product app boot and Product View v1 as current baseline.
  - Describe `apps/iggy3d` and `src/app/iggy3d` as the current product surface.
  - Describe `iggy3d_visual_demo` as a compatibility/test shell until source/tests migrate.
- `docs/build_packets/menu_usefulness_v1.md`
  - Keep the packet as historical where appropriate, but add current-status wording if the doc is still used as a live planning reference.
- `docs/build_packets/menu_usefulness_adjacency/*.md`
  - Update current source-file examples from old visual app assumptions to product app/front-end paths where they are not historical.

### Historical Accepted Build Packets

These files are prior packet evidence. They should not be globally rewritten as if they were current source truth.

- `docs/build_packets/product_app_boot_menu_structure_v1.md`
  - Already records the transition to the product app and has both product and compatibility language.
  - Leave intact unless a later docs packet adds a small `Current status` note.
- `docs/build_packets/visual_demo_decomposition_v1.md`
  - Leave as a historical decomposition plan for the old integration pile and Codex-branded automation-control migration.
  - Do not use it as current launch guidance.
- `docs/build_packets/runtime_packet_8_tactical_combat.md`
  - Leave historical references unless they are used as current builder instructions.

### Room/Asset Planning Docs

These are planning documents that may become active again. Update their launch examples before sending them to a builder.

- `docs/build_packets/room_asset_packet_1_map_toml_to_3d_room.md`
- `docs/build_packets/room_asset_packet_2_loader_hardening_spatial_surface_contract.md`

Required migration:

- Product command first: `./build/iggy3d ...`.
- Compatibility shell command second, clearly labeled.
- No source/test renames in the docs-only packet.

### Vulkan/File-Plan Docs

Do not rewrite these in this reconciliation packet:

- `docs/vulkan/file_plans/*`
- `docs/vulkan/platform_shell.md`
- `docs/vulkan/packaging.md`
- `docs/vulkan/vulkan_package_runtime_lookup.md`
- `docs/vulkan/vulkan_renderer_config.md`
- `docs/vulkan/renderer_file_plan_order.md`
- related Vulkan smoke, WSI, and policy docs

Reason: source and tests still contain visual/package shell concepts, and these docs preserve historical renderer packet context. Broadly removing `iggy3d_visual_demo` here before a source/test target migration would erase useful context and create contradictions with existing test names and CMake paths.

### Product View Terminology

`FIRST PERSON PROXY VIEW` should not appear as the current user-facing label. It may remain only in docs that explicitly explain it as stale wording; the SDL/null fallback now labels itself `TOP-DOWN DEBUG FALLBACK`, while first-person manual acceptance belongs to the Vulkan renderer path.

`demo-only` and `proxy view` are acceptable only when:

- describing historical text,
- documenting a deferred renderer/asset limitation,
- or explicitly saying the old wording is no longer current product wording.

## Future Builder Packet Recommendation

### Packet A: Docs-Only Product Launch Migration

Approved files:

- `README.md` only if current product launch wording needs a small clarification.
- `fixtures/demos/movement_playground/README.md`
- `docs/roadmaps/complete_runtime_todo_roadmap.md`
- `docs/build_packets/menu_usefulness_v1.md`
- `docs/build_packets/menu_usefulness_adjacency/*.md`
- `docs/build_packets/room_asset_packet_1_map_toml_to_3d_room.md`
- `docs/build_packets/room_asset_packet_2_loader_hardening_spatial_surface_contract.md`

Forbidden:

- source files,
- CMake files,
- tests,
- Vulkan historical/file-plan docs,
- fixture data,
- package schema files.

Acceptance:

- product app commands are first in current product docs,
- visual shell commands are explicitly labeled compatibility/test shell,
- stale `FIRST PERSON PROXY VIEW` user-facing wording is absent,
- no app/window launches.

### Packet B: Source/Test Target Migration

Only after Packet A lands, decide whether to:

- keep `iggy3d_visual_demo` as a permanent compatibility binary,
- rename package visual smoke tests,
- migrate `package_visual_*` tests to product app no-window/window-safe equivalents,
- or deprecate visual shell flags in favor of product app flags.

This packet may touch CMake and tests. It must not be mixed into the docs-only migration.

### Packet C: Vulkan Historical Docs Cleanup

Only after the renderer/source target strategy is approved, audit Vulkan docs separately. Keep historical packet context unless replacing it with a clearly versioned current renderer docs set.

## No-Go Surfaces

- Do not rename tests in this reconciliation packet.
- Do not edit source or CMake.
- Do not edit Vulkan historical manuals broadly.
- Do not remove `iggy3d_visual_demo` references from CMake/test docs unless a source/test migration is in scope.
- Do not launch the app or any window.
- Do not add JSON or new machine-contract formats.
- Do not change save, runtime, renderer, or frontend behavior.

## Acceptance Checks For The Future Docs Packet

Run from `/Users/kogaryu/iggy3d`:

```sh
rg -n "iggy3d_visual_demo|./build/iggy3d|FIRST PERSON PROXY VIEW|proxy view|demo-only" README.md docs fixtures -g '*.md'
rg -n "./build/iggy3d_visual_demo" fixtures docs/build_packets docs/roadmaps -g '*.md'
rg -n "iggy3d_visual_demo" docs/vulkan -g '*.md'
git diff --check
```

No-window product receipt commands are acceptable for the future migration packet if the user approves proof commands:

```sh
./build/iggy3d --no-window --print-render-receipt
./build/iggy3d --no-window --scripted-gameplay-smoke --print-render-receipt
```

Do not run window launches as part of this reconciliation audit.
