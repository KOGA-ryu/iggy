#pragma once

#include <filesystem>

#include "session/SessionCommandDispatcher.hpp"
#include "session/SessionScriptRunner.hpp"

namespace dev {

class RuntimeStartupScriptIntake {
public:
	[[nodiscard]] SessionScriptRunResult run(
	    const std::filesystem::path &path,
	    SessionCommandDispatcher &dispatcher) const;
};

} // namespace dev
