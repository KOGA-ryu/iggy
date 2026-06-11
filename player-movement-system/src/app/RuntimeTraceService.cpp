#include "RuntimeTraceService.hpp"

#include <sstream>

namespace dev {

RuntimeTraceService::RuntimeTraceService(RuntimeFrameTrace formatter, RuntimeFrameTraceFileStore fileStore)
    : formatter_(formatter)
    , fileStore_(fileStore)
{
}

std::vector<std::string> RuntimeTraceService::formatRun(const GameLoopResult &result) const
{
	std::vector<std::string> lines;
	{
		std::ostringstream summary;
		summary << "run frames=" << result.summary.framesRun
		        << " frameReports=" << result.frameReports.size()
		        << " rawInput=" << result.summary.rawInputEventsRouted
		        << " sessionResults=" << result.summary.sessionCommandResults.size()
		        << " inventoryScripts=" << result.summary.runtimeInventoryScriptResults.size()
		        << " inventoryResults=" << result.summary.inventoryCommandResults.size()
		        << " movementQueued=" << result.summary.movementCommandsQueued;
		lines.push_back(summary.str());
	}

	for (std::size_t i = 0; i < result.frameReports.size(); ++i) {
		std::ostringstream header;
		header << "frame[" << i << "]";
		lines.push_back(header.str());

		std::vector<std::string> frameLines = formatter_.format(result.frameReports[i]);
		lines.insert(lines.end(), frameLines.begin(), frameLines.end());
	}

	return lines;
}

bool RuntimeTraceService::saveRunTrace(const std::filesystem::path &path, const GameLoopResult &result) const
{
	return fileStore_.save(path, formatRun(result));
}

} // namespace dev
