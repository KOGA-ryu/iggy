#include "scene/ui/UiToolInventory.hpp"

#include <algorithm>
#include <cstddef>

namespace iggy::ui {
namespace {

ResourceId Id(const char *value)
{
	return ResourceId { value };
}

} // namespace

UiToolInventory defaultUiToolInventory()
{
	UiToolInventory inventory;
	inventory.rows = 6;
	inventory.columns = 6;
	inventory.tools = {
		{ Id("tool:select"), "Select", Id("group:runtime"), 0, 0, true },
		{ Id("tool:inspect"), "Inspect", Id("group:runtime"), 0, 1, true },
		{ Id("tool:move"), "Move", Id("group:player"), 1, 0, true },
		{ Id("tool:interact"), "Interact", Id("group:interaction"), 1, 1, true },
		{ Id("tool:pickup"), "Pickup", Id("group:inventory"), 1, 2, true },
		{ Id("tool:collision"), "Collision", Id("group:debug"), 2, 0, true },
		{ Id("tool:render_cache"), "Render Cache", Id("group:debug"), 2, 1, true },
		{ Id("tool:save_slots"), "Save Slots", Id("group:runtime"), 3, 0, true },
		{ Id("tool:settings"), "Settings", Id("group:settings"), 5, 0, true },
	};
	return inventory;
}

const UiToolDescriptor *findUiTool(const UiToolInventory &inventory, const ResourceId &toolId)
{
	const auto found = std::find_if(
		inventory.tools.begin(),
		inventory.tools.end(),
		[&toolId](const UiToolDescriptor &tool) {
			return tool.id == toolId;
		});
	return found == inventory.tools.end() ? nullptr : &*found;
}

std::vector<ResourceId> defaultEnabledUiToolIds(const UiToolInventory &inventory)
{
	std::vector<ResourceId> ids;
	for (const UiToolDescriptor &tool : inventory.tools) {
		if (tool.enabledByDefault)
			ids.push_back(tool.id);
	}
	return ids;
}

bool uiToolIdEnabled(const std::vector<ResourceId> &enabledToolIds, const ResourceId &toolId)
{
	return std::find(enabledToolIds.begin(), enabledToolIds.end(), toolId) != enabledToolIds.end();
}

UiToolBeltLayout buildUiToolBeltLayout(const UiToolInventory &inventory, const std::vector<ResourceId> &enabledToolIds)
{
	UiToolBeltLayout layout;
	layout.rows = inventory.rows;
	layout.columns = inventory.columns;
	if (layout.rows <= 0 || layout.columns <= 0)
		return layout;

	layout.itemIds.resize(static_cast<std::size_t>(layout.rows) * static_cast<std::size_t>(layout.columns));
	for (const UiToolDescriptor &tool : inventory.tools) {
		if (!uiToolIdEnabled(enabledToolIds, tool.id))
			continue;
		if (tool.beltRow < 0 || tool.beltRow >= layout.rows || tool.beltColumn < 0 || tool.beltColumn >= layout.columns)
			continue;
		const std::size_t index = static_cast<std::size_t>(tool.beltRow) * static_cast<std::size_t>(layout.columns)
			+ static_cast<std::size_t>(tool.beltColumn);
		if (index < layout.itemIds.size())
			layout.itemIds[index] = tool.id;
	}
	return layout;
}

} // namespace iggy::ui
