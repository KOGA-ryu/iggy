#include "scene/ui/UiShellModel.hpp"

#include <algorithm>

namespace iggy::ui {
namespace {

bool ContainsResourceId(const std::vector<ResourceId> &ids, const ResourceId &id)
{
	return std::find(ids.begin(), ids.end(), id) != ids.end();
}

std::vector<const UiFeatureDescriptor *> mountedUiFeatures(
	const UiWorkspaceLayout &layout,
	const UiFeatureRegistry &registry)
{
	std::vector<const UiFeatureDescriptor *> features;
	std::vector<ResourceId> mountedIds;
	for (const UiSlotBinding &binding : layout.bindings) {
		if (ContainsResourceId(mountedIds, binding.featureId))
			continue;
		const UiFeatureDescriptor *feature = findUiFeature(registry, binding.featureId);
		if (feature == nullptr || !uiFeatureSupportsSlot(*feature, binding.slot))
			continue;
		features.push_back(feature);
		mountedIds.push_back(binding.featureId);
	}
	return features;
}

} // namespace

const UiFeatureDescriptor *findUiFeature(const UiFeatureRegistry &registry, const ResourceId &featureId)
{
	const auto found = std::find_if(
		registry.features.begin(),
		registry.features.end(),
		[&featureId](const UiFeatureDescriptor &feature) {
			return feature.id == featureId;
		});
	return found == registry.features.end() ? nullptr : &*found;
}

bool uiFeatureSupportsSlot(const UiFeatureDescriptor &feature, UiShellSlot slot)
{
	return std::find(feature.supportedSlots.begin(), feature.supportedSlots.end(), slot) != feature.supportedSlots.end();
}

std::vector<UiMountedSlot> mountUiWorkspaceLayout(const UiWorkspaceLayout &layout, const UiFeatureRegistry &registry)
{
	std::vector<UiMountedSlot> mounted;
	for (const UiSlotBinding &binding : layout.bindings) {
		const UiFeatureDescriptor *feature = findUiFeature(registry, binding.featureId);
		if (feature == nullptr || !uiFeatureSupportsSlot(*feature, binding.slot))
			continue;
		mounted.push_back({ binding.slot, binding.featureId });
	}
	return mounted;
}

const UiMountedSlot *mountedUiSlot(const std::vector<UiMountedSlot> &mounted, UiShellSlot slot)
{
	const auto found = std::find_if(
		mounted.begin(),
		mounted.end(),
		[slot](const UiMountedSlot &entry) {
			return entry.slot == slot;
		});
	return found == mounted.end() ? nullptr : &*found;
}

std::vector<UiMountedPanel> mountUiWorkspacePanels(const UiWorkspaceLayout &layout, const UiFeatureRegistry &registry)
{
	std::vector<UiMountedPanel> mounted;
	for (const UiFeatureDescriptor *feature : mountedUiFeatures(layout, registry)) {
		for (const UiFeaturePanelDescriptor &panel : feature->panels) {
			const UiPanelContentAssignment assignment = uiPanelContentAssignment(
				layout,
				panel.groupId,
				{ panel.groupId, panel.defaultSlot, false });
			mounted.push_back({
				panel.id,
				panel.label,
				feature->id,
				panel.groupId,
				assignment.slot,
				assignment.hidden,
			});
		}
	}
	return mounted;
}

std::vector<UiMountedPalette> mountUiWorkspacePalettes(const UiWorkspaceLayout &layout, const UiFeatureRegistry &registry)
{
	std::vector<UiMountedPalette> mounted;
	for (const UiFeatureDescriptor *feature : mountedUiFeatures(layout, registry)) {
		for (const UiFeaturePaletteDescriptor &palette : feature->palettes) {
			mounted.push_back({
				palette.id,
				palette.label,
				feature->id,
				uiPalettePlacement(layout, palette.id),
			});
		}
	}
	return mounted;
}

std::vector<UiMountedChromePanel> mountUiWorkspaceChromePanels(
	const UiWorkspaceLayout &layout,
	const UiFeatureRegistry &registry)
{
	std::vector<UiMountedChromePanel> mounted;
	for (const UiFeatureDescriptor *feature : mountedUiFeatures(layout, registry)) {
		for (const UiFeatureChromePanelDescriptor &panel : feature->chromePanels) {
			mounted.push_back({
				panel.id,
				panel.label,
				feature->id,
			});
		}
	}
	return mounted;
}

UiPanelContentAssignment uiPanelContentAssignment(
	const UiWorkspaceLayout &layout,
	const ResourceId &groupId,
	UiPanelContentAssignment fallback)
{
	for (const UiPanelContentAssignment &assignment : layout.panelContent) {
		if (assignment.groupId == groupId)
			return assignment;
	}
	fallback.groupId = groupId;
	return fallback;
}

void setUiPanelContentAssignment(UiWorkspaceLayout &layout, UiPanelContentAssignment assignment)
{
	for (UiPanelContentAssignment &existing : layout.panelContent) {
		if (existing.groupId == assignment.groupId) {
			existing = assignment;
			return;
		}
	}
	layout.panelContent.push_back(assignment);
}

UiPalettePlacement uiPalettePlacement(
	const UiWorkspaceLayout &layout,
	const ResourceId &paletteId,
	UiPalettePlacement fallback)
{
	for (const UiPalettePlacement &placement : layout.palettes) {
		if (placement.paletteId == paletteId)
			return placement;
	}
	fallback.paletteId = paletteId;
	return fallback;
}

void setUiPalettePlacement(UiWorkspaceLayout &layout, UiPalettePlacement placement)
{
	for (UiPalettePlacement &existing : layout.palettes) {
		if (existing.paletteId == placement.paletteId) {
			existing = placement;
			return;
		}
	}
	layout.palettes.push_back(placement);
}

UiPalettePlacement clampUiPalettePlacement(
	UiPalettePlacement placement,
	int hostWidth,
	int hostHeight,
	int paletteWidth,
	int paletteHeight)
{
	const int maxX = std::max(0, hostWidth - std::max(0, paletteWidth));
	const int maxY = std::max(0, hostHeight - std::max(0, paletteHeight));
	placement.x = std::clamp(placement.x, 0, maxX);
	placement.y = std::clamp(placement.y, 0, maxY);
	return placement;
}

UiPanelSpec uiPanelSpec(UiShellSlot slot)
{
	switch (slot) {
	case UiShellSlot::Left:
		return { 260, 180, 520, 640, true };
	case UiShellSlot::Right:
		return { 300, 160, 0, 0, false };
	case UiShellSlot::Bottom:
		return { 132, 96, 0, 520, false };
	case UiShellSlot::Main:
		break;
	}
	return {};
}

UiShellPanelsState defaultUiShellPanelsState()
{
	UiShellPanelsState state;
	const UiPanelSpec left = uiPanelSpec(UiShellSlot::Left);
	const UiPanelSpec right = uiPanelSpec(UiShellSlot::Right);
	const UiPanelSpec bottom = uiPanelSpec(UiShellSlot::Bottom);
	state.left = { left.defaultSize, !left.openInitially };
	state.right = { right.defaultSize, !right.openInitially };
	state.bottom = { bottom.defaultSize, !bottom.openInitially };
	return state;
}

UiPanelVisibility uiPanelVisibility(UiShellSlot slot, const UiPanelState &state, int windowWidth, int windowHeight)
{
	if (slot == UiShellSlot::Main)
		return UiPanelVisibility::Visible;
	if (state.collapsed)
		return UiPanelVisibility::Collapsed;

	const UiPanelSpec spec = uiPanelSpec(slot);
	if (spec.autoHideBelow > 0) {
		const int dimension = slot == UiShellSlot::Bottom ? windowHeight : windowWidth;
		if (dimension < spec.autoHideBelow)
			return UiPanelVisibility::AutoHidden;
	}
	return UiPanelVisibility::Visible;
}

int clampUiPanelSize(UiShellSlot slot, int size)
{
	const UiPanelSpec spec = uiPanelSpec(slot);
	if (spec.minSize <= 0)
		return size;
	const int upper = spec.maxSize > 0 ? spec.maxSize : size;
	return std::clamp(size, spec.minSize, std::max(spec.minSize, upper));
}

} // namespace iggy::ui
