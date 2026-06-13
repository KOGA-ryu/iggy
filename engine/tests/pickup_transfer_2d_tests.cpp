#include <cstdlib>
#include <vector>

#include "scene/inventory/PickupTransfer2D.hpp"
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
	Expect(result.built, "pickup transfer inventory fixture should build");
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

iggy::LevelItemDrop2DRegistry Registry(std::vector<iggy::LevelItemDrop2D> drops)
{
	return { drops };
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

iggy::PickupPlan2DResult ReadyPlan(const iggy::LevelItemDrop2D &drop)
{
	iggy::PickupPlan2DResult plan;
	plan.status = iggy::PickupPlan2DStatus::Ready;
	plan.dropId = drop.id;
	plan.actorPosition = drop.position;
	plan.drop = drop;
	return plan;
}

void ExpectNoEvents(const iggy::PickupTransfer2DResult &result, const char *message)
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

void ExpectSuccessfulTransferEvents(
	const iggy::PickupTransfer2DResult &result,
	const iggy::LevelItemDrop2D &drop,
	const char *message)
{
	Expect(result.events.events.size() == 3, message);
	if (result.events.events.size() == 3) {
		ExpectEvent(
			result.events.events[0],
			iggy::InventoryEvent2DType::ItemAdded,
			drop.itemId,
			{},
			drop.count,
			message);
		ExpectEvent(
			result.events.events[1],
			iggy::InventoryEvent2DType::DropConsumed,
			{},
			drop.id,
			0,
			message);
		ExpectEvent(
			result.events.events[2],
			iggy::InventoryEvent2DType::ItemPickedUp,
			drop.itemId,
			drop.id,
			drop.count,
			message);
	}
}

void TestNonReadyPlanDoesNotAddOrConsume()
{
	const iggy::InventoryState2D inventory = Inventory({ Stack("item:potion", 1) });
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::LevelItemDrop2DRegistry drops = Registry({ drop });
	const iggy::PickupPlan2DResult plan = iggy::PickupPlan2D {}.plan(drops, drop.id, { 5.0F, 0.0F });

	const iggy::PickupTransfer2DResult result = iggy::PickupTransfer2D {}.transfer(inventory, drops, plan);

	Expect(result.status == iggy::PickupTransfer2DStatus::PickupNotReady, "non-ready pickup should not transfer");
	Expect(!result.changed, "non-ready pickup should not mark changed");
	Expect(result.plan.status == plan.status && result.plan.dropId == plan.dropId, "non-ready pickup should preserve plan");
	Expect(SameStacks(result.inventory.stacks, inventory.stacks), "non-ready pickup should preserve inventory");
	Expect(SameDrops(result.drops.drops, drops.drops), "non-ready pickup should preserve drops");
	Expect(result.add.status == iggy::InventoryAddItem2DStatus::InvalidItemId, "non-ready pickup should not run inventory add");
	Expect(result.consume.status == iggy::LevelItemDropConsume2DStatus::MissingDropId, "non-ready pickup should not run drop consume");
	ExpectNoEvents(result, "non-ready pickup should not record inventory events");
}

void TestReadyPlanTransfersToEmptyInventoryAndDisablesDropByDefault()
{
	const iggy::InventoryState2D inventory;
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2, { 1.0F, 2.0F }, 0.0F, true);
	const iggy::LevelItemDrop2DRegistry drops = Registry({ drop });

	const iggy::PickupTransfer2DResult result = iggy::PickupTransfer2D {}.transfer(inventory, drops, ReadyPlan(drop));

	Expect(result.status == iggy::PickupTransfer2DStatus::Transferred, "ready pickup should transfer");
	Expect(result.changed, "ready pickup should mark changed");
	Expect(result.add.status == iggy::InventoryAddItem2DStatus::Added, "ready pickup should add inventory item");
	Expect(result.consume.status == iggy::LevelItemDropConsume2DStatus::Consumed, "ready pickup should consume drop");
	Expect(result.consume.mode == iggy::LevelItemDropConsume2DMode::Disable, "ready pickup should default to disable consume mode");
	Expect(SameStacks(result.inventory.stacks, { Stack("item:potion", 2) }), "ready pickup should add stack to empty inventory");
	Expect(result.drops.drops.size() == 1, "disable transfer should preserve drop count");
	if (result.drops.drops.size() == 1) {
		iggy::LevelItemDrop2D disabled = drop;
		disabled.enabled = false;
		Expect(SameDrop(result.drops.drops[0], disabled), "default transfer should disable drop");
	}
	ExpectSuccessfulTransferEvents(result, drop, "default transfer should record add, consume, and pickup events in order");
}

void TestReadyPlanTransfersToExistingStack()
{
	const iggy::InventoryState2D inventory = Inventory({ Stack("item:potion", 3), Stack("item:key", 1) });
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 4);
	const iggy::LevelItemDrop2DRegistry drops = Registry({ drop });

	const iggy::PickupTransfer2DResult result = iggy::PickupTransfer2D {}.transfer(inventory, drops, ReadyPlan(drop));

	Expect(result.status == iggy::PickupTransfer2DStatus::Transferred, "existing stack pickup should transfer");
	Expect(SameStacks(
			   result.inventory.stacks,
			   {
				   Stack("item:potion", 7),
				   Stack("item:key", 1),
			   }),
		"existing stack pickup should increment count and preserve order");
	ExpectSuccessfulTransferEvents(result, drop, "existing stack pickup should record transfer events with requested count");
}

void TestRemoveModeRemovesDrop()
{
	const iggy::InventoryState2D inventory;
	const iggy::LevelItemDrop2D first = Drop("drop:potion", "item:potion", 1);
	const iggy::LevelItemDrop2D second = Drop("drop:key", "item:key", 1);
	const iggy::LevelItemDrop2DRegistry drops = Registry({ first, second });

	const iggy::PickupTransfer2DResult result = iggy::PickupTransfer2D {}.transfer(
		inventory,
		drops,
		ReadyPlan(first),
		{ iggy::LevelItemDropConsume2DMode::Remove });

	Expect(result.status == iggy::PickupTransfer2DStatus::Transferred, "remove mode pickup should transfer");
	Expect(result.consume.mode == iggy::LevelItemDropConsume2DMode::Remove, "remove mode pickup should preserve consume mode");
	Expect(result.drops.drops.size() == 1, "remove mode pickup should remove consumed drop");
	if (result.drops.drops.size() == 1)
		Expect(SameDrop(result.drops.drops[0], second), "remove mode pickup should preserve remaining drops");
	ExpectSuccessfulTransferEvents(result, first, "remove mode pickup should record add, consume, and pickup events in order");
}

void TestInventoryAddFailureDoesNotConsumeDrop()
{
	const iggy::InventoryState2D inventory;
	const iggy::LevelItemDrop2D drop = Drop("drop:bad_item", "", 2);
	const iggy::LevelItemDrop2DRegistry drops = Registry({ drop });

	const iggy::PickupTransfer2DResult result = iggy::PickupTransfer2D {}.transfer(inventory, drops, ReadyPlan(drop));

	Expect(result.status == iggy::PickupTransfer2DStatus::InventoryAddFailed, "invalid pickup item should fail add");
	Expect(!result.changed, "invalid pickup item should not mark transfer changed");
	Expect(result.add.status == iggy::InventoryAddItem2DStatus::InvalidItemId, "invalid pickup item should preserve add failure");
	Expect(result.consume.status == iggy::LevelItemDropConsume2DStatus::MissingDropId, "invalid pickup item should not consume drop");
	Expect(SameStacks(result.inventory.stacks, inventory.stacks), "invalid pickup item should preserve original inventory");
	Expect(SameDrops(result.drops.drops, drops.drops), "invalid pickup item should preserve original drops");
	Expect(result.events.events.size() == 1, "invalid pickup item should record one add-failed event");
	if (result.events.events.size() == 1) {
		ExpectEvent(
			result.events.events[0],
			iggy::InventoryEvent2DType::InventoryAddFailed,
			drop.itemId,
			{},
			drop.count,
			"invalid pickup item should record InventoryAddFailed event payload");
	}
}

void TestDropConsumeFailurePreservesAddedInventoryAndFailureDiagnostics()
{
	const iggy::InventoryState2D inventory;
	const iggy::LevelItemDrop2D plannedDrop = Drop("drop:missing", "item:potion", 2);
	const iggy::LevelItemDrop2DRegistry drops = Registry({ Drop("drop:other", "item:key", 1) });

	const iggy::PickupTransfer2DResult result = iggy::PickupTransfer2D {}.transfer(inventory, drops, ReadyPlan(plannedDrop));

	Expect(result.status == iggy::PickupTransfer2DStatus::DropConsumeFailed, "missing planned drop should fail consume after add");
	Expect(!result.changed, "consume failure should not mark transfer complete");
	Expect(result.add.status == iggy::InventoryAddItem2DStatus::Added, "consume failure should preserve successful add result");
	Expect(result.consume.status == iggy::LevelItemDropConsume2DStatus::DropNotFound, "consume failure should preserve consume diagnostics");
	Expect(SameStacks(result.inventory.stacks, { Stack("item:potion", 2) }), "consume failure should expose partial added inventory");
	Expect(SameDrops(result.drops.drops, drops.drops), "consume failure should preserve consume/original drops");
	Expect(result.events.events.size() == 1, "consume failure should preserve add event only");
	if (result.events.events.size() == 1) {
		ExpectEvent(
			result.events.events[0],
			iggy::InventoryEvent2DType::ItemAdded,
			plannedDrop.itemId,
			{},
			plannedDrop.count,
			"consume failure should preserve ItemAdded event only");
	}
}

void TestOriginalInventoryAndDropsAreNotMutated()
{
	iggy::InventoryState2D inventory = Inventory({ Stack("item:potion", 3) });
	const iggy::InventoryState2D inventoryBefore = inventory;
	iggy::LevelItemDrop2DRegistry drops = Registry({ Drop("drop:potion", "item:potion", 2) });
	const iggy::LevelItemDrop2DRegistry dropsBefore = drops;

	const iggy::PickupTransfer2DResult result = iggy::PickupTransfer2D {}.transfer(inventory, drops, ReadyPlan(drops.drops[0]));

	Expect(result.status == iggy::PickupTransfer2DStatus::Transferred, "immutability pickup setup should transfer");
	Expect(SameStacks(inventory.stacks, inventoryBefore.stacks), "pickup transfer should not mutate original inventory");
	Expect(SameDrops(drops.drops, dropsBefore.drops), "pickup transfer should not mutate original drops");
}

void TestNamespacedItemAndDropIdsArePreserved()
{
	const iggy::InventoryState2D inventory = Inventory({ Stack("potion", 1) });
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::LevelItemDrop2DRegistry drops = Registry({ Drop("potion", "item:other", 1), drop });

	const iggy::PickupTransfer2DResult result = iggy::PickupTransfer2D {}.transfer(
		inventory,
		drops,
		ReadyPlan(drop),
		{ iggy::LevelItemDropConsume2DMode::Remove });

	Expect(result.status == iggy::PickupTransfer2DStatus::Transferred, "namespaced pickup should transfer");
	Expect(SameStacks(
			   result.inventory.stacks,
			   {
				   Stack("potion", 1),
				   Stack("item:potion", 2),
			   }),
		"namespaced pickup item should append separately from unqualified stack");
	Expect(result.drops.drops.size() == 1, "namespaced pickup should remove only matching namespaced drop");
	if (result.drops.drops.size() == 1)
		Expect(result.drops.drops[0].id == iggy::ResourceId { "potion" }, "unqualified drop should remain distinct");
}

} // namespace

int main()
{
	TestNonReadyPlanDoesNotAddOrConsume();
	TestReadyPlanTransfersToEmptyInventoryAndDisablesDropByDefault();
	TestReadyPlanTransfersToExistingStack();
	TestRemoveModeRemovesDrop();
	TestInventoryAddFailureDoesNotConsumeDrop();
	TestDropConsumeFailurePreservesAddedInventoryAndFailureDiagnostics();
	TestOriginalInventoryAndDropsAreNotMutated();
	TestNamespacedItemAndDropIdsArePreserved();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
