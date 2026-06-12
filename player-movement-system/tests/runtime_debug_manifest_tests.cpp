#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "app/RuntimeDebugManifest.hpp"
#include "app/RuntimeDebugManifestIndexText.hpp"
#include "app/RuntimeDebugManifestPathsText.hpp"
#include "app/RuntimeDebugManifestSections.hpp"
#include "app/RuntimeDebugManifestSetupText.hpp"

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

void TestRuntimeDebugManifestSetupTextFormatsSetupAttempts()
{
	dev::RuntimeSetupResult setup;
	setup.startupScriptRan = true;
	setup.inventoryScriptRan = false;
	setup.movementScriptRan = true;

	dev::RuntimeDebugManifestSetupText formatter;

	Expect(formatter.format(setup) == "setup startupScriptRan=true inventoryScriptRan=false movementScriptRan=true", "runtime debug manifest setup text should format setup attempt flags");
}

void TestRuntimeDebugManifestIndexTextFormatsBundleIndex()
{
	dev::RuntimeDebugManifestContext savedContext {
		.traceSaved = true,
	};
	dev::RuntimeDebugManifestContext failedContext {
		.traceSaved = false,
	};

	const std::vector<std::string> saved = dev::RuntimeDebugManifestIndexText {}.format(savedContext);
	const std::vector<std::string> failed = dev::RuntimeDebugManifestIndexText {}.format(failedContext);
	const std::vector<std::string> expectedSaved {
		"bundle version=1",
		"trace=run.trace saved=true",
	};
	const std::vector<std::string> expectedFailed {
		"bundle version=1",
		"trace=run.trace saved=false",
	};

	Expect(saved == expectedSaved, "runtime debug manifest index text should format saved trace index lines");
	Expect(failed == expectedFailed, "runtime debug manifest index text should format failed trace index lines");
}

void TestRuntimeDebugManifestPathsTextFormatsArtifactPaths()
{
	dev::RuntimeDebugManifestContext context {
		.rootPath = "debug/run-001",
		.manifestPath = "debug/run-001/manifest.txt",
		.tracePath = "debug/run-001/run.trace",
	};

	const std::vector<std::string> lines = dev::RuntimeDebugManifestPathsText {}.format(context);
	const std::vector<std::string> expected {
		"paths root=debug/run-001",
		"paths manifest=manifest.txt",
		"paths trace=run.trace",
	};

	Expect(lines == expected, "runtime debug manifest paths text should format artifact path lines");
}

void TestRuntimeDebugManifestSectionsFormatsRunStatus()
{
	dev::GameLoopResult result;
	result.summary.framesRun = 2;
	result.summary.movementInputBlockReasons.push_back(dev::PlayerActionBlockReason::Focus);
	result.summary.movementInputBlockReasons.push_back(dev::PlayerActionBlockReason::Paused);
	result.finalMode = dev::GameSessionMode::Inventory;

	const std::vector<std::string> lines = dev::RuntimeDebugManifestSections {}.formatRunStatus(result);
	const std::vector<std::string> expected {
		"run frames=2 frameReports=0 rawInput=0 movementInputBlocks=2 sessionResults=0 inventoryScripts=0 inventoryResults=0 movementScripts=0 movementQueued=0 finalMode=Inventory",
		"movementInputBlockReasons total=2 focus=1 paused=1 animationLocked=0 animationCommitment=0 stunned=0 none=0",
		"policy latest=none",
	};

	Expect(lines == expected, "runtime debug manifest sections should format run status lines in manifest order");
}

void TestRuntimeDebugManifestSectionsFormatsSetupDetails()
{
	dev::GameLoopResult result;
	result.setup.inventoryScriptRan = true;
	result.setup.inventoryScriptResult = {
		.status = dev::InventoryScriptRunStatus::Completed,
		.commandResults = {
		    { .type = dev::InventoryCommandResultType::Applied },
		},
	};
	result.setup.movementScriptRan = true;
	result.setup.movementScriptResult = {
		.status = dev::MovementScriptRunStatus::Completed,
		.replayReport = {
		    .results = {
		        { .type = dev::MovementCommandDispatchResultType::Accepted },
		    },
		},
	};

	const std::vector<std::string> lines = dev::RuntimeDebugManifestSections {}.formatSetup(result);
	const std::vector<std::string> expected {
		"setup startupScriptRan=false inventoryScriptRan=true movementScriptRan=true",
		"setup movementScript status=Completed results=1 accepted=1 rejected=0",
		"setup inventoryScript status=Completed results=1 applied=1 rejected=0",
	};

	Expect(lines == expected, "runtime debug manifest sections should format setup details in manifest order");
}

void TestRuntimeDebugManifestSectionsFormatsRuntimeScripts()
{
	dev::GameLoopResult result;
	result.summary.runtimeInventoryScriptResults.push_back({
	    .status = dev::InventoryScriptRunStatus::LoadFailed,
	});
	result.summary.runtimeMovementScriptResults.push_back({
	    .status = dev::MovementScriptRunStatus::NoActiveWorld,
	});

	const std::vector<std::string> lines = dev::RuntimeDebugManifestSections {}.formatRuntimeScripts(result);
	const std::vector<std::string> expected {
		"runtime inventoryScripts=1 completed=0 loadFailed=1 noActivePlayer=0 applied=0 rejected=0",
		"runtime movementScripts=1 completed=0 loadFailed=0 noActiveWorld=1 accepted=0 rejected=0",
	};

	Expect(lines == expected, "runtime debug manifest sections should format runtime script aggregate lines in manifest order");
}

void TestRuntimeDebugManifestFormatsFailedRun()
{
	dev::GameLoopResult run;
	run.setup.startupScriptRan = true;
	run.finalMode = dev::GameSessionMode::Empty;

	dev::RuntimeDebugManifestContext context {
		.rootPath = "debug/run-001",
		.manifestPath = "debug/run-001/manifest.txt",
		.tracePath = "debug/run-001/run.trace",
		.traceSaved = false,
	};

	std::vector<std::string> lines = dev::RuntimeDebugManifest {}.format(run, context);

	Expect(ContainsLineFragment(lines, "trace=run.trace saved=false"), "runtime debug bundle manifest should report trace save state");
	Expect(ContainsLineFragment(lines, "run frames=0 frameReports=0"), "runtime debug bundle manifest should summarize empty failed runs");
	Expect(ContainsLineFragment(lines, "finalMode=Empty"), "runtime debug bundle manifest should name empty final mode");
	Expect(ContainsLineFragment(lines, "policy latest=none"), "runtime debug bundle manifest should report no frame policy for zero-frame runs");
	Expect(ContainsLineFragment(lines, "setup startupScriptRan=true inventoryScriptRan=false movementScriptRan=false"), "runtime debug bundle manifest should report setup attempts");
	Expect(ContainsLineFragment(lines, "runtime inventoryScripts=0 completed=0 loadFailed=0 noActivePlayer=0 applied=0 rejected=0"), "runtime debug bundle manifest should report empty runtime inventory scripts");
	Expect(ContainsLineFragment(lines, "runtime movementScripts=0 completed=0 loadFailed=0 noActiveWorld=0 accepted=0 rejected=0"), "runtime debug bundle manifest should report empty runtime movement scripts");
}

void TestRuntimeDebugManifestSummarizesInventoryScripts()
{
	dev::GameLoopResult run;
	run.setup.inventoryScriptRan = true;
	run.setup.inventoryScriptResult = {
		.status = dev::InventoryScriptRunStatus::Completed,
		.commandResults = {
		    {
		        .type = dev::InventoryCommandResultType::Applied,
		        .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 10 },
		    },
		    {
		        .type = dev::InventoryCommandResultType::Rejected,
		        .command = { .type = dev::InventoryCommandType::UnequipSlot, .slot = dev::EquipmentSlot::Weapon },
		    },
		},
	};
	run.summary.runtimeInventoryScriptResults.push_back({
	    .status = dev::InventoryScriptRunStatus::Completed,
	    .commandResults = {
	        {
	            .type = dev::InventoryCommandResultType::Applied,
	            .command = { .type = dev::InventoryCommandType::EquipItem, .itemId = 11 },
	        },
	    },
	});
	run.summary.runtimeInventoryScriptResults.push_back({
	    .status = dev::InventoryScriptRunStatus::LoadFailed,
	});
	run.summary.runtimeInventoryScriptResults.push_back({
	    .status = dev::InventoryScriptRunStatus::NoActivePlayer,
	});
	run.finalMode = dev::GameSessionMode::Gameplay;

	dev::RuntimeDebugManifestContext context {
		.rootPath = "debug/run-inventory",
		.manifestPath = "debug/run-inventory/manifest.txt",
		.tracePath = "debug/run-inventory/run.trace",
		.traceSaved = true,
	};

	std::vector<std::string> lines = dev::RuntimeDebugManifest {}.format(run, context);

	Expect(ContainsLineFragment(lines, "setup startupScriptRan=false inventoryScriptRan=true movementScriptRan=false"), "runtime debug manifest should report configured inventory setup attempt");
	Expect(ContainsLineFragment(lines, "setup inventoryScript status=Completed results=2 applied=1 rejected=1"), "runtime debug manifest should summarize configured inventory setup result");
	Expect(ContainsLineFragment(lines, "runtime inventoryScripts=3 completed=1 loadFailed=1 noActivePlayer=1 applied=1 rejected=0"), "runtime debug manifest should summarize runtime inventory script results");
}

void TestRuntimeDebugManifestSummarizesMovementScripts()
{
	dev::GameLoopResult run;
	run.setup.movementScriptRan = true;
	run.setup.movementScriptResult = {
		.status = dev::MovementScriptRunStatus::Completed,
		.replayReport = {
		    .results = {
		        {
		            .type = dev::MovementCommandDispatchResultType::Accepted,
		            .command = {
		                .type = dev::MovementCommandType::WalkTo,
		                .playerId = 0,
		                .destination = { 1, 0 },
		            },
		        },
		        {
		            .type = dev::MovementCommandDispatchResultType::Rejected,
		            .command = {
		                .type = dev::MovementCommandType::MoveThenAct,
		                .playerId = 0,
		                .destination = { 1, 0 },
		                .destinationAction = std::nullopt,
		            },
		        },
		    },
		},
	};
	run.summary.runtimeMovementScriptResults.push_back({
	    .status = dev::MovementScriptRunStatus::Completed,
	    .replayReport = {
	        .results = {
	            {
	                .type = dev::MovementCommandDispatchResultType::Accepted,
	                .command = {
	                    .type = dev::MovementCommandType::WalkTo,
	                    .playerId = 0,
	                    .destination = { 1, 0 },
	                },
	            },
	        },
	    },
	});
	run.summary.runtimeMovementScriptResults.push_back({
	    .status = dev::MovementScriptRunStatus::LoadFailed,
	});
	run.summary.runtimeMovementScriptResults.push_back({
	    .status = dev::MovementScriptRunStatus::NoActiveWorld,
	});
	run.finalMode = dev::GameSessionMode::Gameplay;

	dev::RuntimeDebugManifestContext context {
		.rootPath = "debug/run-002",
		.manifestPath = "debug/run-002/manifest.txt",
		.tracePath = "debug/run-002/run.trace",
		.traceSaved = true,
	};

	std::vector<std::string> lines = dev::RuntimeDebugManifest {}.format(run, context);

	Expect(ContainsLineFragment(lines, "setup startupScriptRan=false inventoryScriptRan=false movementScriptRan=true"), "runtime debug manifest should report configured movement setup attempt");
	Expect(ContainsLineFragment(lines, "setup movementScript status=Completed results=2 accepted=1 rejected=1"), "runtime debug manifest should summarize configured movement setup replay result");
	Expect(ContainsLineFragment(lines, "runtime movementScripts=3 completed=1 loadFailed=1 noActiveWorld=1 accepted=1 rejected=0"), "runtime debug manifest should summarize runtime movement script results");
}

} // namespace

int main()
{
	TestRuntimeDebugManifestSetupTextFormatsSetupAttempts();
	TestRuntimeDebugManifestIndexTextFormatsBundleIndex();
	TestRuntimeDebugManifestPathsTextFormatsArtifactPaths();
	TestRuntimeDebugManifestSectionsFormatsRunStatus();
	TestRuntimeDebugManifestSectionsFormatsSetupDetails();
	TestRuntimeDebugManifestSectionsFormatsRuntimeScripts();
	TestRuntimeDebugManifestFormatsFailedRun();
	TestRuntimeDebugManifestSummarizesInventoryScripts();
	TestRuntimeDebugManifestSummarizesMovementScripts();

	if (Failures != 0) {
		std::cerr << Failures << " test(s) failed\n";
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
