#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "runtime/RuntimeGameplayAsciiSourcePlan.hpp"
#include "runtime/RuntimeGameplayAsciiSourcePlanTomlFileReader.hpp"
#include "runtime/RuntimeGameplayProfileScenarioDefinition.hpp"
#include "runtime/RuntimeGameplayProfileScenarioValidator.hpp"
#include "runtime/RuntimeGameplayScenarioAuthoringAdapter.hpp"
#include "runtime/RuntimeGameplayState.hpp"
#include "runtime/RuntimeGameplayTomlScenarioPackageReader.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayProductScenarioLoadStatus {
	Loaded,
	InputPathMissing,
	InputPathUnsupported,
	PackageReadFailed,
	PackageInvalid,
	SourceReadFailed,
	ConversionFailed,
	ValidationFailed,
};

enum class RuntimeGameplayProductScenarioLoadIssueCode {
	InputPathMissing,
	InputPathUnsupported,
	PackageIssue,
	TomlFileReadIssue,
	AuthoringConversionIssue,
	ProfileValidationIssue,
};

struct RuntimeGameplayProductScenarioLoadIssue {
	RuntimeGameplayProductScenarioLoadIssueCode code =
		RuntimeGameplayProductScenarioLoadIssueCode::InputPathMissing;
	std::filesystem::path path;
	std::string key;
	std::string detail;
};

struct RuntimeGameplayProductScenarioLoadResult {
	std::filesystem::path inputPath;
	std::filesystem::path sourcePath;
	bool hasPackage = false;
	std::filesystem::path packagePath;
	std::filesystem::path packageRoot;
	std::filesystem::path manifestPath;
	std::filesystem::path mainScenarioPath;
	RuntimeGameplayTomlScenarioPackageManifest packageManifest;
	RuntimeGameplayTomlScenarioPackageReadResult packageRead;
	RuntimeGameplayAsciiSourcePlanTomlFileReadResult sourceRead;
	RuntimeGameplayScenarioAuthoringAdapterResult adapter;
	RuntimeGameplayProfileScenarioValidationResult validation;
	RuntimeGameplayProfileScenarioDefinition definition;
	RuntimeGameplayState initialState;
	bool hasExpectations = false;
	RuntimeGameplayAsciiSourcePlanExpectations expectations;
	std::vector<RuntimeGameplayProductScenarioLoadIssue> issues;
	RuntimeGameplayProductScenarioLoadStatus status =
		RuntimeGameplayProductScenarioLoadStatus::InputPathMissing;

	[[nodiscard]] bool ok() const;
};

class RuntimeGameplayProductScenarioLoader {
public:
	[[nodiscard]] RuntimeGameplayProductScenarioLoadResult load(
		const std::filesystem::path &path) const;
};

} // namespace iggy::runtime
