#include "RuntimeStartupScriptIntake.hpp"

namespace dev {

SessionScriptRunResult RuntimeStartupScriptIntake::run(
    const std::filesystem::path &path,
    SessionCommandDispatcher &dispatcher) const
{
	SessionScriptRunner runner { dispatcher };
	return runner.run(path);
}

} // namespace dev
