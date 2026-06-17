#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "runtime/RuntimeGameplayAuthoringDiagnostics.hpp"
#include "runtime/RuntimeGameplayTomlScenarioFacade.hpp"
#include "runtime/RuntimeGameplayTomlScenarioPackageFacade.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayAuthoringPreviewInputKind {
	TomlFile,
	Package,
};

enum class RuntimeGameplayAuthoringPreviewStatus {
	PackageReadFailed,
	PackageInvalid,
	ReadFailed,
	ConversionFailed,
	LintFailed,
	LintOk,
	RunFailed,
	Ran,
	CheckFailed,
	CheckPassed,
};

struct RuntimeGameplayAuthoringPreviewPackageMetadata {
	bool present = false;
	std::filesystem::path packagePath;
	std::filesystem::path packageRoot;
	std::filesystem::path manifestPath;
	std::filesystem::path mainScenarioPath;
	std::string title;
	std::string description;
	std::string authoringVersion;
	std::filesystem::path main;
};

struct RuntimeGameplayAuthoringPreviewModel {
	std::filesystem::path inputPath;
	std::filesystem::path sourcePath;
	RuntimeGameplayAuthoringPreviewInputKind inputKind =
		RuntimeGameplayAuthoringPreviewInputKind::TomlFile;
	RuntimeGameplayTomlScenarioFacadeConfig config;
	RuntimeGameplayAuthoringPreviewStatus status =
		RuntimeGameplayAuthoringPreviewStatus::ReadFailed;
	RuntimeGameplayTomlScenarioFacadeStatus scenarioStatus =
		RuntimeGameplayTomlScenarioFacadeStatus::ReadFailed;
	RuntimeGameplayTomlScenarioPackageFacadeStatus packageStatus =
		RuntimeGameplayTomlScenarioPackageFacadeStatus::PackageReadFailed;
	RuntimeGameplayAuthoringPreviewPackageMetadata package;
	RuntimeGameplayTomlScenarioRunSummaryProjection summary;
	std::vector<std::string> finalRows;
	std::vector<RuntimeGameplayTomlScenarioTraceFrame> traceFrames;
	RuntimeGameplayTomlScenarioExpectationComparison expectation;
	std::vector<RuntimeGameplayAuthoringDiagnosticEntry> diagnostics;
	std::vector<RuntimeGameplayTomlScenarioPackageIssue> packageIssues;

	[[nodiscard]] bool ok() const;
};

class RuntimeGameplayAuthoringPreviewModelBuilder {
public:
	[[nodiscard]] RuntimeGameplayAuthoringPreviewModel build(
		const std::filesystem::path &path,
		const RuntimeGameplayTomlScenarioFacadeConfig &config = {}) const;
};

} // namespace iggy::runtime
