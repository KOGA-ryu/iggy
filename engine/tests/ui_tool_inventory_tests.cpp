#include <cstdlib>
#include <vector>

#include "scene/ui/UiToolInventory.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

void TestDefaultInventoryProvidesIggyToolVocabulary()
{
	const iggy::ui::UiToolInventory inventory = iggy::ui::defaultUiToolInventory();

	Expect(inventory.rows == 6 && inventory.columns == 6, "default ui tool inventory should define belt dimensions");
	Expect(inventory.tools.size() >= 8, "default ui tool inventory should define useful tools");
	Expect(iggy::ui::findUiTool(inventory, Id("tool:inspect")) != nullptr, "default ui tool inventory should include inspect tool");
	Expect(iggy::ui::findUiTool(inventory, Id("tool:collision")) != nullptr, "default ui tool inventory should include collision debug tool");
	Expect(iggy::ui::findUiTool(inventory, Id("tool:missing")) == nullptr, "default ui tool inventory lookup should return null for missing tool");
}

void TestDefaultEnabledToolIdsUseDescriptorFlags()
{
	iggy::ui::UiToolInventory inventory;
	inventory.tools = {
		{ Id("tool:a"), "A", Id("group:test"), 0, 0, true },
		{ Id("tool:b"), "B", Id("group:test"), 0, 1, false },
		{ Id("tool:c"), "C", Id("group:test"), 0, 2, true },
	};

	const std::vector<iggy::ResourceId> ids = iggy::ui::defaultEnabledUiToolIds(inventory);

	Expect(ids.size() == 2, "default enabled ui tool ids should include enabled descriptors only");
	if (ids.size() == 2) {
		Expect(ids[0] == Id("tool:a"), "default enabled ui tool ids should preserve first enabled id");
		Expect(ids[1] == Id("tool:c"), "default enabled ui tool ids should preserve second enabled id");
	}
}

void TestBuildToolBeltLayoutUsesEnabledIdsAndCoordinates()
{
	iggy::ui::UiToolInventory inventory;
	inventory.rows = 2;
	inventory.columns = 3;
	inventory.tools = {
		{ Id("tool:a"), "A", Id("group:test"), 0, 0, true },
		{ Id("tool:b"), "B", Id("group:test"), 1, 2, true },
		{ Id("tool:c"), "C", Id("group:test"), 9, 9, true },
	};

	const iggy::ui::UiToolBeltLayout layout = iggy::ui::buildUiToolBeltLayout(inventory, { Id("tool:b"), Id("tool:c") });

	Expect(layout.rows == 2 && layout.columns == 3, "ui tool belt layout should preserve inventory dimensions");
	Expect(layout.itemIds.size() == 6, "ui tool belt layout should allocate row-major slots");
	if (layout.itemIds.size() == 6) {
		Expect(layout.itemIds[0].empty(), "ui tool belt layout should leave disabled tool empty");
		Expect(layout.itemIds[5] == Id("tool:b"), "ui tool belt layout should place enabled tool by descriptor coordinate");
	}
}

void TestBuildToolBeltLayoutDoesNotMutateInputs()
{
	const iggy::ui::UiToolInventory inventory = iggy::ui::defaultUiToolInventory();
	const std::vector<iggy::ResourceId> enabled = { Id("tool:select") };

	(void)iggy::ui::buildUiToolBeltLayout(inventory, enabled);

	Expect(inventory.tools.size() == iggy::ui::defaultUiToolInventory().tools.size(), "ui tool belt layout should not mutate inventory");
	Expect(enabled.size() == 1 && enabled[0] == Id("tool:select"), "ui tool belt layout should not mutate enabled id list");
}

} // namespace

int main()
{
	TestDefaultInventoryProvidesIggyToolVocabulary();
	TestDefaultEnabledToolIdsUseDescriptorFlags();
	TestBuildToolBeltLayoutUsesEnabledIdsAndCoordinates();
	TestBuildToolBeltLayoutDoesNotMutateInputs();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
