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
- `LevelMutationCacheUpdateStep`: applies tile edits through `LevelTileMutation` and refreshes derived caches through `LevelDerivedCacheUpdater`.
- `LevelRenderFrame2D`: data-only level render-frame builder.

## `scene/interaction`

- `InteractionTarget2D`, `InteractionTarget2DRegistry`, `InteractionTarget2DRegistryBuilder`: authored/runtime interaction target data and validated target lookup table.
- `InteractionTargetQuery2D::find`: read-only target lookup with missing/disabled/not-found diagnostics.
- `InteractionReach2D::evaluate`: player/actor position to target reach query with explicit reach config.
- `InteractionPlan2D::plan`: target query + reach into ready/blocked interaction plan.
- `InteractionEffect2D`, effect factory helpers, `validate`: backend-free interaction effect intent data.
- `InteractionEvent2D`, event factory helpers, `InteractionEventRecorder2D`: transient interaction event data emitted by effect application.
- `InteractionEffectCatalog2D`, `InteractionEffectCatalog2DBuilder`: target id to ordered interaction effects.
- `InteractionEffectPlan2D::plan`: ready interaction plan to requested effect list.
- `InteractionTargetToggle2D::apply`: returns an updated interaction target registry with one target enabled/disabled when possible.
- `InteractionEffectApplier2D::apply`: applies one interaction effect to an interaction target registry, applying toggle effects and recording inspect/event/toggle events.
- `InteractionEffectPlanApplier2D::apply`: applies a ready effect plan to an interaction target registry and reports applied/deferred/no-op/failed counts plus recorded events.

## `scene/inventory`

- `InventoryState2D`, `InventoryState2DBuilder`: player/scene inventory stack data with deterministic id/count validation.
- `InventoryAddItem2D::add`: returns updated inventory with an added or incremented item stack.
- `ItemDefinition2D`, `ItemDefinition2DCatalog`, `ItemDefinition2DCatalogBuilder`: item metadata and stack-limit source of truth.
- `InventoryEvent2D`, event helpers, `InventoryEventRecorder2D`: transient inventory/pickup event data.
- `InventoryStackPolicy2D::planAdd`: validates add requests against item definitions and current stack counts.
- `InventoryPolicyAddItem2D::add`: policy-aware inventory add that preserves stack-policy diagnostics and records inventory events.
- `LevelItemDrop2D`, `LevelItemDrop2DRegistry`, `LevelItemDrop2DRegistryBuilder`: level item-drop data and validated drop lookup table.
- `LevelItemDropConsume2D::consume`: returns updated item-drop registry by disabling or removing a consumed drop.
- `PickupPlan2D::plan`: actor position + drop id to ready/blocked pickup plan using pickup radius and extra reach.
- `PickupTransfer2D::transfer`: applies a ready pickup plan to inventory and item-drop registries.
- `PickupPolicyTransfer2D::transfer`: policy-aware pickup transfer using item definitions, policy add, drop consume, and inventory events.

## `scene/ui`

- `UiToolBeltState` helpers: tool-belt navigation, occupied-cell normalization, pinned rows, and view projection.
- `UiShellModel` helpers: feature registry lookup, workspace slot mounting, panel assignment, panel sizing, and visibility.
- `UiFeatureContext`, `UiShellActions`: UI read context and host-provided action callbacks.
- `UiToolInventory`, `defaultUiToolInventory`, `buildUiToolBeltLayout`: available tool descriptors and tool-belt layout construction.
- `UiToolIntent`, `buildUiToolIntentActionPlan`, `executeUiActionPlan`: UI intent to host action plan without directly mutating runtime state.
- `UiSettingsState`, `applyUiSettingsToWorkspace`: UI settings projected into workspace layout.
- `UiRuntimeFrameInspectorModel`, `UiInteractionEventPanelModel`: read-only UI models from runtime reports and interaction events.

## `scene/player`

- `PlayerAgentState`, `playerTile`: player-owned scene state.
- `PlayerCommandPlanner2D`: one command to one player plan.
- `PlayerCommandFramePlanner2D`: command frame to ordered player plans plus diagnostics.
- `PlayerMovementExecutor2D::execute`: applies one movement plan through `physics2d::CharacterMove2D`.
- `PlayerInputIntent2D`, intent factory helpers, `validate`: device-free player input intent data.
- `PlayerInputIntentGate2D`: filters player input intents through scene/player input context rules.
- `PlayerInputCommandMapper2D`, `PlayerInputCommandFrameMapper2D`: map accepted player input intents to gameplay commands.
- `PlayerInputGatedCommandFrameMapper2D`: gates intents first, then maps accepted intents while preserving gate and mapping diagnostics.

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
- `RuntimeCommandQueue`, `RuntimeCommandQueueState`: explicit gameplay command-frame queue data and push/drain operations.
- `RuntimePlayerInputQueueStep`: maps or gates player input intents into a gameplay command frame and queues it.
- `RuntimeQueuedCommandRunner`: drains queued command frames through the session command tick runner.
- `RuntimePlayerInputCommandRunner`: composes input intent mapping/gating, command queue push, and queued command running.
- `RuntimePlayerInputCommandReporter`: summarizes ungated or gated input command runner diagnostics without executing behavior.
- `RuntimePlayerInputFrameStep`: one-frame wrapper around ungated or gated input command running plus reporting.
- `RuntimeInteractionCommandStep`, `RuntimeInteractionCommandFrameStep`: evaluate Interact commands against scene interaction targets.
- `RuntimeInteractionEffectCommandStep`, `RuntimeInteractionEffectCommandFrameStep`: evaluate ready interactions into requested effect plans without applying effects.
- `RuntimeInteractionEffectApplyStep`: evaluates one Interact command and applies requested effects to a returned interaction target registry.
- `RuntimeInteractionEffectApplyFrameStep`: applies Interact commands in a command frame sequentially against the carried interaction registry.
- `RuntimeInteractionState`: runtime-carried interaction targets and effects packet; scene/interaction still owns target/effect semantics.
- `RuntimePlayerInputInteractionFrameStep`, `RuntimePlayerInputInteractionFrameReporter`: gated input frame plus interaction diagnostics.
- `RuntimePlayerInputInteractionEffectFrameStep`, `RuntimePlayerInputInteractionEffectFrameReporter`: gated input frame plus interaction/effect-plan diagnostics.
- `RuntimePlayerInputInteractionEffectApplyFrameStep`: gated player input frame plus interaction effect application, returning updated interaction state/targets.
- `RuntimeInventoryState`: explicit runtime-carried inventory + item-drop packet; not part of `RuntimeSessionState`.
- `RuntimePickupStep`: session player + runtime inventory state + drop id to pickup transfer result.
- `RuntimePickupEffectStep`: applies `PickupItem` interaction effects through `RuntimePickupStep`.
- `RuntimePickupEffectFrameStep`: scans applied interaction effects and applies simple pickup effects sequentially.
- `RuntimePolicyPickupStep`: policy-aware pickup using `ItemDefinition2DCatalog` and `PickupPolicyTransfer2D`.
- `RuntimePolicyPickupEffectStep`: applies one `PickupItem` effect through `RuntimePolicyPickupStep`.
- `RuntimePolicyPickupEffectFrameStep`: scans applied interaction effects and applies policy pickup effects sequentially.
- `RuntimePlayerInputInteractionPickupFrameStep`, `RuntimePlayerInputInteractionPickupFrameReporter`: gated input + interaction apply + pickup-effect orchestration and reporting.
- `RuntimeGameplayState`: top-level runtime gameplay packet carrying session, command queue, interaction state, and inventory state.
- `RuntimeGameplayFrameStep`, `RuntimeGameplayFrameRunner`, `RuntimeGameplayFrameReporter`: one-frame and bounded gameplay orchestration over input, interaction, pickup, command ticking, and reports.
- `RuntimeCollisionWorldProvider`: resolves explicit/session/empty collision world.
- `RuntimeSessionCommandTick`: command step followed by session tick.
- `RuntimeSessionCommandTickRunner`: bounded command-frame loop over command ticks.
- `RuntimeLevelMutationStep`: runtime adapter for explicit tile edits via `LevelMutationCacheUpdateStep`.
- `RuntimeSessionMutationCommandStep`: mutation/cache update followed by command tick.
- `RuntimeSessionMutationCommandRunner`: bounded loop over mutation-command frames.
- `RuntimeSessionSnapshotBuilder`, `RuntimeSessionSnapshotRestorer`: capture/restore saveable runtime session state while excluding derived caches.
- `RuntimeSessionSnapshotValidator`: validates snapshot map/player/NPC consistency.
- `RuntimeBinaryWriter`, `RuntimeBinaryReader`: little-endian byte stream helpers.
- `RuntimeSaveChunkArchive`, `RuntimeSaveChunkArchiveBuilder`, `RuntimeSaveChunkArchiveValidator`: backend-free chunked save archive data and validation.
- `RuntimeSessionSnapshotChunkEncoder`, `RuntimeSessionSnapshotChunkDecoder`: convert snapshots to/from chunk archives.
- `RuntimeSaveChunkArchiveEncoder`, `RuntimeSaveChunkArchiveDecoder`: convert chunk archives to/from byte vectors.
- `RuntimeSaveFileEnvelopeEncoder`, `RuntimeSaveFileEnvelopeDecoder`: wrap/unwrap save payload bytes with envelope metadata and checksum validation.
- `RuntimeSaveFileReader`, `RuntimeSaveFileWriter`: local file byte read/write helpers.
- `RuntimeSaveSlotPathPolicy`: resolves validated local save slot paths.
- `RuntimeSaveSlotStore`: stores and loads save envelopes through explicit slots.
- `RuntimeSaveSlotListing`: lists local save slots and optional metadata.
- `RuntimeSaveSlotDeletion`: deletes one resolved local save slot file.
- `RuntimeSessionSaver`, `RuntimeSessionLoader`: compose session snapshots, archives, envelopes, file IO, and restore config.
