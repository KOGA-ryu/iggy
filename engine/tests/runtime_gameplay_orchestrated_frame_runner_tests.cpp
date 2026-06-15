#include <cstdlib>
#include <string_view>
#include <type_traits>
#include <vector>

#include "runtime/RuntimeGameplayOrchestratedFrameRunner.hpp"
#include "runtime/RuntimeGameplayOrchestratedFrameStep.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

const iggy::ResourceId PlayerId { "player:orchestrated-runner" };

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
	map.id = Id("level:orchestrated-runner");
	return map;
}

iggy::runtime::RuntimeSessionState SessionWithPlayer(const iggy::LevelTileMap &map)
{
	iggy::runtime::RuntimeSessionState session;
	session.level.map = map;
	session.level.map.playerStart = { 0, 0 };
	session.tickIndex = 51;
	session.hasPlayer = true;
	session.player = PlayerAgent(PlayerId, { 0.0F, 0.0F }, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::East);
	return session;
}

iggy::runtime::RuntimePlayerCommandExecutionConfig PlayerConfig()
{
	iggy::runtime::RuntimePlayerCommandExecutionConfig config;
	config.movement.maxStep = 1.0F;
	return config;
}

iggy::npc_ai::NpcAgentTickConfig NpcConfig()
{
	iggy::npc_ai::NpcAgentTickConfig config;
	config.maxDistance = 0.25F;
	config.awareness = { 8.0F, 0 };
	return config;
}

iggy::InteractionTarget2D Target(
	const char *id,
	iggy::InteractionTarget2DKind kind = iggy::InteractionTarget2DKind::Usable,
	iggy::Vec2 position = { 0.0F, 0.0F },
	float radius = 0.0F,
	bool enabled = true)
{
	return { Id(id), kind, position, radius, enabled };
}

iggy::InteractionTarget2DRegistry Targets(std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result =
		iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built, "orchestrated runner target registry should build");
	return result.registry;
}

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { Id(targetId), effects };
}

iggy::InteractionEffectCatalog2D Catalog(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result =
		iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "orchestrated runner catalog should build");
	return result.catalog;
}

iggy::InventoryState2D Inventory(std::vector<iggy::InventoryItemStack2D> stacks)
{
	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);
	Expect(result.built, "orchestrated runner inventory should build");
	return result.inventory;
}

iggy::InventoryItemStack2D Stack(const char *itemId, std::uint32_t count)
{
	return { Id(itemId), count };
}

iggy::LevelItemDrop2D Drop(
	const char *id,
	const char *itemId,
	std::uint32_t count,
	iggy::Vec2 position = { 0.0F, 0.0F })
{
	return { Id(id), Id(itemId), count, position, 0.0F, true };
}

iggy::runtime::RuntimeInventoryState InventoryState(
	std::vector<iggy::InventoryItemStack2D> stacks,
	std::vector<iggy::LevelItemDrop2D> drops)
{
	return { Inventory(stacks), iggy::LevelItemDrop2DRegistry { drops } };
}

iggy::NpcActorState2D Actor(const char *npcId, iggy::Vec2 position)
{
	return {
		Id(npcId),
		Id("profile:orchestrated-runner"),
		Id("faction:orchestrated-runner"),
		position,
		Id("goal:orchestrated-runner"),
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
	Expect(result.built, "orchestrated runner actors should build");
	return result.registry;
}

iggy::NpcActorControlState2DRegistry Controls(std::vector<iggy::NpcActorControlState2D> controls)
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);
	Expect(result.built, "orchestrated runner controls should build");
	return result.registry;
}

iggy::NpcActorOccupancy2D Occupancy(const iggy::NpcActorState2DRegistry &actors)
{
	return iggy::NpcActorOccupancyProjector2D {}.project(actors);
}

iggy::runtime::RuntimeGameplayState GameplayState(const iggy::LevelTileMap &map)
{
	iggy::runtime::RuntimeGameplayState state;
	state.session = SessionWithPlayer(map);
	state.inventory.inventory = Inventory({});
	return state;
}

iggy::runtime::RuntimeGameplayFrameInput PlayerFrame(
	iggy::runtime::RuntimeGameplayState state = {},
	std::vector<iggy::PlayerInputIntent2D> intents = {})
{
	iggy::runtime::RuntimeGameplayFrameInput input;
	input.state = state;
	input.actorId = PlayerId;
	input.playerIntents = intents;
	input.fallbackPlayerPosition = { 0.0F, 0.0F };
	input.playerCommandConfig = PlayerConfig();
	input.npcConfig = NpcConfig();
	return input;
}

iggy::NpcActorPathStepOccupancyFilter2D NpcMovementFilter(
	const char *npcId,
	iggy::Vec2 oldPosition,
	iggy::Vec2 proposedPosition)
{
	iggy::NpcActorPathStepOccupancyFilter2D filter;
	filter.step.npcId = Id(npcId);
	filter.step.oldPosition = oldPosition;
	filter.step.proposedPosition = proposedPosition;
	filter.step.oldTile = iggy::tileForPoint(oldPosition);
	filter.step.proposedTile = iggy::tileForPoint(proposedPosition);
	filter.step.moveMode = iggy::NpcMoveMode::Walk;
	filter.step.status = iggy::NpcActorPathStep2DStatus::Proposed;
	filter.step.requestsMovement = true;
	filter.status = iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed;
	filter.requestsMovement = true;
	return filter;
}

iggy::NpcActorMovementFrameApply2DRequest NpcMovementRequest(
	const iggy::NpcActorPathStepOccupancyFilter2D &filter)
{
	iggy::NpcActorMovementFrameApply2DRequest request;
	request.filter = filter;
	return request;
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
	const iggy::NpcStrengthPoolBuildResult strengthBuild = iggy::NpcStrengthPoolBuilder {}.build(strength);
	const iggy::NpcDexterityPoolBuildResult dexterityBuild = iggy::NpcDexterityPoolBuilder {}.build({});
	const iggy::NpcConstitutionPoolBuildResult constitutionBuild = iggy::NpcConstitutionPoolBuilder {}.build({});
	const iggy::NpcIntelligencePoolBuildResult intelligenceBuild = iggy::NpcIntelligencePoolBuilder {}.build({});
	const iggy::NpcWisdomPoolBuildResult wisdomBuild = iggy::NpcWisdomPoolBuilder {}.build({});
	const iggy::NpcCharismaPoolBuildResult charismaBuild = iggy::NpcCharismaPoolBuilder {}.build({});
	Expect(strengthBuild.built, "orchestrated runner strength pool should build");
	Expect(dexterityBuild.built, "orchestrated runner dexterity pool should build");
	Expect(constitutionBuild.built, "orchestrated runner constitution pool should build");
	Expect(intelligenceBuild.built, "orchestrated runner intelligence pool should build");
	Expect(wisdomBuild.built, "orchestrated runner wisdom pool should build");
	Expect(charismaBuild.built, "orchestrated runner charisma pool should build");
	pools.strength = strengthBuild.pool;
	pools.dexterity = dexterityBuild.pool;
	pools.constitution = constitutionBuild.pool;
	pools.intelligence = intelligenceBuild.pool;
	pools.wisdom = wisdomBuild.pool;
	pools.charisma = charismaBuild.pool;
	return pools;
}

iggy::AiMapNode2D AiNode(const char *id, iggy::Vec2 position)
{
	return { Id(id), position, 1.0F, 1.0F, 2.0F, 3.0F, 4.0F, {}, {}, true };
}

iggy::AiMap2D AiMap(std::vector<iggy::AiMapNode2D> nodes = {})
{
	const iggy::AiMap2DBuildResult result = iggy::AiMap2DBuilder {}.build(nodes);
	Expect(result.built, "orchestrated runner AI map should build");
	return result.map;
}

iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame Frame(
	const iggy::runtime::RuntimeGameplayState &state,
	const iggy::LevelTileMap &map,
	std::vector<iggy::NpcMapPlayControlFramePlanSubject> subjects = {},
	iggy::NpcMapPlayControlFramePlanPools pools = Pools(),
	std::vector<iggy::PlayerInputIntent2D> playerIntents = {})
{
	iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame frame;
	frame.playerFrame = PlayerFrame(state, playerIntents);
	frame.subjects = subjects;
	frame.pools = pools;
	frame.aiMap = AiMap();
	frame.movementMap = map;
	frame.previousOccupancy = Occupancy(state.npcActors);
	frame.interactionTargets = Targets({});
	frame.refreshAiMap = AiMap();
	return frame;
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

bool SameControls(
	const iggy::NpcActorControlState2DRegistry &actual,
	const iggy::NpcActorControlState2DRegistry &expected)
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

bool SameStacks(const std::vector<iggy::InventoryItemStack2D> &actual, std::vector<iggy::InventoryItemStack2D> expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index].itemId != expected[index].itemId || actual[index].count != expected[index].count)
			return false;
	}
	return true;
}

void TestEmptyRunnerNoOps()
{
	const iggy::LevelTileMap map = LevelMap({ "..." });
	const iggy::runtime::RuntimeGameplayState state = GameplayState(map);

	const iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerResult result =
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunner {}.run({ state, {} });

	Expect(!result.hasFrames() && !result.changed(), "empty orchestrated runner should report no frames or changes");
	Expect(SameActors(result.state.npcActors, state.npcActors), "empty orchestrated runner should preserve actors");
	Expect(SameControls(result.state.npcControls, state.npcControls), "empty orchestrated runner should preserve controls");
}

void TestSingleFrameMatchesDirectStep()
{
	const iggy::LevelTileMap map = LevelMap({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:single", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:single", { 3.5F, 0.5F }) });
	iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame frame =
		Frame(
			state,
			map,
			{ Subject("npc:single", { 3.5F, 0.5F }) },
			Pools({ StrengthEnt("strength:single", "action:single", iggy::NpcBehaviorStateType::Seeking) }));
	frame.interactionTargets = Targets({ Target("target:single", iggy::InteractionTarget2DKind::Usable, { 1.5F, 0.5F }) });
	frame.refreshAiMap = AiMap({ AiNode("ai:single", { 1.5F, 0.5F }) });

	const iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerResult runner =
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunner {}.run({ state, { frame } });
	const iggy::runtime::RuntimeGameplayOrchestratedFrameResult direct =
		iggy::runtime::RuntimeGameplayOrchestratedFrameStep {}.run({
			frame.playerFrame,
			frame.subjects,
			frame.pools,
			frame.aiMap,
			frame.controlConfig,
			frame.movementMap,
			frame.movementConfig,
			frame.previousOccupancy,
			frame.interactionTargets,
			frame.refreshAiMap,
			frame.refreshConfig,
		});

	Expect(runner.frameResults.size() == 1, "single frame runner should store one frame result");
	Expect(SameActors(runner.state.npcActors, direct.state.npcActors), "single frame runner should match direct actor state");
	Expect(SameControls(runner.state.npcControls, direct.state.npcControls), "single frame runner should match direct control state");
	Expect(runner.npcMovedCount == direct.npcMovedCount, "single frame runner should match direct moved count");
}

void TestTwoFrameCarryPlayerInventoryAndNpcState()
{
	const iggy::LevelTileMap map = LevelMap({ "....." });
	const iggy::InteractionTarget2D firstTarget = Target("target:first", iggy::InteractionTarget2DKind::Pickup);
	const iggy::InteractionTarget2D secondTarget = Target("target:second", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D firstDrop = Drop("drop:first", "item:first", 1);
	const iggy::LevelItemDrop2D secondDrop = Drop("drop:second", "item:second", 2);
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.interaction = {
		Targets({ firstTarget, secondTarget }),
		Catalog({
			Entry("target:first", { iggy::pickupItemInteractionEffect(firstTarget.id, firstDrop.id) }),
			Entry("target:second", { iggy::pickupItemInteractionEffect(secondTarget.id, secondDrop.id) }),
		}),
	};
	state.inventory = InventoryState({}, { firstDrop, secondDrop });
	state.npcActors = Actors({ Actor("npc:carry", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:carry", { 4.5F, 0.5F }) });

	iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame first =
		Frame(
			state,
			map,
			{ Subject("npc:carry", { 4.5F, 0.5F }) },
			Pools({ StrengthEnt("strength:first", "action:first", iggy::NpcBehaviorStateType::Seeking) }),
			{ iggy::playerInteractIntent(firstTarget.id) });
	first.interactionTargets = Targets({ Target("target:npc-first", iggy::InteractionTarget2DKind::Usable, { 1.5F, 0.5F }) });
	first.refreshAiMap = AiMap({ AiNode("ai:first", { 1.5F, 0.5F }) });

	iggy::runtime::RuntimeGameplayState expectedAfterFirst = state;
	expectedAfterFirst.npcActors = Actors({ Actor("npc:carry", { 1.5F, 0.5F }) });
	iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame second =
		Frame(
			expectedAfterFirst,
			map,
			{ Subject("npc:carry", { 4.5F, 0.5F }) },
			Pools({ StrengthEnt("strength:second", "action:second", iggy::NpcBehaviorStateType::Seeking) }),
			{ iggy::playerInteractIntent(secondTarget.id) });
	second.interactionTargets = Targets({ Target("target:npc-second", iggy::InteractionTarget2DKind::Usable, { 2.5F, 0.5F }) });
	second.refreshAiMap = AiMap({ AiNode("ai:second", { 2.5F, 0.5F }) });

	const iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerResult result =
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunner {}.run({ state, { first, second } });
	const iggy::NpcActorState2D *actor = FindActor(result.state.npcActors, Id("npc:carry"));

	Expect(result.frameCount == 2 && result.frameResults.size() == 2, "two frame runner should preserve both frame results");
	Expect(result.pickedUpCount == 2 && result.inventoryEventCount == 6, "two frame runner should aggregate pickup inventory events");
	Expect(SameStacks(result.state.inventory.inventory.stacks, { Stack("item:first", 1), Stack("item:second", 2) }), "two frame runner should carry inventory changes");
	Expect(actor != nullptr && NearVec(actor->position, { 2.5F, 0.5F }), "two frame runner should carry NPC actor movement into later planning");
	Expect(result.npcMovedCount == 2 && result.npcRefreshDirtyTileCount == 4, "two frame runner should aggregate NPC movement refresh facts");
	Expect(result.npcOccupancyRefreshed && result.npcInteractionRefreshed && result.npcAiMapRefreshed, "two frame runner should OR refresh flags");
}

void TestMapAwareControlCanChangeTargetBetweenFrames()
{
	const iggy::LevelTileMap map = LevelMap({ "....." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:map", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:map", { 4.5F, 0.5F }) });

	iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame first =
		Frame(
			state,
			map,
			{ Subject("npc:map", { 4.5F, 0.5F }) },
			Pools({ StrengthEnt("strength:map-a", "action:map-a", iggy::NpcBehaviorStateType::Seeking) }));
	iggy::runtime::RuntimeGameplayState expectedAfterFirst = state;
	expectedAfterFirst.npcActors = Actors({ Actor("npc:map", { 1.5F, 0.5F }) });
	iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame second =
		Frame(
			expectedAfterFirst,
			map,
			{ Subject("npc:map", { 0.5F, 0.5F }) },
			Pools({ StrengthEnt("strength:map-b", "action:map-b", iggy::NpcBehaviorStateType::Seeking) }));

	const iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerResult result =
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunner {}.run({ state, { first, second } });
	const iggy::NpcActorState2D *actor = FindActor(result.state.npcActors, Id("npc:map"));

	Expect(actor != nullptr && NearVec(actor->position, { 0.5F, 0.5F }), "map-aware runner should allow later frame to choose a new target");
	Expect(result.npcControlAppliedCount == 2 && result.npcMovedCount == 2, "map-aware runner should aggregate changed controls and movement");
}

void TestBlockedFrameCanMoveWhenConditionsChangeLater()
{
	const iggy::LevelTileMap map = LevelMap({ "....." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({
		Actor("npc:mover", { 2.5F, 0.5F }),
		Actor("npc:blocker", { 3.5F, 0.5F }),
	});
	state.npcControls = Controls({
		SeekingControl("npc:mover", { 4.5F, 0.5F }),
		IdleControl("npc:blocker"),
	});
	iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame blocked =
		Frame(
			state,
			map,
			{ Subject("npc:mover", { 4.5F, 0.5F }), SubjectNoOverride("npc:blocker") },
			Pools({ StrengthEnt("strength:block", "action:block", iggy::NpcBehaviorStateType::Seeking) }));

	iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame cleared =
		Frame(
			state,
			map,
			{ Subject("npc:mover", { 0.5F, 0.5F }), SubjectNoOverride("npc:blocker") },
			Pools({ StrengthEnt("strength:clear", "action:clear", iggy::NpcBehaviorStateType::Seeking) }));
	cleared.interactionTargets = Targets({ Target("target:clear", iggy::InteractionTarget2DKind::Usable, { 1.5F, 0.5F }) });
	cleared.refreshAiMap = AiMap({ AiNode("ai:clear", { 1.5F, 0.5F }) });

	const iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerResult result =
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunner {}.run({ state, { blocked, cleared } });
	const iggy::NpcActorState2D *actor = FindActor(result.state.npcActors, Id("npc:mover"));

	Expect(result.npcBlockedMovementCount == 1 && result.npcMovedCount == 1, "blocked then retarget runner should aggregate blocked and moved facts");
	Expect(actor != nullptr && NearVec(actor->position, { 1.5F, 0.5F }), "blocked then retarget runner should move when selected target changes");
	Expect(result.npcRefreshDirtyTileCount == 2, "blocked then retarget runner should refresh only moved frame dirty tiles");
}

void TestStalePreparedNpcRequestsIgnored()
{
	const iggy::LevelTileMap map = LevelMap({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({
		Actor("npc:ai", { 0.5F, 0.5F }),
		Actor("npc:prepared", { 2.5F, 0.5F }),
	});
	state.npcControls = Controls({
		SeekingControl("npc:ai", { 3.5F, 0.5F }),
		IdleControl("npc:prepared"),
	});
	iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame frame =
		Frame(
			state,
			map,
			{ Subject("npc:ai", { 3.5F, 0.5F }) },
			Pools({ StrengthEnt("strength:ai", "action:ai", iggy::NpcBehaviorStateType::Seeking) }));
	frame.playerFrame.npcMovementRequests = {
		NpcMovementRequest(NpcMovementFilter("npc:prepared", { 2.5F, 0.5F }, { 3.5F, 0.5F })),
	};
	const std::vector<iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame> frames { frame };

	const iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerResult result =
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunner {}.run({ state, frames });
	const iggy::NpcActorState2D *aiActor = FindActor(result.state.npcActors, Id("npc:ai"));
	const iggy::NpcActorState2D *preparedActor = FindActor(result.state.npcActors, Id("npc:prepared"));

	Expect(aiActor != nullptr && NearVec(aiActor->position, { 1.5F, 0.5F }), "orchestrated runner should move AI-driven actor");
	Expect(preparedActor != nullptr && NearVec(preparedActor->position, { 2.5F, 0.5F }), "orchestrated runner should ignore stale prepared request");
	Expect(frames[0].playerFrame.npcMovementRequests.size() == 1, "orchestrated runner should not mutate input frame request vector");
}

void TestManualLoopParityAndInputImmutability()
{
	const iggy::LevelTileMap map = LevelMap({ "....." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:manual", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:manual", { 4.5F, 0.5F }) });
	const iggy::runtime::RuntimeGameplayState beforeState = state;

	iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame first =
		Frame(
			state,
			map,
			{ Subject("npc:manual", { 4.5F, 0.5F }) },
			Pools({ StrengthEnt("strength:manual-a", "action:manual-a", iggy::NpcBehaviorStateType::Seeking) }));
	first.interactionTargets = Targets({ Target("target:manual-a", iggy::InteractionTarget2DKind::Usable, { 1.5F, 0.5F }) });
	first.refreshAiMap = AiMap({ AiNode("ai:manual-a", { 1.5F, 0.5F }) });
	iggy::runtime::RuntimeGameplayState expectedAfterFirst = state;
	expectedAfterFirst.npcActors = Actors({ Actor("npc:manual", { 1.5F, 0.5F }) });
	iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame second =
		Frame(
			expectedAfterFirst,
			map,
			{ Subject("npc:manual", { 4.5F, 0.5F }) },
			Pools({ StrengthEnt("strength:manual-b", "action:manual-b", iggy::NpcBehaviorStateType::Seeking) }));
	second.interactionTargets = Targets({ Target("target:manual-b", iggy::InteractionTarget2DKind::Usable, { 2.5F, 0.5F }) });
	second.refreshAiMap = AiMap({ AiNode("ai:manual-b", { 2.5F, 0.5F }) });
	std::vector<iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame> frames { first, second };
	const std::vector<iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame> beforeFrames = frames;

	const iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerResult runner =
		iggy::runtime::RuntimeGameplayOrchestratedFrameRunner {}.run({ state, frames });
	iggy::runtime::RuntimeGameplayState carried = state;
	std::size_t movedCount = 0;
	std::size_t dirtyTileCount = 0;
	for (const iggy::runtime::RuntimeGameplayOrchestratedFrameRunnerFrame &frame : frames) {
		iggy::runtime::RuntimeGameplayOrchestratedFrameInput input {
			frame.playerFrame,
			frame.subjects,
			frame.pools,
			frame.aiMap,
			frame.controlConfig,
			frame.movementMap,
			frame.movementConfig,
			frame.previousOccupancy,
			frame.interactionTargets,
			frame.refreshAiMap,
			frame.refreshConfig,
		};
		input.playerFrame.state = carried;
		const iggy::runtime::RuntimeGameplayOrchestratedFrameResult frameResult =
			iggy::runtime::RuntimeGameplayOrchestratedFrameStep {}.run(input);
		carried = frameResult.state;
		movedCount += frameResult.npcMovedCount;
		dirtyTileCount += frameResult.npcRefreshDirtyTileCount;
	}

	Expect(SameActors(runner.state.npcActors, carried.npcActors), "orchestrated runner should match manual loop actor state");
	Expect(SameControls(runner.state.npcControls, carried.npcControls), "orchestrated runner should match manual loop control state");
	Expect(runner.npcMovedCount == movedCount && runner.npcRefreshDirtyTileCount == dirtyTileCount, "orchestrated runner should match manual loop counts");
	Expect(SameActors(state.npcActors, beforeState.npcActors), "orchestrated runner should not mutate input initial actors");
	Expect(SameControls(state.npcControls, beforeState.npcControls), "orchestrated runner should not mutate input initial controls");
	Expect(frames[0].subjects.size() == beforeFrames[0].subjects.size(), "orchestrated runner should not mutate frame subjects");
}

} // namespace

int main()
{
	TestEmptyRunnerNoOps();
	TestSingleFrameMatchesDirectStep();
	TestTwoFrameCarryPlayerInventoryAndNpcState();
	TestMapAwareControlCanChangeTargetBetweenFrames();
	TestBlockedFrameCanMoveWhenConditionsChangeLater();
	TestStalePreparedNpcRequestsIgnored();
	TestManualLoopParityAndInputImmutability();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
