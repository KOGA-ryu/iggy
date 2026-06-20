#include "../apps/native_play/NativeStaticMeshAssetWriter.hpp"
#include "../apps/native_play/NativeStaticMeshExportDirectoryVerification.hpp"
#include "../apps/native_play/NativeStaticMeshFileExport.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include "support/TestHarness.hpp"

namespace {

using iggy::native_play::BuiltInNativeStaticMeshExportAsset;
using iggy::native_play::DefaultNativeStaticMeshExportPolicy;
using iggy::native_play::ExportNativeStaticMeshAssetToDirectory;
using iggy::native_play::ExportNativeStaticMeshPolicyToDirectory;
using iggy::native_play::NativeStaticMeshBuiltInExportId;
using iggy::native_play::NativeStaticMeshExportDirectoryVerificationResult;
using iggy::native_play::NativeStaticMeshExportDirectoryVerificationStatus;
using iggy::native_play::NativeStaticMeshExportManifestFilename;
using iggy::native_play::NativeStaticMeshExportPolicy;
using iggy::native_play::NativeStaticMeshFileExportBatchResult;
using iggy::native_play::NativeStaticMeshFileExportResult;
using iggy::native_play::NativeStaticMeshFileExportStatus;
using iggy::native_play::VerifyNativeStaticMeshExportDirectory;
using iggy::native_play::WriteNativeStaticMeshAssetText;
using iggy::test::Expect;
using iggy::test::Failures;

std::filesystem::path TempRoot()
{
	return std::filesystem::temp_directory_path() /
		"iggy_native_static_mesh_export_directory_verification_tests";
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

void ExportDefaultBatch()
{
	const NativeStaticMeshFileExportBatchResult result =
		ExportNativeStaticMeshPolicyToDirectory(
			DefaultNativeStaticMeshExportPolicy(),
			TempRoot());
	Expect(
		result.status == NativeStaticMeshFileExportStatus::Exported,
		"test setup batch export should succeed");
}

NativeStaticMeshExportDirectoryVerificationResult VerifyDefault()
{
	return VerifyNativeStaticMeshExportDirectory(
		DefaultNativeStaticMeshExportPolicy(),
		TempRoot());
}

void TestBatchExportedDirectoryVerifies()
{
	ResetTempRoot();
	ExportDefaultBatch();

	const NativeStaticMeshExportDirectoryVerificationResult result = VerifyDefault();

	Expect(result.verified(), "batch-exported directory should verify");
	Expect(result.verifiedCount == 3, "verification should count all default assets");
	Expect(result.entries.size() == 3, "verification should report all default assets");
	Expect(result.issueCount == 0, "verified directory should have no issues");
	CleanupTempRoot();
}

void TestMissingOutputDirectoryFailsWithoutCreatingIt()
{
	CleanupTempRoot();
	const std::filesystem::path missing = TempRoot() / "missing";
	const NativeStaticMeshExportDirectoryVerificationResult result =
		VerifyNativeStaticMeshExportDirectory(
			DefaultNativeStaticMeshExportPolicy(),
			missing);

	Expect(
		result.status == NativeStaticMeshExportDirectoryVerificationStatus::MissingOutputDirectory,
		"missing output directory should fail verification");
	Expect(!std::filesystem::exists(TempRoot()), "verification should not create parent directory");
}

void TestFilePathInsteadOfDirectoryFails()
{
	ResetTempRoot();
	const std::filesystem::path filePath = TempRoot() / "not-a-directory";
	WriteText(filePath, "not a directory");

	const NativeStaticMeshExportDirectoryVerificationResult result =
		VerifyNativeStaticMeshExportDirectory(
			DefaultNativeStaticMeshExportPolicy(),
			filePath);

	Expect(
		result.status == NativeStaticMeshExportDirectoryVerificationStatus::OutputDirectoryNotDirectory,
		"file output path should fail as not directory");
	CleanupTempRoot();
}

void TestMissingManifestFails()
{
	ResetTempRoot();
	ExportDefaultBatch();
	std::filesystem::remove(TempRoot() / NativeStaticMeshExportManifestFilename);

	const NativeStaticMeshExportDirectoryVerificationResult result = VerifyDefault();

	Expect(
		result.status == NativeStaticMeshExportDirectoryVerificationStatus::MissingManifest,
		"missing manifest should fail verification");
	CleanupTempRoot();
}

void TestManifestMismatchFails()
{
	ResetTempRoot();
	ExportDefaultBatch();
	WriteText(TempRoot() / NativeStaticMeshExportManifestFilename, "mismatch\n");

	const NativeStaticMeshExportDirectoryVerificationResult result = VerifyDefault();

	Expect(
		result.status == NativeStaticMeshExportDirectoryVerificationStatus::ManifestMismatch,
		"manifest mismatch should fail verification");
	Expect(result.issueCount > 0, "manifest mismatch should report issue count");
	CleanupTempRoot();
}

void TestMissingExpectedAssetFails()
{
	ResetTempRoot();
	ExportDefaultBatch();
	std::filesystem::remove(TempRoot() / "cube.igmesh");

	const NativeStaticMeshExportDirectoryVerificationResult result = VerifyDefault();

	Expect(
		result.status == NativeStaticMeshExportDirectoryVerificationStatus::MissingAsset,
		"missing expected asset should fail verification");
	Expect(result.issueCount > 0, "missing asset should report issue count");
	CleanupTempRoot();
}

void TestCorruptExpectedAssetFailsWithLoadIssue()
{
	ResetTempRoot();
	ExportDefaultBatch();
	WriteText(TempRoot() / "cube.igmesh", "not mesh\n");

	const NativeStaticMeshExportDirectoryVerificationResult result = VerifyDefault();

	Expect(
		result.status == NativeStaticMeshExportDirectoryVerificationStatus::AssetLoadFailed,
		"corrupt asset should fail verification");
	Expect(result.issueCount > 0, "corrupt asset should report load issue count");
	CleanupTempRoot();
}

void TestGeometryMismatchFails()
{
	ResetTempRoot();
	ExportDefaultBatch();
	const auto beanWrite =
		WriteNativeStaticMeshAssetText(
			BuiltInNativeStaticMeshExportAsset(
				NativeStaticMeshBuiltInExportId::Bean));
	Expect(beanWrite.written(), "test setup bean mesh should write");
	WriteText(TempRoot() / "cube.igmesh", beanWrite.text);

	const NativeStaticMeshExportDirectoryVerificationResult result = VerifyDefault();

	Expect(
		result.status == NativeStaticMeshExportDirectoryVerificationStatus::GeometryMismatch,
		"valid mesh with wrong counts should fail verification");
	Expect(result.issueCount > 0, "geometry mismatch should report issue count");
	CleanupTempRoot();
}

void TestSingleExportDirectoryFails()
{
	ResetTempRoot();
	const NativeStaticMeshFileExportResult exportResult =
		ExportNativeStaticMeshAssetToDirectory(
			DefaultNativeStaticMeshExportPolicy(),
			"cube",
			TempRoot());
	Expect(
		exportResult.status == NativeStaticMeshFileExportStatus::Exported,
		"test setup single export should succeed");

	const NativeStaticMeshExportDirectoryVerificationResult result = VerifyDefault();

	Expect(
		result.status == NativeStaticMeshExportDirectoryVerificationStatus::MissingManifest,
		"single export directory should fail because manifest is missing");
	CleanupTempRoot();
}

void TestExtraUnrelatedFilesAreIgnored()
{
	ResetTempRoot();
	ExportDefaultBatch();
	WriteText(TempRoot() / "extra.txt", "ignored");

	const NativeStaticMeshExportDirectoryVerificationResult result = VerifyDefault();

	Expect(result.verified(), "extra unrelated files should be ignored");
	Expect(result.verifiedCount == 3, "extra unrelated files should not change verified count");
	CleanupTempRoot();
}

void TestInvalidPolicyFailsWithoutFilesystemWrites()
{
	ResetTempRoot();
	const NativeStaticMeshExportPolicy policy {
		{
			{ NativeStaticMeshBuiltInExportId::Cube, "cube", "same.igmesh" },
			{ NativeStaticMeshBuiltInExportId::Bean, "bean", "same.igmesh" },
		},
	};

	const NativeStaticMeshExportDirectoryVerificationResult result =
		VerifyNativeStaticMeshExportDirectory(policy, TempRoot());

	Expect(
		result.status == NativeStaticMeshExportDirectoryVerificationStatus::InvalidPolicy,
		"invalid policy should fail verification");
	Expect(result.issueCount > 0, "invalid policy should surface issue count");
	Expect(std::filesystem::is_empty(TempRoot()), "invalid policy verification should not write files");
	CleanupTempRoot();
}

} // namespace

int main()
{
	TestBatchExportedDirectoryVerifies();
	TestMissingOutputDirectoryFailsWithoutCreatingIt();
	TestFilePathInsteadOfDirectoryFails();
	TestMissingManifestFails();
	TestManifestMismatchFails();
	TestMissingExpectedAssetFails();
	TestCorruptExpectedAssetFailsWithLoadIssue();
	TestGeometryMismatchFails();
	TestSingleExportDirectoryFails();
	TestExtraUnrelatedFilesAreIgnored();
	TestInvalidPolicyFailsWithoutFilesystemWrites();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
