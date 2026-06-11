#pragma once

#include <filesystem>

#include "replay/CommandLogFileStore.hpp"
#include "replay/CommandReplayer.hpp"

namespace dev {

enum class MovementScriptRunStatus {
	LoadFailed,
	Completed,
};

struct MovementScriptRunResult {
	MovementScriptRunStatus status = MovementScriptRunStatus::LoadFailed;
	CommandReplayReport replayReport;
};

class MovementScriptRunner {
public:
	explicit MovementScriptRunner(
	    CommandDispatcher &dispatcher,
	    CommandLogFileStore fileStore = CommandLogFileStore {});

	[[nodiscard]] MovementScriptRunResult run(const std::filesystem::path &path) const;

private:
	CommandLogFileStore fileStore_;
	CommandReplayer replayer_;
};

} // namespace dev
