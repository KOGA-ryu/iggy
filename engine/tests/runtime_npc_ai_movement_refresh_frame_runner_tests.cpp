#include <cstdlib>
#include <string_view>
#include <type_traits>
#include <vector>

#include "runtime/RuntimeNpcAiMovementRefreshFrameRunner.hpp"
#include "runtime/RuntimeSessionState.hpp"
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
	map.id = Id("level:runtime-ai-move-refresh-runner");
	return map;
}

iggy::AiMapNode2D AiNode(const char *id, iggy::Vec2 position)
{
	return { Id(id), position, 1.0F, 1.0F, 2.0F, 3.0F, 4.0F, {}, {}, true };
}

iggy::AiMap2D AiMap(std::vector<iggy::AiMapNode2D> nodes = {})
{
	const iggy::AiMap2DBuildResult result = iggy::AiMap2DBuilder {}.build(nodes);
	Expect(result.built, "runtime ai movement refresh runner ai map should build");
	return result.map;
}

iggy::InteractionTarget2DRegistry Targets(std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result =
		iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built, "runtime ai movement refresh runner target registry should build");
	return result.registry;
}

iggy::InteractionTarget2D Target(const char *id, iggy::Vec2 position)
{
	return { Id(id), iggy::InteractionTarget2DKind::Usable, position, 0.5F, true };
}

iggy::NpcActorState2D Actor(const char *npcId, iggy::Vec2 position)
{
	return { Id(npcId), Id("profile:refresh-runner"), Id("faction:refresh-runner"), position, Id("goal:refresh-runner"), true };
}

iggy::NpcActorControlState2D SeekingControl(
	const char *npcId,
	iggy::Vec2 target,
	iggy::NpcMoveMode moveMode = iggy::NpcMoveMode::Still)
{
	return { Id(npcId), iggy::moveToNpcObjective(target), iggy::seekingNpcBehaviorState(target), moveMode };
}

iggy::NpcActorControlState2D IdleControl(const char *npcId)
{
	return { Id(npcId), iggy::waitNpcObjective(), iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still };
}

iggy::NpcActorState2DRegistry Actors(std::vector<iggy::NpcActorState2D> actors)
{
	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build(actors);
	Expect(result.built, "runtime ai movement refresh runner actors should build");
	return result.registry;
}

iggy::NpcActorControlState2DRegistry Controls(std::vector<iggy::NpcActorControlState2D> controls)
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);
	Expect(result.built, "runtime ai movement refresh runner controls should build");
	return result.registry;
}

iggy::NpcMapPlayControlFramePlanSubject Subject(const char *npcId, iggy::Vec2 target)
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

iggy::NpcStrengthEnt StrengthEnt(const char *entryId, const char *actionTag, iggy::NpcBehaviorStateType behavior)
{
	return { Id(entryId), 0, behavior, Id(actionTag), 1.0F, {} };
}

iggy::NpcMapPlayControlFramePlanPools Pools(std::vector<iggy::NpcStrengthEnt> strength = {})
{
	iggy::NpcMapPlayControlFramePlanPools pools;
	const iggy::NpcStrengthPoolBuildResult strengthBuild =
		iggy::NpcStrengthPoolBuilder {}.build(strength);
	const iggy::NpcDexterityPoolBuildResult dexterityBuild =
		iggy::NpcDexterityPoolBuilder {}.build({});
	const iggy::NpcConstitutionPoolBuildResult constitutionBuild =
		iggy::NpcConstitutionPoolBuilder {}.build({});
	const iggy::NpcIntelligencePoolBuildResult intelligenceBuild =
		iggy::NpcIntelligencePoolBuilder {}.build({});
	const iggy::NpcWisdomPoolBuildResult wisdomBuild =
		iggy::NpcWisdomPoolBuilder {}.build({});
	const iggy::NpcCharismaPoolBuildResult charismaBuild =
		iggy::NpcCharismaPoolBuilder {}.build({});
	Expect(strengthBuild.built && dexterityBuild.built && constitutionBuild.built, "runtime ai movement refresh runner pools should build");
	Expect(intelligenceBuild.built && wisdomBuild.built && charismaBuild.built, "runtime ai movement refresh runner empty pools should build");
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
	state.session.tickIndex = 37;
	state.session.level.map = map;
	state.commandQueue.frames = { { { iggy::runtime::GameplayCommand2DFactory {}.wait(Id("player:queued")) } } };
	state.inventory.inventory.stacks = { { Id("item:refresh-runner"), 2 } };
	return state;
}

iggy::runtime::RuntimeNpcAiMovementRefreshFrameRunnerFrame Frame(
	std::vector<iggy::NpcMapPlayControlFramePlanSubject> subjects,
	const iggy::NpcMapPlayControlFramePlanPools &pools,
	const iggy::LevelTileMap &movementMap,
	const iggy::InteractionTarget2DRegistry &targets = {},
	const iggy::AiMap2D &refreshAiMap = {},
	const iggy::NpcActorMovementFramePlan2DConfig &movementConfig = {})
{
	return { subjects, pools, AiMap(), {}, movementMap, movementConfig, targets, refreshAiMap, {} };
}

iggy::runtime::RuntimeNpcAiMovementRefreshFrameRunnerResult Run(
	const iggy::runtime::RuntimeGameplayState &state,
	const std::vector<iggy::runtime::RuntimeNpcAiMovementRefreshFrameRunnerFrame> &frames)
{
	return iggy::runtime::RuntimeNpcAiMovementRefreshFrameRunner {}.run({ state, frames });
}

const iggy::NpcActorState2D *FindActor(const iggy::NpcActorState2DRegistry &registry, const iggy::ResourceId &npcId)
{
	for (const iggy::NpcActorState2D &actor : registry.actors) {
		if (actor.npcId == npcId)
			return &actor;
	}
	return nullptr;
}

bool SameActors(const iggy::NpcActorState2DRegistry &actual, const iggy::NpcActorState2DRegistry &expected)
{
	if (actual.actors.size() != expected.actors.size())
		return false;
	for (std::size_t index = 0; index < actual.actors.size(); ++index) {
		if (actual.actors[index].npcId != expected.actors[index].npcId
			|| !NearVec(actual.actors[index].position, expected.actors[index].position)
			|| actual.actors[index].present != expected.actors[index].present)
			return false;
	}
	return true;
}

bool SameControls(const iggy::NpcActorControlState2DRegistry &actual, const iggy::NpcActorControlState2DRegistry &expected)
{
	if (actual.entries.size() != expected.entries.size())
		return false;
	for (std::size_t index = 0; index < actual.entries.size(); ++index) {
		if (actual.entries[index].npcId != expected.entries[index].npcId
			|| actual.entries[index].behavior.type != expected.entries[index].behavior.type
			|| !NearVec(actual.entries[index].behavior.targetPosition, expected.entries[index].behavior.targetPosition)
			|| actual.entries[index].moveMode != expected.entries[index].moveMode)
			return false;
	}
	return true;
}

std::size_t OccupantCountAt(const iggy::NpcActorState2DRegistry &registry, iggy::TileCoord tile)
{
	const iggy::NpcActorOccupancy2D occupancy =
		iggy::NpcActorOccupancyProjector2D {}.project(registry);
	for (const iggy::NpcActorOccupiedTile2D &occupied : occupancy.occupiedTiles) {
		if (occupied.tile == tile)
			return occupied.npcIds.size();
	}
	return 0;
}

void TestEmptyFrameListNoOps()
{
	const iggy::LevelTileMap map = LevelMap({ "..." });
	const iggy::runtime::RuntimeGameplayState state = GameplayState(map);

	const iggy::runtime::RuntimeNpcAiMovementRefreshFrameRunnerResult result = Run(state, {});

	Expect(!result.hasFrames() && !result.changed() && !result.refreshedAny(), "empty refresh runner should no-op");
	Expect(result.frameResults.empty(), "empty refresh runner should have no frame results");
	Expect(SameActors(result.state.npcActors, state.npcActors), "empty refresh runner should preserve actors");
	Expect(SameControls(result.state.npcControls, state.npcControls), "empty refresh runner should preserve controls");
}

void TestTwoFrameSequenceCarriesStateAndRefreshesEachFrame()
{
	const iggy::LevelTileMap map = LevelMap({ "....." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:carry", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:carry", { 4.5F, 0.5F }) });
	const iggy::NpcMapPlayControlFramePlanPools pools =
		Pools({ StrengthEnt("strength:carry", "action:carry", iggy::NpcBehaviorStateType::Seeking) });
	const std::vector<iggy::runtime::RuntimeNpcAiMovementRefreshFrameRunnerFrame> frames {
		Frame({ Subject("npc:carry", { 4.5F, 0.5F }) }, pools, map, Targets({ Target("target:one", { 1.5F, 0.5F }) }), AiMap({ AiNode("ai:one", { 1.5F, 0.5F }) })),
		Frame({ Subject("npc:carry", { 4.5F, 0.5F }) }, pools, map, Targets({ Target("target:two", { 2.5F, 0.5F }) }), AiMap({ AiNode("ai:two", { 2.5F, 0.5F }) })),
	};

	const iggy::runtime::RuntimeNpcAiMovementRefreshFrameRunnerResult result = Run(state, frames);
	const iggy::NpcActorState2D *actor = FindActor(result.state.npcActors, Id("npc:carry"));

	Expect(result.frameResults.size() == 2 && result.movedCount == 2, "refresh runner should move in both frames");
	Expect(actor != nullptr && NearVec(actor->position, { 2.5F, 0.5F }), "refresh runner should carry actor position");
	Expect(result.occupancyRefreshCount == 2 && result.renderRefreshCount == 2 && result.visibilityRefreshCount == 2, "refresh runner should aggregate refreshes per moved frame");
	Expect(result.interactionRefreshCount == 2 && result.aiMapRefreshCount == 2, "refresh runner should aggregate interaction and AI map refreshes");
}

void TestBlockedThenMovedAggregatesRefreshOnlyForMovedFrame()
{
	const iggy::LevelTileMap map = LevelMap({ "....." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({
		Actor("npc:mover", { 0.5F, 0.5F }),
		Actor("npc:blocker", { 1.5F, 0.5F }),
		Actor("npc:free", { 3.5F, 0.5F }),
	});
	state.npcControls = Controls({
		SeekingControl("npc:mover", { 3.5F, 0.5F }),
		IdleControl("npc:blocker"),
		SeekingControl("npc:free", { 4.5F, 0.5F }),
	});
	const iggy::NpcMapPlayControlFramePlanPools pools =
		Pools({ StrengthEnt("strength:mover", "action:mover", iggy::NpcBehaviorStateType::Seeking) });
	const std::vector<iggy::runtime::RuntimeNpcAiMovementRefreshFrameRunnerFrame> frames {
		Frame({ Subject("npc:mover", { 3.5F, 0.5F }), SubjectNoOverride("npc:blocker") }, pools, map),
		Frame({ Subject("npc:free", { 4.5F, 0.5F }) }, pools, map),
	};

	const iggy::runtime::RuntimeNpcAiMovementRefreshFrameRunnerResult result = Run(state, frames);

	Expect(result.blockedMovementCount == 2 && result.movedCount == 1, "mixed refresh runner should aggregate carried blocked movement and moved frame");
	Expect(result.occupancyRefreshCount == 1 && result.dirtyTileCount == 2, "mixed refresh runner should refresh only moved frame");
	Expect(result.frameResults[0].refresh.dirtyTileCount == 0, "blocked frame should have no refresh dirty tiles");
	Expect(result.frameResults[1].refresh.dirtyTileCount == 2, "moved frame should have refresh dirty tiles");
}

void TestCapacityStackedMovementRefreshesInspectableOccupancy()
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
	const std::vector<iggy::runtime::RuntimeNpcAiMovementRefreshFrameRunnerFrame> frames {
		Frame(
			{ Subject("npc:first", { 1.5F, 0.5F }), Subject("npc:second", { 1.5F, 0.5F }) },
			Pools({ StrengthEnt("strength:stack", "action:stack", iggy::NpcBehaviorStateType::Seeking) }),
			map,
			Targets({}),
			AiMap(),
			movementConfig),
	};

	const iggy::runtime::RuntimeNpcAiMovementRefreshFrameRunnerResult result = Run(state, frames);

	Expect(result.movedCount == 2 && result.occupancyRefreshCount == 1, "stacked refresh runner should move both and refresh occupancy");
	Expect(OccupantCountAt(result.state.npcActors, { 1, 0 }) == 2, "stacked refresh runner should leave final occupancy inspectable");
	Expect(result.frameResults[0].refresh.occupancy.occupancy.hasIssues(), "stacked refresh runner should preserve duplicate occupancy issue");
}

void TestInputsAreNotMutatedAndManualLoopMatches()
{
	const iggy::LevelTileMap map = LevelMap({ "....." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:manual", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:manual", { 4.5F, 0.5F }) });
	const iggy::NpcMapPlayControlFramePlanPools pools =
		Pools({ StrengthEnt("strength:manual", "action:manual", iggy::NpcBehaviorStateType::Seeking) });
	std::vector<iggy::runtime::RuntimeNpcAiMovementRefreshFrameRunnerFrame> frames {
		Frame({ Subject("npc:manual", { 4.5F, 0.5F }) }, pools, map),
		Frame({ Subject("npc:manual", { 4.5F, 0.5F }) }, pools, map),
	};
	const iggy::runtime::RuntimeGameplayState beforeState = state;
	const std::vector<iggy::runtime::RuntimeNpcAiMovementRefreshFrameRunnerFrame> beforeFrames = frames;

	iggy::runtime::RuntimeGameplayState manualState = state;
	std::size_t manualMovedCount = 0;
	iggy::runtime::RuntimeNpcAiMovementRefreshFrameStep step;
	for (const iggy::runtime::RuntimeNpcAiMovementRefreshFrameRunnerFrame &frame : frames) {
		const iggy::NpcActorOccupancy2D previousOccupancy =
			iggy::NpcActorOccupancyProjector2D {}.project(manualState.npcActors);
		const iggy::runtime::RuntimeNpcAiMovementRefreshFrameResult frameResult = step.run({
			{
				manualState,
				frame.subjects,
				frame.pools,
				frame.aiMap,
				frame.controlConfig,
				{},
				frame.movementMap,
				frame.movementConfig,
			},
			previousOccupancy,
			frame.interactionTargets,
			frame.refreshAiMap,
			frame.refreshConfig,
		});
		manualState = frameResult.state;
		manualMovedCount += frameResult.movedCount;
	}
	const iggy::runtime::RuntimeNpcAiMovementRefreshFrameRunnerResult result = Run(state, frames);

	Expect(SameActors(result.state.npcActors, manualState.npcActors), "manual refresh loop should match runner actors");
	Expect(SameControls(result.state.npcControls, manualState.npcControls), "manual refresh loop should match runner controls");
	Expect(result.movedCount == manualMovedCount, "manual refresh loop should match moved count");
	Expect(SameActors(state.npcActors, beforeState.npcActors), "refresh runner should not mutate input actors");
	Expect(SameControls(state.npcControls, beforeState.npcControls), "refresh runner should not mutate input controls");
	Expect(frames.size() == beforeFrames.size(), "refresh runner should not mutate frames");
}

} // namespace

int main()
{
	TestEmptyFrameListNoOps();
	TestTwoFrameSequenceCarriesStateAndRefreshesEachFrame();
	TestBlockedThenMovedAggregatesRefreshOnlyForMovedFrame();
	TestCapacityStackedMovementRefreshesInspectableOccupancy();
	TestInputsAreNotMutatedAndManualLoopMatches();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
