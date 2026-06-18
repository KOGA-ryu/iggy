#pragma once

#include <filesystem>
#include <vector>

#include "runtime/RuntimeGameplayTomlScenarioFacade.hpp"
#include "runtime/RuntimeGameplayTomlScenarioPackageReader.hpp"

namespace iggy::runtime {

enum class RuntimeGameplayTomlScenarioPackageFacadeStatus {
	PackageReadFailed,
	PackageInvalid,
	ScenarioReadFailed,
	ConversionFailed,
	LintFailed,
	LintOk,
	RunFailed,
	Ran,
	CheckFailed,
	CheckPassed,
};

struct RuntimeGameplayTomlScenarioPackageFacadeResult {
	std::filesystem::path packagePath;
	std::filesystem::path packageRoot;
	std::filesystem::path manifestPath;
	std::filesystem::path mainScenarioPath;
	RuntimeGameplayTomlScenarioFacadeConfig config;
	RuntimeGameplayTomlScenarioPackageManifest manifest;
	RuntimeGameplayTomlScenarioFacadeResult scenario;
	std::vector<RuntimeGameplayTomlScenarioPackageIssue> issues;
	RuntimeGameplayTomlScenarioPackageFacadeStatus status =
		RuntimeGameplayTomlScenarioPackageFacadeStatus::PackageReadFailed;

	[[nodiscard]] bool ok() const;
};

class RuntimeGameplayTomlScenarioPackageFacade {
public:
	[[nodiscard]] RuntimeGameplayTomlScenarioPackageFacadeResult execute(
		const std::filesystem::path &path,
		const RuntimeGameplayTomlScenarioFacadeConfig &config = {}) const;
};

} // namespace iggy::runtime
