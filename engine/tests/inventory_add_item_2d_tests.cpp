#include <cstdlib>
#include <vector>

#include "scene/inventory/InventoryAddItem2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::InventoryItemStack2D Stack(const char *itemId, std::uint32_t count)
{
	return { iggy::ResourceId { itemId }, count };
}

iggy::InventoryState2D Inventory(std::vector<iggy::InventoryItemStack2D> stacks)
{
	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);
	Expect(result.built, "inventory add item fixture should build");
	return result.inventory;
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

void ExpectInventory(
	const iggy::InventoryState2D &actual,
	const std::vector<iggy::InventoryItemStack2D> &expected,
	const char *message)
{
	Expect(SameStacks(actual.stacks, expected), message);
}

void TestEmptyItemIdInvalid()
{
	const iggy::InventoryState2D inventory = Inventory({ Stack("item:potion", 3) });

	const iggy::InventoryAddItem2DResult result = iggy::InventoryAddItem2D {}.add(inventory, {}, 1);

	Expect(result.status == iggy::InventoryAddItem2DStatus::InvalidItemId, "empty item add should return InvalidItemId");
	Expect(!result.changed, "empty item add should not change inventory");
	Expect(result.itemId.empty(), "empty item add should preserve requested item id");
	Expect(result.count == 1, "empty item add should preserve requested count");
	ExpectInventory(result.inventory, inventory.stacks, "empty item add should return copied original inventory");
}

void TestZeroCountInvalid()
{
	const iggy::InventoryState2D inventory = Inventory({ Stack("item:potion", 3) });

	const iggy::InventoryAddItem2DResult result =
		iggy::InventoryAddItem2D {}.add(inventory, iggy::ResourceId { "item:potion" }, 0);

	Expect(result.status == iggy::InventoryAddItem2DStatus::InvalidCount, "zero count add should return InvalidCount");
	Expect(!result.changed, "zero count add should not change inventory");
	Expect(result.itemId == iggy::ResourceId { "item:potion" }, "zero count add should preserve requested item id");
	Expect(result.count == 0, "zero count add should preserve requested count");
	ExpectInventory(result.inventory, inventory.stacks, "zero count add should return copied original inventory");
}

void TestAddToEmptyInventoryAppendsStack()
{
	const iggy::InventoryState2D inventory;

	const iggy::InventoryAddItem2DResult result =
		iggy::InventoryAddItem2D {}.add(inventory, iggy::ResourceId { "item:potion" }, 2);

	Expect(result.status == iggy::InventoryAddItem2DStatus::Added, "empty inventory add should return Added");
	Expect(result.changed, "empty inventory add should mark changed");
	Expect(result.itemId == iggy::ResourceId { "item:potion" }, "empty inventory add should preserve requested item id");
	Expect(result.count == 2, "empty inventory add should preserve requested count");
	ExpectInventory(result.inventory, { Stack("item:potion", 2) }, "empty inventory add should append stack");
	Expect(inventory.stacks.empty(), "empty inventory add should not mutate original inventory");
}

void TestAddToExistingStackIncrementsCountAndPreservesOrder()
{
	const iggy::InventoryState2D inventory = Inventory({
		Stack("item:potion", 3),
		Stack("item:key", 1),
		Stack("gold", 25),
	});

	const iggy::InventoryAddItem2DResult result =
		iggy::InventoryAddItem2D {}.add(inventory, iggy::ResourceId { "item:key" }, 4);

	Expect(result.status == iggy::InventoryAddItem2DStatus::Added, "existing stack add should return Added");
	Expect(result.changed, "existing stack add should mark changed");
	ExpectInventory(
		result.inventory,
		{
			Stack("item:potion", 3),
			Stack("item:key", 5),
			Stack("gold", 25),
		},
		"existing stack add should increment count and preserve order");
}

void TestAddMissingItemAppendsAfterExistingStacks()
{
	const iggy::InventoryState2D inventory = Inventory({
		Stack("item:potion", 3),
		Stack("item:key", 1),
	});

	const iggy::InventoryAddItem2DResult result =
		iggy::InventoryAddItem2D {}.add(inventory, iggy::ResourceId { "item:scroll" }, 2);

	Expect(result.status == iggy::InventoryAddItem2DStatus::Added, "missing item add should return Added");
	Expect(result.changed, "missing item add should mark changed");
	ExpectInventory(
		result.inventory,
		{
			Stack("item:potion", 3),
			Stack("item:key", 1),
			Stack("item:scroll", 2),
		},
		"missing item add should append stack after existing stacks");
}

void TestNamespacedAndUnqualifiedItemIdsRemainDistinct()
{
	const iggy::InventoryState2D inventory = Inventory({
		Stack("potion", 1),
	});

	const iggy::InventoryAddItem2DResult result =
		iggy::InventoryAddItem2D {}.add(inventory, iggy::ResourceId { "item:potion" }, 2);

	Expect(result.status == iggy::InventoryAddItem2DStatus::Added, "namespaced item add should return Added");
	ExpectInventory(
		result.inventory,
		{
			Stack("potion", 1),
			Stack("item:potion", 2),
		},
		"namespaced and unqualified item ids should remain distinct");
}

void TestOriginalInventoryIsNotMutated()
{
	const iggy::InventoryState2D inventory = Inventory({
		Stack("item:potion", 3),
		Stack("item:key", 1),
	});
	const iggy::InventoryState2D before = inventory;

	const iggy::InventoryAddItem2DResult result =
		iggy::InventoryAddItem2D {}.add(inventory, iggy::ResourceId { "item:potion" }, 2);

	Expect(result.status == iggy::InventoryAddItem2DStatus::Added, "inventory immutability setup should add item");
	ExpectInventory(inventory, before.stacks, "inventory add item should not mutate original inventory");
}

} // namespace

int main()
{
	TestEmptyItemIdInvalid();
	TestZeroCountInvalid();
	TestAddToEmptyInventoryAppendsStack();
	TestAddToExistingStackIncrementsCountAndPreservesOrder();
	TestAddMissingItemAppendsAfterExistingStacks();
	TestNamespacedAndUnqualifiedItemIdsRemainDistinct();
	TestOriginalInventoryIsNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
