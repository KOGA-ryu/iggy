#include "runtime/RuntimeGameplayProductScenarioLoader.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>

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

std::filesystem::path FixturePath(const char *name)
{
	return std::filesystem::path(IGGY_TEST_FIXTURE_DIR) / name;
}

std::filesystem::path PackageFixturePath(const char *name)
{
	return std::filesystem::path(IGGY_TEST_PACKAGE_FIXTURE_DIR) / name;
}

std::filesystem::path TempRoot()
{
	return std::filesystem::temp_directory_path() /
		"runtime_gameplay_product_scenario_loader_tests_tmp";
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
	std::ofstream stream(path);
	stream << text;
}

std::string ValidManifestText(const char *main = "scenario.toml")
{
	return std::string(R"toml(format_id = "iggy:authored-scenario-package"
version = 1
title = "Temporary Product Loader Package"
description = "Temporary product loader test package."
authoring_version = "iggy:ascii-source-plan@1"
main = ")toml") + main + R"toml("
)toml";
}

template <typename T, typename = void>
struct HasRunField : std::false_type {
};

template <typename T>
struct HasRunField<T, std::void_t<decltype(&T::run)>> : std::true_type {
};

template <typename T, typename = void>
struct HasFinalRowsField : std::false_type {
};

template <typename T>
struct HasFinalRowsField<T, std::void_t<decltype(&T::finalRows)>> :
	std::true_type {
};

template <typename T, typename = void>
struct HasTraceFramesField : std::false_type {
};

template <typename T>
struct HasTraceFramesField<T, std::void_t<decltype(&T::traceFrames)>> :
	std::true_type {
};

using ProductLoadResult =
	iggy::runtime::RuntimeGameplayProductScenarioLoadResult;

static_assert(!HasRunField<ProductLoadResult>::value);
static_assert(!HasFinalRowsField<ProductLoadResult>::value);
static_assert(!HasTraceFramesField<ProductLoadResult>::value);

bool HasIssue(
	const ProductLoadResult &result,
	iggy::runtime::RuntimeGameplayProductScenarioLoadIssueCode code)
{
	for (const iggy::runtime::RuntimeGameplayProductScenarioLoadIssue &issue :
		result.issues) {
		if (issue.code == code)
			return true;
	}
	return false;
}

ProductLoadResult Load(const std::filesystem::path &path)
{
	return iggy::runtime::RuntimeGameplayProductScenarioLoader {}.load(path);
}

void TestDirectTomlFixtureLoadsScenarioWithoutRunning()
{
	const std::filesystem::path fixture = FixturePath("mixed_mini_scenario.toml");
	const std::filesystem::path before = fixture;

	const ProductLoadResult result = Load(fixture);

	Expect(fixture == before, "loader should not mutate caller input path");
	Expect(result.status ==
			iggy::runtime::RuntimeGameplayProductScenarioLoadStatus::Loaded,
		"direct TOML fixture should load");
	Expect(result.ok(), "loaded direct TOML result should be ok");
	Expect(result.inputPath == fixture, "direct load should preserve input path");
	Expect(result.sourcePath == fixture, "direct load should preserve source path");
	Expect(!result.hasPackage, "direct TOML load should not report package");
	Expect(result.sourceRead.ok(), "direct load should include successful read");
	Expect(result.adapter.ok(), "direct load should include successful adapter result");
	Expect(result.validation.ok(),
		"direct load should include successful validation result");
	Expect(result.definition.frames.size() == 3,
		"direct load should preserve profile scenario frames");
	Expect(result.initialState.npcActors.actors.size() == 1,
		"direct load should copy initial gameplay state from definition");
	Expect(result.initialState.npcActors.actors.size() ==
			result.definition.initialState.npcActors.actors.size(),
		"initial state should match loaded definition initial state");
	Expect(result.adapter.packet.asciiSourcePlan.grid.rows ==
			result.sourceRead.text.plan.grid.rows,
		"loader should preserve parsed packet source-plan rows");
	Expect(result.issues.empty(), "loaded direct TOML should have no issues");
}

void TestPackageDirectoryLoadsMainScenarioMetadata()
{
	const std::filesystem::path package =
		PackageFixturePath("moving_guard_room_package");

	const ProductLoadResult result = Load(package);

	Expect(result.status ==
			iggy::runtime::RuntimeGameplayProductScenarioLoadStatus::Loaded,
		"package directory should load");
	Expect(result.ok(), "package directory result should be ok");
	Expect(result.hasPackage, "package directory should report package");
	Expect(result.packagePath == package,
		"package directory should preserve package path");
	Expect(result.packageRoot == package,
		"package directory should preserve package root");
	Expect(result.manifestPath == package / "package.toml",
		"package directory should resolve manifest path");
	Expect(result.mainScenarioPath == package / "scenario.toml",
		"package directory should resolve main scenario path");
	Expect(result.sourcePath == result.mainScenarioPath,
		"package directory should load resolved main source");
	Expect(result.packageManifest.title == "Moving Guard Room Package",
		"package directory should preserve manifest title");
	Expect(result.packageManifest.description ==
			"Package wrapper for the canonical moving guard room scenario.",
		"package directory should preserve manifest description");
	Expect(result.packageManifest.authoringVersion ==
			"iggy:ascii-source-plan@1",
		"package directory should preserve manifest authoring version");
	Expect(result.packageManifest.main == std::filesystem::path("scenario.toml"),
		"package directory should preserve manifest main path");
	Expect(result.sourceRead.ok(), "package load should read main source");
	Expect(result.adapter.ok(), "package load should convert main source");
	Expect(result.validation.ok(), "package load should validate definition");
	Expect(result.definition.frames.size() == 1,
		"package load should preserve frame count");
	Expect(result.issues.empty(), "loaded package should have no issues");
}

void TestPackageManifestPathLoadsSameMainScenario()
{
	const std::filesystem::path manifest =
		PackageFixturePath("moving_guard_room_package/package.toml");

	const ProductLoadResult result = Load(manifest);

	Expect(result.status ==
			iggy::runtime::RuntimeGameplayProductScenarioLoadStatus::Loaded,
		"package manifest path should load");
	Expect(result.hasPackage, "package manifest path should report package");
	Expect(result.packagePath == manifest,
		"package manifest path should preserve package path");
	Expect(result.packageRoot == manifest.parent_path(),
		"package manifest path should preserve package root");
	Expect(result.manifestPath == manifest,
		"package manifest path should preserve manifest path");
	Expect(result.mainScenarioPath == manifest.parent_path() / "scenario.toml",
		"package manifest path should resolve main scenario path");
	Expect(result.sourcePath == result.mainScenarioPath,
		"package manifest path should load resolved main source");
	Expect(result.packageManifest.title == "Moving Guard Room Package",
		"package manifest path should preserve manifest metadata");
}

void TestMissingInputPathFails()
{
	const ProductLoadResult result = Load(TempRoot() / "missing.toml");

	Expect(result.status ==
			iggy::runtime::RuntimeGameplayProductScenarioLoadStatus::
				InputPathMissing,
		"missing input path should fail as missing");
	Expect(!result.ok(), "missing input path should not be ok");
	Expect(HasIssue(
			   result,
			   iggy::runtime::RuntimeGameplayProductScenarioLoadIssueCode::
				   InputPathMissing),
		"missing input path should report input issue");
}

void TestUnsupportedInputPathFails()
{
	const std::filesystem::path path = TempRoot() / "unsupported.txt";
	WriteText(path, "not a scenario");

	const ProductLoadResult result = Load(path);

	Expect(result.status ==
			iggy::runtime::RuntimeGameplayProductScenarioLoadStatus::
				InputPathUnsupported,
		"unsupported existing path should fail as unsupported");
	Expect(HasIssue(
			   result,
			   iggy::runtime::RuntimeGameplayProductScenarioLoadIssueCode::
				   InputPathUnsupported),
		"unsupported existing path should report input issue");
}

void TestMissingPackageManifestFailsDuringPackageRead()
{
	const std::filesystem::path package = TempRoot() / "missing_manifest";
	std::filesystem::create_directories(package);

	const ProductLoadResult result = Load(package);

	Expect(result.status ==
			iggy::runtime::RuntimeGameplayProductScenarioLoadStatus::
				PackageReadFailed,
		"missing package manifest should fail during package read");
	Expect(result.hasPackage,
		"missing manifest package should preserve package fields");
	Expect(HasIssue(
			   result,
			   iggy::runtime::RuntimeGameplayProductScenarioLoadIssueCode::
				   PackageIssue),
		"missing package manifest should report package issue");
}

void TestInvalidPackageMainPathFailsAsPackageInvalid()
{
	const std::filesystem::path package = TempRoot() / "invalid_main";
	WriteText(package / "package.toml", ValidManifestText("../scenario.toml"));

	const ProductLoadResult result = Load(package);

	Expect(result.status ==
			iggy::runtime::RuntimeGameplayProductScenarioLoadStatus::
				PackageInvalid,
		"escaping package main path should be package invalid");
	Expect(HasIssue(
			   result,
			   iggy::runtime::RuntimeGameplayProductScenarioLoadIssueCode::
				   PackageIssue),
		"escaping package main path should report package issue");
}

void TestMissingPackageMainFailsAsSourceReadFailure()
{
	const std::filesystem::path package = TempRoot() / "missing_main";
	WriteText(package / "package.toml", ValidManifestText());

	const ProductLoadResult result = Load(package);

	Expect(result.status ==
			iggy::runtime::RuntimeGameplayProductScenarioLoadStatus::
				SourceReadFailed,
		"missing package main scenario should fail during source read");
	Expect(result.hasPackage,
		"missing package main scenario should still expose package metadata");
	Expect(result.sourcePath == package / "scenario.toml",
		"missing package main scenario should expose resolved source path");
	Expect(HasIssue(
			   result,
			   iggy::runtime::RuntimeGameplayProductScenarioLoadIssueCode::
				   TomlFileReadIssue),
		"missing package main scenario should report source read issue");
}

void TestTomlReadFailureMapsToSourceReadFailure()
{
	const ProductLoadResult result =
		Load(FixturePath("bad_pickup_target_guard_room.toml"));

	Expect(result.status ==
			iggy::runtime::RuntimeGameplayProductScenarioLoadStatus::
				SourceReadFailed,
		"semantic TOML source-plan failure should map to source read failure");
	Expect(result.sourceRead.status ==
			iggy::runtime::RuntimeGameplayAsciiSourcePlanTomlFileReadStatus::
				TomlReadFailed,
		"source read failure should preserve nested TOML read status");
	Expect(HasIssue(
			   result,
			   iggy::runtime::RuntimeGameplayProductScenarioLoadIssueCode::
				   TomlFileReadIssue),
		"source read failure should report TOML read issue");
}

void TestConversionFailurePreservesNestedAdapterResult()
{
	const ProductLoadResult result =
		Load(FixturePath("missing_profile_guard_room.toml"));

	Expect(result.status ==
			iggy::runtime::RuntimeGameplayProductScenarioLoadStatus::
				ConversionFailed,
		"missing actor profile should map to conversion failure");
	Expect(result.sourceRead.ok(),
		"conversion failure should preserve successful source read");
	Expect(result.adapter.status ==
			iggy::runtime::RuntimeGameplayScenarioAuthoringAdapterStatus::
				ConversionFailed,
		"conversion failure should preserve nested adapter status");
	Expect(HasIssue(
			   result,
			   iggy::runtime::RuntimeGameplayProductScenarioLoadIssueCode::
				   AuthoringConversionIssue),
		"conversion failure should report authoring issue");
}

} // namespace

int main()
{
	ResetTempRoot();

	TestDirectTomlFixtureLoadsScenarioWithoutRunning();
	TestPackageDirectoryLoadsMainScenarioMetadata();
	TestPackageManifestPathLoadsSameMainScenario();
	TestMissingInputPathFails();
	TestUnsupportedInputPathFails();
	TestMissingPackageManifestFailsDuringPackageRead();
	TestInvalidPackageMainPathFailsAsPackageInvalid();
	TestMissingPackageMainFailsAsSourceReadFailure();
	TestTomlReadFailureMapsToSourceReadFailure();
	TestConversionFailurePreservesNestedAdapterResult();

	CleanupTempRoot();

	if (Failures != 0)
		return 1;
	return 0;
}
