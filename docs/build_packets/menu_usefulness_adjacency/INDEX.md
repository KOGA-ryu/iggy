# Menu Usefulness Adjacent Planning Bundle

This bundle captures the next planning lanes adjacent to `Menu Usefulness v1`. It exists so Builder Dex can keep momentum after the current menu packet lands without mixing future policy, persistence, dev tools, editor UX, receipts, and gameplay work into one oversized implementation pass.

Hard rules across every packet:

- Do not implement source from this planning bundle until a specific builder packet is dispatched.
- Avoid JSON completely; use existing TOML/key-value/receipt formats.
- Preserve dirty worktree assumptions and never revert unrelated Builder Dex work.
- Prefer no-window/null/receipt tests. Keep `window_launch_count=0` unless a later packet explicitly justifies a window run.
- Runtime truth, save truth, projection/debug, renderer, frontend, and editor authoring stay separate.

## Recommended Implementation Order

1. `control_binding_policy.md`
   - Locks shared action names, menu/gameplay owner stack, keyboard/PS5/Codex semantics, and chord rules before adding more controls.
2. `settings_persistence.md`
   - Adds durable non-JSON settings only after Menu Usefulness has a stable settings model.
3. `debug_receipt_standard.md`
   - Standardizes field names/reason codes before more dev tools and gameplay receipts multiply.
4. `dev_tools_taxonomy.md`
   - Turns dev tools into a useful readout/command taxonomy using the receipt standard.
5. `world_creation_flow.md`
   - Hardens starter save hub and world/new-save semantics after menus and settings are stable.
6. `editor_ux_rules.md`
   - Defines practical editor manipulation UX rules over the current authored-room save/load and menu input stack.
7. `next_gameplay_packet.md`
   - Plans movement/physics playground hardening after menu/input/dev/debug scaffolding can prove it without window spam.

## Shared Current Source Anchors

Likely source files across this bundle:

- `/Users/kogaryu/iggy3d/apps/iggy3d/main.cpp`
- `/Users/kogaryu/iggy3d/src/app/iggy3d/**`
- `/Users/kogaryu/iggy3d/src/app/frontend/*`
- `/Users/kogaryu/iggy3d/src/app/input/GamepadSystemControls.*`
- `/Users/kogaryu/iggy3d/src/runtime/save/SaveFileStore.*`
- `/Users/kogaryu/iggy3d/src/content/authoring/EditableRoomDocument.*`
- `/Users/kogaryu/iggy3d/src/content/authoring/EditableRoomSession.*` if present later
- `/Users/kogaryu/iggy3d/src/runtime/movement/*`
- `/Users/kogaryu/iggy3d/src/runtime/traversal/*` if present
- `/Users/kogaryu/iggy3d/src/projection/debug/*`
- `/Users/kogaryu/iggy3d/tests/unit/*`
- `/Users/kogaryu/iggy3d/tests/smoke/*`
- `/Users/kogaryu/iggy3d/cmake/iggy3d_tests.cmake`
- `/Users/kogaryu/iggy3d/README.md`

## No-Window Verification Baseline

Every adjacent packet should include these patterns unless its dispatch says otherwise:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure -R '<focused-regex>'
./build/iggy3d_headless_demo --package fixtures/demos/first_room/package.iggy3d.toml --summary fixtures/demos/first_room/expected_summary.txt --save /tmp/iggy3d_adjacent_runtime.save
./build/iggy3d_replay_tool --package fixtures/demos/first_room/package.iggy3d.toml --save /tmp/iggy3d_adjacent_runtime.save --expect-summary fixtures/demos/first_room/expected_summary.txt
rg -n '#include[ <"](SDL3/|SDL\.h|SDL_vulkan|vulkan/)|\bVk[A-Z][A-Za-z0-9_]*|\bVK_[A-Z0-9_]+' src/runtime src/content src/projection src/runtime/save
rg -n '/Users/kogaryu/iggy|namespace runtime3d|iggy::three_d|nlohmann|rapidjson|\.json\b|JSON|Json|StatusCode' apps src tests cmake CMakeLists.txt
git diff --check
```

Window runs are not part of this planning bundle's default gates.
