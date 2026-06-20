#include "../apps/native_play/NativeStaticMeshExportManifest.hpp"
#include "../apps/native_play/NativeStaticMeshExportPackageDirectoryReader.hpp"
#include "../apps/native_play/NativeStaticMeshFileExport.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include "support/TestHarness.hpp"

namespace {

using iggy::native_play::DefaultNativeStaticMeshExportPolicy;
using iggy::native_play::ExportNativeStaticMeshPolicyToDirectory;
using iggy::native_play::NativeStaticMeshExportManifestFilename;
using iggy::native_play::NativeStaticMeshExportPackageDirectoryReadResult;
using iggy::native_play::NativeStaticMeshExportPackageDirectoryReadStatus;
using iggy::native_play::NativeStaticMeshExportPackageManifestReadIssueCode;
using iggy::native_play::NativeStaticMeshExportPackageManifestSidecarFilename;
using iggy::native_play::NativeStaticMeshFileExportBatchResult;
using iggy::native_play::NativeStaticMeshFileExportStatus;
using iggy::native_play::ReadNativeStaticMeshExportPackageDirectory;
using iggy::test::Expect;
using iggy::test::Failures;

std::filesystem::path TempRoot()
{
	return std::filesystem::temp_directory_path() /
		"iggy_native_static_mesh_export_package_directory_reader_tests";
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

bool HasPackageManifestReadIssue(
	const NativeStaticMeshExportPackageDirectoryReadResult &result,
	NativeStaticMeshExportPackageManifestReadIssueCode code)
{
	for (const auto &issue : result.packageManifestReadIssues) {
		if (issue.code == code)
			return true;
	}
	return false;
}

void TestBatchExportedDirectoryReads()
{
	ResetTempRoot();
	ExportDefaultBatch();

	const NativeStaticMeshExportPackageDirectoryReadResult result =
		ReadNativeStaticMeshExportPackageDirectory(TempRoot());

	Expect(result.read(), "batch-exported package directory should read");
	Expect(
		result.status == NativeStaticMeshExportPackageDirectoryReadStatus::Read,
		"batch-exported package directory should report read status");
	Expect(result.directory == TempRoot(), "reader should preserve supplied directory");
	Expect(
		result.packageManifestPath ==
			TempRoot() / NativeStaticMeshExportPackageManifestSidecarFilename,
		"reader should project package manifest path");
	Expect(
		result.manifestPath == TempRoot() / NativeStaticMeshExportManifestFilename,
		"reader should project nested mesh manifest path");
	Expect(result.issueCount == 0, "successful package directory read should have no issues");
	Expect(result.packageManifestReadIssues.empty(), "successful read should have no manifest issues");
	Expect(result.document.assets.size() == 3, "reader should parse three package asset rows");
	Expect(result.assets.size() == 3, "reader should project three package assets");
	if (result.assets.size() == 3) {
		Expect(result.assets[0].name == "cube", "first package asset should be cube");
		Expect(
			result.assets[0].filename == "cube.igmesh",
			"first package asset filename should be cube.igmesh");
		Expect(
			result.assets[0].path == TempRoot() / "cube.igmesh",
			"first package asset path should be projected from directory");
		Expect(result.assets[1].name == "bean", "second package asset should be bean");
		Expect(
			result.assets[1].path == TempRoot() / "bean.igmesh",
			"second package asset path should be projected from directory");
		Expect(
			result.assets[2].name == "npc-marker",
			"third package asset should be NPC marker");
		Expect(
			result.assets[2].path == TempRoot() / "npc-marker.igmesh",
			"third package asset path should be projected from directory");
	}

	CleanupTempRoot();
}

void TestMissingDirectoryFailsWithoutCreatingIt()
{
	CleanupTempRoot();
	const std::filesystem::path missing = TempRoot() / "missing";

	const NativeStaticMeshExportPackageDirectoryReadResult result =
		ReadNativeStaticMeshExportPackageDirectory(missing);

	Expect(!result.read(), "missing package directory should not read");
	Expect(
		result.status == NativeStaticMeshExportPackageDirectoryReadStatus::MissingDirectory,
		"missing package directory should report missing-directory status");
	Expect(result.directory == missing, "missing package directory should preserve path");
	Expect(
		!std::filesystem::exists(TempRoot()),
		"package directory reader should not create parent directories");
}

void TestFilePathInsteadOfDirectoryFails()
{
	ResetTempRoot();
	const std::filesystem::path filePath = TempRoot() / "not-a-directory";
	WriteText(filePath, "not a directory");

	const NativeStaticMeshExportPackageDirectoryReadResult result =
		ReadNativeStaticMeshExportPackageDirectory(filePath);

	Expect(!result.read(), "file package directory path should not read");
	Expect(
		result.status == NativeStaticMeshExportPackageDirectoryReadStatus::DirectoryNotDirectory,
		"file package directory path should report not-directory status");
	Expect(result.directory == filePath, "file package directory should preserve path");
	CleanupTempRoot();
}

void TestMissingPackageSidecarFailsWithFileOpenIssue()
{
	ResetTempRoot();
	ExportDefaultBatch();
	const std::filesystem::path packageManifest =
		TempRoot() / NativeStaticMeshExportPackageManifestSidecarFilename;
	std::filesystem::remove(packageManifest);

	const NativeStaticMeshExportPackageDirectoryReadResult result =
		ReadNativeStaticMeshExportPackageDirectory(TempRoot());

	Expect(!result.read(), "missing package manifest sidecar should not read");
	Expect(
		result.status == NativeStaticMeshExportPackageDirectoryReadStatus::PackageManifestReadFailed,
		"missing package manifest sidecar should report read-failed status");
	Expect(
		result.packageManifestPath == packageManifest,
		"missing package manifest sidecar should report package manifest path");
	Expect(result.issueCount == 1, "missing package manifest sidecar should report one issue");
	Expect(
		HasPackageManifestReadIssue(
			result,
			NativeStaticMeshExportPackageManifestReadIssueCode::FileOpenFailed),
		"missing package manifest sidecar should report file-open failure");
	Expect(result.manifestPath.empty(), "missing package manifest should not project mesh manifest path");
	Expect(result.assets.empty(), "missing package manifest should not project assets");
	CleanupTempRoot();
}

void TestMalformedPackageSidecarFailsWithTextIssue()
{
	ResetTempRoot();
	ExportDefaultBatch();
	const std::filesystem::path packageManifest =
		TempRoot() / NativeStaticMeshExportPackageManifestSidecarFilename;
	WriteText(packageManifest, "static-mesh-export-package format=bad\n");

	const NativeStaticMeshExportPackageDirectoryReadResult result =
		ReadNativeStaticMeshExportPackageDirectory(TempRoot());

	Expect(!result.read(), "malformed package manifest sidecar should not read");
	Expect(
		result.status == NativeStaticMeshExportPackageDirectoryReadStatus::PackageManifestReadFailed,
		"malformed package manifest sidecar should report read-failed status");
	Expect(result.issueCount > 0, "malformed package manifest sidecar should report issues");
	Expect(
		HasPackageManifestReadIssue(
			result,
			NativeStaticMeshExportPackageManifestReadIssueCode::MalformedHeader),
		"malformed package manifest sidecar should report text-reader issue");
	Expect(result.manifestPath.empty(), "malformed package manifest should not project mesh manifest path");
	Expect(result.assets.empty(), "malformed package manifest should not project assets");
	CleanupTempRoot();
}

void TestMissingNestedManifestDoesNotFailReader()
{
	ResetTempRoot();
	ExportDefaultBatch();
	std::filesystem::remove(TempRoot() / NativeStaticMeshExportManifestFilename);

	const NativeStaticMeshExportPackageDirectoryReadResult result =
		ReadNativeStaticMeshExportPackageDirectory(TempRoot());

	Expect(result.read(), "missing nested mesh manifest should not fail package reader");
	Expect(
		result.manifestPath == TempRoot() / NativeStaticMeshExportManifestFilename,
		"missing nested mesh manifest should still be projected");
	Expect(result.assets.size() == 3, "missing nested mesh manifest should not block assets");
	CleanupTempRoot();
}

void TestMissingDeclaredAssetDoesNotFailReader()
{
	ResetTempRoot();
	ExportDefaultBatch();
	std::filesystem::remove(TempRoot() / "cube.igmesh");

	const NativeStaticMeshExportPackageDirectoryReadResult result =
		ReadNativeStaticMeshExportPackageDirectory(TempRoot());

	Expect(result.read(), "missing declared asset should not fail package reader");
	Expect(result.assets.size() == 3, "missing declared asset should not change projected rows");
	if (!result.assets.empty()) {
		Expect(
			result.assets[0].path == TempRoot() / "cube.igmesh",
			"missing declared asset path should still be projected");
	}
	CleanupTempRoot();
}

void TestExtraUnrelatedFilesAreIgnored()
{
	ResetTempRoot();
	ExportDefaultBatch();
	WriteText(TempRoot() / "unrelated.txt", "ignored");

	const NativeStaticMeshExportPackageDirectoryReadResult result =
		ReadNativeStaticMeshExportPackageDirectory(TempRoot());

	Expect(result.read(), "extra unrelated files should not fail package reader");
	Expect(result.assets.size() == 3, "extra unrelated files should not affect projected rows");
	CleanupTempRoot();
}

} // namespace

int main()
{
	TestBatchExportedDirectoryReads();
	TestMissingDirectoryFailsWithoutCreatingIt();
	TestFilePathInsteadOfDirectoryFails();
	TestMissingPackageSidecarFailsWithFileOpenIssue();
	TestMalformedPackageSidecarFailsWithTextIssue();
	TestMissingNestedManifestDoesNotFailReader();
	TestMissingDeclaredAssetDoesNotFailReader();
	TestExtraUnrelatedFilesAreIgnored();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
