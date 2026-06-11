#pragma once

#include <filesystem>
#include <vector>

#include "session/SessionCommandLogFileStore.hpp"
#include "session/SessionCommandReplayer.hpp"

namespace dev {

enum class SessionScriptRunStatus {
	LoadFailed,
	Completed,
};

struct SessionScriptRunResult {
	SessionScriptRunStatus status = SessionScriptRunStatus::LoadFailed;
	std::vector<SessionCommandResult> commandResults;
};

class SessionScriptRunner {
public:
	explicit SessionScriptRunner(
	    SessionCommandDispatcher &dispatcher,
	    SessionCommandLogFileStore fileStore = SessionCommandLogFileStore {});

	[[nodiscard]] SessionScriptRunResult run(const std::filesystem::path &path) const;

private:
	SessionCommandLogFileStore fileStore_;
	SessionCommandReplayer replayer_;
};

} // namespace dev
