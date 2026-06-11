#include "RuntimeDebugManifest.hpp"

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

const char *BoolText(bool value)
{
	return value ? "true" : "false";
}

} // namespace

std::vector<std::string> RuntimeDebugManifest::format(
    const GameLoopResult &result,
    const RuntimeDebugManifestContext &context) const
{
	std::vector<std::string> lines;
	lines.push_back("bundle version=1");
	lines.push_back("trace=run.trace saved=" + std::string { BoolText(context.traceSaved) });

	{
		std::ostringstream summary;
		summary << "run frames=" << result.summary.framesRun
		        << " frameReports=" << result.frameReports.size()
		        << " rawInput=" << result.summary.rawInputEventsRouted
		        << " sessionResults=" << result.summary.sessionCommandResults.size()
		        << " inventoryScripts=" << result.summary.runtimeInventoryScriptResults.size()
		        << " inventoryResults=" << result.summary.inventoryCommandResults.size()
		        << " movementQueued=" << result.summary.movementCommandsQueued
		        << " finalMode=" << ToString(result.finalMode);
		lines.push_back(summary.str());
	}

	{
		std::ostringstream setup;
		setup << "setup startupScriptRan=" << BoolText(result.setup.startupScriptRan)
		      << " inventoryScriptRan=" << BoolText(result.setup.inventoryScriptRan);
		lines.push_back(setup.str());
	}

	lines.push_back("paths root=" + context.rootPath.string());
	lines.push_back("paths manifest=" + context.manifestPath.filename().string());
	lines.push_back("paths trace=" + context.tracePath.filename().string());
	return lines;
}

} // namespace dev
