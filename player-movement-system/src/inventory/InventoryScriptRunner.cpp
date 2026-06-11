#include "InventoryScriptRunner.hpp"

#include <optional>

namespace dev {

InventoryScriptRunner::InventoryScriptRunner(InventoryCommandDispatcher &dispatcher, InventoryCommandLogFileStore fileStore)
    : fileStore_(fileStore)
    , replayer_(dispatcher)
{
}

InventoryScriptRunResult InventoryScriptRunner::run(const std::filesystem::path &path) const
{
	std::optional<InventoryCommandLog> log = fileStore_.load(path);
	if (!log.has_value())
		return {};

	return {
		.status = InventoryScriptRunStatus::Completed,
		.commandResults = replayer_.replay(*log),
	};
}

} // namespace dev
