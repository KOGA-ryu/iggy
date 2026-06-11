#include "RuntimeRunSummaryText.hpp"

#include <sstream>

namespace dev {

namespace {

const char *ToString(GameSessionMode mode)
{
	switch (mode) {
	case GameSessionMode::Empty:
		return "Empty";
	case GameSessionMode::Gameplay:
		return "Gameplay";
	case GameSessionMode::Paused:
		return "Paused";
	case GameSessionMode::Inventory:
		return "Inventory";
	}
	return "Unknown";
}

} // namespace

std::string RuntimeRunSummaryText::format(const GameLoopResult &result, RuntimeRunSummaryDetail detail) const
{
	std::ostringstream summary;
	summary << "run frames=" << result.summary.framesRun
	        << " frameReports=" << result.frameReports.size()
	        << " rawInput=" << result.summary.rawInputEventsRouted
	        << " movementInputBlocks=" << result.summary.movementInputBlockReasons.size()
	        << " sessionResults=" << result.summary.sessionCommandResults.size()
	        << " inventoryScripts=" << result.summary.runtimeInventoryScriptResults.size()
	        << " inventoryResults=" << result.summary.inventoryCommandResults.size()
	        << " movementScripts=" << result.summary.runtimeMovementScriptResults.size()
	        << " movementQueued=" << result.summary.movementCommandsQueued;

	if (detail == RuntimeRunSummaryDetail::WithFinalMode)
		summary << " finalMode=" << ToString(result.finalMode);

	return summary.str();
}

} // namespace dev
