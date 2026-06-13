#include <cstdlib>
#include <vector>

#include "runtime/RuntimeInventoryState.hpp"
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
	Expect(result.built, "runtime inventory state inventory fixture should build");
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
	const iggy::LevelItemDrop2DRegistryBuildResult result = iggy::LevelItemDrop2DRegistryBuilder {}.build(drops);
	Expect(result.built, "runtime inventory state drop fixture should build");
	return result.registry;
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

void TestDefaultStateIsEmpty()
{
	const iggy::runtime::RuntimeInventoryState state;

	Expect(state.inventory.stacks.empty(), "default runtime inventory state should have empty inventory");
	Expect(state.drops.drops.empty(), "default runtime inventory state should have empty drops");
}

void TestStatePreservesBuiltInventoryAndDrops()
{
	const std::vector<iggy::InventoryItemStack2D> stacks {
		Stack("item:potion", 3),
		Stack("gold", 25),
	};
	const std::vector<iggy::LevelItemDrop2D> drops {
		Drop("drop:potion", "item:potion", 2, { 1.0F, 2.0F }, 0.25F, true),
		Drop("drop:key", "item:key", 1, { -3.0F, 4.0F }, 1.5F, false),
	};

	const iggy::runtime::RuntimeInventoryState state {
		Inventory(stacks),
		Drops(drops),
	};

	Expect(SameStacks(state.inventory.stacks, stacks), "runtime inventory state should preserve built inventory");
	Expect(SameDrops(state.drops.drops, drops), "runtime inventory state should preserve built drops");
}

void TestCopiedStatePreservesEntries()
{
	const std::vector<iggy::InventoryItemStack2D> stacks {
		Stack("item:scroll", 2),
	};
	const std::vector<iggy::LevelItemDrop2D> drops {
		Drop("drop:gem", "item:gem", 1, { 5.0F, 6.0F }, 0.0F, true),
	};
	const iggy::runtime::RuntimeInventoryState original {
		Inventory(stacks),
		Drops(drops),
	};

	const iggy::runtime::RuntimeInventoryState copy = original;

	Expect(SameStacks(copy.inventory.stacks, stacks), "copied runtime inventory state should preserve inventory");
	Expect(SameDrops(copy.drops.drops, drops), "copied runtime inventory state should preserve drops");
}

void TestCopiedStateDoesNotAliasOriginalMutableValues()
{
	const std::vector<iggy::InventoryItemStack2D> originalStacks {
		Stack("item:potion", 3),
	};
	const std::vector<iggy::LevelItemDrop2D> originalDrops {
		Drop("drop:potion", "item:potion", 1),
	};
	const iggy::runtime::RuntimeInventoryState original {
		Inventory(originalStacks),
		Drops(originalDrops),
	};
	iggy::runtime::RuntimeInventoryState copy = original;
	const std::vector<iggy::InventoryItemStack2D> changedStacks {
		Stack("item:key", 1),
		Stack("gold", 10),
	};
	const std::vector<iggy::LevelItemDrop2D> changedDrops {
		Drop("drop:key", "item:key", 1, { 3.0F, 4.0F }, 1.0F, false),
	};

	copy.inventory = Inventory(changedStacks);
	copy.drops = Drops(changedDrops);

	Expect(SameStacks(copy.inventory.stacks, changedStacks), "mutated runtime inventory state copy should hold changed inventory");
	Expect(SameDrops(copy.drops.drops, changedDrops), "mutated runtime inventory state copy should hold changed drops");
	Expect(SameStacks(original.inventory.stacks, originalStacks), "mutating runtime inventory state copy should not mutate original inventory");
	Expect(SameDrops(original.drops.drops, originalDrops), "mutating runtime inventory state copy should not mutate original drops");
}

void TestConstructionDoesNotValidateOrRejectDefaultMembers()
{
	iggy::InventoryState2D invalidLookingInventory;
	invalidLookingInventory.stacks.push_back({ {}, 0 });
	iggy::LevelItemDrop2DRegistry invalidLookingDrops;
	invalidLookingDrops.drops.push_back({});

	const iggy::runtime::RuntimeInventoryState state {
		invalidLookingInventory,
		invalidLookingDrops,
	};

	Expect(state.inventory.stacks.size() == 1, "runtime inventory state should accept inventory values without validation");
	Expect(state.inventory.stacks[0].itemId.empty(), "runtime inventory state should preserve invalid-looking inventory data");
	Expect(state.inventory.stacks[0].count == 0, "runtime inventory state should preserve invalid-looking inventory count");
	Expect(state.drops.drops.size() == 1, "runtime inventory state should accept drop values without validation");
	Expect(state.drops.drops[0].id.empty(), "runtime inventory state should preserve invalid-looking drop data");
	Expect(state.drops.drops[0].count == 0, "runtime inventory state should preserve invalid-looking drop count");
}

} // namespace

int main()
{
	TestDefaultStateIsEmpty();
	TestStatePreservesBuiltInventoryAndDrops();
	TestCopiedStatePreservesEntries();
	TestCopiedStateDoesNotAliasOriginalMutableValues();
	TestConstructionDoesNotValidateOrRejectDefaultMembers();

	return Failures;
}
