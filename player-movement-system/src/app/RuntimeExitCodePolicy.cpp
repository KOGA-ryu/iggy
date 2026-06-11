#include "RuntimeExitCodePolicy.hpp"

namespace dev {

RuntimeExitCodePolicy::RuntimeExitCodePolicy(
    RuntimeSetupFailurePolicy setupFailurePolicy,
    RuntimeOutputFailurePolicy outputFailurePolicy)
    : setupFailurePolicy_(setupFailurePolicy)
    , outputFailurePolicy_(outputFailurePolicy)
{
}

int RuntimeExitCodePolicy::exitCodeFor(const GameLoopResult &result) const
{
	return failed(result) ? 1 : 0;
}

bool RuntimeExitCodePolicy::failed(const GameLoopResult &result) const
{
	if (setupFailurePolicy_.failed(result.setup))
		return true;
	if (outputFailurePolicy_.failed(result.output))
		return true;
	return false;
}

} // namespace dev
