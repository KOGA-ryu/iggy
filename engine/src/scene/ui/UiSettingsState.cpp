#include "scene/ui/UiSettingsState.hpp"

namespace iggy::ui {

UiSettingsState defaultUiSettingsState(const UiToolInventory &inventory)
{
	UiSettingsState state;
	state.themeId = ResourceId { "theme:default" };
	state.enabledToolIds = defaultEnabledUiToolIds(inventory);
	state.panelContent = {
		{ ResourceId { "panel:runtime_frame" }, UiShellSlot::Right, false },
		{ ResourceId { "panel:interaction_events" }, UiShellSlot::Bottom, false },
		{ ResourceId { "panel:inventory" }, UiShellSlot::Left, false },
		{ ResourceId { "panel:collision" }, UiShellSlot::Right, true },
	};
	return state;
}

UiWorkspaceLayout applyUiSettingsToWorkspace(UiWorkspaceLayout workspace, const UiSettingsState &settings, const UiToolInventory &inventory)
{
	workspace.toolBelt = buildUiToolBeltLayout(inventory, settings.enabledToolIds);
	workspace.panelContent = settings.panelContent;
	return workspace;
}

} // namespace iggy::ui
