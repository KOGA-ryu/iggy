#include <cstdlib>
#include <vector>

#include "scene/inventory/PickupPolicyTransfer2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::InventoryItemStack2D Stack(const char *itemId, std::uint32_t count)
{
	return { Id(itemId), count };
}

iggy::InventoryState2D Inventory(std::vector<iggy::InventoryItemStack2D> stacks)
{
	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);
	Expect(result.built, "pickup policy transfer inventory fixture should build");
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

iggy::LevelItemDrop2DRegistry Registry(std::vector<iggy::LevelItemDrop2D> drops)
{
	return { drops };
}

iggy::ItemDefinition2D Definition(
	const char *itemId,
	const char *displayName = "Potion",
	std::uint32_t maxStackCount = 10,
	iggy::ItemDefinition2DKind kind = iggy::ItemDefinition2DKind::Consumable)
{
	return {
		Id(itemId),
		displayName,
		maxStackCount,
		kind,
	};
}

iggy::ItemDefinition2DCatalog Catalog(std::vector<iggy::ItemDefinition2D> definitions)
{
	const iggy::ItemDefinition2DCatalogBuildResult result =
		iggy::ItemDefinition2DCatalogBuilder {}.build(definitions);
	Expect(result.built, "pickup policy transfer catalog fixture should build");
	return result.catalog;
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

bool SameDefinitions(
	const std::vector<iggy::ItemDefinition2D> &actual,
	const std::vector<iggy::ItemDefinition2D> &expected)
{
	if (actual.size() != expected.size())
		return false;
	for (std::size_t index = 0; index < actual.size(); ++index) {
		if (actual[index].itemId != expected[index].itemId
			|| actual[index].displayName != expected[index].displayName
			|| actual[index].maxStackCount != expected[index].maxStackCount
			|| actual[index].kind != expected[index].kind) {
			return false;
		}
	}
	return true;
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
	const iggy::PickupPolicyTransfer2DResult &result,
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

void TestSuccessfulMissingStackTransferWithinMax()
{
	const iggy::InventoryState2D inventory;
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::LevelItemDrop2DRegistry drops = Registry({ drop });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });

	const iggy::PickupPolicyTransfer2DResult result =
		iggy::PickupPolicyTransfer2D {}.transfer(ReadyPlan(drop), inventory, drops, catalog);

	Expect(result.status == iggy::PickupPolicyTransfer2DStatus::Transferred, "policy pickup missing stack should transfer");
	Expect(result.changed, "policy pickup missing stack should mark changed");
	Expect(result.add.status == iggy::InventoryPolicyAddItem2DStatus::Added, "policy pickup missing stack should preserve policy add success");
	Expect(result.add.plan.status == iggy::InventoryStackPolicy2DStatus::CanAdd, "policy pickup missing stack should preserve can-add plan");
	Expect(result.consume.status == iggy::LevelItemDropConsume2DStatus::Consumed, "policy pickup missing stack should consume drop");
	Expect(SameStacks(result.inventory.stacks, { Stack("item:potion", 2) }), "policy pickup missing stack should add stack");
	Expect(result.drops.drops.size() == 1, "policy pickup disable mode should preserve drop count");
	if (result.drops.drops.size() == 1) {
		iggy::LevelItemDrop2D disabled = drop;
		disabled.enabled = false;
		Expect(SameDrop(result.drops.drops[0], disabled), "policy pickup should disable consumed drop by default");
	}
	ExpectSuccessfulTransferEvents(result, drop, "policy pickup missing stack should record add, consume, picked-up events");
}

void TestSuccessfulExistingStackTransferExactlyToMax()
{
	const iggy::InventoryState2D inventory = Inventory({ Stack("item:potion", 3), Stack("item:key", 1) });
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::LevelItemDrop2DRegistry drops = Registry({ drop });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });

	const iggy::PickupPolicyTransfer2DResult result =
		iggy::PickupPolicyTransfer2D {}.transfer(ReadyPlan(drop), inventory, drops, catalog);

	Expect(result.status == iggy::PickupPolicyTransfer2DStatus::Transferred, "policy pickup existing stack exactly to max should transfer");
	Expect(result.add.plan.currentCount == 3 && result.add.plan.resultingCount == 5, "policy pickup existing stack should preserve policy count diagnostics");
	Expect(SameStacks(
			   result.inventory.stacks,
			   {
				   Stack("item:potion", 5),
				   Stack("item:key", 1),
			   }),
		"policy pickup existing stack should increment to max and preserve order");
	ExpectSuccessfulTransferEvents(result, drop, "policy pickup existing stack should record requested transfer events");
}

void TestOverCapacityExistingStackFailsAddAndDoesNotConsumeDrop()
{
	const iggy::InventoryState2D inventory = Inventory({ Stack("item:potion", 4) });
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::LevelItemDrop2DRegistry drops = Registry({ drop });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });

	const iggy::PickupPolicyTransfer2DResult result =
		iggy::PickupPolicyTransfer2D {}.transfer(ReadyPlan(drop), inventory, drops, catalog);

	Expect(result.status == iggy::PickupPolicyTransfer2DStatus::InventoryAddFailed, "over-capacity policy pickup should fail add");
	Expect(!result.changed, "over-capacity policy pickup should not mark changed");
	Expect(result.add.status == iggy::InventoryPolicyAddItem2DStatus::PlanFailed, "over-capacity policy pickup should preserve policy add plan failure");
	Expect(result.add.plan.status == iggy::InventoryStackPolicy2DStatus::StackLimitExceeded, "over-capacity policy pickup should preserve stack limit status");
	Expect(result.consume.status == iggy::LevelItemDropConsume2DStatus::MissingDropId, "over-capacity policy pickup should not consume drop");
	Expect(SameStacks(result.inventory.stacks, inventory.stacks), "over-capacity policy pickup should preserve inventory");
	Expect(SameDrops(result.drops.drops, drops.drops), "over-capacity policy pickup should preserve drops");
	Expect(result.events.events.empty(), "over-capacity policy pickup should not record add or picked-up events");
}

void TestMissingItemDefinitionFailsAddAndDoesNotConsumeDrop()
{
	const iggy::InventoryState2D inventory;
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 1);
	const iggy::LevelItemDrop2DRegistry drops = Registry({ drop });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:scroll", "Scroll", 5) });

	const iggy::PickupPolicyTransfer2DResult result =
		iggy::PickupPolicyTransfer2D {}.transfer(ReadyPlan(drop), inventory, drops, catalog);

	Expect(result.status == iggy::PickupPolicyTransfer2DStatus::InventoryAddFailed, "missing definition policy pickup should fail add");
	Expect(result.add.status == iggy::InventoryPolicyAddItem2DStatus::PlanFailed, "missing definition policy pickup should preserve policy plan failure");
	Expect(result.add.plan.status == iggy::InventoryStackPolicy2DStatus::ItemDefinitionNotFound, "missing definition policy pickup should preserve missing definition status");
	Expect(result.consume.status == iggy::LevelItemDropConsume2DStatus::MissingDropId, "missing definition policy pickup should not consume drop");
	Expect(SameStacks(result.inventory.stacks, inventory.stacks), "missing definition policy pickup should preserve inventory");
	Expect(SameDrops(result.drops.drops, drops.drops), "missing definition policy pickup should preserve drops");
	Expect(result.events.events.empty(), "missing definition policy pickup should not invent failure events");
}

void TestNonReadyPlanDoesNotAddOrConsume()
{
	const iggy::InventoryState2D inventory = Inventory({ Stack("item:potion", 1) });
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::LevelItemDrop2DRegistry drops = Registry({ drop });
	const iggy::PickupPlan2DResult plan = iggy::PickupPlan2D {}.plan(drops, drop.id, { 5.0F, 0.0F });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });

	const iggy::PickupPolicyTransfer2DResult result =
		iggy::PickupPolicyTransfer2D {}.transfer(plan, inventory, drops, catalog);

	Expect(result.status == iggy::PickupPolicyTransfer2DStatus::PickupNotReady, "non-ready policy pickup should not transfer");
	Expect(!result.changed, "non-ready policy pickup should not mark changed");
	Expect(result.plan.status == plan.status && result.plan.dropId == plan.dropId, "non-ready policy pickup should preserve plan");
	Expect(result.add.status == iggy::InventoryPolicyAddItem2DStatus::PlanFailed, "non-ready policy pickup should not run policy add");
	Expect(result.consume.status == iggy::LevelItemDropConsume2DStatus::MissingDropId, "non-ready policy pickup should not consume drop");
	Expect(SameStacks(result.inventory.stacks, inventory.stacks), "non-ready policy pickup should preserve inventory");
	Expect(SameDrops(result.drops.drops, drops.drops), "non-ready policy pickup should preserve drops");
	Expect(result.events.events.empty(), "non-ready policy pickup should produce no events");
}

void TestConsumeFailurePreservesAddEventOnly()
{
	const iggy::InventoryState2D inventory;
	const iggy::LevelItemDrop2D plannedDrop = Drop("drop:missing", "item:potion", 2);
	const iggy::LevelItemDrop2DRegistry drops = Registry({ Drop("drop:other", "item:key", 1) });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });

	const iggy::PickupPolicyTransfer2DResult result =
		iggy::PickupPolicyTransfer2D {}.transfer(ReadyPlan(plannedDrop), inventory, drops, catalog);

	Expect(result.status == iggy::PickupPolicyTransfer2DStatus::DropConsumeFailed, "policy pickup missing planned drop should fail consume");
	Expect(!result.changed, "consume failure should not mark policy transfer complete");
	Expect(result.add.status == iggy::InventoryPolicyAddItem2DStatus::Added, "consume failure should preserve successful policy add");
	Expect(result.consume.status == iggy::LevelItemDropConsume2DStatus::DropNotFound, "consume failure should preserve consume diagnostics");
	Expect(SameStacks(result.inventory.stacks, { Stack("item:potion", 2) }), "consume failure should expose partial added inventory");
	Expect(SameDrops(result.drops.drops, drops.drops), "consume failure should preserve consume/original drops");
	Expect(result.events.events.size() == 1, "consume failure should preserve add event only");
	if (result.events.events.size() == 1) {
		ExpectEvent(result.events.events[0], iggy::InventoryEvent2DType::ItemAdded, plannedDrop.itemId, {}, plannedDrop.count, "consume failure should preserve ItemAdded event only");
	}
}

void TestRemoveModeRemovesDrop()
{
	const iggy::InventoryState2D inventory;
	const iggy::LevelItemDrop2D first = Drop("drop:potion", "item:potion", 1);
	const iggy::LevelItemDrop2D second = Drop("drop:key", "item:key", 1);
	const iggy::LevelItemDrop2DRegistry drops = Registry({ first, second });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });

	const iggy::PickupPolicyTransfer2DResult result = iggy::PickupPolicyTransfer2D {}.transfer(
		ReadyPlan(first),
		inventory,
		drops,
		catalog,
		{ iggy::LevelItemDropConsume2DMode::Remove });

	Expect(result.status == iggy::PickupPolicyTransfer2DStatus::Transferred, "remove mode policy pickup should transfer");
	Expect(result.consume.mode == iggy::LevelItemDropConsume2DMode::Remove, "remove mode policy pickup should preserve consume mode");
	Expect(result.drops.drops.size() == 1, "remove mode policy pickup should remove consumed drop");
	if (result.drops.drops.size() == 1)
		Expect(SameDrop(result.drops.drops[0], second), "remove mode policy pickup should preserve remaining drops");
	ExpectSuccessfulTransferEvents(result, first, "remove mode policy pickup should record transfer events");
}

void TestNamespacedIdsRemainDistinct()
{
	const iggy::InventoryState2D inventory = Inventory({ Stack("potion", 1) });
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::LevelItemDrop2DRegistry drops = Registry({ Drop("potion", "item:other", 1), drop });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({
		Definition("potion", "Potion", 2),
		Definition("item:potion", "Namespaced Potion", 3),
	});

	const iggy::PickupPolicyTransfer2DResult result = iggy::PickupPolicyTransfer2D {}.transfer(
		ReadyPlan(drop),
		inventory,
		drops,
		catalog,
		{ iggy::LevelItemDropConsume2DMode::Remove });

	Expect(result.status == iggy::PickupPolicyTransfer2DStatus::Transferred, "namespaced policy pickup should transfer");
	Expect(result.add.plan.currentCount == 0 && result.add.plan.maxStackCount == 3, "namespaced policy pickup should use exact namespaced definition and ignore unqualified stack");
	Expect(SameStacks(
			   result.inventory.stacks,
			   {
				   Stack("potion", 1),
				   Stack("item:potion", 2),
			   }),
		"namespaced policy pickup should append separately from unqualified stack");
	Expect(result.drops.drops.size() == 1, "namespaced policy pickup should remove only matching namespaced drop");
	if (result.drops.drops.size() == 1)
		Expect(result.drops.drops[0].id == Id("potion"), "unqualified drop should remain distinct");
}

void TestInputsAreNotMutated()
{
	iggy::InventoryState2D inventory = Inventory({ Stack("item:potion", 3) });
	const iggy::InventoryState2D inventoryBefore = inventory;
	iggy::LevelItemDrop2DRegistry drops = Registry({ Drop("drop:potion", "item:potion", 2) });
	const iggy::LevelItemDrop2DRegistry dropsBefore = drops;
	const std::vector<iggy::ItemDefinition2D> definitions { Definition("item:potion", "Potion", 5) };
	iggy::ItemDefinition2DCatalog catalog = Catalog(definitions);

	const iggy::PickupPolicyTransfer2DResult result =
		iggy::PickupPolicyTransfer2D {}.transfer(ReadyPlan(drops.drops[0]), inventory, drops, catalog);

	Expect(result.status == iggy::PickupPolicyTransfer2DStatus::Transferred, "immutability policy pickup setup should transfer");
	Expect(SameStacks(inventory.stacks, inventoryBefore.stacks), "policy pickup transfer should not mutate original inventory");
	Expect(SameDrops(drops.drops, dropsBefore.drops), "policy pickup transfer should not mutate original drops");
	Expect(SameDefinitions(catalog.entries, definitions), "policy pickup transfer should not mutate catalog");
}

} // namespace

int main()
{
	TestSuccessfulMissingStackTransferWithinMax();
	TestSuccessfulExistingStackTransferExactlyToMax();
	TestOverCapacityExistingStackFailsAddAndDoesNotConsumeDrop();
	TestMissingItemDefinitionFailsAddAndDoesNotConsumeDrop();
	TestNonReadyPlanDoesNotAddOrConsume();
	TestConsumeFailurePreservesAddEventOnly();
	TestRemoveModeRemovesDrop();
	TestNamespacedIdsRemainDistinct();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
