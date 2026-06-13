#include <cstdlib>
#include <vector>

#include "runtime/RuntimePickupStep.hpp"
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
	Expect(result.built, "runtime pickup inventory fixture should build");
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

iggy::runtime::RuntimeSessionState SessionWithPlayer(iggy::Vec2 position = { 0.0F, 0.0F })
{
	iggy::runtime::RuntimeSessionState session;
	session.hasPlayer = true;
	session.player = {
		iggy::ResourceId { "player:one" },
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

void ExpectNoEvents(const iggy::runtime::RuntimePickupResult &result, const char *message)
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
	const iggy::runtime::RuntimePickupResult &result,
	const iggy::ResourceId &dropId,
	const char *message)
{
	Expect(result.events.events.size() == 1, message);
	if (result.events.events.size() == 1)
		ExpectEvent(result.events.events[0], iggy::InventoryEvent2DType::PickupNotReady, {}, dropId, 0, message);
}

void ExpectSuccessfulPickupEvents(
	const iggy::runtime::RuntimePickupResult &result,
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
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { Drop("drop:potion") });

	const iggy::runtime::RuntimePickupResult result =
		iggy::runtime::RuntimePickupStep {}.pickup(session, inventory, iggy::ResourceId { "drop:potion" });

	Expect(result.status == iggy::runtime::RuntimePickupStatus::MissingPlayer, "missing player pickup should report MissingPlayer");
	Expect(result.dropId == iggy::ResourceId { "drop:potion" }, "missing player pickup should preserve requested drop id");
	Expect(!result.changed, "missing player pickup should not change inventory state");
	Expect(SameStacks(result.inventory.inventory.stacks, inventory.inventory.stacks), "missing player pickup should preserve inventory");
	Expect(SameDrops(result.inventory.drops.drops, inventory.drops.drops), "missing player pickup should preserve drops");
	Expect(result.plan.status == iggy::PickupPlan2DStatus::DropNotFound, "missing player pickup should not run planner");
	Expect(result.transfer.status == iggy::PickupTransfer2DStatus::PickupNotReady, "missing player pickup should not run transfer");
	ExpectNoEvents(result, "missing player pickup should not record inventory events");
}

void TestMissingDropReturnsPickupNotReady()
{
	const iggy::runtime::RuntimeSessionState session = SessionWithPlayer();
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({ Stack("item:potion", 1) }, {});

	const iggy::runtime::RuntimePickupResult result =
		iggy::runtime::RuntimePickupStep {}.pickup(session, inventory, iggy::ResourceId { "drop:missing" });

	Expect(result.status == iggy::runtime::RuntimePickupStatus::PickupNotReady, "missing drop pickup should not be ready");
	Expect(result.plan.status == iggy::PickupPlan2DStatus::DropNotFound, "missing drop pickup should preserve plan status");
	Expect(!result.changed, "missing drop pickup should not change inventory state");
	Expect(SameStacks(result.inventory.inventory.stacks, inventory.inventory.stacks), "missing drop pickup should preserve inventory");
	Expect(SameDrops(result.inventory.drops.drops, inventory.drops.drops), "missing drop pickup should preserve drops");
	ExpectPickupNotReadyEvent(result, iggy::ResourceId { "drop:missing" }, "missing drop pickup should record PickupNotReady event");
}

void TestDisabledDropReturnsPickupNotReady()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2, { 0.0F, 0.0F }, 1.0F, false);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });

	const iggy::runtime::RuntimePickupResult result =
		iggy::runtime::RuntimePickupStep {}.pickup(SessionWithPlayer(), inventory, drop.id);

	Expect(result.status == iggy::runtime::RuntimePickupStatus::PickupNotReady, "disabled drop pickup should not be ready");
	Expect(result.plan.status == iggy::PickupPlan2DStatus::DropDisabled, "disabled drop pickup should preserve plan status");
	Expect(!result.changed, "disabled drop pickup should not change inventory state");
	Expect(SameStacks(result.inventory.inventory.stacks, inventory.inventory.stacks), "disabled drop pickup should preserve inventory");
	Expect(SameDrops(result.inventory.drops.drops, inventory.drops.drops), "disabled drop pickup should preserve drops");
	ExpectPickupNotReadyEvent(result, drop.id, "disabled drop pickup should record PickupNotReady event");
}

void TestOutOfRangeDropReturnsPickupNotReady()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2, { 5.0F, 0.0F }, 1.0F, true);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });

	const iggy::runtime::RuntimePickupResult result =
		iggy::runtime::RuntimePickupStep {}.pickup(SessionWithPlayer(), inventory, drop.id);

	Expect(result.status == iggy::runtime::RuntimePickupStatus::PickupNotReady, "out-of-range drop pickup should not be ready");
	Expect(result.plan.status == iggy::PickupPlan2DStatus::OutOfRange, "out-of-range pickup should preserve plan status");
	Expect(!result.changed, "out-of-range drop pickup should not change inventory state");
	ExpectPickupNotReadyEvent(result, drop.id, "out-of-range drop pickup should record PickupNotReady event");
}

void TestReadyPickupTransfersToEmptyInventoryAndDisablesDrop()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 2, { 0.0F, 0.0F }, 0.0F, true);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });

	const iggy::runtime::RuntimePickupResult result =
		iggy::runtime::RuntimePickupStep {}.pickup(SessionWithPlayer(), inventory, drop.id);

	Expect(result.status == iggy::runtime::RuntimePickupStatus::PickedUp, "ready pickup should pick up");
	Expect(result.changed, "ready pickup should mark changed");
	Expect(result.plan.status == iggy::PickupPlan2DStatus::Ready, "ready pickup should preserve ready plan");
	Expect(result.transfer.status == iggy::PickupTransfer2DStatus::Transferred, "ready pickup should preserve transfer result");
	Expect(SameStacks(result.inventory.inventory.stacks, { Stack("item:potion", 2) }), "ready pickup should add item to inventory");
	Expect(result.inventory.drops.drops.size() == 1, "ready pickup disable mode should preserve drop count");
	if (result.inventory.drops.drops.size() == 1) {
		iggy::LevelItemDrop2D disabled = drop;
		disabled.enabled = false;
		Expect(SameDrop(result.inventory.drops.drops[0], disabled), "ready pickup should disable drop by default");
	}
	ExpectSuccessfulPickupEvents(result, drop, "ready pickup should expose transfer inventory events");
}

void TestReadyPickupWithExistingStackIncrementsCount()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 4, { 0.0F, 0.0F }, 0.0F, true);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState(
		{ Stack("item:potion", 3), Stack("item:key", 1) },
		{ drop });

	const iggy::runtime::RuntimePickupResult result =
		iggy::runtime::RuntimePickupStep {}.pickup(SessionWithPlayer(), inventory, drop.id);

	Expect(result.status == iggy::runtime::RuntimePickupStatus::PickedUp, "existing stack runtime pickup should pick up");
	Expect(SameStacks(
			   result.inventory.inventory.stacks,
			   {
				   Stack("item:potion", 7),
				   Stack("item:key", 1),
			   }),
		"existing stack runtime pickup should increment stack and preserve order");
	ExpectSuccessfulPickupEvents(result, drop, "existing stack runtime pickup should expose transfer inventory events");
}

void TestRemoveConsumeModeRemovesDrop()
{
	const iggy::LevelItemDrop2D first = Drop("drop:potion", "item:potion", 1, { 0.0F, 0.0F }, 0.0F, true);
	const iggy::LevelItemDrop2D second = Drop("drop:key", "item:key", 1, { 1.0F, 0.0F }, 2.0F, true);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { first, second });
	iggy::runtime::RuntimePickupConfig config;
	config.transfer.consumeMode = iggy::LevelItemDropConsume2DMode::Remove;

	const iggy::runtime::RuntimePickupResult result =
		iggy::runtime::RuntimePickupStep {}.pickup(SessionWithPlayer(), inventory, first.id, config);

	Expect(result.status == iggy::runtime::RuntimePickupStatus::PickedUp, "remove mode runtime pickup should pick up");
	Expect(result.transfer.consume.mode == iggy::LevelItemDropConsume2DMode::Remove, "remove mode runtime pickup should preserve consume mode");
	Expect(result.inventory.drops.drops.size() == 1, "remove mode runtime pickup should remove drop");
	if (result.inventory.drops.drops.size() == 1)
		Expect(SameDrop(result.inventory.drops.drops[0], second), "remove mode runtime pickup should preserve remaining drop");
	ExpectSuccessfulPickupEvents(result, first, "remove mode runtime pickup should expose transfer inventory events");
}

void TestExtraReachConfigCanMakePickupReady()
{
	const iggy::LevelItemDrop2D drop = Drop("drop:potion", "item:potion", 1, { 3.0F, 0.0F }, 1.0F, true);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState({}, { drop });
	iggy::runtime::RuntimePickupConfig config;
	config.plan.extraReach = 2.0F;

	const iggy::runtime::RuntimePickupResult result =
		iggy::runtime::RuntimePickupStep {}.pickup(SessionWithPlayer(), inventory, drop.id, config);

	Expect(result.status == iggy::runtime::RuntimePickupStatus::PickedUp, "extra reach runtime pickup should pick up");
	Expect(result.plan.allowedDistance == 3.0F, "extra reach runtime pickup should preserve plan allowed distance");
}

void TestTransferFailurePreservesTransferDiagnosticsAndPartialInventory()
{
	const iggy::LevelItemDrop2D invalidDrop = Drop("drop:bad_item", "", 2, { 0.0F, 0.0F }, 0.0F, true);
	iggy::LevelItemDrop2DRegistry drops;
	drops.drops = { invalidDrop };
	const iggy::runtime::RuntimeInventoryState inventory {
		{},
		drops,
	};

	const iggy::runtime::RuntimePickupResult result =
		iggy::runtime::RuntimePickupStep {}.pickup(SessionWithPlayer(), inventory, invalidDrop.id);

	Expect(result.status == iggy::runtime::RuntimePickupStatus::TransferFailed, "invalid ready drop should fail transfer");
	Expect(!result.changed, "failed transfer should not mark runtime pickup changed");
	Expect(result.plan.status == iggy::PickupPlan2DStatus::Ready, "failed transfer setup should preserve ready plan");
	Expect(result.transfer.status == iggy::PickupTransfer2DStatus::InventoryAddFailed, "failed transfer should preserve transfer diagnostics");
	Expect(SameStacks(result.inventory.inventory.stacks, inventory.inventory.stacks), "failed transfer should preserve returned transfer inventory");
	Expect(SameDrops(result.inventory.drops.drops, inventory.drops.drops), "failed transfer should preserve returned transfer drops");
	Expect(result.events.events.size() == 1, "failed transfer should preserve transfer events");
	if (result.events.events.size() == 1) {
		ExpectEvent(
			result.events.events[0],
			iggy::InventoryEvent2DType::InventoryAddFailed,
			invalidDrop.itemId,
			{},
			invalidDrop.count,
			"failed transfer should preserve InventoryAddFailed event");
	}
}

void TestOriginalSessionAndInventoryStateAreNotMutated()
{
	iggy::runtime::RuntimeSessionState session = SessionWithPlayer({ 0.0F, 0.0F });
	const iggy::runtime::RuntimeSessionState sessionBefore = session;
	iggy::runtime::RuntimeInventoryState inventory = InventoryState(
		{ Stack("item:potion", 3) },
		{ Drop("drop:potion", "item:potion", 2) });
	const iggy::runtime::RuntimeInventoryState inventoryBefore = inventory;

	const iggy::runtime::RuntimePickupResult result =
		iggy::runtime::RuntimePickupStep {}.pickup(session, inventory, iggy::ResourceId { "drop:potion" });

	Expect(result.status == iggy::runtime::RuntimePickupStatus::PickedUp, "immutability runtime pickup setup should pick up");
	Expect(session.hasPlayer == sessionBefore.hasPlayer, "runtime pickup should not mutate session player presence");
	Expect(NearVec(session.player.position, sessionBefore.player.position), "runtime pickup should not mutate session player position");
	Expect(session.tickIndex == sessionBefore.tickIndex, "runtime pickup should not mutate session tick");
	Expect(SameStacks(inventory.inventory.stacks, inventoryBefore.inventory.stacks), "runtime pickup should not mutate input inventory");
	Expect(SameDrops(inventory.drops.drops, inventoryBefore.drops.drops), "runtime pickup should not mutate input drops");
}

void TestNamespacedItemAndDropIdsArePreserved()
{
	const iggy::LevelItemDrop2D unqualified = Drop("potion", "item:other", 1, { 1.0F, 0.0F }, 2.0F, true);
	const iggy::LevelItemDrop2D namespaced = Drop("drop:potion", "item:potion", 2, { 0.0F, 0.0F }, 0.0F, true);
	const iggy::runtime::RuntimeInventoryState inventory = InventoryState(
		{ Stack("potion", 1) },
		{ unqualified, namespaced });

	iggy::runtime::RuntimePickupConfig config;
	config.transfer.consumeMode = iggy::LevelItemDropConsume2DMode::Remove;
	const iggy::runtime::RuntimePickupResult result =
		iggy::runtime::RuntimePickupStep {}.pickup(SessionWithPlayer(), inventory, namespaced.id, config);

	Expect(result.status == iggy::runtime::RuntimePickupStatus::PickedUp, "namespaced runtime pickup should pick up");
	Expect(SameStacks(
			   result.inventory.inventory.stacks,
			   {
				   Stack("potion", 1),
				   Stack("item:potion", 2),
			   }),
		"namespaced runtime pickup should preserve distinct item ids");
	Expect(result.inventory.drops.drops.size() == 1, "namespaced runtime pickup should remove only requested drop");
	if (result.inventory.drops.drops.size() == 1)
		Expect(result.inventory.drops.drops[0].id == iggy::ResourceId { "potion" }, "unqualified drop should remain distinct");
}

} // namespace

int main()
{
	TestMissingPlayerReturnsMissingPlayerWithoutPlanOrTransfer();
	TestMissingDropReturnsPickupNotReady();
	TestDisabledDropReturnsPickupNotReady();
	TestOutOfRangeDropReturnsPickupNotReady();
	TestReadyPickupTransfersToEmptyInventoryAndDisablesDrop();
	TestReadyPickupWithExistingStackIncrementsCount();
	TestRemoveConsumeModeRemovesDrop();
	TestExtraReachConfigCanMakePickupReady();
	TestTransferFailurePreservesTransferDiagnosticsAndPartialInventory();
	TestOriginalSessionAndInventoryStateAreNotMutated();
	TestNamespacedItemAndDropIdsArePreserved();

	return Failures;
}
