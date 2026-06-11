#include "RuntimeDebugArtifactBundle.hpp"

#include <sstream>

#include "files/TextFileStore.hpp"

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

bool RuntimeDebugArtifactBundleResult::saved() const
{
	return rootPrepared && traceSaved && manifestSaved;
}

RuntimeDebugArtifactBundle::RuntimeDebugArtifactBundle(RuntimeTraceService traceService)
    : traceService_(traceService)
{
}

RuntimeDebugArtifactBundleResult RuntimeDebugArtifactBundle::save(
    const std::filesystem::path &rootPath,
    const GameLoopResult &result) const
{
	RuntimeDebugArtifactBundleResult bundle {
		.rootPath = rootPath,
		.manifestPath = rootPath / "manifest.txt",
		.tracePath = rootPath / "run.trace",
	};

	std::error_code error;
	std::filesystem::create_directories(rootPath, error);
	if (error)
		return bundle;

	bundle.rootPrepared = true;
	bundle.traceSaved = traceService_.saveRunTrace(bundle.tracePath, result);
	bundle.manifestSaved = TextFileStore {}.saveLines(bundle.manifestPath, formatManifest(result, bundle));
	return bundle;
}

std::vector<std::string> RuntimeDebugArtifactBundle::formatManifest(
    const GameLoopResult &result,
    const RuntimeDebugArtifactBundleResult &bundle) const
{
	std::vector<std::string> lines;
	lines.push_back("bundle version=1");
	lines.push_back("trace=run.trace saved=" + std::string { BoolText(bundle.traceSaved) });

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

	lines.push_back("paths root=" + bundle.rootPath.string());
	lines.push_back("paths manifest=" + bundle.manifestPath.filename().string());
	lines.push_back("paths trace=" + bundle.tracePath.filename().string());
	return lines;
}

} // namespace dev
