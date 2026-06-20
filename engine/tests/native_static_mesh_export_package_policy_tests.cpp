#include "../apps/native_play/NativeStaticMeshExportPackagePolicy.hpp"
#include "../apps/native_play/NativeStaticMeshFileExport.hpp"

#include <cstdlib>
#include <string>

#include "support/TestHarness.hpp"

namespace {

using iggy::native_play::DefaultNativeStaticMeshExportPackagePolicy;
using iggy::native_play::NativeStaticMeshBuiltInExportId;
using iggy::native_play::NativeStaticMeshExportPackageFormatId;
using iggy::native_play::NativeStaticMeshExportPackageFormatVersion;
using iggy::native_play::NativeStaticMeshExportPackageManifestFilename;
using iggy::native_play::NativeStaticMeshExportPackagePolicy;
using iggy::native_play::NativeStaticMeshExportPackagePolicyValidationIssue;
using iggy::native_play::NativeStaticMeshExportPackagePolicyValidationIssueCode;
using iggy::native_play::NativeStaticMeshExportPackagePolicyValidationResult;
using iggy::native_play::NativeStaticMeshExportManifestFilename;
using iggy::native_play::ValidateNativeStaticMeshExportPackagePolicy;
using iggy::test::Expect;
using iggy::test::Failures;

bool HasValidationIssue(
	const NativeStaticMeshExportPackagePolicyValidationResult &result,
	NativeStaticMeshExportPackagePolicyValidationIssueCode code)
{
	for (const NativeStaticMeshExportPackagePolicyValidationIssue &issue :
			result.issues) {
		if (issue.code == code)
			return true;
	}
	return false;
}

void TestDefaultPackagePolicyCarriesStableMetadata()
{
	const NativeStaticMeshExportPackagePolicy policy =
		DefaultNativeStaticMeshExportPackagePolicy();

	Expect(
		policy.formatId == NativeStaticMeshExportPackageFormatId,
		"default package format id should be exact");
	Expect(
		policy.formatId == "iggy:native-static-mesh-export-package",
		"default package format id should match published value");
	Expect(
		policy.version == NativeStaticMeshExportPackageFormatVersion,
		"default package version should match current version");
	Expect(policy.version == 1, "default package version should be 1");
	Expect(
		policy.manifestFilename == NativeStaticMeshExportPackageManifestFilename,
		"default package manifest filename should match current filename");
	Expect(
		policy.manifestFilename == "static-mesh-export-manifest.txt",
		"default package manifest filename should be exact");
	Expect(
		policy.manifestFilename == NativeStaticMeshExportManifestFilename,
		"default package manifest filename should match batch export sidecar");
	Expect(
		policy.meshPolicy.assets.size() == 3,
		"default package policy should wrap default mesh export policy");
	if (policy.meshPolicy.assets.size() == 3) {
		Expect(policy.meshPolicy.assets[0].name == "cube", "cube should be first");
		Expect(policy.meshPolicy.assets[1].name == "bean", "bean should be second");
		Expect(
			policy.meshPolicy.assets[2].name == "npc-marker",
			"npc marker should be third");
	}
}

void TestDefaultPackagePolicyValidatesCleanly()
{
	const NativeStaticMeshExportPackagePolicyValidationResult result =
		ValidateNativeStaticMeshExportPackagePolicy(
			DefaultNativeStaticMeshExportPackagePolicy());

	Expect(result.valid(), "default package policy should validate cleanly");
	Expect(result.issues.empty(), "default package policy should report no issues");
}

void TestValidationRejectsEmptyFormatId()
{
	NativeStaticMeshExportPackagePolicy policy =
		DefaultNativeStaticMeshExportPackagePolicy();
	policy.formatId = "";

	const NativeStaticMeshExportPackagePolicyValidationResult result =
		ValidateNativeStaticMeshExportPackagePolicy(policy);

	Expect(!result.valid(), "empty format id should invalidate package policy");
	Expect(
		HasValidationIssue(
			result,
			NativeStaticMeshExportPackagePolicyValidationIssueCode::EmptyFormatId),
		"empty format id should report issue");
}

void TestValidationRejectsUnsupportedFormatId()
{
	NativeStaticMeshExportPackagePolicy policy =
		DefaultNativeStaticMeshExportPackagePolicy();
	policy.formatId = "iggy:other-format";

	const NativeStaticMeshExportPackagePolicyValidationResult result =
		ValidateNativeStaticMeshExportPackagePolicy(policy);

	Expect(!result.valid(), "unsupported format id should invalidate package policy");
	Expect(
		HasValidationIssue(
			result,
			NativeStaticMeshExportPackagePolicyValidationIssueCode::UnsupportedFormatId),
		"unsupported format id should report issue");
}

void TestValidationRejectsUnsupportedVersion()
{
	NativeStaticMeshExportPackagePolicy policy =
		DefaultNativeStaticMeshExportPackagePolicy();
	policy.version = 2;

	const NativeStaticMeshExportPackagePolicyValidationResult result =
		ValidateNativeStaticMeshExportPackagePolicy(policy);

	Expect(!result.valid(), "unsupported version should invalidate package policy");
	Expect(
		HasValidationIssue(
			result,
			NativeStaticMeshExportPackagePolicyValidationIssueCode::UnsupportedVersion),
		"unsupported version should report issue");
}

void TestValidationRejectsEmptyManifestFilename()
{
	NativeStaticMeshExportPackagePolicy policy =
		DefaultNativeStaticMeshExportPackagePolicy();
	policy.manifestFilename = "";

	const NativeStaticMeshExportPackagePolicyValidationResult result =
		ValidateNativeStaticMeshExportPackagePolicy(policy);

	Expect(!result.valid(), "empty manifest filename should invalidate package policy");
	Expect(
		HasValidationIssue(
			result,
			NativeStaticMeshExportPackagePolicyValidationIssueCode::EmptyManifestFilename),
		"empty manifest filename should report issue");
}

void TestValidationRejectsManifestFilenameSeparators()
{
	NativeStaticMeshExportPackagePolicy slashPolicy =
		DefaultNativeStaticMeshExportPackagePolicy();
	slashPolicy.manifestFilename = "nested/static-mesh-export-manifest.txt";
	NativeStaticMeshExportPackagePolicy backslashPolicy =
		DefaultNativeStaticMeshExportPackagePolicy();
	backslashPolicy.manifestFilename = "nested\\static-mesh-export-manifest.txt";

	const NativeStaticMeshExportPackagePolicyValidationResult slashResult =
		ValidateNativeStaticMeshExportPackagePolicy(slashPolicy);
	const NativeStaticMeshExportPackagePolicyValidationResult backslashResult =
		ValidateNativeStaticMeshExportPackagePolicy(backslashPolicy);

	Expect(!slashResult.valid(), "slash manifest filename should invalidate policy");
	Expect(
		HasValidationIssue(
			slashResult,
			NativeStaticMeshExportPackagePolicyValidationIssueCode::ManifestFilenameContainsSeparator),
		"slash manifest filename should report separator issue");
	Expect(
		!backslashResult.valid(),
		"backslash manifest filename should invalidate policy");
	Expect(
		HasValidationIssue(
			backslashResult,
			NativeStaticMeshExportPackagePolicyValidationIssueCode::ManifestFilenameContainsSeparator),
		"backslash manifest filename should report separator issue");
}

void TestValidationRejectsManifestFilenameCollision()
{
	NativeStaticMeshExportPackagePolicy policy =
		DefaultNativeStaticMeshExportPackagePolicy();
	policy.manifestFilename = "cube.igmesh";

	const NativeStaticMeshExportPackagePolicyValidationResult result =
		ValidateNativeStaticMeshExportPackagePolicy(policy);

	Expect(!result.valid(), "manifest filename collision should invalidate policy");
	Expect(
		HasValidationIssue(
			result,
			NativeStaticMeshExportPackagePolicyValidationIssueCode::ManifestFilenameCollidesWithAssetFilename),
		"manifest filename collision should report issue");
}

void TestValidationRejectsInvalidNestedMeshExportPolicy()
{
	NativeStaticMeshExportPackagePolicy policy =
		DefaultNativeStaticMeshExportPackagePolicy();
	policy.meshPolicy.assets[0].name = "";

	const NativeStaticMeshExportPackagePolicyValidationResult result =
		ValidateNativeStaticMeshExportPackagePolicy(policy);

	Expect(!result.valid(), "invalid nested mesh policy should invalidate package policy");
	Expect(
		HasValidationIssue(
			result,
			NativeStaticMeshExportPackagePolicyValidationIssueCode::InvalidMeshExportPolicy),
		"invalid nested mesh policy should report issue");
	for (const NativeStaticMeshExportPackagePolicyValidationIssue &issue :
			result.issues) {
		if (issue.code ==
				NativeStaticMeshExportPackagePolicyValidationIssueCode::InvalidMeshExportPolicy) {
			Expect(issue.nestedIssueCount > 0, "nested issue count should be surfaced");
			break;
		}
	}
}

void TestValidationIsDeterministic()
{
	NativeStaticMeshExportPackagePolicy policy =
		DefaultNativeStaticMeshExportPackagePolicy();
	policy.formatId = "";
	policy.version = 99;
	policy.manifestFilename = "cube.igmesh";
	policy.meshPolicy.assets[1].defaultFilename = "cube.igmesh";

	const NativeStaticMeshExportPackagePolicyValidationResult first =
		ValidateNativeStaticMeshExportPackagePolicy(policy);
	const NativeStaticMeshExportPackagePolicyValidationResult second =
		ValidateNativeStaticMeshExportPackagePolicy(policy);

	Expect(first.issues.size() == second.issues.size(), "validation issue count should be deterministic");
	if (first.issues.size() != second.issues.size())
		return;
	for (std::size_t index = 0; index < first.issues.size(); ++index) {
		Expect(
			first.issues[index].code == second.issues[index].code,
			"validation issue order should be deterministic");
		Expect(
			first.issues[index].value == second.issues[index].value,
			"validation issue values should be deterministic");
	}
}

void TestManifestFilenameCollisionReportsAssetIndex()
{
	NativeStaticMeshExportPackagePolicy policy =
		DefaultNativeStaticMeshExportPackagePolicy();
	policy.manifestFilename = "npc-marker.igmesh";

	const NativeStaticMeshExportPackagePolicyValidationResult result =
		ValidateNativeStaticMeshExportPackagePolicy(policy);

	for (const NativeStaticMeshExportPackagePolicyValidationIssue &issue :
			result.issues) {
		if (issue.code ==
				NativeStaticMeshExportPackagePolicyValidationIssueCode::ManifestFilenameCollidesWithAssetFilename) {
			Expect(issue.assetIndex == 2, "manifest collision should report matching asset index");
			return;
		}
	}
	Expect(false, "manifest collision issue should be present");
}

void TestPackagePolicyDoesNotNeedFilesystemState()
{
	NativeStaticMeshExportPackagePolicy policy =
		DefaultNativeStaticMeshExportPackagePolicy();
	policy.manifestFilename = "missing-on-disk-is-not-checked.txt";

	const NativeStaticMeshExportPackagePolicyValidationResult result =
		ValidateNativeStaticMeshExportPackagePolicy(policy);

	Expect(result.valid(), "package policy validation should not touch filesystem");
}

} // namespace

int main()
{
	TestDefaultPackagePolicyCarriesStableMetadata();
	TestDefaultPackagePolicyValidatesCleanly();
	TestValidationRejectsEmptyFormatId();
	TestValidationRejectsUnsupportedFormatId();
	TestValidationRejectsUnsupportedVersion();
	TestValidationRejectsEmptyManifestFilename();
	TestValidationRejectsManifestFilenameSeparators();
	TestValidationRejectsManifestFilenameCollision();
	TestValidationRejectsInvalidNestedMeshExportPolicy();
	TestValidationIsDeterministic();
	TestManifestFilenameCollisionReportsAssetIndex();
	TestPackagePolicyDoesNotNeedFilesystemState();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
