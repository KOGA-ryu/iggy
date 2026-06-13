#pragma once

#include <string>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/ui/UiShellModel.hpp"
#include "scene/ui/UiToolInventory.hpp"

namespace iggy::ui {

struct UiSettingsState {
	ResourceId themeId;
	std::vector<ResourceId> enabledToolIds;
	std::vector<UiPanelContentAssignment> panelContent;
	bool showRuntimeInspector = true;
	bool showInteractionEvents = true;
	bool showCollisionDebug = false;
};

[[nodiscard]] UiSettingsState defaultUiSettingsState(const UiToolInventory &inventory = defaultUiToolInventory());
[[nodiscard]] UiWorkspaceLayout applyUiSettingsToWorkspace(
	UiWorkspaceLayout workspace,
	const UiSettingsState &settings,
	const UiToolInventory &inventory = defaultUiToolInventory());

} // namespace iggy::ui
