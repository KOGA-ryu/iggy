#include "RuntimeSetupRunner.hpp"

#include "app/RuntimeStartupScriptIntake.hpp"

namespace dev {

RuntimeSetupRunner::RuntimeSetupRunner(
    SessionCommandDispatcher &sessionDispatcher,
    RuntimeSourceDrainer &sourceDrainer,
    RuntimeSetupFailurePolicy setupFailurePolicy)
    : sessionDispatcher_(sessionDispatcher)
    , sourceDrainer_(sourceDrainer)
    , setupFailurePolicy_(setupFailurePolicy)
{
}

RuntimeSetupRunResult RuntimeSetupRunner::run(const RuntimeSetupSettings &settings) const
{
	RuntimeSetupRunResult result;

	if (settings.startupScript.has_value()) {
		result.setup.startupScriptRan = true;
		result.setup.startupScriptResult = RuntimeStartupScriptIntake {}.run(
		    *settings.startupScript,
		    sessionDispatcher_);
		if (setupFailurePolicy_.failed(result.setup)) {
			result.framesAllowed = false;
			return result;
		}
	}

	if (settings.inventoryScript.has_value()) {
		result.setup.inventoryScriptRan = true;
		result.setup.inventoryScriptResult = sourceDrainer_.runInventoryScript(*settings.inventoryScript);
		if (setupFailurePolicy_.failed(result.setup)) {
			result.framesAllowed = false;
			return result;
		}
		result.inventoryCommandResults = result.setup.inventoryScriptResult.commandResults;
	}

	if (settings.movementScript.has_value()) {
		result.setup.movementScriptRan = true;
		result.setup.movementScriptResult = sourceDrainer_.runMovementScript(*settings.movementScript);
		if (setupFailurePolicy_.failed(result.setup)) {
			result.framesAllowed = false;
			return result;
		}
	}

	return result;
}

} // namespace dev
