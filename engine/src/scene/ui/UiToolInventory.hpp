#pragma once

#include <string>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/ui/UiToolBeltState.hpp"

namespace iggy::ui {

struct UiToolDescriptor {
	ResourceId id;
	std::string label;
	ResourceId groupId;
	int beltRow = 0;
	int beltColumn = 0;
	bool enabledByDefault = true;
};

struct UiToolInventory {
	int rows = 1;
	int columns = 1;
	std::vector<UiToolDescriptor> tools;
};

[[nodiscard]] UiToolInventory defaultUiToolInventory();
[[nodiscard]] const UiToolDescriptor *findUiTool(
	const UiToolInventory &inventory,
	const ResourceId &toolId);
[[nodiscard]] std::vector<ResourceId> defaultEnabledUiToolIds(const UiToolInventory &inventory);
[[nodiscard]] bool uiToolIdEnabled(const std::vector<ResourceId> &enabledToolIds, const ResourceId &toolId);
[[nodiscard]] UiToolBeltLayout buildUiToolBeltLayout(
	const UiToolInventory &inventory,
	const std::vector<ResourceId> &enabledToolIds);

} // namespace iggy::ui
