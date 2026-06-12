# Godot Engine Shape Inventory

Source inspected:
- godot-master top-level directories
- godot-master/core
- godot-master/scene
- godot-master/servers
- godot-master/modules
- godot-master/editor
- engine/src
- player-movement-system/src

Useful shape:
- core: math, io, object, variant, input, config
- scene: main, 2d, resources, animation, gui
- servers: physics_2d, navigation_2d, rendering, audio
- modules: optional implementations behind server contracts
- editor: tooling lives outside runtime-facing scene/server code
- register_core_types / register_scene_types / register_server_types
- initialize_*_module / uninitialize_*_module

Current Iggy shape:
- core/math
- core/resource
- scene/level
- modules/blueprint
- servers/physics2d
- servers/navigation
- servers/audio
- servers/render

Take:
- Keep small runtime contracts in scene/level.
- Keep authoring and validation in modules/blueprint.
- Keep service-style systems under servers only when shared runtime state appears.
- Keep platform, editor, and tooling outside gameplay data contracts.
- Use explicit CMake wiring until the engine grows enough to need registration.

Skip:
- Variant/ClassDB-style reflection.
- Full scene tree and editor plugin architecture.
- 3D server lanes.
- Runtime module initialization hooks for now.

Decision:
- Use Godot's folder lanes as a map, but keep Iggy direct: plain C++ structs, small validators, focused servers, no registration layer yet.
