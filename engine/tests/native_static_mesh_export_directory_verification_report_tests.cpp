#include "../apps/native_play/NativeStaticMeshAssetWriter.hpp"
#include "../apps/native_play/NativeStaticMeshExportDirectoryVerificationReport.hpp"
#include "../apps/native_play/NativeStaticMeshFileExport.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include "support/TestHarness.hpp"

namespace {

using iggy::native_play::BuildNativeStaticMeshExportDirectoryVerificationReport;
using iggy::native_play::BuiltInNativeStaticMeshExportAsset;
using iggy::native_play::DefaultNativeStaticMeshExportPolicy;
using iggy::native_play::ExportNativeStaticMeshPolicyToDirectory;
using iggy::native_play::NativeStaticMeshBuiltInExportId;
using iggy::native_play::NativeStaticMeshExportDirectoryVerificationReport;
using iggy::native_play::NativeStaticMeshExportDirectoryVerificationStatus;
using iggy::native_play::NativeStaticMeshExportManifestFilename;
using iggy::native_play::NativeStaticMeshExportPackageManifestSidecarFilename;
using iggy::native_play::NativeStaticMeshFileExportBatchResult;
using iggy::native_play::NativeStaticMeshFileExportStatus;
using iggy::native_play::WriteNativeStaticMeshAssetText;
using iggy::test::Expect;
using iggy::test::Failures;

std::filesystem::path TempRoot()
{
	return std::filesystem::temp_directory_path() /
		"iggy_native_static_mesh_export_directory_verification_report_tests";
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

NativeStaticMeshExportDirectoryVerificationReport BuildDefaultReport()
{
	return BuildNativeStaticMeshExportDirectoryVerificationReport(
		DefaultNativeStaticMeshExportPolicy(),
		TempRoot());
}

void TestBatchExportedDirectoryReportSucceeds()
{
	ResetTempRoot();
	ExportDefaultBatch();

	const NativeStaticMeshExportDirectoryVerificationReport report =
		BuildDefaultReport();

	Expect(report.verified(), "batch-exported directory report should verify");
	Expect(
		report.verification.status == NativeStaticMeshExportDirectoryVerificationStatus::Verified,
		"batch-exported report status should be verified");
	Expect(report.verification.verifiedCount == 3, "verified report should count three assets");
	Expect(
		report.text.find("static-mesh-export-verification-report status=Verified") != std::string::npos,
		"report should include verified summary row");
	Expect(
		report.text.find("manifest=ok packageManifest=ok") != std::string::npos,
		"verified report should include sidecar success diagnostics");
	Expect(
		report.text.find("asset=cube filename=cube.igmesh status=Verified vertices=8 expectedVertices=8 indices=36 expectedIndices=36 issues=0") != std::string::npos,
		"report should include cube entry row");
	Expect(
		report.text.find("asset=bean filename=bean.igmesh status=Verified") != std::string::npos,
		"report should include bean entry row");
	Expect(
		report.text.find("asset=npc-marker filename=npc-marker.igmesh status=Verified") != std::string::npos,
		"report should include NPC marker entry row");
	CleanupTempRoot();
}

void TestMissingOutputDirectoryReportFails()
{
	CleanupTempRoot();
	const std::filesystem::path missing = TempRoot() / "missing";
	const NativeStaticMeshExportDirectoryVerificationReport report =
		BuildNativeStaticMeshExportDirectoryVerificationReport(
			DefaultNativeStaticMeshExportPolicy(),
			missing);

	Expect(!report.verified(), "missing directory report should fail");
	Expect(
		report.verification.status == NativeStaticMeshExportDirectoryVerificationStatus::MissingOutputDirectory,
		"missing directory report should expose status");
	Expect(
		report.text.find("status=MissingOutputDirectory") != std::string::npos,
		"missing directory report should include status text");
	Expect(
		report.text.find("problem=" + missing.string()) != std::string::npos,
		"missing directory report should include problem path");
}

void TestFilePathInsteadOfDirectoryReportFails()
{
	ResetTempRoot();
	const std::filesystem::path filePath = TempRoot() / "not-a-directory";
	WriteText(filePath, "not a directory");

	const NativeStaticMeshExportDirectoryVerificationReport report =
		BuildNativeStaticMeshExportDirectoryVerificationReport(
			DefaultNativeStaticMeshExportPolicy(),
			filePath);

	Expect(!report.verified(), "file output path report should fail");
	Expect(
		report.verification.status == NativeStaticMeshExportDirectoryVerificationStatus::OutputDirectoryNotDirectory,
		"file output path report should expose not-directory status");
	CleanupTempRoot();
}

void TestMissingManifestReportFails()
{
	ResetTempRoot();
	ExportDefaultBatch();
	const std::filesystem::path manifest =
		TempRoot() / NativeStaticMeshExportManifestFilename;
	std::filesystem::remove(manifest);

	const NativeStaticMeshExportDirectoryVerificationReport report =
		BuildDefaultReport();

	Expect(!report.verified(), "missing manifest report should fail");
	Expect(
		report.verification.status == NativeStaticMeshExportDirectoryVerificationStatus::MissingManifest,
		"missing manifest report should expose status");
	Expect(
		report.text.find("problem=" + manifest.string()) != std::string::npos,
		"missing manifest report should include manifest problem path");
	Expect(
		report.text.find("manifest=missing packageManifest=not-checked") != std::string::npos,
		"missing manifest report should not claim package manifest was checked");
	CleanupTempRoot();
}

void TestManifestMismatchReportFails()
{
	ResetTempRoot();
	ExportDefaultBatch();
	WriteText(TempRoot() / NativeStaticMeshExportManifestFilename, "mismatch\n");

	const NativeStaticMeshExportDirectoryVerificationReport report =
		BuildDefaultReport();

	Expect(!report.verified(), "manifest mismatch report should fail");
	Expect(
		report.verification.status == NativeStaticMeshExportDirectoryVerificationStatus::ManifestMismatch,
		"manifest mismatch report should expose status");
	Expect(
		report.text.find("manifest=mismatch packageManifest=not-checked") != std::string::npos,
		"manifest mismatch report should not claim package manifest was checked");
	CleanupTempRoot();
}

void TestMissingPackageManifestReportFails()
{
	ResetTempRoot();
	ExportDefaultBatch();
	const std::filesystem::path packageManifest =
		TempRoot() / NativeStaticMeshExportPackageManifestSidecarFilename;
	std::filesystem::remove(packageManifest);

	const NativeStaticMeshExportDirectoryVerificationReport report =
		BuildDefaultReport();

	Expect(!report.verified(), "missing package manifest report should fail");
	Expect(
		report.verification.status == NativeStaticMeshExportDirectoryVerificationStatus::MissingPackageManifest,
		"missing package manifest report should expose status");
	Expect(
		report.text.find("status=MissingPackageManifest") != std::string::npos,
		"missing package manifest report should include status text");
	Expect(
		report.text.find("problem=" + packageManifest.string()) != std::string::npos,
		"missing package manifest report should include package manifest problem path");
	Expect(
		report.text.find("manifest=ok packageManifest=missing") != std::string::npos,
		"missing package manifest report should include sidecar diagnostics");
	Expect(
		report.verification.entries.empty(),
		"missing package manifest report should not include asset entries");
	CleanupTempRoot();
}

void TestPackageManifestMismatchReportFails()
{
	ResetTempRoot();
	ExportDefaultBatch();
	const std::filesystem::path packageManifest =
		TempRoot() / NativeStaticMeshExportPackageManifestSidecarFilename;
	WriteText(
		packageManifest,
		"static-mesh-export-package-manifest format=iggy:native-static-mesh-export-package version=1 manifest=other-manifest.txt assets=3\n"
		"asset=cube filename=cube.igmesh\n"
		"asset=bean filename=bean.igmesh\n"
		"asset=npc-marker filename=npc-marker.igmesh\n");

	const NativeStaticMeshExportDirectoryVerificationReport report =
		BuildDefaultReport();

	Expect(!report.verified(), "package manifest mismatch report should fail");
	Expect(
		report.verification.status == NativeStaticMeshExportDirectoryVerificationStatus::PackageManifestMismatch,
		"package manifest mismatch report should expose status");
	Expect(
		report.text.find("status=PackageManifestMismatch") != std::string::npos,
		"package manifest mismatch report should include status text");
	Expect(
		report.text.find("problem=" + packageManifest.string()) != std::string::npos,
		"package manifest mismatch report should include package manifest problem path");
	Expect(
		report.text.find("manifest=ok packageManifest=mismatch") != std::string::npos,
		"package manifest mismatch report should include sidecar diagnostics");
	Expect(
		report.verification.entries.empty(),
		"package manifest mismatch report should not include asset entries");
	CleanupTempRoot();
}

void TestMalformedPackageManifestReportFails()
{
	ResetTempRoot();
	ExportDefaultBatch();
	const std::filesystem::path packageManifest =
		TempRoot() / NativeStaticMeshExportPackageManifestSidecarFilename;
	WriteText(packageManifest, "static-mesh-export-package format=bad\n");

	const NativeStaticMeshExportDirectoryVerificationReport report =
		BuildDefaultReport();

	Expect(!report.verified(), "malformed package manifest report should fail");
	Expect(
		report.verification.status == NativeStaticMeshExportDirectoryVerificationStatus::PackageManifestReadFailed,
		"malformed package manifest report should expose read-failed status");
	Expect(
		report.text.find("status=PackageManifestReadFailed") != std::string::npos,
		"malformed package manifest report should include status text");
	Expect(
		report.text.find("problem=" + packageManifest.string()) != std::string::npos,
		"malformed package manifest report should include package manifest problem path");
	Expect(
		report.text.find("manifest=ok packageManifest=invalid") != std::string::npos,
		"malformed package manifest report should include invalid sidecar diagnostics");
	Expect(
		report.verification.packageManifestReadIssueCount > 0,
		"malformed package manifest report should expose read issue count");
	Expect(
		report.verification.entries.empty(),
		"malformed package manifest report should not include asset entries");
	CleanupTempRoot();
}

void TestMissingAssetReportFailsWithEntry()
{
	ResetTempRoot();
	ExportDefaultBatch();
	std::filesystem::remove(TempRoot() / "cube.igmesh");

	const NativeStaticMeshExportDirectoryVerificationReport report =
		BuildDefaultReport();

	Expect(!report.verified(), "missing asset report should fail");
	Expect(
		report.verification.status == NativeStaticMeshExportDirectoryVerificationStatus::MissingAsset,
		"missing asset report should expose top-level status");
	Expect(
		report.text.find("asset=cube filename=cube.igmesh status=MissingAsset") != std::string::npos,
		"missing asset report should include asset entry status");
	CleanupTempRoot();
}

void TestCorruptAssetReportFailsWithIssueCount()
{
	ResetTempRoot();
	ExportDefaultBatch();
	WriteText(TempRoot() / "cube.igmesh", "not mesh\n");

	const NativeStaticMeshExportDirectoryVerificationReport report =
		BuildDefaultReport();

	Expect(!report.verified(), "corrupt asset report should fail");
	Expect(
		report.verification.status == NativeStaticMeshExportDirectoryVerificationStatus::AssetLoadFailed,
		"corrupt asset report should expose load-failed status");
	Expect(report.verification.issueCount > 0, "corrupt asset report should expose issue count");
	Expect(
		report.text.find("asset=cube filename=cube.igmesh status=AssetLoadFailed") != std::string::npos,
		"corrupt asset report should include load-failed entry");
	CleanupTempRoot();
}

void TestGeometryMismatchReportFails()
{
	ResetTempRoot();
	ExportDefaultBatch();
	const auto beanWrite =
		WriteNativeStaticMeshAssetText(
			BuiltInNativeStaticMeshExportAsset(
				NativeStaticMeshBuiltInExportId::Bean));
	Expect(beanWrite.written(), "test setup bean mesh should write");
	WriteText(TempRoot() / "cube.igmesh", beanWrite.text);

	const NativeStaticMeshExportDirectoryVerificationReport report =
		BuildDefaultReport();

	Expect(!report.verified(), "geometry mismatch report should fail");
	Expect(
		report.verification.status == NativeStaticMeshExportDirectoryVerificationStatus::GeometryMismatch,
		"geometry mismatch report should expose status");
	Expect(
		report.text.find("asset=cube filename=cube.igmesh status=GeometryMismatch") != std::string::npos,
		"geometry mismatch report should include entry status");
	CleanupTempRoot();
}

void TestExtraUnrelatedFileIsIgnored()
{
	ResetTempRoot();
	ExportDefaultBatch();
	WriteText(TempRoot() / "extra.txt", "ignored");

	const NativeStaticMeshExportDirectoryVerificationReport report =
		BuildDefaultReport();

	Expect(report.verified(), "extra unrelated file should not fail report");
	Expect(report.verification.verifiedCount == 3, "extra unrelated file should not change verified count");
	CleanupTempRoot();
}

} // namespace

int main()
{
	TestBatchExportedDirectoryReportSucceeds();
	TestMissingOutputDirectoryReportFails();
	TestFilePathInsteadOfDirectoryReportFails();
	TestMissingManifestReportFails();
	TestManifestMismatchReportFails();
	TestMissingPackageManifestReportFails();
	TestMalformedPackageManifestReportFails();
	TestPackageManifestMismatchReportFails();
	TestMissingAssetReportFailsWithEntry();
	TestCorruptAssetReportFailsWithIssueCount();
	TestGeometryMismatchReportFails();
	TestExtraUnrelatedFileIsIgnored();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
