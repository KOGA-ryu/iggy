#include "RuntimeTraceService.hpp"

#include "app/RuntimeRunSummaryText.hpp"

namespace dev {

RuntimeTraceService::RuntimeTraceService(RuntimeFrameTrace formatter, RuntimeFrameTraceFileStore fileStore)
    : formatter_(formatter)
    , fileStore_(fileStore)
{
}

std::vector<std::string> RuntimeTraceService::formatRun(const GameLoopResult &result) const
{
	std::vector<std::string> lines;
	lines.push_back(RuntimeRunSummaryText {}.format(result, RuntimeRunSummaryDetail::CountsOnly));

	for (std::size_t i = 0; i < result.frameReports.size(); ++i) {
		lines.push_back(frameHeader_.format(i));

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
