# Engine Ownership Boundaries

Purpose: local planning map for small build slices in `engine/`.

Reference shape inspected:
- `godot-master/core/io/resource.h`
- `godot-master/core/templates/rid.h`
- `godot-master/scene/resources/texture.h`
- `godot-master/scene/resources/material.h`
- `godot-master/scene/resources/shader.h`
- `godot-master/scene/2d/sprite_2d.cpp`
- `godot-master/scene/2d/animated_sprite_2d.cpp`
- `godot-master/scene/resources/sprite_frames.cpp`
- `godot-master/scene/2d/physics/character_body_2d.cpp`
- `godot-master/servers/physics_2d/physics_server_2d.h`
- `godot-master/scene/2d/tile_map_layer.cpp`
- `DevilutionX-master/Source/diablo.cpp`
- `DevilutionX-master/Source/controls/plrctrls.cpp`
- `DevilutionX-master/Source/levels/gendung.cpp`
- `DevilutionX-master/Source/lighting.cpp`
- `DevilutionX-master/Source/engine/render/scrollrt.cpp`

Current local anchors:
- `engine/src/core/resource/ResourceId.hpp`
- `engine/src/core/resource/AssetCatalog.hpp`
- `engine/src/runtime/RuntimeSessionState.hpp`
- `engine/src/runtime/RuntimePlayerCommandPlanningStep.hpp`
- `engine/src/scene/player/PlayerAgentState.hpp`
- `engine/src/scene/player/PlayerCommandFramePlanner2D.hpp`
- `engine/src/servers/render/RenderCommand2D.hpp`
- `engine/src/servers/render/RenderCommandList2DComposer.hpp`
- `engine/src/servers/physics2d/CollisionShape2D.hpp`
- `engine/src/servers/physics2d/CollisionWorld2D.hpp`
- `engine/src/servers/physics2d/ShapeQuery2D.hpp`
- `engine/src/scene/level/LevelRenderFrame2D.hpp`
- `engine/src/runtime/RuntimeTickRunner.hpp`

Useful grep:

```sh
rg --files engine/src engine/tests | sort
rg -n "(Texture2D|Material|Shader|RID|get_rid|ResourceCache)" godot-master/scene/resources godot-master/core
rg -n "(Sprite2D|AnimatedSprite2D|SpriteFrames|AnimationPlayer)" godot-master/scene
rg -n "(PhysicsServer2D|CollisionObject2D|RayCast2D|ShapeCast2D|CharacterBody2D)" godot-master/scene godot-master/servers
rg -n "(TileMapLayer|RenderingQuadrant|CellData|dirty|draw_tile)" godot-master/scene/2d/tile_map*
rg -n "(RunGameLoop|ProcessPlayers|ProcessMonsters|ProcessItems|ProcessLightList|ProcessVisionList)" DevilutionX-master/Source
```

## Ownership Rules

- Scene owns authored/runtime state.
  - Level layout, tile values, actors, camera state, animation state, and high-level renderable facts belong in `scene/*`.
  - Scene data may reference resources by `ResourceId`.
  - Scene data should not own backend handles, file loaders, GPU objects, or global service state.

- Resources own reusable asset identity and asset-side data.
  - `ResourceId` and `AssetCatalog` are the stable identity layer.
  - Texture/material/shader resources should describe what something is, not how a backend stores it.
  - Resource records are inputs to render command building and later backend resolution.

- Servers own shared derived systems and query surfaces.
  - Rendering server lane owns command lists, composition, sorting helpers, and future backend-facing registries.
  - Physics server lane owns shape queries, collision worlds, ray/shape casts, and movement queries.
  - Navigation server lane owns pathfinding/path validation, not actor intent.
  - Servers should expose explicit inputs/outputs, not mutate scene state implicitly.

- Runtime owns order.
  - Tick runner/session pipeline decides when input, AI, movement, physics queries, visibility, and render-frame building run.
  - Runtime should coordinate systems, not absorb their data models.

- Builders transform only.
  - Render builders turn scene state plus resource ids into render commands.
  - Animation samplers turn clip data plus playback state into visible sprite frame state.
  - Collision builders turn level tiles into query-world data.
  - Builders should be easy to test as pure data transforms.

- Backends stay behind ids.
  - No scene or gameplay type should depend on a texture object, shader object, window, graphics API, or file format.
  - Backend handles can appear later in a registry layer that resolves `ResourceId` during presentation.

## Bounds Semantics

- `Aabb2` is a raw min/max math type. `contains` and `overlaps` use inclusive edge checks. It does not normalize inverted bounds.
- `CollisionShape2D` uses `Aabb2` exactly as supplied. `boundsOf` returns stored bounds without normalization. `isValid` rejects `Unknown` and inverted AABB bounds, but accepts ordered degenerate bounds.
- Tile visibility is a view/query policy, not core AABB policy. `LevelVisibleTiles` and `LevelTileRenderChunkVisibility` own their half-open non-degenerate and point-bound edge rules through tests.
- Do not silently convert between inclusive AABB behavior and half-open tile visibility behavior. Name and test the boundary that chooses an edge rule.

## Runtime And Physics Notes

- Runtime carries optional `PlayerAgentState` in `RuntimeSessionState` for session continuity.
- `scene/player` owns player command interpretation through `PlayerCommandPlanner2D` and `PlayerCommandFramePlanner2D`.
- Runtime planning steps may delegate to scene/player planners, but must not execute or reinterpret player commands.
- `CollisionWorld2D` is currently a vector-backed valid-shape container only.
- Overlap/raycast-over-world queries and character movement are deferred follow-up slices.

## Data Flow Target

```text
AssetCatalog
  -> Texture/Shader/Material resources
  -> Scene state references ResourceId
  -> Builders produce RenderCommand2D / query worlds
  -> Runtime orders updates and frame assembly
  -> Backend registry resolves ResourceId only at presentation
```

Sprite/animation flow:

```text
SpriteFrameSet2D
  + SpriteAnimationState2D
  -> SpriteAnimationSampler2D
  -> sampled texture id + source rect + transform
  -> RenderCommand2D
```

Physics flow:

```text
LevelTileMap / actor colliders
  -> CollisionWorld2D
  -> Raycast2D / ShapeOverlap2D / ShapeCast2D
  -> CharacterMove2D result
  -> runtime applies accepted movement to scene state
```

ARPG runtime flow:

```text
Input action
  -> gameplay command
  -> tick pipeline
  -> player update
  -> NPC/monster update
  -> item/world interaction
  -> visibility/light updates
  -> render frame assembly
```

## Naming Cues

- Use `State` for owned mutable runtime data.
- Use `Resource` for reusable asset-side descriptions.
- Use `Command` for render or gameplay requests crossing boundaries.
- Use `Query` for read-only questions over existing data.
- Use `Result` for return data from transforms or queries.
- Use `Builder` for one-way construction from source data.
- Use `Sampler` for time/state to sampled visible data.
- Use `Registry` for id-to-object lookup, especially at backend seams.
- Use `Pipeline` or `Runner` for ordered orchestration.

## Build Slice Placement

- `core/resource`: ids, catalogs, resource records that have no rendering behavior.
- `scene/level`: level-owned data, tile maps, visible tile and render-frame assembly.
- `scene/sprite`: sprite-owned visible state and sprite command builders.
- `modules/animation`: clip data, playback state, samplers.
- `servers/render`: render commands, composition, render resource registry, backend-neutral render resources.
- `servers/physics2d`: shapes, collision worlds, ray/shape queries, character movement helpers.
- `runtime`: input command routing, tick stage ordering, session snapshots.

## Defer Boundaries

Do not include these in near-term slices:
- GPU upload or graphics API objects in scene/resource structs.
- Shader compilation or material inheritance.
- Full scene tree, reflection, editor import, hot reload.
- Rigid bodies, joints, forces, callbacks, or physics simulation.
- Diablo-specific item formulas, palette lighting, quest/set-piece generation, network command encoding.
- Godot-style tile terrain editing, per-tile scene instancing, occlusion, or runtime tile override hooks.

## Dispatcher Heuristic

When planning a slice, first answer:

- What system owns the source data?
- Is this slice only transforming data?
- Does the output cross a boundary as a command, query result, or resource id?
- Can it be tested without a backend?
- What existing server/scene/runtime lane should receive it?

If a planned type both owns scene state and resolves backend objects, split it.
