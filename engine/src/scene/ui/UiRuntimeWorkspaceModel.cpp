#include "scene/ui/UiRuntimeWorkspaceModel.hpp"

#include <cstddef>
#include <string>

namespace iggy::ui {
namespace {

ResourceId Id(const char *value)
{
	return ResourceId { value };
}

void AddDiagnostic(
	UiRuntimeWorkspaceModel &model,
	UiRuntimeWorkspaceDiagnosticSeverity severity,
	std::string key,
	std::string message)
{
	model.diagnostics.push_back({ severity, std::move(key), std::move(message) });
}

const UiPanelState &PanelStateForSlot(const UiShellPanelsState &panels, UiShellSlot slot)
{
	switch (slot) {
	case UiShellSlot::Left:
		return panels.left;
	case UiShellSlot::Right:
		return panels.right;
	case UiShellSlot::Bottom:
		return panels.bottom;
	case UiShellSlot::Main:
		break;
	}
	return panels.right;
}

ResourceId FirstEnabledToolId(const UiToolBeltLayout &layout)
{
	for (const ResourceId &toolId : layout.itemIds) {
		if (!toolId.empty())
			return toolId;
	}
	return {};
}

bool ToolBeltContains(const UiToolBeltLayout &layout, const ResourceId &toolId)
{
	if (toolId.empty())
		return false;
	for (const ResourceId &itemId : layout.itemIds) {
		if (itemId == toolId)
			return true;
	}
	return false;
}

UiToolBeltState ToolBeltStateForActiveTool(const UiToolBeltLayout &layout, const ResourceId &activeToolId)
{
	UiToolBeltState state;
	state.rows = layout.rows;
	state.columns = layout.columns;
	state.activeRow = 0;
	state.activeColumn = 0;
	if (state.rows <= 0 || state.columns <= 0 || activeToolId.empty())
		return state;

	for (std::size_t index = 0; index < layout.itemIds.size(); ++index) {
		if (layout.itemIds[index] != activeToolId)
			continue;
		state.activeRow = static_cast<int>(index / static_cast<std::size_t>(state.columns));
		state.activeColumn = static_cast<int>(index % static_cast<std::size_t>(state.columns));
		return state;
	}
	return normalizeUiToolBeltToOccupied(state, layout.itemIds);
}

ResourceId ResolveActiveToolId(
	const UiFeatureContext &context,
	const UiToolBeltLayout &layout,
	UiRuntimeWorkspaceModel &model)
{
	if (!context.activeToolId.empty() && ToolBeltContains(layout, context.activeToolId))
		return context.activeToolId;
	if (!context.activeToolId.empty()) {
		AddDiagnostic(
			model,
			UiRuntimeWorkspaceDiagnosticSeverity::Warning,
			"ui.active_tool",
			"Active tool is not available in the current tool belt");
	}
	return FirstEnabledToolId(layout);
}

UiPanelVisibility ProjectionVisibility(
	const UiShellPanelsState &panels,
	const UiPanelContentAssignment &assignment,
	int windowWidth,
	int windowHeight)
{
	if (assignment.hidden)
		return UiPanelVisibility::Collapsed;
	return uiPanelVisibility(assignment.slot, PanelStateForSlot(panels, assignment.slot), windowWidth, windowHeight);
}

} // namespace

bool UiRuntimeWorkspaceModel::hasDiagnostics() const
{
	return !diagnostics.empty();
}

UiFeatureRegistry defaultUiRuntimeWorkspaceFeatureRegistry()
{
	UiFeatureRegistry registry;
	registry.features = {
		{
			Id("feature:runtime_frame_inspector"),
			"Runtime Frame Inspector",
			{ UiShellSlot::Main, UiShellSlot::Right },
			{ { Id("panel:runtime_frame"), "Runtime Frame", Id("panel:runtime_frame"), UiShellSlot::Right } },
			{ { Id("palette:tool_belt"), "Tool Belt" } },
			{ { Id("chrome:runtime"), "Runtime" } },
		},
		{
			Id("feature:interaction_events"),
			"Interaction Events",
			{ UiShellSlot::Bottom, UiShellSlot::Right },
			{ { Id("panel:interaction_events"), "Interaction Events", Id("panel:interaction_events"), UiShellSlot::Bottom } },
			{},
			{},
		},
		{
			Id("feature:inventory"),
			"Inventory",
			{ UiShellSlot::Left, UiShellSlot::Right },
			{ { Id("panel:inventory"), "Inventory", Id("panel:inventory"), UiShellSlot::Left } },
			{},
			{},
		},
		{
			Id("feature:collision_debug"),
			"Collision Debug",
			{ UiShellSlot::Right, UiShellSlot::Bottom },
			{ { Id("panel:collision"), "Collision", Id("panel:collision"), UiShellSlot::Right } },
			{},
			{},
		},
	};
	return registry;
}

UiWorkspaceLayout defaultUiRuntimeWorkspaceLayout(const UiToolInventory &inventory)
{
	UiWorkspaceLayout workspace;
	workspace.id = Id("workspace:runtime");
	workspace.label = "Runtime Workspace";
	workspace.bindings = {
		{ UiShellSlot::Main, Id("feature:runtime_frame_inspector") },
		{ UiShellSlot::Bottom, Id("feature:interaction_events") },
		{ UiShellSlot::Left, Id("feature:inventory") },
	};
	workspace.toolBelt = buildUiToolBeltLayout(inventory, defaultEnabledUiToolIds(inventory));
	workspace.palettes = {
		{ Id("palette:tool_belt"), 12, 12 },
	};
	workspace.panelContent = {
		{ Id("panel:runtime_frame"), UiShellSlot::Right, false },
		{ Id("panel:interaction_events"), UiShellSlot::Bottom, false },
		{ Id("panel:inventory"), UiShellSlot::Left, false },
	};
	return workspace;
}

UiRuntimeWorkspaceModelInput defaultUiRuntimeWorkspaceModelInput(const UiToolInventory &inventory)
{
	UiRuntimeWorkspaceModelInput input;
	input.inventory = inventory;
	input.settings = defaultUiSettingsState(inventory);
	input.workspace = defaultUiRuntimeWorkspaceLayout(inventory);
	input.features = defaultUiRuntimeWorkspaceFeatureRegistry();
	input.panels = defaultUiShellPanelsState();
	return input;
}

UiRuntimeWorkspaceModel buildUiRuntimeWorkspaceModel(const UiRuntimeWorkspaceModelInput &input)
{
	UiRuntimeWorkspaceModel model;
	model.workspace = applyUiSettingsToWorkspace(input.workspace, input.settings, input.inventory);
	model.panels = input.panels;
	model.mountedSlots = mountUiWorkspaceLayout(model.workspace, input.features);
	model.mountedPanels = mountUiWorkspacePanels(model.workspace, input.features);
	model.mountedPalettes = mountUiWorkspacePalettes(model.workspace, input.features);
	model.mountedChromePanels = mountUiWorkspaceChromePanels(model.workspace, input.features);
	model.toolBelt = model.workspace.toolBelt;
	model.activeToolId = ResolveActiveToolId(input.context, model.toolBelt, model);
	model.toolBeltState = ToolBeltStateForActiveTool(model.toolBelt, model.activeToolId);
	model.toolBeltView = uiToolBeltView(model.toolBeltState, model.toolBelt.itemIds);
	model.palettes = model.workspace.palettes;

	for (const UiToolDescriptor &tool : input.inventory.tools) {
		if (!uiToolIdEnabled(input.settings.enabledToolIds, tool.id))
			continue;
		model.availableTools.push_back({
			tool.id,
			tool.label,
			tool.groupId,
			tool.beltRow,
			tool.beltColumn,
			tool.id == model.activeToolId,
		});
	}

	for (const UiPanelContentAssignment &assignment : model.workspace.panelContent) {
		model.panelProjections.push_back({
			assignment.groupId,
			assignment.slot,
			assignment.hidden,
			ProjectionVisibility(model.panels, assignment, input.windowWidth, input.windowHeight),
		});
	}

	if (input.settings.showRuntimeInspector) {
		model.runtimeInspector = buildUiRuntimeFrameInspectorModel(input.context.session, input.context.latestFrameReport);
		model.hasRuntimeContext = uiFeatureContextHasSession(input.context) || uiFeatureContextHasFrameReport(input.context);
		if (!uiFeatureContextHasSession(input.context)) {
			AddDiagnostic(
				model,
				UiRuntimeWorkspaceDiagnosticSeverity::Warning,
				"runtime.session",
				"Runtime session context is missing");
		}
		if (!uiFeatureContextHasFrameReport(input.context)) {
			AddDiagnostic(
				model,
				UiRuntimeWorkspaceDiagnosticSeverity::Info,
				"runtime.frame_report",
				"Runtime frame report context is missing");
		}
	}

	if (input.settings.showInteractionEvents) {
		model.hasInteractionEventContext = uiFeatureContextHasInteractionEvents(input.context);
		if (input.context.interactionEvents != nullptr) {
			model.interactionEvents = buildUiInteractionEventPanelModel(input.context.interactionEvents->events);
		} else {
			AddDiagnostic(
				model,
				UiRuntimeWorkspaceDiagnosticSeverity::Warning,
				"interaction.events",
				"Interaction event recorder context is missing");
		}
	}

	return model;
}

} // namespace iggy::ui
