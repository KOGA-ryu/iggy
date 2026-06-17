#include "runtime/RuntimeGameplayAuthoringPreviewModel.hpp"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "support/AuthoringTestSupport.hpp"

#ifndef IGGY_TEST_FIXTURE_DIR
#error "IGGY_TEST_FIXTURE_DIR must point at engine/tests/fixtures/runtime/ascii_source_plan"
#endif

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

std::filesystem::path SourceFixturePath(const char *name)
{
	return iggy::test::AuthoringSourceFixturePath(
		IGGY_TEST_FIXTURE_DIR,
		name);
}

std::filesystem::path PackageFixturePath(const char *name)
{
	return iggy::test::AuthoringPackageFixturePath(
		IGGY_TEST_PACKAGE_FIXTURE_DIR,
		name);
}

std::filesystem::path TempRoot()
{
	return std::filesystem::current_path() /
		"runtime_gameplay_authoring_preview_model_tests_tmp";
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

iggy::runtime::RuntimeGameplayAuthoringPreviewModel BuildPreview(
	const std::filesystem::path &path,
	iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode mode =
		iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Run)
{
	iggy::runtime::RuntimeGameplayTomlScenarioFacadeConfig config;
	config.mode = mode;
	config.captureTraceFrames =
		mode == iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Trace;
	return iggy::runtime::RuntimeGameplayAuthoringPreviewModelBuilder {}.build(
		path,
		config);
}

void TestTomlFileRunPreview()
{
	const std::filesystem::path fixture =
		SourceFixturePath("player_picks_up_item_room.toml");
	const iggy::runtime::RuntimeGameplayAuthoringPreviewModel preview =
		BuildPreview(fixture);

	Expect(preview.inputKind ==
		iggy::runtime::RuntimeGameplayAuthoringPreviewInputKind::TomlFile,
		"TOML fixture preview should record file input kind");
	Expect(preview.status ==
		iggy::runtime::RuntimeGameplayAuthoringPreviewStatus::Ran,
		"TOML fixture preview should run");
	Expect(preview.ok(), "TOML fixture preview should be ok");
	Expect(!preview.package.present,
		"TOML fixture preview should not include package metadata");
	Expect(preview.inputPath == fixture,
		"TOML fixture preview should preserve input path");
	Expect(preview.sourcePath == fixture,
		"TOML fixture preview should preserve source path");
	Expect(preview.summary.frameCount == 2,
		"TOML fixture preview should copy frame count");
	Expect(preview.summary.acceptedCommandCount == 2,
		"TOML fixture preview should copy accepted command count");
	Expect(preview.summary.pickedUpCount == 1,
		"TOML fixture preview should copy pickup count");
	Expect(preview.finalRows ==
		std::vector<std::string> {
			"#######",
			"#A.@..#",
			"#.....#",
			"#######",
		},
		"TOML fixture preview should copy final rows");
}

void TestPackageRunPreview()
{
	const std::filesystem::path package =
		PackageFixturePath("player_picks_up_item_package");
	const iggy::runtime::RuntimeGameplayAuthoringPreviewModel preview =
		BuildPreview(package);

	Expect(preview.inputKind ==
		iggy::runtime::RuntimeGameplayAuthoringPreviewInputKind::Package,
		"package preview should record package input kind");
	Expect(preview.status ==
		iggy::runtime::RuntimeGameplayAuthoringPreviewStatus::Ran,
		"package preview should run");
	Expect(preview.ok(), "package preview should be ok");
	Expect(preview.package.present,
		"package preview should include package metadata");
	Expect(preview.package.title == "Player Picks Up Item Package",
		"package preview should copy package title");
	Expect(preview.package.description ==
		"Package wrapper for the canonical player pickup scenario.",
		"package preview should copy package description");
	Expect(preview.package.authoringVersion == "iggy:ascii-source-plan@1",
		"package preview should copy authoring version");
	Expect(preview.package.main == std::filesystem::path("scenario.toml"),
		"package preview should copy package main path");
	Expect(preview.sourcePath == package / "scenario.toml",
		"package preview should expose delegated source path");
	Expect(preview.summary.frameCount == 2,
		"package preview should copy delegated frame count");
	Expect(preview.summary.pickedUpCount == 1,
		"package preview should copy delegated pickup count");
	Expect(preview.finalRows ==
		std::vector<std::string> {
			"#######",
			"#A.@..#",
			"#.....#",
			"#######",
		},
		"package preview should copy delegated final rows");
}

void TestPackageTracePreview()
{
	const iggy::runtime::RuntimeGameplayAuthoringPreviewModel preview =
		BuildPreview(
			PackageFixturePath("player_picks_up_item_package"),
			iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Trace);

	Expect(preview.status ==
		iggy::runtime::RuntimeGameplayAuthoringPreviewStatus::Ran,
		"package trace preview should run");
	Expect(preview.traceFrames.size() == 2,
		"package trace preview should copy trace frame count");
	if (preview.traceFrames.size() == 2) {
		Expect(preview.traceFrames[0].frameId == "frame:move-to-key",
			"package trace preview should copy first frame id");
		Expect(preview.traceFrames[1].frameId == "frame:pickup-key",
			"package trace preview should copy second frame id");
		Expect(preview.traceFrames[1].pickedUpCount == 1,
			"package trace preview should copy frame pickup count");
		Expect(preview.traceFrames[1].rows ==
			std::vector<std::string> {
				"#######",
				"#A.@..#",
				"#.....#",
				"#######",
			},
			"package trace preview should copy frame rows");
	}
}

void TestCheckPreviewCopiesExpectationComparison()
{
	const iggy::runtime::RuntimeGameplayAuthoringPreviewModel preview =
		BuildPreview(
			SourceFixturePath("locked_door_key_room.toml"),
			iggy::runtime::RuntimeGameplayTomlScenarioFacadeMode::Check);

	Expect(preview.status ==
		iggy::runtime::RuntimeGameplayAuthoringPreviewStatus::CheckPassed,
		"check preview should pass for fixture with expectations");
	Expect(preview.ok(), "check preview should be ok");
	Expect(preview.expectation.present,
		"check preview should copy expectation presence");
	Expect(preview.expectation.matched,
		"check preview should copy expectation result");
	Expect(preview.expectation.checkedFinalRows,
		"check preview should copy final rows expectation flag");
	Expect(preview.expectation.checkedPickedUpCount,
		"check preview should copy pickup expectation flag");
}

void TestPackageManifestFailurePreview()
{
	const std::filesystem::path missing = TempRoot() / "missing_manifest";
	std::filesystem::create_directories(missing);
	const iggy::runtime::RuntimeGameplayAuthoringPreviewModel preview =
		BuildPreview(missing);

	Expect(preview.inputKind ==
		iggy::runtime::RuntimeGameplayAuthoringPreviewInputKind::Package,
		"missing manifest preview should use package facade");
	Expect(preview.status ==
		iggy::runtime::RuntimeGameplayAuthoringPreviewStatus::PackageReadFailed,
		"missing manifest preview should report package read failure");
	Expect(preview.package.present,
		"missing manifest preview should expose package projection");
	Expect(!preview.packageIssues.empty(),
		"missing manifest preview should copy package issues");
	Expect(preview.diagnostics.empty(),
		"missing manifest preview should not invent scenario diagnostics");
}

void TestPackageDelegatedFailurePreview()
{
	const iggy::runtime::RuntimeGameplayAuthoringPreviewModel preview =
		BuildPreview(PackageFixturePath("bad_pickup_target_package"));

	Expect(preview.inputKind ==
		iggy::runtime::RuntimeGameplayAuthoringPreviewInputKind::Package,
		"negative package preview should record package input kind");
	Expect(preview.status ==
		iggy::runtime::RuntimeGameplayAuthoringPreviewStatus::ReadFailed,
		"negative package preview should copy delegated read failure");
	Expect(preview.package.present,
		"negative package preview should still expose package metadata");
	Expect(preview.packageIssues.empty(),
		"negative package preview should not invent package issues");
	Expect(!preview.diagnostics.empty(),
		"negative package preview should copy delegated diagnostics");
}

} // namespace

int main()
{
	ResetTempRoot();

	TestTomlFileRunPreview();
	TestPackageRunPreview();
	TestPackageTracePreview();
	TestCheckPreviewCopiesExpectationComparison();
	TestPackageManifestFailurePreview();
	TestPackageDelegatedFailurePreview();

	CleanupTempRoot();

	if (Failures != 0)
		return 1;
	return 0;
}
