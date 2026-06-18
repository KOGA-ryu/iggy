#include <cstdlib>
#include <vector>

#include "scene/ui/UiSettingsState.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

void TestDefaultSettingsStateUsesToolInventoryAndPanelDefaults()
{
	const iggy::ui::UiToolInventory inventory = iggy::ui::defaultUiToolInventory();
	const iggy::ui::UiSettingsState settings = iggy::ui::defaultUiSettingsState(inventory);

	Expect(settings.themeId == Id("theme:default"), "default ui settings should name default theme");
	Expect(settings.theme.base == "#101418", "default ui settings should own theme inputs");
	Expect(settings.activePageId == Id("settings:theme"), "default ui settings should open theme page first");
	Expect(settings.enabledToolIds.size() == iggy::ui::defaultEnabledUiToolIds(inventory).size(), "default ui settings should derive enabled tools from inventory");
	Expect(settings.panelContent.size() >= 3, "default ui settings should include panel assignments");
	Expect(settings.showRuntimeInspector, "default ui settings should show runtime inspector");
	Expect(settings.showInteractionEvents, "default ui settings should show interaction events");
	Expect(settings.showAuthoringPreview, "default ui settings should allow authoring preview");
	Expect(!settings.showCollisionDebug, "default ui settings should hide collision debug");
}

void TestDefaultSettingsPagesAreTableDriven()
{
	const std::vector<iggy::ui::UiSettingsPageDescriptor> pages = iggy::ui::defaultUiSettingsPages();

	Expect(pages.size() == 3, "default ui settings pages should be a compact page table");
	if (pages.size() == 3) {
		Expect(pages[0].id == Id("settings:theme") && pages[0].label == "Theme", "first settings page should edit theme");
		Expect(pages[1].id == Id("settings:tool_belt") && pages[1].label == "Tool Belt", "second settings page should edit tool belt");
		Expect(pages[2].id == Id("settings:panels") && pages[2].label == "Panels", "third settings page should edit panel assignments");
	}
}

void TestApplySettingsRebuildsWorkspaceBeltAndPanelContent()
{
	iggy::ui::UiToolInventory inventory;
	inventory.rows = 1;
	inventory.columns = 3;
	inventory.tools = {
		{ Id("tool:a"), "A", Id("group:test"), 0, 0, true },
		{ Id("tool:b"), "B", Id("group:test"), 0, 1, true },
		{ Id("tool:c"), "C", Id("group:test"), 0, 2, true },
	};
	iggy::ui::UiSettingsState settings;
	settings.enabledToolIds = { Id("tool:b") };
	settings.panelContent = {
		{ Id("panel:test"), iggy::ui::UiShellSlot::Bottom, false },
	};
	iggy::ui::UiWorkspaceLayout workspace;
	workspace.id = Id("workspace:test");
	workspace.label = "Test";

	const iggy::ui::UiWorkspaceLayout applied = iggy::ui::applyUiSettingsToWorkspace(workspace, settings, inventory);

	Expect(applied.id == workspace.id && applied.label == workspace.label, "ui settings apply should preserve workspace identity");
	Expect(applied.toolBelt.itemIds.size() == 3, "ui settings apply should rebuild tool belt layout");
	if (applied.toolBelt.itemIds.size() == 3) {
		Expect(applied.toolBelt.itemIds[0].empty(), "ui settings apply should leave disabled first tool empty");
		Expect(applied.toolBelt.itemIds[1] == Id("tool:b"), "ui settings apply should enable selected tool");
	}
	Expect(applied.panelContent.size() == 1 && applied.panelContent[0].groupId == Id("panel:test"), "ui settings apply should replace panel content assignments");
}

void TestPanelContentAssignmentHelpersInsertUpdateAndFallback()
{
	iggy::ui::UiWorkspaceLayout workspace;
	const iggy::ui::UiPanelContentAssignment fallback { Id("panel:events"), iggy::ui::UiShellSlot::Bottom, false };

	const iggy::ui::UiPanelContentAssignment missing = iggy::ui::uiPanelContentAssignment(workspace, Id("panel:events"), fallback);
	iggy::ui::setUiPanelContentAssignment(workspace, { Id("panel:events"), iggy::ui::UiShellSlot::Right, false });
	iggy::ui::setUiPanelContentAssignment(workspace, { Id("panel:events"), iggy::ui::UiShellSlot::Left, true });
	const iggy::ui::UiPanelContentAssignment assigned = iggy::ui::uiPanelContentAssignment(workspace, Id("panel:events"), fallback);

	Expect(missing.groupId == Id("panel:events") && missing.slot == iggy::ui::UiShellSlot::Bottom, "ui panel assignment lookup should return fallback for missing group");
	Expect(workspace.panelContent.size() == 1, "ui panel assignment setter should update by group id");
	Expect(assigned.slot == iggy::ui::UiShellSlot::Left && assigned.hidden, "ui panel assignment lookup should return updated assignment");
}

} // namespace

int main()
{
	TestDefaultSettingsStateUsesToolInventoryAndPanelDefaults();
	TestDefaultSettingsPagesAreTableDriven();
	TestApplySettingsRebuildsWorkspaceBeltAndPanelContent();
	TestPanelContentAssignmentHelpersInsertUpdateAndFallback();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
