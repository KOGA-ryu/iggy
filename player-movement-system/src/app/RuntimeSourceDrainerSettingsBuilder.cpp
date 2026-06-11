#include "RuntimeSourceDrainerSettingsBuilder.hpp"

namespace dev {

RuntimeSourceDrainerSettings RuntimeSourceDrainerSettingsBuilder::build(
    const RuntimeSourceSettings &sources,
    const RuntimeInputSettings &input) const
{
	return {
		.sessionCommandSources = sources.sessionCommandSources,
		.inventoryScriptSources = sources.inventoryScriptSources,
		.inventoryCommandSources = sources.inventoryCommandSources,
		.movementCommandSources = sources.movementCommandSources,
		.inputPlayerId = input.playerId,
	};
}

} // namespace dev
