#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "app/GameLoop.hpp"
#include "app/RuntimeDebugArtifactBundle.hpp"
#include "app/RuntimeDebugArtifactBundleResultBuilder.hpp"
#include "app/RuntimeDebugArtifactLayout.hpp"
#include "app/RuntimeDebugArtifactRootPreparer.hpp"
#include "app/RuntimeDebugArtifactWriter.hpp"
#include "app/RuntimeDebugManifestContextBuilder.hpp"
#include "app/RuntimeDebugManifestWriteStep.hpp"
#include "app/RuntimeDebugTraceWriteStep.hpp"
#include "app/RuntimeFrameTraceFileStore.hpp"

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

void TestRuntimeDebugArtifactBundleSavesManifestAndTrace()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_debug_bundle_test";
	const std::filesystem::path bundleRoot = root / "bundle";
	std::filesystem::remove_all(root);

	dev::GameLoop loop {
		dev::GameLoopSettings {
		    .saveRoot = root / "saves",
		    .frame = { .maxFrames = 1 },
		}
	};
	loop.session().startNewGame({ .playerStart = { 0, 0 }, .playerHitPoints = 20 });
	dev::GameLoopResult run = loop.runForResult();

	dev::RuntimeDebugArtifactBundle bundle;
	dev::RuntimeDebugArtifactBundleResult result = bundle.save(bundleRoot, run);
	std::optional<std::vector<std::string>> manifest = dev::RuntimeFrameTraceFileStore {}.load(bundleRoot / "manifest.txt");
	std::optional<std::vector<std::string>> trace = dev::RuntimeFrameTraceFileStore {}.load(bundleRoot / "run.trace");

	Expect(result.rootPrepared, "runtime debug bundle should create bundle root");
	Expect(result.traceSaved, "runtime debug bundle should save run trace");
	Expect(result.manifestSaved, "runtime debug bundle should save manifest");
	Expect(result.saved(), "runtime debug bundle should report complete save");
	Expect(result.manifestPath == bundleRoot / "manifest.txt", "runtime debug bundle should use stable manifest path");
	Expect(result.tracePath == bundleRoot / "run.trace", "runtime debug bundle should use stable trace path");
	Expect(manifest.has_value(), "runtime debug bundle manifest should be loadable text");
	Expect(trace.has_value(), "runtime debug bundle trace should be loadable text");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "bundle version=1"), "runtime debug bundle manifest should include version");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "trace=run.trace saved=true"), "runtime debug bundle manifest should index trace artifact");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "run frames=1 frameReports=1"), "runtime debug bundle manifest should summarize run frame counts");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "finalMode=Gameplay"), "runtime debug bundle manifest should include final mode");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "policy latest=Gameplay acceptCommands=true updatePlayers=true updateEnemies=true"), "runtime debug bundle manifest should summarize latest frame policy");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "runtime inventoryScripts=0 completed=0 loadFailed=0 noActivePlayer=0 applied=0 rejected=0"), "runtime debug bundle manifest should summarize runtime inventory scripts");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "runtime movementScripts=0 completed=0 loadFailed=0 noActiveWorld=0 accepted=0 rejected=0"), "runtime debug bundle manifest should summarize runtime movement scripts");
	Expect(trace.has_value() && !trace->empty() && (*trace)[0] == "run frames=1 frameReports=1 rawInput=0 movementInputBlocks=0 sessionResults=0 inventoryScripts=0 inventoryResults=0 movementScripts=0 movementQueued=0", "runtime debug bundle trace should preserve run trace summary");

	std::filesystem::remove_all(root);
}

void TestRuntimeDebugArtifactLayoutNamesBundlePaths()
{
	const std::filesystem::path root = "debug/run-001";

	dev::RuntimeDebugArtifactPaths paths = dev::RuntimeDebugArtifactLayout {}.pathsForRoot(root);

	Expect(paths.rootPath == root, "runtime debug artifact layout should preserve root path");
	Expect(paths.manifestPath == root / "manifest.txt", "runtime debug artifact layout should name manifest path");
	Expect(paths.tracePath == root / "run.trace", "runtime debug artifact layout should name trace path");
}

void TestRuntimeDebugArtifactBundleResultBuilderRecordsBundleState()
{
	const dev::RuntimeDebugArtifactPaths paths {
		.rootPath = "debug/run-001",
		.manifestPath = "debug/run-001/manifest.txt",
		.tracePath = "debug/run-001/run.trace",
	};
	dev::RuntimeDebugArtifactBundleResultBuilder builder { paths };

	dev::RuntimeDebugArtifactBundleResult initial = builder.result();
	Expect(initial.rootPath == paths.rootPath, "runtime debug artifact bundle result builder should copy root path");
	Expect(initial.manifestPath == paths.manifestPath, "runtime debug artifact bundle result builder should copy manifest path");
	Expect(initial.tracePath == paths.tracePath, "runtime debug artifact bundle result builder should copy trace path");
	Expect(!initial.rootPrepared && !initial.traceSaved && !initial.manifestSaved, "runtime debug artifact bundle result builder should default to unsaved state");
	Expect(!initial.saved(), "runtime debug artifact bundle result builder should not report saved before writes");

	builder.markRootPrepared();
	builder.recordWrite({ .traceSaved = true, .manifestSaved = true });
	dev::RuntimeDebugArtifactBundleResult saved = builder.result();

	Expect(saved.rootPrepared, "runtime debug artifact bundle result builder should record prepared root");
	Expect(saved.traceSaved && saved.manifestSaved, "runtime debug artifact bundle result builder should record write flags");
	Expect(saved.saved(), "runtime debug artifact bundle result builder should report complete bundle save");
}

void TestRuntimeDebugArtifactRootPreparerCreatesBundleRoot()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_debug_root_preparer_test";
	std::filesystem::remove_all(root);

	bool prepared = dev::RuntimeDebugArtifactRootPreparer {}.prepare(root / "nested" / "bundle");

	Expect(prepared, "runtime debug artifact root preparer should create missing bundle directories");
	Expect(std::filesystem::is_directory(root / "nested" / "bundle"), "runtime debug artifact root preparer should leave a directory at bundle root");

	std::filesystem::remove_all(root);
}

void TestRuntimeDebugArtifactRootPreparerRejectsRootFile()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_debug_root_preparer_file_test";
	std::filesystem::remove_all(root);
	{
		std::ofstream output { root, std::ios::trunc };
		output << "not a directory\n";
	}

	bool prepared = dev::RuntimeDebugArtifactRootPreparer {}.prepare(root);

	Expect(!prepared, "runtime debug artifact root preparer should reject existing files");

	std::filesystem::remove(root);
}

void TestRuntimeDebugManifestContextBuilderMapsPathsAndTraceState()
{
	const dev::RuntimeDebugArtifactPaths paths {
		.rootPath = "debug/run-001",
		.manifestPath = "debug/run-001/manifest.txt",
		.tracePath = "debug/run-001/run.trace",
	};

	dev::RuntimeDebugManifestContext context = dev::RuntimeDebugManifestContextBuilder {}.build(paths, true);

	Expect(context.rootPath == paths.rootPath, "runtime debug manifest context builder should copy bundle root path");
	Expect(context.manifestPath == paths.manifestPath, "runtime debug manifest context builder should copy manifest path");
	Expect(context.tracePath == paths.tracePath, "runtime debug manifest context builder should copy trace path");
	Expect(context.traceSaved, "runtime debug manifest context builder should copy trace save state");
}

void TestRuntimeDebugTraceWriteStepSavesBundleTrace()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_debug_trace_write_step_test";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::GameLoopResult run;
	run.summary.framesRun = 4;

	dev::RuntimeDebugArtifactPaths paths = dev::RuntimeDebugArtifactLayout {}.pathsForRoot(root);
	const bool saved = dev::RuntimeDebugTraceWriteStep {}.write(paths, run);
	std::optional<std::vector<std::string>> trace = dev::RuntimeFrameTraceFileStore {}.load(paths.tracePath);

	Expect(saved, "runtime debug trace write step should save run trace");
	Expect(trace.has_value(), "runtime debug trace write step should write readable trace");
	Expect(trace.has_value() && !trace->empty() && (*trace)[0] == "run frames=4 frameReports=0 rawInput=0 movementInputBlocks=0 sessionResults=0 inventoryScripts=0 inventoryResults=0 movementScripts=0 movementQueued=0", "runtime debug trace write step should preserve run summary");

	std::filesystem::remove_all(root);
}

void TestRuntimeDebugManifestWriteStepSavesManifestWithTraceState()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_debug_manifest_write_step_test";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::GameLoopResult run;
	run.finalMode = dev::GameSessionMode::Gameplay;
	run.summary.framesRun = 1;

	dev::RuntimeDebugArtifactPaths paths = dev::RuntimeDebugArtifactLayout {}.pathsForRoot(root);
	const bool saved = dev::RuntimeDebugManifestWriteStep {}.write(paths, run, false);
	std::optional<std::vector<std::string>> manifest = dev::RuntimeFrameTraceFileStore {}.load(paths.manifestPath);

	Expect(saved, "runtime debug manifest write step should save manifest lines");
	Expect(manifest.has_value(), "runtime debug manifest write step should write readable manifest");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "trace=run.trace saved=false"), "runtime debug manifest write step should record trace save state");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "root="), "runtime debug manifest write step should include artifact paths");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "run frames=1"), "runtime debug manifest write step should include run frame count");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "finalMode=Gameplay"), "runtime debug manifest write step should include final mode");

	std::filesystem::remove_all(root);
}

void TestRuntimeDebugArtifactWriterSavesTraceAndManifest()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_debug_writer_test";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::GameLoopResult run;
	run.finalMode = dev::GameSessionMode::Gameplay;
	run.summary.framesRun = 2;

	dev::RuntimeDebugArtifactPaths paths = dev::RuntimeDebugArtifactLayout {}.pathsForRoot(root);
	dev::RuntimeDebugArtifactWriteResult result = dev::RuntimeDebugArtifactWriter {}.write(paths, run);
	std::optional<std::vector<std::string>> manifest = dev::RuntimeFrameTraceFileStore {}.load(paths.manifestPath);
	std::optional<std::vector<std::string>> trace = dev::RuntimeFrameTraceFileStore {}.load(paths.tracePath);

	Expect(result.traceSaved, "runtime debug artifact writer should save trace");
	Expect(result.manifestSaved, "runtime debug artifact writer should save manifest");
	Expect(manifest.has_value(), "runtime debug artifact writer should write readable manifest");
	Expect(trace.has_value(), "runtime debug artifact writer should write readable trace");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "trace=run.trace saved=true"), "runtime debug artifact writer manifest should record saved trace");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "policy latest=none"), "runtime debug artifact writer manifest should report no frame policy without frame reports");
	Expect(trace.has_value() && !trace->empty() && (*trace)[0] == "run frames=2 frameReports=0 rawInput=0 movementInputBlocks=0 sessionResults=0 inventoryScripts=0 inventoryResults=0 movementScripts=0 movementQueued=0", "runtime debug artifact writer should preserve trace summary");

	std::filesystem::remove_all(root);
}

void TestRuntimeDebugArtifactWriterRecordsTraceFailureInManifest()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_debug_writer_trace_failure_test";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root);

	dev::RuntimeDebugArtifactPaths paths {
		.rootPath = root,
		.manifestPath = root / "manifest.txt",
		.tracePath = root / "missing-parent" / "run.trace",
	};

	dev::RuntimeDebugArtifactWriteResult result = dev::RuntimeDebugArtifactWriter {}.write(paths, dev::GameLoopResult {});
	std::optional<std::vector<std::string>> manifest = dev::RuntimeFrameTraceFileStore {}.load(paths.manifestPath);

	Expect(!result.traceSaved, "runtime debug artifact writer should report trace save failure");
	Expect(result.manifestSaved, "runtime debug artifact writer should still save manifest after trace failure");
	Expect(manifest.has_value() && ContainsLineFragment(*manifest, "trace=run.trace saved=false"), "runtime debug artifact writer manifest should record failed trace");

	std::filesystem::remove_all(root);
}

void TestRuntimeDebugArtifactBundleRejectsRootFile()
{
	const std::filesystem::path root = std::filesystem::temp_directory_path() / "iggy_runtime_debug_bundle_root_file_test";
	std::filesystem::remove_all(root);
	std::filesystem::create_directories(root.parent_path());
	{
		std::ofstream output { root, std::ios::trunc };
		output << "not a directory\n";
	}

	dev::RuntimeDebugArtifactBundleResult result = dev::RuntimeDebugArtifactBundle {}.save(root, dev::GameLoopResult {});

	Expect(!result.rootPrepared, "runtime debug bundle should reject a root path that is already a file");
	Expect(!result.traceSaved, "runtime debug bundle should not save trace when root cannot be prepared");
	Expect(!result.manifestSaved, "runtime debug bundle should not save manifest when root cannot be prepared");
	Expect(!result.saved(), "runtime debug bundle should report incomplete save on root failure");

	std::filesystem::remove(root);
}

} // namespace

int main()
{
	TestRuntimeDebugArtifactBundleSavesManifestAndTrace();
	TestRuntimeDebugArtifactLayoutNamesBundlePaths();
	TestRuntimeDebugArtifactBundleResultBuilderRecordsBundleState();
	TestRuntimeDebugArtifactRootPreparerCreatesBundleRoot();
	TestRuntimeDebugArtifactRootPreparerRejectsRootFile();
	TestRuntimeDebugManifestContextBuilderMapsPathsAndTraceState();
	TestRuntimeDebugTraceWriteStepSavesBundleTrace();
	TestRuntimeDebugManifestWriteStepSavesManifestWithTraceState();
	TestRuntimeDebugArtifactWriterSavesTraceAndManifest();
	TestRuntimeDebugArtifactWriterRecordsTraceFailureInManifest();
	TestRuntimeDebugArtifactBundleRejectsRootFile();

	if (Failures != 0) {
		std::cerr << Failures << " test(s) failed\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
