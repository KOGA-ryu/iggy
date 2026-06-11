#include "RuntimeExitCodePolicy.hpp"

namespace dev {

RuntimeExitCodePolicy::RuntimeExitCodePolicy(
    RuntimeRunFailurePolicy runFailurePolicy,
    RuntimeExitCodeMapper exitCodeMapper)
    : runFailurePolicy_(runFailurePolicy)
    , exitCodeMapper_(exitCodeMapper)
{
}

int RuntimeExitCodePolicy::exitCodeFor(const GameLoopResult &result) const
{
	return exitCodeMapper_.exitCodeFor(runFailurePolicy_.failed(result));
}

} // namespace dev
