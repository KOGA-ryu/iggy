#include "runtime/RuntimeGameplayTomlScenarioPackageFacade.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#ifndef IGGY_TEST_PACKAGE_FIXTURE_DIR
#error "IGGY_TEST_PACKAGE_FIXTURE_DIR must point at engine/tests/fixtures/runtime/ascii_source_plan_packages"
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

std::filesystem::path FixturePath(const char *name)
{
	return std::filesystem::path(IGGY_TEST_PACKAGE_FIXTURE_DIR) / name;
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
