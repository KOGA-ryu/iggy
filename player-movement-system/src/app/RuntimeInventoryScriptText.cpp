#include "RuntimeInventoryScriptText.hpp"

#include <sstream>

namespace dev {

namespace {

const char *ToString(InventoryScriptRunStatus status)
{
	switch (status) {
	case InventoryScriptRunStatus::LoadFailed:
		return "LoadFailed";
	case InventoryScriptRunStatus::NoActivePlayer:
		return "NoActivePlayer";
	case InventoryScriptRunStatus::Completed:
		return "Completed";
	}
	return "Unknown";
}

std::size_t CountStatus(const std::vector<InventoryScriptRunResult> &results, InventoryScriptRunStatus status)
{
	std::size_t count = 0;
	for (const InventoryScriptRunResult &result : results) {
		if (result.status == status)
			++count;
	}
	return count;
}

std::size_t CountApplied(const std::vector<InventoryCommandResult> &results)
{
	std::size_t count = 0;
	for (const InventoryCommandResult &result : results) {
		if (result.type == InventoryCommandResultType::Applied)
			++count;
	}
	return count;
}

std::size_t CountRejected(const std::vector<InventoryCommandResult> &results)
{
	std::size_t count = 0;
	for (const InventoryCommandResult &result : results) {
		if (result.type == InventoryCommandResultType::Rejected)
			++count;
	}
	return count;
}

std::size_t CountApplied(const std::vector<InventoryScriptRunResult> &results)
{
	std::size_t count = 0;
	for (const InventoryScriptRunResult &result : results)
		count += CountApplied(result.commandResults);
	return count;
}

std::size_t CountRejected(const std::vector<InventoryScriptRunResult> &results)
{
	std::size_t count = 0;
	for (const InventoryScriptRunResult &result : results)
		count += CountRejected(result.commandResults);
	return count;
}

} // namespace

std::string RuntimeInventoryScriptText::formatResult(std::string_view label, const InventoryScriptRunResult &result) const
{
	std::ostringstream line;
	line << label << " status=" << ToString(result.status)
	     << " results=" << result.commandResults.size()
	     << " applied=" << CountApplied(result.commandResults)
	     << " rejected=" << CountRejected(result.commandResults);
	return line.str();
}

std::string RuntimeInventoryScriptText::formatAggregate(std::string_view label, const std::vector<InventoryScriptRunResult> &results) const
{
	std::ostringstream line;
	line << label << "=" << results.size()
	     << " completed=" << CountStatus(results, InventoryScriptRunStatus::Completed)
	     << " loadFailed=" << CountStatus(results, InventoryScriptRunStatus::LoadFailed)
	     << " noActivePlayer=" << CountStatus(results, InventoryScriptRunStatus::NoActivePlayer)
	     << " applied=" << CountApplied(results)
	     << " rejected=" << CountRejected(results);
	return line.str();
}

} // namespace dev
