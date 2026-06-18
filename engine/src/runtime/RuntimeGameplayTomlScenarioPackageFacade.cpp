#include "runtime/RuntimeGameplayTomlScenarioPackageFacade.hpp"

namespace iggy::runtime {
namespace {

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

RuntimeGameplayTomlScenarioPackageFacadeStatus MapPackageReadStatus(
	RuntimeGameplayTomlScenarioPackageReadStatus status)
{
	switch (status) {
	case RuntimeGameplayTomlScenarioPackageReadStatus::Read:
		break;
	case RuntimeGameplayTomlScenarioPackageReadStatus::MissingPackagePath:
	case RuntimeGameplayTomlScenarioPackageReadStatus::PackagePathNotSupported:
	case RuntimeGameplayTomlScenarioPackageReadStatus::MissingManifest:
		return RuntimeGameplayTomlScenarioPackageFacadeStatus::PackageReadFailed;
	case RuntimeGameplayTomlScenarioPackageReadStatus::ManifestInvalid:
		return RuntimeGameplayTomlScenarioPackageFacadeStatus::PackageInvalid;
	}
	return RuntimeGameplayTomlScenarioPackageFacadeStatus::PackageInvalid;
}

void CopyPackageReadFields(
	RuntimeGameplayTomlScenarioPackageFacadeResult &result,
	const RuntimeGameplayTomlScenarioPackageReadResult &read)
{
	result.packagePath = read.packagePath;
	result.packageRoot = read.packageRoot;
	result.manifestPath = read.manifestPath;
	result.mainScenarioPath = read.mainScenarioPath;
	result.manifest = read.manifest;
	result.issues = read.issues;
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
	result.config = config;

	const RuntimeGameplayTomlScenarioPackageReadResult read =
		RuntimeGameplayTomlScenarioPackageReader {}.read(path);
	CopyPackageReadFields(result, read);
	if (!read.ok()) {
		result.status = MapPackageReadStatus(read.status);
		return result;
	}

	result.scenario =
		RuntimeGameplayTomlScenarioFacade {}.execute(result.mainScenarioPath, config);
	result.status = MapScenarioStatus(result.scenario.status);
	return result;
}

} // namespace iggy::runtime
