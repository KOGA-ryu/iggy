#include "RuntimeExitCodePolicy.hpp"

#include "app/RuntimeOutputFinalizer.hpp"

namespace dev {

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
	if (RuntimeOutputFinalizer::failed(result.output))
		return true;
	return false;
}

} // namespace dev
