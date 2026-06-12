#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>

#include "app/RuntimeExitCodeMapper.hpp"
#include "app/RuntimeExitCodePolicy.hpp"
#include "app/RuntimeFinalModeRecorder.hpp"
#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeOutputFailurePolicy.hpp"
#include "app/RuntimeRunFailurePolicy.hpp"
#include "app/RuntimeRunFinalizer.hpp"
#include "app/RuntimeSetupFailurePolicy.hpp"
#include "inventory/InventoryCommand.hpp"
#include "inventory/InventoryEvent.hpp"
#include "replay/CommandReplayReport.hpp"
#include "session/GameSession.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

void TestRuntimeRunFinalizerCapturesFinalModeAndLeavesDisabledOutputsUntouched()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_run_finalizer_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	session.startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	session.setMode(dev::GameSessionMode::Inventory);
	dev::GameLoopResult result;

	dev::RuntimeRunFinalizer {}.finalize(session, dev::RuntimeOutputSettings {}, result);

	Expect(result.finalMode == dev::GameSessionMode::Inventory, "runtime run finalizer should capture final session mode");
	Expect(!result.output.runTraceSaveAttempted, "runtime run finalizer should leave disabled trace output untouched");
	Expect(!result.output.debugBundleSaveAttempted, "runtime run finalizer should leave disabled bundle output untouched");

	std::filesystem::remove_all(root);
}

void TestRuntimeFinalModeRecorderCapturesSessionMode()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_final_mode_recorder_test";
	std::filesystem::remove_all(root);

	dev::GameSession session { root / "saves" };
	session.startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	session.setMode(dev::GameSessionMode::Inventory);
	dev::GameLoopResult result;

	dev::RuntimeFinalModeRecorder {}.record(session, result);

	Expect(result.finalMode == dev::GameSessionMode::Inventory, "runtime final mode recorder should capture current session mode");

	std::filesystem::remove_all(root);
}

void TestRuntimeExitCodePolicyReportsSuccessForCleanRun()
{
	dev::GameLoopResult result;

	Expect(dev::RuntimeExitCodePolicy {}.exitCodeFor(result) == 0, "runtime exit policy should return success for clean run results");
}

void TestRuntimeExitCodeMapperMapsFailureBooleanToProcessCode()
{
	dev::RuntimeExitCodeMapper mapper;

	Expect(mapper.exitCodeFor(false) == 0, "runtime exit code mapper should return success for non-failed runs");
	Expect(mapper.exitCodeFor(true) == 1, "runtime exit code mapper should return failure for failed runs");
}

void TestRuntimeExitCodePolicyFailsSetupErrors()
{
	dev::GameLoopResult startupFailure;
	startupFailure.setup.startupScriptRan = true;
	startupFailure.setup.startupScriptResult.status = dev::SessionScriptRunStatus::LoadFailed;

	dev::GameLoopResult inventoryFailure;
	inventoryFailure.setup.inventoryScriptRan = true;
	inventoryFailure.setup.inventoryScriptResult.status = dev::InventoryScriptRunStatus::LoadFailed;

	dev::GameLoopResult movementFailure;
	movementFailure.setup.movementScriptRan = true;
	movementFailure.setup.movementScriptResult.status = dev::MovementScriptRunStatus::LoadFailed;

	Expect(dev::RuntimeExitCodePolicy {}.exitCodeFor(startupFailure) == 1, "runtime exit policy should return failure for startup load failures");
	Expect(dev::RuntimeExitCodePolicy {}.exitCodeFor(inventoryFailure) == 1, "runtime exit policy should return failure for configured inventory script failures");
	Expect(dev::RuntimeExitCodePolicy {}.exitCodeFor(movementFailure) == 1, "runtime exit policy should return failure for configured movement script failures");
}

void TestRuntimeSetupFailurePolicyFailsSetupLoadErrors()
{
	dev::RuntimeSetupResult clean;

	dev::RuntimeSetupResult startupFailure;
	startupFailure.startupScriptRan = true;
	startupFailure.startupScriptResult.status = dev::SessionScriptRunStatus::LoadFailed;

	dev::RuntimeSetupResult inventoryFailure;
	inventoryFailure.inventoryScriptRan = true;
	inventoryFailure.inventoryScriptResult.status = dev::InventoryScriptRunStatus::LoadFailed;

	dev::RuntimeSetupResult movementFailure;
	movementFailure.movementScriptRan = true;
	movementFailure.movementScriptResult.status = dev::MovementScriptRunStatus::LoadFailed;

	dev::RuntimeSetupResult rejectedInventoryCommand;
	rejectedInventoryCommand.inventoryScriptRan = true;
	rejectedInventoryCommand.inventoryScriptResult.status = dev::InventoryScriptRunStatus::Completed;
	rejectedInventoryCommand.inventoryScriptResult.commandResults.push_back({
	    .type = dev::InventoryCommandResultType::Rejected,
	    .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 99 },
	});

	dev::RuntimeSetupResult rejectedMovementCommand;
	rejectedMovementCommand.movementScriptRan = true;
	rejectedMovementCommand.movementScriptResult.status = dev::MovementScriptRunStatus::Completed;
	rejectedMovementCommand.movementScriptResult.replayReport.results.push_back({
	    .type = dev::MovementCommandDispatchResultType::Rejected,
	    .command = {
	        .type = dev::MovementCommandType::MoveThenAct,
	        .playerId = 0,
	        .destination = { 1, 0 },
	        .destinationAction = std::nullopt,
	    },
	});

	dev::RuntimeSetupFailurePolicy policy;

	Expect(!policy.failed(clean), "runtime setup failure policy should not fail clean setup state");
	Expect(policy.failed(startupFailure), "runtime setup failure policy should fail startup load failures");
	Expect(policy.failed(inventoryFailure), "runtime setup failure policy should fail configured inventory setup failures");
	Expect(policy.failed(movementFailure), "runtime setup failure policy should fail configured movement setup failures");
	Expect(!policy.failed(rejectedInventoryCommand), "runtime setup failure policy should allow completed scripts with rejected commands");
	Expect(!policy.failed(rejectedMovementCommand), "runtime setup failure policy should allow completed movement scripts with rejected commands");
}

void TestRuntimeRunFailurePolicyComposesSetupAndOutputFailures()
{
	dev::GameLoopResult clean;

	dev::GameLoopResult setupFailure;
	setupFailure.setup.startupScriptRan = true;
	setupFailure.setup.startupScriptResult.status = dev::SessionScriptRunStatus::LoadFailed;

	dev::GameLoopResult outputFailure;
	outputFailure.output.debugBundleSaveAttempted = true;
	outputFailure.output.debugBundleSaved = false;

	dev::GameLoopResult commandRejection;
	commandRejection.setup.inventoryScriptRan = true;
	commandRejection.setup.inventoryScriptResult.status = dev::InventoryScriptRunStatus::Completed;
	commandRejection.setup.inventoryScriptResult.commandResults.push_back({
	    .type = dev::InventoryCommandResultType::Rejected,
	    .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 99 },
	});

	dev::GameLoopResult movementCommandRejection;
	movementCommandRejection.setup.movementScriptRan = true;
	movementCommandRejection.setup.movementScriptResult.status = dev::MovementScriptRunStatus::Completed;
	movementCommandRejection.setup.movementScriptResult.replayReport.results.push_back({
	    .type = dev::MovementCommandDispatchResultType::Rejected,
	    .command = {
	        .type = dev::MovementCommandType::MoveThenAct,
	        .playerId = 0,
	        .destination = { 1, 0 },
	        .destinationAction = std::nullopt,
	    },
	});

	dev::RuntimeRunFailurePolicy policy;

	Expect(!policy.failed(clean), "runtime run failure policy should not fail clean runs");
	Expect(policy.failed(setupFailure), "runtime run failure policy should fail setup failures");
	Expect(policy.failed(outputFailure), "runtime run failure policy should fail output failures");
	Expect(!policy.failed(commandRejection), "runtime run failure policy should allow command-level rejections");
	Expect(!policy.failed(movementCommandRejection), "runtime run failure policy should allow movement command-level rejections");
}

void TestRuntimeExitCodePolicyAllowsCommandRejections()
{
	dev::GameLoopResult result;
	result.setup.inventoryScriptRan = true;
	result.setup.inventoryScriptResult.status = dev::InventoryScriptRunStatus::Completed;
	result.setup.inventoryScriptResult.commandResults.push_back({
	    .type = dev::InventoryCommandResultType::Rejected,
	    .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 99 },
	});
	result.setup.movementScriptRan = true;
	result.setup.movementScriptResult.status = dev::MovementScriptRunStatus::Completed;
	result.setup.movementScriptResult.replayReport.results.push_back({
	    .type = dev::MovementCommandDispatchResultType::Rejected,
	    .command = {
	        .type = dev::MovementCommandType::MoveThenAct,
	        .playerId = 0,
	        .destination = { 1, 0 },
	        .destinationAction = std::nullopt,
	    },
	});

	Expect(dev::RuntimeExitCodePolicy {}.exitCodeFor(result) == 0, "runtime exit policy should return success for command-level rejections");
}

void TestRuntimeExitCodePolicyFailsOutputErrors()
{
	dev::GameLoopResult result;
	result.output.runTraceSaveAttempted = true;
	result.output.runTraceSaved = false;

	Expect(dev::RuntimeExitCodePolicy {}.exitCodeFor(result) == 1, "runtime exit policy should return failure for output write failures");
}

void TestRuntimeOutputFailurePolicyFailsAttemptedUnsavedOutputs()
{
	dev::RuntimeOutputResult clean;

	dev::RuntimeOutputResult unattemptedUnsaved;
	unattemptedUnsaved.runTraceSaved = false;
	unattemptedUnsaved.debugBundleSaved = false;

	dev::RuntimeOutputResult failedTrace;
	failedTrace.runTraceSaveAttempted = true;
	failedTrace.runTraceSaved = false;

	dev::RuntimeOutputResult failedBundle;
	failedBundle.debugBundleSaveAttempted = true;
	failedBundle.debugBundleSaved = false;

	dev::RuntimeOutputResult savedOutputs;
	savedOutputs.runTraceSaveAttempted = true;
	savedOutputs.runTraceSaved = true;
	savedOutputs.debugBundleSaveAttempted = true;
	savedOutputs.debugBundleSaved = true;

	dev::RuntimeOutputFailurePolicy policy;

	Expect(!policy.failed(clean), "runtime output failure policy should not fail clean output state");
	Expect(!policy.failed(unattemptedUnsaved), "runtime output failure policy should ignore unattempted unsaved outputs");
	Expect(policy.failed(failedTrace), "runtime output failure policy should fail attempted unsaved trace output");
	Expect(policy.failed(failedBundle), "runtime output failure policy should fail attempted unsaved bundle output");
	Expect(!policy.failed(savedOutputs), "runtime output failure policy should not fail saved requested outputs");
}

} // namespace

int main()
{
	TestRuntimeRunFinalizerCapturesFinalModeAndLeavesDisabledOutputsUntouched();
	TestRuntimeFinalModeRecorderCapturesSessionMode();
	TestRuntimeExitCodePolicyReportsSuccessForCleanRun();
	TestRuntimeExitCodeMapperMapsFailureBooleanToProcessCode();
	TestRuntimeExitCodePolicyFailsSetupErrors();
	TestRuntimeSetupFailurePolicyFailsSetupLoadErrors();
	TestRuntimeRunFailurePolicyComposesSetupAndOutputFailures();
	TestRuntimeExitCodePolicyAllowsCommandRejections();
	TestRuntimeExitCodePolicyFailsOutputErrors();
	TestRuntimeOutputFailurePolicyFailsAttemptedUnsavedOutputs();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
