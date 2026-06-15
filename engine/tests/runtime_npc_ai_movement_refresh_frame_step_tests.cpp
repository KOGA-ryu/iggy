#include <cstdlib>
#include <string_view>
#include <type_traits>
#include <vector>

#include "runtime/RuntimeNpcAiMovementRefreshFrameStep.hpp"
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
	map.id = Id("level:runtime-ai-move-refresh");
	return map;
}

iggy::AiMapNode2D AiNode(const char *id, iggy::Vec2 position, std::vector<iggy::ResourceId> tags = {})
{
	return { Id(id), position, 1.0F, 1.0F, 2.0F, 3.0F, 4.0F, tags, {}, true };
}

iggy::AiMap2D AiMap(std::vector<iggy::AiMapNode2D> nodes = {})
{
	const iggy::AiMap2DBuildResult result = iggy::AiMap2DBuilder {}.build(nodes);
	Expect(result.built, "runtime ai movement refresh ai map should build");
	return result.map;
}

iggy::InteractionTarget2DRegistry Targets(std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result =
		iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built, "runtime ai movement refresh target registry should build");
	return result.registry;
}

iggy::InteractionTarget2D Target(const char *id, iggy::Vec2 position)
{
	return { Id(id), iggy::InteractionTarget2DKind::Usable, position, 0.5F, true };
}

iggy::NpcActorState2D Actor(const char *npcId, iggy::Vec2 position)
{
	return {
		Id(npcId),
		Id("profile:runtime-ai-move-refresh"),
		Id("faction:runtime-ai-move-refresh"),
		position,
		Id("goal:runtime-ai-move-refresh"),
		true,
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
	return { Id(npcId), iggy::waitNpcObjective(), iggy::idleNpcBehaviorState(), iggy::NpcMoveMode::Still };
}

iggy::NpcActorState2DRegistry Actors(std::vector<iggy::NpcActorState2D> actors)
{
	const iggy::NpcActorState2DRegistryBuildResult result =
		iggy::NpcActorState2DRegistryBuilder {}.build(actors);
	Expect(result.built, "runtime ai movement refresh actor registry should build");
	return result.registry;
}

iggy::NpcActorControlState2DRegistry Controls(std::vector<iggy::NpcActorControlState2D> controls)
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);
	Expect(result.built, "runtime ai movement refresh control registry should build");
	return result.registry;
}

iggy::NpcActorOccupancy2D Occupancy(const iggy::NpcActorState2DRegistry &actors)
{
	return iggy::NpcActorOccupancyProjector2D {}.project(actors);
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
	Expect(strengthBuild.built, "runtime ai movement refresh strength pool should build");
	Expect(dexterityBuild.built, "runtime ai movement refresh dexterity pool should build");
	Expect(constitutionBuild.built, "runtime ai movement refresh constitution pool should build");
	Expect(intelligenceBuild.built, "runtime ai movement refresh intelligence pool should build");
	Expect(wisdomBuild.built, "runtime ai movement refresh wisdom pool should build");
	Expect(charismaBuild.built, "runtime ai movement refresh charisma pool should build");
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
	state.session.tickIndex = 31;
	state.session.level.map = map;
	state.commandQueue.frames = { { { iggy::runtime::GameplayCommand2DFactory {}.wait(Id("player:queued")) } } };
	state.inventory.inventory.stacks = { { Id("item:runtime-ai-move-refresh"), 2 } };
	return state;
}

iggy::runtime::RuntimeNpcAiMovementRefreshFrameInput Input(
	const iggy::runtime::RuntimeGameplayState &state,
	std::vector<iggy::NpcMapPlayControlFramePlanSubject> subjects,
	const iggy::NpcMapPlayControlFramePlanPools &pools,
	const iggy::AiMap2D &plannedAiMap,
	const iggy::LevelTileMap &movementMap,
	const iggy::NpcActorOccupancy2D &previousOccupancy,
	const iggy::InteractionTarget2DRegistry &targets,
	const iggy::AiMap2D &refreshAiMap,
	const iggy::NpcMapPlayControlFramePlanConfig &controlConfig = {},
	const iggy::NpcActorMovementFramePlan2DConfig &movementConfig = {},
	const iggy::NpcActorMovementRefreshFrame2DConfig &refreshConfig = {})
{
	return {
		{ state, subjects, pools, plannedAiMap, controlConfig, movementMap, movementConfig },
		previousOccupancy,
		targets,
		refreshAiMap,
		refreshConfig,
	};
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
		const iggy::NpcActorState2D &left = actual.actors[index];
		const iggy::NpcActorState2D &right = expected.actors[index];
		if (left.npcId != right.npcId || !NearVec(left.position, right.position) || left.present != right.present)
			return false;
	}
	return true;
}

bool SameControls(const iggy::NpcActorControlState2DRegistry &actual, const iggy::NpcActorControlState2DRegistry &expected)
{
	if (actual.entries.size() != expected.entries.size())
		return false;
	for (std::size_t index = 0; index < actual.entries.size(); ++index) {
		const iggy::NpcActorControlState2D &left = actual.entries[index];
		const iggy::NpcActorControlState2D &right = expected.entries[index];
		if (left.npcId != right.npcId
			|| left.behavior.type != right.behavior.type
			|| !NearVec(left.behavior.targetPosition, right.behavior.targetPosition)
			|| left.moveMode != right.moveMode)
			return false;
	}
	return true;
}

bool OccupancyContains(const iggy::NpcActorOccupancy2D &occupancy, const iggy::ResourceId &npcId, iggy::TileCoord tile)
{
	for (const iggy::NpcActorOccupancyEntry2D &entry : occupancy.entries) {
		if (entry.npcId == npcId && entry.tile == tile)
			return true;
	}
	return false;
}

void TestEmptyDefaultNoOps()
{
	const iggy::LevelTileMap map = LevelMap({ "..." });
	const iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	const iggy::NpcActorOccupancy2D previousOccupancy = Occupancy(state.npcActors);

	const iggy::runtime::RuntimeNpcAiMovementRefreshFrameResult result =
		iggy::runtime::RuntimeNpcAiMovementRefreshFrameStep {}.run(
			Input(state, {}, Pools(), AiMap(), map, previousOccupancy, Targets({}), AiMap()));

	Expect(result.status == iggy::runtime::RuntimeNpcAiMovementRefreshFrameStatus::NoChanges, "empty refresh frame should report no changes");
	Expect(!result.changedState() && !result.refreshedAny(), "empty refresh frame should not change or refresh");
	Expect(result.dirtyTileCount == 0, "empty refresh frame should have no dirty tiles");
	Expect(SameActors(result.state.npcActors, state.npcActors), "empty refresh frame should preserve actors");
	Expect(SameControls(result.state.npcControls, state.npcControls), "empty refresh frame should preserve controls");
}

void TestMovedActorProducesRefreshPackets()
{
	const iggy::LevelTileMap map = LevelMap({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:mover", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:mover", { 3.5F, 0.5F }) });
	const iggy::NpcActorOccupancy2D previousOccupancy = Occupancy(state.npcActors);

	const iggy::runtime::RuntimeNpcAiMovementRefreshFrameResult result =
		iggy::runtime::RuntimeNpcAiMovementRefreshFrameStep {}.run(
			Input(
				state,
				{ Subject("npc:mover", { 3.5F, 0.5F }) },
				Pools({ StrengthEnt("strength:move", "action:move", iggy::NpcBehaviorStateType::Seeking) }),
				AiMap(),
				map,
				previousOccupancy,
				Targets({ Target("target:new-tile", { 1.5F, 0.5F }) }),
				AiMap({ AiNode("ai:new-tile", { 1.5F, 0.5F }, { Id("zone:new") }) })));

	const iggy::NpcActorState2D *actor = FindActor(result.state.npcActors, Id("npc:mover"));
	Expect(result.changedControls && result.changedActors, "moved refresh frame should change controls and actors");
	Expect(result.movedCount == 1 && result.dirtyTileCount == 2, "moved refresh frame should report movement and dirty old/new tiles");
	Expect(result.occupancyRefreshed && result.interactionRefreshed && result.aiMapRefreshed, "moved refresh frame should refresh data packets");
	Expect(result.renderRefreshed && result.visibilityRefreshed, "moved refresh frame should refresh visual packets");
	Expect(actor != nullptr && NearVec(actor->position, { 1.5F, 0.5F }), "moved refresh frame should return moved actor");
	Expect(OccupancyContains(result.refresh.occupancy.occupancy, Id("npc:mover"), { 1, 0 }), "moved refresh frame should rebuild occupancy from returned actors");
	Expect(result.refresh.interaction.affectedTargetCount == 1, "moved refresh frame should preserve affected interaction target");
	Expect(result.refresh.aiMap.affectedActorCount == 1, "moved refresh frame should preserve affected AI-map actor query");
	Expect(result.refresh.visual.render.affectedActorCount == 1 && result.refresh.visual.visibility.affectedActorCount == 1, "moved refresh frame should preserve visual affected actor facts");
}

void TestBlockedMovementProducesNoRefreshPackets()
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
	const iggy::NpcActorOccupancy2D previousOccupancy = Occupancy(state.npcActors);

	const iggy::runtime::RuntimeNpcAiMovementRefreshFrameResult result =
		iggy::runtime::RuntimeNpcAiMovementRefreshFrameStep {}.run(
			Input(
				state,
				{ Subject("npc:mover", { 3.5F, 0.5F }), SubjectNoOverride("npc:blocker") },
				Pools({ StrengthEnt("strength:block", "action:block", iggy::NpcBehaviorStateType::Seeking) }),
				AiMap(),
				map,
				previousOccupancy,
				Targets({ Target("target:block", { 1.5F, 0.5F }) }),
				AiMap({ AiNode("ai:block", { 1.5F, 0.5F }) })));

	Expect(result.changedControls, "blocked refresh frame should still preserve control diagnostics");
	Expect(result.blockedMovementCount == 1 && result.movedCount == 0, "blocked refresh frame should report blocked movement");
	Expect(!result.refreshedAny() && result.dirtyTileCount == 0, "blocked refresh frame should produce no refresh packets");
	Expect(SameActors(result.state.npcActors, state.npcActors), "blocked refresh frame should not move actors");
}

void TestFoldedNoPlayUsesExistingMovementReadyControl()
{
	const iggy::LevelTileMap map = LevelMap({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:folded", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:folded", { 3.5F, 0.5F }, iggy::NpcMoveMode::Walk) });
	const iggy::NpcActorOccupancy2D previousOccupancy = Occupancy(state.npcActors);

	const iggy::runtime::RuntimeNpcAiMovementRefreshFrameResult result =
		iggy::runtime::RuntimeNpcAiMovementRefreshFrameStep {}.run(
			Input(state, { SubjectNoOverride("npc:folded") }, Pools(), AiMap(), map, previousOccupancy, Targets({}), AiMap()));
	const iggy::NpcActorState2D *actor = FindActor(result.state.npcActors, Id("npc:folded"));

	Expect(result.controlFailedCount == 1 && !result.changedControls, "folded refresh frame should preserve failed/no-change control diagnostics");
	Expect(result.movedCount == 1 && result.refreshedAny(), "folded refresh frame should still move and refresh from existing control");
	Expect(actor != nullptr && NearVec(actor->position, { 1.5F, 0.5F }), "folded refresh frame should move using existing control");
}

void TestManualParityAndInputImmutability()
{
	const iggy::LevelTileMap map = LevelMap({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:manual", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:manual", { 3.5F, 0.5F }) });
	const std::vector<iggy::NpcMapPlayControlFramePlanSubject> subjects { Subject("npc:manual", { 3.5F, 0.5F }) };
	const iggy::NpcMapPlayControlFramePlanPools pools =
		Pools({ StrengthEnt("strength:manual", "action:manual", iggy::NpcBehaviorStateType::Seeking) });
	const iggy::AiMap2D plannedAiMap = AiMap();
	const iggy::NpcActorOccupancy2D previousOccupancy = Occupancy(state.npcActors);
	const iggy::InteractionTarget2DRegistry targets = Targets({ Target("target:manual", { 1.5F, 0.5F }) });
	const iggy::AiMap2D refreshAiMap = AiMap({ AiNode("ai:manual", { 1.5F, 0.5F }) });
	const iggy::runtime::RuntimeGameplayState beforeState = state;

	const iggy::runtime::RuntimeNpcAiMovementRefreshFrameInput input =
		Input(state, subjects, pools, plannedAiMap, map, previousOccupancy, targets, refreshAiMap);
	const iggy::runtime::RuntimeNpcAiMovementRefreshFrameResult helper =
		iggy::runtime::RuntimeNpcAiMovementRefreshFrameStep {}.run(input);
	const iggy::runtime::RuntimeNpcAiMovementPlannedFrameResult planned =
		iggy::runtime::RuntimeNpcAiMovementPlannedFrameStep {}.run(input.planned);
	const iggy::NpcActorMovementRefreshFrame2DResult refresh =
		iggy::NpcActorMovementRefreshFrameProjector2D {}.project({
			planned.movement.movement.report,
			planned.state.npcActors,
			previousOccupancy,
			targets,
			refreshAiMap,
			{},
		});

	Expect(SameActors(helper.state.npcActors, planned.state.npcActors), "refresh helper should match manual planned actor state");
	Expect(helper.dirtyTileCount == refresh.dirtyTileCount, "refresh helper should match manual dirty tile count");
	Expect(helper.occupancyRefreshed == refresh.occupancyRefreshed, "refresh helper should match manual occupancy refresh");
	Expect(SameActors(state.npcActors, beforeState.npcActors), "refresh helper should not mutate input actors");
	Expect(SameControls(state.npcControls, beforeState.npcControls), "refresh helper should not mutate input controls");
}

} // namespace

int main()
{
	TestEmptyDefaultNoOps();
	TestMovedActorProducesRefreshPackets();
	TestBlockedMovementProducesNoRefreshPackets();
	TestFoldedNoPlayUsesExistingMovementReadyControl();
	TestManualParityAndInputImmutability();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
