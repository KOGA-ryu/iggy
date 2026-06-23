# iggy3d Complete Runtime Todo Roadmap

Updated: 2026-06-23

Purpose: maintain the implementation-order todo list from the current playable/editor prototype toward a shippable internal demo runtime. This is the working roadmap for builders. It is not a starter plan and it is not a marketing plan.

Hard rules:

- No JSON. Use TOML-style, key-value control files, save text, and receipt fields.
- No old `/Users/kogaryu/iggy` dependency.
- No repeated window-launch verification. Prefer unit, headless, replay, null renderer, and receipt smokes.
- Keep `window_launch_count=0` for automated packet verification unless a packet explicitly requires manual visible proof.
- Preserve runtime truth, save truth, frontend state, editor authoring truth, renderer output, and debug receipts as separate ownership layers.
- Do not keep adding product behavior directly to `apps/iggy3d_visual_demo/main.cpp`.

## Current Snapshot

The repo currently has a working standalone `iggy3d` runtime with:

- deterministic runtime/session/save/replay base;
- first-room headless acceptance proof;
- Vulkan/null renderer boundary and visual demo app;
- movement playground, first-person controls, controller support, dev HUD/menu surfaces;
- starter/pause/settings/dev/menu usefulness work;
- in-game editable room authoring with save/load/manipulation smokes;
- product app boot/menu scaffold work in progress;
- build packets for menu usefulness, adjacent menu planning, product boot, room assets, Vulkan, and visual demo decomposition.

Primary current risk:

- `apps/iggy3d_visual_demo/main.cpp` is over 7k LOC and still owns too many responsibilities.
- Several packets are dirty in the worktree at once. Before large new implementation, stabilize and commit coherent slices.

## Implementation Order

### 0. Stabilize Current Worktree

Goal: turn current dirty work into auditable commits before more feature work.

Todos:

- [ ] Review current dirty files and split them into coherent commit groups.
- [ ] Verify editor manipulation packet independently.
- [ ] Verify menu usefulness packet independently.
- [ ] Verify product app boot scaffold independently if it is intended to stay.
- [ ] Ensure all new untracked files are either intentionally added or removed.
- [ ] Run focused no-window CTest after each group.
- [ ] Run headless/replay after final group.
- [ ] Commit only after the slice is verified.

Acceptance:

```sh
cmake -S . -B build
cmake --build build -j 8
ctest --test-dir build --output-on-failure -R 'editable_room|visual_editor|menu_input|pause_menu|frontend|starter|settings|dev_tools|opening|ingame_menu|menu_usefulness|save|replay'
./build/iggy3d_headless_demo --package fixtures/demos/first_room/package.iggy3d.toml --summary fixtures/demos/first_room/expected_summary.txt --save /tmp/iggy3d_stabilize.save
./build/iggy3d_replay_tool --package fixtures/demos/first_room/package.iggy3d.toml --save /tmp/iggy3d_stabilize.save --expect-summary fixtures/demos/first_room/expected_summary.txt
git diff --check
```

### 1. Visual Demo Decomposition

Goal: split `apps/iggy3d_visual_demo/main.cpp` into behavior-preserving modules before adding more gameplay/editor/menu work.

Reference packet:

- `docs/build_packets/visual_demo_decomposition_v1.md`

Todos:

- [ ] Extract `VisualDemoOptions` for CLI/options parsing.
- [ ] Rename Codex-branded control surfaces to neutral automation naming.
- [ ] Keep no-window scripted control capability.
- [ ] Extract `AutomationControl` from the current control-file parser.
- [ ] Extract `ReceiptBuilder` for app-specific receipt fields.
- [ ] Extract `DebugHudController` for app debug HUD line assembly.
- [ ] Extract `FrontendController` for starter/pause/settings/dev transitions.
- [ ] Extract `EditorController` for editor state, cursor/probe/ghost, and authored-room command calls.
- [ ] Extract `SaveBridge` for visual-demo save/load orchestration.
- [ ] Extract `InputRouter` for keyboard/gamepad/mouse/automation normalization.
- [ ] Reduce `apps/iggy3d_visual_demo/main.cpp` to a thin entrypoint.

Acceptance:

- Same no-window receipts before and after extraction, except explicitly renamed automation fields.
- No gameplay, renderer, save format, or editor behavior changes.
- `package_visual_*` smokes rebuild the current `iggy3d_visual_demo` binary before running.

### 2. Product App Boot Contract

Goal: make the game boot as a product app, not as a fixture-driven visual demo.

Reference packet:

- `docs/build_packets/product_app_boot_menu_structure_v1.md`

Todos:

- [ ] Add or finish `apps/iggy3d/main.cpp` as the product entrypoint.
- [ ] Add or finish `src/app/iggy3d/AppShell.*`.
- [ ] Add product default world/template lookup.
- [ ] Make `./build/iggy3d` boot to the starter menu.
- [ ] Do not load gameplay behind the starter menu.
- [ ] Keep `--package` as dev-only or deprecated compatibility.
- [ ] Add product app receipts for starter menu, settings, input backend, renderer mode, save root, and launch state.
- [ ] Update README launch command to product app first.
- [ ] Keep `iggy3d_visual_demo` as a dev/test shell until fully replaced.

Acceptance:

```sh
cmake --build build --target iggy3d_app
./build/iggy3d --renderer null --no-window --print-render-receipt
```

Expected receipt fields:

```text
opening_menu=true
frontend_screen=starter
frontend_status=opening_menu_ready
frontend_launch_requested=false
starter_world_suppressed=true
settings_renderer=null
settings_window_mode=no_window
result=pass
```

### 3. Control Binding Policy

Goal: make keyboard, PS5 controller, future Joy-Con, and automation controls speak the same action language.

Reference doc:

- `docs/build_packets/menu_usefulness_adjacency/control_binding_policy.md`

Todos:

- [ ] Define final action names for menu, editor, debug, gameplay, and system controls.
- [ ] Route physical keyboard inputs through `MenuInput` semantics where possible.
- [ ] Route PS5 D-pad/sticks/Cross/Circle/L1/R1 through the same menu semantics.
- [ ] Keep hard quit as a chord: Create + Options.
- [ ] Keep destructive actions behind confirmation or explicit chord.
- [ ] Add `menu.input_frames` or equivalent frame-gated automation input.
- [ ] Add neutral `automation_control` key names and deprecate Codex-branded names.
- [ ] Document controller mappings in README.

Acceptance:

- Keyboard, controller, and automation can navigate the same starter/pause/settings/dev menu rows.
- Gameplay does not receive input while UI owns input.
- No destructive command executes from a single accidental press.

### 4. Settings Persistence

Goal: make settings useful and durable without overbuilding profiles.

Reference doc:

- `docs/build_packets/menu_usefulness_adjacency/settings_persistence.md`

Todos:

- [ ] Define settings file path under the app save/settings root.
- [ ] Use TOML-style text only.
- [ ] Persist input backend, look sensitivity, invert look, controller sensitivity, accessibility flags, developer tools enabled, and window/render preferences where appropriate.
- [ ] Keep gameplay/debug-only toggles runtime-only until they prove useful.
- [ ] Add apply, restore defaults, and back semantics.
- [ ] Add invalid-settings fallback with receipt diagnostics.
- [ ] Add settings load/save unit tests.
- [ ] Add no-window smoke proving settings survive restart.

Acceptance:

```text
settings_loaded=true
settings_saved=true
settings_status=settings_loaded|settings_defaults_restored|settings_invalid_fallback
settings_persistence=saved
```

### 5. Debug and Receipt Standard

Goal: standardize receipts before more runtime systems multiply status fields.

Reference doc:

- `docs/build_packets/menu_usefulness_adjacency/debug_receipt_standard.md`

Todos:

- [ ] Define naming rules for status, reason, count, enabled, visible, selected, owner, and result fields.
- [ ] Separate receipt-only fields from dev HUD fields.
- [ ] Define stable reason code policy.
- [ ] Add tests for duplicate receipt keys where practical.
- [ ] Add receipt field docs for menu, editor, movement, save, renderer, and combat domains.
- [ ] Avoid generic status-code wrappers in gameplay-facing code.

Acceptance:

- New packets use stable lower snake case fields and reason codes.
- Receipts remain deterministic key-value text.
- Dev HUD consumes existing state and does not own runtime truth.

### 6. Dev Tools Taxonomy

Goal: make dev tools a useful internal runtime dashboard.

Reference doc:

- `docs/build_packets/menu_usefulness_adjacency/dev_tools_taxonomy.md`

Todos:

- [ ] Finalize dev tool categories: Runtime, Input, Movement, World, Editor, Renderer, Save, Combat, Physics.
- [ ] Mark each row as read-only or command-capable.
- [ ] Add readouts for tick, clock mode, player position, phase, speed, ground state, slope, traversal preview, active save, authored object counts, renderer backend, and controller state.
- [ ] Add command rows only when they are safe: save snapshot, toggle debug overlay, reset playground, spawn test projectile, dump receipt.
- [ ] Add receipt fields for selected dev category, selected row, enabled state, command name, and readout count.
- [ ] Keep command-capable dev tools behind menu ownership.

Acceptance:

- Dev tools can answer: what mode am I in, what input is active, what is player movement doing, what world/editor object is selected, what renderer/backend is running, what save is active.

### 7. World Creation and Save Browser

Goal: make New World / Continue / Existing Saves / Delete behave like a real game front door.

Reference doc:

- `docs/build_packets/menu_usefulness_adjacency/world_creation_flow.md`

Todos:

- [ ] Define `world_id`, display name, package id, scenario id, created time, last played, tick, and authored room counts in save metadata.
- [ ] Make New World create a save-backed world, not just enter a demo.
- [ ] Make Continue load latest compatible save.
- [ ] Make Existing Saves show compatible, incompatible, and corrupt rows honestly.
- [ ] Add Delete Save confirmation.
- [ ] Add save preview receipt fields.
- [ ] Keep gameplay suppressed until a save/new world is explicitly chosen.
- [ ] Add no-window startup/save browser smokes.

Acceptance:

```text
frontend_screen=starter
starter_world_suppressed=true
save_count=<n>
compatible_save_count=<n>
selected_save_compatible=true|false|unavailable
delete_confirm_required=true|false
```

### 8. Editor UX and World Authoring

Goal: make practical in-game room/world editing durable enough for repeated authoring.

Reference doc:

- `docs/build_packets/menu_usefulness_adjacency/editor_ux_rules.md`

Todos:

- [ ] Finish selection semantics for floors, walls, props, and later entities.
- [ ] Add grid snap sizes and snap-mode receipts.
- [ ] Add transform modes: move, resize, stretch, rotate, height, thickness, semantics.
- [ ] Keep `EditableRoomDocument` as authored geometry truth.
- [ ] Keep runtime room/collision/traversal as rebuilt output.
- [ ] Add locked, hidden, disabled object behavior.
- [ ] Add controller/keyboard/editor action bindings.
- [ ] Add save/load roundtrip for all authored fields.
- [ ] Add editor dev HUD readouts.
- [ ] Add object palette only after core manipulation is stable.

Acceptance:

- User can create a simple room, edit it, save it, load it, and keep editing.
- Collision/traversal rebuild after every applied edit.
- No renderer ownership of authored truth.

### 9. Real 3D Room and Asset Pipeline

Goal: move from hardcoded primitives to real room data and object placement.

Relevant packets/docs:

- `docs/build_packets/room_asset_packet_1_map_toml_to_3d_room.md`
- `docs/build_packets/room_asset_packet_1_render_proof_contract.md`
- `docs/build_packets/room_asset_packet_2_loader_hardening_spatial_surface_contract.md`
- `docs/roadmaps/real_3d_room_runtime_roadmap.md`

Todos:

- [ ] Define room asset source truth independent of visual demo.
- [ ] Load floors, walls, props, spawn points, traversal tags, and collision tags from room data.
- [ ] Separate visible mesh from collision shape.
- [ ] Support authored room edits as deltas or native room documents.
- [ ] Add prop/object placement with ids and semantics.
- [ ] Add Blender/export path later, after native room data is stable.
- [ ] Add material ids and simple unlit textures.
- [ ] Add room package tests and no-window render receipt proof.

Acceptance:

- A test room is loaded from data, not hardcoded scene code.
- Objects in the room have ids, transforms, semantics, collision, and receipt visibility.

### 10. Movement and Physics Playground Hardening

Goal: make movement feel testable, explainable, and extensible before combat complexity grows.

Reference doc:

- `docs/build_packets/menu_usefulness_adjacency/next_gameplay_packet.md`

Todos:

- [ ] Define movement math module ownership for slopes, speed multipliers, acceleration, air control, and ground snapping.
- [ ] Add slope policy bands with speed/stamina/footing multipliers.
- [ ] Add grounded/falling/jumping/dashing/clambering/wire-walking motor phases.
- [ ] Add jump acceleration and air strafing parameters.
- [ ] Add crouch, vault, clamber, dash, wire-walk test zones in the movement playground.
- [ ] Add wall-slot detection for clamber, not just button press.
- [ ] Add traversal affordance registry for wall/floor types.
- [ ] Add collision query debug receipts for ground, wall, ledge, and traversal previews.
- [ ] Add movement dev HUD readouts for position, speed, grade, ground normal, phase, and selected affordance.

Acceptance:

- Movement is controlled by explicit hardcoded parameters that are easy to later move into data.
- Slopes, jumps, air movement, and traversal slots are receipt-testable.

### 11. Physics Engine Boundary

Goal: decide and build the physics seam without letting physics own gameplay truth.

Todos:

- [ ] Define minimum physics world state: bodies, transforms, velocities, shapes, materials, static/dynamic/kinematic.
- [ ] Define fixed physics timestep.
- [ ] Decide initial implementation: bespoke kinematic controller first, external physics later only behind an interface.
- [ ] Add collider shapes: capsule, box, sphere, convex hull, triangle mesh/heightfield later.
- [ ] Add query system: raycast, sweep, overlap, shape cast.
- [ ] Add collision material semantics: friction, bounce, surface type.
- [ ] Add trigger/event semantics.
- [ ] Add debug visualization receipts for colliders, normals, contacts, velocity, broadphase bounds.
- [ ] Add determinism policy for replay/multiplayer.

Acceptance:

- Player movement and projectiles use physics queries through a stable interface.
- Physics can be debugged without relying on visual inspection.

### 12. Projectiles and Abilities

Goal: support spells and tactical actions with real projectile motion, not hitscan-only behavior.

Todos:

- [ ] Define ability data ownership: id, cost, cooldown, cast time, projectile spec, movement override, targeting rules.
- [ ] Add projectile kinematics: position, velocity, acceleration, gravity scale, lifetime, collision radius.
- [ ] Add projectile collision against world surfaces and entities.
- [ ] Add impact events: damage, impulse, status, spawn effect marker.
- [ ] Add ability movement overrides that can bypass slope restraints while keeping wall collision.
- [ ] Add resource/cooldown tests.
- [ ] Add cleric/paladin and mage/rogue archetype boards as data seeds after mechanics stabilize.
- [ ] Add dev tool ability test launcher.

Acceptance:

- A projectile can be spawned, travels through space, collides with wall/entity, applies effect, and emits receipts.

### 13. Tactical Time and Camera Mode

Goal: make real-time first/third-person play and slowed tactical view one coherent loop.

Todos:

- [ ] Define first-person/small-third-person real-time camera behavior.
- [ ] Define tactical camera transition on slow-time activation.
- [ ] Add tactical overhead/orbit controls.
- [ ] Add command queuing while slowed/paused if turn-like combat needs it.
- [ ] Add tick/step semantics for tactical mode.
- [ ] Add camera receipts and debug HUD fields.
- [ ] Preserve replay determinism across camera mode changes.

Acceptance:

- User can play in first/third person, enter slow tactical view, inspect/issue actions, and return without runtime drift.

### 14. Renderer and Vulkan World Rendering

Goal: render actual room/object data instead of simple proof geometry.

Relevant docs:

- `docs/vulkan/`
- `docs/vulkan/file_plans/`

Todos:

- [ ] Keep renderer consuming projection data only.
- [ ] Render room meshes with depth and stable camera projection.
- [ ] Render editor ghosts, selected objects, debug lines, and traversal previews.
- [ ] Add material/texture pipeline for simple objects.
- [ ] Add mesh upload/resource lifetime policy.
- [ ] Add frame capture where platform allows.
- [ ] Add no-window/null render tests and limited manual visible proof gates.
- [ ] Keep MoltenVK/macOS path documented.

Acceptance:

- Vulkan can show the authored room, player marker/model, props, and debug overlays without owning runtime state.

### 15. Save, Replay, Compatibility, and World Durability

Goal: make all user-created worlds durable across builds.

Todos:

- [ ] Version save sections explicitly.
- [ ] Add compatibility checks for package/scenario/world versions.
- [ ] Add save migration policy.
- [ ] Add save preview metadata independent from full load.
- [ ] Add autosave/manual save distinction.
- [ ] Add crash-safe write path using temp file then rename.
- [ ] Add replay proof for gameplay-affecting commands.
- [ ] Add receipt fields for save compatibility and migration decisions.

Acceptance:

- User worlds do not silently corrupt or load into incompatible runtime states.

### 16. Multiplayer-Ready Runtime

Goal: keep single-player first while preserving future multiplayer support.

Todos:

- [ ] Define authority roles: local, host, client, replay.
- [ ] Define deterministic command submission and ordering.
- [ ] Define replication packets for player inputs, world snapshots, and correction.
- [ ] Keep physics/movement deterministic enough for replay or define reconciliation boundaries.
- [ ] Add local multiplayer session tests before network sockets.
- [ ] Add packet encode/decode tests.
- [ ] Keep save/replay decoupled from renderer.

Acceptance:

- Runtime has authority boundaries and packet codecs even before network transport exists.

### 17. AI, NPCs, and Encounter Runtime

Goal: make NPCs participate in the tactical loop.

Todos:

- [ ] Define NPC state: faction, perception, intent, movement budget, abilities, health.
- [ ] Add target selection and reach/path feasibility checks.
- [ ] Add simple decision policies.
- [ ] Add combat turn/slow-time participation.
- [ ] Add training dummy -> active NPC progression.
- [ ] Add debug receipts for AI decisions and rejected actions.

Acceptance:

- NPC can choose a target, move/act through command/session ownership, and be replayed.

### 18. Product Demo Acceptance

Goal: ship an internal demo with a real loop.

Todos:

- [ ] Boot product app to starter menu.
- [ ] Create/load/delete saves.
- [ ] Enter a real 3D room.
- [ ] Move with keyboard and PS5 controller.
- [ ] Open pause/settings/dev tools.
- [ ] Edit a simple room and save it.
- [ ] Test jump/crouch/dash/clamber/vault/wire-walk playground.
- [ ] Cast at least one projectile spell.
- [ ] Enter slow tactical mode and return.
- [ ] Persist and replay deterministic summary.
- [ ] Produce acceptance receipt and manual visible proof.

Acceptance:

```text
product_demo_ready=true
starter_menu_ready=true
world_save_ready=true
room_visible=true
player_control_ready=true
movement_playground_ready=true
editor_save_load_ready=true
projectile_spell_ready=true
tactical_mode_ready=true
replay_hash=pass
result=pass
```

## Builder Packet Queue

Recommended next packet order:

1. Stabilization commit/audit for current dirty work.
2. Visual Demo Decomposition v1 Phase 1: options + automation control extraction.
3. Product App Boot Contract v1: `./build/iggy3d` starts at starter menu.
4. Control Binding Policy v1: physical inputs routed through shared actions.
5. Settings Persistence v1.
6. Debug/Receipt Standard v1.
7. Dev Tools Taxonomy v1.
8. World Creation Flow v1.
9. Editor UX v2: selection/transforms/grid/object palette.
10. Movement Physics Playground v1.
11. Projectile Ability v1.
12. Real 3D Room Data v1.
13. Vulkan World Rendering v1.
14. Tactical Camera/Slow-Time v1.
15. Save Compatibility/Migration v1.
16. Multiplayer Authority/Replication v1.
17. Product Demo Acceptance v1.

## What Not To Do Next

- Do not add another large feature directly into `apps/iggy3d_visual_demo/main.cpp`.
- Do not replace the save format while editor/world creation is still moving.
- Do not start full physics integration before the character controller/query seam is defined.
- Do not start Blender import before native room/object semantics are stable.
- Do not bind destructive controller commands to single buttons.
- Do not make renderer/Vulkan own gameplay, editor, or save state.
- Do not remove automated control-file testing while deleting Codex-branded names.
