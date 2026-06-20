#include "../apps/native_play/NativeStaticMeshAssetLoader.hpp"
#include "../apps/native_play/NativeStaticMeshAssetWriter.hpp"
#include "../apps/native_play/NativeStaticMeshFileExport.hpp"
#include "../apps/native_play/NativeStaticMeshExportManifest.hpp"
#include "../apps/native_play/NativeStaticMeshExportPackageManifest.hpp"

#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <iterator>
#include <string>

#include "support/TestHarness.hpp"

namespace {

using iggy::native_play::BuiltInNativeStaticMeshExportAsset;
using iggy::native_play::BuildNativeStaticMeshExportManifestText;
using iggy::native_play::BuildNativeStaticMeshExportPackageManifestText;
using iggy::native_play::BuildNativeStaticMeshFileExportSuccessText;
using iggy::native_play::DefaultNativeStaticMeshExportPolicy;
using iggy::native_play::DefaultNativeStaticMeshExportPackagePolicy;
using iggy::native_play::ExportNativeStaticMeshAssetToDirectory;
using iggy::native_play::ExportNativeStaticMeshPolicyToDirectory;
using iggy::native_play::FindNativeStaticMeshExportAsset;
using iggy::native_play::LoadNativeStaticMeshAssetFile;
using iggy::native_play::NativeStaticMeshAssetLoadResult;
using iggy::native_play::NativeStaticMeshAssetWriteResult;
using iggy::native_play::NativeStaticMeshFileExportBatchResult;
using iggy::native_play::NativeStaticMeshExportAssetRef;
using iggy::native_play::NativeStaticMeshExportPolicy;
using iggy::native_play::NativeStaticMeshFileExportResult;
using iggy::native_play::NativeStaticMeshFileExportStatus;
using iggy::native_play::NativeStaticMeshFileExportStatusText;
using iggy::native_play::NativeStaticMeshExportManifestFilename;
using iggy::native_play::NativeStaticMeshExportManifestResult;
using iggy::native_play::NativeStaticMeshExportPackageManifestSidecarFilename;
using iggy::native_play::NativeStaticMeshExportPackageManifestResult;
using iggy::native_play::NativeStaticMeshExportPackagePolicy;
using iggy::native_play::WriteNativeStaticMeshAssetText;
using iggy::test::Expect;
using iggy::test::Failures;

std::filesystem::path TempRoot()
{
	return std::filesystem::temp_directory_path() /
		"iggy_native_static_mesh_file_export_tests";
}

void ResetTempRoot()
{
	std::error_code ignored;
	std::filesystem::remove_all(TempRoot(), ignored);
	std::filesystem::create_directories(TempRoot(), ignored);
}

void CleanupTempRoot()
{
	std::error_code ignored;
	std::filesystem::remove_all(TempRoot(), ignored);
}

void WriteText(const std::filesystem::path &path, const std::string &text)
{
	std::ofstream file(path, std::ios::binary);
	file << text;
}

std::string ReadText(const std::filesystem::path &path)
{
	std::ifstream file(path, std::ios::binary);
	return {
		std::istreambuf_iterator<char>(file),
		std::istreambuf_iterator<char>(),
	};
}

void TestExportsDefaultPolicyAssetsAndReloads()
{
	ResetTempRoot();
	const NativeStaticMeshExportPolicy policy = DefaultNativeStaticMeshExportPolicy();

	for (const NativeStaticMeshExportAssetRef &asset : policy.assets) {
		const NativeStaticMeshFileExportResult result =
			ExportNativeStaticMeshAssetToDirectory(policy, asset.name, TempRoot());

		Expect(
			result.status == NativeStaticMeshFileExportStatus::Exported,
			asset.name + " should export");
		Expect(result.outputPath == TempRoot() / asset.defaultFilename, asset.name + " should use default filename");
		Expect(result.byteCount > 0, asset.name + " should report written bytes");
		Expect(result.issueCount == 0, asset.name + " should report no writer issues");
		Expect(std::filesystem::exists(result.outputPath), asset.name + " file should exist");

		const NativeStaticMeshAssetLoadResult loaded =
			LoadNativeStaticMeshAssetFile(result.outputPath);
		Expect(loaded.loaded(), asset.name + " exported file should reload");

		const NativeStaticMeshAssetWriteResult expected =
			WriteNativeStaticMeshAssetText(BuiltInNativeStaticMeshExportAsset(asset.id));
		Expect(expected.written(), asset.name + " expected writer output should write");
		Expect(result.byteCount == expected.text.size(), asset.name + " byte count should match writer text size");
	}

	CleanupTempRoot();
}

void TestBatchExportsDefaultPolicyAssetsAndReloads()
{
	ResetTempRoot();
	const NativeStaticMeshExportPolicy policy = DefaultNativeStaticMeshExportPolicy();
	const NativeStaticMeshFileExportBatchResult result =
		ExportNativeStaticMeshPolicyToDirectory(policy, TempRoot());

	Expect(
		result.status == NativeStaticMeshFileExportStatus::Exported,
		"batch export should succeed");
	Expect(result.entries.size() == policy.assets.size(), "batch export should report every policy asset");
	Expect(result.exportedCount == policy.assets.size(), "batch export should count exported assets");
	Expect(
		result.manifestOutputPath == TempRoot() / NativeStaticMeshExportManifestFilename,
		"batch export should report manifest output path");
	Expect(
		result.packageManifestOutputPath == TempRoot() / NativeStaticMeshExportPackageManifestSidecarFilename,
		"batch export should report package manifest output path");

	std::size_t expectedBytes = 0;
	for (const NativeStaticMeshExportAssetRef &asset : policy.assets) {
		const std::filesystem::path output = TempRoot() / asset.defaultFilename;
		Expect(std::filesystem::exists(output), asset.name + " batch output should exist");
		const NativeStaticMeshAssetLoadResult loaded =
			LoadNativeStaticMeshAssetFile(output);
		Expect(loaded.loaded(), asset.name + " batch output should reload");

		const NativeStaticMeshAssetWriteResult expected =
			WriteNativeStaticMeshAssetText(BuiltInNativeStaticMeshExportAsset(asset.id));
		Expect(expected.written(), asset.name + " expected writer output should write");
		expectedBytes += expected.text.size();
	}
	Expect(result.byteCount == expectedBytes, "batch byte count should match writer text sizes");

	const NativeStaticMeshExportManifestResult manifest =
		BuildNativeStaticMeshExportManifestText(policy);
	Expect(manifest.written(), "default manifest should write");
	Expect(
		std::filesystem::exists(result.manifestOutputPath),
		"batch export should write manifest sidecar");
	Expect(
		ReadText(result.manifestOutputPath) == manifest.text,
		"batch manifest sidecar should match manifest builder output");
	Expect(
		result.manifestByteCount == manifest.text.size(),
		"batch manifest byte count should match manifest text size");
	NativeStaticMeshExportPackagePolicy packagePolicy =
		DefaultNativeStaticMeshExportPackagePolicy();
	packagePolicy.meshPolicy = policy;
	const NativeStaticMeshExportPackageManifestResult packageManifest =
		BuildNativeStaticMeshExportPackageManifestText(packagePolicy);
	Expect(packageManifest.written(), "default package manifest should write");
	Expect(
		std::filesystem::exists(result.packageManifestOutputPath),
		"batch export should write package manifest sidecar");
	Expect(
		ReadText(result.packageManifestOutputPath) == packageManifest.text,
		"batch package manifest sidecar should match package manifest builder output");
	Expect(
		result.packageManifestByteCount == packageManifest.text.size(),
		"batch package manifest byte count should match package manifest text size");

	CleanupTempRoot();
}

void TestUnknownAssetDoesNotCreateFiles()
{
	ResetTempRoot();
	const NativeStaticMeshFileExportResult result =
		ExportNativeStaticMeshAssetToDirectory(
			DefaultNativeStaticMeshExportPolicy(),
			"nope",
			TempRoot());

	Expect(
		result.status == NativeStaticMeshFileExportStatus::UnknownAsset,
		"unknown asset should be rejected");
	Expect(std::filesystem::is_empty(TempRoot()), "unknown asset should not create files");
	CleanupTempRoot();
}

void TestMissingOutputDirectoryRejectedWithoutCreatingParents()
{
	CleanupTempRoot();
	const std::filesystem::path missing = TempRoot() / "missing";
	const NativeStaticMeshFileExportResult result =
		ExportNativeStaticMeshAssetToDirectory(
			DefaultNativeStaticMeshExportPolicy(),
			"cube",
			missing);

	Expect(
		result.status == NativeStaticMeshFileExportStatus::MissingOutputDirectory,
		"missing output directory should be rejected");
	Expect(!std::filesystem::exists(TempRoot()), "export should not create parent directories");
}

void TestBatchMissingOutputDirectoryRejectedWithoutCreatingParents()
{
	CleanupTempRoot();
	const std::filesystem::path missing = TempRoot() / "missing";
	const NativeStaticMeshFileExportBatchResult result =
		ExportNativeStaticMeshPolicyToDirectory(
			DefaultNativeStaticMeshExportPolicy(),
			missing);

	Expect(
		result.status == NativeStaticMeshFileExportStatus::MissingOutputDirectory,
		"batch missing output directory should be rejected");
	Expect(!std::filesystem::exists(TempRoot()), "batch export should not create parent directories");
}

void TestOutputPathMustBeDirectory()
{
	ResetTempRoot();
	const std::filesystem::path filePath = TempRoot() / "not-a-directory";
	WriteText(filePath, "not a directory");

	const NativeStaticMeshFileExportResult result =
		ExportNativeStaticMeshAssetToDirectory(
			DefaultNativeStaticMeshExportPolicy(),
			"cube",
			filePath);

	Expect(
		result.status == NativeStaticMeshFileExportStatus::OutputDirectoryNotDirectory,
		"file output path should be rejected as not directory");
	Expect(!std::filesystem::exists(filePath / "cube.igmesh"), "not-directory export should not create target");
	CleanupTempRoot();
}

void TestBatchOutputPathMustBeDirectory()
{
	ResetTempRoot();
	const std::filesystem::path filePath = TempRoot() / "not-a-directory";
	WriteText(filePath, "not a directory");

	const NativeStaticMeshFileExportBatchResult result =
		ExportNativeStaticMeshPolicyToDirectory(
			DefaultNativeStaticMeshExportPolicy(),
			filePath);

	Expect(
		result.status == NativeStaticMeshFileExportStatus::OutputDirectoryNotDirectory,
		"batch file output path should be rejected as not directory");
	Expect(!std::filesystem::exists(filePath / "cube.igmesh"), "batch not-directory export should not create target");
	CleanupTempRoot();
}

void TestTargetAlreadyExistsIsRejected()
{
	ResetTempRoot();
	const NativeStaticMeshExportPolicy policy = DefaultNativeStaticMeshExportPolicy();
	const NativeStaticMeshExportAssetRef *cube =
		FindNativeStaticMeshExportAsset(policy, "cube");
	Expect(cube != nullptr, "cube policy ref should exist");
	if (cube == nullptr)
		return;

	const std::filesystem::path target = TempRoot() / cube->defaultFilename;
	WriteText(target, "existing");
	const NativeStaticMeshFileExportResult result =
		ExportNativeStaticMeshAssetToDirectory(policy, "cube", TempRoot());

	Expect(
		result.status == NativeStaticMeshFileExportStatus::TargetAlreadyExists,
		"existing target should be rejected");
	const NativeStaticMeshAssetLoadResult loaded =
		LoadNativeStaticMeshAssetFile(target);
	Expect(!loaded.loaded(), "existing target should not be overwritten with valid mesh text");
	CleanupTempRoot();
}

void TestBatchTargetAlreadyExistsPreflightsBeforeWriting()
{
	ResetTempRoot();
	const NativeStaticMeshExportPolicy policy = DefaultNativeStaticMeshExportPolicy();
	const NativeStaticMeshExportAssetRef *bean =
		FindNativeStaticMeshExportAsset(policy, "bean");
	Expect(bean != nullptr, "bean policy ref should exist");
	if (bean == nullptr)
		return;

	WriteText(TempRoot() / bean->defaultFilename, "existing");
	const NativeStaticMeshFileExportBatchResult result =
		ExportNativeStaticMeshPolicyToDirectory(policy, TempRoot());

	Expect(
		result.status == NativeStaticMeshFileExportStatus::TargetAlreadyExists,
		"batch existing target should be rejected");
	Expect(!std::filesystem::exists(TempRoot() / "cube.igmesh"), "batch preflight should not write earlier assets");
	Expect(!std::filesystem::exists(TempRoot() / "npc-marker.igmesh"), "batch preflight should not write later assets");
	Expect(
		!std::filesystem::exists(TempRoot() / NativeStaticMeshExportManifestFilename),
		"batch target collision should not write manifest sidecar");
	Expect(
		!std::filesystem::exists(TempRoot() / NativeStaticMeshExportPackageManifestSidecarFilename),
		"batch target collision should not write package manifest sidecar");
	CleanupTempRoot();
}

void TestBatchManifestTargetAlreadyExistsPreflightsBeforeWriting()
{
	ResetTempRoot();
	WriteText(TempRoot() / NativeStaticMeshExportManifestFilename, "existing");

	const NativeStaticMeshFileExportBatchResult result =
		ExportNativeStaticMeshPolicyToDirectory(
			DefaultNativeStaticMeshExportPolicy(),
			TempRoot());

	Expect(
		result.status == NativeStaticMeshFileExportStatus::TargetAlreadyExists,
		"existing manifest target should reject batch export");
	Expect(!std::filesystem::exists(TempRoot() / "cube.igmesh"), "manifest collision should not write cube");
	Expect(!std::filesystem::exists(TempRoot() / "bean.igmesh"), "manifest collision should not write bean");
	Expect(!std::filesystem::exists(TempRoot() / "npc-marker.igmesh"), "manifest collision should not write NPC marker");
	Expect(
		!std::filesystem::exists(TempRoot() / NativeStaticMeshExportPackageManifestSidecarFilename),
		"manifest collision should not write package manifest sidecar");
	Expect(
		ReadText(TempRoot() / NativeStaticMeshExportManifestFilename) == "existing",
		"manifest collision should not overwrite existing sidecar");
	CleanupTempRoot();
}

void TestBatchPackageManifestTargetAlreadyExistsPreflightsBeforeWriting()
{
	ResetTempRoot();
	WriteText(TempRoot() / NativeStaticMeshExportPackageManifestSidecarFilename, "existing");

	const NativeStaticMeshFileExportBatchResult result =
		ExportNativeStaticMeshPolicyToDirectory(
			DefaultNativeStaticMeshExportPolicy(),
			TempRoot());

	Expect(
		result.status == NativeStaticMeshFileExportStatus::TargetAlreadyExists,
		"existing package manifest target should reject batch export");
	Expect(
		result.packageManifestOutputPath == TempRoot() / NativeStaticMeshExportPackageManifestSidecarFilename,
		"package manifest collision should report package manifest output path");
	Expect(!std::filesystem::exists(TempRoot() / "cube.igmesh"), "package manifest collision should not write cube");
	Expect(!std::filesystem::exists(TempRoot() / "bean.igmesh"), "package manifest collision should not write bean");
	Expect(!std::filesystem::exists(TempRoot() / "npc-marker.igmesh"), "package manifest collision should not write NPC marker");
	Expect(
		!std::filesystem::exists(TempRoot() / NativeStaticMeshExportManifestFilename),
		"package manifest collision should not write mesh manifest sidecar");
	Expect(
		ReadText(TempRoot() / NativeStaticMeshExportPackageManifestSidecarFilename) == "existing",
		"package manifest collision should not overwrite existing sidecar");
	CleanupTempRoot();
}

void TestSingleExportDoesNotWriteManifestSidecar()
{
	ResetTempRoot();
	const NativeStaticMeshFileExportResult result =
		ExportNativeStaticMeshAssetToDirectory(
			DefaultNativeStaticMeshExportPolicy(),
			"cube",
			TempRoot());

	Expect(
		result.status == NativeStaticMeshFileExportStatus::Exported,
		"single export should still succeed");
	Expect(std::filesystem::exists(TempRoot() / "cube.igmesh"), "single export should write selected mesh");
	Expect(
		!std::filesystem::exists(TempRoot() / NativeStaticMeshExportManifestFilename),
		"single export should not write manifest sidecar");
	Expect(
		!std::filesystem::exists(TempRoot() / NativeStaticMeshExportPackageManifestSidecarFilename),
		"single export should not write package manifest sidecar");
	CleanupTempRoot();
}

void TestSingleExportSuccessText()
{
	ResetTempRoot();
	const NativeStaticMeshFileExportResult result =
		ExportNativeStaticMeshAssetToDirectory(
			DefaultNativeStaticMeshExportPolicy(),
			"cube",
			TempRoot());

	Expect(
		result.status == NativeStaticMeshFileExportStatus::Exported,
		"single export success text test should export cube");
	const std::string expected =
		"static-mesh-export name=cube output=" +
		(TempRoot() / "cube.igmesh").string() +
		" bytes=523\n";
	Expect(
		BuildNativeStaticMeshFileExportSuccessText("cube", result) == expected,
		"single export success text should match CLI contract");
	CleanupTempRoot();
}

void TestInvalidPolicyRejectsSingleExportWithoutWriting()
{
	ResetTempRoot();
	const NativeStaticMeshExportPolicy policy {
		{ { iggy::native_play::NativeStaticMeshBuiltInExportId::Cube, "", "cube.igmesh" } },
	};

	const NativeStaticMeshFileExportResult result =
		ExportNativeStaticMeshAssetToDirectory(policy, "cube", TempRoot());

	Expect(
		result.status == NativeStaticMeshFileExportStatus::InvalidPolicy,
		"single export should reject invalid policy");
	Expect(result.issueCount > 0, "invalid policy result should surface validation issue count");
	Expect(std::filesystem::is_empty(TempRoot()), "invalid policy single export should not create files");
	CleanupTempRoot();
}

void TestInvalidPolicyRejectsBatchExportBeforeWriting()
{
	ResetTempRoot();
	const NativeStaticMeshExportPolicy policy {
		{
			{ iggy::native_play::NativeStaticMeshBuiltInExportId::Cube, "cube", "same.igmesh" },
			{ iggy::native_play::NativeStaticMeshBuiltInExportId::Bean, "bean", "same.igmesh" },
		},
	};

	const NativeStaticMeshFileExportBatchResult result =
		ExportNativeStaticMeshPolicyToDirectory(policy, TempRoot());

	Expect(
		result.status == NativeStaticMeshFileExportStatus::InvalidPolicy,
		"batch export should reject invalid policy");
	Expect(result.issueCount > 0, "batch invalid policy should surface validation issue count");
	Expect(std::filesystem::is_empty(TempRoot()), "batch invalid policy should not create files");
	CleanupTempRoot();
}

void TestFileExportStatusText()
{
	struct Case {
		NativeStaticMeshFileExportStatus status;
		const char *text;
	};

	const Case cases[] = {
		{ NativeStaticMeshFileExportStatus::Exported, "Exported" },
		{ NativeStaticMeshFileExportStatus::InvalidPolicy, "InvalidPolicy" },
		{ NativeStaticMeshFileExportStatus::UnknownAsset, "UnknownAsset" },
		{ NativeStaticMeshFileExportStatus::MissingOutputDirectory, "MissingOutputDirectory" },
		{ NativeStaticMeshFileExportStatus::OutputDirectoryNotDirectory, "OutputDirectoryNotDirectory" },
		{ NativeStaticMeshFileExportStatus::TargetAlreadyExists, "TargetAlreadyExists" },
		{ NativeStaticMeshFileExportStatus::WriterFailed, "WriterFailed" },
		{ NativeStaticMeshFileExportStatus::FileOpenFailed, "FileOpenFailed" },
		{ NativeStaticMeshFileExportStatus::WriteFailed, "WriteFailed" },
	};

	for (const Case &testCase : cases) {
		Expect(
			std::string(NativeStaticMeshFileExportStatusText(testCase.status)) ==
				testCase.text,
			"file export status text should match stable spelling");
	}
	Expect(
		std::string(NativeStaticMeshFileExportStatusText(
			static_cast<NativeStaticMeshFileExportStatus>(999))) == "Unknown",
		"file export status text should report unknown fallback");
}

} // namespace

int main()
{
	TestExportsDefaultPolicyAssetsAndReloads();
	TestBatchExportsDefaultPolicyAssetsAndReloads();
	TestUnknownAssetDoesNotCreateFiles();
	TestMissingOutputDirectoryRejectedWithoutCreatingParents();
	TestBatchMissingOutputDirectoryRejectedWithoutCreatingParents();
	TestOutputPathMustBeDirectory();
	TestBatchOutputPathMustBeDirectory();
	TestTargetAlreadyExistsIsRejected();
	TestBatchTargetAlreadyExistsPreflightsBeforeWriting();
	TestBatchManifestTargetAlreadyExistsPreflightsBeforeWriting();
	TestBatchPackageManifestTargetAlreadyExistsPreflightsBeforeWriting();
	TestSingleExportDoesNotWriteManifestSidecar();
	TestSingleExportSuccessText();
	TestInvalidPolicyRejectsSingleExportWithoutWriting();
	TestInvalidPolicyRejectsBatchExportBeforeWriting();
	TestFileExportStatusText();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
