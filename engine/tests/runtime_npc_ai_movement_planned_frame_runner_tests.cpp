#include <cstdlib>
#include <string_view>
#include <type_traits>
#include <vector>

#include "runtime/RuntimeNpcAiMovementPlannedFrameRunner.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/npc/NpcActorOccupancy2D.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::NearVec;

template <typename T, typename = void>
struct HasNpcActorsField : std::false_type {
};

template <typename T>
struct HasNpcActorsField<T, std::void_t<decltype(&T::npcActors)>> : std::true_type {
};

template <typename T, typename = void>
struct HasNpcControlsField : std::false_type {
};

template <typename T>
struct HasNpcControlsField<T, std::void_t<decltype(&T::npcControls)>> : std::true_type {
};

static_assert(!HasNpcActorsField<iggy::runtime::RuntimeSessionState>::value);
static_assert(!HasNpcControlsField<iggy::runtime::RuntimeSessionState>::value);

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::LevelTileMap LevelMap(std::vector<std::string_view> rows)
{
	iggy::LevelTileMap map = MapFromRows(rows);
	map.id = Id("level:runtime-ai-move-runner");
	return map;
}

iggy::AiMapNode2D AiNode(
	const char *id,
	iggy::Vec2 position,
	std::vector<iggy::ResourceId> tags)
{
	return {
		Id(id),
		position,
		3.0F,
		1.0F,
		2.0F,
		3.0F,
		4.0F,
		tags,
		{},
		true,
	};
}

iggy::AiMap2D AiMap(std::vector<iggy::AiMapNode2D> nodes = {})
{
	const iggy::AiMap2DBuildResult result = iggy::AiMap2DBuilder {}.build(nodes);
	Expect(result.built, "runtime ai movement runner ai map should build");
	return result.map;
}

iggy::NpcActorState2D Actor(const char *npcId, iggy::Vec2 position, bool present = true)
{
	return {
		Id(npcId),
		Id("profile:runtime-ai-move-runner"),
		Id("faction:runtime-ai-move-runner"),
		position,
		Id("goal:runtime-ai-move-runner"),
		present,
	};
}

iggy::NpcActorControlState2D SeekingControl(
	const char *npcId,
	iggy::Vec2 target,
	iggy::NpcMoveMode moveMode = iggy::NpcMoveMode::Still)
{
	return {
		Id(npcId),
		iggy::moveToNpcObjective(target),
		iggy::seekingNpcBehaviorState(target),
		moveMode,
	};
}

iggy::NpcActorControlState2D IdleControl(const char *npcId)
{
	return {
		Id(npcId),
		iggy::waitNpcObjective(),
		iggy::idleNpcBehaviorState(),
		iggy::NpcMoveMode::Still,
	};
}

iggy::NpcActorState2DRegistry Actors(std::vector<iggy::NpcActorState2D> actors)
{
	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build(actors);
	Expect(result.built, "runtime ai movement runner actor registry should build");
	return result.registry;
}

iggy::NpcActorControlState2DRegistry Controls(std::vector<iggy::NpcActorControlState2D> controls)
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);
	Expect(result.built, "runtime ai movement runner control registry should build");
	return result.registry;
}

iggy::NpcMapPlayControlFramePlanSubject Subject(
	const char *npcId,
	iggy::Vec2 target)
{
	iggy::NpcMapPlayControlFramePlanSubject subject;
	subject.npcId = Id(npcId);
	subject.hasProposalContextOverride = true;
	subject.proposalContext.hasTargetPosition = true;
	subject.proposalContext.targetPosition = target;
	return subject;
}

iggy::NpcMapPlayControlFramePlanSubject SubjectNoOverride(const char *npcId)
{
	iggy::NpcMapPlayControlFramePlanSubject subject;
	subject.npcId = Id(npcId);
	return subject;
}

iggy::NpcStrengthEnt StrengthEnt(
	const char *entryId,
	const char *actionTag,
	iggy::NpcBehaviorStateType behavior,
	float weight,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return { Id(entryId), 0, behavior, Id(actionTag), weight, mapTags };
}

iggy::NpcDexterityEnt DexterityEnt(
	const char *entryId,
	const char *actionTag,
	iggy::NpcBehaviorStateType behavior,
	float weight,
	std::vector<iggy::ResourceId> mapTags = {})
{
	return { Id(entryId), 0, behavior, Id(actionTag), weight, mapTags };
}

iggy::NpcMapPlayControlFramePlanPools Pools(
	std::vector<iggy::NpcStrengthEnt> strength = {},
	std::vector<iggy::NpcDexterityEnt> dexterity = {})
{
	iggy::NpcMapPlayControlFramePlanPools pools;
	const iggy::NpcStrengthPoolBuildResult strengthBuild =
		iggy::NpcStrengthPoolBuilder {}.build(strength);
	const iggy::NpcDexterityPoolBuildResult dexterityBuild =
		iggy::NpcDexterityPoolBuilder {}.build(dexterity);
	const iggy::NpcConstitutionPoolBuildResult constitutionBuild =
		iggy::NpcConstitutionPoolBuilder {}.build({});
	const iggy::NpcIntelligencePoolBuildResult intelligenceBuild =
		iggy::NpcIntelligencePoolBuilder {}.build({});
	const iggy::NpcWisdomPoolBuildResult wisdomBuild =
		iggy::NpcWisdomPoolBuilder {}.build({});
	const iggy::NpcCharismaPoolBuildResult charismaBuild =
		iggy::NpcCharismaPoolBuilder {}.build({});
	Expect(strengthBuild.built, "runtime ai movement runner strength pool should build");
	Expect(dexterityBuild.built, "runtime ai movement runner dexterity pool should build");
	Expect(constitutionBuild.built, "runtime ai movement runner constitution pool should build");
	Expect(intelligenceBuild.built, "runtime ai movement runner intelligence pool should build");
	Expect(wisdomBuild.built, "runtime ai movement runner wisdom pool should build");
	Expect(charismaBuild.built, "runtime ai movement runner charisma pool should build");
	pools.strength = strengthBuild.pool;
	pools.dexterity = dexterityBuild.pool;
	pools.constitution = constitutionBuild.pool;
	pools.intelligence = intelligenceBuild.pool;
	pools.wisdom = wisdomBuild.pool;
	pools.charisma = charismaBuild.pool;
	return pools;
}

iggy::runtime::RuntimeGameplayState GameplayState(const iggy::LevelTileMap &map)
{
	iggy::runtime::RuntimeGameplayState state;
	state.session.tickIndex = 29;
	state.session.level.map = map;
	state.commandQueue.frames = { { { iggy::runtime::GameplayCommand2DFactory {}.wait(Id("player:queued")) } } };
	state.inventory.inventory.stacks = { { Id("item:runtime-ai-move-runner"), 2 } };
	return state;
}

iggy::runtime::RuntimeNpcAiMovementPlannedFrameRunnerFrame Frame(
	std::vector<iggy::NpcMapPlayControlFramePlanSubject> subjects,
	const iggy::NpcMapPlayControlFramePlanPools &pools,
	const iggy::AiMap2D &aiMap,
	const iggy::LevelTileMap &movementMap,
	const iggy::NpcMapPlayControlFramePlanConfig &controlConfig = {},
	const iggy::NpcActorMovementFramePlan2DConfig &movementConfig = {})
{
	return {
		subjects,
		pools,
		aiMap,
		controlConfig,
		movementMap,
		movementConfig,
	};
}

iggy::runtime::RuntimeNpcAiMovementPlannedFrameRunnerResult Run(
	const iggy::runtime::RuntimeGameplayState &state,
	const std::vector<iggy::runtime::RuntimeNpcAiMovementPlannedFrameRunnerFrame> &frames)
{
	return iggy::runtime::RuntimeNpcAiMovementPlannedFrameRunner {}.run({ state, frames });
}

const iggy::NpcActorState2D *FindActor(
	const iggy::NpcActorState2DRegistry &registry,
	const iggy::ResourceId &npcId)
{
	for (const iggy::NpcActorState2D &actor : registry.actors) {
		if (actor.npcId == npcId)
			return &actor;
	}
	return nullptr;
}

bool SameActors(
	const iggy::NpcActorState2DRegistry &actual,
	const iggy::NpcActorState2DRegistry &expected)
{
	if (actual.actors.size() != expected.actors.size())
		return false;
	for (std::size_t index = 0; index < actual.actors.size(); ++index) {
		const iggy::NpcActorState2D &left = actual.actors[index];
		const iggy::NpcActorState2D &right = expected.actors[index];
		if (left.npcId != right.npcId
			|| left.aiProfileId != right.aiProfileId
			|| left.factionId != right.factionId
			|| !NearVec(left.position, right.position)
			|| left.currentGoalId != right.currentGoalId
			|| left.present != right.present)
			return false;
	}
	return true;
}

bool SameControls(
	const iggy::NpcActorControlState2DRegistry &actual,
	const iggy::NpcActorControlState2DRegistry &expected)
{
	if (actual.entries.size() != expected.entries.size())
		return false;
	for (std::size_t index = 0; index < actual.entries.size(); ++index) {
		const iggy::NpcActorControlState2D &left = actual.entries[index];
		const iggy::NpcActorControlState2D &right = expected.entries[index];
		if (left.npcId != right.npcId
			|| left.objective.type != right.objective.type
			|| left.objective.targetId != right.objective.targetId
			|| !NearVec(left.objective.targetPosition, right.objective.targetPosition)
			|| left.behavior.type != right.behavior.type
			|| left.behavior.targetId != right.behavior.targetId
			|| !NearVec(left.behavior.targetPosition, right.behavior.targetPosition)
			|| left.moveMode != right.moveMode)
			return false;
	}
	return true;
}

std::size_t OccupantCountAt(
	const iggy::NpcActorState2DRegistry &registry,
	iggy::TileCoord tile)
{
	const iggy::NpcActorOccupancy2D occupancy =
		iggy::NpcActorOccupancyProjector2D {}.project(registry);
	for (const iggy::NpcActorOccupiedTile2D &occupied : occupancy.occupiedTiles) {
		if (occupied.tile == tile)
			return occupied.npcIds.size();
	}
	return 0;
}

void ExpectNonNpcStatePreserved(
	const iggy::runtime::RuntimeGameplayState &actual,
	const iggy::runtime::RuntimeGameplayState &expected)
{
	Expect(actual.session.tickIndex == expected.session.tickIndex, "ai movement runner should preserve session tick");
	Expect(actual.commandQueue.frames.size() == expected.commandQueue.frames.size(), "ai movement runner should preserve command queue");
	Expect(actual.interaction.targets.targets().size() == expected.interaction.targets.targets().size(), "ai movement runner should preserve interaction state");
	Expect(actual.inventory.inventory.stacks.size() == expected.inventory.inventory.stacks.size(), "ai movement runner should preserve inventory state");
}

void TestEmptyFrameListNoOps()
{
	const iggy::LevelTileMap map = LevelMap({ "..." });
	const iggy::runtime::RuntimeGameplayState state = GameplayState(map);

	const iggy::runtime::RuntimeNpcAiMovementPlannedFrameRunnerResult result = Run(state, {});

	Expect(!result.hasFrames(), "empty ai movement runner should have no frames");
	Expect(!result.changed(), "empty ai movement runner should not change");
	Expect(result.frameCount == 0 && result.frameResults.empty(), "empty ai movement runner should preserve no frame results");
	Expect(SameActors(result.state.npcActors, state.npcActors), "empty ai movement runner should preserve actors");
	Expect(SameControls(result.state.npcControls, state.npcControls), "empty ai movement runner should preserve controls");
}

void TestSingleFrameMatchesDirectStep()
{
	const iggy::LevelTileMap map = LevelMap({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:single", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:single", { 3.5F, 0.5F }) });
	const std::vector<iggy::runtime::RuntimeNpcAiMovementPlannedFrameRunnerFrame> frames {
		Frame(
			{ Subject("npc:single", { 3.5F, 0.5F }) },
			Pools({ StrengthEnt("strength:single", "action:single", iggy::NpcBehaviorStateType::Seeking, 1.0F) }),
			AiMap(),
			map),
	};

	const iggy::runtime::RuntimeNpcAiMovementPlannedFrameResult direct =
		iggy::runtime::RuntimeNpcAiMovementPlannedFrameStep {}.run({
			state,
			frames[0].subjects,
			frames[0].pools,
			frames[0].aiMap,
			frames[0].controlConfig,
			{},
			frames[0].movementMap,
			frames[0].movementConfig,
		});
	const iggy::runtime::RuntimeNpcAiMovementPlannedFrameRunnerResult result = Run(state, frames);

	Expect(result.hasFrames() && result.frameCount == 1, "single ai movement runner should report one frame");
	Expect(SameActors(result.state.npcActors, direct.state.npcActors), "single ai movement runner should match direct step actors");
	Expect(SameControls(result.state.npcControls, direct.state.npcControls), "single ai movement runner should match direct step controls");
	Expect(result.controlPlannedRequestCount == direct.controlPlannedRequestCount, "single ai movement runner should aggregate control requests");
	Expect(result.movedCount == direct.movedCount, "single ai movement runner should aggregate moved count");
}

void TestTwoFrameSequenceCarriesState()
{
	const iggy::LevelTileMap map = LevelMap({ "....." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:carry", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:carry", { 4.5F, 0.5F }) });
	const iggy::NpcMapPlayControlFramePlanPools pools =
		Pools({ StrengthEnt("strength:carry", "action:carry", iggy::NpcBehaviorStateType::Seeking, 1.0F) });
	const std::vector<iggy::runtime::RuntimeNpcAiMovementPlannedFrameRunnerFrame> frames {
		Frame({ Subject("npc:carry", { 4.5F, 0.5F }) }, pools, AiMap(), map),
		Frame({ Subject("npc:carry", { 4.5F, 0.5F }) }, pools, AiMap(), map),
	};

	const iggy::runtime::RuntimeNpcAiMovementPlannedFrameRunnerResult result = Run(state, frames);
	const iggy::NpcActorState2D *actor = FindActor(result.state.npcActors, Id("npc:carry"));

	Expect(result.frameResults.size() == 2 && result.movedCount == 2, "two-frame ai movement runner should move twice");
	Expect(actor != nullptr && NearVec(actor->position, { 2.5F, 0.5F }), "two-frame ai movement runner should carry actor position forward");
	Expect(result.changedFrameCount == 2 && result.actorChangedFrameCount == 2, "two-frame ai movement runner should count changed actor frames");
	Expect(result.needsOccupancyRebuild && result.needsRenderRefresh && result.needsVisibilityRefresh, "two-frame ai movement runner should OR movement refresh flags");
}

void TestMapAwareSelectionCanChangeBetweenFrames()
{
	const iggy::LevelTileMap map = LevelMap({ "....." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:map-aware", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:map-aware", { 4.5F, 0.5F }) });
	iggy::NpcMapPlayControlFramePlanConfig controlConfig;
	controlConfig.step.mapRead.matchedMapTagBonus = 4.0F;
	const iggy::NpcMapPlayControlFramePlanPools pools = Pools(
		{ StrengthEnt("strength:cover", "action:cover", iggy::NpcBehaviorStateType::Seeking, 2.0F, { Id("zone:cover") }) },
		{ DexterityEnt("dexterity:speed", "action:speed", iggy::NpcBehaviorStateType::Seeking, 5.0F, { Id("zone:speed") }) });
	const std::vector<iggy::runtime::RuntimeNpcAiMovementPlannedFrameRunnerFrame> frames {
		Frame({ Subject("npc:map-aware", { 4.5F, 0.5F }) }, pools, AiMap({ AiNode("ai:cover", { 0.5F, 0.5F }, { Id("zone:cover") }) }), map, controlConfig),
		Frame({ Subject("npc:map-aware", { 4.5F, 0.5F }) }, pools, AiMap({ AiNode("ai:speed", { 1.5F, 0.5F }, { Id("zone:speed") }) }), map, controlConfig),
	};

	const iggy::runtime::RuntimeNpcAiMovementPlannedFrameRunnerResult result = Run(state, frames);

	Expect(result.controlMapChangedSelectionCount == 1, "map-aware ai movement runner should count only changed map selection frame");
	Expect(result.frameResults[0].control.step.entries[0].mapPlay.mapSelectedActionTag == Id("action:cover"), "first frame should use map cover action");
	Expect(result.frameResults[1].control.step.entries[0].mapPlay.mapSelectedActionTag == Id("action:speed"), "second frame should use changed map action");
	Expect(result.movedCount == 2, "map-aware ai movement runner should still move across both frames");
}

void TestFoldedFrameCarriesExistingControl()
{
	const iggy::LevelTileMap map = LevelMap({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:folded", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:folded", { 3.5F, 0.5F }, iggy::NpcMoveMode::Walk) });
	const std::vector<iggy::runtime::RuntimeNpcAiMovementPlannedFrameRunnerFrame> frames {
		Frame({ SubjectNoOverride("npc:folded") }, Pools(), AiMap(), map),
		Frame({ Subject("npc:folded", { 3.5F, 0.5F }) }, Pools({ StrengthEnt("strength:folded", "action:folded", iggy::NpcBehaviorStateType::Seeking, 1.0F) }), AiMap(), map),
	};

	const iggy::runtime::RuntimeNpcAiMovementPlannedFrameRunnerResult result = Run(state, frames);
	const iggy::NpcActorState2D *actor = FindActor(result.state.npcActors, Id("npc:folded"));

	Expect(result.controlFailedCount == 1, "folded frame should aggregate failed control diagnostics");
	Expect(result.movedCount == 2, "folded frame should still move using existing carried control");
	Expect(actor != nullptr && NearVec(actor->position, { 2.5F, 0.5F }), "folded frame runner should carry actor state through subsequent frame");
}

void TestBlockedFrameCarriesUnmovedActor()
{
	const iggy::LevelTileMap map = LevelMap({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({
		Actor("npc:mover", { 0.5F, 0.5F }),
		Actor("npc:blocker", { 1.5F, 0.5F }),
	});
	state.npcControls = Controls({
		SeekingControl("npc:mover", { 3.5F, 0.5F }),
		IdleControl("npc:blocker"),
	});
	const iggy::NpcMapPlayControlFramePlanPools pools =
		Pools({ StrengthEnt("strength:block", "action:block", iggy::NpcBehaviorStateType::Seeking, 1.0F) });
	const std::vector<iggy::runtime::RuntimeNpcAiMovementPlannedFrameRunnerFrame> frames {
		Frame({ Subject("npc:mover", { 3.5F, 0.5F }), SubjectNoOverride("npc:blocker") }, pools, AiMap(), map),
		Frame({ Subject("npc:mover", { 3.5F, 0.5F }), SubjectNoOverride("npc:blocker") }, pools, AiMap(), map),
	};

	const iggy::runtime::RuntimeNpcAiMovementPlannedFrameRunnerResult result = Run(state, frames);
	const iggy::NpcActorState2D *mover = FindActor(result.state.npcActors, Id("npc:mover"));

	Expect(result.blockedMovementCount == 2 && result.movedCount == 0, "blocked ai movement runner should aggregate blocked movement");
	Expect(result.controlChangedFrameCount == 2, "blocked ai movement runner should count both applied control frames");
	Expect(mover != nullptr && NearVec(mover->position, { 0.5F, 0.5F }), "blocked ai movement runner should carry unmoved actor into next frame");
}

void TestReservationCapacityAggregatesAcrossFrames()
{
	const iggy::LevelTileMap map = LevelMap({ "..." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({
		Actor("npc:first", { 0.5F, 0.5F }),
		Actor("npc:second", { 2.5F, 0.5F }),
	});
	state.npcControls = Controls({
		SeekingControl("npc:first", { 1.5F, 0.5F }),
		SeekingControl("npc:second", { 1.5F, 0.5F }),
	});
	iggy::NpcActorMovementFramePlan2DConfig movementConfig;
	movementConfig.runReservation = true;
	movementConfig.reservation.policy.maxOccupantsPerTile = 2;
	const std::vector<iggy::runtime::RuntimeNpcAiMovementPlannedFrameRunnerFrame> frames {
		Frame(
			{ Subject("npc:first", { 1.5F, 0.5F }), Subject("npc:second", { 1.5F, 0.5F }) },
			Pools({ StrengthEnt("strength:stack", "action:stack", iggy::NpcBehaviorStateType::Seeking, 1.0F) }),
			AiMap(),
			map,
			{},
			movementConfig),
	};

	const iggy::runtime::RuntimeNpcAiMovementPlannedFrameRunnerResult result = Run(state, frames);

	Expect(result.movementReservationAcceptedCount == 2 && result.movementReservationRejectedCount == 0, "reservation ai movement runner should aggregate accepted reservations");
	Expect(result.movedCount == 2, "reservation ai movement runner should aggregate moved actors");
	Expect(OccupantCountAt(result.state.npcActors, { 1, 0 }) == 2, "reservation ai movement runner should leave stacked occupancy inspectable");
}

void TestInputsAreNotMutated()
{
	const iggy::LevelTileMap map = LevelMap({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:immutable", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:immutable", { 3.5F, 0.5F }) });
	std::vector<iggy::runtime::RuntimeNpcAiMovementPlannedFrameRunnerFrame> frames {
		Frame(
			{ Subject("npc:immutable", { 3.5F, 0.5F }) },
			Pools({ StrengthEnt("strength:immutable", "action:immutable", iggy::NpcBehaviorStateType::Seeking, 1.0F) }),
			AiMap({ AiNode("ai:immutable", { 0.5F, 0.5F }, { Id("zone:immutable") }) }),
			map),
	};
	const iggy::runtime::RuntimeGameplayState beforeState = state;
	const std::vector<iggy::runtime::RuntimeNpcAiMovementPlannedFrameRunnerFrame> beforeFrames = frames;

	(void)Run(state, frames);

	Expect(SameActors(state.npcActors, beforeState.npcActors), "ai movement runner should not mutate input actors");
	Expect(SameControls(state.npcControls, beforeState.npcControls), "ai movement runner should not mutate input controls");
	Expect(frames.size() == beforeFrames.size(), "ai movement runner should not mutate input frames");
	Expect(frames[0].subjects.size() == beforeFrames[0].subjects.size(), "ai movement runner should not mutate frame subjects");
	Expect(frames[0].pools.strength.entries.size() == beforeFrames[0].pools.strength.entries.size(), "ai movement runner should not mutate frame pools");
	Expect(frames[0].aiMap.nodes.size() == beforeFrames[0].aiMap.nodes.size(), "ai movement runner should not mutate frame ai map");
	Expect(frames[0].movementMap.tiles.size() == beforeFrames[0].movementMap.tiles.size(), "ai movement runner should not mutate frame movement map");
}

void TestManualLoopMatchesRunner()
{
	const iggy::LevelTileMap map = LevelMap({ "....." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:manual", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:manual", { 4.5F, 0.5F }) });
	const iggy::NpcMapPlayControlFramePlanPools pools =
		Pools({ StrengthEnt("strength:manual", "action:manual", iggy::NpcBehaviorStateType::Seeking, 1.0F) });
	const std::vector<iggy::runtime::RuntimeNpcAiMovementPlannedFrameRunnerFrame> frames {
		Frame({ Subject("npc:manual", { 4.5F, 0.5F }) }, pools, AiMap(), map),
		Frame({ Subject("npc:manual", { 4.5F, 0.5F }) }, pools, AiMap(), map),
	};

	iggy::runtime::RuntimeGameplayState manualState = state;
	std::size_t manualMovedCount = 0;
	std::size_t manualControlRequests = 0;
	iggy::runtime::RuntimeNpcAiMovementPlannedFrameStep step;
	for (const iggy::runtime::RuntimeNpcAiMovementPlannedFrameRunnerFrame &frame : frames) {
		const iggy::runtime::RuntimeNpcAiMovementPlannedFrameResult frameResult = step.run({
			manualState,
			frame.subjects,
			frame.pools,
			frame.aiMap,
			frame.controlConfig,
			{},
			frame.movementMap,
			frame.movementConfig,
		});
		manualState = frameResult.state;
		manualMovedCount += frameResult.movedCount;
		manualControlRequests += frameResult.controlPlannedRequestCount;
	}
	const iggy::runtime::RuntimeNpcAiMovementPlannedFrameRunnerResult result = Run(state, frames);

	Expect(SameActors(result.state.npcActors, manualState.npcActors), "manual loop should match runner actors");
	Expect(SameControls(result.state.npcControls, manualState.npcControls), "manual loop should match runner controls");
	Expect(result.movedCount == manualMovedCount, "manual loop should match runner moved count");
	Expect(result.controlPlannedRequestCount == manualControlRequests, "manual loop should match runner control request count");
	ExpectNonNpcStatePreserved(result.state, state);
}

} // namespace

int main()
{
	TestEmptyFrameListNoOps();
	TestSingleFrameMatchesDirectStep();
	TestTwoFrameSequenceCarriesState();
	TestMapAwareSelectionCanChangeBetweenFrames();
	TestFoldedFrameCarriesExistingControl();
	TestBlockedFrameCarriesUnmovedActor();
	TestReservationCapacityAggregatesAcrossFrames();
	TestInputsAreNotMutated();
	TestManualLoopMatchesRunner();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
