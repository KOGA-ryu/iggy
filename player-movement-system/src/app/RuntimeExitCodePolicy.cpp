#include "RuntimeExitCodePolicy.hpp"

namespace dev {

RuntimeExitCodePolicy::RuntimeExitCodePolicy(RuntimeOutputFailurePolicy outputFailurePolicy)
    : outputFailurePolicy_(outputFailurePolicy)
{
}

int RuntimeExitCodePolicy::exitCodeFor(const GameLoopResult &result) const
{
	return failed(result) ? 1 : 0;
}

bool RuntimeExitCodePolicy::failed(const GameLoopResult &result) const
{
	if (result.setup.startupScriptRan && result.setup.startupScriptResult.status == SessionScriptRunStatus::LoadFailed)
		return true;
	if (result.setup.inventoryScriptRan && result.setup.inventoryScriptResult.status != InventoryScriptRunStatus::Completed)
		return true;
	if (outputFailurePolicy_.failed(result.output))
		return true;
	return false;
}

} // namespace dev
