#include "runtime/RuntimeGameplayAuthoringPreviewModel.hpp"

namespace iggy::runtime {
namespace {

RuntimeGameplayAuthoringPreviewStatus MapScenarioStatus(
	RuntimeGameplayTomlScenarioFacadeStatus status)
{
	switch (status) {
	case RuntimeGameplayTomlScenarioFacadeStatus::ReadFailed:
		return RuntimeGameplayAuthoringPreviewStatus::ReadFailed;
	case RuntimeGameplayTomlScenarioFacadeStatus::ConversionFailed:
		return RuntimeGameplayAuthoringPreviewStatus::ConversionFailed;
	case RuntimeGameplayTomlScenarioFacadeStatus::LintFailed:
		return RuntimeGameplayAuthoringPreviewStatus::LintFailed;
	case RuntimeGameplayTomlScenarioFacadeStatus::LintOk:
		return RuntimeGameplayAuthoringPreviewStatus::LintOk;
	case RuntimeGameplayTomlScenarioFacadeStatus::RunFailed:
		return RuntimeGameplayAuthoringPreviewStatus::RunFailed;
	case RuntimeGameplayTomlScenarioFacadeStatus::Ran:
		return RuntimeGameplayAuthoringPreviewStatus::Ran;
	case RuntimeGameplayTomlScenarioFacadeStatus::CheckFailed:
		return RuntimeGameplayAuthoringPreviewStatus::CheckFailed;
	case RuntimeGameplayTomlScenarioFacadeStatus::CheckPassed:
		return RuntimeGameplayAuthoringPreviewStatus::CheckPassed;
	}
	return RuntimeGameplayAuthoringPreviewStatus::ReadFailed;
}

RuntimeGameplayAuthoringPreviewStatus MapPackageStatus(
	RuntimeGameplayTomlScenarioPackageFacadeStatus status)
{
	switch (status) {
	case RuntimeGameplayTomlScenarioPackageFacadeStatus::PackageReadFailed:
		return RuntimeGameplayAuthoringPreviewStatus::PackageReadFailed;
	case RuntimeGameplayTomlScenarioPackageFacadeStatus::PackageInvalid:
		return RuntimeGameplayAuthoringPreviewStatus::PackageInvalid;
	case RuntimeGameplayTomlScenarioPackageFacadeStatus::ScenarioReadFailed:
		return RuntimeGameplayAuthoringPreviewStatus::ReadFailed;
	case RuntimeGameplayTomlScenarioPackageFacadeStatus::ConversionFailed:
		return RuntimeGameplayAuthoringPreviewStatus::ConversionFailed;
	case RuntimeGameplayTomlScenarioPackageFacadeStatus::LintFailed:
		return RuntimeGameplayAuthoringPreviewStatus::LintFailed;
	case RuntimeGameplayTomlScenarioPackageFacadeStatus::LintOk:
		return RuntimeGameplayAuthoringPreviewStatus::LintOk;
	case RuntimeGameplayTomlScenarioPackageFacadeStatus::RunFailed:
		return RuntimeGameplayAuthoringPreviewStatus::RunFailed;
	case RuntimeGameplayTomlScenarioPackageFacadeStatus::Ran:
		return RuntimeGameplayAuthoringPreviewStatus::Ran;
	case RuntimeGameplayTomlScenarioPackageFacadeStatus::CheckFailed:
		return RuntimeGameplayAuthoringPreviewStatus::CheckFailed;
	case RuntimeGameplayTomlScenarioPackageFacadeStatus::CheckPassed:
		return RuntimeGameplayAuthoringPreviewStatus::CheckPassed;
	}
	return RuntimeGameplayAuthoringPreviewStatus::PackageReadFailed;
}

bool IsPackagePath(const std::filesystem::path &path)
{
	std::error_code ignored;
	return std::filesystem::is_directory(path, ignored) ||
		path.filename() == "package.toml";
}

void CopyScenarioProjection(
	RuntimeGameplayAuthoringPreviewModel &model,
	const RuntimeGameplayTomlScenarioFacadeResult &scenario)
{
	model.sourcePath = scenario.path;
	model.scenarioStatus = scenario.status;
	model.summary = scenario.runSummary;
	model.finalRows = scenario.runSummary.finalRows;
	model.traceFrames = scenario.traceFrames;
	model.expectation = scenario.expectationComparison;
	model.diagnostics = scenario.diagnostics;
}

RuntimeGameplayAuthoringPreviewPackageMetadata CopyPackageMetadata(
	const RuntimeGameplayTomlScenarioPackageFacadeResult &package)
{
	RuntimeGameplayAuthoringPreviewPackageMetadata metadata;
	metadata.present = true;
	metadata.packagePath = package.packagePath;
	metadata.packageRoot = package.packageRoot;
	metadata.manifestPath = package.manifestPath;
	metadata.mainScenarioPath = package.mainScenarioPath;
	metadata.title = package.manifest.title;
	metadata.description = package.manifest.description;
	metadata.authoringVersion = package.manifest.authoringVersion;
	metadata.main = package.manifest.main;
	return metadata;
}

} // namespace

bool RuntimeGameplayAuthoringPreviewModel::ok() const
{
	return status == RuntimeGameplayAuthoringPreviewStatus::Ran ||
		status == RuntimeGameplayAuthoringPreviewStatus::LintOk ||
		status == RuntimeGameplayAuthoringPreviewStatus::CheckPassed;
}

RuntimeGameplayAuthoringPreviewModel
RuntimeGameplayAuthoringPreviewModelBuilder::build(
	const std::filesystem::path &path,
	const RuntimeGameplayTomlScenarioFacadeConfig &config) const
{
	RuntimeGameplayAuthoringPreviewModel model;
	model.inputPath = path;
	model.config = config;

	if (IsPackagePath(path)) {
		model.inputKind = RuntimeGameplayAuthoringPreviewInputKind::Package;
		const RuntimeGameplayTomlScenarioPackageFacadeResult package =
			RuntimeGameplayTomlScenarioPackageFacade {}.execute(path, config);
		model.status = MapPackageStatus(package.status);
		model.packageStatus = package.status;
		model.package = CopyPackageMetadata(package);
		model.packageIssues = package.issues;
		CopyScenarioProjection(model, package.scenario);
		if (!package.mainScenarioPath.empty())
			model.sourcePath = package.mainScenarioPath;
		return model;
	}

	model.inputKind = RuntimeGameplayAuthoringPreviewInputKind::TomlFile;
	const RuntimeGameplayTomlScenarioFacadeResult scenario =
		RuntimeGameplayTomlScenarioFacade {}.execute(path, config);
	model.status = MapScenarioStatus(scenario.status);
	CopyScenarioProjection(model, scenario);
	return model;
}

} // namespace iggy::runtime
