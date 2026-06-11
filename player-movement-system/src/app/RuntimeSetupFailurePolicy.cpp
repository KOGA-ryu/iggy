#include "RuntimeSetupFailurePolicy.hpp"

namespace dev {

bool RuntimeSetupFailurePolicy::failed(const RuntimeSetupResult &result) const
{
	if (result.startupScriptRan && result.startupScriptResult.status == SessionScriptRunStatus::LoadFailed)
		return true;
	if (result.inventoryScriptRan && result.inventoryScriptResult.status != InventoryScriptRunStatus::Completed)
		return true;
	return false;
}

} // namespace dev
