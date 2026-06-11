#include "SessionScriptRunner.hpp"

#include <optional>

namespace dev {

SessionScriptRunner::SessionScriptRunner(SessionCommandDispatcher &dispatcher, SessionCommandLogFileStore fileStore)
    : fileStore_(fileStore)
    , replayer_(dispatcher)
{
}

SessionScriptRunResult SessionScriptRunner::run(const std::filesystem::path &path) const
{
	std::optional<SessionCommandLog> log = fileStore_.load(path);
	if (!log.has_value())
		return {};

	return {
		.status = SessionScriptRunStatus::Completed,
		.commandResults = replayer_.replay(*log),
	};
}

} // namespace dev
