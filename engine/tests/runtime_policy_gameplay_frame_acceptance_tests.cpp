#include <cstdlib>
#include <type_traits>
#include <vector>

#include "runtime/RuntimePolicyGameplayFrameRunner.hpp"
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

const iggy::ResourceId PlayerId { "player:policy-gameplay-frame-acceptance" };

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
	session.level.map.id = Id("level:policy-gameplay-frame-acceptance");
	session.level.map.playerStart = { 0, 0 };
	session.tickIndex = 47;
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
	Expect(result.built, "policy gameplay runner acceptance target registry fixture should build");
	return result.registry;
}

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { Id(targetId), effects };
}

iggy::InteractionEffectCatalog2D EffectCatalog(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "policy gameplay runner acceptance effect catalog fixture should build");
	return result.catalog;
}

iggy::InventoryItemStack2D Stack(const char *itemId, std::uint32_t count)
{
	return { Id(itemId), count };
}

iggy::InventoryState2D Inventory(std::vector<iggy::InventoryItemStack2D> stacks)
{
	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);
	Expect(result.built, "policy gameplay runner acceptance inventory fixture should build");
	return result.inventory;
}

iggy::LevelItemDrop2D Drop(
	const char *id,
	const char *itemId,
	std::uint32_t count,
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
	Expect(result.built, "policy gameplay runner acceptance item catalog fixture should build");
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
	Expect(build.built, "policy gameplay runner acceptance collision fixture should build");
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

void ExpectDropEnabled(
	const iggy::LevelItemDrop2DRegistry &registry,
	const iggy::ResourceId &dropId,
	bool enabled,
	const char *message)
{
	const iggy::LevelItemDrop2D *drop = registry.find(dropId);
	Expect(drop != nullptr, message);
	if (drop != nullptr)
		Expect(drop->enabled == enabled, message);
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

void TestPolicyRunnerFullGameplayRoad()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:potion", iggy::InteractionTarget2DKind::Pickup, { 0.25F, 0.0F }),
		Target("target:stack", iggy::InteractionTarget2DKind::Pickup, { 0.25F, 0.0F }),
		Target("target:over", iggy::InteractionTarget2DKind::Pickup, { 0.25F, 0.0F }),
		Target("target:missing_definition", iggy::InteractionTarget2DKind::Pickup, { 0.25F, 0.0F }),
		Target("target:combo", iggy::InteractionTarget2DKind::Pickup, { 0.25F, 0.0F }),
		Target("target:move_pickup", iggy::InteractionTarget2DKind::Pickup, { 1.0F, 0.0F }),
		Target("target:blocked", iggy::InteractionTarget2DKind::Pickup, { 0.25F, 0.0F }),
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 0.25F, 0.0F }, 0.0F, true),
	};
	const std::vector<iggy::LevelItemDrop2D> drops {
		Drop("drop:potion", "item:potion", 2, { 0.25F, 0.0F }),
		Drop("drop:stack", "item:potion", 2, { 0.25F, 0.0F }),
		Drop("drop:over", "item:potion", 1, { 0.25F, 0.0F }),
		Drop("drop:missing_definition", "item:missing", 1, { 0.25F, 0.0F }),
		Drop("drop:key", "item:key", 1, { 0.25F, 0.0F }),
		Drop("drop:move", "item:move", 1, { 1.0F, 0.0F }),
		Drop("drop:blocked", "item:blocked", 1, { 0.25F, 0.0F }),
	};
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry(targets),
		EffectCatalog({
			Entry("target:potion", { iggy::pickupItemInteractionEffect(Id("target:potion"), Id("drop:potion")) }),
			Entry("target:stack", { iggy::pickupItemInteractionEffect(Id("target:stack"), Id("drop:stack")) }),
			Entry("target:over", { iggy::pickupItemInteractionEffect(Id("target:over"), Id("drop:over")) }),
			Entry("target:missing_definition", { iggy::pickupItemInteractionEffect(Id("target:missing_definition"), Id("drop:missing_definition")) }),
			Entry("target:combo", {
				iggy::toggleTargetInteractionEffect(Id("target:door"), false),
				iggy::pickupItemInteractionEffect(Id("target:combo"), Id("drop:key")),
			}),
			Entry("target:move_pickup", { iggy::pickupItemInteractionEffect(Id("target:move_pickup"), Id("drop:move")) }),
			Entry("target:blocked", { iggy::pickupItemInteractionEffect(Id("target:blocked"), Id("drop:blocked")) }),
		}),
	};
	const iggy::runtime::RuntimeInventoryState inventory =
		InventoryState({ Stack("item:potion", 3) }, drops);
	const iggy::runtime::RuntimeGameplayState initial =
		GameplayState(SessionWithPlayer(), interaction, inventory);
	const iggy::ItemDefinition2DCatalog items = ItemCatalog({
		Definition("item:potion", "Potion", 7),
		Definition("item:key", "Key", 1),
		Definition("item:move", "Move Gem", 1),
		Definition("item:blocked", "Blocked", 1),
	});
	iggy::PlayerInputContext2D blockedContext;
	blockedContext.interactionEnabled = false;
	const std::vector<iggy::runtime::RuntimePolicyGameplayFrameRunnerFrame> frames {
		Frame({ iggy::playerMoveToPointIntent({ 0.25F, 0.0F }) }),
		Frame({ iggy::playerInteractIntent(Id("target:potion")) }),
		Frame({ iggy::playerInteractIntent(Id("target:stack")) }),
		Frame({ iggy::playerInteractIntent(Id("target:over")) }),
		Frame({ iggy::playerInteractIntent(Id("target:missing_definition")) }),
		Frame({ iggy::playerInteractIntent(Id("target:combo")) }),
		Frame({
			iggy::playerMoveToPointIntent({ 1.0F, 0.0F }),
			iggy::playerInteractIntent(Id("target:move_pickup")),
		}),
		Frame({ iggy::playerInteractIntent(Id("target:blocked")) }, blockedContext),
	};
	iggy::runtime::RuntimePolicyGameplayFrameRunnerInput input { initial, frames, items };

	const iggy::runtime::RuntimePolicyGameplayFrameRunnerResult result =
		iggy::runtime::RuntimePolicyGameplayFrameRunner {}.run(input);

	Expect(result.ticks.size() == frames.size(), "policy gameplay acceptance should produce one tick per input frame");
	Expect(NearVec(result.ticks[0].frame.state.session.player.position, { 0.25F, 0.0F }), "move-only policy frame should move player");
	Expect(result.ticks[0].frame.state.interaction.targets.targets().size() == targets.size(), "move-only policy frame should preserve interaction targets");
	Expect(SameStacks(result.ticks[0].frame.state.inventory.inventory.stacks, { Stack("item:potion", 3) }), "move-only policy frame should preserve inventory");
	Expect(result.ticks[1].report.pickedUpCount == 1, "reachable policy pickup should report success");
	Expect(result.ticks[2].report.pickedUpCount == 1, "existing stack exact-max policy pickup should report success");
	Expect(result.ticks[3].report.pickupFailedCount == 1, "over-max policy pickup should report failure");
	Expect(result.ticks[4].report.pickupFailedCount == 1, "missing-definition policy pickup should report failure");
	Expect(result.ticks[5].report.interactionChanged && result.ticks[5].report.pickedUpCount == 1, "combo policy frame should report interaction and pickup");
	Expect(result.ticks[6].report.pickedUpCount == 1, "move-then-interact policy frame should use post-move reach");
	Expect(result.ticks[7].report.blockedIntentCount == 1, "context-blocked policy frame should report blocked intent");

	Expect(NearVec(result.finalState.session.player.position, { 1.0F, 0.0F }), "policy gameplay acceptance should return final player position");
	ExpectTargetEnabled(result.finalState.interaction.targets, Id("target:door"), false, "policy gameplay acceptance should carry target toggle");
	Expect(SameStacks(result.finalState.inventory.inventory.stacks, {
		Stack("item:potion", 7),
		Stack("item:key", 1),
		Stack("item:move", 1),
	}), "policy gameplay acceptance should carry inventory changes and capacity failure");
	ExpectDropEnabled(result.finalState.inventory.drops, Id("drop:potion"), false, "policy gameplay acceptance should consume first pickup drop");
	ExpectDropEnabled(result.finalState.inventory.drops, Id("drop:stack"), false, "policy gameplay acceptance should consume exact-max drop");
	ExpectDropEnabled(result.finalState.inventory.drops, Id("drop:over"), true, "policy gameplay acceptance should not consume over-capacity drop");
	ExpectDropEnabled(result.finalState.inventory.drops, Id("drop:missing_definition"), true, "policy gameplay acceptance should not consume missing-definition drop");
	ExpectDropEnabled(result.finalState.inventory.drops, Id("drop:key"), false, "policy gameplay acceptance should consume combo pickup drop");
	ExpectDropEnabled(result.finalState.inventory.drops, Id("drop:move"), false, "policy gameplay acceptance should consume post-move pickup drop");
	ExpectDropEnabled(result.finalState.inventory.drops, Id("drop:blocked"), true, "policy gameplay acceptance should not consume blocked drop");
	Expect(result.inventoryEvents.events.size() == 12, "policy gameplay acceptance should aggregate four successful pickup event triplets");
	if (result.inventoryEvents.events.size() == 12) {
		ExpectInventoryEvent(result.inventoryEvents.events[0], iggy::InventoryEvent2DType::ItemAdded, Id("item:potion"), {}, 2, "policy gameplay acceptance event 0");
		ExpectInventoryEvent(result.inventoryEvents.events[2], iggy::InventoryEvent2DType::ItemPickedUp, Id("item:potion"), Id("drop:potion"), 2, "policy gameplay acceptance event 2");
		ExpectInventoryEvent(result.inventoryEvents.events[3], iggy::InventoryEvent2DType::ItemAdded, Id("item:potion"), {}, 2, "policy gameplay acceptance event 3");
		ExpectInventoryEvent(result.inventoryEvents.events[5], iggy::InventoryEvent2DType::ItemPickedUp, Id("item:potion"), Id("drop:stack"), 2, "policy gameplay acceptance event 5");
		ExpectInventoryEvent(result.inventoryEvents.events[6], iggy::InventoryEvent2DType::ItemAdded, Id("item:key"), {}, 1, "policy gameplay acceptance event 6");
		ExpectInventoryEvent(result.inventoryEvents.events[8], iggy::InventoryEvent2DType::ItemPickedUp, Id("item:key"), Id("drop:key"), 1, "policy gameplay acceptance event 8");
		ExpectInventoryEvent(result.inventoryEvents.events[9], iggy::InventoryEvent2DType::ItemAdded, Id("item:move"), {}, 1, "policy gameplay acceptance event 9");
		ExpectInventoryEvent(result.inventoryEvents.events[11], iggy::InventoryEvent2DType::ItemPickedUp, Id("item:move"), Id("drop:move"), 1, "policy gameplay acceptance event 11");
	}
	ExpectPlayerAgent(input.initialState.session.player, initial.session.player, "policy gameplay acceptance should not mutate input player");
	Expect(input.initialState.inventory.drops.find(Id("drop:potion"))->enabled, "policy gameplay acceptance should not mutate input drops");
	Expect(input.frames.size() == frames.size() && input.frames[1].playerIntents[0].targetId == Id("target:potion"), "policy gameplay acceptance should not mutate input frames");
}

void TestQueueRejectedMappedInteractPreservesDiagnosticsAndDoesNotMutate()
{
	const iggy::InteractionTarget2D target = Target("target:queue", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:queue", "item:queue", 1);
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ target }),
		EffectCatalog({ Entry("target:queue", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::runtime::RuntimeCommandQueueState fullQueue { { WaitFrame() } };
	const iggy::runtime::RuntimeGameplayState initial = GameplayState(SessionWithPlayer(), interaction, inventory, fullQueue);
	iggy::runtime::RuntimeCommandQueueConfig rejectConfig;
	rejectConfig.maxFrames = 1;
	const iggy::runtime::RuntimePolicyGameplayFrameRunnerInput input {
		initial,
		{ Frame({ iggy::playerInteractIntent(target.id) }, {}, rejectConfig) },
		ItemCatalog({ Definition("item:queue", "Queue", 1) }),
	};

	const iggy::runtime::RuntimePolicyGameplayFrameRunnerResult result =
		iggy::runtime::RuntimePolicyGameplayFrameRunner {}.run(input);

	Expect(result.ticks.size() == 1, "queue-rejected policy acceptance should produce one tick");
	Expect(result.ticks[0].frame.frame.interaction.playerInput.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected, "queue-rejected policy acceptance should preserve rejection diagnostics");
	Expect(result.ticks[0].report.inventoryEventCount == 0, "queue-rejected policy acceptance should report no inventory events");
	Expect(result.ticks[0].frame.frame.pickup.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::NoPickupEffects, "queue-rejected policy acceptance should skip pickup effects");
	Expect(result.inventoryEvents.events.empty(), "queue-rejected policy acceptance should aggregate no inventory events");
	Expect(result.finalState.inventory.inventory.stacks.empty(), "queue-rejected policy acceptance should not mutate inventory");
	ExpectDropEnabled(result.finalState.inventory.drops, drop.id, true, "queue-rejected policy acceptance should not consume drop");
	Expect(result.finalState.commandQueue.frames.size() == fullQueue.frames.size(), "queue-rejected policy acceptance should preserve full queue");
	Expect(input.initialState.inventory.drops.find(drop.id)->enabled, "queue-rejected policy acceptance should not mutate input drops");
}

void TestExplicitCollisionWorldControlsMovementReachAndPickup()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	SetCollisionCache(session, BlockingWorld());
	const iggy::InteractionTarget2D target = Target("target:explicit", iggy::InteractionTarget2DKind::Pickup, { 0.0F, 0.0F }, 1.0F);
	const iggy::LevelItemDrop2D drop = Drop("drop:explicit", "item:explicit", 1, { 1.0F, 0.0F });
	const iggy::runtime::RuntimeInteractionState interaction {
		Registry({ target }),
		EffectCatalog({ Entry("target:explicit", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }),
	};
	const iggy::runtime::RuntimeGameplayState initial =
		GameplayState(session, interaction, InventoryState({}, { drop }));
	const iggy::runtime::RuntimePolicyGameplayFrameRunnerInput input {
		initial,
		{
			Frame({
				iggy::playerMoveToPointIntent({ 2.0F, 0.0F }),
				iggy::playerInteractIntent(target.id),
			}),
		},
		ItemCatalog({ Definition("item:explicit", "Explicit", 1) }),
	};
	const iggy::physics2d::CollisionWorld2D emptyWorld;

	const iggy::runtime::RuntimePolicyGameplayFrameRunnerResult blocked =
		iggy::runtime::RuntimePolicyGameplayFrameRunner {}.run(input);
	const iggy::runtime::RuntimePolicyGameplayFrameRunnerResult explicitResult =
		iggy::runtime::RuntimePolicyGameplayFrameRunner {}.run(input, emptyWorld);

	Expect(blocked.ticks.size() == 1 && explicitResult.ticks.size() == 1, "explicit collision policy acceptance should produce one tick for both runs");
	Expect(blocked.ticks[0].report.pickedUpCount == 0, "session collision cache should block movement and pickup reach");
	Expect(blocked.ticks[0].report.pickupNotReadyCount == 1, "blocked movement should report pickup not ready");
	Expect(explicitResult.ticks[0].report.pickedUpCount == 1, "explicit empty world should allow movement and pickup");
	Expect(NearVec(explicitResult.finalState.session.player.position, { 1.0F, 0.0F }), "explicit collision policy acceptance should move player");
	Expect(SameStacks(explicitResult.finalState.inventory.inventory.stacks, { Stack("item:explicit", 1) }), "explicit collision policy acceptance should update inventory");
	Expect(explicitResult.inventoryEvents.events.size() == 3, "explicit collision policy acceptance should aggregate pickup events");
}

} // namespace

int main()
{
	TestPolicyRunnerFullGameplayRoad();
	TestQueueRejectedMappedInteractPreservesDiagnosticsAndDoesNotMutate();
	TestExplicitCollisionWorldControlsMovementReachAndPickup();

	return Failures;
}
