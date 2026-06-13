#include <cstdlib>
#include <vector>

#include "scene/ui/UiShellModel.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;

iggy::ResourceId Id(const char *value)
{
	return iggy::ResourceId { value };
}

iggy::ui::UiFeatureRegistry Registry()
{
	return {
		{
			{ Id("ui:runtime"), "Runtime", { iggy::ui::UiShellSlot::Main, iggy::ui::UiShellSlot::Right } },
			{ Id("ui:events"), "Events", { iggy::ui::UiShellSlot::Bottom } },
			{ Id("ui:tools"), "Tools", { iggy::ui::UiShellSlot::Left } },
		}
	};
}

void TestFindFeatureById()
{
	const iggy::ui::UiFeatureRegistry registry = Registry();

	const iggy::ui::UiFeatureDescriptor *feature = iggy::ui::findUiFeature(registry, Id("ui:runtime"));
	const iggy::ui::UiFeatureDescriptor *missing = iggy::ui::findUiFeature(registry, Id("ui:missing"));

	Expect(feature != nullptr && feature->label == "Runtime", "ui feature registry should find descriptor by id");
	Expect(missing == nullptr, "ui feature registry should return null for missing id");
}

void TestMountWorkspaceSkipsMissingAndUnsupportedBindings()
{
	iggy::ui::UiWorkspaceLayout layout;
	layout.bindings = {
		{ iggy::ui::UiShellSlot::Main, Id("ui:runtime") },
		{ iggy::ui::UiShellSlot::Left, Id("ui:runtime") },
		{ iggy::ui::UiShellSlot::Bottom, Id("ui:events") },
		{ iggy::ui::UiShellSlot::Right, Id("ui:missing") },
	};

	const std::vector<iggy::ui::UiMountedSlot> mounted = iggy::ui::mountUiWorkspaceLayout(layout, Registry());

	Expect(mounted.size() == 2, "ui workspace mount should skip missing and unsupported bindings");
	if (mounted.size() == 2) {
		Expect(mounted[0].slot == iggy::ui::UiShellSlot::Main && mounted[0].featureId == Id("ui:runtime"), "ui workspace mount should preserve first valid binding");
		Expect(mounted[1].slot == iggy::ui::UiShellSlot::Bottom && mounted[1].featureId == Id("ui:events"), "ui workspace mount should preserve second valid binding");
	}
}

void TestMountedSlotLookup()
{
	const std::vector<iggy::ui::UiMountedSlot> mounted {
		{ iggy::ui::UiShellSlot::Main, Id("ui:runtime") },
		{ iggy::ui::UiShellSlot::Bottom, Id("ui:events") },
	};

	const iggy::ui::UiMountedSlot *bottom = iggy::ui::mountedUiSlot(mounted, iggy::ui::UiShellSlot::Bottom);
	const iggy::ui::UiMountedSlot *left = iggy::ui::mountedUiSlot(mounted, iggy::ui::UiShellSlot::Left);

	Expect(bottom != nullptr && bottom->featureId == Id("ui:events"), "mounted ui slot lookup should return mounted feature");
	Expect(left == nullptr, "mounted ui slot lookup should return null for empty slot");
}

void TestMountedFeaturePanelsUseFeatureDescriptorsAndPanelAssignments()
{
	iggy::ui::UiFeatureRegistry registry;
	registry.features = {
		{
			Id("feature:runtime"),
			"Runtime",
			{ iggy::ui::UiShellSlot::Main },
			{
				{ Id("panel:runtime"), "Runtime", Id("group:runtime"), iggy::ui::UiShellSlot::Right },
				{ Id("panel:events"), "Events", Id("group:events"), iggy::ui::UiShellSlot::Bottom },
			},
			{ { Id("palette:tools"), "Tools" } },
			{ { Id("chrome:settings"), "Settings" } },
		},
		{
			Id("feature:missing_slot"),
			"Missing Slot",
			{ iggy::ui::UiShellSlot::Left },
			{ { Id("panel:missing"), "Missing", Id("group:missing"), iggy::ui::UiShellSlot::Left } },
			{},
			{},
		},
	};
	iggy::ui::UiWorkspaceLayout layout;
	layout.bindings = {
		{ iggy::ui::UiShellSlot::Main, Id("feature:runtime") },
		{ iggy::ui::UiShellSlot::Right, Id("feature:missing_slot") },
	};
	layout.panelContent = {
		{ Id("group:runtime"), iggy::ui::UiShellSlot::Left, false },
		{ Id("group:events"), iggy::ui::UiShellSlot::Bottom, true },
	};
	layout.palettes = {
		{ Id("palette:tools"), 30, 40 },
	};

	const std::vector<iggy::ui::UiMountedPanel> panels = iggy::ui::mountUiWorkspacePanels(layout, registry);
	const std::vector<iggy::ui::UiMountedPalette> palettes = iggy::ui::mountUiWorkspacePalettes(layout, registry);
	const std::vector<iggy::ui::UiMountedChromePanel> chrome = iggy::ui::mountUiWorkspaceChromePanels(layout, registry);

	Expect(panels.size() == 2, "ui shell should mount panels supplied by valid mounted features only");
	if (panels.size() == 2) {
		Expect(panels[0].id == Id("panel:runtime") && panels[0].slot == iggy::ui::UiShellSlot::Left, "ui shell should apply panel assignment slot overrides");
		Expect(panels[1].id == Id("panel:events") && panels[1].hidden, "ui shell should preserve hidden panel assignments");
	}
	Expect(palettes.size() == 1 && palettes[0].placement.x == 30 && palettes[0].placement.y == 40, "ui shell should mount feature palettes with workspace placement");
	Expect(chrome.size() == 1 && chrome[0].id == Id("chrome:settings"), "ui shell should mount feature chrome panel descriptors");
}

void TestDefaultPanelStateMatchesReferenceShape()
{
	const iggy::ui::UiShellPanelsState state = iggy::ui::defaultUiShellPanelsState();

	Expect(state.left.size == 260 && !state.left.collapsed, "default ui panels should open left panel");
	Expect(state.right.size == 300 && state.right.collapsed, "default ui panels should collapse right panel");
	Expect(state.bottom.size == 132 && state.bottom.collapsed, "default ui panels should collapse bottom panel");
}

void TestPanelVisibilitySeparatesCollapsedAndAutoHidden()
{
	const iggy::ui::UiPanelState open { 260, false };
	const iggy::ui::UiPanelState closed { 260, true };

	Expect(iggy::ui::uiPanelVisibility(iggy::ui::UiShellSlot::Left, open, 1280, 720) == iggy::ui::UiPanelVisibility::Visible, "open left panel should be visible at wide width");
	Expect(iggy::ui::uiPanelVisibility(iggy::ui::UiShellSlot::Left, open, 639, 720) == iggy::ui::UiPanelVisibility::AutoHidden, "open left panel should auto-hide below width threshold");
	Expect(iggy::ui::uiPanelVisibility(iggy::ui::UiShellSlot::Left, closed, 639, 720) == iggy::ui::UiPanelVisibility::Collapsed, "manual collapse should outrank auto-hide");
	Expect(iggy::ui::uiPanelVisibility(iggy::ui::UiShellSlot::Bottom, open, 1280, 519) == iggy::ui::UiPanelVisibility::AutoHidden, "open bottom panel should auto-hide below height threshold");
	Expect(iggy::ui::uiPanelVisibility(iggy::ui::UiShellSlot::Main, closed, 1, 1) == iggy::ui::UiPanelVisibility::Visible, "main slot should always be visible");
}

void TestPanelSizeClampUsesSlotBands()
{
	Expect(iggy::ui::clampUiPanelSize(iggy::ui::UiShellSlot::Left, 10) == 180, "left panel clamp should enforce min");
	Expect(iggy::ui::clampUiPanelSize(iggy::ui::UiShellSlot::Left, 900) == 520, "left panel clamp should enforce max");
	Expect(iggy::ui::clampUiPanelSize(iggy::ui::UiShellSlot::Bottom, 900) == 900, "bottom panel clamp should allow unbounded max");
	Expect(iggy::ui::clampUiPanelSize(iggy::ui::UiShellSlot::Main, 42) == 42, "main slot clamp should leave size unchanged");
}

void TestPanelContentAssignmentIsKeyedByGroup()
{
	iggy::ui::UiWorkspaceLayout layout;
	iggy::ui::setUiPanelContentAssignment(layout, { Id("panel:runtime"), iggy::ui::UiShellSlot::Right, false });
	iggy::ui::setUiPanelContentAssignment(layout, { Id("panel:runtime"), iggy::ui::UiShellSlot::Bottom, true });

	const iggy::ui::UiPanelContentAssignment runtime = iggy::ui::uiPanelContentAssignment(layout, Id("panel:runtime"));
	const iggy::ui::UiPanelContentAssignment missing = iggy::ui::uiPanelContentAssignment(
		layout,
		Id("panel:missing"),
		{ {}, iggy::ui::UiShellSlot::Left, false });

	Expect(layout.panelContent.size() == 1, "ui panel content assignment should update by group id");
	Expect(runtime.slot == iggy::ui::UiShellSlot::Bottom && runtime.hidden, "ui panel content assignment should preserve replacement");
	Expect(missing.groupId == Id("panel:missing"), "ui panel content fallback should receive requested group id");
	Expect(missing.slot == iggy::ui::UiShellSlot::Left, "ui panel content fallback should preserve caller defaults");
}

void TestPalettePlacementIsWorkspaceDataKeyedByPalette()
{
	iggy::ui::UiWorkspaceLayout layout;
	iggy::ui::setUiPalettePlacement(layout, { Id("palette:tools"), 40, 50 });
	iggy::ui::setUiPalettePlacement(layout, { Id("palette:tools"), 70, 80 });

	const iggy::ui::UiPalettePlacement tools = iggy::ui::uiPalettePlacement(layout, Id("palette:tools"));
	const iggy::ui::UiPalettePlacement missing = iggy::ui::uiPalettePlacement(layout, Id("palette:missing"));

	Expect(layout.palettes.size() == 1, "ui palette placement should update by palette id");
	Expect(tools.x == 70 && tools.y == 80, "ui palette placement should preserve replacement coordinates");
	Expect(missing.paletteId == Id("palette:missing"), "ui palette placement fallback should receive requested palette id");
	Expect(missing.x == 12 && missing.y == 12, "ui palette placement fallback should use default placement");
}

void TestPalettePlacementClampKeepsStaleCoordinatesVisible()
{
	const iggy::ui::UiPalettePlacement placement = iggy::ui::clampUiPalettePlacement(
		{ Id("palette:tools"), 900, -5 },
		640,
		480,
		200,
		100);
	const iggy::ui::UiPalettePlacement tinyHost = iggy::ui::clampUiPalettePlacement(
		{ Id("palette:wide"), 10, 20 },
		40,
		30,
		200,
		100);

	Expect(placement.paletteId == Id("palette:tools"), "ui palette clamp should preserve palette id");
	Expect(placement.x == 440 && placement.y == 0, "ui palette clamp should fit placement within host area");
	Expect(tinyHost.x == 0 && tinyHost.y == 0, "ui palette clamp should degrade oversized palettes to origin");
}

void TestMountDoesNotMutateLayoutOrRegistry()
{
	iggy::ui::UiWorkspaceLayout layout;
	layout.bindings = {
		{ iggy::ui::UiShellSlot::Main, Id("ui:runtime") },
		{ iggy::ui::UiShellSlot::Right, Id("ui:missing") },
	};
	const iggy::ui::UiFeatureRegistry registry = Registry();

	(void)iggy::ui::mountUiWorkspaceLayout(layout, registry);

	Expect(layout.bindings.size() == 2, "ui workspace mount should not mutate layout");
	Expect(registry.features.size() == 3, "ui workspace mount should not mutate registry");
}

} // namespace

int main()
{
	TestFindFeatureById();
	TestMountWorkspaceSkipsMissingAndUnsupportedBindings();
	TestMountedSlotLookup();
	TestMountedFeaturePanelsUseFeatureDescriptorsAndPanelAssignments();
	TestDefaultPanelStateMatchesReferenceShape();
	TestPanelVisibilitySeparatesCollapsedAndAutoHidden();
	TestPanelSizeClampUsesSlotBands();
	TestPanelContentAssignmentIsKeyedByGroup();
	TestPalettePlacementIsWorkspaceDataKeyedByPalette();
	TestPalettePlacementClampKeepsStaleCoordinatesVisible();
	TestMountDoesNotMutateLayoutOrRegistry();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
