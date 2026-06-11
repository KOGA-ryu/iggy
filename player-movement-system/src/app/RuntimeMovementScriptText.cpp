#include "RuntimeMovementScriptText.hpp"

#include <sstream>

namespace dev {

namespace {

const char *ToString(MovementScriptRunStatus status)
{
	switch (status) {
	case MovementScriptRunStatus::LoadFailed:
		return "LoadFailed";
	case MovementScriptRunStatus::NoActiveWorld:
		return "NoActiveWorld";
	case MovementScriptRunStatus::Completed:
		return "Completed";
	}
	return "Unknown";
}

std::size_t CountStatus(const std::vector<MovementScriptRunResult> &results, MovementScriptRunStatus status)
{
	std::size_t count = 0;
	for (const MovementScriptRunResult &result : results) {
		if (result.status == status)
			++count;
	}
	return count;
}

std::size_t CountAccepted(const std::vector<MovementScriptRunResult> &results)
{
	std::size_t count = 0;
	for (const MovementScriptRunResult &result : results)
		count += result.replayReport.acceptedCount();
	return count;
}

std::size_t CountRejected(const std::vector<MovementScriptRunResult> &results)
{
	std::size_t count = 0;
	for (const MovementScriptRunResult &result : results)
		count += result.replayReport.rejectedCount();
	return count;
}

} // namespace

std::string RuntimeMovementScriptText::formatResult(std::string_view label, const MovementScriptRunResult &result) const
{
	std::ostringstream line;
	line << label << " status=" << ToString(result.status)
	     << " results=" << result.replayReport.results.size()
	     << " accepted=" << result.replayReport.acceptedCount()
	     << " rejected=" << result.replayReport.rejectedCount();
	return line.str();
}

std::string RuntimeMovementScriptText::formatAggregate(std::string_view label, const std::vector<MovementScriptRunResult> &results) const
{
	std::ostringstream line;
	line << label << "=" << results.size()
	     << " completed=" << CountStatus(results, MovementScriptRunStatus::Completed)
	     << " loadFailed=" << CountStatus(results, MovementScriptRunStatus::LoadFailed)
	     << " noActiveWorld=" << CountStatus(results, MovementScriptRunStatus::NoActiveWorld)
	     << " accepted=" << CountAccepted(results)
	     << " rejected=" << CountRejected(results);
	return line.str();
}

} // namespace dev
