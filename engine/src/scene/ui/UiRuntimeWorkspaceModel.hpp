#pragma once

#include <string>
#include <vector>

#include "scene/ui/UiFeatureContext.hpp"
#include "scene/ui/UiInteractionEventPanelModel.hpp"
#include "scene/ui/UiRuntimeFrameInspectorModel.hpp"
#include "scene/ui/UiSettingsState.hpp"
#include "scene/ui/UiShellModel.hpp"
#include "scene/ui/UiToolBeltState.hpp"
#include "scene/ui/UiToolInventory.hpp"

namespace iggy::ui {

enum class UiRuntimeWorkspaceDiagnosticSeverity {
	Info,
	Warning,
};

struct UiRuntimeWorkspaceDiagnosticRow {
	UiRuntimeWorkspaceDiagnosticSeverity severity = UiRuntimeWorkspaceDiagnosticSeverity::Info;
	std::string key;
	std::string message;
};

struct UiRuntimeWorkspaceToolView {
	ResourceId toolId;
	std::string label;
	ResourceId groupId;
	int row = 0;
	int column = 0;
	bool active = false;
};

struct UiRuntimeWorkspacePanelProjection {
	ResourceId groupId;
	UiShellSlot slot = UiShellSlot::Right;
	bool hidden = false;
	UiPanelVisibility visibility = UiPanelVisibility::Visible;
};

struct UiRuntimeWorkspaceModelInput {
	UiSettingsState settings;
	UiFeatureContext context;
	UiToolInventory inventory;
	UiWorkspaceLayout workspace;
	UiFeatureRegistry features;
	UiShellPanelsState panels;
	int windowWidth = 1280;
	int windowHeight = 720;
};

struct UiRuntimeWorkspaceModel {
	UiWorkspaceLayout workspace;
	UiShellPanelsState panels;
	std::vector<UiMountedSlot> mountedSlots;
	ResourceId activeToolId;
	UiToolBeltLayout toolBelt;
	UiToolBeltState toolBeltState;
	UiToolBeltView toolBeltView;
	std::vector<UiRuntimeWorkspaceToolView> availableTools;
	std::vector<UiRuntimeWorkspacePanelProjection> panelProjections;
	std::vector<UiPalettePlacement> palettes;
	UiRuntimeFrameInspectorModel runtimeInspector;
	UiInteractionEventPanelModel interactionEvents;
	bool hasRuntimeContext = false;
	bool hasInteractionEventContext = false;
	std::vector<UiRuntimeWorkspaceDiagnosticRow> diagnostics;

	[[nodiscard]] bool hasDiagnostics() const;
};

[[nodiscard]] UiFeatureRegistry defaultUiRuntimeWorkspaceFeatureRegistry();
[[nodiscard]] UiWorkspaceLayout defaultUiRuntimeWorkspaceLayout(const UiToolInventory &inventory = defaultUiToolInventory());
[[nodiscard]] UiRuntimeWorkspaceModelInput defaultUiRuntimeWorkspaceModelInput(
	const UiToolInventory &inventory = defaultUiToolInventory());
[[nodiscard]] UiRuntimeWorkspaceModel buildUiRuntimeWorkspaceModel(const UiRuntimeWorkspaceModelInput &input);

} // namespace iggy::ui
