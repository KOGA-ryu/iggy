#include <cstdlib>
#include <vector>

#include "runtime/RuntimePlayerInputInteractionPickupFrameStep.hpp"
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

const iggy::ResourceId PlayerId { "player:input-interaction-pickup-frame" };

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
	session.level.map.id = iggy::ResourceId { "level:input-interaction-pickup-frame" };
	session.level.map.playerStart = { 0, 0 };
	session.tickIndex = 89;
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
	iggy::InteractionTarget2DKind kind = iggy::InteractionTarget2DKind::Usable,
	iggy::Vec2 position = { 0.0F, 0.0F },
	float radius = 0.0F,
	bool enabled = true)
{
	return { iggy::ResourceId { id }, kind, position, radius, enabled };
}

iggy::InteractionTarget2DRegistry Registry(std::vector<iggy::InteractionTarget2D> targets)
{
	const iggy::InteractionTarget2DRegistryBuildResult result = iggy::InteractionTarget2DRegistryBuilder {}.build(targets);
	Expect(result.built, "runtime player input interaction pickup registry fixture should build");
	return result.registry;
}

iggy::InteractionEffectEntry2D Entry(const char *targetId, std::vector<iggy::InteractionEffect2D> effects)
{
	return { iggy::ResourceId { targetId }, effects };
}

iggy::InteractionEffectCatalog2D Catalog(std::vector<iggy::InteractionEffectEntry2D> entries)
{
	const iggy::InteractionEffectCatalog2DBuildResult result = iggy::InteractionEffectCatalog2DBuilder {}.build(entries);
	Expect(result.built, "runtime player input interaction pickup catalog fixture should build");
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
	return { iggy::ResourceId { itemId }, count };
}

iggy::InventoryState2D Inventory(std::vector<iggy::InventoryItemStack2D> stacks)
{
	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);
	Expect(result.built, "runtime player input interaction pickup inventory fixture should build");
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
		iggy::ResourceId { id },
		iggy::ResourceId { itemId },
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

iggy::runtime::RuntimePlayerInputInteractionStateApplyFrameInput InteractionInput(
	iggy::runtime::RuntimePlayerInputGatedFrameStepInput playerInput,
	iggy::runtime::RuntimeInteractionState interaction,
	iggy::InteractionReach2DConfig reach = {})
{
	return { playerInput, interaction, reach, {}, {} };
}

iggy::runtime::RuntimePlayerInputInteractionPickupFrameInput Input(
	iggy::runtime::RuntimePlayerInputInteractionStateApplyFrameInput interactionInput,
	iggy::runtime::RuntimeInventoryState inventory,
	iggy::runtime::RuntimePickupConfig pickup = {})
{
	return { interactionInput, inventory, pickup };
}

iggy::physics2d::CollisionObject2D Object(iggy::ResourceId id, iggy::Aabb2 bounds)
{
	return { id, iggy::physics2d::makeAabbShape(bounds), true };
}

iggy::physics2d::CollisionWorld2D World(std::vector<iggy::physics2d::CollisionObject2D> objects)
{
	const iggy::physics2d::CollisionWorldBuildResult build = iggy::physics2d::CollisionWorld2DBuilder {}.build(objects);
	Expect(build.built, "runtime player input interaction pickup collision fixture should build");
	return build.world;
}

iggy::physics2d::CollisionWorld2D BlockingWorld()
{
	return World({
		Object(iggy::ResourceId { "wall:east" }, { { 0.5F, -0.5F }, { 1.5F, 0.5F } }),
	});
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

void ExpectNoInventoryEvents(
	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult &result,
	const char *message)
{
	Expect(result.inventoryEvents.events.empty(), message);
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

void ExpectSuccessfulPickupInventoryEventsAt(
	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult &result,
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

void TestNoPickupItemEffectsLeavesInventoryUnchanged()
{
	const iggy::InteractionTarget2D target = Target("target:toggle");
	std::vector<iggy::InteractionTarget2D> toggled { target };
	toggled[0].enabled = false;
	const iggy::InteractionEffectCatalog2D effects = Catalog({
		Entry("target:toggle", { iggy::toggleTargetInteractionEffect(target.id, false) }),
	});
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { Drop("drop:potion") });

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameStep {}.run(
			Input(
				InteractionInput(
					PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(target.id) }),
					InteractionState(Registry({ target }), effects)),
				inventory));

	Expect(result.interaction.application.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::Applied, "no-pickup adapter should still run interaction effects");
	ExpectRegistryTargets(result.interactionState.targets, toggled, "no-pickup adapter should return updated interaction state");
	Expect(result.pickup.status == iggy::runtime::RuntimePickupEffectFrameStatus::NoPickupEffects, "no-pickup adapter should report no pickup effects");
	Expect(result.pickup.entries.empty(), "no-pickup adapter should append no pickup entries");
	ExpectInventoryState(result.inventory, inventory, "no-pickup adapter should preserve inventory state");
	ExpectNoInventoryEvents(result, "no-pickup adapter should expose no inventory events");
}

void TestReachablePickupItemEffectTransfersItem()
{
	const iggy::InteractionTarget2D target = Target("target:pickup", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameStep {}.run(
			Input(
				InteractionInput(
					PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(target.id) }),
					InteractionState(
						Registry({ target }),
						Catalog({ Entry("target:pickup", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }))),
				inventory));

	Expect(result.interaction.application.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::NoOp, "pickup-only interaction application should be no-op target mutation");
	Expect(result.pickup.status == iggy::runtime::RuntimePickupEffectFrameStatus::PickedUp, "pickup adapter should pick up item");
	Expect(result.pickup.pickedUpCount == 1, "pickup adapter should count picked-up effect");
	Expect(result.pickup.changed, "pickup adapter should mark pickup changed");
	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:potion", 2) }), "pickup adapter should add item to inventory");
	Expect(result.inventory.drops.drops.size() == 1, "pickup adapter should keep disabled drop in default mode");
	if (result.inventory.drops.drops.size() == 1) {
		iggy::LevelItemDrop2D disabled = drop;
		disabled.enabled = false;
		Expect(SameDrop(result.inventory.drops.drops[0], disabled), "pickup adapter should disable consumed drop");
	}
	Expect(result.inventoryEvents.events.size() == 3, "pickup adapter should expose pickup inventory events");
	ExpectSuccessfulPickupInventoryEventsAt(result, 0, drop, "pickup adapter should preserve pickup inventory event order");
}

void TestExistingStackIncrementsAndRemoveModeRemovesDrop()
{
	const iggy::InteractionTarget2D target = Target("target:pickup_remove", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 3);
	const iggy::LevelItemDrop2D other = Drop("drop:key", "item:key", 1, { 1.0F, 0.0F }, 2.0F);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({ Stack("item:potion", 4) }, { drop, other });
	iggy::runtime::RuntimePickupConfig pickup;
	pickup.transfer.consumeMode = iggy::LevelItemDropConsume2DMode::Remove;

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameStep {}.run(
			Input(
				InteractionInput(
					PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(target.id) }),
					InteractionState(
						Registry({ target }),
						Catalog({ Entry("target:pickup_remove", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }))),
				inventory,
				pickup));

	Expect(result.pickup.status == iggy::runtime::RuntimePickupEffectFrameStatus::PickedUp, "remove-mode pickup adapter should pick up item");
	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:potion", 7) }), "remove-mode pickup adapter should increment existing stack");
	Expect(result.inventory.drops.drops.size() == 1, "remove-mode pickup adapter should remove consumed drop");
	if (result.inventory.drops.drops.size() == 1)
		Expect(SameDrop(result.inventory.drops.drops[0], other), "remove-mode pickup adapter should preserve remaining drop");
	Expect(result.inventoryEvents.events.size() == 3, "remove-mode pickup adapter should expose pickup inventory events");
	ExpectSuccessfulPickupInventoryEventsAt(result, 0, drop, "remove-mode pickup adapter should preserve requested pickup count in events");
}

void TestToggleAndPickupInSameInteractionUpdateBothStates()
{
	const std::vector<iggy::InteractionTarget2D> targets {
		Target("target:combo", iggy::InteractionTarget2DKind::Usable),
		Target("target:door", iggy::InteractionTarget2DKind::Door, { 0.0F, 0.0F }, 0.0F, true),
	};
	std::vector<iggy::InteractionTarget2D> expectedTargets = targets;
	expectedTargets[1].enabled = false;
	const iggy::LevelItemDrop2D drop = Drop("drop:key", "item:key", 1);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameStep {}.run(
			Input(
				InteractionInput(
					PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(targets[0].id) }),
					InteractionState(
						Registry(targets),
						Catalog({
							Entry("target:combo", {
								iggy::toggleTargetInteractionEffect(targets[1].id, false),
								iggy::pickupItemInteractionEffect(targets[0].id, drop.id),
							}),
						}))),
				inventory));

	Expect(result.interaction.application.status == iggy::runtime::RuntimeInteractionEffectApplyFrameStatus::Applied, "combo adapter should apply interaction target mutation");
	ExpectRegistryTargets(result.interactionState.targets, expectedTargets, "combo adapter should return updated interaction targets");
	Expect(result.pickup.status == iggy::runtime::RuntimePickupEffectFrameStatus::PickedUp, "combo adapter should pick up item");
	Expect(result.pickup.pickedUpCount == 1, "combo adapter should count pickup");
	Expect(result.pickup.entries.size() == 1, "combo adapter should append one pickup entry");
	if (result.pickup.entries.size() == 1)
		Expect(result.pickup.entries[0].effectIndex == 1, "combo adapter should preserve pickup effect index after toggle");
	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:key", 1) }), "combo adapter should update inventory");
	Expect(result.inventoryEvents.events.size() == 3, "combo adapter should expose pickup inventory events");
	ExpectSuccessfulPickupInventoryEventsAt(result, 0, drop, "combo adapter should preserve pickup inventory events");
}

void TestContextBlockedInteractDoesNotMutateEitherState()
{
	iggy::PlayerInputContext2D context;
	context.interactionEnabled = false;
	const iggy::InteractionTarget2D target = Target("target:blocked", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 1);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::InteractionEffectCatalog2D effects = Catalog({
		Entry("target:blocked", { iggy::pickupItemInteractionEffect(target.id, drop.id) }),
	});
	const iggy::runtime::RuntimeInteractionState interaction = InteractionState(Registry({ target }), effects);

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameStep {}.run(
			Input(
				InteractionInput(PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(target.id) }, context), interaction),
				inventory));

	Expect(result.interaction.playerInput.command.intake.mapping.gateIssues.size() == 1, "context-blocked pickup adapter should preserve gate issue");
	Expect(!result.interaction.application.hasInteractions(), "context-blocked pickup adapter should not apply interaction frame");
	Expect(result.pickup.status == iggy::runtime::RuntimePickupEffectFrameStatus::NoPickupEffects, "context-blocked pickup adapter should find no pickup effects");
	ExpectRegistryTargets(result.interactionState.targets, { target }, "context-blocked pickup adapter should preserve interaction targets");
	ExpectInventoryState(result.inventory, inventory, "context-blocked pickup adapter should preserve inventory");
	ExpectNoInventoryEvents(result, "context-blocked pickup adapter should expose no inventory events");
}

void TestQueueRejectedMappedInteractDoesNotConsumePickup()
{
	const iggy::InteractionTarget2D target = Target("target:queued", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 1);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::runtime::RuntimeCommandQueueState fullQueue { { WaitFrame() } };
	const iggy::InteractionEffectCatalog2D effects = Catalog({
		Entry("target:queued", { iggy::pickupItemInteractionEffect(target.id, drop.id) }),
	});

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameStep {}.run(
			Input(
				InteractionInput(
					PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(target.id) }, {}, fullQueue, { 1 }),
					InteractionState(Registry({ target }), effects)),
				inventory));

	Expect(result.interaction.playerInput.command.status == iggy::runtime::RuntimePlayerInputCommandRunnerStatus::QueueRejected, "queue-rejected pickup adapter should preserve queue rejection");
	Expect(result.interaction.playerInput.command.intake.mapping.frame.commands.size() == 1, "queue-rejected pickup adapter should preserve mapped frame diagnostics");
	Expect(!result.interaction.application.hasInteractions(), "queue-rejected pickup adapter should skip interaction application");
	Expect(result.pickup.status == iggy::runtime::RuntimePickupEffectFrameStatus::NoPickupEffects, "queue-rejected pickup adapter should consume no pickup effects");
	Expect(SameQueue(result.queue, fullQueue), "queue-rejected pickup adapter should preserve queue");
	ExpectInventoryState(result.inventory, inventory, "queue-rejected pickup adapter should preserve inventory");
	ExpectNoInventoryEvents(result, "queue-rejected pickup adapter should expose no inventory events");
}

void TestMissingDisabledAndOutOfRangeDropsMapThroughPickupResult()
{
	const iggy::InteractionTarget2D missingTarget = Target("target:missing_drop", iggy::InteractionTarget2DKind::Pickup);
	const iggy::InteractionTarget2D disabledTarget = Target("target:disabled_drop", iggy::InteractionTarget2DKind::Pickup);
	const iggy::InteractionTarget2D farTarget = Target("target:far_drop", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D disabled = Drop("drop:disabled", "item:disabled", 1, { 0.0F, 0.0F }, 1.0F, false);
	const iggy::LevelItemDrop2D far = Drop("drop:far", "item:far", 1, { 5.0F, 0.0F }, 1.0F, true);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { disabled, far });
	const iggy::runtime::RuntimeInteractionState interaction = InteractionState(
		Registry({ missingTarget, disabledTarget, farTarget }),
		Catalog({
			Entry("target:missing_drop", { iggy::pickupItemInteractionEffect(missingTarget.id, iggy::ResourceId { "drop:missing" }) }),
			Entry("target:disabled_drop", { iggy::pickupItemInteractionEffect(disabledTarget.id, disabled.id) }),
			Entry("target:far_drop", { iggy::pickupItemInteractionEffect(farTarget.id, far.id) }),
		}));

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult missingResult =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameStep {}.run(
			Input(InteractionInput(PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(missingTarget.id) }), interaction), inventory));
	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult disabledResult =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameStep {}.run(
			Input(InteractionInput(PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(disabledTarget.id) }), interaction), inventory));
	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult farResult =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameStep {}.run(
			Input(InteractionInput(PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(farTarget.id) }), interaction), inventory));

	Expect(missingResult.pickup.status == iggy::runtime::RuntimePickupEffectFrameStatus::PickupNotReady, "missing drop pickup adapter should report PickupNotReady");
	Expect(disabledResult.pickup.status == iggy::runtime::RuntimePickupEffectFrameStatus::PickupNotReady, "disabled drop pickup adapter should report PickupNotReady");
	Expect(farResult.pickup.status == iggy::runtime::RuntimePickupEffectFrameStatus::PickupNotReady, "out-of-range drop pickup adapter should report PickupNotReady");
	if (!missingResult.pickup.entries.empty())
		Expect(missingResult.pickup.entries[0].result.pickup.plan.status == iggy::PickupPlan2DStatus::DropNotFound, "missing drop pickup adapter should preserve drop-not-found plan");
	if (!disabledResult.pickup.entries.empty())
		Expect(disabledResult.pickup.entries[0].result.pickup.plan.status == iggy::PickupPlan2DStatus::DropDisabled, "disabled drop pickup adapter should preserve disabled plan");
	if (!farResult.pickup.entries.empty())
		Expect(farResult.pickup.entries[0].result.pickup.plan.status == iggy::PickupPlan2DStatus::OutOfRange, "out-of-range drop pickup adapter should preserve out-of-range plan");
	ExpectInventoryState(missingResult.inventory, inventory, "missing drop pickup adapter should preserve inventory");
	ExpectInventoryState(disabledResult.inventory, inventory, "disabled drop pickup adapter should preserve inventory");
	ExpectInventoryState(farResult.inventory, inventory, "out-of-range drop pickup adapter should preserve inventory");
	Expect(missingResult.inventoryEvents.events.size() == 1, "missing drop pickup adapter should expose not-ready event");
	if (missingResult.inventoryEvents.events.size() == 1)
		ExpectInventoryEvent(missingResult.inventoryEvents.events[0], iggy::InventoryEvent2DType::PickupNotReady, {}, iggy::ResourceId { "drop:missing" }, 0, "missing drop pickup adapter should preserve missing drop event");
	Expect(disabledResult.inventoryEvents.events.size() == 1, "disabled drop pickup adapter should expose not-ready event");
	if (disabledResult.inventoryEvents.events.size() == 1)
		ExpectInventoryEvent(disabledResult.inventoryEvents.events[0], iggy::InventoryEvent2DType::PickupNotReady, {}, disabled.id, 0, "disabled drop pickup adapter should preserve disabled drop event");
	Expect(farResult.inventoryEvents.events.size() == 1, "out-of-range drop pickup adapter should expose not-ready event");
	if (farResult.inventoryEvents.events.size() == 1)
		ExpectInventoryEvent(farResult.inventoryEvents.events[0], iggy::InventoryEvent2DType::PickupNotReady, {}, far.id, 0, "out-of-range drop pickup adapter should preserve far drop event");
}

void TestTransferFailureExposesInventoryFailureEvent()
{
	const iggy::InteractionTarget2D target = Target("target:invalid_drop", iggy::InteractionTarget2DKind::Pickup);
	const iggy::LevelItemDrop2D invalidDrop = Drop("drop:invalid", "", 2);
	iggy::LevelItemDrop2DRegistry drops;
	drops.drops = { invalidDrop };
	const iggy::runtime::RuntimeInventoryState inventory {
		{},
		drops,
	};

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameStep {}.run(
			Input(
				InteractionInput(
					PlayerInput(SessionWithPlayer(), { iggy::playerInteractIntent(target.id) }),
					InteractionState(
						Registry({ target }),
						Catalog({ Entry("target:invalid_drop", { iggy::pickupItemInteractionEffect(target.id, invalidDrop.id) }) }))),
				inventory));

	Expect(result.pickup.status == iggy::runtime::RuntimePickupEffectFrameStatus::Failed, "invalid drop pickup adapter should report failed pickup frame");
	Expect(result.pickup.failedCount == 1, "invalid drop pickup adapter should count failure");
	ExpectInventoryState(result.inventory, inventory, "invalid drop pickup adapter should preserve failed inventory state");
	Expect(result.inventoryEvents.events.size() == 1, "invalid drop pickup adapter should expose failure event");
	if (result.inventoryEvents.events.size() == 1)
		ExpectInventoryEvent(
			result.inventoryEvents.events[0],
			iggy::InventoryEvent2DType::InventoryAddFailed,
			{},
			{},
			2,
			"invalid drop pickup adapter should preserve inventory add failure event");
}

void TestMoveThenInteractCanMakePickupReachable()
{
	const iggy::InteractionTarget2D target = Target("target:after_move", iggy::InteractionTarget2DKind::Pickup, { 1.0F, 0.0F }, 0.0F);
	const iggy::LevelItemDrop2D drop = Drop("drop:after_move", "item:after_move", 1, { 1.0F, 0.0F }, 0.0F);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameStep {}.run(
			Input(
				InteractionInput(
					PlayerInput(SessionWithPlayer({ 0.0F, 0.0F }), {
						iggy::playerMoveToPointIntent({ 1.0F, 0.0F }),
						iggy::playerInteractIntent(target.id),
					}),
					InteractionState(
						Registry({ target }),
						Catalog({ Entry("target:after_move", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }))),
				inventory));

	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "move-then-pickup adapter should preserve post-move player position");
	Expect(result.pickup.status == iggy::runtime::RuntimePickupEffectFrameStatus::PickedUp, "move-then-pickup adapter should pick up after movement");
	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:after_move", 1) }), "move-then-pickup adapter should add moved-to item");
	if (!result.pickup.entries.empty())
		Expect(NearVec(result.pickup.entries[0].result.pickup.plan.actorPosition, { 1.0F, 0.0F }), "move-then-pickup adapter should evaluate pickup reach from post-move position");
	Expect(result.inventoryEvents.events.size() == 3, "move-then-pickup adapter should expose pickup inventory events");
	ExpectSuccessfulPickupInventoryEventsAt(result, 0, drop, "move-then-pickup adapter should preserve pickup events");
}

void TestExplicitWorldOverloadCanAffectMovementReachAndPickup()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	SetCollisionCache(session, BlockingWorld());
	const iggy::physics2d::CollisionWorld2D emptyWorld;
	const iggy::InteractionTarget2D target = Target("target:explicit", iggy::InteractionTarget2DKind::Pickup, { 1.0F, 0.0F }, 0.0F);
	const iggy::LevelItemDrop2D drop = Drop("drop:explicit", "item:explicit", 1, { 1.0F, 0.0F }, 0.0F);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameStep {}.run(
			Input(
				InteractionInput(
					PlayerInput(session, {
						iggy::playerMoveToPointIntent({ 2.0F, 0.0F }),
						iggy::playerInteractIntent(target.id),
					}),
					InteractionState(
						Registry({ target }),
						Catalog({ Entry("target:explicit", { iggy::pickupItemInteractionEffect(target.id, drop.id) }) }))),
				inventory),
			emptyWorld);

	Expect(NearVec(result.session.player.position, { 1.0F, 0.0F }), "explicit-world pickup adapter should preserve movement override result");
	Expect(result.pickup.status == iggy::runtime::RuntimePickupEffectFrameStatus::PickedUp, "explicit-world pickup adapter should pick up after movement override");
	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:explicit", 1) }), "explicit-world pickup adapter should add explicit-world item");
	Expect(result.inventoryEvents.events.size() == 3, "explicit-world pickup adapter should expose pickup inventory events");
	ExpectSuccessfulPickupInventoryEventsAt(result, 0, drop, "explicit-world pickup adapter should preserve pickup events");
}

void TestOriginalInputsAreNotMutated()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::runtime::RuntimeSessionState sessionBefore = session;
	const iggy::InteractionTarget2D target = Target("target:immutable", iggy::InteractionTarget2DKind::Pickup);
	const iggy::InteractionEffectCatalog2D effects = Catalog({
		Entry("target:immutable", { iggy::pickupItemInteractionEffect(target.id, iggy::ResourceId { "drop:potion" }) }),
	});
	const iggy::runtime::RuntimeInteractionState interaction = InteractionState(Registry({ target }), effects);
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 1);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::runtime::RuntimeInventoryState inventoryBefore = inventory;
	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameInput input =
		Input(
			InteractionInput(PlayerInput(session, { iggy::playerInteractIntent(target.id) }), interaction),
			inventory);

	const iggy::runtime::RuntimePlayerInputInteractionPickupFrameResult result =
		iggy::runtime::RuntimePlayerInputInteractionPickupFrameStep {}.run(input);

	Expect(result.pickup.status == iggy::runtime::RuntimePickupEffectFrameStatus::PickedUp, "immutability pickup adapter setup should pick up");
	ExpectPlayerAgent(input.interactionInput.playerInput.commandInput.session.player, sessionBefore.player, "pickup adapter input session");
	ExpectRegistryTargets(input.interactionInput.interaction.targets, { target }, "pickup adapter should not mutate input interaction targets");
	ExpectCatalogPreserved(input.interactionInput.interaction.effects, effects, "pickup adapter should not mutate input interaction effects");
	ExpectInventoryState(input.inventory, inventoryBefore, "pickup adapter should not mutate input inventory state");
}

} // namespace

int main()
{
	TestNoPickupItemEffectsLeavesInventoryUnchanged();
	TestReachablePickupItemEffectTransfersItem();
	TestExistingStackIncrementsAndRemoveModeRemovesDrop();
	TestToggleAndPickupInSameInteractionUpdateBothStates();
	TestContextBlockedInteractDoesNotMutateEitherState();
	TestQueueRejectedMappedInteractDoesNotConsumePickup();
	TestMissingDisabledAndOutOfRangeDropsMapThroughPickupResult();
	TestTransferFailureExposesInventoryFailureEvent();
	TestMoveThenInteractCanMakePickupReachable();
	TestExplicitWorldOverloadCanAffectMovementReachAndPickup();
	TestOriginalInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
