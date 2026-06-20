#include "../apps/native_play/NativeStaticMeshExportPackageManifest.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include "support/TestHarness.hpp"

namespace {

using iggy::native_play::BuildNativeStaticMeshExportPackageManifestText;
using iggy::native_play::DefaultNativeStaticMeshExportPackagePolicy;
using iggy::native_play::NativeStaticMeshExportPackageManifestReadIssueCode;
using iggy::native_play::NativeStaticMeshExportPackageManifestReadIssueCodeText;
using iggy::native_play::NativeStaticMeshExportPackageManifestReadResult;
using iggy::native_play::NativeStaticMeshExportPackageManifestResult;
using iggy::native_play::NativeStaticMeshExportPackageManifestStatus;
using iggy::native_play::NativeStaticMeshExportPackagePolicy;
using iggy::native_play::ReadNativeStaticMeshExportPackageManifestFile;
using iggy::native_play::ReadNativeStaticMeshExportPackageManifestText;
using iggy::test::Expect;
using iggy::test::Failures;

const char *ExpectedDefaultManifestText()
{
	return
		"static-mesh-export-package-manifest format=iggy:native-static-mesh-export-package version=1 manifest=static-mesh-export-manifest.txt assets=3\n"
		"asset=cube filename=cube.igmesh\n"
		"asset=bean filename=bean.igmesh\n"
		"asset=npc-marker filename=npc-marker.igmesh\n";
}

void TestDefaultPackageManifestBuilds()
{
	const NativeStaticMeshExportPackageManifestResult result =
		BuildNativeStaticMeshExportPackageManifestText(
			DefaultNativeStaticMeshExportPackagePolicy());

	Expect(result.written(), "default package manifest should write");
	Expect(
		result.status == NativeStaticMeshExportPackageManifestStatus::Built,
		"default package manifest status should be built");
	Expect(result.issueCount == 0, "default package manifest should report no issues");
	Expect(
		result.text == ExpectedDefaultManifestText(),
		"default package manifest text should be exact");
}

void TestDefaultPackageManifestIncludesStableSummary()
{
	const NativeStaticMeshExportPackageManifestResult result =
		BuildNativeStaticMeshExportPackageManifestText(
			DefaultNativeStaticMeshExportPackagePolicy());

	Expect(
		result.text.find("format=iggy:native-static-mesh-export-package") !=
			std::string::npos,
		"default package manifest should include format id");
	Expect(
		result.text.find("version=1") != std::string::npos,
		"default package manifest should include version");
	Expect(
		result.text.find("manifest=static-mesh-export-manifest.txt") !=
			std::string::npos,
		"default package manifest should include nested manifest filename");
	Expect(
		result.text.find("assets=3") != std::string::npos,
		"default package manifest should include asset count");
}

void TestAssetRowsFollowNestedMeshPolicyOrder()
{
	const NativeStaticMeshExportPackageManifestResult result =
		BuildNativeStaticMeshExportPackageManifestText(
			DefaultNativeStaticMeshExportPackagePolicy());

	const std::size_t cube = result.text.find("asset=cube filename=cube.igmesh");
	const std::size_t bean = result.text.find("asset=bean filename=bean.igmesh");
	const std::size_t npc =
		result.text.find("asset=npc-marker filename=npc-marker.igmesh");
	Expect(cube != std::string::npos, "cube row should be present");
	Expect(bean != std::string::npos, "bean row should be present");
	Expect(npc != std::string::npos, "NPC marker row should be present");
	Expect(cube < bean && bean < npc, "asset rows should follow policy order");
}

void TestRepeatedCallsAreDeterministic()
{
	const NativeStaticMeshExportPackagePolicy policy =
		DefaultNativeStaticMeshExportPackagePolicy();

	const NativeStaticMeshExportPackageManifestResult first =
		BuildNativeStaticMeshExportPackageManifestText(policy);
	const NativeStaticMeshExportPackageManifestResult second =
		BuildNativeStaticMeshExportPackageManifestText(policy);

	Expect(first.written(), "first deterministic package manifest should write");
	Expect(second.written(), "second deterministic package manifest should write");
	Expect(first.text == second.text, "package manifest output should be deterministic");
}

void TestInvalidPolicyReturnsFailureWithoutText()
{
	NativeStaticMeshExportPackagePolicy policy =
		DefaultNativeStaticMeshExportPackagePolicy();
	policy.version = 2;

	const NativeStaticMeshExportPackageManifestResult result =
		BuildNativeStaticMeshExportPackageManifestText(policy);

	Expect(!result.written(), "invalid package policy should not write manifest");
	Expect(
		result.status == NativeStaticMeshExportPackageManifestStatus::InvalidPolicy,
		"invalid package policy should report invalid-policy status");
	Expect(result.issueCount > 0, "invalid package policy should report issues");
	Expect(result.text.empty(), "invalid package policy should produce no text");
}

void TestBuilderDoesNotNeedFilesystemState()
{
	NativeStaticMeshExportPackagePolicy policy =
		DefaultNativeStaticMeshExportPackagePolicy();
	policy.manifestFilename = "not-present-on-disk.txt";

	const NativeStaticMeshExportPackageManifestResult result =
		BuildNativeStaticMeshExportPackageManifestText(policy);

	Expect(result.written(), "package manifest builder should not touch filesystem");
	Expect(
		result.text.find("manifest=not-present-on-disk.txt") != std::string::npos,
		"package manifest should use supplied manifest filename value");
}

bool HasReadIssue(
	const NativeStaticMeshExportPackageManifestReadResult &result,
	NativeStaticMeshExportPackageManifestReadIssueCode code)
{
	for (const auto &issue : result.issues) {
		if (issue.code == code)
			return true;
	}
	return false;
}

void ExpectReadIssue(
	std::string text,
	NativeStaticMeshExportPackageManifestReadIssueCode code,
	const char *message)
{
	const NativeStaticMeshExportPackageManifestReadResult result =
		ReadNativeStaticMeshExportPackageManifestText(text);
	Expect(!result.read(), "malformed package manifest should not read");
	Expect(HasReadIssue(result, code), message);
}

std::filesystem::path TempPackageManifestPath(const char *name)
{
	return std::filesystem::temp_directory_path() /
		("iggy-native-package-manifest-" + std::string { name } + ".txt");
}

void WriteTextFile(const std::filesystem::path &path, const std::string &text)
{
	std::ofstream file(path, std::ios::binary);
	file << text;
}

void TestDefaultPackageManifestReadsBack()
{
	const NativeStaticMeshExportPackageManifestResult manifest =
		BuildNativeStaticMeshExportPackageManifestText(
			DefaultNativeStaticMeshExportPackagePolicy());

	const NativeStaticMeshExportPackageManifestReadResult result =
		ReadNativeStaticMeshExportPackageManifestText(manifest.text);

	Expect(result.read(), "default package manifest should read back");
	Expect(
		result.document.formatId == "iggy:native-static-mesh-export-package",
		"read package manifest should preserve format id");
	Expect(result.document.version == 1, "read package manifest should preserve version");
	Expect(
		result.document.manifestFilename == "static-mesh-export-manifest.txt",
		"read package manifest should preserve nested manifest filename");
	Expect(result.document.assets.size() == 3, "read package manifest should have assets");
	if (result.document.assets.size() == 3) {
		Expect(result.document.assets[0].name == "cube", "first asset should be cube");
		Expect(
			result.document.assets[0].filename == "cube.igmesh",
			"first asset filename should be cube.igmesh");
		Expect(result.document.assets[1].name == "bean", "second asset should be bean");
		Expect(
			result.document.assets[1].filename == "bean.igmesh",
			"second asset filename should be bean.igmesh");
		Expect(
			result.document.assets[2].name == "npc-marker",
			"third asset should be NPC marker");
		Expect(
			result.document.assets[2].filename == "npc-marker.igmesh",
			"third asset filename should be npc-marker.igmesh");
	}
}

void TestReaderRejectsEmptyInput()
{
	ExpectReadIssue(
		{},
		NativeStaticMeshExportPackageManifestReadIssueCode::EmptyInput,
		"empty package manifest should report empty input");
}

void TestReaderRejectsBadHeaderToken()
{
	ExpectReadIssue(
		"static-mesh-export-package format=iggy:native-static-mesh-export-package version=1 manifest=static-mesh-export-manifest.txt assets=0\n",
		NativeStaticMeshExportPackageManifestReadIssueCode::MalformedHeader,
		"bad package manifest header token should report malformed header");
}

void TestReaderRejectsUnsupportedFormat()
{
	ExpectReadIssue(
		"static-mesh-export-package-manifest format=bad version=1 manifest=static-mesh-export-manifest.txt assets=0\n",
		NativeStaticMeshExportPackageManifestReadIssueCode::UnsupportedFormatId,
		"unsupported package manifest format should report unsupported format");
}

void TestReaderRejectsUnsupportedVersion()
{
	ExpectReadIssue(
		"static-mesh-export-package-manifest format=iggy:native-static-mesh-export-package version=2 manifest=static-mesh-export-manifest.txt assets=0\n",
		NativeStaticMeshExportPackageManifestReadIssueCode::UnsupportedVersion,
		"unsupported package manifest version should report unsupported version");
}

void TestReaderRejectsMalformedAssetCount()
{
	ExpectReadIssue(
		"static-mesh-export-package-manifest format=iggy:native-static-mesh-export-package version=1 manifest=static-mesh-export-manifest.txt assets=three\n",
		NativeStaticMeshExportPackageManifestReadIssueCode::MalformedAssetCount,
		"malformed package manifest asset count should be reported");
}

void TestReaderRejectsMissingHeaderFields()
{
	ExpectReadIssue(
		"static-mesh-export-package-manifest format=iggy:native-static-mesh-export-package version=1 manifest=static-mesh-export-manifest.txt\n",
		NativeStaticMeshExportPackageManifestReadIssueCode::MissingField,
		"missing package manifest header fields should be reported");
}

void TestReaderRejectsExtraHeaderTokens()
{
	ExpectReadIssue(
		"static-mesh-export-package-manifest format=iggy:native-static-mesh-export-package version=1 manifest=static-mesh-export-manifest.txt assets=0 extra=1\n",
		NativeStaticMeshExportPackageManifestReadIssueCode::ExtraToken,
		"extra package manifest header token should be reported");
}

void TestReaderRejectsMissingAssetFields()
{
	ExpectReadIssue(
		"static-mesh-export-package-manifest format=iggy:native-static-mesh-export-package version=1 manifest=static-mesh-export-manifest.txt assets=1\n"
		"asset=cube\n",
		NativeStaticMeshExportPackageManifestReadIssueCode::MissingField,
		"missing package manifest asset row fields should be reported");
}

void TestReaderRejectsExtraAssetTokens()
{
	ExpectReadIssue(
		"static-mesh-export-package-manifest format=iggy:native-static-mesh-export-package version=1 manifest=static-mesh-export-manifest.txt assets=1\n"
		"asset=cube filename=cube.igmesh extra=1\n",
		NativeStaticMeshExportPackageManifestReadIssueCode::ExtraToken,
		"extra package manifest asset row token should be reported");
}

void TestReaderRejectsMalformedAssetRow()
{
	ExpectReadIssue(
		"static-mesh-export-package-manifest format=iggy:native-static-mesh-export-package version=1 manifest=static-mesh-export-manifest.txt assets=1\n"
		"asset= filename=cube.igmesh\n",
		NativeStaticMeshExportPackageManifestReadIssueCode::MalformedAssetRow,
		"malformed package manifest asset row should be reported");
}

void TestReaderRejectsUnexpectedLine()
{
	ExpectReadIssue(
		"static-mesh-export-package-manifest format=iggy:native-static-mesh-export-package version=1 manifest=static-mesh-export-manifest.txt assets=0\n"
		"comment nope\n",
		NativeStaticMeshExportPackageManifestReadIssueCode::UnexpectedLine,
		"unexpected package manifest line should be reported");
}

void TestReaderRejectsAssetCountMismatch()
{
	ExpectReadIssue(
		"static-mesh-export-package-manifest format=iggy:native-static-mesh-export-package version=1 manifest=static-mesh-export-manifest.txt assets=2\n"
		"asset=cube filename=cube.igmesh\n",
		NativeStaticMeshExportPackageManifestReadIssueCode::AssetCountMismatch,
		"package manifest asset count mismatch should be reported");
}

void TestReaderRejectsDuplicateAssetNames()
{
	ExpectReadIssue(
		"static-mesh-export-package-manifest format=iggy:native-static-mesh-export-package version=1 manifest=static-mesh-export-manifest.txt assets=2\n"
		"asset=cube filename=cube.igmesh\n"
		"asset=cube filename=bean.igmesh\n",
		NativeStaticMeshExportPackageManifestReadIssueCode::DuplicateAssetName,
		"duplicate package manifest asset names should be reported");
}

void TestReaderRejectsDuplicateAssetFilenames()
{
	ExpectReadIssue(
		"static-mesh-export-package-manifest format=iggy:native-static-mesh-export-package version=1 manifest=static-mesh-export-manifest.txt assets=2\n"
		"asset=cube filename=cube.igmesh\n"
		"asset=bean filename=cube.igmesh\n",
		NativeStaticMeshExportPackageManifestReadIssueCode::DuplicateAssetFilename,
		"duplicate package manifest asset filenames should be reported");
}

void TestReaderRejectsSeparatorFilenames()
{
	ExpectReadIssue(
		"static-mesh-export-package-manifest format=iggy:native-static-mesh-export-package version=1 manifest=static-mesh-export-manifest.txt assets=1\n"
		"asset=cube filename=models/cube.igmesh\n",
		NativeStaticMeshExportPackageManifestReadIssueCode::MalformedAssetRow,
		"separator-containing package manifest asset filename should be reported");
}

void TestReaderRejectsSeparatorManifestFilename()
{
	ExpectReadIssue(
		"static-mesh-export-package-manifest format=iggy:native-static-mesh-export-package version=1 manifest=exports/static-mesh-export-manifest.txt assets=0\n",
		NativeStaticMeshExportPackageManifestReadIssueCode::MalformedHeader,
		"separator-containing package manifest nested manifest filename should be reported");
}

void TestFileReaderReadsGeneratedManifest()
{
	const std::filesystem::path path = TempPackageManifestPath("generated");
	std::filesystem::remove(path);
	const NativeStaticMeshExportPackageManifestResult manifest =
		BuildNativeStaticMeshExportPackageManifestText(
			DefaultNativeStaticMeshExportPackagePolicy());
	WriteTextFile(path, manifest.text);

	const NativeStaticMeshExportPackageManifestReadResult result =
		ReadNativeStaticMeshExportPackageManifestFile(path);

	std::filesystem::remove(path);
	Expect(result.read(), "package manifest file reader should read generated text");
	Expect(
		result.document.formatId == "iggy:native-static-mesh-export-package",
		"package manifest file reader should preserve format id");
	Expect(result.document.version == 1, "package manifest file reader should preserve version");
	Expect(
		result.document.manifestFilename == "static-mesh-export-manifest.txt",
		"package manifest file reader should preserve nested manifest filename");
	Expect(result.document.assets.size() == 3, "package manifest file reader should preserve rows");
	if (result.document.assets.size() == 3) {
		Expect(result.document.assets[0].name == "cube", "file reader first row should be cube");
		Expect(
			result.document.assets[2].filename == "npc-marker.igmesh",
			"file reader last row filename should be npc-marker.igmesh");
	}
}

void TestFileReaderReportsMissingFile()
{
	const std::filesystem::path path = TempPackageManifestPath("missing");
	std::filesystem::remove(path);

	const NativeStaticMeshExportPackageManifestReadResult result =
		ReadNativeStaticMeshExportPackageManifestFile(path);

	Expect(!result.read(), "missing package manifest file should not read");
	Expect(result.issues.size() == 1, "missing package manifest file should report one issue");
	Expect(
		HasReadIssue(
			result,
			NativeStaticMeshExportPackageManifestReadIssueCode::FileOpenFailed),
		"missing package manifest file should report file-open failure");
	if (!result.issues.empty()) {
		Expect(result.issues[0].line == 0, "file-open failure should use line zero");
		Expect(
			result.issues[0].token == path.string(),
			"file-open failure should report the supplied path");
	}
}

void TestFileReaderPropagatesTextReaderIssues()
{
	const std::filesystem::path path = TempPackageManifestPath("malformed");
	std::filesystem::remove(path);
	WriteTextFile(
		path,
		"static-mesh-export-package-manifest format=bad version=1 manifest=static-mesh-export-manifest.txt assets=0\n");

	const NativeStaticMeshExportPackageManifestReadResult result =
		ReadNativeStaticMeshExportPackageManifestFile(path);

	std::filesystem::remove(path);
	Expect(!result.read(), "malformed package manifest file should not read");
	Expect(
		HasReadIssue(
			result,
			NativeStaticMeshExportPackageManifestReadIssueCode::UnsupportedFormatId),
		"malformed package manifest file should propagate text reader issue");
	Expect(
		!HasReadIssue(
			result,
			NativeStaticMeshExportPackageManifestReadIssueCode::FileOpenFailed),
		"malformed readable file should not report file-open failure");
}

void TestReadIssueCodeText()
{
	struct Case {
		NativeStaticMeshExportPackageManifestReadIssueCode code;
		const char *text;
	};

	const Case cases[] = {
		{ NativeStaticMeshExportPackageManifestReadIssueCode::FileOpenFailed, "FileOpenFailed" },
		{ NativeStaticMeshExportPackageManifestReadIssueCode::EmptyInput, "EmptyInput" },
		{ NativeStaticMeshExportPackageManifestReadIssueCode::MalformedHeader, "MalformedHeader" },
		{ NativeStaticMeshExportPackageManifestReadIssueCode::UnsupportedFormatId, "UnsupportedFormatId" },
		{ NativeStaticMeshExportPackageManifestReadIssueCode::UnsupportedVersion, "UnsupportedVersion" },
		{ NativeStaticMeshExportPackageManifestReadIssueCode::MalformedAssetCount, "MalformedAssetCount" },
		{ NativeStaticMeshExportPackageManifestReadIssueCode::MissingField, "MissingField" },
		{ NativeStaticMeshExportPackageManifestReadIssueCode::MalformedAssetRow, "MalformedAssetRow" },
		{ NativeStaticMeshExportPackageManifestReadIssueCode::AssetCountMismatch, "AssetCountMismatch" },
		{ NativeStaticMeshExportPackageManifestReadIssueCode::DuplicateAssetName, "DuplicateAssetName" },
		{ NativeStaticMeshExportPackageManifestReadIssueCode::DuplicateAssetFilename, "DuplicateAssetFilename" },
		{ NativeStaticMeshExportPackageManifestReadIssueCode::ExtraToken, "ExtraToken" },
		{ NativeStaticMeshExportPackageManifestReadIssueCode::UnexpectedLine, "UnexpectedLine" },
	};

	for (const Case &testCase : cases) {
		Expect(
			std::string(NativeStaticMeshExportPackageManifestReadIssueCodeText(
				testCase.code)) == testCase.text,
			"package manifest read issue code text should match stable spelling");
	}
	Expect(
		std::string(NativeStaticMeshExportPackageManifestReadIssueCodeText(
			static_cast<NativeStaticMeshExportPackageManifestReadIssueCode>(999))) ==
			"Unknown",
		"package manifest read issue code text should report unknown fallback");
}

} // namespace

int main()
{
	TestDefaultPackageManifestBuilds();
	TestDefaultPackageManifestIncludesStableSummary();
	TestAssetRowsFollowNestedMeshPolicyOrder();
	TestRepeatedCallsAreDeterministic();
	TestInvalidPolicyReturnsFailureWithoutText();
	TestBuilderDoesNotNeedFilesystemState();
	TestDefaultPackageManifestReadsBack();
	TestReaderRejectsEmptyInput();
	TestReaderRejectsBadHeaderToken();
	TestReaderRejectsUnsupportedFormat();
	TestReaderRejectsUnsupportedVersion();
	TestReaderRejectsMalformedAssetCount();
	TestReaderRejectsMissingHeaderFields();
	TestReaderRejectsExtraHeaderTokens();
	TestReaderRejectsMissingAssetFields();
	TestReaderRejectsExtraAssetTokens();
	TestReaderRejectsMalformedAssetRow();
	TestReaderRejectsUnexpectedLine();
	TestReaderRejectsAssetCountMismatch();
	TestReaderRejectsDuplicateAssetNames();
	TestReaderRejectsDuplicateAssetFilenames();
	TestReaderRejectsSeparatorFilenames();
	TestReaderRejectsSeparatorManifestFilename();
	TestFileReaderReadsGeneratedManifest();
	TestFileReaderReportsMissingFile();
	TestFileReaderPropagatesTextReaderIssues();
	TestReadIssueCodeText();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
