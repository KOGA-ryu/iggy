#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace iggy::runtime {

enum class RuntimeGameplayTomlScenarioPackageReadStatus {
	Read,
	MissingPackagePath,
	PackagePathNotSupported,
	MissingManifest,
	ManifestInvalid,
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

struct RuntimeGameplayTomlScenarioPackageReadResult {
	std::filesystem::path packagePath;
	std::filesystem::path packageRoot;
	std::filesystem::path manifestPath;
	std::filesystem::path mainScenarioPath;
	RuntimeGameplayTomlScenarioPackageManifest manifest;
	std::vector<RuntimeGameplayTomlScenarioPackageIssue> issues;
	RuntimeGameplayTomlScenarioPackageReadStatus status =
		RuntimeGameplayTomlScenarioPackageReadStatus::MissingPackagePath;

	[[nodiscard]] bool ok() const;
};

class RuntimeGameplayTomlScenarioPackageReader {
public:
	[[nodiscard]] RuntimeGameplayTomlScenarioPackageReadResult read(
		const std::filesystem::path &path) const;
};

} // namespace iggy::runtime
