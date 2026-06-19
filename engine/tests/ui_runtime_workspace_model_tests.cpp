#include <cstdlib>

#include "runtime/RuntimeGameplayAuthoringPreviewModel.hpp"
#include "runtime/RuntimeGameplayProductPlayMode.hpp"
#include "runtime/RuntimePlayerInputInteractionEffectFrameReport.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/interaction/InteractionEvent2D.hpp"
#include "scene/ui/UiRuntimeWorkspaceModel.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

const iggy::ui::UiRuntimeWorkspaceDiagnosticRow *FindDiagnostic(
	const iggy::ui::UiRuntimeWorkspaceModel &model,
	const std::string &key)
{
	for (const iggy::ui::UiRuntimeWorkspaceDiagnosticRow &row : model.diagnostics) {
		if (row.key == key)
			return &row;
	}
	return nullptr;
}

const iggy::ui::UiRuntimeWorkspaceToolView *FindToolView(
	const iggy::ui::UiRuntimeWorkspaceModel &model,
	const iggy::ResourceId &toolId)
{
	for (const iggy::ui::UiRuntimeWorkspaceToolView &tool : model.availableTools) {
		if (tool.toolId == toolId)
			return &tool;
	}
	return nullptr;
}

const iggy::ui::UiRuntimeWorkspacePanelProjection *FindPanelProjection(
	const iggy::ui::UiRuntimeWorkspaceModel &model,
	const iggy::ResourceId &groupId)
{
	for (const iggy::ui::UiRuntimeWorkspacePanelProjection &panel : model.panelProjections) {
		if (panel.groupId == groupId)
			return &panel;
	}
	return nullptr;
}

const iggy::ui::UiProductPlayModePanelRow *FindProductRow(
	const std::vector<iggy::ui::UiProductPlayModePanelRow> &rows,
	const std::string &key)
{
	for (const iggy::ui::UiProductPlayModePanelRow &row : rows) {
		if (row.key == key)
			return &row;
	}
	return nullptr;
}

void TestDefaultEmptyContextBuildsValidWorkspaceWithDiagnostics()
{
	const iggy::ui::UiRuntimeWorkspaceModelInput input = iggy::ui::defaultUiRuntimeWorkspaceModelInput();
	const iggy::ui::UiRuntimeWorkspaceModel model = iggy::ui::buildUiRuntimeWorkspaceModel(input);

	Expect(model.workspace.id == Id("workspace:runtime"), "workspace model should preserve default workspace id");
	Expect(!model.mountedSlots.empty(), "workspace model should mount default shell features");
	Expect(!model.activeToolId.empty(), "workspace model should select a usable active tool");
	Expect(!model.availableTools.empty(), "workspace model should expose available tools");
	Expect(model.runtimeInspector.rows.size() == 11, "workspace model should project runtime inspector rows even with missing context");
	Expect(model.hasDiagnostics(), "workspace model should surface missing-context diagnostics");
	Expect(FindDiagnostic(model, "runtime.session") != nullptr, "workspace model should diagnose missing session");
	Expect(FindDiagnostic(model, "runtime.frame_report") != nullptr, "workspace model should diagnose missing frame report");
	Expect(FindDiagnostic(model, "interaction.events") != nullptr, "workspace model should diagnose missing interaction event recorder");
	Expect(FindDiagnostic(model, "authoring.preview") == nullptr, "workspace model should treat missing authoring preview as optional");
	Expect(FindDiagnostic(model, "product.play") == nullptr, "workspace model should treat missing product play as optional");
}

void TestRuntimeContextProducesRuntimeInspectorPanelData()
{
	iggy::ui::UiRuntimeWorkspaceModelInput input = iggy::ui::defaultUiRuntimeWorkspaceModelInput();
	iggy::runtime::RuntimeSessionState session;
	session.tickIndex = 17;
	session.hasPlayer = true;
	iggy::runtime::RuntimePlayerInputInteractionEffectFrameReport report;
	report.acceptedCommandCount = 4;
	report.requestedEffectCount = 2;
	input.context.session = &session;
	input.context.latestFrameReport = &report;

	const iggy::ui::UiRuntimeWorkspaceModel model = iggy::ui::buildUiRuntimeWorkspaceModel(input);

	Expect(model.hasRuntimeContext, "workspace model should report runtime context when session/report exist");
	Expect(model.runtimeInspector.hasSession, "workspace model runtime inspector should see session");
	Expect(model.runtimeInspector.tickIndex == 17, "workspace model runtime inspector should preserve tick index");
	Expect(model.runtimeInspector.acceptedCommandCount == 4, "workspace model runtime inspector should preserve accepted count");
	Expect(model.runtimeInspector.requestedEffectCount == 2, "workspace model runtime inspector should preserve requested effects count");
}

void TestInteractionRecorderProducesInteractionPanelData()
{
	iggy::ui::UiRuntimeWorkspaceModelInput input = iggy::ui::defaultUiRuntimeWorkspaceModelInput();
	iggy::InteractionEventRecorder2D recorder;
	iggy::recordInteractionEvent(recorder, iggy::targetToggledInteractionEvent(Id("target:door"), false));
	iggy::recordInteractionEvent(recorder, iggy::inspectTextRequestedInteractionEvent(Id("target:sign"), "Look"));
	input.context.interactionEvents = &recorder;

	const iggy::ui::UiRuntimeWorkspaceModel model = iggy::ui::buildUiRuntimeWorkspaceModel(input);

	Expect(model.hasInteractionEventContext, "workspace model should report interaction event context");
	Expect(model.interactionEvents.rows.size() == 2, "workspace model should project interaction event rows");
	Expect(model.interactionEvents.targetToggleCount == 1, "workspace model should preserve toggle event count");
	Expect(model.interactionEvents.inspectTextRequestCount == 1, "workspace model should preserve inspect event count");
	Expect(FindDiagnostic(model, "interaction.events") == nullptr, "workspace model should not diagnose present interaction recorder as missing");
}

void TestAuthoringPreviewProducesPreviewPanelData()
{
	iggy::ui::UiRuntimeWorkspaceModelInput input = iggy::ui::defaultUiRuntimeWorkspaceModelInput();
	iggy::runtime::RuntimeGameplayAuthoringPreviewModel preview;
	preview.inputPath = "fixtures/mixed_mini_scenario.toml";
	preview.sourcePath = "fixtures/mixed_mini_scenario.toml";
	preview.status = iggy::runtime::RuntimeGameplayAuthoringPreviewStatus::Ran;
	preview.scenarioStatus = iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::Ran;
	preview.summary.frameCount = 1;
	preview.finalRows = { "###", "#@#", "###" };
	input.context.authoringPreview = &preview;

	const iggy::ui::UiRuntimeWorkspaceModel model = iggy::ui::buildUiRuntimeWorkspaceModel(input);

	Expect(model.hasAuthoringPreviewContext, "workspace model should report authoring preview context");
	Expect(model.authoringPreview.present, "workspace model should project authoring preview panel data");
	Expect(model.authoringPreview.finalRows == std::vector<std::string>({ "###", "#@#", "###" }), "workspace model should preserve authoring final rows");
	Expect(FindDiagnostic(model, "authoring.preview") == nullptr, "workspace model should not diagnose present authoring preview as missing");
}

void TestProductPlayContextProducesHiddenPanelData()
{
	iggy::ui::UiRuntimeWorkspaceModelInput input = iggy::ui::defaultUiRuntimeWorkspaceModelInput();
	iggy::runtime::RuntimeGameplayProductPlayModeBuildResult build;
	build.status = iggy::runtime::RuntimeGameplayProductPlayModeBuildStatus::Ready;
	build.loop.status = iggy::runtime::RuntimeGameplayProductLoopStatus::Ready;
	iggy::runtime::RuntimeGameplayProductPlayModeState state;
	state.loop.loaded = true;
	state.loop.scenario.frames.resize(2);
	state.loop.nextFrameIndex = 1;
	state.hasInputFocus = false;
	iggy::runtime::RuntimeGameplayProductPlayModeFrameResult frame;
	frame.status = iggy::runtime::RuntimeGameplayProductPlayModeFrameStatus::Stepped;
	frame.surface.status = iggy::runtime::RuntimeGameplayProductPlaySurfaceFrameStatus::Stepped;
	frame.surface.ignoredInputEventCount = 3;
	input.context.productPlayModeBuild = &build;
	input.context.productPlayModeState = &state;
	input.context.latestProductPlayModeFrame = &frame;

	const iggy::ui::UiRuntimeWorkspaceModel model = iggy::ui::buildUiRuntimeWorkspaceModel(input);

	Expect(model.hasProductPlayModeContext, "workspace model should report product play mode context");
	Expect(model.productPlayMode.present, "workspace model should project product play mode panel data");
	Expect(model.productPlayMode.state.size() == 4, "workspace model should preserve product play state rows");
	const iggy::ui::UiRuntimeWorkspacePanelProjection *panel =
		FindPanelProjection(model, Id("panel:product_play"));
	Expect(panel != nullptr && panel->hidden &&
			panel->visibility == iggy::ui::UiPanelVisibility::Collapsed,
		"workspace model should register product play panel hidden by default");
	Expect(FindDiagnostic(model, "product.play") == nullptr,
		"workspace model should not diagnose present product play context");
}

void TestProductPlayContextProjectsTargetContextRows()
{
	iggy::ui::UiRuntimeWorkspaceModelInput input = iggy::ui::defaultUiRuntimeWorkspaceModelInput();
	iggy::runtime::RuntimeGameplayProductPlayModeBuildResult build;
	build.status = iggy::runtime::RuntimeGameplayProductPlayModeBuildStatus::Ready;
	iggy::runtime::RuntimeGameplayProductInputFrameTargetContextResult targetContext;
	targetContext.status =
		iggy::runtime::RuntimeGameplayProductInputFrameTargetContextStatus::TargetProjected;
	targetContext.hasPrimaryTileEvent = true;
	targetContext.primaryTileEventIndex = 2;
	targetContext.primaryTile = { 7, 8 };
	targetContext.target.status =
		iggy::runtime::RuntimeGameplayProductInteractionTargetQueryStatus::TargetFound;
	targetContext.target.hasTarget = true;
	targetContext.target.targetId = Id("target:door");
	targetContext.targetContext.status =
		iggy::runtime::RuntimeGameplayProductInputTargetContextStatus::TargetProjected;
	input.context.productPlayModeBuild = &build;
	input.context.latestProductInputFrameTargetContext = &targetContext;

	const iggy::ui::UiRuntimeWorkspaceModel model =
		iggy::ui::buildUiRuntimeWorkspaceModel(input);

	Expect(model.hasProductPlayModeContext,
		"workspace model should preserve product play presence with target context");
	Expect(model.productPlayMode.present,
		"workspace model should build product play panel with target context");
	const iggy::ui::UiProductPlayModePanelRow *status =
		FindProductRow(model.productPlayMode.targetContext,
			"targetContext.status");
	const iggy::ui::UiProductPlayModePanelRow *targetId =
		FindProductRow(model.productPlayMode.targetContext,
			"targetContext.target.targetId");
	Expect(status != nullptr && status->value == "TargetProjected",
		"workspace model should project target-context status row");
	Expect(targetId != nullptr && targetId->value == "target:door",
		"workspace model should project target id row");
}

void TestTargetContextAloneDoesNotCreateProductPlayContext()
{
	iggy::ui::UiRuntimeWorkspaceModelInput input = iggy::ui::defaultUiRuntimeWorkspaceModelInput();
	iggy::runtime::RuntimeGameplayProductInputFrameTargetContextResult targetContext;
	targetContext.status =
		iggy::runtime::RuntimeGameplayProductInputFrameTargetContextStatus::NoEligiblePrimaryTile;
	input.context.latestProductInputFrameTargetContext = &targetContext;

	const iggy::ui::UiRuntimeWorkspaceModel model =
		iggy::ui::buildUiRuntimeWorkspaceModel(input);

	Expect(!model.hasProductPlayModeContext,
		"workspace model should not treat target diagnostics alone as product play context");
	Expect(!model.productPlayMode.present,
		"workspace model should not build product panel from target diagnostics alone");
	Expect(model.productPlayMode.targetContext.empty(),
		"workspace model should not project target rows without product play context");
	Expect(FindDiagnostic(model, "product.play") == nullptr,
		"workspace model should not diagnose target-only product context as missing");
}

void TestProductPlayFeatureAndPanelAreRegistered()
{
	const iggy::ui::UiFeatureRegistry registry =
		iggy::ui::defaultUiRuntimeWorkspaceFeatureRegistry();
	const iggy::ui::UiFeatureDescriptor *feature =
		iggy::ui::findUiFeature(registry, Id("feature:product_play"));

	Expect(feature != nullptr, "workspace registry should include product play feature");
	Expect(feature != nullptr && feature->panels.size() == 1,
		"product play feature should register one panel");
	if (feature != nullptr && !feature->panels.empty()) {
		Expect(feature->panels[0].id == Id("panel:product_play"),
			"product play feature should register product play panel id");
		Expect(feature->panels[0].groupId == Id("panel:product_play"),
			"product play feature should register product play panel group");
	}
}

void TestDisabledToolsAreFilteredFromBeltAndAvailableToolViews()
{
	iggy::ui::UiToolInventory inventory;
	inventory.rows = 1;
	inventory.columns = 3;
	inventory.tools = {
		{ Id("tool:a"), "A", Id("group:test"), 0, 0, true },
		{ Id("tool:b"), "B", Id("group:test"), 0, 1, true },
		{ Id("tool:c"), "C", Id("group:test"), 0, 2, true },
	};
	iggy::ui::UiRuntimeWorkspaceModelInput input = iggy::ui::defaultUiRuntimeWorkspaceModelInput(inventory);
	input.settings.enabledToolIds = { Id("tool:b") };
	input.context.activeToolId = Id("tool:a");

	const iggy::ui::UiRuntimeWorkspaceModel model = iggy::ui::buildUiRuntimeWorkspaceModel(input);

	Expect(model.toolBelt.itemIds.size() == 3, "workspace model should preserve tool belt slot count");
	if (model.toolBelt.itemIds.size() == 3) {
		Expect(model.toolBelt.itemIds[0].empty(), "workspace model should filter disabled first tool from belt");
		Expect(model.toolBelt.itemIds[1] == Id("tool:b"), "workspace model should keep enabled tool in belt");
		Expect(model.toolBelt.itemIds[2].empty(), "workspace model should filter disabled last tool from belt");
	}
	Expect(model.availableTools.size() == 1, "workspace model should expose only enabled tools");
	Expect(FindToolView(model, Id("tool:b")) != nullptr, "workspace model should include enabled tool view");
	Expect(FindToolView(model, Id("tool:a")) == nullptr, "workspace model should exclude disabled active tool from available views");
	Expect(model.activeToolId == Id("tool:b"), "workspace model should fall back to first available active tool");
	Expect(FindDiagnostic(model, "ui.active_tool") != nullptr, "workspace model should diagnose unavailable active tool");
}

void TestPanelAssignmentsAndPalettesComeFromSettingsAndWorkspace()
{
	iggy::ui::UiRuntimeWorkspaceModelInput input = iggy::ui::defaultUiRuntimeWorkspaceModelInput();
	input.settings.panelContent = {
		{ Id("panel:custom"), iggy::ui::UiShellSlot::Left, false },
		{ Id("panel:hidden"), iggy::ui::UiShellSlot::Right, true },
	};
	input.workspace.palettes = {
		{ Id("palette:tools"), 33, 44 },
	};
	input.panels.left.collapsed = false;
	input.panels.right.collapsed = false;

	const iggy::ui::UiRuntimeWorkspaceModel model = iggy::ui::buildUiRuntimeWorkspaceModel(input);

	const iggy::ui::UiRuntimeWorkspacePanelProjection *custom = FindPanelProjection(model, Id("panel:custom"));
	const iggy::ui::UiRuntimeWorkspacePanelProjection *hidden = FindPanelProjection(model, Id("panel:hidden"));

	Expect(model.panelProjections.size() == 2, "workspace model should use settings panel assignments");
	Expect(custom != nullptr && custom->slot == iggy::ui::UiShellSlot::Left, "workspace model should preserve settings panel slot");
	Expect(custom != nullptr && custom->visibility == iggy::ui::UiPanelVisibility::Visible, "workspace model should compute visible panel state");
	Expect(hidden != nullptr && hidden->hidden && hidden->visibility == iggy::ui::UiPanelVisibility::Collapsed, "workspace model should collapse hidden assignments");
	Expect(model.palettes.size() == 1 && model.palettes[0].paletteId == Id("palette:tools") && model.palettes[0].x == 33 && model.palettes[0].y == 44, "workspace model should preserve workspace palette placements");
}

void TestProjectionDoesNotMutateInputs()
{
	iggy::ui::UiToolInventory inventory;
	inventory.rows = 1;
	inventory.columns = 2;
	inventory.tools = {
		{ Id("tool:a"), "A", Id("group:test"), 0, 0, true },
		{ Id("tool:b"), "B", Id("group:test"), 0, 1, true },
	};
	iggy::ui::UiRuntimeWorkspaceModelInput input = iggy::ui::defaultUiRuntimeWorkspaceModelInput(inventory);
	input.settings.enabledToolIds = { Id("tool:a") };
	input.context.activeToolId = Id("tool:a");
	input.panels.left.collapsed = false;
	iggy::runtime::RuntimeSessionState session;
	session.tickIndex = 9;
	input.context.session = &session;

	const iggy::ui::UiRuntimeWorkspaceModelInput before = input;

	(void)iggy::ui::buildUiRuntimeWorkspaceModel(input);

	Expect(input.settings.enabledToolIds.size() == 1 && input.settings.enabledToolIds[0] == Id("tool:a"), "workspace model should not mutate settings");
	Expect(input.inventory.tools.size() == 2 && input.inventory.tools[0].id == Id("tool:a"), "workspace model should not mutate inventory");
	Expect(input.workspace.id == before.workspace.id && input.workspace.panelContent.size() == before.workspace.panelContent.size(), "workspace model should not mutate workspace");
	Expect(input.panels.left.collapsed == before.panels.left.collapsed, "workspace model should not mutate panel state");
	Expect(session.tickIndex == 9, "workspace model should not mutate session");
}

} // namespace

int main()
{
	TestDefaultEmptyContextBuildsValidWorkspaceWithDiagnostics();
	TestRuntimeContextProducesRuntimeInspectorPanelData();
	TestInteractionRecorderProducesInteractionPanelData();
	TestAuthoringPreviewProducesPreviewPanelData();
	TestProductPlayContextProducesHiddenPanelData();
	TestProductPlayContextProjectsTargetContextRows();
	TestTargetContextAloneDoesNotCreateProductPlayContext();
	TestProductPlayFeatureAndPanelAreRegistered();
	TestDisabledToolsAreFilteredFromBeltAndAvailableToolViews();
	TestPanelAssignmentsAndPalettesComeFromSettingsAndWorkspace();
	TestProjectionDoesNotMutateInputs();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
