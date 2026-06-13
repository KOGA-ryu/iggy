#include <cstdlib>
#include <vector>

#include "runtime/RuntimePolicyPickupStep.hpp"
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
	Expect(result.built, "runtime policy pickup inventory fixture should build");
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

iggy::LevelItemDrop2DRegistry Drops(std::vector<iggy::LevelItemDrop2D> drops)
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
	Expect(result.built, "runtime policy pickup catalog fixture should build");
	return result.catalog;
}

iggy::runtime::RuntimeSessionState SessionWithPlayer(iggy::Vec2 position = { 0.0F, 0.0F })
{
	iggy::runtime::RuntimeSessionState session;
	session.hasPlayer = true;
	session.player = {
		Id("player:one"),
		position,
		{ 0, 0 },
		iggy::PlayerMovementStatus::Idle,
		iggy::PlayerFacing2D::South,
	};
	session.tickIndex = 7;
	return session;
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
	const iggy::runtime::RuntimePolicyPickupResult &result,
	const iggy::ResourceId &dropId,
	const char *message)
{
	Expect(result.events.events.size() == 1, message);
	if (result.events.events.size() == 1)
		ExpectEvent(result.events.events[0], iggy::InventoryEvent2DType::PickupNotReady, {}, dropId, 0, message);
}

void ExpectSuccessfulPickupEvents(
	const iggy::runtime::RuntimePolicyPickupResult &result,
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

void TestMissingPlayerReturnsMissingPlayerWithoutPlanOrTransfer()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	session.hasPlayer = false;
	session.player = {};
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });

	const iggy::runtime::RuntimePolicyPickupResult result =
		iggy::runtime::RuntimePolicyPickupStep {}.pickup(session, inventory, catalog, drop.id);

	Expect(result.status == iggy::runtime::RuntimePolicyPickupStatus::MissingPlayer, "missing player policy pickup should report MissingPlayer");
	Expect(result.dropId == drop.id, "missing player policy pickup should preserve requested drop id");
	Expect(!result.changed, "missing player policy pickup should not change inventory state");
	Expect(SameStacks(result.inventory.inventory.stacks, inventory.inventory.stacks), "missing player policy pickup should preserve inventory");
	Expect(SameDrops(result.inventory.drops.drops, inventory.drops.drops), "missing player policy pickup should preserve drops");
	Expect(result.plan.status == iggy::PickupPlan2DStatus::DropNotFound, "missing player policy pickup should not run planner");
	Expect(result.transfer.status == iggy::PickupPolicyTransfer2DStatus::PickupNotReady, "missing player policy pickup should not run transfer");
	Expect(result.events.events.empty(), "missing player policy pickup should not record inventory events");
}

void TestMissingDropReturnsPickupNotReady()
{
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({ Stack("item:potion", 1) }, {});
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });

	const iggy::runtime::RuntimePolicyPickupResult result =
		iggy::runtime::RuntimePolicyPickupStep {}.pickup(SessionWithPlayer(), inventory, catalog, Id("drop:missing"));

	Expect(result.status == iggy::runtime::RuntimePolicyPickupStatus::PickupNotReady, "missing drop policy pickup should not be ready");
	Expect(result.plan.status == iggy::PickupPlan2DStatus::DropNotFound, "missing drop policy pickup should preserve plan status");
	Expect(!result.changed, "missing drop policy pickup should not change inventory state");
	Expect(SameStacks(result.inventory.inventory.stacks, inventory.inventory.stacks), "missing drop policy pickup should preserve inventory");
	Expect(SameDrops(result.inventory.drops.drops, inventory.drops.drops), "missing drop policy pickup should preserve drops");
	ExpectPickupNotReadyEvent(result, Id("drop:missing"), "missing drop policy pickup should record PickupNotReady event");
}

void TestDisabledDropReturnsPickupNotReady()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2, { 0.0F, 0.0F }, 1.0F, false);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });

	const iggy::runtime::RuntimePolicyPickupResult result =
		iggy::runtime::RuntimePolicyPickupStep {}.pickup(SessionWithPlayer(), inventory, catalog, drop.id);

	Expect(result.status == iggy::runtime::RuntimePolicyPickupStatus::PickupNotReady, "disabled drop policy pickup should not be ready");
	Expect(result.plan.status == iggy::PickupPlan2DStatus::DropDisabled, "disabled drop policy pickup should preserve plan status");
	Expect(!result.changed, "disabled drop policy pickup should not change inventory state");
	Expect(SameDrops(result.inventory.drops.drops, inventory.drops.drops), "disabled drop policy pickup should preserve drops");
	ExpectPickupNotReadyEvent(result, drop.id, "disabled drop policy pickup should record PickupNotReady event");
}

void TestOutOfRangeDropReturnsPickupNotReady()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2, { 5.0F, 0.0F }, 1.0F, true);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });

	const iggy::runtime::RuntimePolicyPickupResult result =
		iggy::runtime::RuntimePolicyPickupStep {}.pickup(SessionWithPlayer(), inventory, catalog, drop.id);

	Expect(result.status == iggy::runtime::RuntimePolicyPickupStatus::PickupNotReady, "out-of-range policy pickup should not be ready");
	Expect(result.plan.status == iggy::PickupPlan2DStatus::OutOfRange, "out-of-range policy pickup should preserve plan status");
	Expect(!result.changed, "out-of-range policy pickup should not change inventory state");
	ExpectPickupNotReadyEvent(result, drop.id, "out-of-range policy pickup should record PickupNotReady event");
}

void TestSuccessfulPickupWithinMax()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2, { 0.0F, 0.0F }, 0.0F, true);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });

	const iggy::runtime::RuntimePolicyPickupResult result =
		iggy::runtime::RuntimePolicyPickupStep {}.pickup(SessionWithPlayer(), inventory, catalog, drop.id);

	Expect(result.status == iggy::runtime::RuntimePolicyPickupStatus::PickedUp, "ready policy pickup should pick up");
	Expect(result.changed, "ready policy pickup should mark changed");
	Expect(result.plan.status == iggy::PickupPlan2DStatus::Ready, "ready policy pickup should preserve ready plan");
	Expect(result.transfer.status == iggy::PickupPolicyTransfer2DStatus::Transferred, "ready policy pickup should preserve transfer result");
	Expect(result.transfer.add.plan.status == iggy::InventoryStackPolicy2DStatus::CanAdd, "ready policy pickup should preserve can-add diagnostics");
	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:potion", 2) }), "ready policy pickup should add item to inventory");
	Expect(result.inventory.drops.drops.size() == 1, "ready policy pickup disable mode should preserve drop count");
	if (result.inventory.drops.drops.size() == 1) {
		iggy::LevelItemDrop2D disabled = drop;
		disabled.enabled = false;
		Expect(SameDrop(result.inventory.drops.drops[0], disabled), "ready policy pickup should disable drop by default");
	}
	ExpectSuccessfulPickupEvents(result, drop, "ready policy pickup should expose transfer inventory events");
}

void TestExistingStackPickupExactlyToMax()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2, { 0.0F, 0.0F }, 0.0F, true);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState(
		{ Stack("item:potion", 3), Stack("item:key", 1) },
		{ drop });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });

	const iggy::runtime::RuntimePolicyPickupResult result =
		iggy::runtime::RuntimePolicyPickupStep {}.pickup(SessionWithPlayer(), inventory, catalog, drop.id);

	Expect(result.status == iggy::runtime::RuntimePolicyPickupStatus::PickedUp, "existing stack policy pickup exactly to max should pick up");
	Expect(result.transfer.add.plan.currentCount == 3 && result.transfer.add.plan.resultingCount == 5, "existing stack policy pickup should preserve count diagnostics");
	Expect(SameStacks(
			   result.inventory.inventory.stacks,
			   {
				   Stack("item:potion", 5),
				   Stack("item:key", 1),
			   }),
		"existing stack policy pickup should increment stack to max");
	ExpectSuccessfulPickupEvents(result, drop, "existing stack policy pickup should expose transfer inventory events");
}

void TestOverCapacityPickupReturnsTransferFailed()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2, { 0.0F, 0.0F }, 0.0F, true);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({ Stack("item:potion", 4) }, { drop });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });

	const iggy::runtime::RuntimePolicyPickupResult result =
		iggy::runtime::RuntimePolicyPickupStep {}.pickup(SessionWithPlayer(), inventory, catalog, drop.id);

	Expect(result.status == iggy::runtime::RuntimePolicyPickupStatus::TransferFailed, "over-capacity policy pickup should fail transfer");
	Expect(!result.changed, "over-capacity policy pickup should not mark changed");
	Expect(result.transfer.status == iggy::PickupPolicyTransfer2DStatus::InventoryAddFailed, "over-capacity policy pickup should preserve transfer add failure");
	Expect(result.transfer.add.status == iggy::InventoryPolicyAddItem2DStatus::PlanFailed, "over-capacity policy pickup should preserve policy add plan failure");
	Expect(result.transfer.add.plan.status == iggy::InventoryStackPolicy2DStatus::StackLimitExceeded, "over-capacity policy pickup should preserve stack limit diagnostics");
	Expect(SameStacks(result.inventory.inventory.stacks, inventory.inventory.stacks), "over-capacity policy pickup should preserve inventory");
	Expect(SameDrops(result.inventory.drops.drops, inventory.drops.drops), "over-capacity policy pickup should not consume drop");
	Expect(result.events.events.empty(), "over-capacity policy pickup should not record picked-up events");
}

void TestMissingItemDefinitionReturnsTransferFailed()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 1, { 0.0F, 0.0F }, 0.0F, true);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:scroll", "Scroll", 5) });

	const iggy::runtime::RuntimePolicyPickupResult result =
		iggy::runtime::RuntimePolicyPickupStep {}.pickup(SessionWithPlayer(), inventory, catalog, drop.id);

	Expect(result.status == iggy::runtime::RuntimePolicyPickupStatus::TransferFailed, "missing definition policy pickup should fail transfer");
	Expect(result.transfer.status == iggy::PickupPolicyTransfer2DStatus::InventoryAddFailed, "missing definition policy pickup should preserve transfer add failure");
	Expect(result.transfer.add.plan.status == iggy::InventoryStackPolicy2DStatus::ItemDefinitionNotFound, "missing definition policy pickup should preserve definition diagnostics");
	Expect(SameStacks(result.inventory.inventory.stacks, inventory.inventory.stacks), "missing definition policy pickup should preserve inventory");
	Expect(SameDrops(result.inventory.drops.drops, inventory.drops.drops), "missing definition policy pickup should not consume drop");
	Expect(result.events.events.empty(), "missing definition policy pickup should not invent failure events");
}

void TestRemoveConsumeModeRemovesDrop()
{
	const iggy::LevelItemDrop2D first = Drop("drop:potion", "item:potion", 1, { 0.0F, 0.0F }, 0.0F, true);
	const iggy::LevelItemDrop2D second = Drop("drop:key", "item:key", 1, { 1.0F, 0.0F }, 2.0F, true);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { first, second });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });
	iggy::runtime::RuntimePolicyPickupConfig config;
	config.transfer.consumeMode = iggy::LevelItemDropConsume2DMode::Remove;

	const iggy::runtime::RuntimePolicyPickupResult result =
		iggy::runtime::RuntimePolicyPickupStep {}.pickup(SessionWithPlayer(), inventory, catalog, first.id, config);

	Expect(result.status == iggy::runtime::RuntimePolicyPickupStatus::PickedUp, "remove mode policy runtime pickup should pick up");
	Expect(result.transfer.consume.mode == iggy::LevelItemDropConsume2DMode::Remove, "remove mode policy runtime pickup should preserve consume mode");
	Expect(result.inventory.drops.drops.size() == 1, "remove mode policy runtime pickup should remove drop");
	if (result.inventory.drops.drops.size() == 1)
		Expect(SameDrop(result.inventory.drops.drops[0], second), "remove mode policy runtime pickup should preserve remaining drop");
	ExpectSuccessfulPickupEvents(result, first, "remove mode policy runtime pickup should expose transfer inventory events");
}

void TestExtraReachConfigCanMakePickupReady()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 1, { 3.0F, 0.0F }, 1.0F, true);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });
	iggy::runtime::RuntimePolicyPickupConfig config;
	config.plan.extraReach = 2.0F;

	const iggy::runtime::RuntimePolicyPickupResult result =
		iggy::runtime::RuntimePolicyPickupStep {}.pickup(SessionWithPlayer(), inventory, catalog, drop.id, config);

	Expect(result.status == iggy::runtime::RuntimePolicyPickupStatus::PickedUp, "extra reach policy runtime pickup should pick up");
	Expect(result.plan.allowedDistance == 3.0F, "extra reach policy runtime pickup should preserve plan allowed distance");
}

void TestNamespacedItemAndDropIdsArePreserved()
{
	const iggy::LevelItemDrop2D unqualified = Drop("potion", "item:other", 1, { 1.0F, 0.0F }, 2.0F, true);
	const iggy::LevelItemDrop2D namespaced = Drop("drop:potion", "item:potion", 2, { 0.0F, 0.0F }, 0.0F, true);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState(
		{ Stack("potion", 1) },
		{ unqualified, namespaced });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({
		Definition("potion", "Potion", 2),
		Definition("item:potion", "Namespaced Potion", 3),
	});
	iggy::runtime::RuntimePolicyPickupConfig config;
	config.transfer.consumeMode = iggy::LevelItemDropConsume2DMode::Remove;

	const iggy::runtime::RuntimePolicyPickupResult result =
		iggy::runtime::RuntimePolicyPickupStep {}.pickup(SessionWithPlayer(), inventory, catalog, namespaced.id, config);

	Expect(result.status == iggy::runtime::RuntimePolicyPickupStatus::PickedUp, "namespaced policy runtime pickup should pick up");
	Expect(result.transfer.add.plan.currentCount == 0 && result.transfer.add.plan.maxStackCount == 3, "namespaced policy runtime pickup should use exact namespaced definition");
	Expect(SameStacks(
			   result.inventory.inventory.stacks,
			   {
				   Stack("potion", 1),
				   Stack("item:potion", 2),
			   }),
		"namespaced policy runtime pickup should preserve distinct item ids");
	Expect(result.inventory.drops.drops.size() == 1, "namespaced policy runtime pickup should remove only requested drop");
	if (result.inventory.drops.drops.size() == 1)
		Expect(result.inventory.drops.drops[0].id == Id("potion"), "unqualified drop should remain distinct");
}

void TestOriginalSessionAndInventoryStateAreNotMutated()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::runtime::RuntimeSessionState sessionBefore = session;
	iggy::runtime::RuntimeInventoryState inventory = InventoryState(
		{ Stack("item:potion", 3) },
		{ Drop("drop:potion", "item:potion", 2) });
	const iggy::runtime::RuntimeInventoryState inventoryBefore = inventory;
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });

	const iggy::runtime::RuntimePolicyPickupResult result =
		iggy::runtime::RuntimePolicyPickupStep {}.pickup(session, inventory, catalog, Id("drop:potion"));

	Expect(result.status == iggy::runtime::RuntimePolicyPickupStatus::PickedUp, "immutability policy runtime pickup setup should pick up");
	Expect(session.hasPlayer == sessionBefore.hasPlayer, "policy runtime pickup should not mutate session player presence");
	Expect(NearVec(session.player.position, sessionBefore.player.position), "policy runtime pickup should not mutate session player position");
	Expect(session.tickIndex == sessionBefore.tickIndex, "policy runtime pickup should not mutate session tick");
	Expect(SameStacks(inventory.inventory.stacks, inventoryBefore.inventory.stacks), "policy runtime pickup should not mutate input inventory");
	Expect(SameDrops(inventory.drops.drops, inventoryBefore.drops.drops), "policy runtime pickup should not mutate input drops");
}

} // namespace

int main()
{
	TestMissingPlayerReturnsMissingPlayerWithoutPlanOrTransfer();
	TestMissingDropReturnsPickupNotReady();
	TestDisabledDropReturnsPickupNotReady();
	TestOutOfRangeDropReturnsPickupNotReady();
	TestSuccessfulPickupWithinMax();
	TestExistingStackPickupExactlyToMax();
	TestOverCapacityPickupReturnsTransferFailed();
	TestMissingItemDefinitionReturnsTransferFailed();
	TestRemoveConsumeModeRemovesDrop();
	TestExtraReachConfigCanMakePickupReady();
	TestNamespacedItemAndDropIdsArePreserved();
	TestOriginalSessionAndInventoryStateAreNotMutated();

	return Failures;
}
