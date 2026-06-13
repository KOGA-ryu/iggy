#pragma once

#include <string>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/ui/UiToolBeltState.hpp"

namespace iggy::ui {

enum class UiShellSlot {
	Main,
	Left,
	Right,
	Bottom,
};

struct UiFeatureDescriptor {
	ResourceId id;
	std::string label;
	std::vector<UiShellSlot> supportedSlots;
};

struct UiFeatureRegistry {
	std::vector<UiFeatureDescriptor> features;
};

struct UiSlotBinding {
	UiShellSlot slot = UiShellSlot::Main;
	ResourceId featureId;
};

struct UiPanelContentAssignment {
	ResourceId groupId;
	UiShellSlot slot = UiShellSlot::Right;
	bool hidden = false;
};

struct UiWorkspaceLayout {
	ResourceId id;
	std::string label;
	std::vector<UiSlotBinding> bindings;
	UiToolBeltLayout toolBelt;
	std::vector<UiPanelContentAssignment> panelContent;
};

struct UiMountedSlot {
	UiShellSlot slot = UiShellSlot::Main;
	ResourceId featureId;
};

struct UiPanelSpec {
	int defaultSize = 0;
	int minSize = 0;
	int maxSize = 0;
	int autoHideBelow = 0;
	bool openInitially = false;
};

enum class UiPanelVisibility {
	Visible,
	Collapsed,
	AutoHidden,
};

struct UiPanelState {
	int size = 0;
	bool collapsed = false;
};

struct UiShellPanelsState {
	UiPanelState left;
	UiPanelState right;
	UiPanelState bottom;
};

[[nodiscard]] const UiFeatureDescriptor *findUiFeature(
	const UiFeatureRegistry &registry,
	const ResourceId &featureId);
[[nodiscard]] bool uiFeatureSupportsSlot(const UiFeatureDescriptor &feature, UiShellSlot slot);
[[nodiscard]] std::vector<UiMountedSlot> mountUiWorkspaceLayout(
	const UiWorkspaceLayout &layout,
	const UiFeatureRegistry &registry);
[[nodiscard]] const UiMountedSlot *mountedUiSlot(
	const std::vector<UiMountedSlot> &mounted,
	UiShellSlot slot);

[[nodiscard]] UiPanelSpec uiPanelSpec(UiShellSlot slot);
[[nodiscard]] UiShellPanelsState defaultUiShellPanelsState();
[[nodiscard]] UiPanelVisibility uiPanelVisibility(
	UiShellSlot slot,
	const UiPanelState &state,
	int windowWidth,
	int windowHeight);
[[nodiscard]] int clampUiPanelSize(UiShellSlot slot, int size);

} // namespace iggy::ui
