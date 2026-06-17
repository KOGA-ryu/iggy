#include "runtime/RuntimeGameplayTomlScenarioPackageFacade.hpp"
#include "runtime/RuntimeGameplayTomlScenarioFacade.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "support/AuthoringTestSupport.hpp"

#ifndef IGGY_TEST_PACKAGE_FIXTURE_DIR
#error "IGGY_TEST_PACKAGE_FIXTURE_DIR must point at engine/tests/fixtures/runtime/ascii_source_plan_packages"
#endif

#ifndef IGGY_TEST_FIXTURE_DIR
#error "IGGY_TEST_FIXTURE_DIR must point at engine/tests/fixtures/runtime/ascii_source_plan"
#endif

namespace {

int Failures = 0;

void Expect(bool condition, const char *message)
{
	if (!condition) {
		std::cerr << "FAIL: " << message << '\n';
		++Failures;
	}
}

void Expect(bool condition, const std::string &message)
{
	Expect(condition, message.c_str());
}

std::filesystem::path FixturePath(const char *name)
{
	return iggy::test::AuthoringPackageFixturePath(
		IGGY_TEST_PACKAGE_FIXTURE_DIR,
		name);
}

std::filesystem::path SourceFixturePath(const char *name)
{
	return iggy::test::AuthoringSourceFixturePath(
		IGGY_TEST_FIXTURE_DIR,
		name);
}

std::filesystem::path TempRoot()
{
	return std::filesystem::current_path() /
		"runtime_gameplay_toml_scenario_package_facade_tests_tmp";
}

std::filesystem::path TempPackagePath(const char *name)
{
	return TempRoot() / name;
}

void ResetTempRoot()
{
	std::error_code ignored;
	std::filesystem::remove_all(TempRoot(), ignored);
	std::filesystem::create_directories(TempRoot(), ignored);
}

void CleanupTempRoot()
{
	std::error_code ignored;
	std::filesystem::remove_all(TempRoot(), ignored);
}

void WriteText(const std::filesystem::path &path, const std::string &text)
{
	std::filesystem::create_directories(path.parent_path());
	std::ofstream stream(path, std::ios::binary | std::ios::trunc);
	stream << text;
}

std::string ValidManifestText(const char *main = "scenario.toml")
{
	return std::string(R"toml(format_id = "iggy:authored-scenario-package"
version = 1
title = "Test Package"
description = "Temporary test package."
authoring_version = "iggy:ascii-source-plan@1"
main = ")toml") + main + R"toml("
)toml";
}

bool HasIssue(
	const iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeResult &result,
	iggy::runtime::RuntimeGameplayTomlScenarioPackageIssueCode code)
{
	for (const iggy::runtime::RuntimeGameplayTomlScenarioPackageIssue &issue :
		result.issues) {
		if (issue.code == code)
			return true;
	}
	return false;
}

iggy::runtime::RuntimeGameplayTomlScenarioFacadeResult ExecuteSourceFixture(
	const char *name,
	iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode mode)
{
	iggy::runtime::RuntimeGameplayTomlScenarioFacadeConfig config;
	config.mode = mode;
	config.captureTraceFrames =
		mode == iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Trace;
	return iggy::runtime::RuntimeGameplayTomlScenarioFacade {}.execute(
		SourceFixturePath(name),
		config);
}

iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeResult ExecutePackageFixture(
	const char *name,
	iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode mode)
{
	iggy::runtime::RuntimeGameplayTomlScenarioFacadeConfig config;
	config.mode = mode;
	config.captureTraceFrames =
		mode == iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Trace;
	return iggy::runtime::RuntimeGameplayTomlScenarioPackageFacade {}.execute(
		FixturePath(name),
		config);
}

void ExpectRunSummaryParity(
	const iggy::runtime::RuntimeGameplayTomlScenarioFacadeResult &source,
	const iggy::runtime::RuntimeGameplayTomlScenarioFacadeResult &package,
	const char *context)
{
	Expect(package.runSummary.frameCount == source.runSummary.frameCount,
		std::string(context) + " should match frame count");
	Expect(package.runSummary.acceptedCommandCount ==
		source.runSummary.acceptedCommandCount,
		std::string(context) + " should match accepted command count");
	Expect(package.runSummary.pickedUpCount == source.runSummary.pickedUpCount,
		std::string(context) + " should match pickup count");
	Expect(package.runSummary.interactionChanged ==
		source.runSummary.interactionChanged,
		std::string(context) + " should match interaction changed flag");
	Expect(package.runSummary.npcMovedCount == source.runSummary.npcMovedCount,
		std::string(context) + " should match NPC moved count");
	Expect(package.runSummary.npcBlockedMovementCount ==
		source.runSummary.npcBlockedMovementCount,
		std::string(context) + " should match blocked NPC movement count");
	Expect(package.runSummary.finalRows == source.runSummary.finalRows,
		std::string(context) + " should match final rows");
}

void ExpectExpectationParity(
	const iggy::runtime::RuntimeGameplayTomlScenarioFacadeResult &source,
	const iggy::runtime::RuntimeGameplayTomlScenarioFacadeResult &package,
	const char *context)
{
	Expect(package.expectationComparison.present ==
		source.expectationComparison.present,
		std::string(context) + " should match expectation presence");
	Expect(package.expectationComparison.matched ==
		source.expectationComparison.matched,
		std::string(context) + " should match expectation result");
}

void ExpectTraceParity(
	const iggy::runtime::RuntimeGameplayTomlScenarioFacadeResult &source,
	const iggy::runtime::RuntimeGameplayTomlScenarioFacadeResult &package,
	const char *context)
{
	Expect(package.traceFrames.size() == source.traceFrames.size(),
		std::string(context) + " should match trace frame count");
	if (package.traceFrames.size() != source.traceFrames.size())
		return;

	for (std::size_t index = 0; index < source.traceFrames.size(); ++index) {
		Expect(package.traceFrames[index].frameId ==
			source.traceFrames[index].frameId,
			std::string(context) + " should match trace frame id");
		Expect(package.traceFrames[index].acceptedCommandCount ==
			source.traceFrames[index].acceptedCommandCount,
			std::string(context) + " should match trace accepted command count");
		Expect(package.traceFrames[index].pickedUpCount ==
			source.traceFrames[index].pickedUpCount,
			std::string(context) + " should match trace pickup count");
		Expect(package.traceFrames[index].interactionChanged ==
			source.traceFrames[index].interactionChanged,
			std::string(context) + " should match trace interaction flag");
		Expect(package.traceFrames[index].npcMovedCount ==
			source.traceFrames[index].npcMovedCount,
			std::string(context) + " should match trace NPC moved count");
		Expect(package.traceFrames[index].rows == source.traceFrames[index].rows,
			std::string(context) + " should match trace rows");
	}
}

void TestPackageDirectoryRunsMainScenario()
{
	const std::filesystem::path package =
		FixturePath("moving_guard_room_package");
	const iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeResult result =
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacade {}.execute(
			package);

	Expect(result.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::Ran,
		"package directory should run");
	Expect(result.ok(), "package directory run should be ok");
	Expect(result.packageRoot == package,
		"package directory should preserve package root");
	Expect(result.manifestPath == package / "package.toml",
		"package directory should resolve manifest path");
	Expect(result.mainScenarioPath == package / "scenario.toml",
		"package directory should resolve main scenario path");
	Expect(result.manifest.formatId == "iggy:authored-scenario-package",
		"package manifest should preserve format id");
	Expect(result.manifest.version == 1,
		"package manifest should preserve version");
	Expect(result.manifest.title == "Moving Guard Room Package",
		"package manifest should preserve title");
	Expect(result.manifest.description ==
		"Package wrapper for the canonical moving guard room scenario.",
		"package manifest should preserve description");
	Expect(result.manifest.authoringVersion == "iggy:ascii-source-plan@1",
		"package manifest should preserve authoring version");
	Expect(result.manifest.main == std::filesystem::path("scenario.toml"),
		"package manifest should preserve main path");
	Expect(result.scenario.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::Ran,
		"package should delegate run to TOML scenario facade");
	Expect(result.scenario.runSummary.frameCount == 1,
		"package run should preserve frame count");
	Expect(result.scenario.runSummary.npcMovedCount == 1,
		"package run should preserve NPC movement count");
	Expect(result.scenario.runSummary.finalRows ==
		std::vector<std::string> {
			"#######",
			"#.A..@#",
			"#.....#",
			"#######",
		},
		"package run should preserve final rows");
}

void TestPackageManifestPathRunsMainScenario()
{
	const std::filesystem::path manifest =
		FixturePath("moving_guard_room_package/package.toml");
	const iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeResult result =
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacade {}.execute(
			manifest);

	Expect(result.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::Ran,
		"package manifest path should run");
	Expect(result.packageRoot == manifest.parent_path(),
		"package manifest path should preserve parent package root");
	Expect(result.mainScenarioPath == manifest.parent_path() / "scenario.toml",
		"package manifest path should resolve main scenario path");
}

void TestPickupPackageRunsMainScenario()
{
	const std::filesystem::path package =
		FixturePath("player_picks_up_item_package");
	const iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeResult result =
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacade {}.execute(
			package);

	Expect(result.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::Ran,
		"pickup package directory should run");
	Expect(result.manifest.title == "Player Picks Up Item Package",
		"pickup package manifest should preserve title");
	Expect(result.scenario.runSummary.frameCount == 2,
		"pickup package should preserve delegated frame count");
	Expect(result.scenario.runSummary.acceptedCommandCount == 2,
		"pickup package should preserve accepted command count");
	Expect(result.scenario.runSummary.pickedUpCount == 1,
		"pickup package should preserve pickup count");
	Expect(result.scenario.runSummary.finalRows ==
		std::vector<std::string> {
			"#######",
			"#A.@..#",
			"#.....#",
			"#######",
		},
		"pickup package should preserve final rows");
}

void TestNegativePackageDelegatesSourcePlanFailure()
{
	const std::filesystem::path package =
		FixturePath("bad_pickup_target_package");
	const iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeResult result =
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacade {}.execute(
			package);

	Expect(result.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::ScenarioReadFailed,
		"negative package should fail in delegated scenario read");
	Expect(result.issues.empty(),
		"negative package should not report manifest issues");
	Expect(result.scenario.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::ReadFailed,
		"negative package should preserve delegated read failure");
	Expect(result.scenario.read.text.status ==
		iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadStatus::SourcePlanInvalid,
		"negative package should preserve source-plan invalid status");
}

void TestPickupPackageRunCheckTraceParity()
{
	const char *sourceName = "player_picks_up_item_room.toml";
	const char *packageName = "player_picks_up_item_package";

	const iggy::runtime::RuntimeGameplayTomlScenarioFacadeResult sourceRun =
		ExecuteSourceFixture(
			sourceName,
			iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Run);
	const iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeResult packageRun =
		ExecutePackageFixture(
			packageName,
			iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Run);
	Expect(sourceRun.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::Ran,
		"source pickup run should succeed for parity");
	Expect(packageRun.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::Ran,
		"package pickup run should succeed for parity");
	ExpectRunSummaryParity(sourceRun, packageRun.scenario, "pickup run parity");
	ExpectExpectationParity(sourceRun, packageRun.scenario, "pickup run parity");

	const iggy::runtime::RuntimeGameplayTomlScenarioFacadeResult sourceCheck =
		ExecuteSourceFixture(
			sourceName,
			iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Check);
	const iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeResult packageCheck =
		ExecutePackageFixture(
			packageName,
			iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Check);
	Expect(packageCheck.scenario.status == sourceCheck.status,
		"package pickup check should match source check status");
	Expect(packageCheck.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::CheckFailed,
		"package pickup check should preserve missing-expectation check result");
	ExpectRunSummaryParity(sourceCheck, packageCheck.scenario, "pickup check parity");
	ExpectExpectationParity(
		sourceCheck,
		packageCheck.scenario,
		"pickup check parity");

	const iggy::runtime::RuntimeGameplayTomlScenarioFacadeResult sourceTrace =
		ExecuteSourceFixture(
			sourceName,
			iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Trace);
	const iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeResult packageTrace =
		ExecutePackageFixture(
			packageName,
			iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Trace);
	Expect(sourceTrace.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::Ran,
		"source pickup trace should run for parity");
	Expect(packageTrace.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::Ran,
		"package pickup trace should run for parity");
	ExpectRunSummaryParity(sourceTrace, packageTrace.scenario, "pickup trace parity");
	ExpectExpectationParity(
		sourceTrace,
		packageTrace.scenario,
		"pickup trace parity");
	ExpectTraceParity(sourceTrace, packageTrace.scenario, "pickup trace parity");
}

void TestNegativePackageDiagnosticsParity()
{
	const iggy::runtime::RuntimeGameplayTomlScenarioFacadeResult source =
		ExecuteSourceFixture(
			"bad_pickup_target_guard_room.toml",
			iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Run);
	const iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeResult package =
		ExecutePackageFixture(
			"bad_pickup_target_package",
			iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Run);

	Expect(source.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::ReadFailed,
		"source negative pickup should fail during read");
	Expect(package.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::ScenarioReadFailed,
		"package negative pickup should fail during delegated read");
	Expect(package.scenario.read.text.status == source.read.text.status,
		"negative package should match source TOML status");
	Expect(package.scenario.read.text.issues.size() == source.read.text.issues.size(),
		"negative package should match source TOML issue count");
	if (!source.read.text.issues.empty() &&
		!package.scenario.read.text.issues.empty()) {
		const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue
			&sourceIssue = source.read.text.issues.front();
		const iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlReadIssue
			&packageIssue = package.scenario.read.text.issues.front();
		Expect(packageIssue.code == sourceIssue.code,
			"negative package should match source TOML issue code");
		Expect(packageIssue.table == sourceIssue.table,
			"negative package should match source TOML issue table");
		Expect(packageIssue.key == sourceIssue.key,
			"negative package should match source TOML issue key");
		Expect(packageIssue.sourceIssue.code == sourceIssue.sourceIssue.code,
			"negative package should match source-plan issue code");
		Expect(packageIssue.sourceIssue.id == sourceIssue.sourceIssue.id,
			"negative package should match source-plan issue id");
	}
}

void TestPackageTraceModeDelegatesToScenarioFacade()
{
	iggy::runtime::RuntimeGameplayTomlScenarioFacadeConfig config;
	config.mode = iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Trace;
	const iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeResult result =
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacade {}.execute(
			FixturePath("moving_guard_room_package"),
			config);

	Expect(result.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::Ran,
		"package trace run should succeed");
	Expect(result.scenario.traceFrames.size() == 1,
		"package trace run should preserve delegated trace frames");
	if (result.scenario.traceFrames.size() == 1) {
		Expect(result.scenario.traceFrames[0].frameId == "frame:fixture",
			"package trace run should preserve delegated frame id");
	}
}

void TestMissingPackagePathFails()
{
	const iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeResult result =
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacade {}.execute(
			TempPackagePath("missing"));

	Expect(result.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::PackageReadFailed,
		"missing package path should fail during package read");
	Expect(HasIssue(
		result,
		iggy::runtime::RuntimeGameplayTomlScenarioPackageIssueCode::MissingPackagePath),
		"missing package path should report issue");
}

void TestMissingManifestFails()
{
	const std::filesystem::path package = TempPackagePath("missing_manifest");
	std::filesystem::create_directories(package);
	const iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeResult result =
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacade {}.execute(
			package);

	Expect(result.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::PackageReadFailed,
		"missing manifest should fail during package read");
	Expect(HasIssue(
		result,
		iggy::runtime::RuntimeGameplayTomlScenarioPackageIssueCode::MissingManifest),
		"missing manifest should report issue");
}

void TestUnsupportedVersionFails()
{
	const std::filesystem::path package = TempPackagePath("bad_version");
	WriteText(
		package / "package.toml",
		R"toml(format_id = "iggy:authored-scenario-package"
version = 2
title = "Bad Version"
description = "Temporary bad version package."
authoring_version = "iggy:ascii-source-plan@1"
main = "scenario.toml"
)toml");
	const iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeResult result =
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacade {}.execute(
			package);

	Expect(result.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::PackageInvalid,
		"unsupported package version should be invalid");
	Expect(HasIssue(
		result,
		iggy::runtime::RuntimeGameplayTomlScenarioPackageIssueCode::UnsupportedVersion),
		"unsupported package version should report issue");
}

void TestEscapingMainPathFails()
{
	const std::filesystem::path package = TempPackagePath("escaping_main");
	WriteText(
		package / "package.toml",
		R"toml(format_id = "iggy:authored-scenario-package"
version = 1
title = "Escaping Main"
description = "Temporary escaping main package."
authoring_version = "iggy:ascii-source-plan@1"
main = "../scenario.toml"
)toml");
	const iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeResult result =
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacade {}.execute(
			package);

	Expect(result.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::PackageInvalid,
		"escaping package main path should be invalid");
	Expect(HasIssue(
		result,
		iggy::runtime::RuntimeGameplayTomlScenarioPackageIssueCode::InvalidMainPath),
		"escaping package main path should report issue");
}

void TestMissingTitleFails()
{
	const std::filesystem::path package = TempPackagePath("missing_title");
	WriteText(
		package / "package.toml",
		R"toml(format_id = "iggy:authored-scenario-package"
version = 1
description = "Temporary missing title package."
authoring_version = "iggy:ascii-source-plan@1"
main = "scenario.toml"
)toml");
	const iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeResult result =
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacade {}.execute(
			package);

	Expect(result.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::PackageInvalid,
		"missing package title should be invalid");
	Expect(HasIssue(
		result,
		iggy::runtime::RuntimeGameplayTomlScenarioPackageIssueCode::MissingTitle),
		"missing package title should report issue");
}

void TestEmptyDescriptionFails()
{
	const std::filesystem::path package = TempPackagePath("empty_description");
	WriteText(
		package / "package.toml",
		R"toml(format_id = "iggy:authored-scenario-package"
version = 1
title = "Empty Description"
description = ""
authoring_version = "iggy:ascii-source-plan@1"
main = "scenario.toml"
)toml");
	const iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeResult result =
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacade {}.execute(
			package);

	Expect(result.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::PackageInvalid,
		"empty package description should be invalid");
	Expect(HasIssue(
		result,
		iggy::runtime::RuntimeGameplayTomlScenarioPackageIssueCode::InvalidMetadataValue),
		"empty package description should report invalid metadata issue");
}

void TestMissingAuthoringVersionFails()
{
	const std::filesystem::path package =
		TempPackagePath("missing_authoring_version");
	WriteText(
		package / "package.toml",
		R"toml(format_id = "iggy:authored-scenario-package"
version = 1
title = "Missing Authoring Version"
description = "Temporary missing authoring version package."
main = "scenario.toml"
)toml");
	const iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeResult result =
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacade {}.execute(
			package);

	Expect(result.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::PackageInvalid,
		"missing package authoring version should be invalid");
	Expect(HasIssue(
		result,
		iggy::runtime::RuntimeGameplayTomlScenarioPackageIssueCode::MissingAuthoringVersion),
		"missing package authoring version should report issue");
}

void TestMissingMainScenarioDelegatesReadFailure()
{
	const std::filesystem::path package = TempPackagePath("missing_main");
	WriteText(package / "package.toml", ValidManifestText());
	const iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeResult result =
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacade {}.execute(
			package);

	Expect(result.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioPackageFacadeStatus::ScenarioReadFailed,
		"missing package main scenario should delegate read failure");
	Expect(result.scenario.status ==
		iggy::runtime::RuntimeGameplayTomlScenarioFacadeStatus::ReadFailed,
		"missing package main scenario should preserve nested read failure");
}

} // namespace

int main()
{
	ResetTempRoot();

	TestPackageDirectoryRunsMainScenario();
	TestPackageManifestPathRunsMainScenario();
	TestPickupPackageRunsMainScenario();
	TestNegativePackageDelegatesSourcePlanFailure();
	TestPickupPackageRunCheckTraceParity();
	TestNegativePackageDiagnosticsParity();
	TestPackageTraceModeDelegatesToScenarioFacade();
	TestMissingPackagePathFails();
	TestMissingManifestFails();
	TestUnsupportedVersionFails();
	TestEscapingMainPathFails();
	TestMissingTitleFails();
	TestEmptyDescriptionFails();
	TestMissingAuthoringVersionFails();
	TestMissingMainScenarioDelegatesReadFailure();

	CleanupTempRoot();

	if (Failures != 0)
		return 1;
	return 0;
}
