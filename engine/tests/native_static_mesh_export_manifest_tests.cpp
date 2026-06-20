#include "../apps/native_play/NativeStaticMeshExportManifest.hpp"
#include "../apps/native_play/NativeStaticMeshExportPolicy.hpp"
#include "../apps/native_play/NativeStaticMeshExportReport.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include "support/TestHarness.hpp"

namespace {

using iggy::native_play::BuildNativeStaticMeshExportManifestText;
using iggy::native_play::BuildNativeStaticMeshExportReport;
using iggy::native_play::DefaultNativeStaticMeshExportPolicy;
using iggy::native_play::NativeStaticMeshBuiltInExportId;
using iggy::native_play::NativeStaticMeshExportManifestReadIssueCode;
using iggy::native_play::NativeStaticMeshExportManifestReadIssueCodeText;
using iggy::native_play::NativeStaticMeshExportManifestReadResult;
using iggy::native_play::NativeStaticMeshExportManifestResult;
using iggy::native_play::NativeStaticMeshExportManifestStatus;
using iggy::native_play::NativeStaticMeshExportPolicy;
using iggy::native_play::NativeStaticMeshExportReport;
using iggy::native_play::NativeStaticMeshExportReportEntry;
using iggy::native_play::ReadNativeStaticMeshExportManifestFile;
using iggy::native_play::ReadNativeStaticMeshExportManifestText;
using iggy::test::Expect;
using iggy::test::Failures;

std::string ExpectedManifestText(const NativeStaticMeshExportReport &report)
{
	std::string text =
		"static-mesh-export-manifest version=1 assets=" +
		std::to_string(report.assetCount) +
		" bytes=" + std::to_string(report.byteCount) + "\n";
	for (const NativeStaticMeshExportReportEntry &entry : report.entries) {
		text +=
			"asset=" + entry.name +
			" filename=" + entry.defaultFilename +
			" vertices=" + std::to_string(entry.vertexCount) +
			" indices=" + std::to_string(entry.indexCount) +
			" bytes=" + std::to_string(entry.byteCount) + "\n";
	}
	return text;
}

std::filesystem::path TempManifestPath(const std::string &name)
{
	return std::filesystem::temp_directory_path() /
		("iggy_native_static_mesh_export_manifest_" + name + ".txt");
}

void WriteTextFile(const std::filesystem::path &path, const std::string &text)
{
	std::ofstream file(path, std::ios::binary);
	file << text;
}

void TestDefaultManifestMatchesReport()
{
	const NativeStaticMeshExportPolicy policy = DefaultNativeStaticMeshExportPolicy();
	const NativeStaticMeshExportReport report =
		BuildNativeStaticMeshExportReport(policy);
	const NativeStaticMeshExportManifestResult result =
		BuildNativeStaticMeshExportManifestText(policy);

	Expect(result.written(), "default manifest should write");
	Expect(
		result.status == NativeStaticMeshExportManifestStatus::Built,
		"default manifest status should be built");
	Expect(result.issueCount == 0, "default manifest should report no issues");
	Expect(result.text == ExpectedManifestText(report), "default manifest should match export report counts");
}

void TestDefaultManifestContainsPolicyOrderAndFilenames()
{
	const NativeStaticMeshExportManifestResult result =
		BuildNativeStaticMeshExportManifestText(DefaultNativeStaticMeshExportPolicy());

	Expect(result.text.find("asset=cube filename=cube.igmesh") != std::string::npos, "manifest should contain cube row");
	Expect(result.text.find("asset=bean filename=bean.igmesh") != std::string::npos, "manifest should contain bean row");
	Expect(result.text.find("asset=npc-marker filename=npc-marker.igmesh") != std::string::npos, "manifest should contain NPC marker row");
	Expect(
		result.text.find("asset=cube") < result.text.find("asset=bean") &&
			result.text.find("asset=bean") < result.text.find("asset=npc-marker"),
		"manifest should preserve default policy order");
}

void TestDefaultManifestIsDeterministic()
{
	const NativeStaticMeshExportPolicy policy = DefaultNativeStaticMeshExportPolicy();
	const NativeStaticMeshExportManifestResult first =
		BuildNativeStaticMeshExportManifestText(policy);
	const NativeStaticMeshExportManifestResult second =
		BuildNativeStaticMeshExportManifestText(policy);

	Expect(first.written(), "first manifest should write");
	Expect(second.written(), "second manifest should write");
	Expect(first.text == second.text, "manifest output should be deterministic");
}

void TestInvalidPolicyDoesNotWriteManifest()
{
	const NativeStaticMeshExportPolicy policy {
		{
			{ NativeStaticMeshBuiltInExportId::Cube, "cube", "same.igmesh" },
			{ NativeStaticMeshBuiltInExportId::Bean, "bean", "same.igmesh" },
		},
	};

	const NativeStaticMeshExportManifestResult result =
		BuildNativeStaticMeshExportManifestText(policy);

	Expect(!result.written(), "invalid policy should not produce successful manifest");
	Expect(
		result.status == NativeStaticMeshExportManifestStatus::InvalidPolicy,
		"invalid policy should report invalid policy status");
	Expect(result.issueCount > 0, "invalid policy manifest should surface issue count");
	Expect(result.text.empty(), "invalid policy manifest should not emit text");
}

bool HasReadIssue(
	const NativeStaticMeshExportManifestReadResult &result,
	NativeStaticMeshExportManifestReadIssueCode code)
{
	for (const auto &issue : result.issues) {
		if (issue.code == code)
			return true;
	}
	return false;
}

void ExpectReadIssue(
	const std::string &text,
	NativeStaticMeshExportManifestReadIssueCode code,
	const char *message)
{
	const NativeStaticMeshExportManifestReadResult result =
		ReadNativeStaticMeshExportManifestText(text);
	Expect(!result.read(), "malformed manifest should not read successfully");
	Expect(HasReadIssue(result, code), message);
}

void TestDefaultManifestReads()
{
	const NativeStaticMeshExportManifestResult manifest =
		BuildNativeStaticMeshExportManifestText(DefaultNativeStaticMeshExportPolicy());
	const NativeStaticMeshExportReport report =
		BuildNativeStaticMeshExportReport(DefaultNativeStaticMeshExportPolicy());

	const NativeStaticMeshExportManifestReadResult read =
		ReadNativeStaticMeshExportManifestText(manifest.text);

	Expect(read.read(), "generated mesh export manifest should read");
	Expect(read.issues.empty(), "generated mesh export manifest should have no read issues");
	Expect(read.document.version == 1, "generated mesh export manifest should expose version 1");
	Expect(
		read.document.assetCount == report.assetCount,
		"generated mesh export manifest should expose asset count");
	Expect(
		read.document.byteCount == report.byteCount,
		"generated mesh export manifest should expose total byte count");
	Expect(
		read.document.assets.size() == report.entries.size(),
		"generated mesh export manifest should expose asset rows");
	if (read.document.assets.size() < 3)
		return;
	Expect(
		read.document.assets[0].name == "cube" &&
			read.document.assets[0].filename == "cube.igmesh" &&
			read.document.assets[0].vertexCount == report.entries[0].vertexCount &&
			read.document.assets[0].indexCount == report.entries[0].indexCount &&
			read.document.assets[0].byteCount == report.entries[0].byteCount,
		"generated mesh export manifest should preserve cube row facts");
	Expect(
		read.document.assets[1].name == "bean" &&
			read.document.assets[2].name == "npc-marker",
		"generated mesh export manifest should preserve policy row order");
}

void TestManifestReaderRejectsEmptyInput()
{
	ExpectReadIssue(
		"",
		NativeStaticMeshExportManifestReadIssueCode::EmptyInput,
		"empty mesh export manifest should report empty input");
}

void TestManifestReaderRejectsMalformedHeader()
{
	ExpectReadIssue(
		"static-mesh-export version=1 assets=0 bytes=0\n",
		NativeStaticMeshExportManifestReadIssueCode::MalformedHeader,
		"bad mesh export manifest header should report malformed header");
}

void TestManifestReaderRejectsUnsupportedVersion()
{
	ExpectReadIssue(
		"static-mesh-export-manifest version=2 assets=0 bytes=0\n",
		NativeStaticMeshExportManifestReadIssueCode::UnsupportedVersion,
		"unsupported mesh export manifest version should report unsupported version");
}

void TestManifestReaderRejectsMalformedHeaderCounts()
{
	ExpectReadIssue(
		"static-mesh-export-manifest version=1 assets=nope bytes=0\n",
		NativeStaticMeshExportManifestReadIssueCode::MalformedAssetCount,
		"malformed asset count should report asset-count issue");
	ExpectReadIssue(
		"static-mesh-export-manifest version=1 assets=0 bytes=nope\n",
		NativeStaticMeshExportManifestReadIssueCode::MalformedByteCount,
		"malformed byte count should report byte-count issue");
}

void TestManifestReaderRejectsMissingAndExtraFields()
{
	ExpectReadIssue(
		"static-mesh-export-manifest version=1 assets=0\n",
		NativeStaticMeshExportManifestReadIssueCode::MissingField,
		"missing header field should report missing field");
	ExpectReadIssue(
		"static-mesh-export-manifest version=1 assets=0 bytes=0 extra=1\n",
		NativeStaticMeshExportManifestReadIssueCode::ExtraToken,
		"extra header token should report extra token");
	ExpectReadIssue(
		"static-mesh-export-manifest version=1 assets=1 bytes=0\n"
		"asset=cube filename=cube.igmesh vertices=8\n",
		NativeStaticMeshExportManifestReadIssueCode::MissingField,
		"missing asset row field should report missing field");
	ExpectReadIssue(
		"static-mesh-export-manifest version=1 assets=1 bytes=523\n"
		"asset=cube filename=cube.igmesh vertices=8 indices=36 bytes=523 extra=1\n",
		NativeStaticMeshExportManifestReadIssueCode::ExtraToken,
		"extra asset row token should report extra token");
}

void TestManifestReaderRejectsMalformedRows()
{
	ExpectReadIssue(
		"static-mesh-export-manifest version=1 assets=1 bytes=523\n"
		"row=cube filename=cube.igmesh vertices=8 indices=36 bytes=523\n",
		NativeStaticMeshExportManifestReadIssueCode::UnexpectedLine,
		"unexpected asset row directive should report unexpected line");
	ExpectReadIssue(
		"static-mesh-export-manifest version=1 assets=1 bytes=523\n"
		"asset=cube filename=bad/cube.igmesh vertices=8 indices=36 bytes=523\n",
		NativeStaticMeshExportManifestReadIssueCode::MalformedAssetRow,
		"separator-containing asset filename should report malformed row");
	ExpectReadIssue(
		"static-mesh-export-manifest version=1 assets=1 bytes=523\n"
		"asset=cube filename=cube.igmesh vertices=nope indices=36 bytes=523\n",
		NativeStaticMeshExportManifestReadIssueCode::MalformedAssetRow,
		"non-numeric asset count should report malformed row");
}

void TestManifestReaderRejectsDuplicateRows()
{
	ExpectReadIssue(
		"static-mesh-export-manifest version=1 assets=2 bytes=1046\n"
		"asset=cube filename=cube.igmesh vertices=8 indices=36 bytes=523\n"
		"asset=cube filename=cube-copy.igmesh vertices=8 indices=36 bytes=523\n",
		NativeStaticMeshExportManifestReadIssueCode::DuplicateAssetName,
		"duplicate asset name should report duplicate name");
	ExpectReadIssue(
		"static-mesh-export-manifest version=1 assets=2 bytes=1046\n"
		"asset=cube filename=cube.igmesh vertices=8 indices=36 bytes=523\n"
		"asset=bean filename=cube.igmesh vertices=8 indices=36 bytes=523\n",
		NativeStaticMeshExportManifestReadIssueCode::DuplicateAssetFilename,
		"duplicate asset filename should report duplicate filename");
}

void TestManifestReaderRejectsCountMismatches()
{
	ExpectReadIssue(
		"static-mesh-export-manifest version=1 assets=2 bytes=523\n"
		"asset=cube filename=cube.igmesh vertices=8 indices=36 bytes=523\n",
		NativeStaticMeshExportManifestReadIssueCode::AssetCountMismatch,
		"wrong asset count should report asset-count mismatch");
	ExpectReadIssue(
		"static-mesh-export-manifest version=1 assets=1 bytes=999\n"
		"asset=cube filename=cube.igmesh vertices=8 indices=36 bytes=523\n",
		NativeStaticMeshExportManifestReadIssueCode::ByteCountMismatch,
		"wrong total byte count should report byte-count mismatch");
}

void TestFileReaderReadsGeneratedManifest()
{
	const std::filesystem::path path = TempManifestPath("generated");
	std::filesystem::remove(path);
	const NativeStaticMeshExportManifestResult manifest =
		BuildNativeStaticMeshExportManifestText(DefaultNativeStaticMeshExportPolicy());
	const NativeStaticMeshExportReport report =
		BuildNativeStaticMeshExportReport(DefaultNativeStaticMeshExportPolicy());
	WriteTextFile(path, manifest.text);

	const NativeStaticMeshExportManifestReadResult result =
		ReadNativeStaticMeshExportManifestFile(path);

	std::filesystem::remove(path);
	Expect(result.read(), "mesh export manifest file reader should read generated text");
	Expect(result.document.version == 1, "mesh export manifest file reader should preserve version");
	Expect(
		result.document.assetCount == report.assetCount,
		"mesh export manifest file reader should preserve asset count");
	Expect(
		result.document.byteCount == report.byteCount,
		"mesh export manifest file reader should preserve total byte count");
	Expect(
		result.document.assets.size() == report.entries.size(),
		"mesh export manifest file reader should preserve rows");
	if (result.document.assets.size() == report.entries.size() &&
			result.document.assets.size() >= 3) {
		Expect(result.document.assets[0].name == "cube", "file reader first row should be cube");
		Expect(
			result.document.assets[0].vertexCount == report.entries[0].vertexCount &&
				result.document.assets[0].indexCount == report.entries[0].indexCount &&
				result.document.assets[0].byteCount == report.entries[0].byteCount,
			"file reader should preserve first row facts");
		Expect(
			result.document.assets[2].filename == "npc-marker.igmesh",
			"file reader last row filename should be npc-marker.igmesh");
	}
}

void TestFileReaderReportsMissingFile()
{
	const std::filesystem::path path = TempManifestPath("missing");
	std::filesystem::remove(path);

	const NativeStaticMeshExportManifestReadResult result =
		ReadNativeStaticMeshExportManifestFile(path);

	Expect(!result.read(), "missing mesh export manifest file should not read");
	Expect(result.issues.size() == 1, "missing mesh export manifest file should report one issue");
	Expect(
		HasReadIssue(
			result,
			NativeStaticMeshExportManifestReadIssueCode::FileOpenFailed),
		"missing mesh export manifest file should report file-open failure");
	if (!result.issues.empty()) {
		Expect(result.issues[0].line == 0, "file-open failure should use line zero");
		Expect(
			result.issues[0].token == path.string(),
			"file-open failure should report the supplied path");
	}
}

void TestFileReaderPropagatesTextReaderIssues()
{
	const std::filesystem::path path = TempManifestPath("malformed");
	std::filesystem::remove(path);
	WriteTextFile(
		path,
		"static-mesh-export-manifest version=2 assets=0 bytes=0\n");

	const NativeStaticMeshExportManifestReadResult result =
		ReadNativeStaticMeshExportManifestFile(path);

	std::filesystem::remove(path);
	Expect(!result.read(), "malformed mesh export manifest file should not read");
	Expect(
		HasReadIssue(
			result,
			NativeStaticMeshExportManifestReadIssueCode::UnsupportedVersion),
		"malformed mesh export manifest file should propagate text reader issue");
	Expect(
		!HasReadIssue(
			result,
			NativeStaticMeshExportManifestReadIssueCode::FileOpenFailed),
		"malformed readable file should not report file-open failure");
}

void TestReadIssueCodeText()
{
	struct Case {
		NativeStaticMeshExportManifestReadIssueCode code;
		const char *text;
	};

	const Case cases[] = {
		{ NativeStaticMeshExportManifestReadIssueCode::FileOpenFailed, "FileOpenFailed" },
		{ NativeStaticMeshExportManifestReadIssueCode::EmptyInput, "EmptyInput" },
		{ NativeStaticMeshExportManifestReadIssueCode::MalformedHeader, "MalformedHeader" },
		{ NativeStaticMeshExportManifestReadIssueCode::UnsupportedVersion, "UnsupportedVersion" },
		{ NativeStaticMeshExportManifestReadIssueCode::MalformedAssetCount, "MalformedAssetCount" },
		{ NativeStaticMeshExportManifestReadIssueCode::MalformedByteCount, "MalformedByteCount" },
		{ NativeStaticMeshExportManifestReadIssueCode::MissingField, "MissingField" },
		{ NativeStaticMeshExportManifestReadIssueCode::MalformedAssetRow, "MalformedAssetRow" },
		{ NativeStaticMeshExportManifestReadIssueCode::AssetCountMismatch, "AssetCountMismatch" },
		{ NativeStaticMeshExportManifestReadIssueCode::ByteCountMismatch, "ByteCountMismatch" },
		{ NativeStaticMeshExportManifestReadIssueCode::DuplicateAssetName, "DuplicateAssetName" },
		{ NativeStaticMeshExportManifestReadIssueCode::DuplicateAssetFilename, "DuplicateAssetFilename" },
		{ NativeStaticMeshExportManifestReadIssueCode::ExtraToken, "ExtraToken" },
		{ NativeStaticMeshExportManifestReadIssueCode::UnexpectedLine, "UnexpectedLine" },
	};

	for (const Case &testCase : cases) {
		Expect(
			std::string(NativeStaticMeshExportManifestReadIssueCodeText(
				testCase.code)) == testCase.text,
			"mesh export manifest read issue code text should match stable spelling");
	}
	Expect(
		std::string(NativeStaticMeshExportManifestReadIssueCodeText(
			static_cast<NativeStaticMeshExportManifestReadIssueCode>(999))) ==
			"Unknown",
		"mesh export manifest read issue code text should report unknown fallback");
}

} // namespace

int main()
{
	TestDefaultManifestMatchesReport();
	TestDefaultManifestContainsPolicyOrderAndFilenames();
	TestDefaultManifestIsDeterministic();
	TestInvalidPolicyDoesNotWriteManifest();
	TestDefaultManifestReads();
	TestManifestReaderRejectsEmptyInput();
	TestManifestReaderRejectsMalformedHeader();
	TestManifestReaderRejectsUnsupportedVersion();
	TestManifestReaderRejectsMalformedHeaderCounts();
	TestManifestReaderRejectsMissingAndExtraFields();
	TestManifestReaderRejectsMalformedRows();
	TestManifestReaderRejectsDuplicateRows();
	TestManifestReaderRejectsCountMismatches();
	TestFileReaderReadsGeneratedManifest();
	TestFileReaderReportsMissingFile();
	TestFileReaderPropagatesTextReaderIssues();
	TestReadIssueCodeText();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
