#include <cstdlib>
#include <type_traits>
#include <vector>

#include "runtime/RuntimePolicyGameplayFrameRunner.hpp"
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
struct HasInteractionField : std::false_type {
};

template <typename T>
struct HasInteractionField<T, std::void_t<decltype(&T::interaction)>> : std::true_type {
};

template <typename T, typename = void>
struct HasInventoryField : std::false_type {
};

template <typename T>
struct HasInventoryField<T, std::void_t<decltype(&T::inventory)>> : std::true_type {
};

template <typename T, typename = void>
struct HasCommandQueueField : std::false_type {
};

template <typename T>
struct HasCommandQueueField<T, std::void_t<decltype(&T::commandQueue)>> : std::true_type {
};

template <typename T, typename = void>
struct HasItemDefinitionsField : std::false_type {
};

template <typename T>
struct HasItemDefinitionsField<T, std::void_t<decltype(&T::itemDefinitions)>> : std::true_type {
};

static_assert(!HasInteractionField<iggy::runtime::RuntimeSessionState>::value);
static_assert(!HasInventoryField<iggy::runtime::RuntimeSessionState>::value);
static_assert(!HasCommandQueueField<iggy::runtime::RuntimeSessionState>::value);
static_assert(!HasItemDefinitionsField<iggy::runtime::RuntimeSessionState>::value);

const iggy::ResourceId PlayerId { "player:policy-gameplay-runner" };

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

iggy::runtime::RuntimeSessionState SessionWithPlayer(iggy::Vec2 position = { 0.0F, 0.0F })
{
	iggy::runtime::RuntimeSessionState session;
	session.level.map = MapFromRows({ "...", "..." });
	session.level.map.id = Id("level:policy-gameplay-runner");
	session.level.map.playerStart = { 0, 0 };
	session.tickIndex = 23;
	session.hasPlayer = true;
	session.player = PlayerAgent(PlayerId, position, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::East);
	return session;
}

iggy::runtime::GameplayCommandFrame2D WaitFrame()
{
	return CommandFrame({ iggy::runtime::GameplayCommand2DFactory {}.wait(PlayerId) });
}

iggy::NpcActorState2D NpcActor(const char *npcId, iggy::Vec2 position = { 2.0F, 2.0F })
{
	return {
		Id(npcId),
		Id("profile:policy-gameplay-runner"),
		Id("faction:policy-gameplay-runner"),
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
	if (blockingNpcId[0] != '\0') {
		filter.blockingNpcId = Id(blockingNpcId);
	}
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

iggy::InteractionTarget2DRegistry Registry(std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result = iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built, "runtime policy gameplay runner target registry fixture should build");
	return result.registry;
}

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { Id(targetId), effects };
}

iggy::InteractionEffectCatalog2D EffectCatalog(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "runtime policy gameplay runner effect catalog fixture should build");
	return result.catalog;
}

iggy::InventoryItemStack2D Stack(const char *itemId, std::uint32_t count)
{
	return { Id(itemId), count };
}

iggy::InventoryState2D Inventory(std::vector<iggy::InventoryItemStack2D> stacks)
{
	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);
	Expect(result.built, "runtime policy gameplay runner inventory fixture should build");
	return result.inventory;
}

iggy::LevelItemDrop2D Drop(
	const char *id,
	const char *itemId = "item:potion",
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

iggy::ItemDefinition2D Definition(const char *itemId, const char *displayName = "Item", std::uint32_t maxStackCount = 10)
{
	return { Id(itemId), displayName, maxStackCount, iggy::ItemDefinition2DKind::Material };
}

iggy::ItemDefinition2DCatalog ItemCatalog(std::vector<iggy::ItemDefinition2D> definitions)
{
	const iggy::ItemDefinition2DCatalogBuildResult result =
		iggy::ItemDefinition2DCatalogBuilder {}.build(definitions);
	Expect(result.built, "runtime policy gameplay runner item catalog fixture should build");
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

iggy::runtime::RuntimePolicyGameplayFrameRunnerFrame Frame(
	std::vector<iggy::PlayerInputIntent2D> intents,
	iggy::PlayerInputContext2D context = {},
	iggy::runtime::RuntimeCommandQueueConfig queueConfig = {})
{
	iggy::runtime::RuntimePolicyGameplayFrameRunnerFrame frame;
	frame.playerInputContext = context;
	frame.actorId = PlayerId;
	frame.playerIntents = intents;
	frame.commandQueueConfig = queueConfig;
	frame.fallbackPlayerPosition = { 1.5F, 1.5F };
	frame.playerCommandConfig = PlayerConfig();
	frame.npcConfig = NpcConfig();
	return frame;
}

iggy::physics2d::CollisionObject2D Object(iggy::ResourceId id, iggy::Aabb2 bounds)
{
	return { id, iggy::physics2d::makeAabbShape(bounds), true };
}

iggy::physics2d::CollisionWorld2D World(std::vector<iggy::physics2d::CollisionObject2D> objects)
{
	const iggy::physics2d::CollisionWorldBuildResult build = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);
	Expect(build.built, "runtime policy gameplay runner collision fixture should build");
	return build.world;
}

iggy::physics2d::CollisionWorld2D BlockingWorld()
{
	return World({ Object(Id("wall:east"), { { 0.5F, -0.5F }, { 1.5F, 0.5F } }) });
}

void SetCollisionCache(iggy::runtime::RuntimeSessionState &session, const iggy::physics2d::CollisionWorld2D &world)
{
	session.derivedCaches.hasCollisionCache = true;
	session.derivedCaches.collision.world = world;
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

void ExpectTargetEnabled(
	const iggy::InteractionTarget2DRegistry &registry,
	const iggy::ResourceId &targetId,
	bool enabled,
	const char *message)
{
	const iggy::InteractionTarget2D *target = registry.find(targetId);
	Expect(target != nullptr, message);
	if (target != nullptr)
		Expect(target->enabled == enabled, message);
}

void ExpectInventoryEvent(
	const iggy::InventoryEvent2D &event,
	iggy::InventoryEvent2DType type,
	const iggy::ResourceId &itemId,
	const iggy::ResourceId &dropId,
	std::uint32_t count,
	const char *message)
{
	Expect(event.type == type, message);
	Expect(event.itemId == itemId, message);
	Expect(event.dropId == dropId, message);
	Expect(event.count == count, message);
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

void TestEmptyFrameListNoOp()
{
	const iggy::runtime::RuntimeGameplayState initial =
		GameplayState(SessionWithPlayer({ 2.0F, 3.0F }), {}, {}, { { WaitFrame() } });
	const iggy::runtime::RuntimePolicyGameplayFrameRunnerInput input { initial, {}, ItemCatalog({ Definition("item:potion") }) };

	const iggy::runtime::RuntimePolicyGameplayFrameRunnerResult result =
		iggy::runtime::RuntimePolicyGameplayFrameRunner {}.run(input);

	Expect(result.ticks.empty(), "empty policy gameplay runner should produce no ticks");
	ExpectPlayerAgent(result.finalState.session.player, initial.session.player, "empty policy gameplay runner should preserve player");
	Expect(result.finalState.commandQueue.frames.size() == 1, "empty policy gameplay runner should preserve queue");
	Expect(result.finalState.inventory.inventory.stacks.empty(), "empty policy gameplay runner should preserve inventory");
	Expect(result.inventoryEvents.events.empty(), "empty policy gameplay runner should aggregate no inventory events");
	Expect(result.npcMovedCount == 0, "empty policy gameplay runner should aggregate no NPC movement");
	Expect(result.npcMovementDirtyTileCount == 0, "empty policy gameplay runner should aggregate no NPC dirty tiles");
}

void TestMultiFrameMovementCarriesSessionForward()
{
	const iggy::runtime::RuntimeGameplayState initial = GameplayState(SessionWithPlayer({ 0.0F, 0.0F }));
	const std::vector<iggy::runtime::RuntimePolicyGameplayFrameRunnerFrame> frames {
		Frame({ iggy::playerMoveToPointIntent({ 1.0F, 0.0F }) }),
		Frame({ iggy::playerMoveToPointIntent({ 2.0F, 0.0F }) }),
	};

	const iggy::runtime::RuntimePolicyGameplayFrameRunnerResult result =
		iggy::runtime::RuntimePolicyGameplayFrameRunner {}.run({ initial, frames, {} });

	Expect(result.ticks.size() == 2, "movement policy gameplay runner should produce two ticks");
	Expect(NearVec(result.ticks[0].frame.state.session.player.position, { 1.0F, 0.0F }), "first movement policy frame should move player");
	Expect(NearVec(result.ticks[1].frame.state.session.player.position, { 2.0F, 0.0F }), "second movement policy frame should use carried player position");
	Expect(NearVec(result.finalState.session.player.position, { 2.0F, 0.0F }), "policy gameplay runner should carry final player position");
	Expect(result.ticks[0].report.acceptedCommandCount == 1 && result.ticks[1].report.acceptedCommandCount == 1, "policy gameplay runner should collect per-frame reports");
}

void TestSuccessfulPolicyPickupConsumesDropAndLaterFrameSeesConsumedState()
{
	const iggy::InteractionTarget2D target = Target("target:pickup", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ target }),
		EffectCatalog({ Entry("target:pickup", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	const iggy::runtime::RuntimeGameplayState initial =
		GameplayState(SessionWithPlayer(), interaction, InventoryState({}, { drop }));
	const std::vector<iggy::runtime::RuntimePolicyGameplayFrameRunnerFrame> frames {
		Frame({ iggy::playerInteractIntent(target.id) }),
		Frame({ iggy::playerInteractIntent(target.id) }),
	};

	const iggy::runtime::RuntimePolicyGameplayFrameRunnerResult result =
		iggy::runtime::RuntimePolicyGameplayFrameRunner {}.run({
			initial,
			frames,
			ItemCatalog({ Definition("item:potion", "Potion", 5) }),
		});

	Expect(result.ticks.size() == 2, "policy pickup runner should produce two ticks");
	Expect(result.ticks[0].report.pickedUpCount == 1, "first policy pickup frame should report pickup");
	Expect(result.ticks[1].report.pickupNotReadyCount == 1, "second policy pickup frame should see consumed drop");
	Expect(SameStacks(result.finalState.inventory.inventory.stacks, { Stack("item:potion", 2) }), "policy pickup runner should carry inventory forward");
	Expect(result.finalState.inventory.drops.drops.size() == 1 && !result.finalState.inventory.drops.drops[0].enabled, "policy pickup runner should carry consumed drop");
	Expect(result.inventoryEvents.events.size() == 4, "policy pickup runner should aggregate pickup and not-ready events");
	if (result.inventoryEvents.events.size() == 4) {
		ExpectInventoryEvent(result.inventoryEvents.events[0], iggy::InventoryEvent2DType::ItemAdded, Id("item:potion"), {}, 2, "policy pickup runner should aggregate item-added event first");
		ExpectInventoryEvent(result.inventoryEvents.events[1], iggy::InventoryEvent2DType::DropConsumed, {}, drop.id, 0, "policy pickup runner should aggregate drop-consumed event second");
		ExpectInventoryEvent(result.inventoryEvents.events[2], iggy::InventoryEvent2DType::ItemPickedUp, Id("item:potion"), drop.id, 2, "policy pickup runner should aggregate picked-up event third");
		ExpectInventoryEvent(result.inventoryEvents.events[3], iggy::InventoryEvent2DType::PickupNotReady, {}, drop.id, 0, "policy pickup runner should aggregate later not-ready event");
	}
}

void TestExistingStackMaxThenOverCapacityFailureCarriesForward()
{
	const iggy::InteractionTarget2D firstTarget = Target("target:first", iggy::InteractionTarget2DKind::Pickup);
	const iggy::InteractionTarget2D secondTarget = Target("target:second", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D firstDrop = Drop("drop:first", "item:potion", 2);
	const iggy::LevelItemDrop2D secondDrop = Drop("drop:second", "item:potion", 1);
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ firstTarget, secondTarget }),
		EffectCatalog({
			Entry("target:first", { iggy::pickupItemInteractionEffect(firstTarget.id, firstDrop.id) }),
			Entry("target:second", { iggy::pickupItemInteractionEffect(secondTarget.id, secondDrop.id) }),
		}),
	};
	const iggy::runtime::RuntimeGameplayState initial =
		GameplayState(SessionWithPlayer(), interaction, InventoryState({ Stack("item:potion", 3) }, { firstDrop, secondDrop }));
	const std::vector<iggy::runtime::RuntimePolicyGameplayFrameRunnerFrame> frames {
		Frame({ iggy::playerInteractIntent(firstTarget.id) }),
		Frame({ iggy::playerInteractIntent(secondTarget.id) }),
	};

	const iggy::runtime::RuntimePolicyGameplayFrameRunnerResult result =
		iggy::runtime::RuntimePolicyGameplayFrameRunner {}.run({
			initial,
			frames,
			ItemCatalog({ Definition("item:potion", "Potion", 5) }),
		});

	Expect(result.ticks.size() == 2, "over-capacity policy runner should produce two ticks");
	Expect(result.ticks[0].report.pickedUpCount == 1, "first policy runner frame should fill stack to max");
	Expect(result.ticks[1].report.pickupFailedCount == 1, "second policy runner frame should report over-capacity failure");
	Expect(SameStacks(result.finalState.inventory.inventory.stacks, { Stack("item:potion", 5) }), "over-capacity policy runner should keep stack at max");
	Expect(result.finalState.inventory.drops.drops.size() == 2, "over-capacity policy runner should preserve both drops");
	if (result.finalState.inventory.drops.drops.size() == 2) {
		Expect(!result.finalState.inventory.drops.drops[0].enabled, "over-capacity policy runner should consume first drop");
		Expect(result.finalState.inventory.drops.drops[1].enabled, "over-capacity policy runner should not consume second drop");
	}
	Expect(result.inventoryEvents.events.size() == 3, "over-capacity policy runner should aggregate only first successful pickup events");
}

void TestMissingItemDefinitionFailureLeavesDropForLaterState()
{
	const iggy::InteractionTarget2D target = Target("target:missing", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:missing", "item:missing", 1);
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ target }),
		EffectCatalog({ Entry("target:missing", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	const iggy::runtime::RuntimeGameplayState initial =
		GameplayState(SessionWithPlayer(), interaction, InventoryState({}, { drop }));

	const iggy::runtime::RuntimePolicyGameplayFrameRunnerResult result =
		iggy::runtime::RuntimePolicyGameplayFrameRunner {}.run({
			initial,
			{ Frame({ iggy::playerInteractIntent(target.id) }) },
			ItemCatalog({ Definition("item:other", "Other", 5) }),
		});

	Expect(result.ticks.size() == 1, "missing-definition policy runner should produce one tick");
	Expect(result.ticks[0].report.pickupFailedCount == 1, "missing-definition policy runner should report pickup failure");
	Expect(result.finalState.inventory.inventory.stacks.empty(), "missing-definition policy runner should not add inventory stack");
	Expect(result.finalState.inventory.drops.drops.size() == 1 && result.finalState.inventory.drops.drops[0].enabled, "missing-definition policy runner should leave drop enabled");
	Expect(result.inventoryEvents.events.empty(), "missing-definition policy runner should aggregate no success inventory events");
}

void TestToggleCarriesToLaterFrames()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:lever"),
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F, true),
	};
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry(targets),
		EffectCatalog({ Entry("target:lever", { iggy::toggleTargetInteractionEffect(Id("target:door"), false) }) }),
	};
	const iggy::runtime::RuntimeGameplayState initial = GameplayState(SessionWithPlayer(), interaction);
	const std::vector<iggy::runtime::RuntimePolicyGameplayFrameRunnerFrame> frames {
		Frame({ iggy::playerInteractIntent(Id("target:lever")) }),
		Frame({ iggy::playerInteractIntent(Id("target:door")) }),
	};

	const iggy::runtime::RuntimePolicyGameplayFrameRunnerResult result =
		iggy::runtime::RuntimePolicyGameplayFrameRunner {}.run({ initial, frames, {} });

	Expect(result.ticks.size() == 2, "toggle policy runner should produce two ticks");
	ExpectTargetEnabled(result.finalState.interaction.targets, Id("target:door"), false, "toggle policy runner should carry toggled target");
	Expect(result.ticks[0].report.interactionChanged, "toggle policy runner should report first-frame interaction change");
	Expect(!result.ticks[1].report.interactionChanged, "toggle policy runner later frame should observe disabled target without mutation");
}

void TestQueueRejectedFrameDoesNotMutateAndLaterFrameRuns()
{
	const iggy::InteractionTarget2D target = Target("target:queue", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:queue", "item:queue", 1);
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ target }),
		EffectCatalog({ Entry("target:queue", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	const iggy::runtime::RuntimeCommandQueueState fullQueue { { WaitFrame() } };
	const iggy::runtime::RuntimeGameplayState initial =
		GameplayState(SessionWithPlayer(), interaction, InventoryState({}, { drop }), fullQueue);
	iggy::runtime::RuntimeCommandQueueConfig rejectedConfig;
	rejectedConfig.maxFrames = 1;

	const iggy::runtime::RuntimePolicyGameplayFrameRunnerResult result =
		iggy::runtime::RuntimePolicyGameplayFrameRunner {}.run({
			initial,
			{
				Frame({ iggy::playerInteractIntent(target.id) }, {}, rejectedConfig),
				Frame({ iggy::playerInteractIntent(target.id) }),
			},
			ItemCatalog({ Definition("item:queue", "Queue", 1) }),
		});

	Expect(result.ticks.size() == 2, "queue policy runner should continue after rejected frame");
	Expect(result.ticks[0].frame.frame.interaction.playerInput.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected, "queue policy runner should preserve rejected first frame status");
	Expect(result.ticks[0].report.interaction.playerInput.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected, "queue policy runner report should preserve rejected first frame status");
	Expect(result.ticks[0].report.inventoryEventCount == 0, "queue policy runner rejected frame should have no inventory events");
	Expect(result.ticks[1].report.pickedUpCount == 1, "queue policy runner should allow later accepted pickup");
	Expect(SameStacks(result.finalState.inventory.inventory.stacks, { Stack("item:queue", 1) }), "queue policy runner should carry later pickup result");
	Expect(result.inventoryEvents.events.size() == 3, "queue policy runner should aggregate only later pickup events");
}

void TestExplicitCollisionWorldAppliesToEveryFrame()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	SetCollisionCache(session, BlockingWorld());
	const iggy::runtime::RuntimeGameplayState initial = GameplayState(session);
	const std::vector<iggy::runtime::RuntimePolicyGameplayFrameRunnerFrame> frames {
		Frame({ iggy::playerMoveToPointIntent({ 2.0F, 0.0F }) }),
		Frame({ iggy::playerMoveToPointIntent({ 2.0F, 0.0F }) }),
	};
	const iggy::physics2d::CollisionWorld2D emptyWorld;

	const iggy::runtime::RuntimePolicyGameplayFrameRunnerResult result =
		iggy::runtime::RuntimePolicyGameplayFrameRunner {}.run({ initial, frames, {} }, emptyWorld);

	Expect(result.ticks.size() == 2, "explicit-world policy runner should produce two ticks");
	Expect(NearVec(result.ticks[0].frame.state.session.player.position, { 1.0F, 0.0F }), "explicit-world policy runner should affect first frame");
	Expect(NearVec(result.ticks[1].frame.state.session.player.position, { 2.0F, 0.0F }), "explicit-world policy runner should affect second frame");
	Expect(NearVec(result.finalState.session.player.position, { 2.0F, 0.0F }), "explicit-world policy runner should carry final movement");
}

void TestPreparedNpcMovementAggregatesAndCarriesAcrossPolicyFrames()
{
	iggy::runtime::RuntimeGameplayState initial = GameplayState(SessionWithPlayer({ 0.0F, 0.0F }));
	initial.npcActors = { { NpcActor("npc:policy-runner-mover", { 4.5F, 0.5F }) } };
	initial.npcControls = { { NpcControl("npc:policy-runner-mover") } };
	iggy::runtime::RuntimePolicyGameplayFrameRunnerFrame first = Frame({});
	first.npcMovementRequests = {
		NpcMovementRequest(NpcMovementFilter("npc:policy-runner-mover", { 4.5F, 0.5F }, { 5.5F, 0.5F })),
	};
	iggy::runtime::RuntimePolicyGameplayFrameRunnerFrame second = Frame({});
	second.npcMovementRequests = {
		NpcMovementRequest(NpcMovementFilter("npc:policy-runner-mover", { 5.5F, 0.5F }, { 6.5F, 0.5F })),
	};

	const iggy::runtime::RuntimePolicyGameplayFrameRunnerResult result =
		iggy::runtime::RuntimePolicyGameplayFrameRunner {}.run({ initial, { first, second }, {} });

	Expect(result.ticks.size() == 2, "policy NPC movement runner should preserve one tick per frame");
	Expect(NearVec(result.ticks[0].frame.state.npcActors.actors[0].position, { 5.5F, 0.5F }), "first policy NPC movement frame should update actor");
	Expect(NearVec(result.ticks[1].frame.state.npcActors.actors[0].position, { 6.5F, 0.5F }), "second policy NPC movement frame should carry previous actor position");
	Expect(NearVec(result.finalState.npcActors.actors[0].position, { 6.5F, 0.5F }), "policy NPC movement runner should return final carried actor position");
	Expect(result.finalState.npcControls.entries.size() == 1 && result.finalState.npcControls.entries[0].npcId == Id("npc:policy-runner-mover"), "policy NPC movement runner should preserve controls");
	Expect(result.npcMovedCount == 2, "policy NPC movement runner should aggregate moved count");
	Expect(result.npcMovementDirtyTileCount == 4, "policy NPC movement runner should aggregate per-frame dirty tile counts");
	Expect(result.npcMovementNeedsOccupancyRebuild, "policy NPC movement runner should aggregate occupancy refresh flag");
	Expect(result.npcMovementNeedsAiMapQueryRefresh, "policy NPC movement runner should aggregate AI map refresh flag");
	Expect(result.npcMovementNeedsInteractionRefresh, "policy NPC movement runner should aggregate interaction refresh flag");
	Expect(result.npcMovementNeedsRenderRefresh, "policy NPC movement runner should aggregate render refresh flag");
	Expect(result.npcMovementNeedsVisibilityRefresh, "policy NPC movement runner should aggregate visibility refresh flag");
	Expect(result.ticks[0].report.npcMovedCount == 1 && result.ticks[1].report.npcMovedCount == 1, "policy NPC movement runner should preserve per-tick movement reports");
	Expect(initial.npcActors.actors.size() == 1 && NearVec(initial.npcActors.actors[0].position, { 4.5F, 0.5F }), "policy NPC movement runner should not mutate input actor registry");
}

void TestBlockedAndMissingNpcMovementAggregateWithoutMovingActors()
{
	iggy::runtime::RuntimeGameplayState initial = GameplayState(SessionWithPlayer());
	initial.npcActors = { { NpcActor("npc:policy-runner-blocked", { 3.5F, 0.5F }) } };
	iggy::runtime::RuntimePolicyGameplayFrameRunnerFrame frame = Frame({});
	frame.npcMovementRequests = {
		NpcMovementRequest(NpcMovementFilter(
			"npc:policy-runner-blocked",
			{ 3.5F, 0.5F },
			{ 4.5F, 0.5F },
			iggy::NpcActorPathStepOccupancyFilter2DStatus::BlockedByNpc,
			false,
			"npc:policy-runner-blocker")),
		NpcMovementRequest(NpcMovementFilter("npc:policy-runner-missing", { 4.5F, 0.5F }, { 5.5F, 0.5F })),
	};

	const iggy::runtime::RuntimePolicyGameplayFrameRunnerResult result =
		iggy::runtime::RuntimePolicyGameplayFrameRunner {}.run({ initial, { frame }, {} });

	Expect(result.npcMovedCount == 0, "blocked/missing policy NPC runner should aggregate no moved actors");
	Expect(result.npcBlockedMovementCount == 1, "blocked/missing policy NPC runner should aggregate blocked count");
	Expect(result.npcMissingActorMovementCount == 1, "blocked/missing policy NPC runner should aggregate missing actor count");
	Expect(result.npcMovementDirtyTileCount == 0, "blocked/missing policy NPC runner should aggregate no dirty tiles");
	Expect(!result.npcMovementNeedsOccupancyRebuild, "blocked/missing policy NPC runner should not aggregate refresh flags");
	Expect(NearVec(result.finalState.npcActors.actors[0].position, { 3.5F, 0.5F }), "blocked/missing policy NPC runner should preserve actor position");
	Expect(result.ticks.size() == 1 && result.ticks[0].report.npcBlockedMovementCount == 1, "blocked/missing policy NPC runner should preserve per-tick blocked report");
}

void TestPolicyPickupAndNpcMovementShareFrameWithoutChangingInventoryEvents()
{
	const iggy::InteractionTarget2D target = Target("target:npc-move-pickup", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:npc-move-pickup", "item:npc-move-pickup", 1);
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ target }),
		EffectCatalog({ Entry("target:npc-move-pickup", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	iggy::runtime::RuntimeGameplayState initial =
		GameplayState(SessionWithPlayer(), interaction, InventoryState({}, { drop }));
	initial.npcActors = { { NpcActor("npc:policy-runner-pickup-mover", { 7.5F, 0.5F }) } };
	iggy::runtime::RuntimePolicyGameplayFrameRunnerFrame frame = Frame({ iggy::playerInteractIntent(target.id) });
	frame.npcMovementRequests = {
		NpcMovementRequest(NpcMovementFilter("npc:policy-runner-pickup-mover", { 7.5F, 0.5F }, { 8.5F, 0.5F })),
	};

	const iggy::runtime::RuntimePolicyGameplayFrameRunnerResult result =
		iggy::runtime::RuntimePolicyGameplayFrameRunner {}.run({
			initial,
			{ frame },
			ItemCatalog({ Definition("item:npc-move-pickup", "NPC Move Pickup", 1) }),
		});

	Expect(result.ticks.size() == 1, "policy pickup plus NPC movement should produce one tick");
	Expect(result.ticks[0].report.pickedUpCount == 1, "policy pickup plus NPC movement should preserve pickup count");
	Expect(result.inventoryEvents.events.size() == 3, "policy pickup plus NPC movement should preserve policy inventory event recording");
	Expect(SameStacks(result.finalState.inventory.inventory.stacks, { Stack("item:npc-move-pickup", 1) }), "policy pickup plus NPC movement should update inventory");
	Expect(NearVec(result.finalState.npcActors.actors[0].position, { 8.5F, 0.5F }), "policy pickup plus NPC movement should update NPC actor");
	Expect(result.npcMovedCount == 1, "policy pickup plus NPC movement should aggregate NPC movement");
}

void TestExplicitWorldOverloadCarriesPreparedNpcMovement()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	SetCollisionCache(session, BlockingWorld());
	iggy::runtime::RuntimeGameplayState initial = GameplayState(session);
	initial.npcActors = { { NpcActor("npc:policy-runner-explicit", { 7.5F, 0.5F }) } };
	iggy::runtime::RuntimePolicyGameplayFrameRunnerFrame frame = Frame({
		iggy::playerMoveToPointIntent({ 2.0F, 0.0F }),
	});
	frame.npcMovementRequests = {
		NpcMovementRequest(NpcMovementFilter("npc:policy-runner-explicit", { 7.5F, 0.5F }, { 8.5F, 0.5F })),
	};
	const iggy::physics2d::CollisionWorld2D emptyWorld;

	const iggy::runtime::RuntimePolicyGameplayFrameRunnerResult result =
		iggy::runtime::RuntimePolicyGameplayFrameRunner {}.run({ initial, { frame }, {} }, emptyWorld);

	Expect(NearVec(result.finalState.session.player.position, { 1.0F, 0.0F }), "explicit-world policy NPC runner should preserve explicit collision behavior");
	Expect(NearVec(result.finalState.npcActors.actors[0].position, { 8.5F, 0.5F }), "explicit-world policy NPC runner should still apply prepared NPC movement");
	Expect(result.npcMovedCount == 1, "explicit-world policy NPC runner should aggregate prepared NPC movement");
}

void TestInputStateAndFrameVectorAreNotMutated()
{
	const iggy::InteractionTarget2D target = Target("target:immutable", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:immutable", "item:immutable", 1);
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ target }),
		EffectCatalog({ Entry("target:immutable", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	const iggy::runtime::RuntimeGameplayState initial =
		GameplayState(SessionWithPlayer(), interaction, InventoryState({}, { drop }));
	iggy::runtime::RuntimePolicyGameplayFrameRunnerFrame frame = Frame({ iggy::playerInteractIntent(target.id) });
	frame.npcMovementRequests = {
		NpcMovementRequest(NpcMovementFilter("npc:policy-immutable-missing", { 4.5F, 0.5F }, { 5.5F, 0.5F })),
	};
	const std::vector<iggy::runtime::RuntimePolicyGameplayFrameRunnerFrame> frames { frame };
	const iggy::runtime::RuntimePolicyGameplayFrameRunnerInput input {
		initial,
		frames,
		ItemCatalog({ Definition("item:immutable", "Immutable", 1) }),
	};
	const std::vector<iggy::runtime::RuntimePolicyGameplayFrameRunnerFrame> originalFrames = frames;

	const iggy::runtime::RuntimePolicyGameplayFrameRunnerResult result =
		iggy::runtime::RuntimePolicyGameplayFrameRunner {}.run(input);

	Expect(result.ticks.size() == 1 && result.ticks[0].report.pickedUpCount == 1, "policy runner immutability setup should pick up");
	ExpectPlayerAgent(input.initialState.session.player, initial.session.player, "policy runner should not mutate input player");
	Expect(input.initialState.inventory.inventory.stacks.empty(), "policy runner should not mutate input inventory");
	Expect(input.initialState.inventory.drops.drops.size() == 1 && input.initialState.inventory.drops.drops[0].enabled, "policy runner should not mutate input drops");
	Expect(input.frames.size() == 1 && input.frames[0].playerIntents.size() == 1, "policy runner should not mutate frame vector");
	Expect(input.frames[0].playerIntents[0].targetId == target.id, "policy runner should not mutate intent payload");
	Expect(input.frames[0].npcMovementRequests.size() == 1, "policy runner should not mutate NPC movement request vector");
	Expect(SameNpcMovementFilter(input.frames[0].npcMovementRequests[0].filter, originalFrames[0].npcMovementRequests[0].filter), "policy runner should not mutate NPC movement request payload");
}

} // namespace

int main()
{
	TestEmptyFrameListNoOp();
	TestMultiFrameMovementCarriesSessionForward();
	TestSuccessfulPolicyPickupConsumesDropAndLaterFrameSeesConsumedState();
	TestExistingStackMaxThenOverCapacityFailureCarriesForward();
	TestMissingItemDefinitionFailureLeavesDropForLaterState();
	TestToggleCarriesToLaterFrames();
	TestQueueRejectedFrameDoesNotMutateAndLaterFrameRuns();
	TestExplicitCollisionWorldAppliesToEveryFrame();
	TestPreparedNpcMovementAggregatesAndCarriesAcrossPolicyFrames();
	TestBlockedAndMissingNpcMovementAggregateWithoutMovingActors();
	TestPolicyPickupAndNpcMovementShareFrameWithoutChangingInventoryEvents();
	TestExplicitWorldOverloadCarriesPreparedNpcMovement();
	TestInputStateAndFrameVectorAreNotMutated();

	return Failures;
}
