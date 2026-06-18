#include "runtime/RuntimeGameplayProductScenarioLoader.hpp"

namespace iggy::runtime {
namespace {

void AddIssue(
	RuntimeGameplayProductScenarioLoadResult &result,
	RuntimeGameplayProductScenarioLoadIssueCode code,
	const std::filesystem::path &path,
	const std::string &key,
	const std::string &detail)
{
	RuntimeGameplayProductScenarioLoadIssue issue;
	issue.code = code;
	issue.path = path;
	issue.key = key;
	issue.detail = detail;
	result.issues.push_back(issue);
}

RuntimeGameplayProductScenarioLoadStatus MapPackageStatus(
	RuntimeGameplayTomlScenarioPackageReadStatus status)
{
	switch (status) {
	case RuntimeGameplayTomlScenarioPackageReadStatus::Read:
		return RuntimeGameplayProductScenarioLoadStatus::Loaded;
	case RuntimeGameplayTomlScenarioPackageReadStatus::MissingPackagePath:
	case RuntimeGameplayTomlScenarioPackageReadStatus::PackagePathNotSupported:
	case RuntimeGameplayTomlScenarioPackageReadStatus::MissingManifest:
		return RuntimeGameplayProductScenarioLoadStatus::PackageReadFailed;
	case RuntimeGameplayTomlScenarioPackageReadStatus::ManifestInvalid:
		return RuntimeGameplayProductScenarioLoadStatus::PackageInvalid;
	}
	return RuntimeGameplayProductScenarioLoadStatus::PackageInvalid;
}

void AddPackageIssues(RuntimeGameplayProductScenarioLoadResult &result)
{
	for (const RuntimeGameplayTomlScenarioPackageIssue &issue :
		result.packageRead.issues) {
		AddIssue(
			result,
			RuntimeGameplayProductScenarioLoadIssueCode::PackageIssue,
			issue.path,
			issue.key,
			issue.detail);
	}
}

void AddSourceReadIssues(RuntimeGameplayProductScenarioLoadResult &result)
{
	for (const RuntimeGameplayAsciiSourcePlanTomlFileReadIssue &issue :
		result.sourceRead.issues) {
		AddIssue(
			result,
			RuntimeGameplayProductScenarioLoadIssueCode::TomlFileReadIssue,
			issue.path,
			"",
			issue.detail);
	}
	if (!result.sourceRead.text.issues.empty()) {
		for (const RuntimeGameplayAsciiSourcePlanTomlReadIssue &issue :
			result.sourceRead.text.issues) {
			AddIssue(
				result,
				RuntimeGameplayProductScenarioLoadIssueCode::TomlFileReadIssue,
				result.sourceRead.path,
				issue.key,
				issue.detail);
		}
	}
	if (result.sourceRead.issues.empty() &&
		result.sourceRead.text.issues.empty()) {
		AddIssue(
			result,
			RuntimeGameplayProductScenarioLoadIssueCode::TomlFileReadIssue,
			result.sourceRead.path,
			"",
			"source TOML file could not be read");
	}
}

void AddAuthoringIssues(RuntimeGameplayProductScenarioLoadResult &result)
{
	for (const RuntimeGameplayScenarioAuthoringAdapterIssue &issue :
		result.adapter.issues) {
		AddIssue(
			result,
			RuntimeGameplayProductScenarioLoadIssueCode::
				AuthoringConversionIssue,
			result.sourcePath,
			"",
			"authoring conversion failed");
		(void)issue;
	}
	if (result.adapter.issues.empty()) {
		AddIssue(
			result,
			RuntimeGameplayProductScenarioLoadIssueCode::
				AuthoringConversionIssue,
			result.sourcePath,
			"",
			"authoring conversion failed");
	}
}

void AddValidationIssues(RuntimeGameplayProductScenarioLoadResult &result)
{
	for (const RuntimeGameplayProfileScenarioIssue &issue :
		result.validation.issues) {
		AddIssue(
			result,
			RuntimeGameplayProductScenarioLoadIssueCode::
				ProfileValidationIssue,
			result.sourcePath,
			"",
			"profile scenario validation failed");
		(void)issue;
	}
	if (result.validation.issues.empty()) {
		AddIssue(
			result,
			RuntimeGameplayProductScenarioLoadIssueCode::
				ProfileValidationIssue,
			result.sourcePath,
			"",
			"profile scenario validation failed");
	}
}

bool IsPackageManifestPath(const std::filesystem::path &path)
{
	return path.filename() == "package.toml";
}

bool IsTomlSourcePath(const std::filesystem::path &path)
{
	return path.extension() == ".toml" && !IsPackageManifestPath(path);
}

void CopyPackageReadFields(RuntimeGameplayProductScenarioLoadResult &result)
{
	result.hasPackage = true;
	result.packagePath = result.packageRead.packagePath;
	result.packageRoot = result.packageRead.packageRoot;
	result.manifestPath = result.packageRead.manifestPath;
	result.mainScenarioPath = result.packageRead.mainScenarioPath;
	result.packageManifest = result.packageRead.manifest;
}

void LoadSource(
	RuntimeGameplayProductScenarioLoadResult &result,
	const std::filesystem::path &sourcePath)
{
	result.sourcePath = sourcePath;
	result.sourceRead =
		RuntimeGameplayAsciiSourcePlanTomlFileReader {}.read(sourcePath);
	if (!result.sourceRead.ok()) {
		result.status =
			RuntimeGameplayProductScenarioLoadStatus::SourceReadFailed;
		AddSourceReadIssues(result);
		return;
	}

	result.hasExpectations = result.sourceRead.text.plan.hasExpectations();
	result.expectations = result.sourceRead.text.plan.expectations;

	RuntimeGameplayScenarioAuthoringPacket packet;
	packet.source = RuntimeGameplayScenarioAuthoringSource::AsciiSourcePlan;
	packet.hasAsciiSourcePlan = true;
	packet.asciiSourcePlan = result.sourceRead.text.plan;
	result.adapter = RuntimeGameplayScenarioAuthoringAdapter {}.convert(packet);
	if (!result.adapter.ok()) {
		result.status =
			RuntimeGameplayProductScenarioLoadStatus::ConversionFailed;
		AddAuthoringIssues(result);
		return;
	}

	result.definition = result.adapter.profileScenario;
	result.initialState = result.definition.initialState;
	result.validation =
		RuntimeGameplayProfileScenarioValidator {}.validate(result.definition);
	if (!result.validation.ok()) {
		result.status =
			RuntimeGameplayProductScenarioLoadStatus::ValidationFailed;
		AddValidationIssues(result);
		return;
	}

	result.status = RuntimeGameplayProductScenarioLoadStatus::Loaded;
}

} // namespace

bool RuntimeGameplayProductScenarioLoadResult::ok() const
{
	return status == RuntimeGameplayProductScenarioLoadStatus::Loaded;
}

RuntimeGameplayProductScenarioLoadResult
RuntimeGameplayProductScenarioLoader::load(const std::filesystem::path &path) const
{
	RuntimeGameplayProductScenarioLoadResult result;
	result.inputPath = path;

	std::error_code error;
	if (path.empty() || !std::filesystem::exists(path, error)) {
		result.status =
			RuntimeGameplayProductScenarioLoadStatus::InputPathMissing;
		AddIssue(
			result,
			RuntimeGameplayProductScenarioLoadIssueCode::InputPathMissing,
			path,
			"",
			"input path does not exist");
		return result;
	}

	if (std::filesystem::is_directory(path, error) ||
		(std::filesystem::is_regular_file(path, error) &&
			IsPackageManifestPath(path))) {
		result.packageRead =
			RuntimeGameplayTomlScenarioPackageReader {}.read(path);
		CopyPackageReadFields(result);
		if (!result.packageRead.ok()) {
			result.status = MapPackageStatus(result.packageRead.status);
			AddPackageIssues(result);
			return result;
		}
		LoadSource(result, result.packageRead.mainScenarioPath);
		return result;
	}

	if (std::filesystem::is_regular_file(path, error) &&
		IsTomlSourcePath(path)) {
		LoadSource(result, path);
		return result;
	}

	result.status = RuntimeGameplayProductScenarioLoadStatus::InputPathUnsupported;
	AddIssue(
		result,
		RuntimeGameplayProductScenarioLoadIssueCode::InputPathUnsupported,
		path,
		"",
		"input path must be a TOML source file, package directory, or package.toml");
	return result;
}

} // namespace iggy::runtime
