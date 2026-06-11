#include "RuntimeRunFailurePolicy.hpp"

namespace dev {

RuntimeRunFailurePolicy::RuntimeRunFailurePolicy(
    RuntimeSetupFailurePolicy setupFailurePolicy,
    RuntimeOutputFailurePolicy outputFailurePolicy)
    : setupFailurePolicy_(setupFailurePolicy)
    , outputFailurePolicy_(outputFailurePolicy)
{
}

bool RuntimeRunFailurePolicy::failed(const GameLoopResult &result) const
{
	if (setupFailurePolicy_.failed(result.setup))
		return true;
	if (outputFailurePolicy_.failed(result.output))
		return true;
	return false;
}

} // namespace dev
