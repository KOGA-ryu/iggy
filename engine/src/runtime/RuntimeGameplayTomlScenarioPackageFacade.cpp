#include "runtime/RuntimeGameplayTomlScenarioPackageFacade.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

namespace iggy::runtime {
namespace {

constexpr const char *PackageFormatId = "iggy:authored-scenario-package";
constexpr int PackageVersion = 1;

std::string Trim(const std::string &text)
{
	const std::size_t begin = text.find_first_not_of(" \t\r\n");
	if (begin == std::string::npos)
		return "";
	const std::size_t end = text.find_last_not_of(" \t\r\n");
	return text.substr(begin, end - begin + 1);
}

std::string StripComment(const std::string &line)
{
	bool quoted = false;
	bool escaped = false;
	for (std::size_t index = 0; index < line.size(); ++index) {
		const char ch = line[index];
		if (escaped) {
			escaped = false;
			continue;
		}
		if (ch == '\\' && quoted) {
			escaped = true;
			continue;
		}
		if (ch == '"') {
			quoted = !quoted;
			continue;
		}
		if (ch == '#' && !quoted)
			return line.substr(0, index);
	}
	return line;
}

bool ParseQuotedString(const std::string &value, std::string &out)
{
	if (value.size() < 2 || value.front() != '"' || value.back() != '"')
		return false;

	std::string parsed;
	bool escaped = false;
	for (std::size_t index = 1; index + 1 < value.size(); ++index) {
		const char ch = value[index];
		if (escaped) {
			parsed.push_back(ch);
			escaped = false;
			continue;
		}
		if (ch == '\\') {
			escaped = true;
			continue;
		}
		if (ch == '"')
			return false;
		parsed.push_back(ch);
	}
	if (escaped)
		return false;
	out = parsed;
	return true;
}

bool ParseInt(const std::string &value, int &out)
{
	if (value.empty())
		return false;
	std::size_t consumed = 0;
	try {
		const int parsed = std::stoi(value, &consumed, 10);
		if (consumed != value.size())
			return false;
		out = parsed;
		return true;
	} catch (...) {
		return false;
	}
}

void AddIssue(
	RuntimeGameplayTomlScenarioPackageFacadeResult &result,
	RuntimeGameplayTomlScenarioPackageIssueCode code,
	const std::filesystem::path &path,
	std::size_t line,
	const std::string &key,
	const std::string &detail)
{
	RuntimeGameplayTomlScenarioPackageIssue issue;
	issue.code = code;
	issue.path = path;
	issue.line = line;
	issue.key = key;
	issue.detail = detail;
	result.issues.push_back(issue);
}

bool HasComponent(const std::filesystem::path &path, const char *component)
{
	for (const std::filesystem::path &part : path) {
		if (part == component)
			return true;
	}
	return false;
}

bool ValidRelativeMainPath(const std::filesystem::path &main)
{
	return !main.empty() && !main.is_absolute() && !HasComponent(main, "..");
}

RuntimeGameplayTomlScenarioPackageFacadeStatus MapScenarioStatus(
	RuntimeGameplayTomlScenarioFacadeStatus status)
{
	switch (status) {
	case RuntimeGameplayTomlScenarioFacadeStatus::ReadFailed:
		return RuntimeGameplayTomlScenarioPackageFacadeStatus::ScenarioReadFailed;
	case RuntimeGameplayTomlScenarioFacadeStatus::ConversionFailed:
		return RuntimeGameplayTomlScenarioPackageFacadeStatus::ConversionFailed;
	case RuntimeGameplayTomlScenarioFacadeStatus::LintFailed:
		return RuntimeGameplayTomlScenarioPackageFacadeStatus::LintFailed;
	case RuntimeGameplayTomlScenarioFacadeStatus::LintOk:
		return RuntimeGameplayTomlScenarioPackageFacadeStatus::LintOk;
	case RuntimeGameplayTomlScenarioFacadeStatus::RunFailed:
		return RuntimeGameplayTomlScenarioPackageFacadeStatus::RunFailed;
	case RuntimeGameplayTomlScenarioFacadeStatus::Ran:
		return RuntimeGameplayTomlScenarioPackageFacadeStatus::Ran;
	case RuntimeGameplayTomlScenarioFacadeStatus::CheckFailed:
		return RuntimeGameplayTomlScenarioPackageFacadeStatus::CheckFailed;
	case RuntimeGameplayTomlScenarioFacadeStatus::CheckPassed:
		return RuntimeGameplayTomlScenarioPackageFacadeStatus::CheckPassed;
	}
	return RuntimeGameplayTomlScenarioPackageFacadeStatus::PackageInvalid;
}

bool ReadManifest(RuntimeGameplayTomlScenarioPackageFacadeResult &result)
{
	std::ifstream stream(result.manifestPath);
	if (!stream) {
		AddIssue(
			result,
			RuntimeGameplayTomlScenarioPackageIssueCode::ManifestReadFailed,
			result.manifestPath,
			0,
			"",
			"package manifest could not be read");
		return false;
	}

	bool hasFormatId = false;
	bool hasVersion = false;
	bool hasTitle = false;
	bool hasDescription = false;
	bool hasAuthoringVersion = false;
	bool hasMain = false;
	std::string rawLine;
	std::size_t lineNumber = 0;
	while (std::getline(stream, rawLine)) {
		++lineNumber;
		const std::string line = Trim(StripComment(rawLine));
		if (line.empty())
			continue;
		if (line.front() == '[') {
			AddIssue(
				result,
				RuntimeGameplayTomlScenarioPackageIssueCode::SyntaxError,
				result.manifestPath,
				lineNumber,
				"",
				"package manifest does not support tables");
			return false;
		}

		const std::size_t equals = line.find('=');
		if (equals == std::string::npos) {
			AddIssue(
				result,
				RuntimeGameplayTomlScenarioPackageIssueCode::SyntaxError,
				result.manifestPath,
				lineNumber,
				"",
				"expected key = value");
			return false;
		}

		const std::string key = Trim(line.substr(0, equals));
		const std::string value = Trim(line.substr(equals + 1));
		if (key == "format_id") {
			if (hasFormatId) {
				AddIssue(
					result,
					RuntimeGameplayTomlScenarioPackageIssueCode::DuplicateKey,
					result.manifestPath,
					lineNumber,
					key,
					"duplicate package manifest key");
				return false;
			}
			std::string parsed;
			if (!ParseQuotedString(value, parsed)) {
				AddIssue(
					result,
					RuntimeGameplayTomlScenarioPackageIssueCode::WrongType,
					result.manifestPath,
					lineNumber,
					key,
					"format_id must be a string");
				return false;
			}
			result.manifest.formatId = parsed;
			hasFormatId = true;
		} else if (key == "version") {
			if (hasVersion) {
				AddIssue(
					result,
					RuntimeGameplayTomlScenarioPackageIssueCode::DuplicateKey,
					result.manifestPath,
					lineNumber,
					key,
					"duplicate package manifest key");
				return false;
			}
			int parsed = 0;
			if (!ParseInt(value, parsed)) {
				AddIssue(
					result,
					RuntimeGameplayTomlScenarioPackageIssueCode::WrongType,
					result.manifestPath,
					lineNumber,
					key,
					"version must be an integer");
				return false;
			}
			result.manifest.version = parsed;
			hasVersion = true;
		} else if (key == "title") {
			if (hasTitle) {
				AddIssue(
					result,
					RuntimeGameplayTomlScenarioPackageIssueCode::DuplicateKey,
					result.manifestPath,
					lineNumber,
					key,
					"duplicate package manifest key");
				return false;
			}
			std::string parsed;
			if (!ParseQuotedString(value, parsed)) {
				AddIssue(
					result,
					RuntimeGameplayTomlScenarioPackageIssueCode::WrongType,
					result.manifestPath,
					lineNumber,
					key,
					"title must be a string");
				return false;
			}
			result.manifest.title = parsed;
			hasTitle = true;
		} else if (key == "description") {
			if (hasDescription) {
				AddIssue(
					result,
					RuntimeGameplayTomlScenarioPackageIssueCode::DuplicateKey,
					result.manifestPath,
					lineNumber,
					key,
					"duplicate package manifest key");
				return false;
			}
			std::string parsed;
			if (!ParseQuotedString(value, parsed)) {
				AddIssue(
					result,
					RuntimeGameplayTomlScenarioPackageIssueCode::WrongType,
					result.manifestPath,
					lineNumber,
					key,
					"description must be a string");
				return false;
			}
			result.manifest.description = parsed;
			hasDescription = true;
		} else if (key == "authoring_version") {
			if (hasAuthoringVersion) {
				AddIssue(
					result,
					RuntimeGameplayTomlScenarioPackageIssueCode::DuplicateKey,
					result.manifestPath,
					lineNumber,
					key,
					"duplicate package manifest key");
				return false;
			}
			std::string parsed;
			if (!ParseQuotedString(value, parsed)) {
				AddIssue(
					result,
					RuntimeGameplayTomlScenarioPackageIssueCode::WrongType,
					result.manifestPath,
					lineNumber,
					key,
					"authoring_version must be a string");
				return false;
			}
			result.manifest.authoringVersion = parsed;
			hasAuthoringVersion = true;
		} else if (key == "main") {
			if (hasMain) {
				AddIssue(
					result,
					RuntimeGameplayTomlScenarioPackageIssueCode::DuplicateKey,
					result.manifestPath,
					lineNumber,
					key,
					"duplicate package manifest key");
				return false;
			}
			std::string parsed;
			if (!ParseQuotedString(value, parsed)) {
				AddIssue(
					result,
					RuntimeGameplayTomlScenarioPackageIssueCode::WrongType,
					result.manifestPath,
					lineNumber,
					key,
					"main must be a string");
				return false;
			}
			result.manifest.main = std::filesystem::path(parsed);
			hasMain = true;
		} else {
			AddIssue(
				result,
				RuntimeGameplayTomlScenarioPackageIssueCode::UnsupportedKey,
				result.manifestPath,
				lineNumber,
				key,
				"unsupported package manifest key");
			return false;
		}
	}

	if (!hasFormatId) {
		AddIssue(
			result,
			RuntimeGameplayTomlScenarioPackageIssueCode::MissingFormatId,
			result.manifestPath,
			0,
			"format_id",
			"package manifest is missing format_id");
		return false;
	}
	if (result.manifest.formatId != PackageFormatId) {
		AddIssue(
			result,
			RuntimeGameplayTomlScenarioPackageIssueCode::UnsupportedFormatId,
			result.manifestPath,
			0,
			"format_id",
			"unsupported package format_id");
		return false;
	}
	if (!hasVersion) {
		AddIssue(
			result,
			RuntimeGameplayTomlScenarioPackageIssueCode::MissingVersion,
			result.manifestPath,
			0,
			"version",
			"package manifest is missing version");
		return false;
	}
	if (result.manifest.version != PackageVersion) {
		AddIssue(
			result,
			RuntimeGameplayTomlScenarioPackageIssueCode::UnsupportedVersion,
			result.manifestPath,
			0,
			"version",
			"unsupported package version");
		return false;
	}
	if (!hasTitle) {
		AddIssue(
			result,
			RuntimeGameplayTomlScenarioPackageIssueCode::MissingTitle,
			result.manifestPath,
			0,
			"title",
			"package manifest is missing title");
		return false;
	}
	if (Trim(result.manifest.title).empty()) {
		AddIssue(
			result,
			RuntimeGameplayTomlScenarioPackageIssueCode::InvalidMetadataValue,
			result.manifestPath,
			0,
			"title",
			"title must not be empty");
		return false;
	}
	if (!hasDescription) {
		AddIssue(
			result,
			RuntimeGameplayTomlScenarioPackageIssueCode::MissingDescription,
			result.manifestPath,
			0,
			"description",
			"package manifest is missing description");
		return false;
	}
	if (Trim(result.manifest.description).empty()) {
		AddIssue(
			result,
			RuntimeGameplayTomlScenarioPackageIssueCode::InvalidMetadataValue,
			result.manifestPath,
			0,
			"description",
			"description must not be empty");
		return false;
	}
	if (!hasAuthoringVersion) {
		AddIssue(
			result,
			RuntimeGameplayTomlScenarioPackageIssueCode::MissingAuthoringVersion,
			result.manifestPath,
			0,
			"authoring_version",
			"package manifest is missing authoring_version");
		return false;
	}
	if (Trim(result.manifest.authoringVersion).empty()) {
		AddIssue(
			result,
			RuntimeGameplayTomlScenarioPackageIssueCode::InvalidMetadataValue,
			result.manifestPath,
			0,
			"authoring_version",
			"authoring_version must not be empty");
		return false;
	}
	if (!hasMain) {
		AddIssue(
			result,
			RuntimeGameplayTomlScenarioPackageIssueCode::MissingMain,
			result.manifestPath,
			0,
			"main",
			"package manifest is missing main");
		return false;
	}
	if (!ValidRelativeMainPath(result.manifest.main)) {
		AddIssue(
			result,
			RuntimeGameplayTomlScenarioPackageIssueCode::InvalidMainPath,
			result.manifestPath,
			0,
			"main",
			"main must be a non-empty relative path inside the package");
		return false;
	}
	return true;
}

} // namespace

bool RuntimeGameplayTomlScenarioPackageFacadeResult::ok() const
{
	return status == RuntimeGameplayTomlScenarioPackageFacadeStatus::Ran
		|| status == RuntimeGameplayTomlScenarioPackageFacadeStatus::LintOk
		|| status == RuntimeGameplayTomlScenarioPackageFacadeStatus::CheckPassed;
}

RuntimeGameplayTomlScenarioPackageFacadeResult
RuntimeGameplayTomlScenarioPackageFacade::execute(
	const std::filesystem::path &path,
	const RuntimeGameplayTomlScenarioFacadeConfig &config) const
{
	RuntimeGameplayTomlScenarioPackageFacadeResult result;
	result.packagePath = path;
	result.config = config;

	std::error_code error;
	if (path.empty() || !std::filesystem::exists(path, error)) {
		AddIssue(
			result,
			RuntimeGameplayTomlScenarioPackageIssueCode::MissingPackagePath,
			path,
			0,
			"",
			"package path does not exist");
		result.status =
			RuntimeGameplayTomlScenarioPackageFacadeStatus::PackageReadFailed;
		return result;
	}

	if (std::filesystem::is_directory(path, error)) {
		result.packageRoot = path;
		result.manifestPath = path / "package.toml";
	} else if (std::filesystem::is_regular_file(path, error) &&
		path.filename() == "package.toml") {
		result.manifestPath = path;
		result.packageRoot = path.parent_path();
	} else {
		AddIssue(
			result,
			RuntimeGameplayTomlScenarioPackageIssueCode::PackagePathNotSupported,
			path,
			0,
			"",
			"package path must be a directory or package.toml file");
		result.status =
			RuntimeGameplayTomlScenarioPackageFacadeStatus::PackageReadFailed;
		return result;
	}

	if (!std::filesystem::exists(result.manifestPath, error)) {
		AddIssue(
			result,
			RuntimeGameplayTomlScenarioPackageIssueCode::MissingManifest,
			result.manifestPath,
			0,
			"",
			"package manifest does not exist");
		result.status =
			RuntimeGameplayTomlScenarioPackageFacadeStatus::PackageReadFailed;
		return result;
	}

	if (!ReadManifest(result)) {
		result.status =
			RuntimeGameplayTomlScenarioPackageFacadeStatus::PackageInvalid;
		return result;
	}

	result.mainScenarioPath =
		(result.packageRoot / result.manifest.main).lexically_normal();
	result.scenario =
		RuntimeGameplayTomlScenarioFacade {}.execute(result.mainScenarioPath, config);
	result.status = MapScenarioStatus(result.scenario.status);
	return result;
}

} // namespace iggy::runtime
