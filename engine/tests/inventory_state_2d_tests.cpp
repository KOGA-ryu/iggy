#include <cstdlib>
#include <vector>

#include "scene/inventory/InventoryState2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::InventoryItemStack2D Stack(const char *itemId, std::uint32_t count)
{
	return { iggy::ResourceId { itemId }, count };
}

void ExpectStack(const iggy::InventoryItemStack2D &actual, const iggy::InventoryItemStack2D &expected, const char *message)
{
	Expect(actual.itemId == expected.itemId, message);
	Expect(actual.count == expected.count, message);
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

void TestEmptyInputBuildsValidEmptyInventory()
{
	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build({});

	Expect(result.built, "empty inventory input should build");
	Expect(result.issues.empty(), "empty inventory input should have no issues");
	Expect(result.inventory.stacks.empty(), "empty inventory input should produce empty inventory");
	Expect(!result.inventory.contains(iggy::ResourceId { "item:missing" }), "empty inventory should not contain missing item");
	Expect(result.inventory.find(iggy::ResourceId { "item:missing" }) == nullptr, "empty inventory should return null for missing item");
}

void TestSuccessfulBuildPreservesOrderAndFields()
{
	const std::vector<iggy::InventoryItemStack2D> stacks {
		Stack("item:potion", 3),
		Stack("item:key", 1),
		Stack("gold", 25),
	};

	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);

	Expect(result.built, "valid inventory stacks should build");
	Expect(result.issues.empty(), "valid inventory stacks should have no issues");
	Expect(result.inventory.stacks.size() == stacks.size(), "valid inventory should preserve stack count");
	if (result.inventory.stacks.size() == stacks.size()) {
		ExpectStack(result.inventory.stacks[0], stacks[0], "first inventory stack should preserve fields");
		ExpectStack(result.inventory.stacks[1], stacks[1], "second inventory stack should preserve fields");
		ExpectStack(result.inventory.stacks[2], stacks[2], "third inventory stack should preserve fields");
	}
}

void TestFindAndContainsUseExactIds()
{
	const std::vector<iggy::InventoryItemStack2D> stacks {
		Stack("item:potion", 4),
		Stack("item:scroll", 2),
	};
	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);

	Expect(result.built, "inventory lookup setup should build");
	Expect(result.inventory.contains(iggy::ResourceId { "item:potion" }), "inventory should contain exact item id");
	Expect(!result.inventory.contains(iggy::ResourceId { "item:missing" }), "inventory should not contain missing item id");
	const iggy::InventoryItemStack2D *stack = result.inventory.find(iggy::ResourceId { "item:scroll" });
	Expect(stack != nullptr, "inventory should find exact item id");
	if (stack != nullptr)
		Expect(stack->count == 2, "inventory find should return matching stack payload");
	Expect(result.inventory.find(iggy::ResourceId { "item:missing" }) == nullptr, "inventory find should return null for missing id");
}

void TestEmptyItemIdFails()
{
	const iggy::InventoryItemStack2D stack = Stack("", 1);

	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build({ stack });

	Expect(!result.built, "empty inventory item id should fail build");
	Expect(result.inventory.stacks.empty(), "failed empty-item build should not publish inventory stacks");
	Expect(result.issues.size() == 1, "empty inventory item id should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::InventoryState2DIssueCode::EmptyItemId, "empty item issue should use EmptyItemId code");
		Expect(result.issues[0].stackIndex == 0, "empty item issue should preserve stack index");
		ExpectStack(result.issues[0].stack, stack, "empty item issue should preserve stack payload");
	}
}

void TestZeroCountFails()
{
	const iggy::InventoryItemStack2D stack = Stack("item:empty", 0);

	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build({ stack });

	Expect(!result.built, "zero inventory count should fail build");
	Expect(result.inventory.stacks.empty(), "failed zero-count build should not publish inventory stacks");
	Expect(result.issues.size() == 1, "zero inventory count should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::InventoryState2DIssueCode::ZeroCount, "zero count issue should use ZeroCount code");
		Expect(result.issues[0].stackIndex == 0, "zero count issue should preserve stack index");
		ExpectStack(result.issues[0].stack, stack, "zero count issue should preserve stack payload");
	}
}

void TestDuplicateItemIdFailsForLaterStack()
{
	const std::vector<iggy::InventoryItemStack2D> stacks {
		Stack("item:potion", 1),
		Stack("item:potion", 5),
	};

	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);

	Expect(!result.built, "duplicate inventory item id should fail build");
	Expect(result.inventory.stacks.empty(), "failed duplicate-item build should not publish inventory stacks");
	Expect(result.issues.size() == 1, "duplicate inventory item id should report one issue for later duplicate");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::InventoryState2DIssueCode::DuplicateItemId, "duplicate item issue should use DuplicateItemId code");
		Expect(result.issues[0].stackIndex == 1, "duplicate item issue should preserve later stack index");
		ExpectStack(result.issues[0].stack, stacks[1], "duplicate item issue should preserve later stack payload");
	}
}

void TestMultipleIssuesAreReportedInDeterministicInputOrder()
{
	const std::vector<iggy::InventoryItemStack2D> stacks {
		Stack("item:potion", 2),
		Stack("", 0),
		Stack("item:potion", 0),
		Stack("item:potion", 7),
	};

	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);

	Expect(!result.built, "multiple inventory issues should fail build");
	Expect(result.inventory.stacks.empty(), "failed multi-issue inventory build should not publish inventory stacks");
	Expect(result.issues.size() == 5, "multiple inventory issues should all be reported");
	if (result.issues.size() == 5) {
		Expect(result.issues[0].code == iggy::InventoryState2DIssueCode::EmptyItemId && result.issues[0].stackIndex == 1, "empty item id should be first issue for second stack");
		Expect(result.issues[1].code == iggy::InventoryState2DIssueCode::ZeroCount && result.issues[1].stackIndex == 1, "zero count should follow empty id for second stack");
		Expect(result.issues[2].code == iggy::InventoryState2DIssueCode::ZeroCount && result.issues[2].stackIndex == 2, "zero count should be first issue for third stack");
		Expect(result.issues[3].code == iggy::InventoryState2DIssueCode::DuplicateItemId && result.issues[3].stackIndex == 2, "duplicate item should follow zero count for third stack");
		Expect(result.issues[4].code == iggy::InventoryState2DIssueCode::DuplicateItemId && result.issues[4].stackIndex == 3, "duplicate item should be reported for fourth stack");
	}
}

void TestNamespacedAndUnqualifiedIdsAreDistinct()
{
	const std::vector<iggy::InventoryItemStack2D> stacks {
		Stack("potion", 1),
		Stack("item:potion", 1),
	};

	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);

	Expect(result.built, "namespaced and unqualified inventory item ids should build as distinct ids");
	Expect(result.issues.empty(), "namespaced and unqualified inventory item ids should not produce duplicate issues");
	Expect(result.inventory.contains(iggy::ResourceId { "potion" }), "inventory should contain unqualified item id");
	Expect(result.inventory.contains(iggy::ResourceId { "item:potion" }), "inventory should contain namespaced item id");
	const iggy::InventoryItemStack2D *unqualified = result.inventory.find(iggy::ResourceId { "potion" });
	const iggy::InventoryItemStack2D *namespaced = result.inventory.find(iggy::ResourceId { "item:potion" });
	Expect(unqualified != nullptr && namespaced != nullptr && unqualified != namespaced, "distinct inventory item ids should resolve to distinct stacks");
}

void TestInputVectorIsNotMutated()
{
	std::vector<iggy::InventoryItemStack2D> stacks {
		Stack("item:potion", 3),
		Stack("item:key", 1),
	};
	const std::vector<iggy::InventoryItemStack2D> before = stacks;

	const iggy::InventoryState2DBuildResult result = iggy::InventoryState2DBuilder {}.build(stacks);

	Expect(result.built, "inventory immutability setup should build");
	Expect(SameStacks(stacks, before), "inventory builder should not mutate input stacks");
}

} // namespace

int main()
{
	TestEmptyInputBuildsValidEmptyInventory();
	TestSuccessfulBuildPreservesOrderAndFields();
	TestFindAndContainsUseExactIds();
	TestEmptyItemIdFails();
	TestZeroCountFails();
	TestDuplicateItemIdFailsForLaterStack();
	TestMultipleIssuesAreReportedInDeterministicInputOrder();
	TestNamespacedAndUnqualifiedIdsAreDistinct();
	TestInputVectorIsNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
