#include "../apps/native_play/NativeStaticMeshExportManifest.hpp"
#include "../apps/native_play/NativeStaticMeshExportPackageDirectoryReport.hpp"
#include "../apps/native_play/NativeStaticMeshFileExport.hpp"

#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include "support/TestHarness.hpp"

namespace {

using iggy::native_play::BuildNativeStaticMeshExportPackageDirectoryReport;
using iggy::native_play::DefaultNativeStaticMeshExportPolicy;
using iggy::native_play::ExportNativeStaticMeshPolicyToDirectory;
using iggy::native_play::NativeStaticMeshExportManifestFilename;
using iggy::native_play::NativeStaticMeshExportPackageDirectoryAssetFacts;
using iggy::native_play::NativeStaticMeshExportPackageDirectoryManifestComparisonCode;
using iggy::native_play::NativeStaticMeshExportPackageDirectoryPathFacts;
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

std::uintmax_t FileByteCount(const std::filesystem::path &path)
{
	std::error_code error;
	const std::uintmax_t byteCount = std::filesystem::file_size(path, error);
	return error ? 0 : byteCount;
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

const NativeStaticMeshExportPackageDirectoryAssetFacts *FindAssetFacts(
	const NativeStaticMeshExportPackageDirectoryReport &report,
	const std::string &name)
{
	for (const NativeStaticMeshExportPackageDirectoryAssetFacts &facts :
			report.assetFacts) {
		if (facts.asset.name == name) {
			return &facts;
		}
	}
	return nullptr;
}

void ExpectFacts(
	const NativeStaticMeshExportPackageDirectoryPathFacts &facts,
	bool exists,
	bool regularFile,
	std::uintmax_t byteCount,
	const char *message)
{
	Expect(facts.exists == exists, message);
	Expect(facts.regularFile == regularFile, message);
	Expect(facts.byteCount == byteCount, message);
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
			(TempRoot() / NativeStaticMeshExportPackageManifestSidecarFilename).string() +
				" packageManifestExists=1 packageManifestRegularFile=1 packageManifestBytes=" +
				std::to_string(FileByteCount(
					TempRoot() / NativeStaticMeshExportPackageManifestSidecarFilename))) !=
			std::string::npos,
		"package directory report should include package manifest file facts");
	Expect(
		report.text.find(
			"manifest=" + (TempRoot() / NativeStaticMeshExportManifestFilename).string() +
				" manifestExists=1 manifestRegularFile=1 manifestBytes=" +
				std::to_string(FileByteCount(
					TempRoot() / NativeStaticMeshExportManifestFilename)) +
				" manifestRead=ok manifestReadIssues=0 manifestMatches=3 manifestMismatches=0 manifestComparisonIssues=0") !=
			std::string::npos,
		"package directory report should include nested manifest file and read facts");
	Expect(
		report.text.find(
			"asset=cube filename=cube.igmesh path=" +
			(TempRoot() / "cube.igmesh").string() +
				" exists=1 regularFile=1 bytes=" +
				std::to_string(FileByteCount(TempRoot() / "cube.igmesh"))) != std::string::npos,
		"package directory report should include cube asset row");
	Expect(
		report.text.find(
			"asset=bean filename=bean.igmesh path=" +
			(TempRoot() / "bean.igmesh").string() +
				" exists=1 regularFile=1 bytes=" +
				std::to_string(FileByteCount(TempRoot() / "bean.igmesh"))) != std::string::npos,
		"package directory report should include bean asset row");
	Expect(
		report.text.find(
			"asset=npc-marker filename=npc-marker.igmesh path=" +
			(TempRoot() / "npc-marker.igmesh").string() +
				" exists=1 regularFile=1 bytes=" +
				std::to_string(FileByteCount(TempRoot() / "npc-marker.igmesh"))) !=
			std::string::npos,
		"package directory report should include NPC marker asset row");
	Expect(
		report.text.find("packageManifestReadIssue") == std::string::npos,
		"successful package directory report should not include read issue rows");
	Expect(
		report.packageManifestFactsRecorded,
		"successful package directory report should record package manifest file facts");
	ExpectFacts(
		report.packageManifestFacts,
		true,
		true,
		FileByteCount(TempRoot() / NativeStaticMeshExportPackageManifestSidecarFilename),
		"successful package directory report should expose package manifest file facts");
	Expect(
		report.manifestFactsRecorded,
		"successful package directory report should record nested manifest file facts");
	ExpectFacts(
		report.manifestFacts,
		true,
		true,
		FileByteCount(TempRoot() / NativeStaticMeshExportManifestFilename),
		"successful package directory report should expose nested manifest file facts");
	Expect(
		report.assetFacts.size() == 3,
		"successful package directory report should expose package-declared asset file facts");
	const NativeStaticMeshExportPackageDirectoryAssetFacts *cubeFacts =
		FindAssetFacts(report, "cube");
	Expect(cubeFacts != nullptr, "successful package directory report should expose cube facts");
	if (cubeFacts != nullptr) {
		ExpectFacts(
			cubeFacts->facts,
			true,
			true,
			FileByteCount(TempRoot() / "cube.igmesh"),
			"successful package directory report should expose cube file facts");
	}
	Expect(
		report.manifestReadAttempted,
		"successful package directory report should attempt nested manifest read");
	Expect(
		report.manifestRead.read(),
		"successful package directory report should expose successful nested manifest read");
	Expect(
		report.manifestRead.document.assets.size() == 3,
		"successful package directory report should expose nested manifest asset rows");
	Expect(
		report.manifestComparison.matchCount == 3,
		"successful package directory report should expose structured manifest match count");
	Expect(
		report.manifestComparison.comparisons.empty(),
		"successful package directory report should expose no structured manifest comparisons");
	Expect(
		report.text.find("\nmanifestReadIssue code=") == std::string::npos,
		"successful package directory report should not include manifest read issue rows");
	Expect(
		report.text.find("\nmanifestComparison code=") == std::string::npos,
		"successful package directory report should not include comparison rows");
	Expect(
		report.text.find(
			"manifestAsset=cube filename=cube.igmesh vertices=8 indices=36 bytes=523") !=
			std::string::npos,
		"successful package directory report should include cube manifest asset row");
	Expect(
		report.text.find(
			"manifestAsset=bean filename=bean.igmesh vertices=234 indices=1296 bytes=23882") !=
			std::string::npos,
		"successful package directory report should include bean manifest asset row");
	Expect(
		report.text.find(
			"manifestAsset=npc-marker filename=npc-marker.igmesh vertices=98 indices=504 bytes=9474") !=
			std::string::npos,
		"successful package directory report should include NPC marker manifest asset row");
	Expect(
		report.text.find("manifestAsset=cube") < report.text.find("manifestAsset=bean") &&
			report.text.find("manifestAsset=bean") <
				report.text.find("manifestAsset=npc-marker") &&
			report.text.find("manifestAsset=npc-marker") <
				report.text.find("asset=cube filename=cube.igmesh path="),
		"manifest asset rows should preserve manifest order and precede package asset rows");
	CleanupTempRoot();
}

void TestMissingFromManifestComparisonStillReportsRead()
{
	ResetTempRoot();
	ExportDefaultBatch();
	WriteText(
		TempRoot() / NativeStaticMeshExportManifestFilename,
		"static-mesh-export-manifest version=1 assets=2 bytes=24405\n"
		"asset=cube filename=cube.igmesh vertices=8 indices=36 bytes=523\n"
		"asset=bean filename=bean.igmesh vertices=234 indices=1296 bytes=23882\n");

	const NativeStaticMeshExportPackageDirectoryReport report = BuildDefaultReport();

	Expect(report.readOk(), "missing-from-manifest comparison should not fail package report");
	Expect(
		report.text.find("manifestRead=ok manifestReadIssues=0 manifestMatches=2 manifestMismatches=1 manifestComparisonIssues=1") !=
			std::string::npos,
		"missing-from-manifest comparison should update manifest comparison counts");
	Expect(
		report.text.find(
			"manifestComparison code=MissingFromManifest asset=npc-marker packageFilename=npc-marker.igmesh") !=
			std::string::npos,
		"missing-from-manifest comparison should emit deterministic comparison row");
	Expect(
		report.text.find("asset=npc-marker filename=npc-marker.igmesh") != std::string::npos,
		"missing-from-manifest comparison should keep package asset rows");
	CleanupTempRoot();
}

void TestMissingFromPackageComparisonStillReportsRead()
{
	ResetTempRoot();
	ExportDefaultBatch();
	WriteText(
		TempRoot() / NativeStaticMeshExportManifestFilename,
		"static-mesh-export-manifest version=1 assets=4 bytes=33886\n"
		"asset=cube filename=cube.igmesh vertices=8 indices=36 bytes=523\n"
		"asset=bean filename=bean.igmesh vertices=234 indices=1296 bytes=23882\n"
		"asset=npc-marker filename=npc-marker.igmesh vertices=98 indices=504 bytes=9474\n"
		"asset=extra filename=extra.igmesh vertices=1 indices=3 bytes=7\n");

	const NativeStaticMeshExportPackageDirectoryReport report = BuildDefaultReport();

	Expect(report.readOk(), "missing-from-package comparison should not fail package report");
	Expect(
		report.text.find("manifestRead=ok manifestReadIssues=0 manifestMatches=3 manifestMismatches=1 manifestComparisonIssues=1") !=
			std::string::npos,
		"missing-from-package comparison should update manifest comparison counts");
	Expect(
		report.text.find(
			"manifestComparison code=MissingFromPackage manifestAsset=extra manifestFilename=extra.igmesh") !=
			std::string::npos,
		"missing-from-package comparison should emit deterministic comparison row");
	Expect(
		report.text.find("manifestAsset=extra filename=extra.igmesh vertices=1 indices=3 bytes=7") !=
			std::string::npos,
		"missing-from-package comparison should keep projected manifest asset row");
	CleanupTempRoot();
}

void TestFilenameMismatchComparisonStillReportsRead()
{
	ResetTempRoot();
	ExportDefaultBatch();
	WriteText(
		TempRoot() / NativeStaticMeshExportManifestFilename,
		"static-mesh-export-manifest version=1 assets=3 bytes=33879\n"
		"asset=cube filename=cube-renamed.igmesh vertices=8 indices=36 bytes=523\n"
		"asset=bean filename=bean.igmesh vertices=234 indices=1296 bytes=23882\n"
		"asset=npc-marker filename=npc-marker.igmesh vertices=98 indices=504 bytes=9474\n");

	const NativeStaticMeshExportPackageDirectoryReport report = BuildDefaultReport();

	Expect(report.readOk(), "filename mismatch comparison should not fail package report");
	Expect(
		report.text.find("manifestRead=ok manifestReadIssues=0 manifestMatches=2 manifestMismatches=1 manifestComparisonIssues=1") !=
			std::string::npos,
		"filename mismatch comparison should update manifest comparison counts");
	Expect(
		report.text.find(
			"manifestComparison code=FilenameMismatch asset=cube packageFilename=cube.igmesh manifestFilename=cube-renamed.igmesh") !=
			std::string::npos,
		"filename mismatch comparison should emit deterministic comparison row");
	Expect(
		report.text.find("asset=cube filename=cube.igmesh path=") != std::string::npos,
		"filename mismatch comparison should keep package asset row");
	CleanupTempRoot();
}

void TestCombinedComparisonOrderingStillReportsRead()
{
	ResetTempRoot();
	ExportDefaultBatch();
	WriteText(
		TempRoot() / NativeStaticMeshExportManifestFilename,
		"static-mesh-export-manifest version=1 assets=3 bytes=24412\n"
		"asset=cube filename=cube-renamed.igmesh vertices=8 indices=36 bytes=523\n"
		"asset=bean filename=bean.igmesh vertices=234 indices=1296 bytes=23882\n"
		"asset=extra filename=extra.igmesh vertices=1 indices=3 bytes=7\n");

	const NativeStaticMeshExportPackageDirectoryReport report = BuildDefaultReport();

	Expect(report.readOk(), "combined comparison mismatch should not fail package report");
	Expect(
		report.text.find("manifestRead=ok manifestReadIssues=0 manifestMatches=1 manifestMismatches=3 manifestComparisonIssues=3") !=
			std::string::npos,
		"combined comparison mismatch should update manifest comparison counts");
	const std::size_t filenameMismatch = report.text.find(
		"manifestComparison code=FilenameMismatch asset=cube packageFilename=cube.igmesh manifestFilename=cube-renamed.igmesh");
	const std::size_t missingFromManifest = report.text.find(
		"manifestComparison code=MissingFromManifest asset=npc-marker packageFilename=npc-marker.igmesh");
	const std::size_t missingFromPackage = report.text.find(
		"manifestComparison code=MissingFromPackage manifestAsset=extra manifestFilename=extra.igmesh");
	Expect(
		filenameMismatch != std::string::npos,
		"combined comparison mismatch should include filename mismatch row");
	Expect(
		missingFromManifest != std::string::npos,
		"combined comparison mismatch should include missing-from-manifest row");
	Expect(
		missingFromPackage != std::string::npos,
		"combined comparison mismatch should include missing-from-package row");
	Expect(
		report.manifestComparison.matchCount == 1,
		"combined comparison mismatch should expose structured match count");
	Expect(
		report.manifestComparison.comparisons.size() == 3,
		"combined comparison mismatch should expose structured comparison count");
	if (report.manifestComparison.comparisons.size() == 3) {
		Expect(
			report.manifestComparison.comparisons[0].code ==
				NativeStaticMeshExportPackageDirectoryManifestComparisonCode::FilenameMismatch,
			"combined comparison mismatch should expose filename mismatch first");
		Expect(
			report.manifestComparison.comparisons[1].code ==
				NativeStaticMeshExportPackageDirectoryManifestComparisonCode::MissingFromManifest,
			"combined comparison mismatch should expose missing-from-manifest second");
		Expect(
			report.manifestComparison.comparisons[2].code ==
				NativeStaticMeshExportPackageDirectoryManifestComparisonCode::MissingFromPackage,
			"combined comparison mismatch should expose missing-from-package third");
	}
	Expect(
		filenameMismatch < missingFromManifest &&
			missingFromManifest < missingFromPackage,
		"combined comparison mismatch should emit package-order rows before manifest-only rows");
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
			"packageManifestExists=0 packageManifestRegularFile=0 packageManifestBytes=0") !=
			std::string::npos,
		"missing package sidecar report should include missing package manifest file facts");
	Expect(
		report.text.find(
			"packageManifestReadIssue code=FileOpenFailed line=0 token=" +
			packageManifest.string()) != std::string::npos,
		"missing package sidecar report should include file-open issue row");
	Expect(
		report.packageManifestFactsRecorded,
		"missing package sidecar report should record package manifest file facts");
	ExpectFacts(
		report.packageManifestFacts,
		false,
		false,
		0,
		"missing package sidecar report should expose missing package manifest file facts");
	Expect(
		!report.manifestFactsRecorded,
		"missing package sidecar report should not record nested manifest file facts");
	Expect(
		report.assetFacts.empty(),
		"missing package sidecar report should not expose asset file facts");
	Expect(
		!report.manifestReadAttempted,
		"missing package sidecar report should not attempt nested manifest read");
	Expect(
		report.manifestComparison.matchCount == 0 &&
			report.manifestComparison.comparisons.empty(),
		"missing package sidecar report should expose empty structured manifest comparison");
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
			"packageManifestExists=1 packageManifestRegularFile=1 packageManifestBytes=" +
			std::to_string(FileByteCount(packageManifest))) != std::string::npos,
		"malformed package sidecar report should include package manifest file facts");
	Expect(
		report.text.find(
			"packageManifestReadIssue code=MalformedHeader line=1 token=static-mesh-export-package") !=
			std::string::npos,
		"malformed package sidecar report should include parser issue row");
	Expect(
		report.packageManifestFactsRecorded,
		"malformed package sidecar report should record package manifest file facts");
	ExpectFacts(
		report.packageManifestFacts,
		true,
		true,
		FileByteCount(packageManifest),
		"malformed package sidecar report should expose package manifest file facts");
	Expect(
		!report.manifestFactsRecorded,
		"malformed package sidecar report should not record nested manifest file facts");
	Expect(
		report.assetFacts.empty(),
		"malformed package sidecar report should not expose asset file facts");
	Expect(
		!report.manifestReadAttempted,
		"malformed package sidecar report should not attempt nested manifest read");
	Expect(
		report.manifestComparison.matchCount == 0 &&
			report.manifestComparison.comparisons.empty(),
		"malformed package sidecar report should expose empty structured manifest comparison");
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
		report.text.find("manifestExists=0 manifestRegularFile=0 manifestBytes=0") !=
			std::string::npos,
		"missing nested mesh manifest should report missing file facts");
	Expect(
		report.text.find("manifestRead=invalid manifestReadIssues=1") !=
			std::string::npos,
		"missing nested mesh manifest should report invalid manifest read state");
	Expect(
		report.text.find(
			"manifestReadIssue code=FileOpenFailed line=0 token=" +
			(TempRoot() / NativeStaticMeshExportManifestFilename).string()) !=
			std::string::npos,
		"missing nested mesh manifest should report file-open issue row");
	Expect(
		report.manifestFactsRecorded,
		"missing nested mesh manifest should record nested manifest file facts");
	ExpectFacts(
		report.manifestFacts,
		false,
		false,
		0,
		"missing nested mesh manifest should expose missing nested manifest file facts");
	Expect(
		report.manifestReadAttempted,
		"missing nested mesh manifest should expose attempted nested manifest read");
	Expect(
		!report.manifestRead.read(),
		"missing nested mesh manifest should expose failed nested manifest read");
	Expect(
		report.manifestComparison.matchCount == 0 &&
			report.manifestComparison.comparisons.empty(),
		"missing nested mesh manifest should expose empty structured manifest comparison");
	Expect(
		report.text.find("manifestAsset=") == std::string::npos,
		"missing nested mesh manifest should not emit manifest asset rows");
	Expect(
		report.text.find("\nmanifestComparison code=") == std::string::npos,
		"missing nested mesh manifest should not emit comparison rows");
	Expect(
		report.text.find("asset=cube filename=cube.igmesh") != std::string::npos,
		"missing nested mesh manifest should not suppress asset rows");
	CleanupTempRoot();
}

void TestMalformedNestedManifestStillReportsRead()
{
	ResetTempRoot();
	ExportDefaultBatch();
	WriteText(
		TempRoot() / NativeStaticMeshExportManifestFilename,
		"static-mesh-export-manifest version=2 assets=0 bytes=0\n");

	const NativeStaticMeshExportPackageDirectoryReport report = BuildDefaultReport();

	Expect(report.readOk(), "malformed nested mesh manifest should not fail package report");
	Expect(
		report.text.find("status=Read") != std::string::npos,
		"malformed nested mesh manifest package report should stay read");
	Expect(
		report.text.find("manifestRead=invalid manifestReadIssues=1") !=
			std::string::npos,
		"malformed nested mesh manifest should report invalid manifest read state");
	Expect(
		report.text.find(
			"manifestReadIssue code=UnsupportedVersion line=1 token=2") !=
			std::string::npos,
		"malformed nested mesh manifest should report parser issue row");
	Expect(
		report.manifestReadAttempted,
		"malformed nested mesh manifest should expose attempted nested manifest read");
	Expect(
		!report.manifestRead.read(),
		"malformed nested mesh manifest should expose failed nested manifest read");
	Expect(
		report.manifestComparison.matchCount == 0 &&
			report.manifestComparison.comparisons.empty(),
		"malformed nested mesh manifest should expose empty structured manifest comparison");
	Expect(
		report.text.find("manifestAsset=") == std::string::npos,
		"malformed nested mesh manifest should not emit manifest asset rows");
	Expect(
		report.text.find("\nmanifestComparison code=") == std::string::npos,
		"malformed nested mesh manifest should not emit comparison rows");
	Expect(
		report.text.find("asset=cube filename=cube.igmesh") != std::string::npos,
		"malformed nested mesh manifest should not suppress asset rows");
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
			(TempRoot() / "cube.igmesh").string() +
				" exists=0 regularFile=0 bytes=0") != std::string::npos,
		"missing declared mesh asset path should report missing file facts");
	const NativeStaticMeshExportPackageDirectoryAssetFacts *cubeFacts =
		FindAssetFacts(report, "cube");
	Expect(cubeFacts != nullptr, "missing declared mesh asset should expose cube facts");
	if (cubeFacts != nullptr) {
		ExpectFacts(
			cubeFacts->facts,
			false,
			false,
			0,
			"missing declared mesh asset should expose missing cube file facts");
	}
	Expect(
		report.text.find(
			"asset=bean filename=bean.igmesh path=" +
			(TempRoot() / "bean.igmesh").string() +
				" exists=1 regularFile=1 bytes=" +
				std::to_string(FileByteCount(TempRoot() / "bean.igmesh"))) != std::string::npos,
		"present declared mesh asset path should report existing file facts");
	CleanupTempRoot();
}

void TestDirectoryAtDeclaredAssetStillReportsRead()
{
	ResetTempRoot();
	ExportDefaultBatch();
	std::filesystem::remove(TempRoot() / "cube.igmesh");
	std::error_code ignored;
	std::filesystem::create_directory(TempRoot() / "cube.igmesh", ignored);

	const NativeStaticMeshExportPackageDirectoryReport report = BuildDefaultReport();

	Expect(report.readOk(), "directory at declared mesh asset should not fail package report");
	Expect(
		report.text.find("status=Read") != std::string::npos,
		"directory at declared mesh asset package report should stay read");
	Expect(
		report.text.find(
			"asset=cube filename=cube.igmesh path=" +
			(TempRoot() / "cube.igmesh").string() +
				" exists=1 regularFile=0 bytes=0") != std::string::npos,
		"directory at declared mesh asset path should report non-regular file facts");
	const NativeStaticMeshExportPackageDirectoryAssetFacts *cubeFacts =
		FindAssetFacts(report, "cube");
	Expect(cubeFacts != nullptr, "directory at declared mesh asset should expose cube facts");
	if (cubeFacts != nullptr) {
		ExpectFacts(
			cubeFacts->facts,
			true,
			false,
			0,
			"directory at declared mesh asset should expose non-regular cube facts");
	}
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
	TestMissingFromManifestComparisonStillReportsRead();
	TestMissingFromPackageComparisonStillReportsRead();
	TestFilenameMismatchComparisonStillReportsRead();
	TestCombinedComparisonOrderingStillReportsRead();
	TestMissingDirectoryReportFails();
	TestFilePathInsteadOfDirectoryReportFails();
	TestMissingPackageSidecarReportIncludesIssueRow();
	TestMalformedPackageSidecarReportIncludesIssueRow();
	TestMissingNestedManifestStillReportsRead();
	TestMalformedNestedManifestStillReportsRead();
	TestMissingDeclaredAssetStillReportsRead();
	TestDirectoryAtDeclaredAssetStillReportsRead();
	TestExtraUnrelatedFileIsIgnored();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
