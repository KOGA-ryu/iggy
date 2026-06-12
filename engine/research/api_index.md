# API Index

Purpose: fast source-of-truth lookup for major public engine types and functions by subsystem. This is not exhaustive; it lists anchor APIs that planner/reviewer handoffs should reuse before inventing new seams.

## `core/math`

- `Vec2`: 2D vector math.
- `Aabb2`: raw min/max bounds; inclusive `contains` / `overlaps`; no implicit normalization.
- `Rect2`: source rectangles and 2D rectangular metadata.
- `Ray2`: ray query input data.

## `core/resource`

- `ResourceId`: stable logical id value.
- `AssetRecord`, `AssetCatalog`, `AssetCatalogBuilder`: backend-free logical asset metadata.

## `servers/render`

- `RenderCommand2D`: backend-free 2D render command with quad bounds, material id, optional texture id/source rect, layer, order.
- `RenderCommandList2D`, `RenderCommandListBuilder2D`: command list and append helpers.
- `RenderCommandList2DComposer`: appends/merges command lists and normalizes order.
- `TextureResourceCatalog`, `ShaderResourceCatalog`, `MaterialResourceCatalog`: backend-free render resource metadata catalogs.
- `RenderResourceRegistry`: resolves material ids to plain material/shader/texture metadata.

## `servers/physics2d`

- `CollisionShape2D`, `makeAabbShape`, `boundsOf`, `isValid`: generic collision shape contract.
- `CollisionObject2D`, `CollisionWorld2D`, `CollisionWorld2DBuilder`: vector-backed collision query container.
- `CollisionOverlap2D::queryAabb`: read-only AABB overlap query.
- `CollisionMotionQuery2D::sweepAabb`: swept AABB preflight query.
- `CharacterMove2D::move`: clamps requested AABB movement through `CollisionMotionQuery2D`.

## `servers/navigation`

- `NavigationGridValidator`: validates navigation grid inputs.
- `NavigationGridPathfinder`: computes grid paths.
- `NavigationPathFollower`: follows planned paths.
- `NavigationRequest`, `NavigationPath`: navigation request/result data.

## `modules/animation`

- `SpriteFrameSet2D`, `SpriteFrameSet2DBuilder`: named sprite clips and frame metadata.
- `SpriteAnimationState2D`, `SpriteAnimationState2DStepper`: playback state advancement.
- `SpriteAnimationSampler2D::sample`: maps state + clip to visible frame.

## `modules/npc_ai`

- `NpcAgentState`, `NpcAgentEntry`: NPC runtime state records.
- `NpcAgentController`, `NpcAgentBatchUpdater`: NPC update orchestration.
- `NpcAgentRenderCommands`: NPC state to render commands.
- `NpcTickReporter`: NPC tick report generation.

## `scene/level`

- `LevelTileMap`: authoritative tile map data.
- `LevelRuntimeState`: authoritative gameplay level state.
- `LevelTileMutation::apply`: applies explicit tile edits and reports changed tiles.
- `LevelGridQuery` helpers: tile indexing, tile bounds, map containment, tile lookup.
- `LevelVisibleTiles`, `LevelTileDrawList`, `LevelTileRenderCommands`: direct tile visibility/render-command path.
- `LevelTileRenderChunkCache`, `LevelTileRenderChunkVisibility`, `LevelTileRenderChunkCommands`: cached tile render path.
- `LevelRenderCacheState`, `LevelRenderCacheBuilder`, `LevelRenderCacheUpdater`: derived render cache state.
- `LevelCollisionWorldBuilder`: converts blocked tiles to `physics2d::CollisionWorld2D`.
- `LevelCollisionCacheState`, `LevelCollisionCacheBuilder`, `LevelCollisionCacheUpdater`: derived collision cache state.
- `LevelDerivedCacheState`, `LevelDerivedCacheBuilder`, `LevelDerivedCacheUpdater`: render + collision cache packet.
- `LevelRenderFrame2D`: data-only level render-frame builder.

## `scene/player`

- `PlayerAgentState`, `playerTile`: player-owned scene state.
- `PlayerCommandPlanner2D`: one command to one player plan.
- `PlayerCommandFramePlanner2D`: command frame to ordered player plans plus diagnostics.
- `PlayerMovementExecutor2D::execute`: applies one movement plan through `physics2d::CharacterMove2D`.

## `scene/sprite`

- `SpriteRenderCommands2D`: sampled sprite frame + placement config to render command.

## `runtime`

- `GameplayCommand2D`, command factory helpers, `validate`: gameplay command packet.
- `GameplayCommandFrame2DValidator`: frame-level command validation/filtering.
- `RuntimeSessionState`: runtime-carried session packet.
- `RuntimeSessionBuilder`: initializes session state and optional derived caches/player.
- `RuntimeTick`, `RuntimeTickRunner`: older level/NPC tick path over `LevelRuntimeState`.
- `RuntimeSessionTick`, `RuntimeSessionTickRunner`: session-level tick path.
- `RuntimePlayerCommandPlanningStep`: delegates session player command planning.
- `RuntimePlayerCommandExecutionStep`: executes first movement plan against caller-supplied collision world.
- `RuntimePlayerCommandStep`: composes planning + execution.
- `RuntimeCollisionWorldProvider`: resolves explicit/session/empty collision world.
- `RuntimeSessionCommandTick`: command step followed by session tick.
- `RuntimeSessionCommandTickRunner`: bounded command-frame loop over command ticks.

