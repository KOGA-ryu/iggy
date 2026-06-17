#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "runtime/RuntimeGameplayTomlScenarioFacade.hpp"

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

enum class RuntimeGameplayTomlScenarioPackageIssueCode {
	MissingPackagePath,
	PackagePathNotSupported,
	MissingManifest,
	ManifestReadFailed,
	SyntaxError,
	DuplicateKey,
	UnsupportedKey,
	WrongType,
	MissingFormatId,
	UnsupportedFormatId,
	MissingVersion,
	UnsupportedVersion,
	MissingTitle,
	MissingDescription,
	MissingAuthoringVersion,
	InvalidMetadataValue,
	MissingMain,
	InvalidMainPath,
};

struct RuntimeGameplayTomlScenarioPackageManifest {
	std::string formatId;
	int version = 0;
	std::string title;
	std::string description;
	std::string authoringVersion;
	std::filesystem::path main;
};

struct RuntimeGameplayTomlScenarioPackageIssue {
	RuntimeGameplayTomlScenarioPackageIssueCode code =
		RuntimeGameplayTomlScenarioPackageIssueCode::MissingManifest;
	std::filesystem::path path;
	std::size_t line = 0;
	std::string key;
	std::string detail;
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
