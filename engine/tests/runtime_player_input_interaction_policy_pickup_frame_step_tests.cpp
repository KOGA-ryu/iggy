#include <cstdlib>
#include <vector>

#include "runtime/RuntimePlayerInputInteractionPolicyPickupFrameStep.hpp"
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

const iggy::ResourceId PlayerId { "player:input-interaction-policy-pickup-frame" };

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

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::runtime::RuntimeSessionState SessionWithPlayer(iggy::Vec2 position = { 0.0F, 0.0F })
{
	iggy::runtime::RuntimeSessionState session;
	session.level.map = MapFromRows({ "...", "..." });
	session.level.map.id = Id("level:input-interaction-policy-pickup-frame");
	session.level.map.playerStart = { 0, 0 };
	session.tickIndex = 101;
	session.hasPlayer = true;
	session.player = PlayerAgent(PlayerId, position, { 0, 0 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::East);
	return session;
}

iggy::runtime::GameplayCommandFrame2D WaitFrame()
{
	return CommandFrame({ iggy::runtime::GameplayCommand2DFactory {}.wait(PlayerId) });
}

iggy::runtime::RuntimePlayerInputGatedFrameStepInput PlayerInput(
	iggy::runtime::RuntimeSessionState session,
	std::vector<iggy::PlayerInputIntent2D> intents,
	iggy::PlayerInputContext2D context = {},
	iggy::runtime::RuntimeCommandQueueState queue = {},
	iggy::runtime::RuntimeCommandQueueConfig queueConfig = {})
{
	return {
		{
			session,
			queue,
			queueConfig,
			PlayerId,
			context,
			intents,
			{ 1.5F, 1.5F },
			PlayerConfig(),
			NpcConfig(),
		},
	};
}

iggy::InteractionTarget2D Target(
	const char *id,
	iggy::InteractionTarget2DKind kind = iggy::InteractionTarget2DKind::Pickup,
	iggy::Vec2 position = { 0.0F, 0.0F },
	float radius = 0.0F,
	bool enabled = true)
{
	return { Id(id), kind, position, radius, enabled };
}

iggy::InteractionTarget2DRegistry Registry(std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result = iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built, "runtime player input interaction policy pickup registry fixture should build");
	return result.registry;
}

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { Id(targetId), effects };
}

iggy::InteractionEffectCatalog2D EffectCatalog(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "runtime player input interaction policy pickup effect catalog fixture should build");
	return result.catalog;
}

iggy::runtime::RuntimeInteractionState InteractionState(
	iggy::InteractionTarget2DRegistry targets,
	iggy::InteractionEffectCatalog2D effects)
{
	return { targets, effects };
}

iggy::InventoryItemStack2D Stack(const char *itemId, std::uint32_t count)
{
	return { Id(itemId), count };
}

iggy::InventoryState2D Inventory(std::vector<iggy::InventoryItemStack2D> stacks)
{
	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);
	Expect(result.built, "runtime player input interaction policy pickup inventory fixture should build");
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
	Expect(result.built, "runtime player input interaction policy pickup item catalog fixture should build");
	return result.catalog;
}

iggy::runtime::RuntimePlayerInputInteractionStateApplyFrameInput InteractionInput(
	iggy::runtime::RuntimePlayerInputGatedFrameStepInput playerInput,
	iggy::runtime::RuntimeInteractionState interaction,
	iggy::InteractionReach2DConfig reach = {})
{
	return { playerInput, interaction, reach };
}

iggy::runtime::RuntimePlayerInputInteractionPolicyPickupFrameInput Input(
	iggy::runtime::RuntimePlayerInputInteractionStateApplyFrameInput interactionInput,
	iggy::runtime::RuntimeInventoryState inventory,
	iggy::ItemDefinition2DCatalog itemDefinitions,
	iggy::runtime::RuntimePolicyPickupConfig pickup = {})
{
	return { interactionInput, inventory, itemDefinitions, pickup };
}

iggy::physics2d::CollisionObject2D Object(iggy::ResourceId id, iggy::Aabb2 bounds)
{
	return { id, iggy::physics2d::makeAabbShape(bounds), true };
}

iggy::physics2d::CollisionWorld2D World(std::vector<iggy::physics2d::CollisionObject2D> objects)
{
	const iggy::physics2d::CollisionWorldBuildResult build = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);
	Expect(build.built, "runtime player input interaction policy pickup collision fixture should build");
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

bool SameDrops(
	const std::vector<iggy::LevelItemDrop2D> &actual,
	const std::vector<iggy::LevelItemDrop2D> &expected)
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

void ExpectTarget(const iggy::InteractionTarget2D &actual, const iggy::InteractionTarget2D &expected, const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.kind == expected.kind, message);
	Expect(NearVec(actual.position, expected.position), message);
	Expect(actual.radius == expected.radius, message);
	Expect(actual.enabled == expected.enabled, message);
}

void ExpectRegistryTargets(
	const iggy::InteractionTarget2DRegistry &registry,
	const std::vector<iggy::InteractionTarget2D> &expected,
	const char *message)
{
	Expect(registry.targets().size() == expected.size(), message);
	if (registry.targets().size() != expected.size())
		return;
	for (std::size_t index = 0; index < expected.size(); ++index)
		ExpectTarget(registry.targets()[index], expected[index], message);
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

void ExpectNoInventoryEvents(
	const iggy::runtime::RuntimePlayerInputInteractionPolicyPickupFrameResult &result,
	const char *message)
{
	Expect(result.inventoryEvents.events.empty(), message);
}

void ExpectSuccessfulPickupInventoryEventsAt(
	const iggy::runtime::RuntimePlayerInputInteractionPolicyPickupFrameResult &result,
	std::size_t offset,
	const iggy::LevelItemDrop2D &drop,
	const char *message)
{
	Expect(result.inventoryEvents.events.size() >= offset + 3, message);
	if (result.inventoryEvents.events.size() >= offset + 3) {
		ExpectInventoryEvent(result.inventoryEvents.events[offset], iggy::InventoryEvent2DType::ItemAdded, drop.itemId, {}, drop.count, message);
		ExpectInventoryEvent(result.inventoryEvents.events[offset + 1], iggy::InventoryEvent2DType::DropConsumed, {}, drop.id, 0, message);
		ExpectInventoryEvent(result.inventoryEvents.events[offset + 2], iggy::InventoryEvent2DType::ItemPickedUp, drop.itemId, drop.id, drop.count, message);
	}
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

bool SameEffect(const iggy::InteractionEffect2D &actual, const iggy::InteractionEffect2D &expected)
{
	return actual.type == expected.type
		&& actual.targetId == expected.targetId
		&& actual.eventId == expected.eventId
		&& actual.dropId == expected.dropId
		&& actual.text == expected.text
		&& actual.enabledValue == expected.enabledValue;
}

bool SameEffects(const std::vector<iggy::InteractionEffect2D> &actual, const std::vector<iggy::InteractionEffect2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (!SameEffect(actual[index], expected[index]))
			return false;
	}
	return true;
}

void ExpectCatalogPreserved(
	const iggy::InteractionEffectCatalog2D &actual,
	const iggy::InteractionEffectCatalog2D &expected,
	const char *message)
{
	Expect(actual.entries().size() == expected.entries().size(), message);
	if (actual.entries().size() != expected.entries().size())
		return;
	for (std::size_t entryIndex = 0; entryIndex < actual.entries().size(); ++entryIndex) {
		Expect(actual.entries()[entryIndex].targetId == expected.entries()[entryIndex].targetId, message);
		Expect(SameEffects(actual.entries()[entryIndex].effects, expected.entries()[entryIndex].effects), message);
	}
}

void TestSuccessfulReachablePickupWithinMax()
{
	const iggy::InteractionTarget2D target = Target("target:pickup");
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::ItemDefinition2DCatalog items = ItemCatalog({ Definition("item:potion", "Potion", 5) });

	const iggy::runtime::RuntimePlayerInputInteractionPolicyPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPolicyPickupFrameStep {}.run(
			Input(
				InteractionInput(
					PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(target.id) }),
					InteractionState(Registry({ target }), EffectCatalog({ Entry("target:pickup", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }))),
				inventory,
				items));

	Expect(result.pickup.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::PickedUp, "policy pickup adapter should pick up within max");
	Expect(result.pickup.pickedUpCount == 1, "policy pickup adapter should count picked-up effect");
	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:potion", 2) }), "policy pickup adapter should add item");
	Expect(result.inventory.drops.drops.size() == 1, "policy pickup adapter should keep disabled drop");
	if (result.inventory.drops.drops.size() == 1) {
		iggy::LevelItemDrop2D disabled = drop;
		disabled.enabled = false;
		Expect(SameDrop(result.inventory.drops.drops[0], disabled), "policy pickup adapter should disable consumed drop");
	}
	Expect(result.inventoryEvents.events.size() == 3, "policy pickup adapter should expose pickup events");
	ExpectSuccessfulPickupInventoryEventsAt(result, 0, drop, "policy pickup adapter should preserve event order");
}

void TestExistingStackExactlyToMaxSucceeds()
{
	const iggy::InteractionTarget2D target = Target("target:stack");
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({ Stack("item:potion", 3) }, { drop });
	const iggy::ItemDefinition2DCatalog items = ItemCatalog({ Definition("item:potion", "Potion", 5) });

	const iggy::runtime::RuntimePlayerInputInteractionPolicyPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPolicyPickupFrameStep {}.run(
			Input(
				InteractionInput(
					PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(target.id) }),
					InteractionState(Registry({ target }), EffectCatalog({ Entry("target:stack", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }))),
				inventory,
				items));

	Expect(result.pickup.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::PickedUp, "policy pickup adapter should allow exactly max");
	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:potion", 5) }), "policy pickup adapter should increment to max");
	if (!result.pickup.entries.empty())
		Expect(result.pickup.entries[0].result.pickup.transfer.add.plan.resultingCount == 5, "policy pickup adapter should preserve resulting-count diagnostics");
}

void TestOverCapacityFailsWithoutConsumingDrop()
{
	const iggy::InteractionTarget2D target = Target("target:over_capacity");
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({ Stack("item:potion", 4) }, { drop });
	const iggy::ItemDefinition2DCatalog items = ItemCatalog({ Definition("item:potion", "Potion", 5) });

	const iggy::runtime::RuntimePlayerInputInteractionPolicyPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPolicyPickupFrameStep {}.run(
			Input(
				InteractionInput(
					PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(target.id) }),
					InteractionState(Registry({ target }), EffectCatalog({ Entry("target:over_capacity", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }))),
				inventory,
				items));

	Expect(result.pickup.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::Failed, "over-capacity policy pickup adapter should fail");
	Expect(result.pickup.failedCount == 1, "over-capacity policy pickup adapter should count failure");
	if (!result.pickup.entries.empty())
		Expect(result.pickup.entries[0].result.pickup.transfer.add.plan.status == iggy::InventoryStackPolicy2DStatus::StackLimitExceeded, "over-capacity policy pickup adapter should preserve stack diagnostics");
	ExpectInventoryState(result.inventory, inventory, "over-capacity policy pickup adapter should not consume drop");
	ExpectNoInventoryEvents(result, "over-capacity policy pickup adapter should not record picked-up event");
}

void TestMissingItemDefinitionFailsWithoutConsumingDrop()
{
	const iggy::InteractionTarget2D target = Target("target:missing_definition");
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 1);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::ItemDefinition2DCatalog items = ItemCatalog({ Definition("item:scroll", "Scroll", 5) });

	const iggy::runtime::RuntimePlayerInputInteractionPolicyPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPolicyPickupFrameStep {}.run(
			Input(
				InteractionInput(
					PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(target.id) }),
					InteractionState(Registry({ target }), EffectCatalog({ Entry("target:missing_definition", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }))),
				inventory,
				items));

	Expect(result.pickup.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::Failed, "missing definition policy pickup adapter should fail");
	if (!result.pickup.entries.empty())
		Expect(result.pickup.entries[0].result.pickup.transfer.add.plan.status == iggy::InventoryStackPolicy2DStatus::ItemDefinitionNotFound, "missing definition policy pickup adapter should preserve definition diagnostics");
	ExpectInventoryState(result.inventory, inventory, "missing definition policy pickup adapter should preserve inventory and drops");
	ExpectNoInventoryEvents(result, "missing definition policy pickup adapter should not record events");
}

void TestToggleTargetAndPickupUpdateBothStates()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:combo", iggy::InteractionTarget2DKind::Usable),
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F, true),
	};
	std::vector<iggy::InteractionTarget2D> expectedTargets = targets;
	expectedTargets[1].enabled = false;
	const iggy::LevelItemDrop2D drop = Drop("drop:key", "item:key", 1);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::ItemDefinition2DCatalog items = ItemCatalog({ Definition("item:key", "Key", 1) });

	const iggy::runtime::RuntimePlayerInputInteractionPolicyPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPolicyPickupFrameStep {}.run(
			Input(
				InteractionInput(
					PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(targets[0].id) }),
					InteractionState(
						Registry(targets),
						EffectCatalog({
							Entry("target:combo", {
								iggy::toggleTargetInteractionEffect(targets[1].id, false),
								iggy::pickupItemInteractionEffect(targets[0].id, drop.id),
							}),
						}))),
				inventory,
				items));

	Expect(result.interaction.application.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::Applied, "combo policy pickup adapter should apply interaction target mutation");
	ExpectRegistryTargets(result.interactionState.targets, expectedTargets, "combo policy pickup adapter should update interaction targets");
	Expect(result.pickup.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::PickedUp, "combo policy pickup adapter should pick up item");
	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:key", 1) }), "combo policy pickup adapter should update inventory");
	if (!result.pickup.entries.empty())
		Expect(result.pickup.entries[0].effectIndex == 1, "combo policy pickup adapter should preserve pickup effect index after toggle");
	ExpectSuccessfulPickupInventoryEventsAt(result, 0, drop, "combo policy pickup adapter should expose inventory events");
}

void TestContextBlockedInteractDoesNotConsumePickup()
{
	iggy::PlayerInputContext2D context;
	context.interactionEnabled = false;
	const iggy::InteractionTarget2D target = Target("target:blocked");
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 1);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::ItemDefinition2DCatalog items = ItemCatalog({ Definition("item:potion", "Potion", 5) });

	const iggy::runtime::RuntimePlayerInputInteractionPolicyPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPolicyPickupFrameStep {}.run(
			Input(
				InteractionInput(
					PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(target.id) }, context),
					InteractionState(Registry({ target }), EffectCatalog({ Entry("target:blocked", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }))),
				inventory,
				items));

	Expect(result.interaction.playerInput.command.intake.mapping.gateIssues.size() == 1, "context-blocked policy pickup adapter should preserve gate issue");
	Expect(!result.interaction.application.hasInteractions(), "context-blocked policy pickup adapter should have no interaction application");
	Expect(result.pickup.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::NoPickupEffects, "context-blocked policy pickup adapter should find no pickup effects");
	ExpectInventoryState(result.inventory, inventory, "context-blocked policy pickup adapter should preserve inventory");
	ExpectNoInventoryEvents(result, "context-blocked policy pickup adapter should expose no inventory events");
}

void TestQueueRejectedMappedInteractDoesNotConsumePickup()
{
	const iggy::InteractionTarget2D target = Target("target:queued");
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 1);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::runtime::RuntimeCommandQueueState fullQueue { { WaitFrame() } };
	const iggy::ItemDefinition2DCatalog items = ItemCatalog({ Definition("item:potion", "Potion", 5) });

	const iggy::runtime::RuntimePlayerInputInteractionPolicyPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPolicyPickupFrameStep {}.run(
			Input(
				InteractionInput(
					PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(target.id) }, {}, fullQueue, { 1 }),
					InteractionState(Registry({ target }), EffectCatalog({ Entry("target:queued", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }))),
				inventory,
				items));

	Expect(result.interaction.playerInput.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected, "queue-rejected policy pickup adapter should preserve queue rejection");
	Expect(!result.interaction.application.hasInteractions(), "queue-rejected policy pickup adapter should skip interaction application");
	Expect(result.pickup.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::NoPickupEffects, "queue-rejected policy pickup adapter should consume no pickup effects");
	Expect(SameQueue(result.queue, fullQueue), "queue-rejected policy pickup adapter should preserve queue");
	ExpectInventoryState(result.inventory, inventory, "queue-rejected policy pickup adapter should preserve inventory");
	ExpectNoInventoryEvents(result, "queue-rejected policy pickup adapter should expose no inventory events");
}

void TestMoveThenInteractUsesPostMovePosition()
{
	const iggy::InteractionTarget2D target = Target("target:after_move", iggy::InteractionTarget2DKind::Pickup, { 1.0F, 0.0F }, 0.0F);
	const iggy::LevelItemDrop2D drop = Drop("drop:after_move", "item:after_move", 1, { 1.0F, 0.0F }, 0.0F);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::ItemDefinition2DCatalog items = ItemCatalog({ Definition("item:after_move", "After Move", 1) });

	const iggy::runtime::RuntimePlayerInputInteractionPolicyPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPolicyPickupFrameStep {}.run(
			Input(
				InteractionInput(
					PlayerInput(SessionWithPlayer({ 0.0F, 0.0F }), {
						iggy::playerMoveToPointIntent({ 1.0F, 0.0F }),
						iggy::playerInteractIntent(target.id),
					}),
					InteractionState(Registry({ target }), EffectCatalog({ Entry("target:after_move", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }))),
				inventory,
				items));

	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "move-then-policy-pickup adapter should preserve post-move player position");
	Expect(result.pickup.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::PickedUp, "move-then-policy-pickup adapter should pick up after movement");
	if (!result.pickup.entries.empty())
		Expect(NearVec(result.pickup.entries[0].result.pickup.plan.actorPosition, { 1.0F, 0.0F }), "move-then-policy-pickup adapter should evaluate pickup from post-move position");
	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:after_move", 1) }), "move-then-policy-pickup adapter should update inventory");
}

void TestExplicitWorldOverloadAffectsMovementReachAndPickup()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	SetCollisionCache(session, BlockingWorld());
	const iggy::physics2d::CollisionWorld2D emptyWorld;
	const iggy::InteractionTarget2D target = Target("target:explicit", iggy::InteractionTarget2DKind::Pickup, { 1.0F, 0.0F }, 0.0F);
	const iggy::LevelItemDrop2D drop = Drop("drop:explicit", "item:explicit", 1, { 1.0F, 0.0F }, 0.0F);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::ItemDefinition2DCatalog items = ItemCatalog({ Definition("item:explicit", "Explicit", 1) });

	const iggy::runtime::RuntimePlayerInputInteractionPolicyPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPolicyPickupFrameStep {}.run(
			Input(
				InteractionInput(
					PlayerInput(session, {
						iggy::playerMoveToPointIntent({ 2.0F, 0.0F }),
						iggy::playerInteractIntent(target.id),
					}),
					InteractionState(Registry({ target }), EffectCatalog({ Entry("target:explicit", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }))),
				inventory,
				items),
			emptyWorld);

	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "explicit-world policy pickup adapter should preserve movement override result");
	Expect(result.pickup.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::PickedUp, "explicit-world policy pickup adapter should pick up after movement override");
	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:explicit", 1) }), "explicit-world policy pickup adapter should update inventory");
}

void TestOriginalInputsAreNotMutated()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::runtime::RuntimeSessionState sessionBefore = session;
	const iggy::InteractionTarget2D target = Target("target:immutable");
	const iggy::InteractionEffectCatalog2D effects =
		EffectCatalog({ Entry("target:immutable", { iggy::pickupItemInteractionEffect(target.id, Id("drop:potion")) }) });
	const iggy::runtime::RuntimeInteractionState interaction = InteractionState(Registry({ target }), effects);
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 1);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::runtime::RuntimeInventoryState inventoryBefore = inventory;
	const iggy::ItemDefinition2DCatalog items = ItemCatalog({ Definition("item:potion", "Potion", 5) });
	const iggy::runtime::RuntimePlayerInputInteractionPolicyPickupFrameInput input =
		Input(
			InteractionInput(PlayerInput(session, { iggy::playerInteractIntent(target.id) }), interaction),
			inventory,
			items);

	const iggy::runtime::RuntimePlayerInputInteractionPolicyPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPolicyPickupFrameStep {}.run(input);

	Expect(result.pickup.status == iggy::runtime::RuntimePolicyPickupEffectFrameStatus::PickedUp, "immutability policy pickup adapter setup should pick up");
	ExpectPlayerAgent(input.interactionInput.playerInput.commandInput.session.player, sessionBefore.player, "policy pickup adapter input session");
	ExpectRegistryTargets(input.interactionInput.interaction.targets, { target }, "policy pickup adapter should not mutate input interaction targets");
	ExpectCatalogPreserved(input.interactionInput.interaction.effects, effects, "policy pickup adapter should not mutate input interaction effects");
	ExpectInventoryState(input.inventory, inventoryBefore, "policy pickup adapter should not mutate input inventory");
}

} // namespace

int main()
{
	TestSuccessfulReachablePickupWithinMax();
	TestExistingStackExactlyToMaxSucceeds();
	TestOverCapacityFailsWithoutConsumingDrop();
	TestMissingItemDefinitionFailsWithoutConsumingDrop();
	TestToggleTargetAndPickupUpdateBothStates();
	TestContextBlockedInteractDoesNotConsumePickup();
	TestQueueRejectedMappedInteractDoesNotConsumePickup();
	TestMoveThenInteractUsesPostMovePosition();
	TestExplicitWorldOverloadAffectsMovementReachAndPickup();
	TestOriginalInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
