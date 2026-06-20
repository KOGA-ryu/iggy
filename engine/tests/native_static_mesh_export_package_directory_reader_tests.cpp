#include "../apps/native_play/NativeStaticMeshExportManifest.hpp"
#include "../apps/native_play/NativeStaticMeshExportPackageDirectoryLoad.hpp"
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
using iggy::native_play::NativeStaticMeshExportPackageDirectoryLoadedAssetStatus;
using iggy::native_play::NativeStaticMeshExportPackageDirectoryLoadResult;
using iggy::native_play::NativeStaticMeshExportPackageDirectoryLoadStatus;
using iggy::native_play::NativeStaticMeshExportPackageDirectoryManifestComparisonCode;
using iggy::native_play::NativeStaticMeshExportPackageDirectoryReadResult;
using iggy::native_play::NativeStaticMeshExportPackageDirectoryReadStatus;
using iggy::native_play::NativeStaticMeshExportPackageDirectoryReadStatusText;
using iggy::native_play::NativeStaticMeshExportPackageManifestReadIssueCode;
using iggy::native_play::NativeStaticMeshExportPackageManifestSidecarFilename;
using iggy::native_play::NativeStaticMeshAssetLoadIssueCode;
using iggy::native_play::NativeStaticMeshFileExportBatchResult;
using iggy::native_play::NativeStaticMeshFileExportStatus;
using iggy::native_play::LoadNativeStaticMeshExportPackageDirectory;
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

bool LoadedAssetHasIssue(
	const NativeStaticMeshExportPackageDirectoryLoadResult &result,
	std::size_t assetIndex,
	NativeStaticMeshAssetLoadIssueCode code)
{
	if (assetIndex >= result.assets.size()) {
		return false;
	}
	for (const auto &issue : result.assets[assetIndex].loadIssues) {
		if (issue.code == code) {
			return true;
		}
	}
	return false;
}

void WriteValidMismatchedNestedManifest()
{
	WriteText(
		TempRoot() / NativeStaticMeshExportManifestFilename,
		"static-mesh-export-manifest version=1 assets=3 bytes=33879\n"
		"asset=cube filename=cube-renamed.igmesh vertices=8 indices=36 bytes=523\n"
		"asset=bean filename=bean.igmesh vertices=234 indices=1296 bytes=23882\n"
		"asset=npc-marker filename=npc-marker.igmesh vertices=98 indices=504 bytes=9474\n");
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

void TestBatchExportedDirectoryLoads()
{
	ResetTempRoot();
	ExportDefaultBatch();

	const NativeStaticMeshExportPackageDirectoryLoadResult result =
		LoadNativeStaticMeshExportPackageDirectory(TempRoot());

	Expect(result.loaded(), "batch-exported package directory should load");
	Expect(
		result.status == NativeStaticMeshExportPackageDirectoryLoadStatus::Loaded,
		"loaded package should expose loaded status");
	Expect(result.directory == TempRoot(), "load result should preserve supplied directory");
	Expect(result.read.read(), "load result should retain successful package read");
	Expect(result.manifestRead.read(), "load result should retain successful manifest read");
	Expect(result.manifestComparison.matchCount == 3, "load result should compare three manifest rows");
	Expect(
		result.manifestComparison.comparisons.empty(),
		"loaded package should not expose manifest comparison mismatches");
	Expect(result.loadedCount == 3, "loaded package should count three loaded assets");
	Expect(result.issueCount == 0, "loaded package should have no load issues");
	Expect(result.assets.size() == 3, "loaded package should expose three loaded rows");
	if (result.assets.size() == 3) {
		Expect(result.assets[0].loaded(), "cube loaded row should be loaded");
		Expect(result.assets[0].packageAsset.name == "cube", "first loaded asset should be cube");
		Expect(result.assets[0].expectedVertexCount == 8, "cube expected vertex count should come from manifest");
		Expect(result.assets[0].expectedIndexCount == 36, "cube expected index count should come from manifest");
		Expect(result.assets[0].actualVertexCount == 8, "cube actual vertex count should come from asset");
		Expect(result.assets[0].actualIndexCount == 36, "cube actual index count should come from asset");
		Expect(result.assets[0].asset.vertices.size() == 8, "cube asset should be retained in memory");
		Expect(result.assets[1].packageAsset.name == "bean", "second loaded asset should be bean");
		Expect(result.assets[1].actualVertexCount == 234, "bean actual vertex count should match fixture");
		Expect(result.assets[2].packageAsset.name == "npc-marker", "third loaded asset should be NPC marker");
		Expect(result.assets[2].actualIndexCount == 504, "NPC marker actual index count should match fixture");
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

void TestMissingPackageSidecarLoadFails()
{
	ResetTempRoot();
	ExportDefaultBatch();
	std::filesystem::remove(TempRoot() / NativeStaticMeshExportPackageManifestSidecarFilename);

	const NativeStaticMeshExportPackageDirectoryLoadResult result =
		LoadNativeStaticMeshExportPackageDirectory(TempRoot());

	Expect(!result.loaded(), "missing package sidecar should not load");
	Expect(
		result.status ==
			NativeStaticMeshExportPackageDirectoryLoadStatus::PackageDirectoryReadFailed,
		"missing package sidecar load should expose package read failure");
	Expect(
		result.read.status ==
			NativeStaticMeshExportPackageDirectoryReadStatus::PackageManifestReadFailed,
		"missing package sidecar load should retain package reader status");
	Expect(result.issueCount == 1, "missing package sidecar load should retain reader issue count");
	Expect(result.assets.empty(), "missing package sidecar load should not attempt asset loads");
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

void TestMissingNestedManifestLoadFails()
{
	ResetTempRoot();
	ExportDefaultBatch();
	std::filesystem::remove(TempRoot() / NativeStaticMeshExportManifestFilename);

	const NativeStaticMeshExportPackageDirectoryLoadResult result =
		LoadNativeStaticMeshExportPackageDirectory(TempRoot());

	Expect(!result.loaded(), "missing nested mesh manifest should not load package");
	Expect(
		result.status == NativeStaticMeshExportPackageDirectoryLoadStatus::ManifestReadFailed,
		"missing nested mesh manifest should expose manifest read failure");
	Expect(result.read.read(), "missing nested mesh manifest load should retain package read");
	Expect(!result.manifestRead.read(), "missing nested mesh manifest load should retain failed manifest read");
	Expect(result.issueCount == 1, "missing nested mesh manifest should expose manifest issue count");
	Expect(result.assets.empty(), "missing nested mesh manifest should block asset loads");
	CleanupTempRoot();
}

void TestMalformedNestedManifestLoadFails()
{
	ResetTempRoot();
	ExportDefaultBatch();
	WriteText(
		TempRoot() / NativeStaticMeshExportManifestFilename,
		"static-mesh-export-manifest version=2 assets=0 bytes=0\n");

	const NativeStaticMeshExportPackageDirectoryLoadResult result =
		LoadNativeStaticMeshExportPackageDirectory(TempRoot());

	Expect(!result.loaded(), "malformed nested mesh manifest should not load package");
	Expect(
		result.status == NativeStaticMeshExportPackageDirectoryLoadStatus::ManifestReadFailed,
		"malformed nested mesh manifest should expose manifest read failure");
	Expect(result.issueCount > 0, "malformed nested mesh manifest should expose read issues");
	Expect(result.assets.empty(), "malformed nested mesh manifest should block asset loads");
	CleanupTempRoot();
}

void TestManifestMismatchLoadFailsBeforeAssets()
{
	ResetTempRoot();
	ExportDefaultBatch();
	WriteValidMismatchedNestedManifest();

	const NativeStaticMeshExportPackageDirectoryLoadResult result =
		LoadNativeStaticMeshExportPackageDirectory(TempRoot());

	Expect(!result.loaded(), "manifest mismatch should not load package");
	Expect(
		result.status == NativeStaticMeshExportPackageDirectoryLoadStatus::ManifestComparisonFailed,
		"manifest mismatch should expose comparison failure");
	Expect(result.manifestRead.read(), "manifest mismatch should retain valid manifest read");
	Expect(result.manifestComparison.comparisons.size() == 1, "manifest mismatch should expose one comparison issue");
	if (!result.manifestComparison.comparisons.empty()) {
		Expect(
			result.manifestComparison.comparisons[0].code ==
				NativeStaticMeshExportPackageDirectoryManifestComparisonCode::FilenameMismatch,
			"manifest mismatch should expose filename mismatch");
	}
	Expect(result.issueCount == result.manifestComparison.comparisons.size(), "manifest mismatch issue count should match comparisons");
	Expect(result.assets.empty(), "manifest mismatch should block asset loads");
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

void TestMissingDeclaredAssetLoadFails()
{
	ResetTempRoot();
	ExportDefaultBatch();
	std::filesystem::remove(TempRoot() / "cube.igmesh");

	const NativeStaticMeshExportPackageDirectoryLoadResult result =
		LoadNativeStaticMeshExportPackageDirectory(TempRoot());

	Expect(!result.loaded(), "missing declared asset should not load package");
	Expect(
		result.status == NativeStaticMeshExportPackageDirectoryLoadStatus::MissingAsset,
		"missing declared asset should expose missing-asset status");
	Expect(result.assets.size() == 3, "missing declared asset should still expose package rows");
	Expect(result.loadedCount == 2, "missing one asset should still count later successful loads");
	Expect(result.issueCount == 1, "missing one asset should expose one issue");
	if (!result.assets.empty()) {
		Expect(
			result.assets[0].status ==
				NativeStaticMeshExportPackageDirectoryLoadedAssetStatus::MissingAsset,
			"missing cube row should expose missing-asset status");
		Expect(
			LoadedAssetHasIssue(
				result,
				0,
				NativeStaticMeshAssetLoadIssueCode::FileOpenFailed),
			"missing cube row should retain file-open issue");
	}
	CleanupTempRoot();
}

void TestCorruptDeclaredAssetLoadFails()
{
	ResetTempRoot();
	ExportDefaultBatch();
	WriteText(TempRoot() / "cube.igmesh", "not mesh\n");

	const NativeStaticMeshExportPackageDirectoryLoadResult result =
		LoadNativeStaticMeshExportPackageDirectory(TempRoot());

	Expect(!result.loaded(), "corrupt declared asset should not load package");
	Expect(
		result.status == NativeStaticMeshExportPackageDirectoryLoadStatus::AssetLoadFailed,
		"corrupt declared asset should expose asset-load-failed status");
	Expect(result.assets.size() == 3, "corrupt declared asset should still expose package rows");
	Expect(result.loadedCount == 2, "corrupt one asset should still count later successful loads");
	Expect(result.issueCount == 1, "corrupt one asset should expose one issue");
	if (!result.assets.empty()) {
		Expect(
			result.assets[0].status ==
				NativeStaticMeshExportPackageDirectoryLoadedAssetStatus::AssetLoadFailed,
			"corrupt cube row should expose asset-load-failed status");
		Expect(
			LoadedAssetHasIssue(
				result,
				0,
				NativeStaticMeshAssetLoadIssueCode::UnknownDirective),
			"corrupt cube row should retain parser issue");
	}
	CleanupTempRoot();
}

void TestGeometryMismatchLoadFails()
{
	ResetTempRoot();
	ExportDefaultBatch();
	std::filesystem::copy_file(
		TempRoot() / "bean.igmesh",
		TempRoot() / "cube.igmesh",
		std::filesystem::copy_options::overwrite_existing);

	const NativeStaticMeshExportPackageDirectoryLoadResult result =
		LoadNativeStaticMeshExportPackageDirectory(TempRoot());

	Expect(!result.loaded(), "geometry mismatch should not load package");
	Expect(
		result.status == NativeStaticMeshExportPackageDirectoryLoadStatus::GeometryMismatch,
		"geometry mismatch should expose geometry-mismatch status");
	Expect(result.assets.size() == 3, "geometry mismatch should still expose package rows");
	Expect(result.loadedCount == 2, "geometry mismatch should count only matching assets as loaded");
	Expect(result.issueCount == 1, "one geometry mismatch should expose one issue");
	if (!result.assets.empty()) {
		Expect(
			result.assets[0].status ==
				NativeStaticMeshExportPackageDirectoryLoadedAssetStatus::GeometryMismatch,
			"cube row should expose geometry mismatch");
		Expect(result.assets[0].expectedVertexCount == 8, "cube expected vertices should remain manifest count");
		Expect(result.assets[0].actualVertexCount == 234, "cube actual vertices should reflect bean file");
		Expect(result.assets[0].expectedIndexCount == 36, "cube expected indices should remain manifest count");
		Expect(result.assets[0].actualIndexCount == 1296, "cube actual indices should reflect bean file");
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

void TestReadStatusText()
{
	struct Case {
		NativeStaticMeshExportPackageDirectoryReadStatus status;
		const char *text;
	};

	const Case cases[] = {
		{ NativeStaticMeshExportPackageDirectoryReadStatus::Read, "Read" },
		{ NativeStaticMeshExportPackageDirectoryReadStatus::MissingDirectory, "MissingDirectory" },
		{ NativeStaticMeshExportPackageDirectoryReadStatus::DirectoryNotDirectory, "DirectoryNotDirectory" },
		{ NativeStaticMeshExportPackageDirectoryReadStatus::PackageManifestReadFailed, "PackageManifestReadFailed" },
	};

	for (const Case &testCase : cases) {
		Expect(
			std::string(NativeStaticMeshExportPackageDirectoryReadStatusText(
				testCase.status)) == testCase.text,
			"package directory read status text should match stable spelling");
	}
	Expect(
		std::string(NativeStaticMeshExportPackageDirectoryReadStatusText(
			static_cast<NativeStaticMeshExportPackageDirectoryReadStatus>(999))) ==
			"Unknown",
		"package directory read status text should report unknown fallback");
}

} // namespace

int main()
{
	TestBatchExportedDirectoryReads();
	TestBatchExportedDirectoryLoads();
	TestMissingDirectoryFailsWithoutCreatingIt();
	TestFilePathInsteadOfDirectoryFails();
	TestMissingPackageSidecarFailsWithFileOpenIssue();
	TestMissingPackageSidecarLoadFails();
	TestMalformedPackageSidecarFailsWithTextIssue();
	TestMissingNestedManifestLoadFails();
	TestMalformedNestedManifestLoadFails();
	TestManifestMismatchLoadFailsBeforeAssets();
	TestMissingNestedManifestDoesNotFailReader();
	TestMissingDeclaredAssetDoesNotFailReader();
	TestMissingDeclaredAssetLoadFails();
	TestCorruptDeclaredAssetLoadFails();
	TestGeometryMismatchLoadFails();
	TestExtraUnrelatedFilesAreIgnored();
	TestReadStatusText();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
