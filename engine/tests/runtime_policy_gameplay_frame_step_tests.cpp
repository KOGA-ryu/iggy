#include <cstdlib>
#include <type_traits>
#include <vector>

#include "runtime/RuntimePolicyGameplayFrameStep.hpp"
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

const iggy::ResourceId PlayerId { "player:policy-gameplay-frame" };

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
	session.level.map.id = Id("level:policy-gameplay-frame");
	session.level.map.playerStart = { 0, 0 };
	session.tickIndex = 19;
	session.hasPlayer = true;
	session.player = PlayerAgent(PlayerId, position, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::East);
	return session;
}

iggy::runtime::GameplayCommandFrame2D WaitFrame()
{
	return CommandFrame({ iggy::runtime::GameplayCommand2DFactory {}.wait(PlayerId) });
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
	Expect(result.built, "runtime policy gameplay frame target registry fixture should build");
	return result.registry;
}

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { Id(targetId), effects };
}

iggy::InteractionEffectCatalog2D EffectCatalog(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "runtime policy gameplay frame effect catalog fixture should build");
	return result.catalog;
}

iggy::InventoryItemStack2D Stack(const char *itemId, std::uint32_t count)
{
	return { Id(itemId), count };
}

iggy::InventoryState2D Inventory(std::vector<iggy::InventoryItemStack2D> stacks)
{
	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);
	Expect(result.built, "runtime policy gameplay frame inventory fixture should build");
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
	return {
		Id(id),
		Id(itemId),
		count,
		position,
		pickupRadius,
		enabled,
	};
}

iggy::runtime::RuntimeInventoryState InventoryState(
	std::vector<iggy::InventoryItemStack2D> stacks,
	std::vector<iggy::LevelItemDrop2D> drops)
{
	return {
		Inventory(stacks),
		iggy::LevelItemDrop2DRegistry { drops },
	};
}

iggy::ItemDefinition2D Definition(const char *itemId, const char *displayName = "Item", std::uint32_t maxStackCount = 10)
{
	return {
		Id(itemId),
		displayName,
		maxStackCount,
		iggy::ItemDefinition2DKind::Material,
	};
}

iggy::ItemDefinition2DCatalog ItemCatalog(std::vector<iggy::ItemDefinition2D> definitions)
{
	const iggy::ItemDefinition2DCatalogBuildResult result =
		iggy::ItemDefinition2DCatalogBuilder {}.build(definitions);
	Expect(result.built, "runtime policy gameplay frame item catalog fixture should build");
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

iggy::runtime::RuntimePolicyGameplayFrameInput Input(
	iggy::runtime::RuntimeGameplayState state,
	std::vector<iggy::PlayerInputIntent2D> intents,
	iggy::ItemDefinition2DCatalog itemDefinitions = {},
	iggy::PlayerInputContext2D context = {},
	iggy::runtime::RuntimeCommandQueueConfig queueConfig = {})
{
	iggy::runtime::RuntimePolicyGameplayFrameInput input;
	input.state = state;
	input.playerInputContext = context;
	input.actorId = PlayerId;
	input.playerIntents = intents;
	input.commandQueueConfig = queueConfig;
	input.fallbackPlayerPosition = { 1.5F, 1.5F };
	input.playerCommandConfig = PlayerConfig();
	input.npcConfig = NpcConfig();
	input.itemDefinitions = itemDefinitions;
	return input;
}

iggy::physics2d::CollisionObject2D Object(iggy::ResourceId id, iggy::Aabb2 bounds)
{
	return { id, iggy::physics2d::makeAabbShape(bounds), true };
}

iggy::physics2d::CollisionWorld2D World(std::vector<iggy::physics2d::CollisionObject2D> objects)
{
	const iggy::physics2d::CollisionWorldBuildResult build = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);
	Expect(build.built, "runtime policy gameplay frame collision fixture should build");
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

bool SameDrop(const iggy::LevelItemDrop2D &actual, const iggy::LevelItemDrop2D &expected)
{
	return actual.id == expected.id
		&& actual.itemId == expected.itemId
		&& actual.count == expected.count
		&& NearVec(actual.position, expected.position)
		&& actual.pickupRadius == expected.pickupRadius
		&& actual.enabled == expected.enabled;
}

bool SameDrops(const std::vector<iggy::LevelItemDrop2D> &actual, const std::vector<iggy::LevelItemDrop2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!SameDrop(actual[index], expected[index]))
			return false;
	}
	return true;
}

void ExpectInventoryState(
	const iggy::runtime::RuntimeInventoryState &actual,
	const iggy::runtime::RuntimeInventoryState &expected,
	const char *message)
{
	Expect(SameStacks(actual.inventory.stacks, expected.inventory.stacks), message);
	Expect(SameDrops(actual.drops.drops, expected.drops.drops), message);
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

bool SameCommand(const iggy::runtime::GameplayCommand2D &actual, const iggy::runtime::GameplayCommand2D &expected)
{
	return actual.type == expected.type
		&& actual.actorId == expected.actorId
		&& NearVec(actual.targetPoint, expected.targetPoint)
		&& actual.targetTile == expected.targetTile
		&& actual.targetId == expected.targetId;
}

bool SameFrame(const iggy::runtime::GameplayCommandFrame2D &actual, const iggy::runtime::GameplayCommandFrame2D &expected)
{
	if (actual.commands.size() != expected.commands.size())
		return false;
	for (std::size_t index = 0; index < actual.commands.size(); ++index) {
		if (!SameCommand(actual.commands[index], expected.commands[index]))
			return false;
	}
	return true;
}

bool SameQueue(const iggy::runtime::RuntimeCommandQueueState &actual, const iggy::runtime::RuntimeCommandQueueState &expected)
{
	if (actual.frames.size() != expected.frames.size())
		return false;
	for (std::size_t index = 0; index < actual.frames.size(); ++index) {
		if (!SameFrame(actual.frames[index], expected.frames[index]))
			return false;
	}
	return true;
}

void TestMoveOnlyFrameCarriesSessionAndLeavesExplicitStateStable()
{
	const iggy::runtime::RuntimeGameplayState state = GameplayState(SessionWithPlayer({ 0.0F, 0.0F }));

	const iggy::runtime::RuntimePolicyGameplayFrameResult result =
		iggy::runtime::RuntimePolicyGameplayFrameStep {}.run(
			Input(state, { iggy::playerMoveToPointIntent({ 1.0F, 0.0F }) }));

	Expect(NearVec(result.state.session.player.position, { 1.0F, 0.0F }), "policy gameplay frame should move player");
	Expect(result.state.interaction.targets.targets().empty(), "policy gameplay frame move-only should preserve empty interaction state");
	Expect(result.state.inventory.inventory.stacks.empty(), "policy gameplay frame move-only should preserve empty inventory");
	Expect(result.inventoryEvents.events.empty(), "policy gameplay frame move-only should expose no inventory events");
}

void TestSuccessfulPolicyPickupWithinMaxUpdatesInventory()
{
	const iggy::InteractionTarget2D target = Target("target:pickup", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ target }),
		EffectCatalog({ Entry("target:pickup", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	const iggy::runtime::RuntimeGameplayState state = GameplayState(SessionWithPlayer(), interaction, InventoryState({}, { drop }));
	const iggy::ItemDefinition2DCatalog items = ItemCatalog({ Definition("item:potion", "Potion", 5) });

	const iggy::runtime::RuntimePolicyGameplayFrameResult result =
		iggy::runtime::RuntimePolicyGameplayFrameStep {}.run(Input(state, { iggy::playerInteractIntent(target.id) }, items));

	Expect(result.frame.pickup.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::PickedUp, "policy gameplay frame should pick up within max");
	Expect(SameStacks(result.state.inventory.inventory.stacks, { Stack("item:potion", 2) }), "policy gameplay frame should update inventory");
	Expect(result.state.inventory.drops.drops.size() == 1 && !result.state.inventory.drops.drops[0].enabled, "policy gameplay frame should disable consumed drop");
	Expect(result.inventoryEvents.events.size() == 3, "policy gameplay frame should expose pickup inventory events");
	if (result.inventoryEvents.events.size() == 3) {
		ExpectInventoryEvent(result.inventoryEvents.events[0], iggy::InventoryEvent2DType::ItemAdded, Id("item:potion"), {}, 2, "policy gameplay frame should preserve item added event");
		ExpectInventoryEvent(result.inventoryEvents.events[1], iggy::InventoryEvent2DType::DropConsumed, {}, drop.id, 0, "policy gameplay frame should preserve drop consumed event");
		ExpectInventoryEvent(result.inventoryEvents.events[2], iggy::InventoryEvent2DType::ItemPickedUp, Id("item:potion"), drop.id, 2, "policy gameplay frame should preserve picked-up event");
	}
}

void TestExistingStackExactlyToMaxSucceeds()
{
	const iggy::InteractionTarget2D target = Target("target:stack", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ target }),
		EffectCatalog({ Entry("target:stack", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	const iggy::runtime::RuntimeGameplayState state = GameplayState(
		SessionWithPlayer(),
		interaction,
		InventoryState({ Stack("item:potion", 3) }, { drop }));
	const iggy::ItemDefinition2DCatalog items = ItemCatalog({ Definition("item:potion", "Potion", 5) });

	const iggy::runtime::RuntimePolicyGameplayFrameResult result =
		iggy::runtime::RuntimePolicyGameplayFrameStep {}.run(Input(state, { iggy::playerInteractIntent(target.id) }, items));

	Expect(result.frame.pickup.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::PickedUp, "policy gameplay frame should allow stack exactly to max");
	Expect(SameStacks(result.state.inventory.inventory.stacks, { Stack("item:potion", 5) }), "policy gameplay frame should increment existing stack to max");
}

void TestOverCapacityFailsWithoutConsumingDrop()
{
	const iggy::InteractionTarget2D target = Target("target:over_capacity", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({ Stack("item:potion", 4) }, { drop });
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ target }),
		EffectCatalog({ Entry("target:over_capacity", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	const iggy::runtime::RuntimeGameplayState state = GameplayState(SessionWithPlayer(), interaction, inventory);
	const iggy::ItemDefinition2DCatalog items = ItemCatalog({ Definition("item:potion", "Potion", 5) });

	const iggy::runtime::RuntimePolicyGameplayFrameResult result =
		iggy::runtime::RuntimePolicyGameplayFrameStep {}.run(Input(state, { iggy::playerInteractIntent(target.id) }, items));

	Expect(result.frame.pickup.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::Failed, "over-capacity policy gameplay frame should fail pickup");
	if (!result.frame.pickup.entries.empty())
		Expect(result.frame.pickup.entries[0].result.pickup.transfer.add.plan.status == iggy::InventoryStackPolicy2DStatus::StackLimitExceeded, "over-capacity policy gameplay frame should preserve stack diagnostics");
	ExpectInventoryState(result.state.inventory, inventory, "over-capacity policy gameplay frame should not consume drop");
	Expect(result.inventoryEvents.events.empty(), "over-capacity policy gameplay frame should not record picked-up events");
}

void TestMissingItemDefinitionFailsWithoutConsumingDrop()
{
	const iggy::InteractionTarget2D target = Target("target:missing_definition", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 1);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ target }),
		EffectCatalog({ Entry("target:missing_definition", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	const iggy::runtime::RuntimeGameplayState state = GameplayState(SessionWithPlayer(), interaction, inventory);
	const iggy::ItemDefinition2DCatalog items = ItemCatalog({ Definition("item:scroll", "Scroll", 5) });

	const iggy::runtime::RuntimePolicyGameplayFrameResult result =
		iggy::runtime::RuntimePolicyGameplayFrameStep {}.run(Input(state, { iggy::playerInteractIntent(target.id) }, items));

	Expect(result.frame.pickup.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::Failed, "missing definition policy gameplay frame should fail pickup");
	if (!result.frame.pickup.entries.empty())
		Expect(result.frame.pickup.entries[0].result.pickup.transfer.add.plan.status == iggy::InventoryStackPolicy2DStatus::ItemDefinitionNotFound, "missing definition policy gameplay frame should preserve diagnostics");
	ExpectInventoryState(result.state.inventory, inventory, "missing definition policy gameplay frame should not consume drop");
}

void TestToggleTargetAndPickupCarryBothStateChanges()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:combo"),
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F, true),
	};
	const iggy::LevelItemDrop2D drop = Drop("drop:key", "item:key", 1);
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry(targets),
		EffectCatalog({
			Entry("target:combo", {
				iggy::toggleTargetInteractionEffect(Id("target:door"), false),
				iggy::pickupItemInteractionEffect(Id("target:combo"), drop.id),
			}),
		}),
	};
	const iggy::runtime::RuntimeGameplayState state = GameplayState(SessionWithPlayer(), interaction, InventoryState({}, { drop }));
	const iggy::ItemDefinition2DCatalog items = ItemCatalog({ Definition("item:key", "Key", 1) });

	const iggy::runtime::RuntimePolicyGameplayFrameResult result =
		iggy::runtime::RuntimePolicyGameplayFrameStep {}.run(Input(state, { iggy::playerInteractIntent(Id("target:combo")) }, items));

	ExpectTargetEnabled(result.state.interaction.targets, Id("target:door"), false, "policy gameplay frame should return toggled target");
	Expect(SameStacks(result.state.inventory.inventory.stacks, { Stack("item:key", 1) }), "policy gameplay frame should return picked-up key");
}

void TestContextBlockedAndQueueRejectedDoNotMutateExplicitState()
{
	iggy::PlayerInputContext2D blockedContext;
	blockedContext.interactionEnabled = false;
	const iggy::InteractionTarget2D target = Target("target:blocked", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:blocked", "item:blocked", 1);
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ target }),
		EffectCatalog({ Entry("target:blocked", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::ItemDefinition2DCatalog items = ItemCatalog({ Definition("item:blocked", "Blocked", 1) });

	const iggy::runtime::RuntimePolicyGameplayFrameResult blocked =
		iggy::runtime::RuntimePolicyGameplayFrameStep {}.run(
			Input(GameplayState(SessionWithPlayer(), interaction, inventory), { iggy::playerInteractIntent(target.id) }, items, blockedContext));

	const iggy::runtime::RuntimeCommandQueueState fullQueue { { WaitFrame() } };
	iggy::runtime::RuntimeCommandQueueConfig queueConfig;
	queueConfig.maxFrames = 1;
	const iggy::runtime::RuntimePolicyGameplayFrameResult rejected =
		iggy::runtime::RuntimePolicyGameplayFrameStep {}.run(
			Input(GameplayState(SessionWithPlayer(), interaction, inventory, fullQueue), { iggy::playerInteractIntent(target.id) }, items, {}, queueConfig));

	Expect(blocked.frame.pickup.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::NoPickupEffects, "context-blocked policy gameplay frame should consume no pickup effects");
	ExpectInventoryState(blocked.state.inventory, inventory, "context-blocked policy gameplay frame should preserve inventory");
	Expect(blocked.inventoryEvents.events.empty(), "context-blocked policy gameplay frame should expose no inventory events");
	Expect(rejected.frame.interaction.playerInput.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected, "queue-rejected policy gameplay frame should preserve rejection");
	Expect(rejected.frame.pickup.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::NoPickupEffects, "queue-rejected policy gameplay frame should consume no pickup effects");
	Expect(SameQueue(rejected.state.commandQueue, fullQueue), "queue-rejected policy gameplay frame should preserve queue");
	ExpectInventoryState(rejected.state.inventory, inventory, "queue-rejected policy gameplay frame should preserve inventory");
}

void TestExplicitCollisionWorldAffectsMovementReachAndPickup()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	SetCollisionCache(session, BlockingWorld());
	const iggy::InteractionTarget2D target = Target("target:explicit", iggy::InteractionTarget2DKind::Pickup, { 1.0F, 0.0F }, 0.0F);
	const iggy::LevelItemDrop2D drop = Drop("drop:explicit", "item:explicit", 1, { 1.0F, 0.0F }, 0.0F);
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ target }),
		EffectCatalog({ Entry("target:explicit", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	const iggy::runtime::RuntimeGameplayState state = GameplayState(session, interaction, InventoryState({}, { drop }));
	const iggy::ItemDefinition2DCatalog items = ItemCatalog({ Definition("item:explicit", "Explicit", 1) });
	const iggy::physics2d::CollisionWorld2D emptyWorld;

	const iggy::runtime::RuntimePolicyGameplayFrameResult result =
		iggy::runtime::RuntimePolicyGameplayFrameStep {}.run(
			Input(state, {
				iggy::playerMoveToPointIntent({ 2.0F, 0.0F }),
				iggy::playerInteractIntent(target.id),
			}, items),
			emptyWorld);

	Expect(NearVec(result.state.session.player.position, { 1.0F, 0.0F }), "explicit-world policy gameplay frame should preserve movement override");
	Expect(SameStacks(result.state.inventory.inventory.stacks, { Stack("item:explicit", 1) }), "explicit-world policy gameplay frame should pick up after movement override");
}

void TestInputStateIsNotMutated()
{
	const iggy::InteractionTarget2D target = Target("target:immutable", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:immutable", "item:immutable", 1);
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ target }),
		EffectCatalog({ Entry("target:immutable", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::runtime::RuntimeGameplayState state = GameplayState(SessionWithPlayer(), interaction, inventory);
	const iggy::ItemDefinition2DCatalog items = ItemCatalog({ Definition("item:immutable", "Immutable", 1) });
	const iggy::runtime::RuntimePolicyGameplayFrameInput input =
		Input(state, { iggy::playerInteractIntent(target.id) }, items);

	const iggy::runtime::RuntimePolicyGameplayFrameResult result =
		iggy::runtime::RuntimePolicyGameplayFrameStep {}.run(input);

	Expect(result.frame.pickup.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::PickedUp, "immutability policy gameplay frame setup should pick up");
	ExpectPlayerAgent(input.state.session.player, state.session.player, "policy gameplay frame should not mutate input session player");
	Expect(input.state.interaction.targets.find(target.id)->enabled == target.enabled, "policy gameplay frame should not mutate input interaction targets");
	ExpectInventoryState(input.state.inventory, inventory, "policy gameplay frame should not mutate input inventory");
}

} // namespace

int main()
{
	TestMoveOnlyFrameCarriesSessionAndLeavesExplicitStateStable();
	TestSuccessfulPolicyPickupWithinMaxUpdatesInventory();
	TestExistingStackExactlyToMaxSucceeds();
	TestOverCapacityFailsWithoutConsumingDrop();
	TestMissingItemDefinitionFailsWithoutConsumingDrop();
	TestToggleTargetAndPickupCarryBothStateChanges();
	TestContextBlockedAndQueueRejectedDoNotMutateExplicitState();
	TestExplicitCollisionWorldAffectsMovementReachAndPickup();
	TestInputStateIsNotMutated();

	return Failures;
}
