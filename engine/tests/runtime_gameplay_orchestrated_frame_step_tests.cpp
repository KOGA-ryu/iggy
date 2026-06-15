#include <cstdlib>
#include <string_view>
#include <type_traits>
#include <vector>

#include "runtime/RuntimeGameplayOrchestratedFrameStep.hpp"
#include "runtime/RuntimeNpcAiMovementRefreshFrameStep.hpp"
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

const iggy::ResourceId PlayerId { "player:orchestrated-frame" };

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
	map.id = Id("level:orchestrated-frame");
	return map;
}

iggy::runtime::RuntimeSessionState SessionWithPlayer(
	const iggy::LevelTileMap &map,
	iggy::Vec2 position = { 0.0F, 0.0F })
{
	iggy::runtime::RuntimeSessionState session;
	session.level.map = map;
	session.level.map.playerStart = { 0, 0 };
	session.tickIndex = 41;
	session.hasPlayer = true;
	session.player = PlayerAgent(PlayerId, position, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::East);
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
	Expect(result.built, "orchestrated frame target registry should build");
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
	Expect(result.built, "orchestrated frame effect catalog should build");
	return result.catalog;
}

iggy::InventoryState2D Inventory(std::vector<iggy::InventoryItemStack2D> stacks)
{
	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);
	Expect(result.built, "orchestrated frame inventory should build");
	return result.inventory;
}

iggy::InventoryItemStack2D Stack(const char *itemId, std::uint32_t count)
{
	return { Id(itemId), count };
}

iggy::LevelItemDrop2D Drop(
	const char *id,
	const char *itemId = "item:orchestrated",
	std::uint32_t count = 1,
	iggy::Vec2 position = { 0.0F, 0.0F },
	float pickupRadius = 0.0F,
	bool enabled = true)
{
	return { Id(id), Id(itemId), count, position, pickupRadius, enabled };
}

iggy::runtime::RuntimeInventoryState InventoryState(
	std::vector<iggy::InventoryItemStack2D> stacks,
	std::vector<iggy::LevelItemDrop2D> drops)
{
	return { Inventory(stacks), iggy::LevelItemDrop2DRegistry { drops } };
}

iggy::runtime::RuntimeGameplayState GameplayState(const iggy::LevelTileMap &map)
{
	iggy::runtime::RuntimeGameplayState state;
	state.session = SessionWithPlayer(map);
	state.commandQueue.frames = { { { iggy::runtime::GameplayCommand2DFactory {}.wait(PlayerId) } } };
	state.inventory.inventory = Inventory({});
	return state;
}

iggy::runtime::RuntimeGameplayFrameInput PlayerFrameInput(
	iggy::runtime::RuntimeGameplayState state,
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

iggy::NpcActorState2D Actor(const char *npcId, iggy::Vec2 position)
{
	return {
		Id(npcId),
		Id("profile:orchestrated-frame"),
		Id("faction:orchestrated-frame"),
		position,
		Id("goal:orchestrated-frame"),
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
	Expect(result.built, "orchestrated frame actor registry should build");
	return result.registry;
}

iggy::NpcActorControlState2DRegistry Controls(std::vector<iggy::NpcActorControlState2D> controls)
{
	const iggy::NpcActorControlState2DRegistryBuildResult result =
		iggy::NpcActorControlState2DRegistryBuilder {}.build(controls);
	Expect(result.built, "orchestrated frame control registry should build");
	return result.registry;
}

iggy::NpcActorOccupancy2D Occupancy(const iggy::NpcActorState2DRegistry &actors)
{
	return iggy::NpcActorOccupancyProjector2D {}.project(actors);
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
	Expect(strengthBuild.built, "orchestrated frame strength pool should build");
	Expect(dexterityBuild.built, "orchestrated frame dexterity pool should build");
	Expect(constitutionBuild.built, "orchestrated frame constitution pool should build");
	Expect(intelligenceBuild.built, "orchestrated frame intelligence pool should build");
	Expect(wisdomBuild.built, "orchestrated frame wisdom pool should build");
	Expect(charismaBuild.built, "orchestrated frame charisma pool should build");
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
	Expect(result.built, "orchestrated frame AI map should build");
	return result.map;
}

iggy::runtime::RuntimeGameplayOrchestratedFrameInput Input(
	iggy::runtime::RuntimeGameplayState state,
	const iggy::LevelTileMap &map,
	std::vector<iggy::NpcMapPlayControlFramePlanSubject> subjects = {},
	iggy::NpcMapPlayControlFramePlanPools pools = Pools(),
	std::vector<iggy::PlayerInputIntent2D> playerIntents = {})
{
	iggy::runtime::RuntimeGameplayOrchestratedFrameInput input;
	input.playerFrame = PlayerFrameInput(state, playerIntents);
	input.subjects = subjects;
	input.pools = pools;
	input.aiMap = AiMap();
	input.movementMap = map;
	input.previousOccupancy = Occupancy(state.npcActors);
	input.interactionTargets = Targets({});
	input.refreshAiMap = AiMap();
	return input;
}

const iggy::NpcActorState2D *FindActor(const iggy::NpcActorState2DRegistry &registry, const iggy::ResourceId &npcId)
{
	for (const iggy::NpcActorState2D &actor : registry.actors) {
		if (actor.npcId == npcId)
			return &actor;
	}
	return nullptr;
}

const iggy::NpcActorControlState2D *FindControl(
	const iggy::NpcActorControlState2DRegistry &registry,
	const iggy::ResourceId &npcId)
{
	for (const iggy::NpcActorControlState2D &control : registry.entries) {
		if (control.npcId == npcId)
			return &control;
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

void TestEmptyDefaultFrameNoOps()
{
	const iggy::LevelTileMap map = LevelMap({ "..." });
	const iggy::runtime::RuntimeGameplayState state;

	const iggy::runtime::RuntimeGameplayOrchestratedFrameResult result =
		iggy::runtime::RuntimeGameplayOrchestratedFrameStep {}.run(Input(state, map));

	Expect(result.status == iggy::runtime::RuntimeGameplayOrchestratedFrameStatus::NoChanges, "empty orchestrated frame should report no changes");
	Expect(!result.changedGameplayState() && !result.refreshedNpcData(), "empty orchestrated frame should not change or refresh");
	Expect(result.npcMovedCount == 0 && result.npcRefreshDirtyTileCount == 0, "empty orchestrated frame should have no NPC movement facts");
	Expect(SameActors(result.state.npcActors, state.npcActors), "empty orchestrated frame should preserve NPC actors");
	Expect(SameControls(result.state.npcControls, state.npcControls), "empty orchestrated frame should preserve NPC controls");
}

void TestPlayerPickupPreservedWhenNpcNoOps()
{
	const iggy::LevelTileMap map = LevelMap({ "..." });
	const iggy::InteractionTarget2D target = Target("target:pickup", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:pickup", "item:pickup", 2);
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.interaction = { Targets({ target }), Catalog({ Entry("target:pickup", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }) };
	state.inventory = InventoryState({}, { drop });

	const iggy::runtime::RuntimeGameplayOrchestratedFrameResult result =
		iggy::runtime::RuntimeGameplayOrchestratedFrameStep {}.run(
			Input(state, map, {}, Pools(), { iggy::playerInteractIntent(target.id) }));

	Expect(result.pickedUpCount == 1 && result.inventoryChanged, "orchestrated frame should preserve player pickup facts");
	Expect(SameStacks(result.state.inventory.inventory.stacks, { Stack("item:pickup", 2) }), "orchestrated frame should preserve inventory from player frame");
	Expect(result.npcControlPlannedRequestCount == 0 && result.npcMovedCount == 0, "player-only orchestrated frame should not invent NPC work");
}

void TestNpcAiMovementRunsAfterPlayerFrame()
{
	const iggy::LevelTileMap map = LevelMap({ "...." });
	const iggy::InteractionTarget2D target = Target("target:pickup", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:pickup", "item:pickup", 1);
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.interaction = { Targets({ target }), Catalog({ Entry("target:pickup", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }) };
	state.inventory = InventoryState({}, { drop });
	state.npcActors = Actors({ Actor("npc:mover", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:mover", { 3.5F, 0.5F }) });
	iggy::runtime::RuntimeGameplayOrchestratedFrameInput input =
		Input(
			state,
			map,
			{ Subject("npc:mover", { 3.5F, 0.5F }) },
			Pools({ StrengthEnt("strength:move", "action:move", iggy::NpcBehaviorStateType::Seeking) }),
			{ iggy::playerInteractIntent(target.id) });
	input.interactionTargets = Targets({ Target("target:npc-new-tile", iggy::InteractionTarget2DKind::Usable, { 1.5F, 0.5F }) });
	input.refreshAiMap = AiMap({ AiNode("ai:npc-new-tile", { 1.5F, 0.5F }) });

	const iggy::runtime::RuntimeGameplayOrchestratedFrameResult result =
		iggy::runtime::RuntimeGameplayOrchestratedFrameStep {}.run(input);
	const iggy::NpcActorState2D *actor = FindActor(result.state.npcActors, Id("npc:mover"));
	const iggy::NpcActorControlState2D *control = FindControl(result.state.npcControls, Id("npc:mover"));

	Expect(result.pickedUpCount == 1 && SameStacks(result.state.inventory.inventory.stacks, { Stack("item:pickup", 1) }), "orchestrated frame should preserve player pickup before NPC movement");
	Expect(result.npcControlsChanged && result.npcActorsChanged, "orchestrated frame should update NPC controls and actors through helper");
	Expect(actor != nullptr && NearVec(actor->position, { 1.5F, 0.5F }), "orchestrated frame should move NPC after player frame");
	Expect(control != nullptr && control->moveMode == iggy::NpcMoveMode::Walk, "orchestrated frame should preserve AI-selected control");
	Expect(result.npcOccupancyRefreshed && result.npcInteractionRefreshed && result.npcAiMapRefreshed, "orchestrated frame should expose NPC refresh packets");
	Expect(result.npcRenderRefreshed && result.npcVisibilityRefreshed, "orchestrated frame should expose visual refresh packets");
}

void TestNpcBlockedMovementPreservesPlayerFrameAndControls()
{
	const iggy::LevelTileMap map = LevelMap({ "...." });
	const iggy::InteractionTarget2D target = Target("target:pickup", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:pickup", "item:pickup", 1);
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.interaction = { Targets({ target }), Catalog({ Entry("target:pickup", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }) };
	state.inventory = InventoryState({}, { drop });
	state.npcActors = Actors({
		Actor("npc:mover", { 0.5F, 0.5F }),
		Actor("npc:blocker", { 1.5F, 0.5F }),
	});
	state.npcControls = Controls({
		SeekingControl("npc:mover", { 3.5F, 0.5F }),
		IdleControl("npc:blocker"),
	});

	const iggy::runtime::RuntimeGameplayOrchestratedFrameResult result =
		iggy::runtime::RuntimeGameplayOrchestratedFrameStep {}.run(
			Input(
				state,
				map,
				{ Subject("npc:mover", { 3.5F, 0.5F }), SubjectNoOverride("npc:blocker") },
				Pools({ StrengthEnt("strength:block", "action:block", iggy::NpcBehaviorStateType::Seeking) }),
				{ iggy::playerInteractIntent(target.id) }));
	const iggy::NpcActorState2D *actor = FindActor(result.state.npcActors, Id("npc:mover"));
	const iggy::NpcActorControlState2D *control = FindControl(result.state.npcControls, Id("npc:mover"));

	Expect(result.pickedUpCount == 1 && result.inventoryChanged, "blocked NPC orchestrated frame should still preserve player pickup");
	Expect(result.npcControlsChanged && result.npcBlockedMovementCount == 1, "blocked NPC orchestrated frame should preserve changed controls and block diagnostics");
	Expect(actor != nullptr && NearVec(actor->position, { 0.5F, 0.5F }), "blocked NPC orchestrated frame should not move blocked actor");
	Expect(control != nullptr && control->moveMode == iggy::NpcMoveMode::Walk, "blocked NPC orchestrated frame should keep updated control");
	Expect(!result.refreshedNpcData() && result.npcRefreshDirtyTileCount == 0, "blocked NPC orchestrated frame should not produce movement refresh packets");
}

void TestPreparedNpcRequestsAreNotUsedByOrchestrator()
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
	iggy::runtime::RuntimeGameplayOrchestratedFrameInput input =
		Input(
			state,
			map,
			{ Subject("npc:ai", { 3.5F, 0.5F }) },
			Pools({ StrengthEnt("strength:ai", "action:ai", iggy::NpcBehaviorStateType::Seeking) }));
	input.playerFrame.npcMovementRequests = {
		NpcMovementRequest(NpcMovementFilter("npc:prepared", { 2.5F, 0.5F }, { 3.5F, 0.5F })),
	};
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> originalRequests =
		input.playerFrame.npcMovementRequests;

	const iggy::runtime::RuntimeGameplayOrchestratedFrameResult result =
		iggy::runtime::RuntimeGameplayOrchestratedFrameStep {}.run(input);
	const iggy::NpcActorState2D *aiActor = FindActor(result.state.npcActors, Id("npc:ai"));
	const iggy::NpcActorState2D *preparedActor = FindActor(result.state.npcActors, Id("npc:prepared"));

	Expect(aiActor != nullptr && NearVec(aiActor->position, { 1.5F, 0.5F }), "orchestrated frame should move NPC through AI helper");
	Expect(preparedActor != nullptr && NearVec(preparedActor->position, { 2.5F, 0.5F }), "orchestrated frame should ignore old prepared NPC request hook");
	Expect(input.playerFrame.npcMovementRequests.size() == originalRequests.size(), "orchestrated frame should not mutate prepared request vector");
	Expect(input.playerFrame.npcMovementRequests[0].filter.step.npcId == originalRequests[0].filter.step.npcId, "orchestrated frame should not mutate prepared request payload");
}

void TestManualCompositionParityAndInputImmutability()
{
	const iggy::LevelTileMap map = LevelMap({ "...." });
	iggy::runtime::RuntimeGameplayState state = GameplayState(map);
	state.npcActors = Actors({ Actor("npc:manual", { 0.5F, 0.5F }) });
	state.npcControls = Controls({ SeekingControl("npc:manual", { 3.5F, 0.5F }) });
	const iggy::runtime::RuntimeGameplayState beforeState = state;
	iggy::runtime::RuntimeGameplayOrchestratedFrameInput input =
		Input(
			state,
			map,
			{ Subject("npc:manual", { 3.5F, 0.5F }) },
			Pools({ StrengthEnt("strength:manual", "action:manual", iggy::NpcBehaviorStateType::Seeking) }));
	input.interactionTargets = Targets({ Target("target:manual", iggy::InteractionTarget2DKind::Usable, { 1.5F, 0.5F }) });
	input.refreshAiMap = AiMap({ AiNode("ai:manual", { 1.5F, 0.5F }) });

	const iggy::runtime::RuntimeGameplayOrchestratedFrameResult helper =
		iggy::runtime::RuntimeGameplayOrchestratedFrameStep {}.run(input);
	iggy::runtime::RuntimeGameplayFrameInput manualPlayerInput = input.playerFrame;
	manualPlayerInput.npcMovementRequests.clear();
	const iggy::runtime::RuntimeGameplayFrameResult manualPlayer =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(manualPlayerInput);
	const iggy::runtime::RuntimeNpcAiMovementRefreshFrameResult manualNpc =
		iggy::runtime::RuntimeNpcAiMovementRefreshFrameStep {}.run({
			{
				manualPlayer.state,
				input.subjects,
				input.pools,
				input.aiMap,
				input.controlConfig,
				input.movementMap,
				input.movementConfig,
			},
			input.previousOccupancy,
			input.interactionTargets,
			input.refreshAiMap,
			input.refreshConfig,
		});

	Expect(SameActors(helper.state.npcActors, manualNpc.state.npcActors), "orchestrated frame should match manual NPC actor state");
	Expect(SameControls(helper.state.npcControls, manualNpc.state.npcControls), "orchestrated frame should match manual NPC control state");
	Expect(helper.npcMovedCount == manualNpc.movedCount, "orchestrated frame should match manual movement count");
	Expect(helper.npcRefreshDirtyTileCount == manualNpc.dirtyTileCount, "orchestrated frame should match manual refresh dirty tile count");
	Expect(SameActors(input.playerFrame.state.npcActors, beforeState.npcActors), "orchestrated frame should not mutate input actors");
	Expect(SameControls(input.playerFrame.state.npcControls, beforeState.npcControls), "orchestrated frame should not mutate input controls");
}

} // namespace

int main()
{
	TestEmptyDefaultFrameNoOps();
	TestPlayerPickupPreservedWhenNpcNoOps();
	TestNpcAiMovementRunsAfterPlayerFrame();
	TestNpcBlockedMovementPreservesPlayerFrameAndControls();
	TestPreparedNpcRequestsAreNotUsedByOrchestrator();
	TestManualCompositionParityAndInputImmutability();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
