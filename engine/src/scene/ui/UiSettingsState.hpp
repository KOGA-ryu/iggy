#pragma once

#include <string>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/ui/UiShellModel.hpp"
#include "scene/ui/UiTheme.hpp"
#include "scene/ui/UiToolInventory.hpp"

namespace iggy::ui {

struct UiSettingsPageDescriptor {
	ResourceId id;
	std::string label;
};

struct UiSettingsState {
	ResourceId themeId;
	UiThemeInputs theme;
	ResourceId activePageId;
	std::vector<ResourceId> enabledToolIds;
	std::vector<UiPanelContentAssignment> panelContent;
	bool showRuntimeInspector = true;
	bool showInteractionEvents = true;
	bool showCollisionDebug = false;
};

[[nodiscard]] std::vector<UiSettingsPageDescriptor> defaultUiSettingsPages();
[[nodiscard]] UiSettingsState defaultUiSettingsState(const UiToolInventory &inventory = defaultUiToolInventory());
[[nodiscard]] UiWorkspaceLayout applyUiSettingsToWorkspace(
	UiWorkspaceLayout workspace,
	const UiSettingsState &settings,
	const UiToolInventory &inventory = defaultUiToolInventory());

} // namespace iggy::ui
