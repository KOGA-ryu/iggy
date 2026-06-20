#include "../apps/native_play/NativeStaticMeshExportManifest.hpp"
#include "../apps/native_play/NativeStaticMeshExportPackageDirectoryReport.hpp"
#include "../apps/native_play/NativeStaticMeshFileExport.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include "support/TestHarness.hpp"

namespace {

using iggy::native_play::BuildNativeStaticMeshExportPackageDirectoryReport;
using iggy::native_play::DefaultNativeStaticMeshExportPolicy;
using iggy::native_play::ExportNativeStaticMeshPolicyToDirectory;
using iggy::native_play::NativeStaticMeshExportManifestFilename;
using iggy::native_play::NativeStaticMeshExportPackageDirectoryReadStatus;
using iggy::native_play::NativeStaticMeshExportPackageDirectoryReport;
using iggy::native_play::NativeStaticMeshExportPackageManifestSidecarFilename;
using iggy::native_play::NativeStaticMeshFileExportBatchResult;
using iggy::native_play::NativeStaticMeshFileExportStatus;
using iggy::test::Expect;
using iggy::test::Failures;

std::filesystem::path TempRoot()
{
	return std::filesystem::temp_directory_path() /
		"iggy_native_static_mesh_export_package_directory_report_tests";
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

NativeStaticMeshExportPackageDirectoryReport BuildDefaultReport()
{
	return BuildNativeStaticMeshExportPackageDirectoryReport(TempRoot());
}

void TestBatchExportedDirectoryReportReads()
{
	ResetTempRoot();
	ExportDefaultBatch();

	const NativeStaticMeshExportPackageDirectoryReport report = BuildDefaultReport();

	Expect(report.readOk(), "batch-exported package directory report should read");
	Expect(
		report.read.status == NativeStaticMeshExportPackageDirectoryReadStatus::Read,
		"batch-exported package directory report should expose read status");
	Expect(
		report.text.find(
			"static-mesh-export-package-directory-report status=Read") !=
			std::string::npos,
		"package directory report should include read summary");
	Expect(
		report.text.find("directory=" + TempRoot().string()) != std::string::npos,
		"package directory report should include directory path");
	Expect(
		report.text.find("assets=3 issues=0") != std::string::npos,
		"package directory report should include asset and issue counts");
	Expect(
		report.text.find(
			"packageManifest=" +
			(TempRoot() / NativeStaticMeshExportPackageManifestSidecarFilename).string()) !=
			std::string::npos,
		"package directory report should include package manifest path");
	Expect(
		report.text.find(
			"manifest=" + (TempRoot() / NativeStaticMeshExportManifestFilename).string()) !=
			std::string::npos,
		"package directory report should include nested manifest path");
	Expect(
		report.text.find("manifestExists=1") != std::string::npos,
		"package directory report should include existing nested manifest diagnostic");
	Expect(
		report.text.find(
			"asset=cube filename=cube.igmesh path=" +
			(TempRoot() / "cube.igmesh").string() + " exists=1") != std::string::npos,
		"package directory report should include cube asset row");
	Expect(
		report.text.find(
			"asset=bean filename=bean.igmesh path=" +
			(TempRoot() / "bean.igmesh").string() + " exists=1") != std::string::npos,
		"package directory report should include bean asset row");
	Expect(
		report.text.find(
			"asset=npc-marker filename=npc-marker.igmesh path=" +
			(TempRoot() / "npc-marker.igmesh").string() + " exists=1") != std::string::npos,
		"package directory report should include NPC marker asset row");
	Expect(
		report.text.find("packageManifestReadIssue") == std::string::npos,
		"successful package directory report should not include read issue rows");
	CleanupTempRoot();
}

void TestMissingDirectoryReportFails()
{
	CleanupTempRoot();
	const std::filesystem::path missing = TempRoot() / "missing";

	const NativeStaticMeshExportPackageDirectoryReport report =
		BuildNativeStaticMeshExportPackageDirectoryReport(missing);

	Expect(!report.readOk(), "missing package directory report should fail");
	Expect(
		report.read.status == NativeStaticMeshExportPackageDirectoryReadStatus::MissingDirectory,
		"missing package directory report should expose missing-directory status");
	Expect(
		report.text ==
			"static-mesh-export-package-directory-report status=MissingDirectory directory=" +
				missing.string() + " assets=0 issues=0\n",
		"missing package directory report should include summary only");
	Expect(
		!std::filesystem::exists(TempRoot()),
		"package directory report should not create parent directories");
}

void TestFilePathInsteadOfDirectoryReportFails()
{
	ResetTempRoot();
	const std::filesystem::path filePath = TempRoot() / "not-a-directory";
	WriteText(filePath, "not a directory");

	const NativeStaticMeshExportPackageDirectoryReport report =
		BuildNativeStaticMeshExportPackageDirectoryReport(filePath);

	Expect(!report.readOk(), "file package directory report should fail");
	Expect(
		report.read.status == NativeStaticMeshExportPackageDirectoryReadStatus::DirectoryNotDirectory,
		"file package directory report should expose not-directory status");
	Expect(
		report.text ==
			"static-mesh-export-package-directory-report status=DirectoryNotDirectory directory=" +
				filePath.string() + " assets=0 issues=0\n",
		"file package directory report should include summary only");
	CleanupTempRoot();
}

void TestMissingPackageSidecarReportIncludesIssueRow()
{
	ResetTempRoot();
	ExportDefaultBatch();
	const std::filesystem::path packageManifest =
		TempRoot() / NativeStaticMeshExportPackageManifestSidecarFilename;
	std::filesystem::remove(packageManifest);

	const NativeStaticMeshExportPackageDirectoryReport report = BuildDefaultReport();

	Expect(!report.readOk(), "missing package sidecar report should fail");
	Expect(
		report.read.status == NativeStaticMeshExportPackageDirectoryReadStatus::PackageManifestReadFailed,
		"missing package sidecar report should expose read-failed status");
	Expect(
		report.text.find("status=PackageManifestReadFailed") != std::string::npos,
		"missing package sidecar report should include status");
	Expect(
		report.text.find("assets=0 issues=1") != std::string::npos,
		"missing package sidecar report should include issue count");
	Expect(
		report.text.find("packageManifest=" + packageManifest.string()) != std::string::npos,
		"missing package sidecar report should include package manifest path");
	Expect(
		report.text.find(
			"packageManifestReadIssue code=FileOpenFailed line=0 token=" +
			packageManifest.string()) != std::string::npos,
		"missing package sidecar report should include file-open issue row");
	Expect(
		report.text.find("asset=cube") == std::string::npos,
		"missing package sidecar report should not include asset rows");
	CleanupTempRoot();
}

void TestMalformedPackageSidecarReportIncludesIssueRow()
{
	ResetTempRoot();
	ExportDefaultBatch();
	const std::filesystem::path packageManifest =
		TempRoot() / NativeStaticMeshExportPackageManifestSidecarFilename;
	WriteText(packageManifest, "static-mesh-export-package format=bad\n");

	const NativeStaticMeshExportPackageDirectoryReport report = BuildDefaultReport();

	Expect(!report.readOk(), "malformed package sidecar report should fail");
	Expect(
		report.read.status == NativeStaticMeshExportPackageDirectoryReadStatus::PackageManifestReadFailed,
		"malformed package sidecar report should expose read-failed status");
	Expect(
		report.text.find("packageManifest=" + packageManifest.string()) != std::string::npos,
		"malformed package sidecar report should include package manifest path");
	Expect(
		report.text.find(
			"packageManifestReadIssue code=MalformedHeader line=1 token=static-mesh-export-package") !=
			std::string::npos,
		"malformed package sidecar report should include parser issue row");
	Expect(
		report.text.find("asset=cube") == std::string::npos,
		"malformed package sidecar report should not include asset rows");
	CleanupTempRoot();
}

void TestMissingNestedManifestStillReportsRead()
{
	ResetTempRoot();
	ExportDefaultBatch();
	std::filesystem::remove(TempRoot() / NativeStaticMeshExportManifestFilename);

	const NativeStaticMeshExportPackageDirectoryReport report = BuildDefaultReport();

	Expect(report.readOk(), "missing nested mesh manifest should not fail package report");
	Expect(
		report.text.find("status=Read") != std::string::npos,
		"missing nested mesh manifest package report should stay read");
	Expect(
		report.text.find(
			"manifest=" + (TempRoot() / NativeStaticMeshExportManifestFilename).string()) !=
			std::string::npos,
		"missing nested mesh manifest path should still be reported");
	Expect(
		report.text.find("manifestExists=0") != std::string::npos,
		"missing nested mesh manifest should report missing presence diagnostic");
	Expect(
		report.text.find("asset=cube filename=cube.igmesh") != std::string::npos,
		"missing nested mesh manifest should not suppress asset rows");
	CleanupTempRoot();
}

void TestMissingDeclaredAssetStillReportsRead()
{
	ResetTempRoot();
	ExportDefaultBatch();
	std::filesystem::remove(TempRoot() / "cube.igmesh");

	const NativeStaticMeshExportPackageDirectoryReport report = BuildDefaultReport();

	Expect(report.readOk(), "missing declared mesh asset should not fail package report");
	Expect(
		report.text.find("status=Read") != std::string::npos,
		"missing declared mesh asset package report should stay read");
	Expect(
		report.text.find(
			"asset=cube filename=cube.igmesh path=" +
			(TempRoot() / "cube.igmesh").string() + " exists=0") != std::string::npos,
		"missing declared mesh asset path should report missing presence diagnostic");
	Expect(
		report.text.find(
			"asset=bean filename=bean.igmesh path=" +
			(TempRoot() / "bean.igmesh").string() + " exists=1") != std::string::npos,
		"present declared mesh asset path should report existing presence diagnostic");
	CleanupTempRoot();
}

void TestExtraUnrelatedFileIsIgnored()
{
	ResetTempRoot();
	ExportDefaultBatch();
	WriteText(TempRoot() / "unrelated.txt", "ignored");

	const NativeStaticMeshExportPackageDirectoryReport report = BuildDefaultReport();

	Expect(report.readOk(), "extra unrelated file should not fail package report");
	Expect(
		report.text.find("unrelated.txt") == std::string::npos,
		"extra unrelated file should not appear in package report");
	Expect(
		report.text.find("assets=3 issues=0") != std::string::npos,
		"extra unrelated file should not affect package report counts");
	CleanupTempRoot();
}

} // namespace

int main()
{
	TestBatchExportedDirectoryReportReads();
	TestMissingDirectoryReportFails();
	TestFilePathInsteadOfDirectoryReportFails();
	TestMissingPackageSidecarReportIncludesIssueRow();
	TestMalformedPackageSidecarReportIncludesIssueRow();
	TestMissingNestedManifestStillReportsRead();
	TestMissingDeclaredAssetStillReportsRead();
	TestExtraUnrelatedFileIsIgnored();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
