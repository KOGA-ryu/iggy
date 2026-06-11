#include "MovementScriptRunner.hpp"

#include <optional>

namespace dev {

MovementScriptRunner::MovementScriptRunner(CommandDispatcher &dispatcher, CommandLogFileStore fileStore)
    : fileStore_(fileStore)
    , replayer_(dispatcher)
{
}

MovementScriptRunResult MovementScriptRunner::run(const std::filesystem::path &path) const
{
	std::optional<CommandLog> log = fileStore_.load(path);
	if (!log.has_value())
		return {};

	return {
		.status = MovementScriptRunStatus::Completed,
		.replayReport = replayer_.replay(*log),
	};
}

} // namespace dev
