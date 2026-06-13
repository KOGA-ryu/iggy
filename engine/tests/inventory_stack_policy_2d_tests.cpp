#include <cstdlib>
#include <vector>

#include "scene/inventory/InventoryStackPolicy2D.hpp"
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
	Expect(result.built, "inventory stack policy inventory fixture should build");
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
	Expect(result.built, "inventory stack policy catalog fixture should build");
	return result.catalog;
}

void ExpectDefinition(
	const iggy::ItemDefinition2D &actual,
	const iggy::ItemDefinition2D &expected,
	const char *message)
{
	Expect(actual.itemId == expected.itemId, message);
	Expect(actual.displayName == expected.displayName, message);
	Expect(actual.maxStackCount == expected.maxStackCount, message);
	Expect(actual.kind == expected.kind, message);
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

void TestCanAddToMissingStackWithinMax()
{
	const iggy::ItemDefinition2D definition = Definition("item:potion", "Potion", 5);
	const iggy::InventoryState2D inventory = Inventory({});
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ definition });

	const iggy::InventoryStackPolicy2DResult result =
		iggy::InventoryStackPolicy2D {}.planAdd(inventory, catalog, Id("item:potion"), 3);

	Expect(result.status == iggy::InventoryStackPolicy2DStatus::CanAdd, "missing stack within max should be addable");
	Expect(result.canAdd(), "canAdd should be true for CanAdd status");
	Expect(result.itemId == Id("item:potion"), "missing stack result should preserve requested item id");
	Expect(result.requestedCount == 3, "missing stack result should preserve requested count");
	Expect(result.currentCount == 0, "missing stack result should report zero current count");
	Expect(result.maxStackCount == 5, "missing stack result should report definition max stack");
	Expect(result.resultingCount == 3, "missing stack result should report resulting count");
	ExpectDefinition(result.definition, definition, "missing stack result should copy definition");
}

void TestCanAddToExistingStackExactlyToMax()
{
	const iggy::ItemDefinition2D definition = Definition("item:potion", "Potion", 5);
	const iggy::InventoryState2D inventory = Inventory({ Stack("item:potion", 2) });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ definition });

	const iggy::InventoryStackPolicy2DResult result =
		iggy::InventoryStackPolicy2D {}.planAdd(inventory, catalog, Id("item:potion"), 3);

	Expect(result.status == iggy::InventoryStackPolicy2DStatus::CanAdd, "existing stack exactly at max should be addable");
	Expect(result.canAdd(), "canAdd should be true when existing stack reaches max exactly");
	Expect(result.currentCount == 2, "existing stack result should preserve current count");
	Expect(result.maxStackCount == 5, "existing stack result should preserve max stack");
	Expect(result.resultingCount == 5, "existing stack result should preserve resulting max count");
	ExpectDefinition(result.definition, definition, "existing stack result should copy definition");
}

void TestExceedingMaxIsRejectedForExistingStack()
{
	const iggy::ItemDefinition2D definition = Definition("item:potion", "Potion", 5);
	const iggy::InventoryState2D inventory = Inventory({ Stack("item:potion", 4) });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ definition });

	const iggy::InventoryStackPolicy2DResult result =
		iggy::InventoryStackPolicy2D {}.planAdd(inventory, catalog, Id("item:potion"), 2);

	Expect(result.status == iggy::InventoryStackPolicy2DStatus::StackLimitExceeded, "existing stack over max should be rejected");
	Expect(!result.canAdd(), "canAdd should be false for stack limit exceeded");
	Expect(result.currentCount == 4, "existing over-max result should preserve current count");
	Expect(result.maxStackCount == 5, "existing over-max result should preserve max stack");
	Expect(result.resultingCount == 6, "existing over-max result should expose attempted resulting count");
	ExpectDefinition(result.definition, definition, "existing over-max result should copy definition");
}

void TestExceedingMaxIsRejectedForMissingStack()
{
	const iggy::ItemDefinition2D definition = Definition("item:potion", "Potion", 5);
	const iggy::InventoryState2D inventory = Inventory({});
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ definition });

	const iggy::InventoryStackPolicy2DResult result =
		iggy::InventoryStackPolicy2D {}.planAdd(inventory, catalog, Id("item:potion"), 6);

	Expect(result.status == iggy::InventoryStackPolicy2DStatus::StackLimitExceeded, "missing stack over max should be rejected");
	Expect(!result.canAdd(), "canAdd should be false for missing stack over max");
	Expect(result.currentCount == 0, "missing over-max result should preserve zero current count");
	Expect(result.maxStackCount == 5, "missing over-max result should preserve max stack");
	Expect(result.resultingCount == 6, "missing over-max result should expose attempted resulting count");
	ExpectDefinition(result.definition, definition, "missing over-max result should copy definition");
}

void TestEmptyItemIdReturnsMissingItemId()
{
	const iggy::InventoryState2D inventory = Inventory({});
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion") });

	const iggy::InventoryStackPolicy2DResult result =
		iggy::InventoryStackPolicy2D {}.planAdd(inventory, catalog, Id(""), 1);

	Expect(result.status == iggy::InventoryStackPolicy2DStatus::MissingItemId, "empty item id should return MissingItemId");
	Expect(!result.canAdd(), "canAdd should be false for missing item id");
	Expect(result.itemId.empty(), "missing item id result should preserve empty requested id");
	Expect(result.requestedCount == 1, "missing item id result should preserve requested count");
	Expect(result.currentCount == 0 && result.maxStackCount == 0 && result.resultingCount == 0, "missing item id result should not invent count diagnostics");
}

void TestZeroCountReturnsInvalidCount()
{
	const iggy::InventoryState2D inventory = Inventory({});
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:potion") });

	const iggy::InventoryStackPolicy2DResult result =
		iggy::InventoryStackPolicy2D {}.planAdd(inventory, catalog, Id("item:potion"), 0);

	Expect(result.status == iggy::InventoryStackPolicy2DStatus::InvalidCount, "zero count should return InvalidCount");
	Expect(!result.canAdd(), "canAdd should be false for invalid count");
	Expect(result.itemId == Id("item:potion"), "invalid count result should preserve item id");
	Expect(result.requestedCount == 0, "invalid count result should preserve zero requested count");
	Expect(result.currentCount == 0 && result.maxStackCount == 0 && result.resultingCount == 0, "invalid count result should not invent count diagnostics");
}

void TestMissingDefinitionReturnsItemDefinitionNotFound()
{
	const iggy::InventoryState2D inventory = Inventory({ Stack("item:potion", 2) });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ Definition("item:scroll") });

	const iggy::InventoryStackPolicy2DResult result =
		iggy::InventoryStackPolicy2D {}.planAdd(inventory, catalog, Id("item:potion"), 1);

	Expect(result.status == iggy::InventoryStackPolicy2DStatus::ItemDefinitionNotFound, "missing definition should return ItemDefinitionNotFound");
	Expect(!result.canAdd(), "canAdd should be false for missing definition");
	Expect(result.itemId == Id("item:potion"), "missing definition result should preserve item id");
	Expect(result.requestedCount == 1, "missing definition result should preserve requested count");
	Expect(result.currentCount == 0 && result.maxStackCount == 0 && result.resultingCount == 0, "missing definition result should not invent count diagnostics");
}

void TestNamespacedAndUnqualifiedIdsRemainDistinct()
{
	const iggy::InventoryState2D inventory = Inventory({ Stack("potion", 1) });
	const iggy::ItemDefinition2DCatalog catalog = Catalog({
		Definition("potion", "Potion", 2),
		Definition("item:potion", "Namespaced Potion", 5),
	});

	const iggy::InventoryStackPolicy2DResult unqualified =
		iggy::InventoryStackPolicy2D {}.planAdd(inventory, catalog, Id("potion"), 1);
	const iggy::InventoryStackPolicy2DResult namespaced =
		iggy::InventoryStackPolicy2D {}.planAdd(inventory, catalog, Id("item:potion"), 4);

	Expect(unqualified.status == iggy::InventoryStackPolicy2DStatus::CanAdd, "unqualified item id should use unqualified definition");
	Expect(unqualified.currentCount == 1 && unqualified.maxStackCount == 2 && unqualified.resultingCount == 2, "unqualified item id should preserve exact stack diagnostics");
	Expect(namespaced.status == iggy::InventoryStackPolicy2DStatus::CanAdd, "namespaced item id should use namespaced definition");
	Expect(namespaced.currentCount == 0 && namespaced.maxStackCount == 5 && namespaced.resultingCount == 4, "namespaced item id should not use unqualified stack");
}

void TestUnknownKindDefinitionIsValidPolicyData()
{
	const iggy::ItemDefinition2D definition =
		Definition("item:mystery", "Mystery", 2, iggy::ItemDefinition2DKind::Unknown);
	const iggy::InventoryState2D inventory = Inventory({});
	const iggy::ItemDefinition2DCatalog catalog = Catalog({ definition });

	const iggy::InventoryStackPolicy2DResult result =
		iggy::InventoryStackPolicy2D {}.planAdd(inventory, catalog, Id("item:mystery"), 2);

	Expect(result.status == iggy::InventoryStackPolicy2DStatus::CanAdd, "unknown kind should remain valid when catalog definition is valid");
	Expect(result.canAdd(), "canAdd should be true for valid unknown kind definition");
	ExpectDefinition(result.definition, definition, "unknown kind result should preserve definition");
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

	const iggy::InventoryStackPolicy2DResult result =
		iggy::InventoryStackPolicy2D {}.planAdd(inventory, catalog, Id("item:potion"), 3);

	Expect(result.status == iggy::InventoryStackPolicy2DStatus::CanAdd, "immutability setup should be addable");
	Expect(SameStacks(inventory.stacks, stacks), "stack policy should not mutate inventory input");
	Expect(SameDefinitions(catalog.entries, definitions), "stack policy should not mutate catalog input");
}

} // namespace

int main()
{
	TestCanAddToMissingStackWithinMax();
	TestCanAddToExistingStackExactlyToMax();
	TestExceedingMaxIsRejectedForExistingStack();
	TestExceedingMaxIsRejectedForMissingStack();
	TestEmptyItemIdReturnsMissingItemId();
	TestZeroCountReturnsInvalidCount();
	TestMissingDefinitionReturnsItemDefinitionNotFound();
	TestNamespacedAndUnqualifiedIdsRemainDistinct();
	TestUnknownKindDefinitionIsValidPolicyData();
	TestInputsAreNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
