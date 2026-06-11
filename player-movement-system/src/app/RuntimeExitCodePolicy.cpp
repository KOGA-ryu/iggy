#include "RuntimeExitCodePolicy.hpp"

namespace dev {

RuntimeExitCodePolicy::RuntimeExitCodePolicy(RuntimeRunFailurePolicy runFailurePolicy)
    : runFailurePolicy_(runFailurePolicy)
{
}

int RuntimeExitCodePolicy::exitCodeFor(const GameLoopResult &result) const
{
	return failed(result) ? 1 : 0;
}

bool RuntimeExitCodePolicy::failed(const GameLoopResult &result) const
{
	return runFailurePolicy_.failed(result);
}

} // namespace dev
