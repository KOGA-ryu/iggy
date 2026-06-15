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
- `LevelCollisionSource2D`, `LevelCollisionSource2DBuilder`: derived collision build input produced from compile outputs; not authored layout truth and not runtime/session-owned state.
- `LevelCollisionSourceWorldBuilder2D`, `LevelCollisionWorldMerge2D`, `LevelCombinedDerivedCacheBuilder2D`: opt-in collision world/cache construction from compiled collision source boxes plus tile-map collision.

## `scene/draft`

- `DraftDocument2D`, `DraftDocument2DBuilder`: authored semantic draft symbols; current project/editor source of truth for draft layout intent.
- `DraftCompilePlanner2D`: classifies enabled draft symbols into planned output categories.
- `DraftWallCompiler2D`, `DraftDoorCompiler2D`: validate and project wall/door symbols into compiled draft facts.
- `DraftWallDoorAttach2D`, `DraftWallDoorMorphPlanner2D`, `DraftWallCutCompiler2D`: resolve wall/door relationships and produce final wall segment facts.
- `DraftBuildingCompiler2D`: auditable building compile report for walls and doors.
- `DraftLevelGeometryPlanner2D`: converts building compile wall segments into level/collision geometry plan data.

## `scene/debug`

- `SceneAsciiCanvas2D`: value-returning ASCII projection canvas for debug/import-proof rows and strings; not authoritative gameplay state.

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

## `scene/ai`

- `AiMap2D`, `AiMapQuery2D`: map-owned tactical AI substrate and read-only local situation query.
- `NpcTraitSet`: full Strength, Dexterity, Constitution, Intelligence, Wisdom, and Charisma score packet with 0..20 validation.
- `NpcStrengthPool`, `NpcStrengthDraw`: Strength behavior pool entries and read-only unlock draw by strength score and behavior state.
- `NpcDexterityPool`, `NpcDexterityDraw`: Dexterity behavior pool entries and read-only unlock draw by dexterity score and behavior state.
- `NpcConstitutionPool`, `NpcConstitutionDraw`: Constitution behavior pool entries and read-only unlock draw by constitution score and behavior state.
- `NpcIntelligencePool`, `NpcIntelligenceDraw`: Intelligence behavior pool entries and read-only unlock draw by intelligence score and behavior state.
- `NpcWisdomPool`, `NpcWisdomDraw`: Wisdom behavior pool entries and read-only unlock draw by wisdom score and behavior state.
- `NpcCharismaPool`, `NpcCharismaDraw`: Charisma behavior pool entries and read-only unlock draw by charisma score and behavior state.
- `NpcHand`, `NpcHandAssembler`: read-only cross-trait candidate collection assembled from trait draw results.
- `NpcRead`, `NpcReader`: conservative read-only ranking report over Hand entries using weight as the initial score.
- `NpcMapRead`, `NpcMapReader`: opt-in map-aware Read report that ranks Hand entries with local AI map tag matches and UI-tunable scoring config.
- `NpcMapReadProjection`: adapter from map-aware Read results into the existing `NpcRead` shape for Play/Fold consumers.
- `NpcMapPlayReport`, `NpcMapPlayReporter`: read-only map-aware card-chain report preserving raw Read, map-aware Read, projected Read, Play, Tell, and Fold facts.
- `NpcMapPlayControlFramePlan`, `NpcMapPlayControlFramePlanner`: scene-level read-only planner that joins actor/control frame state, per-NPC trait sets, six trait pools, and AI map queries into prepared `NpcMapPlayControlFrameStep2D` requests; no control apply/runtime mutation.
- `NpcMapPlayControlFrameStep2D`, `NpcMapPlayControlFrameStepper2D`: scene-level thought/control orchestration from per-NPC map-aware play inputs to control proposals, frame apply, and frame report.
- `NpcPlay`, `NpcPlaySelector`: deterministic read-only Play commit selecting the first ranked Read entry.
- `NpcPlayControlProposal`, `NpcPlayControlProjector`: pure projection from kept Fold/selected Play facts into proposed objective, behavior state, and move mode data.
- `NpcTell`, `NpcTeller`: explanation/trace projection over Hand, Read, and Play facts.
- `NpcFold`, `NpcFolder`: explicit keep/fold policy over Tell and Play score/issue facts.
- `NpcAiBehaviorPool`, `NpcAiBehaviorPoolBuilder`: validated reusable NPC behavior presets for tuning/profile construction.
- `NpcObjective`: durable NPC goal vocabulary and validation.
- AI naming and semantic ownership contract: `engine/research/ai_naming_ownership.md`.
- `NpcAiProfile2D`, `NpcAiCurrentState2D`: NPC temperament and current AI state data.
- `NpcAiContextScore2D`, `NpcAiBehaviorIntent2D`, `NpcAiIntentTarget2D`, `NpcAiDecision2D`: read-only decision pipeline from tactical context and NPC bias to chosen instinct/anchor.
- `NpcAiRouteRequest2D`, `NpcAiNavigationRequest2D`, `NpcAiPathReport2D`, `NpcAiMovementProposal2D`: route, navigation, path, and movement proposal reports.
- `NpcAiMovementCommandMapper2D`, `NpcAiCommandFrameMapper2D`: convert movement proposals into runtime gameplay command data without queueing/execution.
- Ace-facing naming guide: `engine/research/npc_ai_tuning_names.md`.

## `scene/npc`

- `NpcActorState2D`, `NpcActorState2DRegistry`, `NpcActorState2DRegistryBuilder`: scene-owned NPC actor identity/profile/current-goal state and validated lookup table.
- `NpcMoveMode`: movement intensity vocabulary and speed multiplier helpers.
- `NpcBehaviorState`: actor-carried active behavior state vocabulary and validation.
- `NpcActorControlState2D`, `NpcActorControlState2DRegistry`, `NpcActorControlState2DRegistryBuilder`: scene-owned NPC objective/behavior/move-mode control packet and validated lookup table.
- `NpcActorEscapeRouteTarget2D`, `NpcActorEscapeRouteTargetProjector2D`: read-only adapter that feeds a successful local escape target into route-target projection while preserving escape and route diagnostics; no pathfinding or mutation.
- `NpcActorEscapeTarget2D`, `NpcActorEscapeTargetProjector2D`: read-only bounded local escape destination selection for `MoveAwayFrom` movement intents using level tile walkability; no pathfinding or mutation.
- `NpcActorMovementExecutor2D`: single-actor by-value movement executor that applies an allowed occupancy-filtered path step to a copied actor and emits post-move facts; no registry/runtime mutation.
- `NpcActorMovementFrameApply2D`, `NpcActorMovementFrameApplier2D`: by-value ordered registry movement apply layer that delegates found actor/filter pairs to the single-actor executor and preserves per-entry diagnostics; no runtime/session mutation.
- `NpcActorMovementReservation2D`, `NpcActorMovementReservationProjector2D`: read-only same-frame destination reservation pass over prepared movement requests with default no-sharing and configurable per-tile capacity; no actor mutation or occupancy rebuild.
- `NpcActorMovementReservedFrameApply2D`, `NpcActorMovementReservedFrameApplier2D`: scene-level adapter that runs movement reservation once, applies accepted requests through `NpcActorMovementFrameApplier2D`, and preserves reservation/apply/report diagnostics.
- `NpcActorMovementIntent2D`, `NpcActorMovementIntentProjector2D`: read-only projection from joined actor/control frame state into one actor movement intent; no pathfinding or mutation.
- `NpcActorMovementFramePlan2D`, `NpcActorMovementFramePlanner2D`: scene-level read-only planner that composes actor/control frame state, route/navigation/path/step, and occupancy filtering into prepared movement frame apply requests; includes opt-in occupancy policy and reservation diagnostics while preserving default hard-block planning; no apply/runtime mutation.
- NPC actor movement runtime boundary: `engine/research/npc_actor_movement_runtime_boundary.md`.
- `NpcActorMovementFrameReport2D`, `NpcActorMovementFrameReporter2D`: projection-only report over movement frame apply results with dirty-tile aggregation, refresh flags, and deterministic summary events; no cache/runtime consumers.
- `NpcActorMovementRefreshFrame2D`, `NpcActorMovementRefreshFrameProjector2D`: combined movement-derived refresh report that projects refresh work once and composes occupancy, interaction, AI map, and visual refresh packet consumers; no downstream cache/render/FOV/AI/runtime execution.
- `NpcActorMovementRefreshWork2D`, `NpcActorMovementRefreshWorkProjector2D`: projection-only extraction of explicit refresh work packets from movement frame report dirty tiles and refresh flags; no cache/runtime consumers.
- `NpcActorAiMapRefresh2D`, `NpcActorAiMapRefresher2D`: movement-derived AI map query refresh projection that consumes AI map refresh work, reports affected NPCs on dirty tiles, and optionally preserves pure `AiMapQuery2D` diagnostics; no AI decision execution or map mutation.
- `NpcActorInteractionRefresh2D`, `NpcActorInteractionRefresher2D`: movement-derived interaction refresh projection that consumes interaction refresh work and reports dirty tiles plus affected NPC/interaction target facts; no interaction execution or runtime mutation.
- `NpcActorVisualRefresh2D`, `NpcActorVisualRefresher2D`: movement-derived render/visibility refresh packet projection that splits visual dirty-tile work into independent render and visibility packets; no renderer, FOV, cache, UI, or runtime mutation.
- `NpcActorMovementFrameIntent2D`, `NpcActorMovementFrameIntentProjector2D`: read-only batch projection from actor/control frame-state projection into per-actor movement intents; no pathfinding or mutation.
- `NpcActorNavigationRequest2D`, `NpcActorNavigationRequestBuilder2D`: read-only adapter from ready NPC route targets to static tile-map navigation request validation; no pathfinding or mutation.
- `NpcActorOccupancy2D`, `NpcActorOccupancyProjector2D`: read-only derived occupancy projection from NPC actor registry positions into occupied tile groups with duplicate-tile diagnostics; actor position remains actor truth.
- `NpcActorOccupancyPolicy2D`: read-only occupancy policy over derived occupancy queries with default hard-block behavior and configurable per-tile capacity for future sharing rules; no faction/profile/spirit semantics.
- `NpcActorOccupancyQuery2D`: read-only tile occupancy/blocking queries over derived NPC occupancy, including exact self-occupancy checks; no pathfinding or mutation.
- `NpcActorOccupancyRefresh2D`, `NpcActorOccupancyRefresher2D`: by-value refresh consumer that rebuilds derived NPC occupancy from the supplied actor registry only when movement refresh work requests `OccupancyRebuild`.
- `NpcActorPathReport2D`, `NpcActorPathReporter2D`: read-only pathfinding report over accepted NPC actor navigation requests using `NavigationGridPathfinder`; no step selection or mutation.
- `NpcActorPathStep2D`, `NpcActorPathStepper2D`: read-only bounded movement-step proposal over found NPC actor paths using `NavigationPathFollower` and `NpcMoveMode` speed; no actor mutation or path-state ownership.
- `NpcActorPathStepOccupancyFilter2D`, `NpcActorPathStepOccupancyFilterProjector2D`: read-only default hard-block occupancy filter over proposed NPC path steps; no reservations, sharing policy, or mutation.
- `NpcActorPathStepOccupancyPolicyFilter2D`, `NpcActorPathStepOccupancyPolicyFilterProjector2D`: opt-in policy-aware occupancy filter over proposed NPC path steps that emits an executor-compatible filter plus policy diagnostics.
- `NpcActorPostMoveReport2D`, `NpcActorPostMoveReporter2D`: report-only post-move facts contract describing movement/block/rejection tiles and refresh needs without calling refresh/cache/runtime systems.
- `NpcActorRouteTarget2D`, `NpcActorRouteTargetProjector2D`: read-only projection from movement intent into a concrete route target when one exists; fleeing requires an explicit escape destination and no pathfinding runs here.
- `NpcPlayControlApply2D`, `NpcPlayControlApplier2D`: mutation boundary that applies a valid Play control proposal into NPC actor control registry data by value.
- `NpcPlayControlFrameApply2D`, `NpcPlayControlFrameApplier2D`: ordered batch mutation over NPC Play control proposals with sequential duplicate-NPC handling.
- `NpcPlayControlFrameReport2D`, `NpcPlayControlFrameReporter2D`: projection-only report over NPC Play control frame apply results and summary events.
- `NpcActorFrameState2D`, `NpcActorFrameStateProjector2D`: per-frame actor + optional control join/projection with missing/orphan diagnostics.

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
- `RuntimePlayerInputInteractionPolicyPickupFrameStep`: gated input + interaction apply + catalog-aware pickup-effect orchestration.
- `RuntimeGameplayState`: top-level runtime gameplay packet carrying session, command queue, interaction state, inventory state, and NPC actor/control registries.
- `RuntimeGameplaySnapshotBuilder`, `RuntimeGameplaySnapshotRestorer`: data-only gameplay snapshot capture/restore over `RuntimeGameplayState`, wrapping `RuntimeSessionSnapshot` plus gameplay-owned children without disk serialization.
- `RuntimeGameplaySnapshotChunkEncoder`, `RuntimeGameplaySnapshotChunkDecoder`: archive-level gameplay snapshot chunk codec that composes existing session snapshot chunks plus gameplay-owned command, interaction, inventory, NPC actor, and NPC control chunks; no file/save-slot integration.
- `RuntimeGameplaySnapshotSaver`, `RuntimeGameplaySnapshotLoader`: gameplay-specific snapshot file/envelope IO over the gameplay chunk codec, without changing session save/load or save-slot ownership.
- `RuntimeGameplaySaveSlotStore`: gameplay-specific named slot API over `RuntimeGameplaySnapshotSaveLoad`, with engine-only inspect/list/delete/no-overwrite rename/copy/summary helpers; separate from the existing session `RuntimeSaveSlotStore` and without UI/runtime autosave integration.
- `RuntimeGameplaySnapshotValidator`: pure validation/reporting over gameplay snapshots, preserving nested session validation plus new NPC actor/control bounds and join diagnostics.
- `RuntimeGameplayFrameStep`, `RuntimeGameplayFrameRunner`, `RuntimeGameplayFrameReporter`: one-frame and bounded gameplay orchestration over input, interaction, pickup, command ticking, optional prepared NPC actor movement requests, per-tick NPC movement reports, and runner-level movement aggregation.
- `RuntimePolicyGameplayFrameStep`, `RuntimePolicyGameplayFrameRunner`, `RuntimePolicyGameplayFrameReporter`: catalog-aware gameplay frame orchestration and reporting over policy pickup, optional prepared NPC actor movement requests, per-tick NPC movement reports, and runner-level movement aggregation.
- `RuntimeNpcActorMovementFrameStep`: runtime adapter over prepared scene/npc movement apply requests; delegates actor registry mutation to scene/npc and returns updated `RuntimeGameplayState` without generating pathfinding or movement filters.
- `RuntimeNpcActorMovementRequestPlanStep`: thin runtime-adjacent helper that delegates current gameplay NPC actor/control state plus a level map and planner config to `NpcActorMovementFramePlanner2D`, returning prepared movement requests and mirrored reservation counts without applying movement.
- `RuntimeNpcActorMovementPlannedFrameStep`: explicit runtime-adjacent composition helper that calls request planning and then prepared movement frame application when directly invoked; it does not add auto-planning to raw/policy gameplay frame steps or runners.
- `RuntimeNpcActorMovementPlannedFrameRunner`: explicit runtime-adjacent sequence runner that repeats `RuntimeNpcActorMovementPlannedFrameStep` over caller-supplied map/config frames, carrying `RuntimeGameplayState` forward without integrating auto-planning into gameplay runners.
- `RuntimeNpcAiControlPlannedFrameStep`: explicit runtime-adjacent AI control composition helper that calls `NpcMapPlayControlFramePlanner` and `NpcMapPlayControlFrameStepper2D` when directly invoked, returning a copied gameplay state with only `npcControls` replaced.
- `RuntimeNpcAiProfileControlPlannedFrameStep`: explicit runtime-adjacent helper that resolves actor `aiProfileId` values through `NpcAiProfileTraitCatalog` before delegating to `RuntimeNpcAiControlPlannedFrameStep`; callers no longer need to hand-build map-play control subjects for profile-backed actors.
- `RuntimeNpcAiMovementPlannedFrameStep`: explicit runtime-adjacent one-frame helper that runs AI control planning/apply, then movement planning/apply using the post-control state; raw/policy gameplay frames and runners still do not auto-run AI or movement planning.
- `RuntimeNpcAiMovementPlannedFrameRunner`: explicit runtime-adjacent sequence runner that repeats `RuntimeNpcAiMovementPlannedFrameStep` over caller-supplied AI/map/movement frames, carrying gameplay state forward without integrating auto-planning into gameplay loops.
- `RuntimeNpcAiMovementRefreshFrameStep`: explicit runtime-adjacent helper that runs one AI+movement planned frame and then projects movement refresh packets; it does not execute cache, render, visibility, interaction, or gameplay frame consumers.
- `RuntimeNpcAiMovementRefreshFrameRunner`: explicit runtime-adjacent sequence runner that repeats AI+movement+refresh frames, deriving previous occupancy from carried actor state per frame and aggregating refresh packet facts without cache execution.
- `RuntimeGameplayOrchestratedFrameStep`: explicit top-level gameplay helper that runs the existing player/input/interaction/pickup frame path with prepared NPC movement disabled, then runs the caller-selected NPC AI+movement+refresh helper over the post-player state; it does not alter raw/policy frame auto-planning boundaries.
- `RuntimeGameplayOrchestratedFrameRunner`: explicit top-level gameplay sequence helper that repeats orchestrated player-first/NPC-second frames while carrying gameplay state and aggregating inventory, NPC movement, and refresh packet facts; old raw/policy runners remain prepared-request-only paths.
- `RuntimeGameplayOrchestratedFrameReporter`, `RuntimeGameplayOrchestratedFrameRunnerReporter`: compact projection packets for explicit orchestrated frame/runner results, intended for tooling inspection without running gameplay loops or traversing all nested diagnostics.
- `RuntimeGameplayScenarioRunner`: engine replay/test/tooling harness over the explicit orchestrated frame runner and compact runner reporter; consumes an initial gameplay state plus scripted orchestrated frames and returns final state plus report packets without parser, UI, or save/load integration.
- `RuntimeGameplayScenarioDefinition`, `RuntimeGameplayScenarioValidator`: typed scenario authoring contract and deterministic validation layer for future ASCII/Edi/IDE tooling; converts to `RuntimeGameplayScenario` without parsing external formats or running gameplay.
- NPC actor save/load ownership boundary: `engine/research/npc_actor_save_load_ownership.md`.
- `NpcAiProfileTraitCatalog`, `NpcAiProfileTraitResolver`: scene/ai profile-trait bridge that maps actor `aiProfileId` values to `NpcMapPlayControlFramePlanSubject` inputs for the map-play control planner; behavior-pool borrowing, inheritance, rank/profile-role naming, and runtime orchestration remain future work.
- `RuntimeNpcAiCommandQueueStep`: pushes already-mapped NPC AI command frames into the runtime command queue.
- `RuntimeNpcAiMovementQueueStep`: maps NPC movement proposals to gameplay commands and queues them.
- `RuntimeNpcAiDecisionQueueStep`: runs scene/ai decision, route, path, movement proposal, and command mapping for NPC inputs, then queues the result.
- `RuntimePlayerNpcAiQueueStep`: composes player input queueing with NPC AI queueing in an explicit order.
- `RuntimePlayerNpcAiCommandFrameStep`: runs combined player/NPC AI queue intake through the queued command runner.
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

## `apps/qt_shell`

- `IggyQtShellWindow`: Qt host shell window that projects scene/ui models into desktop chrome, activity rail, workspace, panels, tool belt, palette, settings, and status bar widgets.
- `IggyQtShellUi` helpers: Qt-only style/theme projection and widget factories such as `makeChromeButton`, `makeRailButton`, and `makeSectionLabel`.
