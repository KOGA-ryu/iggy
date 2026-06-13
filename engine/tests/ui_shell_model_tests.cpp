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
	TestDefaultPanelStateMatchesReferenceShape();
	TestPanelVisibilitySeparatesCollapsedAndAutoHidden();
	TestPanelSizeClampUsesSlotBands();
	TestMountDoesNotMutateLayoutOrRegistry();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
