# E121: Creative No-Window Bake Scenario Gate

## Objective

Create a focused no-window truth-gate test for the Creative document -> RoomBake
-> product active-room/collision path.

This is the second foundation gate from the ownership-map packet. The test
should prove a real CreativeDocument edit sequence can create, move, delete,
undo, and bake without launching a window.

## Why This Exists

Bad cards come from weak gates and fuzzy ownership. The receipt key-order oracle
now guards receipt splits. The next missing gate is Creative no-window bake
truth:

- CreativeDocument remains authored truth.
- RoomBake is the Creative -> runtime room adapter.
- Product active-room/collision refresh consumes the baked RoomAsset.
- Snapshot undo restores document truth.

Before broader kernel/job/bake-freeze work, this path needs one compact scenario
that catches broken create/move/delete/undo/bake behavior without relying on the
standalone capture app or a live product window.

## Required Work

1. Add a focused unit test target.
   - Suggested test file:
     - `tests/unit/product_creative_no_window_bake_scenario_tests.cpp`
   - Suggested target:
     - `product_creative_no_window_bake_scenario_tests`
   - Register it in `cmake/iggy3d_tests.cmake`.
2. Use existing public seams only.
   - Launch or set up Creative no-window state using existing product/creative
     helpers and public APIs.
   - Use `creative::Facade` / `CreativeDocument` APIs for document edits.
   - Use `refreshProductCreativeBakedActiveRoom(...)` for product active-room
     and collision refresh.
   - Use the app-owned snapshot undo seam from `CreativeAppState` for undo,
     not direct object-vector mutation.
3. Build one deterministic scenario:
   - Start from an empty Creative document/session state with no live window.
   - Create at least one Floor and one Crate or equivalent renderable structural
     objects through public create APIs.
   - Bake/refresh and assert:
     - refresh accepted;
     - static mesh count includes the authored renderables;
     - spatial/collision query surfaces are ready and nonzero;
     - bake receipt/revision fields are coherent.
   - Push a snapshot, move one renderable object through a public mutation/move
     seam, bake/refresh, and assert the baked mesh center or equivalent RoomBake
     output changes as expected.
   - Push a snapshot, delete one renderable object through the public removal
     seam, bake/refresh, and assert counts drop as expected.
   - Apply undo through the app-owned undo stack, bake/refresh, and assert the
     deleted object and baked active-room/collision counts are restored.
4. Prefer direct assertions over text-only receipt scraping.
   - Inspect structured results where available:
     - `ProductCreativeBakedActiveRoomRefreshResult`
     - `CreativeRoomBakeReceipt`
     - `ProductAppWindowState.activeRoom`
     - `ProductAppWindowState.activeRoomCollision`
     - document object count/revision/dirty state as needed.
5. Keep this as a truth gate, not a UI test.
   - Do not click UI rows.
   - Do not route through SDL, Vulkan, standalone capture, or live windows.
   - Existing input-frame tests already cover product UI command routing; this
     card should prove the model/bake/runtime refresh chain.

## Acceptance Notes

The exact counts depend on the chosen objects, but the test should pin them
explicitly. For the common Floor + Crate case, expected shape is likely:

- static meshes: 2 (`floor`, `prop`)
- spatial surfaces: 3 (1 walkable floor + 2 crate blocker surfaces)
- collision query surfaces: 3

If the chosen fixture differs, explain the exact count policy in the completion
brief.

## Do Not

- Do not launch a window.
- Do not run standalone capture.
- Do not add or change UI behavior.
- Do not change RoomBake classification policy.
- Do not change CreativeDocument mutation semantics.
- Do not change save/load, renderer/Vulkan, runtime simulation, or descriptors.
- Do not make this a broad integration mega-test.
- Do not stage, commit, or push.

## Suggested Verification

```sh
cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_no_window_bake_scenario_tests creative_document_room_bake_tests product_creative_world_launch_tests -j10
ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_no_window_bake_scenario_tests|creative_document_room_bake_tests|product_creative_world_launch_tests)$' --output-on-failure
git -C /Users/kogaryu/iggy3d diff --check
```

Run a focused trailing-whitespace scan over touched files.

## Completion Brief

Append:

- Files changed:
- Scenario shape:
- Bake/refresh assertions:
- Tests/checks run:
- Concerns/deferred:

## Completion Brief

- Files changed:
  - `cmake/iggy3d_tests.cmake`
  - `tests/unit/product_creative_no_window_bake_scenario_tests.cpp`
  - `docs/creative_mode/builder_tasks/done/E121-creative-no-window-bake-scenario-gate.md`
- Scenario shape:
  - Added focused target `product_creative_no_window_bake_scenario_tests`.
  - Launches a blank Creative world through `launchProductCreativeNewWorld(...)` using `ProductWindowMode::NoWindow`.
  - Creates Floor and Crate through `creative::Facade::createDocumentObject(...)` with authored bounds and truthful `transform.position` anchors at bounds centers.
  - Refreshes product active-room/collision through `refreshProductCreativeBakedActiveRoom(...)`.
  - Pushes an app-owned snapshot with `creative::pushCreativeUndoSnapshot(...)`, moves the Crate through `creative::moveDocumentObject(...)`, refreshes, and checks the baked prop center changed.
  - Pushes another snapshot, deletes the Crate through `creative::Facade::removeDocumentObject(...)`, refreshes, and checks Floor-only bake/collision counts.
  - Restores through `creative::applyLastCreativeUndoSnapshot(...)`, refreshes, and checks the deleted Crate and baked active-room/collision counts return.
- Bake/refresh assertions:
  - Floor+Crate refresh pins accepted status, document id/object count, bake measured flag, baked document revision, `creative_room_baked` receipt reason, 2 static meshes, 3 spatial surfaces, 2 static mesh source records, 3 spatial surface source records, active-room loaded, collision ready, and collision query surface count 3.
  - Move phase pins the Crate prop mesh center from `{1.5, 0.5, 5.5}` to `{4.5, 0.5, 7.5}`.
  - Delete phase pins 1 static mesh, 1 spatial surface, no prop mesh, active-room loaded, collision ready, and collision query surface count 1.
  - Undo phase pins the snapshot receipt, restored document revision/object count, restored Crate object, restored Floor+Crate bake counts, and remaining undo depth.
- Tests/checks run:
  - `cmake --build /Users/kogaryu/iggy3d/build --target iggy3d product_creative_no_window_bake_scenario_tests creative_document_room_bake_tests product_creative_world_launch_tests -j10`
  - `ctest --test-dir /Users/kogaryu/iggy3d/build -R '^(product_creative_no_window_bake_scenario_tests|creative_document_room_bake_tests|product_creative_world_launch_tests)$' --output-on-failure`
  - `git -C /Users/kogaryu/iggy3d diff --check`
  - Focused trailing-whitespace scan over touched files.
- Concerns/deferred:
  - This is intentionally not a UI/input/Vulkan/live-window test; it gates the model -> RoomBake -> active-room/collision path directly.
  - The test uses a no-op activation hook so it stays focused on bake/refresh/collision rather than reasoning-graph side effects.
