#include <cstdlib>
#include <vector>

#include "scene/inventory/ItemDefinition2D.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ItemDefinition2D Definition(
	const char *itemId,
	const char *displayName = "Potion",
	std::uint32_t maxStackCount = 1,
	iggy::ItemDefinition2DKind kind = iggy::ItemDefinition2DKind::Consumable)
{
	return {
		iggy::ResourceId { itemId },
		displayName,
		maxStackCount,
		kind,
	};
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

void TestEmptyInputBuildsValidEmptyCatalog()
{
	const iggy::ItemDefinition2DCatalogBuildResult result = iggy::ItemDefinition2DCatalogBuilder {}.build({});

	Expect(result.built, "empty item definition input should build");
	Expect(result.issues.empty(), "empty item definition input should have no issues");
	Expect(result.catalog.entries.empty(), "empty item definition input should produce empty catalog");
	Expect(!result.catalog.contains(iggy::ResourceId { "item:missing" }), "empty item definition catalog should not contain missing id");
	Expect(result.catalog.find(iggy::ResourceId { "item:missing" }) == nullptr, "empty item definition catalog should return null for missing id");
}

void TestSuccessfulBuildPreservesOrderAndFields()
{
	const std::vector<iggy::ItemDefinition2D> definitions {
		Definition("item:potion", "Potion", 10, iggy::ItemDefinition2DKind::Consumable),
		Definition("item:key", "Rusty Key", 1, iggy::ItemDefinition2DKind::KeyItem),
		Definition("ore", "Iron Ore", 99, iggy::ItemDefinition2DKind::Material),
		Definition("item:sword", "Short Sword", 1, iggy::ItemDefinition2DKind::Equipment),
		Definition("item:unknown", "Mystery Item", 4, iggy::ItemDefinition2DKind::Unknown),
	};

	const iggy::ItemDefinition2DCatalogBuildResult result =
		iggy::ItemDefinition2DCatalogBuilder {}.build(definitions);

	Expect(result.built, "valid item definitions should build");
	Expect(result.issues.empty(), "valid item definitions should have no issues");
	Expect(result.catalog.entries.size() == definitions.size(), "valid item definition catalog should preserve entry count");
	if (result.catalog.entries.size() == definitions.size()) {
		ExpectDefinition(result.catalog.entries[0], definitions[0], "first item definition should preserve fields");
		ExpectDefinition(result.catalog.entries[1], definitions[1], "second item definition should preserve fields");
		ExpectDefinition(result.catalog.entries[2], definitions[2], "third item definition should preserve fields");
		ExpectDefinition(result.catalog.entries[3], definitions[3], "fourth item definition should preserve fields");
		ExpectDefinition(result.catalog.entries[4], definitions[4], "unknown item kind should be valid and preserved");
	}
}

void TestFindAndContainsUseExactItemIds()
{
	const std::vector<iggy::ItemDefinition2D> definitions {
		Definition("item:potion", "Potion", 10),
		Definition("item:scroll", "Town Portal", 20),
	};
	const iggy::ItemDefinition2DCatalogBuildResult result =
		iggy::ItemDefinition2DCatalogBuilder {}.build(definitions);

	Expect(result.built, "item definition lookup setup should build");
	Expect(result.catalog.contains(iggy::ResourceId { "item:potion" }), "item definition catalog should contain exact item id");
	Expect(!result.catalog.contains(iggy::ResourceId { "item:missing" }), "item definition catalog should not contain missing item id");
	const iggy::ItemDefinition2D *definition = result.catalog.find(iggy::ResourceId { "item:scroll" });
	Expect(definition != nullptr, "item definition catalog should find exact item id");
	if (definition != nullptr)
		Expect(definition->displayName == "Town Portal" && definition->maxStackCount == 20, "item definition find should return matching payload");
	Expect(result.catalog.find(iggy::ResourceId { "item:missing" }) == nullptr, "item definition find should return null for missing id");
}

void TestEmptyItemIdFails()
{
	const iggy::ItemDefinition2D definition = Definition("", "Nameless", 1);

	const iggy::ItemDefinition2DCatalogBuildResult result =
		iggy::ItemDefinition2DCatalogBuilder {}.build({ definition });

	Expect(!result.built, "empty item definition id should fail build");
	Expect(result.catalog.entries.empty(), "failed empty-id build should not publish definitions");
	Expect(result.issues.size() == 1, "empty item definition id should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::ItemDefinition2DIssueCode::EmptyItemId, "empty item issue should use EmptyItemId code");
		Expect(result.issues[0].entryIndex == 0, "empty item issue should preserve entry index");
		ExpectDefinition(result.issues[0].entry, definition, "empty item issue should preserve entry payload");
	}
}

void TestDuplicateItemIdFailsForLaterEntry()
{
	const std::vector<iggy::ItemDefinition2D> definitions {
		Definition("item:potion", "Potion", 10),
		Definition("item:potion", "Better Potion", 20),
	};

	const iggy::ItemDefinition2DCatalogBuildResult result =
		iggy::ItemDefinition2DCatalogBuilder {}.build(definitions);

	Expect(!result.built, "duplicate item definition id should fail build");
	Expect(result.catalog.entries.empty(), "failed duplicate-id build should not publish definitions");
	Expect(result.issues.size() == 1, "duplicate item definition id should report one issue for later duplicate");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::ItemDefinition2DIssueCode::DuplicateItemId, "duplicate item issue should use DuplicateItemId code");
		Expect(result.issues[0].entryIndex == 1, "duplicate item issue should preserve later entry index");
		ExpectDefinition(result.issues[0].entry, definitions[1], "duplicate item issue should preserve later entry payload");
	}
}

void TestEmptyDisplayNameFails()
{
	const iggy::ItemDefinition2D definition = Definition("item:blank", "", 1);

	const iggy::ItemDefinition2DCatalogBuildResult result =
		iggy::ItemDefinition2DCatalogBuilder {}.build({ definition });

	Expect(!result.built, "empty display name should fail build");
	Expect(result.catalog.entries.empty(), "failed empty-name build should not publish definitions");
	Expect(result.issues.size() == 1, "empty display name should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::ItemDefinition2DIssueCode::EmptyDisplayName, "empty display name issue should use EmptyDisplayName code");
		Expect(result.issues[0].entryIndex == 0, "empty display name issue should preserve entry index");
		ExpectDefinition(result.issues[0].entry, definition, "empty display name issue should preserve entry payload");
	}
}

void TestZeroMaxStackCountFails()
{
	const iggy::ItemDefinition2D definition = Definition("item:empty_stack", "Empty Stack", 0);

	const iggy::ItemDefinition2DCatalogBuildResult result =
		iggy::ItemDefinition2DCatalogBuilder {}.build({ definition });

	Expect(!result.built, "zero max stack count should fail build");
	Expect(result.catalog.entries.empty(), "failed zero-stack build should not publish definitions");
	Expect(result.issues.size() == 1, "zero max stack count should report one issue");
	if (result.issues.size() == 1) {
		Expect(result.issues[0].code == iggy::ItemDefinition2DIssueCode::ZeroMaxStackCount, "zero max stack issue should use ZeroMaxStackCount code");
		Expect(result.issues[0].entryIndex == 0, "zero max stack issue should preserve entry index");
		ExpectDefinition(result.issues[0].entry, definition, "zero max stack issue should preserve entry payload");
	}
}

void TestMultipleIssuesAreReportedInDeterministicInputOrder()
{
	const std::vector<iggy::ItemDefinition2D> definitions {
		Definition("item:potion", "Potion", 10),
		Definition("", "", 0),
		Definition("item:potion", "", 0),
		Definition("item:potion", "Duplicate Potion", 5),
	};

	const iggy::ItemDefinition2DCatalogBuildResult result =
		iggy::ItemDefinition2DCatalogBuilder {}.build(definitions);

	Expect(!result.built, "multiple item definition issues should fail build");
	Expect(result.catalog.entries.empty(), "failed multi-issue definition build should not publish definitions");
	Expect(result.issues.size() == 7, "multiple item definition issues should all be reported");
	if (result.issues.size() == 7) {
		Expect(result.issues[0].code == iggy::ItemDefinition2DIssueCode::EmptyItemId && result.issues[0].entryIndex == 1, "empty item id should be first issue for second definition");
		Expect(result.issues[1].code == iggy::ItemDefinition2DIssueCode::EmptyDisplayName && result.issues[1].entryIndex == 1, "empty display name should follow empty id for second definition");
		Expect(result.issues[2].code == iggy::ItemDefinition2DIssueCode::ZeroMaxStackCount && result.issues[2].entryIndex == 1, "zero stack should follow empty display name for second definition");
		Expect(result.issues[3].code == iggy::ItemDefinition2DIssueCode::DuplicateItemId && result.issues[3].entryIndex == 2, "duplicate id should be first issue for third definition");
		Expect(result.issues[4].code == iggy::ItemDefinition2DIssueCode::EmptyDisplayName && result.issues[4].entryIndex == 2, "empty display name should follow duplicate id for third definition");
		Expect(result.issues[5].code == iggy::ItemDefinition2DIssueCode::ZeroMaxStackCount && result.issues[5].entryIndex == 2, "zero stack should follow empty display name for third definition");
		Expect(result.issues[6].code == iggy::ItemDefinition2DIssueCode::DuplicateItemId && result.issues[6].entryIndex == 3, "duplicate id should be reported for fourth definition");
	}
}

void TestNamespacedAndUnqualifiedItemIdsAreDistinct()
{
	const std::vector<iggy::ItemDefinition2D> definitions {
		Definition("potion", "Potion", 10),
		Definition("item:potion", "Namespaced Potion", 10),
	};

	const iggy::ItemDefinition2DCatalogBuildResult result =
		iggy::ItemDefinition2DCatalogBuilder {}.build(definitions);

	Expect(result.built, "namespaced and unqualified item definition ids should build as distinct ids");
	Expect(result.issues.empty(), "namespaced and unqualified item definition ids should not produce duplicate issues");
	Expect(result.catalog.contains(iggy::ResourceId { "potion" }), "item definition catalog should contain unqualified id");
	Expect(result.catalog.contains(iggy::ResourceId { "item:potion" }), "item definition catalog should contain namespaced id");
	const iggy::ItemDefinition2D *unqualified = result.catalog.find(iggy::ResourceId { "potion" });
	const iggy::ItemDefinition2D *namespaced = result.catalog.find(iggy::ResourceId { "item:potion" });
	Expect(unqualified != nullptr && namespaced != nullptr && unqualified != namespaced, "distinct item definition ids should resolve to distinct definitions");
}

void TestInputVectorIsNotMutated()
{
	std::vector<iggy::ItemDefinition2D> definitions {
		Definition("item:potion", "Potion", 10),
		Definition("item:key", "Key", 1, iggy::ItemDefinition2DKind::KeyItem),
	};
	const std::vector<iggy::ItemDefinition2D> before = definitions;

	const iggy::ItemDefinition2DCatalogBuildResult result =
		iggy::ItemDefinition2DCatalogBuilder {}.build(definitions);

	Expect(result.built, "item definition immutability setup should build");
	Expect(SameDefinitions(definitions, before), "item definition builder should not mutate input definitions");
}

} // namespace

int main()
{
	TestEmptyInputBuildsValidEmptyCatalog();
	TestSuccessfulBuildPreservesOrderAndFields();
	TestFindAndContainsUseExactItemIds();
	TestEmptyItemIdFails();
	TestDuplicateItemIdFailsForLaterEntry();
	TestEmptyDisplayNameFails();
	TestZeroMaxStackCountFails();
	TestMultipleIssuesAreReportedInDeterministicInputOrder();
	TestNamespacedAndUnqualifiedItemIdsAreDistinct();
	TestInputVectorIsNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
