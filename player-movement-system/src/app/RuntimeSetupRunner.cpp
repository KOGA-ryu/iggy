#include "RuntimeSetupRunner.hpp"

#include "session/SessionScriptRunner.hpp"

namespace dev {

RuntimeSetupRunner::RuntimeSetupRunner(
    SessionCommandDispatcher &sessionDispatcher,
    RuntimeSourceDrainer &sourceDrainer)
    : sessionDispatcher_(sessionDispatcher)
    , sourceDrainer_(sourceDrainer)
{
}

RuntimeSetupRunResult RuntimeSetupRunner::run(const RuntimeSetupSettings &settings) const
{
	RuntimeSetupRunResult result;

	if (settings.startupScript.has_value()) {
		result.setup.startupScriptRan = true;
		SessionScriptRunner runner { sessionDispatcher_ };
		result.setup.startupScriptResult = runner.run(*settings.startupScript);
		if (result.setup.startupScriptResult.status == SessionScriptRunStatus::LoadFailed) {
			result.framesAllowed = false;
			return result;
		}
	}

	if (settings.inventoryScript.has_value()) {
		result.setup.inventoryScriptRan = true;
		result.setup.inventoryScriptResult = sourceDrainer_.runInventoryScript(*settings.inventoryScript);
		if (result.setup.inventoryScriptResult.status != InventoryScriptRunStatus::Completed) {
			result.framesAllowed = false;
			return result;
		}
		result.inventoryCommandResults = result.setup.inventoryScriptResult.commandResults;
	}

	if (settings.movementScript.has_value()) {
		result.setup.movementScriptRan = true;
		result.setup.movementScriptResult = sourceDrainer_.runMovementScript(*settings.movementScript);
		if (result.setup.movementScriptResult.status != MovementScriptRunStatus::Completed) {
			result.framesAllowed = false;
			return result;
		}
	}

	return result;
}

} // namespace dev
