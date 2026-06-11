#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "app/RuntimeArtifactOutputPlan.hpp"
#include "app/RuntimeArtifactOutputRequestRunner.hpp"
#include "app/RuntimeDebugBundleOutputStep.hpp"
#include "app/RuntimeFrameTraceFileStore.hpp"
#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeOutputFailurePolicy.hpp"
#include "app/RuntimeOutputResultBuilder.hpp"
#include "app/RuntimeRunTraceOutputStep.hpp"
#include "session/GameSessionMode.hpp"

namespace {

int Failures = 0;

void Expect(bool condition, std::string_view message)
{
	if (condition)
		return;
	std::cerr << "FAIL: " << message << '\n';
	++Failures;
}

bool ContainsLineFragment(const std::vector<std::string> &lines, std::string_view fragment)
{
	for (const std::string &line : lines) {
		if (line.find(fragment) != std::string::npos)
			return true;
	}
	return false;
}

void TestRuntimeOutputSettingsDefaultDisablesArtifacts()
{
	dev::RuntimeOutputSettings output;

	Expect(!output.runTracePath.has_value(), "runtime output settings should default to no run trace path");
	Expect(!output.debugBundlePath.has_value(), "runtime output settings should default to no debug bundle path");
}

void TestRuntimeOutputResultDefaultsToNoAttempts()
{
	dev::RuntimeOutputResult output;

	Expect(!output.runTraceSaveAttempted, "runtime output result should default to no trace attempt");
	Expect(!output.runTraceSaved, "runtime output result should default to unsaved trace");
	Expect(!output.debugBundleSaveAttempted, "runtime output result should default to no bundle attempt");
	Expect(!output.debugBundleSaved, "runtime output result should default to unsaved bundle");
	Expect(!dev::RuntimeOutputFailurePolicy {}.failed(output), "runtime output result should not fail when nothing was requested");
}

void TestRuntimeOutputResultBuilderRecordsArtifactAttempts()
{
	dev::RuntimeOutputResultBuilder builder;

	builder.beginRunTraceSave();
	dev::GameLoopResult traceAttempt = builder.applyTo(dev::GameLoopResult {});
	Expect(traceAttempt.output.runTraceSaveAttempted, "runtime output result builder should apply in-progress trace attempt to run result");
	Expect(!traceAttempt.output.runTraceSaved, "runtime output result builder should not mark trace saved before completion");

	builder.completeRunTraceSave(true);
	builder.beginDebugBundleSave();
	dev::GameLoopResult bundleAttempt = builder.applyTo(dev::GameLoopResult {});
	Expect(bundleAttempt.output.runTraceSaveAttempted && bundleAttempt.output.runTraceSaved, "runtime output result builder should preserve completed trace result");
	Expect(bundleAttempt.output.debugBundleSaveAttempted, "runtime output result builder should apply in-progress bundle attempt to run result");
	Expect(!bundleAttempt.output.debugBundleSaved, "runtime output result builder should not mark bundle saved before completion");

	builder.completeDebugBundleSave(false);
	dev::RuntimeOutputResult output = builder.result();
	Expect(output.runTraceSaveAttempted && output.runTraceSaved, "runtime output result builder should report completed trace save");
	Expect(output.debugBundleSaveAttempted && !output.debugBundleSaved, "runtime output result builder should report failed bundle save");
}

void TestRuntimeRunTraceOutputStepSavesTraceAndUpdatesOutput()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_run_trace_output_step_test";
	const std::filesystem::path tracePath = root / "run.trace";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::GameLoopResult result;
	result.summary.framesRun = 2;
	dev::RuntimeOutputResultBuilder output;

	dev::RuntimeRunTraceOutputStep {}.save(tracePath, result, output);

	std::optional<std::vector<std::string>> trace = dev::RuntimeFrameTraceFileStore {}.load(tracePath);
	const dev::RuntimeOutputResult outputResult = output.result();

	Expect(outputResult.runTraceSaveAttempted, "runtime run trace output step should mark trace save attempted");
	Expect(outputResult.runTraceSaved, "runtime run trace output step should record successful trace save");
	Expect(!outputResult.debugBundleSaveAttempted, "runtime run trace output step should not touch debug bundle output flags");
	Expect(trace.has_value() && !trace->empty(), "runtime run trace output step should save trace lines");
	Expect(trace.has_value() && (*trace)[0] == "run frames=2 frameReports=0 rawInput=0 movementInputBlocks=0 sessionResults=0 inventoryScripts=0 inventoryResults=0 movementScripts=0 movementQueued=0", "runtime run trace output step should save the supplied run result snapshot");

	std::filesystem::remove_all(root);
}

void TestRuntimeDebugBundleOutputStepSavesBundleAndUpdatesOutput()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_debug_bundle_output_step_test";
	const std::filesystem::path bundlePath = root / "bundle";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::GameLoopResult result;
	result.summary.framesRun = 3;
	result.finalMode = dev::GameSessionMode::Gameplay;
	dev::RuntimeOutputResult existingOutput;
	existingOutput.runTraceSaveAttempted = true;
	existingOutput.runTraceSaved = true;
	dev::RuntimeOutputResultBuilder output { existingOutput };

	dev::RuntimeDebugBundleOutputStep {}.save(bundlePath, result, output);

	std::optional<std::vector<std::string>> manifest = dev::RuntimeFrameTraceFileStore {}.load(bundlePath / "manifest.txt");
	std::optional<std::vector<std::string>> trace = dev::RuntimeFrameTraceFileStore {}.load(bundlePath / "run.trace");
	const dev::RuntimeOutputResult outputResult = output.result();

	Expect(outputResult.runTraceSaveAttempted && outputResult.runTraceSaved, "runtime debug bundle output step should preserve existing trace output flags");
	Expect(outputResult.debugBundleSaveAttempted, "runtime debug bundle output step should mark bundle save attempted");
	Expect(outputResult.debugBundleSaved, "runtime debug bundle output step should record successful bundle save");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "trace=run.trace saved=true"), "runtime debug bundle output step should save manifest lines");
	Expect(trace.has_value() && !trace->empty(), "runtime debug bundle output step should save trace lines");
	Expect(trace.has_value() && (*trace)[0] == "run frames=3 frameReports=0 rawInput=0 movementInputBlocks=0 sessionResults=0 inventoryScripts=0 inventoryResults=0 movementScripts=0 movementQueued=0", "runtime debug bundle output step should save the supplied run result snapshot");

	std::filesystem::remove_all(root);
}

void TestRuntimeArtifactOutputPlanBuildsOrderedRequests()
{
	const std::filesystem::path tracePath = "debug/run.trace";
	const std::filesystem::path bundlePath = "debug/bundle";
	dev::RuntimeArtifactOutputPlan plan;

	std::vector<dev::RuntimeArtifactOutputRequest> disabled = plan.build({});
	std::vector<dev::RuntimeArtifactOutputRequest> traceOnly = plan.build({
	    .runTracePath = tracePath,
	});
	std::vector<dev::RuntimeArtifactOutputRequest> bundleOnly = plan.build({
	    .debugBundlePath = bundlePath,
	});
	std::vector<dev::RuntimeArtifactOutputRequest> both = plan.build({
	    .runTracePath = tracePath,
	    .debugBundlePath = bundlePath,
	});

	Expect(disabled.empty(), "runtime artifact output plan should skip disabled outputs");
	Expect(traceOnly.size() == 1, "runtime artifact output plan should include configured trace output");
	Expect(traceOnly.size() == 1 && traceOnly[0].kind == dev::RuntimeArtifactOutputKind::RunTrace && traceOnly[0].path == tracePath, "runtime artifact output plan should preserve trace path");
	Expect(bundleOnly.size() == 1, "runtime artifact output plan should include configured bundle output");
	Expect(bundleOnly.size() == 1 && bundleOnly[0].kind == dev::RuntimeArtifactOutputKind::DebugBundle && bundleOnly[0].path == bundlePath, "runtime artifact output plan should preserve bundle path");
	Expect(both.size() == 2, "runtime artifact output plan should include both configured outputs");
	Expect(both.size() == 2 && both[0].kind == dev::RuntimeArtifactOutputKind::RunTrace && both[1].kind == dev::RuntimeArtifactOutputKind::DebugBundle, "runtime artifact output plan should save standalone trace before debug bundle");
}

void TestRuntimeArtifactOutputRequestRunnerRunsTraceAndBundleRequests()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_artifact_output_request_runner_test";
	const std::filesystem::path tracePath = root / "run.trace";
	const std::filesystem::path bundlePath = root / "bundle";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::GameLoopResult result;
	result.summary.framesRun = 2;
	result.finalMode = dev::GameSessionMode::Gameplay;
	dev::RuntimeOutputResultBuilder output;
	dev::RuntimeArtifactOutputRequestRunner runner;

	runner.run(
	    {
	        .kind = dev::RuntimeArtifactOutputKind::RunTrace,
	        .path = tracePath,
	    },
	    result,
	    output);
	runner.run(
	    {
	        .kind = dev::RuntimeArtifactOutputKind::DebugBundle,
	        .path = bundlePath,
	    },
	    result,
	    output);

	std::optional<std::vector<std::string>> trace = dev::RuntimeFrameTraceFileStore {}.load(tracePath);
	std::optional<std::vector<std::string>> bundleManifest = dev::RuntimeFrameTraceFileStore {}.load(bundlePath / "manifest.txt");
	std::optional<std::vector<std::string>> bundleTrace = dev::RuntimeFrameTraceFileStore {}.load(bundlePath / "run.trace");
	const dev::RuntimeOutputResult outputResult = output.result();

	Expect(outputResult.runTraceSaveAttempted && outputResult.runTraceSaved, "runtime artifact output request runner should update trace output flags");
	Expect(outputResult.debugBundleSaveAttempted && outputResult.debugBundleSaved, "runtime artifact output request runner should update bundle output flags");
	Expect(trace.has_value() && !trace->empty(), "runtime artifact output request runner should save standalone trace");
	Expect(bundleManifest.has_value(), "runtime artifact output request runner should save bundle manifest");
	Expect(bundleTrace.has_value() && !bundleTrace->empty(), "runtime artifact output request runner should save bundle trace");

	std::filesystem::remove_all(root);
}

} // namespace

int main()
{
	TestRuntimeOutputSettingsDefaultDisablesArtifacts();
	TestRuntimeOutputResultDefaultsToNoAttempts();
	TestRuntimeOutputResultBuilderRecordsArtifactAttempts();
	TestRuntimeRunTraceOutputStepSavesTraceAndUpdatesOutput();
	TestRuntimeDebugBundleOutputStepSavesBundleAndUpdatesOutput();
	TestRuntimeArtifactOutputPlanBuildsOrderedRequests();
	TestRuntimeArtifactOutputRequestRunnerRunsTraceAndBundleRequests();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
