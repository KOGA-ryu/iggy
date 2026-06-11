#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "app/RuntimeArtifactOutputPlan.hpp"
#include "app/RuntimeArtifactOutputRequestRunner.hpp"
#include "app/RuntimeFrameTraceFileStore.hpp"
#include "app/RuntimeLoopTypes.hpp"
#include "app/RuntimeOutputResultBuilder.hpp"
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
	TestRuntimeArtifactOutputPlanBuildsOrderedRequests();
	TestRuntimeArtifactOutputRequestRunnerRunsTraceAndBundleRequests();
	return Failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
