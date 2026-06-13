#include <cstdlib>
#include <vector>

#include "scene/inventory/InventoryPolicyAddItem2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

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
	Expect(result.built, "inventory policy add fixture inventory should build");
	return result.inventory;
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
	Expect(result.built, "inventory policy add fixture catalog should build");
	return result.catalog;
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

void ExpectInventory(
	const iggy::InventoryState2D &actual,
	const std::vector<iggy::InventoryItemStack2D> &expected,
	const char *message)
{
	Expect(SameStacks(actual.stacks, expected), message);
}

void ExpectSingleItemAddedEvent(
	const iggy::InventoryPolicyAddItem2DResult &result,
	const iggy::ResourceId &itemId,
	std::uint32_t count,
	const char *message)
{
	Expect(result.events.events.size() == 1, message);
	if (result.events.events.size() == 1) {
		const iggy::InventoryEvent2D &event = result.events.events[0];
		Expect(event.type == iggy::InventoryEvent2DType::ItemAdded, message);
		Expect(event.itemId == itemId, message);
		Expect(event.dropId.empty(), message);
		Expect(event.count == count, message);
	}
}

void TestAddsMissingStackWithinMax()
{
	const iggy::InventoryState2D inventory = Inventory({});
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });

	const iggy::InventoryPolicyAddItem2DResult result =
		iggy::InventoryPolicyAddItem2D {}.add(inventory, catalog, Id("item:potion"), 3);

	Expect(result.status == iggy::InventoryPolicyAddItem2DStatus::Added, "policy add missing stack should return Added");
	Expect(result.added(), "added helper should be true for successful policy add");
	Expect(result.changed, "policy add missing stack should mark changed");
	Expect(result.plan.status == iggy::InventoryStackPolicy2DStatus::CanAdd, "policy add missing stack should preserve CanAdd plan");
	Expect(result.add.status == iggy::InventoryAddItem2DStatus::Added, "policy add missing stack should preserve raw add status");
	ExpectInventory(result.inventory, { Stack("item:potion", 3) }, "policy add missing stack should append item");
	ExpectInventory(result.add.inventory, { Stack("item:potion", 3) }, "policy add missing stack should preserve add inventory");
	ExpectSingleItemAddedEvent(result, Id("item:potion"), 3, "policy add missing stack should preserve ItemAdded event");
}

void TestAddsExistingStackExactlyToMax()
{
	const iggy::InventoryState2D inventory = Inventory({ Stack("item:potion", 2) });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });

	const iggy::InventoryPolicyAddItem2DResult result =
		iggy::InventoryPolicyAddItem2D {}.add(inventory, catalog, Id("item:potion"), 3);

	Expect(result.status == iggy::InventoryPolicyAddItem2DStatus::Added, "policy add existing stack to max should return Added");
	Expect(result.added(), "added helper should be true when existing stack reaches max");
	Expect(result.changed, "policy add existing stack should mark changed");
	Expect(result.plan.currentCount == 2 && result.plan.maxStackCount == 5 && result.plan.resultingCount == 5, "policy add existing stack should preserve plan count diagnostics");
	Expect(result.add.status == iggy::InventoryAddItem2DStatus::Added, "policy add existing stack should preserve raw add status");
	ExpectInventory(result.inventory, { Stack("item:potion", 5) }, "policy add existing stack should increment to max");
	ExpectSingleItemAddedEvent(result, Id("item:potion"), 3, "policy add existing stack should preserve requested ItemAdded count");
}

void TestRejectsMissingStackOverMaxViaPlanFailed()
{
	const iggy::InventoryState2D inventory = Inventory({});
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });

	const iggy::InventoryPolicyAddItem2DResult result =
		iggy::InventoryPolicyAddItem2D {}.add(inventory, catalog, Id("item:potion"), 6);

	Expect(result.status == iggy::InventoryPolicyAddItem2DStatus::PlanFailed, "policy add missing stack over max should return PlanFailed");
	Expect(!result.added(), "added helper should be false for plan failure");
	Expect(!result.changed, "policy add missing stack over max should not mark changed");
	Expect(result.plan.status == iggy::InventoryStackPolicy2DStatus::StackLimitExceeded, "policy add missing stack over max should preserve stack policy failure");
	Expect(result.add.status == iggy::InventoryAddItem2DStatus::InvalidItemId, "policy add plan failure should not run raw add");
	ExpectInventory(result.inventory, inventory.stacks, "policy add missing stack over max should preserve original inventory");
	Expect(result.events.events.empty(), "policy add missing stack over max should record no events");
}

void TestRejectsExistingStackOverMaxViaPlanFailed()
{
	const iggy::InventoryState2D inventory = Inventory({ Stack("item:potion", 4) });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });

	const iggy::InventoryPolicyAddItem2DResult result =
		iggy::InventoryPolicyAddItem2D {}.add(inventory, catalog, Id("item:potion"), 2);

	Expect(result.status == iggy::InventoryPolicyAddItem2DStatus::PlanFailed, "policy add existing stack over max should return PlanFailed");
	Expect(!result.added(), "added helper should be false for existing stack plan failure");
	Expect(!result.changed, "policy add existing stack over max should not mark changed");
	Expect(result.plan.status == iggy::InventoryStackPolicy2DStatus::StackLimitExceeded, "policy add existing stack over max should preserve stack policy failure");
	Expect(result.plan.currentCount == 4 && result.plan.resultingCount == 6, "policy add existing stack over max should preserve count diagnostics");
	ExpectInventory(result.inventory, inventory.stacks, "policy add existing stack over max should preserve original inventory");
	Expect(result.events.events.empty(), "policy add existing stack over max should record no events");
}

void TestMissingDefinitionReturnsPlanFailed()
{
	const iggy::InventoryState2D inventory = Inventory({});
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:scroll", "Scroll", 5) });

	const iggy::InventoryPolicyAddItem2DResult result =
		iggy::InventoryPolicyAddItem2D {}.add(inventory, catalog, Id("item:potion"), 1);

	Expect(result.status == iggy::InventoryPolicyAddItem2DStatus::PlanFailed, "missing definition should return PlanFailed");
	Expect(result.plan.status == iggy::InventoryStackPolicy2DStatus::ItemDefinitionNotFound, "missing definition should preserve plan status");
	ExpectInventory(result.inventory, inventory.stacks, "missing definition should preserve original inventory");
	Expect(result.events.events.empty(), "missing definition should produce no events");
}

void TestEmptyItemIdReturnsPlanFailed()
{
	const iggy::InventoryState2D inventory = Inventory({});
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });

	const iggy::InventoryPolicyAddItem2DResult result =
		iggy::InventoryPolicyAddItem2D {}.add(inventory, catalog, Id(""), 1);

	Expect(result.status == iggy::InventoryPolicyAddItem2DStatus::PlanFailed, "empty item id should return PlanFailed");
	Expect(result.plan.status == iggy::InventoryStackPolicy2DStatus::MissingItemId, "empty item id should preserve plan status");
	Expect(result.plan.itemId.empty(), "empty item id should preserve requested id in plan");
	ExpectInventory(result.inventory, inventory.stacks, "empty item id should preserve original inventory");
	Expect(result.events.events.empty(), "empty item id should produce no events");
}

void TestZeroCountReturnsPlanFailed()
{
	const iggy::InventoryState2D inventory = Inventory({});
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion", "Potion", 5) });

	const iggy::InventoryPolicyAddItem2DResult result =
		iggy::InventoryPolicyAddItem2D {}.add(inventory, catalog, Id("item:potion"), 0);

	Expect(result.status == iggy::InventoryPolicyAddItem2DStatus::PlanFailed, "zero count should return PlanFailed");
	Expect(result.plan.status == iggy::InventoryStackPolicy2DStatus::InvalidCount, "zero count should preserve plan status");
	Expect(result.plan.requestedCount == 0, "zero count should preserve requested count in plan");
	ExpectInventory(result.inventory, inventory.stacks, "zero count should preserve original inventory");
	Expect(result.events.events.empty(), "zero count should produce no events");
}

void TestNamespacedAndUnqualifiedIdsRemainDistinct()
{
	const iggy::InventoryState2D inventory = Inventory({ Stack("potion", 1) });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({
		Definition("potion", "Potion", 2),
		Definition("item:potion", "Namespaced Potion", 3),
	});

	const iggy::InventoryPolicyAddItem2DResult result =
		iggy::InventoryPolicyAddItem2D {}.add(inventory, catalog, Id("item:potion"), 2);

	Expect(result.status == iggy::InventoryPolicyAddItem2DStatus::Added, "namespaced item id should add through namespaced definition");
	Expect(result.plan.currentCount == 0 && result.plan.maxStackCount == 3 && result.plan.resultingCount == 2, "namespaced policy add should not use unqualified stack");
	ExpectInventory(
		result.inventory,
		{
			Stack("potion", 1),
			Stack("item:potion", 2),
		},
		"namespaced policy add should append distinct namespaced stack");
}

void TestInputsAreNotMutated()
{
	const std::vector<iggy::InventoryItemStack2D> stacks {
		Stack("item:potion", 2),
		Stack("item:key", 1),
	};
	const std::vector<iggy::ItemDefinition2D> definitions {
		Definition("item:potion", "Potion", 5),
		Definition("item:key", "Key", 1, iggy::ItemDefinition2DKind::KeyItem),
	};
	iggy::InventoryState2D inventory = Inventory(stacks);
	iggy::ItemDefinition2DCatalog catalog = Catalog(definitions);

	const iggy::InventoryPolicyAddItem2DResult result =
		iggy::InventoryPolicyAddItem2D {}.add(inventory, catalog, Id("item:potion"), 3);

	Expect(result.status == iggy::InventoryPolicyAddItem2DStatus::Added, "immutability setup should add item");
	Expect(SameStacks(inventory.stacks, stacks), "policy add should not mutate inventory input");
	Expect(SameDefinitions(catalog.entries, definitions), "policy add should not mutate catalog input");
}

} // namespace

int main()
{
	TestAddsMissingStackWithinMax();
	TestAddsExistingStackExactlyToMax();
	TestRejectsMissingStackOverMaxViaPlanFailed();
	TestRejectsExistingStackOverMaxViaPlanFailed();
	TestMissingDefinitionReturnsPlanFailed();
	TestEmptyItemIdReturnsPlanFailed();
	TestZeroCountReturnsPlanFailed();
	TestNamespacedAndUnqualifiedIdsRemainDistinct();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
