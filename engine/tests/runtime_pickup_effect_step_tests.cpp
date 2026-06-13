#include <cstdlib>
#include <vector>

#include "runtime/RuntimePickupEffectStep.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::InventoryItemStack2D Stack(const char *itemId, std::uint32_t count)
{
	return { iggy::ResourceId { itemId }, count };
}

iggy::InventoryState2D Inventory(std::vector<iggy::InventoryItemStack2D> stacks)
{
	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);
	Expect(result.built, "runtime pickup effect inventory fixture should build");
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

iggy::LevelItemDrop2DRegistry Drops(std::vector<iggy::LevelItemDrop2D> drops)
{
	return { drops };
}

iggy::runtime::RuntimeInventoryState InventoryState(
	std::vector<iggy::InventoryItemStack2D> stacks,
	std::vector<iggy::LevelItemDrop2D> drops)
{
	return {
		Inventory(stacks),
		Drops(drops),
	};
}

iggy::runtime::RuntimeSessionState SessionWithPlayer(iggy::Vec2 position = { 0.0F, 0.0F })
{
	iggy::runtime::RuntimeSessionState session;
	session.hasPlayer = true;
	session.player = {
		iggy::ResourceId { "player:pickup-effect" },
		position,
		{ 0, 0 },
		iggy::PlayerMovementStatus::Idle,
		iggy::PlayerFacing2D::South,
	};
	session.tickIndex = 13;
	return session;
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

bool SameEffect(const iggy::InteractionEffect2D &actual, const iggy::InteractionEffect2D &expected)
{
	return actual.type == expected.type
		&& actual.targetId == expected.targetId
		&& actual.eventId == expected.eventId
		&& actual.dropId == expected.dropId
		&& actual.text == expected.text
		&& actual.enabledValue == expected.enabledValue;
}

void ExpectInventoryState(
	const iggy::runtime::RuntimeInventoryState &actual,
	const iggy::runtime::RuntimeInventoryState &expected,
	const char *message)
{
	Expect(SameStacks(actual.inventory.stacks, expected.inventory.stacks), message);
	Expect(SameDrops(actual.drops.drops, expected.drops.drops), message);
}

void ExpectNoEvents(const iggy::runtime::RuntimePickupEffectResult &result, const char *message)
{
	Expect(result.events.events.empty(), message);
}

void ExpectEvent(
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

void ExpectPickupNotReadyEvent(
	const iggy::runtime::RuntimePickupEffectResult &result,
	const iggy::ResourceId &dropId,
	const char *message)
{
	Expect(result.events.events.size() == 1, message);
	if (result.events.events.size() == 1)
		ExpectEvent(result.events.events[0], iggy::InventoryEvent2DType::PickupNotReady, {}, dropId, 0, message);
}

void ExpectSuccessfulPickupEvents(
	const iggy::runtime::RuntimePickupEffectResult &result,
	const iggy::LevelItemDrop2D &drop,
	const char *message)
{
	Expect(result.events.events.size() == 3, message);
	if (result.events.events.size() == 3) {
		ExpectEvent(result.events.events[0], iggy::InventoryEvent2DType::ItemAdded, drop.itemId, {}, drop.count, message);
		ExpectEvent(result.events.events[1], iggy::InventoryEvent2DType::DropConsumed, {}, drop.id, 0, message);
		ExpectEvent(result.events.events[2], iggy::InventoryEvent2DType::ItemPickedUp, drop.itemId, drop.id, drop.count, message);
	}
}

void TestNonPickupEffectReturnsNotPickupEffectWithoutMutation()
{
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { Drop("drop:potion") });
	const iggy::InteractionEffect2D effect = iggy::inspectTextInteractionEffect(iggy::ResourceId { "target:sign" }, "Read me");

	const iggy::runtime::RuntimePickupEffectResult result =
		iggy::runtime::RuntimePickupEffectStep {}.apply(SessionWithPlayer(), inventory, effect);

	Expect(result.status == iggy::runtime::RuntimePickupEffectStatus::NotPickupEffect, "non-pickup effect should report NotPickupEffect");
	Expect(result.effectStatus == iggy::InteractionEffect2DStatus::Valid, "non-pickup effect should preserve valid effect status");
	Expect(SameEffect(result.effect, effect), "non-pickup effect should preserve effect payload");
	Expect(!result.changed, "non-pickup effect should not mark changed");
	ExpectInventoryState(result.inventory, inventory, "non-pickup effect should preserve inventory state");
	Expect(result.pickup.status == iggy::runtime::RuntimePickupStatus::PickupNotReady, "non-pickup effect should not run pickup");
	ExpectNoEvents(result, "non-pickup effect should not record inventory events");
}

void TestInvalidPickupEffectReturnsInvalidEffectWithoutPickup()
{
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { Drop("drop:potion") });
	const iggy::InteractionEffect2D effect = iggy::pickupItemInteractionEffect(iggy::ResourceId { "target:chest" }, {});

	const iggy::runtime::RuntimePickupEffectResult result =
		iggy::runtime::RuntimePickupEffectStep {}.apply(SessionWithPlayer(), inventory, effect);

	Expect(result.status == iggy::runtime::RuntimePickupEffectStatus::InvalidEffect, "invalid pickup effect should report InvalidEffect");
	Expect(result.effectStatus == iggy::InteractionEffect2DStatus::MissingDropId, "invalid pickup effect should preserve validation status");
	Expect(SameEffect(result.effect, effect), "invalid pickup effect should preserve effect payload");
	Expect(!result.changed, "invalid pickup effect should not mark changed");
	ExpectInventoryState(result.inventory, inventory, "invalid pickup effect should preserve inventory state");
	Expect(result.pickup.status == iggy::runtime::RuntimePickupStatus::PickupNotReady, "invalid pickup effect should not run pickup");
	ExpectNoEvents(result, "invalid pickup effect should not record inventory events");
}

void TestMissingPlayerMapsToMissingPlayer()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	session.hasPlayer = false;
	session.player = {};
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { Drop("drop:potion") });
	const iggy::InteractionEffect2D effect =
		iggy::pickupItemInteractionEffect(iggy::ResourceId { "target:chest" }, iggy::ResourceId { "drop:potion" });

	const iggy::runtime::RuntimePickupEffectResult result =
		iggy::runtime::RuntimePickupEffectStep {}.apply(session, inventory, effect);

	Expect(result.status == iggy::runtime::RuntimePickupEffectStatus::MissingPlayer, "missing-player pickup effect should report MissingPlayer");
	Expect(result.pickup.status == iggy::runtime::RuntimePickupStatus::MissingPlayer, "missing-player pickup effect should preserve pickup status");
	Expect(!result.changed, "missing-player pickup effect should not mark changed");
	ExpectInventoryState(result.inventory, inventory, "missing-player pickup effect should preserve inventory state");
	ExpectNoEvents(result, "missing-player pickup effect should preserve empty pickup events");
}

void TestNotReadyDropsMapToPickupNotReady()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::runtime::RuntimeInventoryState missingInventory = InventoryState({}, {});
	const iggy::runtime::RuntimeInventoryState disabledInventory =
		InventoryState({}, { Drop("drop:disabled", "item:potion", 1, { 0.0F, 0.0F }, 1.0F, false) });
	const iggy::runtime::RuntimeInventoryState outOfRangeInventory =
		InventoryState({}, { Drop("drop:far", "item:potion", 1, { 5.0F, 0.0F }, 1.0F, true) });

	const iggy::runtime::RuntimePickupEffectResult missingResult =
		iggy::runtime::RuntimePickupEffectStep {}.apply(
			session,
			missingInventory,
			iggy::pickupItemInteractionEffect(iggy::ResourceId { "target:chest" }, iggy::ResourceId { "drop:missing" }));
	const iggy::runtime::RuntimePickupEffectResult disabledResult =
		iggy::runtime::RuntimePickupEffectStep {}.apply(
			session,
			disabledInventory,
			iggy::pickupItemInteractionEffect(iggy::ResourceId { "target:chest" }, iggy::ResourceId { "drop:disabled" }));
	const iggy::runtime::RuntimePickupEffectResult outOfRangeResult =
		iggy::runtime::RuntimePickupEffectStep {}.apply(
			session,
			outOfRangeInventory,
			iggy::pickupItemInteractionEffect(iggy::ResourceId { "target:chest" }, iggy::ResourceId { "drop:far" }));

	Expect(missingResult.status == iggy::runtime::RuntimePickupEffectStatus::PickupNotReady, "missing drop pickup effect should not be ready");
	Expect(missingResult.pickup.plan.status == iggy::PickupPlan2DStatus::DropNotFound, "missing drop pickup effect should preserve plan");
	Expect(disabledResult.status == iggy::runtime::RuntimePickupEffectStatus::PickupNotReady, "disabled drop pickup effect should not be ready");
	Expect(disabledResult.pickup.plan.status == iggy::PickupPlan2DStatus::DropDisabled, "disabled drop pickup effect should preserve plan");
	Expect(outOfRangeResult.status == iggy::runtime::RuntimePickupEffectStatus::PickupNotReady, "out-of-range drop pickup effect should not be ready");
	Expect(outOfRangeResult.pickup.plan.status == iggy::PickupPlan2DStatus::OutOfRange, "out-of-range pickup effect should preserve plan");
	ExpectInventoryState(missingResult.inventory, missingInventory, "missing drop pickup effect should preserve inventory");
	ExpectInventoryState(disabledResult.inventory, disabledInventory, "disabled drop pickup effect should preserve inventory");
	ExpectInventoryState(outOfRangeResult.inventory, outOfRangeInventory, "out-of-range pickup effect should preserve inventory");
	ExpectPickupNotReadyEvent(missingResult, iggy::ResourceId { "drop:missing" }, "missing drop pickup effect should record PickupNotReady event");
	ExpectPickupNotReadyEvent(disabledResult, iggy::ResourceId { "drop:disabled" }, "disabled drop pickup effect should record PickupNotReady event");
	ExpectPickupNotReadyEvent(outOfRangeResult, iggy::ResourceId { "drop:far" }, "out-of-range pickup effect should record PickupNotReady event");
}

void TestReadyPickupTransfersItemAndDisablesDrop()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2, { 0.0F, 0.0F }, 0.0F, true);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::InteractionEffect2D effect =
		iggy::pickupItemInteractionEffect(iggy::ResourceId { "target:chest" }, drop.id);

	const iggy::runtime::RuntimePickupEffectResult result =
		iggy::runtime::RuntimePickupEffectStep {}.apply(SessionWithPlayer(), inventory, effect);

	Expect(result.status == iggy::runtime::RuntimePickupEffectStatus::PickedUp, "ready pickup effect should pick up");
	Expect(result.effectStatus == iggy::InteractionEffect2DStatus::Valid, "ready pickup effect should preserve valid status");
	Expect(result.pickup.status == iggy::runtime::RuntimePickupStatus::PickedUp, "ready pickup effect should preserve pickup result");
	Expect(result.changed, "ready pickup effect should mark changed");
	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:potion", 2) }), "ready pickup effect should add item stack");
	Expect(result.inventory.drops.drops.size() == 1, "ready pickup effect disable mode should keep drop");
	if (result.inventory.drops.drops.size() == 1) {
		iggy::LevelItemDrop2D disabled = drop;
		disabled.enabled = false;
		Expect(SameDrop(result.inventory.drops.drops[0], disabled), "ready pickup effect should disable drop");
	}
	ExpectSuccessfulPickupEvents(result, drop, "ready pickup effect should propagate pickup inventory events");
}

void TestReadyPickupExistingStackIncrementsAndRemoveModeRemovesDrop()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 3, { 0.0F, 0.0F }, 0.0F, true);
	const iggy::LevelItemDrop2D other = Drop("drop:key", "item:key", 1, { 1.0F, 0.0F }, 2.0F, true);
	const iggy::runtime::RuntimeInventoryState inventory =
		InventoryState({ Stack("item:potion", 4) }, { drop, other });
	iggy::runtime::RuntimePickupConfig config;
	config.transfer.consumeMode = iggy::LevelItemDropConsume2DMode::Remove;

	const iggy::runtime::RuntimePickupEffectResult result =
		iggy::runtime::RuntimePickupEffectStep {}.apply(
			SessionWithPlayer(),
			inventory,
			iggy::pickupItemInteractionEffect(iggy::ResourceId { "target:chest" }, drop.id),
			config);

	Expect(result.status == iggy::runtime::RuntimePickupEffectStatus::PickedUp, "remove mode pickup effect should pick up");
	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:potion", 7) }), "remove mode pickup effect should increment existing stack");
	Expect(result.inventory.drops.drops.size() == 1, "remove mode pickup effect should remove consumed drop");
	if (result.inventory.drops.drops.size() == 1)
		Expect(SameDrop(result.inventory.drops.drops[0], other), "remove mode pickup effect should preserve remaining drop");
	ExpectSuccessfulPickupEvents(result, drop, "remove mode pickup effect should propagate pickup inventory events");
}

void TestTransferFailedPreservesDiagnostics()
{
	const iggy::LevelItemDrop2D invalidDrop = Drop("drop:bad", "", 2, { 0.0F, 0.0F }, 0.0F, true);
	iggy::LevelItemDrop2DRegistry drops;
	drops.drops = { invalidDrop };
	const iggy::runtime::RuntimeInventoryState inventory {
		{},
		drops,
	};

	const iggy::runtime::RuntimePickupEffectResult result =
		iggy::runtime::RuntimePickupEffectStep {}.apply(
			SessionWithPlayer(),
			inventory,
			iggy::pickupItemInteractionEffect(iggy::ResourceId { "target:chest" }, invalidDrop.id));

	Expect(result.status == iggy::runtime::RuntimePickupEffectStatus::TransferFailed, "invalid ready drop pickup effect should report TransferFailed");
	Expect(result.pickup.status == iggy::runtime::RuntimePickupStatus::TransferFailed, "invalid ready drop pickup effect should preserve pickup failure");
	Expect(result.pickup.transfer.status == iggy::PickupTransfer2DStatus::InventoryAddFailed, "invalid ready drop pickup effect should preserve transfer diagnostics");
	Expect(!result.changed, "invalid ready drop pickup effect should not mark changed");
	ExpectInventoryState(result.inventory, inventory, "invalid ready drop pickup effect should preserve returned inventory state");
	Expect(result.events.events.size() == 1, "invalid ready drop pickup effect should preserve transfer failure event");
	if (result.events.events.size() == 1)
		ExpectEvent(
			result.events.events[0],
			iggy::InventoryEvent2DType::InventoryAddFailed,
			{},
			{},
			2,
			"invalid ready drop pickup effect should record InventoryAddFailed event");
}

void TestOriginalInputsAreNotMutated()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::runtime::RuntimeSessionState sessionBefore = session;
	iggy::runtime::RuntimeInventoryState inventory = InventoryState(
		{ Stack("item:potion", 1) },
		{ Drop("drop:potion", "item:potion", 2) });
	const iggy::runtime::RuntimeInventoryState inventoryBefore = inventory;
	iggy::InteractionEffect2D effect =
		iggy::pickupItemInteractionEffect(iggy::ResourceId { "target:chest" }, iggy::ResourceId { "drop:potion" });
	const iggy::InteractionEffect2D effectBefore = effect;

	const iggy::runtime::RuntimePickupEffectResult result =
		iggy::runtime::RuntimePickupEffectStep {}.apply(session, inventory, effect);

	Expect(result.status == iggy::runtime::RuntimePickupEffectStatus::PickedUp, "immutability pickup effect setup should pick up");
	Expect(session.hasPlayer == sessionBefore.hasPlayer, "pickup effect should not mutate session player presence");
	Expect(NearVec(session.player.position, sessionBefore.player.position), "pickup effect should not mutate session player position");
	Expect(session.tickIndex == sessionBefore.tickIndex, "pickup effect should not mutate session tick");
	ExpectInventoryState(inventory, inventoryBefore, "pickup effect should not mutate input inventory state");
	Expect(SameEffect(effect, effectBefore), "pickup effect should not mutate input effect");
}

void TestNamespacedDropAndItemIdsArePreserved()
{
	const iggy::LevelItemDrop2D unqualified = Drop("potion", "item:other", 1, { 1.0F, 0.0F }, 2.0F, true);
	const iggy::LevelItemDrop2D namespaced = Drop("drop:potion", "item:potion", 2, { 0.0F, 0.0F }, 0.0F, true);
	const iggy::runtime::RuntimeInventoryState inventory =
		InventoryState({ Stack("potion", 1) }, { unqualified, namespaced });
	iggy::runtime::RuntimePickupConfig config;
	config.transfer.consumeMode = iggy::LevelItemDropConsume2DMode::Remove;

	const iggy::runtime::RuntimePickupEffectResult result =
		iggy::runtime::RuntimePickupEffectStep {}.apply(
			SessionWithPlayer(),
			inventory,
			iggy::pickupItemInteractionEffect(iggy::ResourceId { "target:chest" }, namespaced.id),
			config);

	Expect(result.status == iggy::runtime::RuntimePickupEffectStatus::PickedUp, "namespaced pickup effect should pick up");
	Expect(SameStacks(
			   result.inventory.inventory.stacks,
			   {
				   Stack("potion", 1),
				   Stack("item:potion", 2),
			   }),
		"namespaced pickup effect should preserve distinct item ids");
	Expect(result.inventory.drops.drops.size() == 1, "namespaced pickup effect should remove only requested drop");
	if (result.inventory.drops.drops.size() == 1)
		Expect(result.inventory.drops.drops[0].id == iggy::ResourceId { "potion" }, "unqualified drop should remain distinct");
}

} // namespace

int main()
{
	TestNonPickupEffectReturnsNotPickupEffectWithoutMutation();
	TestInvalidPickupEffectReturnsInvalidEffectWithoutPickup();
	TestMissingPlayerMapsToMissingPlayer();
	TestNotReadyDropsMapToPickupNotReady();
	TestReadyPickupTransfersItemAndDisablesDrop();
	TestReadyPickupExistingStackIncrementsAndRemoveModeRemovesDrop();
	TestTransferFailedPreservesDiagnostics();
	TestOriginalInputsAreNotMutated();
	TestNamespacedDropAndItemIdsArePreserved();

	return Failures;
}
