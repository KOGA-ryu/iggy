#include <cstdlib>
#include <string_view>
#include <type_traits>
#include <vector>

#include "runtime/RuntimeNpcAiMovementPlannedFrameStep.hpp"
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
	map.id = Id("level:runtime-ai-move-planned-frame");
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
	Expect(result.built, "runtime ai movement ai map should build");
	return result.map;
}

iggy::NpcActorState2D Actor(const char *npcId, iggy::Vec2 position, bool present = true)
{
	return {
		Id(npcId),
		Id("profile:runtime-ai-move"),
		Id("faction:runtime-ai-move"),
		position,
		Id("goal:runtime-ai-move"),
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
	Expect(result.built, "runtime ai movement actor registry should build");
	return result.registry;
}

iggy::NpcActorControlState2DRegistry Controls(std::vector<iggy::NpcActorControlState2D> controls)
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);
	Expect(result.built, "runtime ai movement control registry should build");
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
	Expect(strengthBuild.built, "runtime ai movement strength pool should build");
	Expect(dexterityBuild.built, "runtime ai movement dexterity pool should build");
	Expect(constitutionBuild.built, "runtime ai movement constitution pool should build");
	Expect(intelligenceBuild.built, "runtime ai movement intelligence pool should build");
	Expect(wisdomBuild.built, "runtime ai movement wisdom pool should build");
	Expect(charismaBuild.built, "runtime ai movement charisma pool should build");
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
	state.session.tickIndex = 17;
	state.session.level.map = map;
	state.commandQueue.frames = { { { iggy::runtime::GameplayCommand2DFactory {}.wait(Id("player:queued")) } } };
	state.inventory.inventory.stacks = { { Id("item:runtime-ai-move"), 2 } };
	return state;
}

iggy::runtime::RuntimeNpcAiMovementPlannedFrameInput Input(
	const iggy::runtime::RuntimeGameplayState &state,
	std::vector<iggy::NpcMapPlayControlFramePlanSubject> subjects,
	const iggy::NpcMapPlayControlFramePlanPools &pools,
	const iggy::AiMap2D &aiMap,
	const iggy::LevelTileMap &movementMap,
	const iggy::NpcMapPlayControlFramePlanConfig &controlConfig = {},
	const iggy::NpcActorMovementFramePlan2DConfig &movementConfig = {})
{
	return {
		state,
		subjects,
		pools,
		aiMap,
		controlConfig,
		movementMap,
		movementConfig,
	};
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
	Expect(actual.session.tickIndex == expected.session.tickIndex, "ai movement should preserve session tick");
	Expect(actual.commandQueue.frames.size() == expected.commandQueue.frames.size(), "ai movement should preserve command queue");
	Expect(actual.interaction.targets.targets().size() == expected.interaction.targets.targets().size(), "ai movement should preserve interaction state");
	Expect(actual.inventory.inventory.stacks.size() == expected.inventory.inventory.stacks.size(), "ai movement should preserve inventory state");
}

void TestEmptyDefaultStateNoOps()
{
	const iggy::LevelTileMap movementMap = LevelMap({ "..." });
	const iggy::runtime::RuntimeGameplayState state = GameplayState(movementMap);

	const iggy::runtime::RuntimeNpcAiMovementPlannedFrameResult result =
		iggy::runtime::RuntimeNpcAiMovementPlannedFrameStep {}.run(
			Input(state, {}, Pools(), AiMap(), movementMap));

	Expect(result.status == iggy::runtime::RuntimeNpcAiMovementPlannedFrameStatus::NoChanges, "empty ai movement frame should report no changes");
	Expect(!result.ranAnyRequests(), "empty ai movement frame should not report request work");
	Expect(!result.changedState(), "empty ai movement frame should not change state");
	Expect(result.control.plannedRequestCount == 0 && result.movement.plannedRequestCount == 0, "empty ai movement frame should preserve nested no-op results");
	Expect(SameActors(result.state.npcActors, state.npcActors), "empty ai movement frame should preserve actors");
	Expect(SameControls(result.state.npcControls, state.npcControls), "empty ai movement frame should preserve controls");
}

void TestControlUpdateFeedsMovementInSameFrame()
{
	const iggy::LevelTileMap movementMap = LevelMap({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(movementMap);
	state.npcActors = Actors({ Actor("npc:think-move", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:think-move", { 3.5F, 0.5F }, iggy::NpcMoveMode::Still) });
	iggy::NpcMapPlayControlFramePlanConfig controlConfig;
	controlConfig.step.mapRead.matchedMapTagBonus = 4.0F;

	const iggy::runtime::RuntimeNpcAiMovementPlannedFrameResult result =
		iggy::runtime::RuntimeNpcAiMovementPlannedFrameStep {}.run(
			Input(
				state,
				{ Subject("npc:think-move", { 3.5F, 0.5F }) },
				Pools(
					{ StrengthEnt("strength:map-seek", "action:map-seek", iggy::NpcBehaviorStateType::Seeking, 2.0F, { Id("zone:cover") }) },
					{ DexterityEnt("dexterity:raw-seek", "action:raw-seek", iggy::NpcBehaviorStateType::Seeking, 5.0F, { Id("zone:loud") }) }),
				AiMap({ AiNode("ai:cover", { 0.5F, 0.5F }, { Id("zone:cover") }) }),
				movementMap,
				controlConfig));
	const iggy::NpcActorState2D *actor = FindActor(result.state.npcActors, Id("npc:think-move"));

	Expect(result.status == iggy::runtime::RuntimeNpcAiMovementPlannedFrameStatus::Ran, "ai movement frame should run requests");
	Expect(result.controlPlannedRequestCount == 1 && result.controlAppliedCount == 1, "ai movement should apply one control request");
	Expect(result.controlMapChangedSelectionCount == 1, "ai movement should preserve map-aware selection count");
	Expect(result.movementPlannedRequestCount == 1 && result.movedCount == 1, "ai movement should plan and apply movement after control update");
	Expect(result.changedControls && result.changedActors, "ai movement should report changed controls and actors");
	Expect(result.control.step.entries[0].mapPlay.rawSelectedActionTag == Id("action:raw-seek"), "ai movement should preserve raw selected action");
	Expect(result.control.step.entries[0].mapPlay.mapSelectedActionTag == Id("action:map-seek"), "ai movement should preserve map selected action");
	Expect(result.state.npcControls.entries[0].moveMode == iggy::NpcMoveMode::Walk, "ai movement should keep updated movement-ready control");
	Expect(actor != nullptr && NearVec(actor->position, { 1.5F, 0.5F }), "ai movement should move actor using post-control state");
	ExpectNonNpcStatePreserved(result.state, state);
}

void TestNoControlChangeMovementUsesExistingControl()
{
	const iggy::LevelTileMap movementMap = LevelMap({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(movementMap);
	state.npcActors = Actors({ Actor("npc:existing-move", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:existing-move", { 3.5F, 0.5F }, iggy::NpcMoveMode::Walk) });

	const iggy::runtime::RuntimeNpcAiMovementPlannedFrameResult result =
		iggy::runtime::RuntimeNpcAiMovementPlannedFrameStep {}.run(
			Input(state, { SubjectNoOverride("npc:existing-move") }, Pools(), AiMap(), movementMap));
	const iggy::NpcActorState2D *actor = FindActor(result.state.npcActors, Id("npc:existing-move"));

	Expect(result.controlPlannedRequestCount == 1 && result.controlFailedCount == 1, "folded control request should preserve failed control diagnostics");
	Expect(!result.changedControls, "folded control request should not change controls");
	Expect(result.movementPlannedRequestCount == 1 && result.movedCount == 1, "movement should still use existing controls after folded AI control");
	Expect(actor != nullptr && NearVec(actor->position, { 1.5F, 0.5F }), "movement should follow existing movement-ready control");
}

void TestBlockedMovementPreservesUpdatedControls()
{
	const iggy::LevelTileMap movementMap = LevelMap({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(movementMap);
	state.npcActors = Actors({
		Actor("npc:blocked-mover", { 0.5F, 0.5F }),
		Actor("npc:blocker", { 1.5F, 0.5F }),
	});
	state.npcControls = Controls({
		SeekingControl("npc:blocked-mover", { 3.5F, 0.5F }, iggy::NpcMoveMode::Still),
		IdleControl("npc:blocker"),
	});

	const iggy::runtime::RuntimeNpcAiMovementPlannedFrameResult result =
		iggy::runtime::RuntimeNpcAiMovementPlannedFrameStep {}.run(
			Input(
				state,
				{ Subject("npc:blocked-mover", { 3.5F, 0.5F }), SubjectNoOverride("npc:blocker") },
				Pools({ StrengthEnt("strength:block", "action:block", iggy::NpcBehaviorStateType::Seeking, 1.0F) }),
				AiMap(),
				movementMap));
	const iggy::NpcActorState2D *mover = FindActor(result.state.npcActors, Id("npc:blocked-mover"));

	Expect(result.changedControls, "blocked movement should still preserve updated controls");
	Expect(result.blockedCount == 1 && result.movedCount == 0, "blocked movement should preserve blocked diagnostics");
	Expect(result.movement.movement.apply.entries[0].executor.postMove.blockingNpcId == Id("npc:blocker"), "blocked movement should preserve blocker id");
	Expect(result.state.npcControls.entries[0].moveMode == iggy::NpcMoveMode::Walk, "blocked movement should keep updated control");
	Expect(mover != nullptr && NearVec(mover->position, { 0.5F, 0.5F }), "blocked movement should not move blocked actor");
}

void TestMovementCapacityPolicyAllowsStackAfterAiControl()
{
	const iggy::LevelTileMap movementMap = LevelMap({ "..." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(movementMap);
	state.npcActors = Actors({
		Actor("npc:first", { 0.5F, 0.5F }),
		Actor("npc:second", { 2.5F, 0.5F }),
	});
	state.npcControls = Controls({
		SeekingControl("npc:first", { 1.5F, 0.5F }, iggy::NpcMoveMode::Still),
		SeekingControl("npc:second", { 1.5F, 0.5F }, iggy::NpcMoveMode::Still),
	});
	iggy::NpcActorMovementFramePlan2DConfig movementConfig;
	movementConfig.runReservation = true;
	movementConfig.reservation.policy.maxOccupantsPerTile = 2;

	const iggy::runtime::RuntimeNpcAiMovementPlannedFrameResult result =
		iggy::runtime::RuntimeNpcAiMovementPlannedFrameStep {}.run(
			Input(
				state,
				{ Subject("npc:first", { 1.5F, 0.5F }), Subject("npc:second", { 1.5F, 0.5F }) },
				Pools({ StrengthEnt("strength:stack", "action:stack", iggy::NpcBehaviorStateType::Seeking, 1.0F) }),
				AiMap(),
				movementMap,
				{},
				movementConfig));

	Expect(result.controlAppliedCount == 2, "capacity policy should update both controls before movement");
	Expect(result.movementReservationAcceptedCount == 2 && result.movementReservationRejectedCount == 0, "capacity policy should accept both reservations");
	Expect(result.movedCount == 2, "capacity policy should move both actors");
	Expect(OccupantCountAt(result.state.npcActors, { 1, 0 }) == 2, "capacity policy should leave stacked occupancy inspectable");
}

void TestInputsAreNotMutated()
{
	const iggy::LevelTileMap movementMap = LevelMap({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(movementMap);
	state.npcActors = Actors({ Actor("npc:immutable", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:immutable", { 3.5F, 0.5F }, iggy::NpcMoveMode::Still) });
	std::vector<iggy::NpcMapPlayControlFramePlanSubject> subjects {
		Subject("npc:immutable", { 3.5F, 0.5F }),
	};
	iggy::NpcMapPlayControlFramePlanPools pools =
		Pools({ StrengthEnt("strength:immutable", "action:immutable", iggy::NpcBehaviorStateType::Seeking, 1.0F) });
	const iggy::AiMap2D aiMap = AiMap({ AiNode("ai:immutable", { 0.5F, 0.5F }, { Id("zone:immutable") }) });
	iggy::NpcMapPlayControlFramePlanConfig controlConfig;
	iggy::NpcActorMovementFramePlan2DConfig movementConfig;
	movementConfig.runReservation = true;
	movementConfig.reservation.policy.maxOccupantsPerTile = 2;

	const iggy::runtime::RuntimeGameplayState beforeState = state;
	const std::vector<iggy::NpcMapPlayControlFramePlanSubject> beforeSubjects = subjects;
	const iggy::NpcMapPlayControlFramePlanPools beforePools = pools;
	const iggy::AiMap2D beforeAiMap = aiMap;
	const iggy::LevelTileMap beforeMovementMap = movementMap;
	const iggy::NpcMapPlayControlFramePlanConfig beforeControlConfig = controlConfig;
	const iggy::NpcActorMovementFramePlan2DConfig beforeMovementConfig = movementConfig;

	(void)iggy::runtime::RuntimeNpcAiMovementPlannedFrameStep {}.run(
		Input(state, subjects, pools, aiMap, movementMap, controlConfig, movementConfig));

	Expect(SameActors(state.npcActors, beforeState.npcActors), "ai movement should not mutate input actors");
	Expect(SameControls(state.npcControls, beforeState.npcControls), "ai movement should not mutate input controls");
	Expect(subjects.size() == beforeSubjects.size() && subjects[0].npcId == beforeSubjects[0].npcId, "ai movement should not mutate subjects");
	Expect(pools.strength.entries.size() == beforePools.strength.entries.size(), "ai movement should not mutate pools");
	Expect(aiMap.nodes.size() == beforeAiMap.nodes.size(), "ai movement should not mutate AI map");
	Expect(movementMap.tiles.size() == beforeMovementMap.tiles.size(), "ai movement should not mutate movement map");
	Expect(controlConfig.includeAbsentActors == beforeControlConfig.includeAbsentActors, "ai movement should not mutate control config");
	Expect(movementConfig.runReservation == beforeMovementConfig.runReservation, "ai movement should not mutate movement config");
}

void TestManualCompositionMatchesHelper()
{
	const iggy::LevelTileMap movementMap = LevelMap({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(movementMap);
	state.npcActors = Actors({ Actor("npc:manual", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:manual", { 3.5F, 0.5F }, iggy::NpcMoveMode::Still) });
	const std::vector<iggy::NpcMapPlayControlFramePlanSubject> subjects {
		Subject("npc:manual", { 3.5F, 0.5F }),
	};
	const iggy::NpcMapPlayControlFramePlanPools pools =
		Pools({ StrengthEnt("strength:manual", "action:manual", iggy::NpcBehaviorStateType::Seeking, 1.0F) });
	const iggy::AiMap2D aiMap = AiMap();
	const iggy::NpcMapPlayControlFramePlanConfig controlConfig;
	const iggy::NpcActorMovementFramePlan2DConfig movementConfig;

	const iggy::runtime::RuntimeNpcAiControlPlannedFrameResult control =
		iggy::runtime::RuntimeNpcAiControlPlannedFrameStep {}.run({
			state,
			subjects,
			pools,
			aiMap,
			controlConfig,
		});
	const iggy::runtime::RuntimeNpcActorMovementPlannedFrameResult movement =
		iggy::runtime::RuntimeNpcActorMovementPlannedFrameStep {}.run({
			control.state,
			movementMap,
			movementConfig,
		});
	const iggy::runtime::RuntimeNpcAiMovementPlannedFrameResult helper =
		iggy::runtime::RuntimeNpcAiMovementPlannedFrameStep {}.run(
			Input(state, subjects, pools, aiMap, movementMap, controlConfig, movementConfig));

	Expect(SameActors(helper.state.npcActors, movement.state.npcActors), "ai movement helper should match manual actor state");
	Expect(SameControls(helper.state.npcControls, movement.state.npcControls), "ai movement helper should match manual control state");
	Expect(helper.controlPlannedRequestCount == control.plannedRequestCount, "ai movement helper should match manual control planned count");
	Expect(helper.movementPlannedRequestCount == movement.plannedRequestCount, "ai movement helper should match manual movement planned count");
	Expect(helper.changedControls == control.changedControls && helper.changedActors == movement.changed, "ai movement helper should match manual change flags");
}

} // namespace

int main()
{
	TestEmptyDefaultStateNoOps();
	TestControlUpdateFeedsMovementInSameFrame();
	TestNoControlChangeMovementUsesExistingControl();
	TestBlockedMovementPreservesUpdatedControls();
	TestMovementCapacityPolicyAllowsStackAfterAiControl();
	TestInputsAreNotMutated();
	TestManualCompositionMatchesHelper();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
