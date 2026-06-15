#include <cstdlib>
#include <type_traits>
#include <vector>

#include "runtime/RuntimeGameplayFrameReporter.hpp"
#include "runtime/RuntimeGameplayFrameRunner.hpp"
#include "runtime/RuntimeGameplayFrameStep.hpp"
#include "runtime/RuntimePolicyGameplayFrameReporter.hpp"
#include "runtime/RuntimePolicyGameplayFrameRunner.hpp"
#include "runtime/RuntimePolicyGameplayFrameStep.hpp"
#include "scene/level/TileCoord.hpp"
#include "support/CommandFrameFixtures.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::CommandFrame;
using iggy::test::Expect;
using iggy::test::ExpectPlayerAgent;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

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

const iggy::ResourceId RawPlayerId { "player:prepared-npc-raw" };
const iggy::ResourceId PolicyPlayerId { "player:prepared-npc-policy" };

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
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

iggy::runtime::RuntimeSessionState SessionWithPlayer(
	const iggy::ResourceId &playerId,
	iggy::Vec2 position = { 0.0F, 0.0F })
{
	iggy::runtime::RuntimeSessionState session;
	session.level.map = MapFromRows({ "...", "..." });
	session.level.map.id = Id("level:prepared-npc-acceptance");
	session.level.map.playerStart = { 0, 0 };
	session.tickIndex = 61;
	session.hasPlayer = true;
	session.player = PlayerAgent(playerId, position, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::East);
	return session;
}

iggy::NpcActorState2D NpcActor(const char *npcId, iggy::Vec2 position)
{
	return {
		Id(npcId),
		Id("profile:prepared-npc"),
		Id("faction:prepared-npc"),
		position,
		{},
		true,
	};
}

iggy::NpcActorControlState2D NpcControl(const char *npcId)
{
	return {
		Id(npcId),
		iggy::waitNpcObjective(),
		iggy::idleNpcBehaviorState(),
		iggy::NpcMoveMode::Still,
	};
}

iggy::NpcActorPathStepOccupancyFilter2D NpcMovementFilter(
	const char *npcId,
	iggy::Vec2 oldPosition,
	iggy::Vec2 proposedPosition,
	iggy::NpcActorPathStepOccupancyFilter2DStatus status = iggy::NpcActorPathStepOccupancyFilter2DStatus::Allowed,
	bool requestsMovement = true,
	const char *blockingNpcId = "")
{
	iggy::NpcActorPathStepOccupancyFilter2D filter;
	filter.step.npcId = Id(npcId);
	filter.step.oldPosition = oldPosition;
	filter.step.proposedPosition = proposedPosition;
	filter.step.oldTile = iggy::tileForPoint(oldPosition);
	filter.step.proposedTile = iggy::tileForPoint(proposedPosition);
	filter.step.moveMode = iggy::NpcMoveMode::Walk;
	filter.step.status = status == iggy::NpcActorPathStepOccupancyFilter2DStatus::NoStepProposal
		? iggy::NpcActorPathStep2DStatus::NoPath
		: iggy::NpcActorPathStep2DStatus::Proposed;
	filter.step.requestsMovement = status != iggy::NpcActorPathStepOccupancyFilter2DStatus::NoStepProposal;
	filter.status = status;
	filter.requestsMovement = requestsMovement;
	if (blockingNpcId[0] != '\0')
		filter.blockingNpcId = Id(blockingNpcId);
	return filter;
}

iggy::NpcActorMovementFrameApply2DRequest NpcMovementRequest(
	const iggy::NpcActorPathStepOccupancyFilter2D &filter)
{
	iggy::NpcActorMovementFrameApply2DRequest request;
	request.filter = filter;
	return request;
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

iggy::InteractionTarget2DRegistry TargetRegistry(std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result =
		iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built, "prepared NPC movement target registry fixture should build");
	return result.registry;
}

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { Id(targetId), effects };
}

iggy::InteractionEffectCatalog2D EffectCatalog(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result =
		iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "prepared NPC movement effect catalog fixture should build");
	return result.catalog;
}

iggy::InventoryItemStack2D Stack(const char *itemId, std::uint32_t count)
{
	return { Id(itemId), count };
}

iggy::InventoryState2D Inventory(std::vector<iggy::InventoryItemStack2D> stacks)
{
	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);
	Expect(result.built, "prepared NPC movement inventory fixture should build");
	return result.inventory;
}

iggy::LevelItemDrop2D Drop(const char *id, const char *itemId, std::uint32_t count = 1)
{
	return { Id(id), Id(itemId), count, { 0.0F, 0.0F }, 0.0F, true };
}

iggy::runtime::RuntimeInventoryState InventoryState(
	std::vector<iggy::InventoryItemStack2D> stacks,
	std::vector<iggy::LevelItemDrop2D> drops)
{
	return { Inventory(stacks), iggy::LevelItemDrop2DRegistry { drops } };
}

iggy::ItemDefinition2D Definition(const char *itemId, const char *displayName = "Item", std::uint32_t maxStackCount = 10)
{
	return { Id(itemId), displayName, maxStackCount, iggy::ItemDefinition2DKind::Material };
}

iggy::ItemDefinition2DCatalog ItemCatalog(std::vector<iggy::ItemDefinition2D> definitions)
{
	const iggy::ItemDefinition2DCatalogBuildResult result =
		iggy::ItemDefinition2DCatalogBuilder {}.build(definitions);
	Expect(result.built, "prepared NPC movement item catalog fixture should build");
	return result.catalog;
}

iggy::runtime::RuntimeGameplayState GameplayState(
	iggy::runtime::RuntimeSessionState session,
	iggy::runtime::RuntimeInteractionState interaction = {},
	iggy::runtime::RuntimeInventoryState inventory = {},
	iggy::runtime::RuntimeCommandQueueState queue = {})
{
	return { session, queue, interaction, inventory };
}

bool SameStacks(
	const std::vector<iggy::InventoryItemStack2D> &actual,
	const std::vector<iggy::InventoryItemStack2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index].itemId != expected[index].itemId || actual[index].count != expected[index].count)
			return false;
	}
	return true;
}

bool HasRawEvent(
	const std::vector<iggy::runtime::RuntimeGameplayFrameEvent> &events,
	iggy::runtime::RuntimeGameplayFrameEvent expected)
{
	for (const iggy::runtime::RuntimeGameplayFrameEvent event : events) {
		if (event == expected)
			return true;
	}
	return false;
}

bool HasPolicyEvent(
	const std::vector<iggy::runtime::RuntimePolicyGameplayFrameEvent> &events,
	iggy::runtime::RuntimePolicyGameplayFrameEvent expected)
{
	for (const iggy::runtime::RuntimePolicyGameplayFrameEvent event : events) {
		if (event == expected)
			return true;
	}
	return false;
}

bool SameNpcMovementFilter(
	const iggy::NpcActorPathStepOccupancyFilter2D &actual,
	const iggy::NpcActorPathStepOccupancyFilter2D &expected)
{
	return actual.step.npcId == expected.step.npcId
		&& NearVec(actual.step.oldPosition, expected.step.oldPosition)
		&& NearVec(actual.step.proposedPosition, expected.step.proposedPosition)
		&& actual.step.oldTile == expected.step.oldTile
		&& actual.step.proposedTile == expected.step.proposedTile
		&& actual.status == expected.status
		&& actual.requestsMovement == expected.requestsMovement
		&& actual.blockingNpcId == expected.blockingNpcId;
}

void TestRawFramePreparedNpcMovementWithExistingInteraction()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:raw-lever"),
		Target("target:raw-door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F, true),
	};
	const iggy::runtime::RuntimeInteractionState interaction {
		TargetRegistry(targets),
		EffectCatalog({ Entry("target:raw-lever", { iggy::toggleTargetInteractionEffect(Id("target:raw-door"), false) }) }),
	};
	iggy::runtime::RuntimeGameplayState state = GameplayState(SessionWithPlayer(RawPlayerId), interaction);
	state.npcActors = { { NpcActor("npc:raw-frame", { 4.5F, 0.5F }) } };
	state.npcControls = { { NpcControl("npc:raw-frame") } };
	const iggy::runtime::RuntimeGameplayState originalState = state;

	iggy::runtime::RuntimeGameplayFrameInput input;
	input.state = state;
	input.actorId = RawPlayerId;
	input.playerIntents = { iggy::playerInteractIntent(Id("target:raw-lever")) };
	input.fallbackPlayerPosition = { 1.5F, 1.5F };
	input.playerCommandConfig = PlayerConfig();
	input.npcConfig = NpcConfig();
	input.npcMovementRequests = {
		NpcMovementRequest(NpcMovementFilter("npc:raw-frame", { 4.5F, 0.5F }, { 5.5F, 0.5F })),
	};
	const std::vector<iggy::NpcActorMovementFrameApply2DRequest> originalRequests = input.npcMovementRequests;

	const iggy::runtime::RuntimeGameplayFrameResult result =
		iggy::runtime::RuntimeGameplayFrameStep {}.run(input);
	const iggy::runtime::RuntimeGameplayFrameReport report =
		iggy::runtime::RuntimeGameplayFrameReporter {}.report(result);

	const iggy::InteractionTarget2D *door = result.state.interaction.targets.find(Id("target:raw-door"));
	Expect(door != nullptr && !door->enabled, "raw frame should preserve existing interaction mutation with prepared NPC movement");
	Expect(NearVec(result.state.npcActors.actors[0].position, { 5.5F, 0.5F }), "raw frame should apply prepared NPC movement");
	Expect(result.state.npcControls.entries.size() == 1 && result.state.npcControls.entries[0].npcId == Id("npc:raw-frame"), "raw frame should preserve NPC controls");
	Expect(report.npcMovedCount == 1, "raw frame report should project NPC moved count");
	Expect(report.npcMovementDirtyTileCount == 2, "raw frame report should project NPC dirty tile count");
	Expect(report.npcMovementNeedsOccupancyRebuild, "raw frame report should project occupancy refresh");
	Expect(HasRawEvent(report.events, iggy::runtime::RuntimeGameplayFrameEvent::NpcMovementChanged), "raw frame report should emit NPC movement event");
	Expect(NearVec(input.state.npcActors.actors[0].position, originalState.npcActors.actors[0].position), "raw frame should not mutate input state");
	Expect(SameNpcMovementFilter(input.npcMovementRequests[0].filter, originalRequests[0].filter), "raw frame should not mutate request vector");
}

void TestPolicyFramePreparedNpcMovementWithPolicyPickup()
{
	const iggy::InteractionTarget2D target = Target("target:policy-pickup", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:policy-pickup", "item:policy-pickup", 1);
	const iggy::runtime::RuntimeInteractionState interaction {
		TargetRegistry({ target }),
		EffectCatalog({ Entry("target:policy-pickup", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	iggy::runtime::RuntimeGameplayState state =
		GameplayState(SessionWithPlayer(PolicyPlayerId), interaction, InventoryState({}, { drop }));
	state.npcActors = { { NpcActor("npc:policy-frame", { 6.5F, 0.5F }) } };
	state.npcControls = { { NpcControl("npc:policy-frame") } };

	iggy::runtime::RuntimePolicyGameplayFrameInput input;
	input.state = state;
	input.actorId = PolicyPlayerId;
	input.playerIntents = { iggy::playerInteractIntent(target.id) };
	input.fallbackPlayerPosition = { 1.5F, 1.5F };
	input.playerCommandConfig = PlayerConfig();
	input.npcConfig = NpcConfig();
	input.itemDefinitions = ItemCatalog({ Definition("item:policy-pickup", "Policy Pickup", 1) });
	input.npcMovementRequests = {
		NpcMovementRequest(NpcMovementFilter("npc:policy-frame", { 6.5F, 0.5F }, { 7.5F, 0.5F })),
	};

	const iggy::runtime::RuntimePolicyGameplayFrameResult result =
		iggy::runtime::RuntimePolicyGameplayFrameStep {}.run(input);
	const iggy::runtime::RuntimePolicyGameplayFrameReport report =
		iggy::runtime::RuntimePolicyGameplayFrameReporter {}.report(result);

	Expect(SameStacks(result.state.inventory.inventory.stacks, { Stack("item:policy-pickup", 1) }), "policy frame should preserve policy pickup inventory change");
	Expect(result.inventoryEvents.events.size() == 3, "policy frame should preserve policy pickup inventory events");
	Expect(NearVec(result.state.npcActors.actors[0].position, { 7.5F, 0.5F }), "policy frame should apply prepared NPC movement");
	Expect(result.state.npcControls.entries.size() == 1 && result.state.npcControls.entries[0].npcId == Id("npc:policy-frame"), "policy frame should preserve NPC controls");
	Expect(report.npcMovedCount == 1, "policy frame report should project NPC moved count");
	Expect(report.npcMovementDirtyTileCount == 2, "policy frame report should project NPC dirty tile count");
	Expect(report.npcMovementNeedsRenderRefresh, "policy frame report should project render refresh");
	Expect(HasPolicyEvent(report.events, iggy::runtime::RuntimePolicyGameplayFrameEvent::NpcMovementChanged), "policy frame report should emit NPC movement event");
	Expect(input.state.inventory.inventory.stacks.empty(), "policy frame should not mutate input inventory");
	Expect(NearVec(input.state.npcActors.actors[0].position, { 6.5F, 0.5F }), "policy frame should not mutate input NPC actors");
}

void TestRawRunnerPreparedNpcMovementCarriesAndAggregates()
{
	iggy::runtime::RuntimeGameplayState initial = GameplayState(SessionWithPlayer(RawPlayerId));
	initial.npcActors = { { NpcActor("npc:raw-runner", { 4.5F, 0.5F }) } };
	initial.npcControls = { { NpcControl("npc:raw-runner") } };
	iggy::runtime::RuntimeGameplayFrameRunnerFrame first;
	first.actorId = RawPlayerId;
	first.fallbackPlayerPosition = { 1.5F, 1.5F };
	first.playerCommandConfig = PlayerConfig();
	first.npcConfig = NpcConfig();
	first.npcMovementRequests = {
		NpcMovementRequest(NpcMovementFilter("npc:raw-runner", { 4.5F, 0.5F }, { 5.5F, 0.5F })),
	};
	iggy::runtime::RuntimeGameplayFrameRunnerFrame second = first;
	second.npcMovementRequests = {
		NpcMovementRequest(NpcMovementFilter("npc:raw-runner", { 5.5F, 0.5F }, { 6.5F, 0.5F })),
	};
	const std::vector<iggy::runtime::RuntimeGameplayFrameRunnerFrame> frames { first, second };

	const iggy::runtime::RuntimeGameplayFrameRunnerResult result =
		iggy::runtime::RuntimeGameplayFrameRunner {}.run({ initial, frames });

	Expect(result.ticks.size() == 2, "raw runner should produce one tick per prepared movement frame");
	Expect(NearVec(result.ticks[0].frame.state.npcActors.actors[0].position, { 5.5F, 0.5F }), "raw runner first tick should move NPC");
	Expect(NearVec(result.finalState.npcActors.actors[0].position, { 6.5F, 0.5F }), "raw runner should carry moved NPC into later frame");
	Expect(result.finalState.npcControls.entries.size() == 1 && result.finalState.npcControls.entries[0].npcId == Id("npc:raw-runner"), "raw runner should preserve NPC controls");
	Expect(result.npcMovedCount == 2, "raw runner should aggregate moved count");
	Expect(result.npcMovementDirtyTileCount == 4, "raw runner should aggregate dirty tile counts");
	Expect(result.npcMovementNeedsOccupancyRebuild && result.npcMovementNeedsVisibilityRefresh, "raw runner should aggregate refresh flags");
	Expect(NearVec(initial.npcActors.actors[0].position, { 4.5F, 0.5F }), "raw runner should not mutate input state");
	Expect(SameNpcMovementFilter(frames[0].npcMovementRequests[0].filter, first.npcMovementRequests[0].filter), "raw runner should not mutate frame requests");
}

void TestPolicyRunnerPreparedNpcMovementCarriesAndAggregatesWithInventoryEvents()
{
	const iggy::InteractionTarget2D target = Target("target:policy-runner-pickup", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:policy-runner-pickup", "item:policy-runner-pickup", 1);
	const iggy::runtime::RuntimeInteractionState interaction {
		TargetRegistry({ target }),
		EffectCatalog({ Entry("target:policy-runner-pickup", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	iggy::runtime::RuntimeGameplayState initial =
		GameplayState(SessionWithPlayer(PolicyPlayerId), interaction, InventoryState({}, { drop }));
	initial.npcActors = { { NpcActor("npc:policy-runner", { 7.5F, 0.5F }) } };
	initial.npcControls = { { NpcControl("npc:policy-runner") } };
	iggy::runtime::RuntimePolicyGameplayFrameRunnerFrame first;
	first.actorId = PolicyPlayerId;
	first.playerIntents = { iggy::playerInteractIntent(target.id) };
	first.fallbackPlayerPosition = { 1.5F, 1.5F };
	first.playerCommandConfig = PlayerConfig();
	first.npcConfig = NpcConfig();
	first.npcMovementRequests = {
		NpcMovementRequest(NpcMovementFilter("npc:policy-runner", { 7.5F, 0.5F }, { 8.5F, 0.5F })),
	};
	iggy::runtime::RuntimePolicyGameplayFrameRunnerFrame second = first;
	second.playerIntents = {};
	second.npcMovementRequests = {
		NpcMovementRequest(NpcMovementFilter("npc:policy-runner", { 8.5F, 0.5F }, { 9.5F, 0.5F })),
	};

	const iggy::runtime::RuntimePolicyGameplayFrameRunnerResult result =
		iggy::runtime::RuntimePolicyGameplayFrameRunner {}.run({
			initial,
			{ first, second },
			ItemCatalog({ Definition("item:policy-runner-pickup", "Runner Pickup", 1) }),
		});

	Expect(result.ticks.size() == 2, "policy runner should produce one tick per prepared movement frame");
	Expect(result.ticks[0].report.pickedUpCount == 1, "policy runner first tick should preserve pickup behavior");
	Expect(result.inventoryEvents.events.size() == 3, "policy runner should aggregate policy inventory events");
	Expect(NearVec(result.ticks[0].frame.state.npcActors.actors[0].position, { 8.5F, 0.5F }), "policy runner first tick should move NPC");
	Expect(NearVec(result.finalState.npcActors.actors[0].position, { 9.5F, 0.5F }), "policy runner should carry moved NPC into later frame");
	Expect(result.finalState.npcControls.entries.size() == 1 && result.finalState.npcControls.entries[0].npcId == Id("npc:policy-runner"), "policy runner should preserve NPC controls");
	Expect(result.npcMovedCount == 2, "policy runner should aggregate moved count");
	Expect(result.npcMovementDirtyTileCount == 4, "policy runner should aggregate dirty tile counts");
	Expect(result.npcMovementNeedsAiMapQueryRefresh && result.npcMovementNeedsInteractionRefresh, "policy runner should aggregate refresh flags");
	Expect(initial.npcActors.actors.size() == 1 && NearVec(initial.npcActors.actors[0].position, { 7.5F, 0.5F }), "policy runner should not mutate input actor registry");
}

void TestBlockedAndMissingDiagnosticsStayAtPreparedBoundary()
{
	iggy::runtime::RuntimeGameplayState initial = GameplayState(SessionWithPlayer(RawPlayerId));
	initial.npcActors = { { NpcActor("npc:blocked", { 2.5F, 0.5F }) } };
	iggy::runtime::RuntimeGameplayFrameRunnerFrame frame;
	frame.actorId = RawPlayerId;
	frame.fallbackPlayerPosition = { 1.5F, 1.5F };
	frame.playerCommandConfig = PlayerConfig();
	frame.npcConfig = NpcConfig();
	frame.npcMovementRequests = {
		NpcMovementRequest(NpcMovementFilter(
			"npc:blocked",
			{ 2.5F, 0.5F },
			{ 3.5F, 0.5F },
			iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc,
			false,
			"npc:blocker")),
		NpcMovementRequest(NpcMovementFilter("npc:missing", { 3.5F, 0.5F }, { 4.5F, 0.5F })),
	};

	const iggy::runtime::RuntimeGameplayFrameRunnerResult result =
		iggy::runtime::RuntimeGameplayFrameRunner {}.run({ initial, { frame } });

	Expect(NearVec(result.finalState.npcActors.actors[0].position, { 2.5F, 0.5F }), "blocked/missing prepared requests should not move actor");
	Expect(result.npcMovedCount == 0, "blocked/missing prepared requests should aggregate no movement");
	Expect(result.npcBlockedMovementCount == 1, "blocked/missing prepared requests should aggregate blocked count");
	Expect(result.npcMissingActorMovementCount == 1, "blocked/missing prepared requests should aggregate missing count");
	Expect(result.npcMovementDirtyTileCount == 0, "blocked/missing prepared requests should aggregate no dirty tiles");
	Expect(result.ticks.size() == 1 && HasRawEvent(result.ticks[0].report.events, iggy::runtime::RuntimeGameplayFrameEvent::NpcMovementBlocked), "blocked prepared request should project raw report event");
	Expect(HasRawEvent(result.ticks[0].report.events, iggy::runtime::RuntimeGameplayFrameEvent::NpcMovementActorMissing), "missing prepared request should project raw report event");
	Expect(!result.npcMovementNeedsOccupancyRebuild, "blocked/missing prepared requests should not ask for occupancy rebuild");
}

} // namespace

int main()
{
	TestRawFramePreparedNpcMovementWithExistingInteraction();
	TestPolicyFramePreparedNpcMovementWithPolicyPickup();
	TestRawRunnerPreparedNpcMovementCarriesAndAggregates();
	TestPolicyRunnerPreparedNpcMovementCarriesAndAggregatesWithInventoryEvents();
	TestBlockedAndMissingDiagnosticsStayAtPreparedBoundary();

	return Failures;
}
