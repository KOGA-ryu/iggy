#include "../apps/native_play/NativeStaticMeshAssetLoader.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include "support/TestHarness.hpp"

#ifndef IGGY_NATIVE_PLAY_TEST_ASSET_DIR
#error "IGGY_NATIVE_PLAY_TEST_ASSET_DIR must point at apps/native_play/assets"
#endif

namespace {

using iggy::native_play::LoadNativeStaticMeshAssetFile;
using iggy::native_play::LoadNativeStaticMeshAssetText;
using iggy::native_play::NativeStaticMeshAssetLoadIssueCode;
using iggy::native_play::NativeStaticMeshAssetLoadIssueCodeText;
using iggy::native_play::NativeStaticMeshAssetLoadResult;
using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::Near;

std::filesystem::path TempRoot()
{
	return std::filesystem::current_path() / "native_static_mesh_asset_loader_tests_tmp";
}

std::filesystem::path TempPath(const char *name)
{
	return TempRoot() / name;
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
	std::ofstream stream(path, std::ios::binary | std::ios::trunc);
	stream << text;
}

bool HasIssue(
	const NativeStaticMeshAssetLoadResult &result,
	NativeStaticMeshAssetLoadIssueCode code)
{
	for (const auto &issue : result.issues) {
		if (issue.code == code)
			return true;
	}
	return false;
}

void TestLoadIssueCodeText()
{
	Expect(
		std::string { NativeStaticMeshAssetLoadIssueCodeText(
			NativeStaticMeshAssetLoadIssueCode::FileOpenFailed) } == "FileOpenFailed",
		"file-open loader issue text should match");
	Expect(
		std::string { NativeStaticMeshAssetLoadIssueCodeText(
			NativeStaticMeshAssetLoadIssueCode::UnknownDirective) } == "UnknownDirective",
		"unknown-directive loader issue text should match");
	Expect(
		std::string { NativeStaticMeshAssetLoadIssueCodeText(
			NativeStaticMeshAssetLoadIssueCode::MalformedVertex) } == "MalformedVertex",
		"malformed-vertex loader issue text should match");
	Expect(
		std::string { NativeStaticMeshAssetLoadIssueCodeText(
			NativeStaticMeshAssetLoadIssueCode::MalformedTriangle) } == "MalformedTriangle",
		"malformed-triangle loader issue text should match");
	Expect(
		std::string { NativeStaticMeshAssetLoadIssueCodeText(
			NativeStaticMeshAssetLoadIssueCode::IndexOutOfRange) } == "IndexOutOfRange",
		"index-range loader issue text should match");
	Expect(
		std::string { NativeStaticMeshAssetLoadIssueCodeText(
			NativeStaticMeshAssetLoadIssueCode::ExtraToken) } == "ExtraToken",
		"extra-token loader issue text should match");
	Expect(
		std::string { NativeStaticMeshAssetLoadIssueCodeText(
			NativeStaticMeshAssetLoadIssueCode::InvalidMesh) } == "InvalidMesh",
		"invalid-mesh loader issue text should match");
	Expect(
		std::string { NativeStaticMeshAssetLoadIssueCodeText(
			static_cast<NativeStaticMeshAssetLoadIssueCode>(999)) } == "Unknown",
		"unknown loader issue text should use fallback");
}

void TestLoadsTriangleFromText()
{
	const NativeStaticMeshAssetLoadResult result = LoadNativeStaticMeshAssetText(R"(
# vertex color static mesh
v 0 0 0 1 0 0
v 1 0 0 0 1 0
v 0 1 0 0 0 1
tri 0 1 2
)");

	Expect(result.loaded(), "valid triangle text should load");
	Expect(result.asset.vertices.size() == 3, "valid triangle should have three vertices");
	Expect(result.asset.indices.size() == 3, "valid triangle should have three indices");
	if (result.asset.vertices.size() == 3) {
		Expect(Near(result.asset.vertices[1].position.x, 1.0F), "loader should preserve vertex position");
		Expect(Near(result.asset.vertices[2].color[2], 1.0F), "loader should preserve vertex color");
	}
	if (result.asset.indices.size() == 3) {
		Expect(result.asset.indices[0] == 0, "loader should preserve first index");
		Expect(result.asset.indices[1] == 1, "loader should preserve second index");
		Expect(result.asset.indices[2] == 2, "loader should preserve third index");
	}
}

void TestLoadsTriangleFromFile()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("triangle.igmesh");
	WriteText(path, R"(
v -1 0 0 0.5 0.6 0.7
v 0 1 0 0.7 0.6 0.5
v 1 0 0 0.4 0.5 0.6
tri 0 1 2
)");

	const NativeStaticMeshAssetLoadResult result = LoadNativeStaticMeshAssetFile(path);

	Expect(result.loaded(), "valid triangle file should load");
	Expect(result.asset.vertices.size() == 3, "valid triangle file should preserve vertices");
	Expect(result.asset.indices.size() == 3, "valid triangle file should preserve indices");
	CleanupTempRoot();
}

void TestLoadsNativePlayerAssetFixture()
{
	const std::filesystem::path path =
		std::filesystem::path(IGGY_NATIVE_PLAY_TEST_ASSET_DIR) / "player.igmesh";

	const NativeStaticMeshAssetLoadResult result = LoadNativeStaticMeshAssetFile(path);

	Expect(result.loaded(), "checked-in native player asset should load");
	Expect(result.asset.vertices.size() == 6, "native player asset should preserve vertex count");
	Expect(result.asset.indices.size() == 24, "native player asset should preserve index count");
}

void TestLoadsNativeNpcAssetFixture()
{
	const std::filesystem::path path =
		std::filesystem::path(IGGY_NATIVE_PLAY_TEST_ASSET_DIR) / "npc.igmesh";

	const NativeStaticMeshAssetLoadResult result = LoadNativeStaticMeshAssetFile(path);

	Expect(result.loaded(), "checked-in native NPC asset should load");
	Expect(result.asset.vertices.size() == 7, "native NPC asset should preserve vertex count");
	Expect(result.asset.indices.size() == 30, "native NPC asset should preserve index count");
}

void TestLoadsNativeFloorAssetFixture()
{
	const std::filesystem::path path =
		std::filesystem::path(IGGY_NATIVE_PLAY_TEST_ASSET_DIR) / "floor.igmesh";

	const NativeStaticMeshAssetLoadResult result = LoadNativeStaticMeshAssetFile(path);

	Expect(result.loaded(), "checked-in native floor asset should load");
	Expect(result.asset.vertices.size() == 4, "native floor asset should preserve vertex count");
	Expect(result.asset.indices.size() == 6, "native floor asset should preserve index count");
}

void TestLoadsNativeWallAssetFixture()
{
	const std::filesystem::path path =
		std::filesystem::path(IGGY_NATIVE_PLAY_TEST_ASSET_DIR) / "wall.igmesh";

	const NativeStaticMeshAssetLoadResult result = LoadNativeStaticMeshAssetFile(path);

	Expect(result.loaded(), "checked-in native wall asset should load");
	Expect(result.asset.vertices.size() == 8, "native wall asset should preserve vertex count");
	Expect(result.asset.indices.size() == 36, "native wall asset should preserve index count");
}

void TestMissingFileReportsOpenFailure()
{
	ResetTempRoot();

	const NativeStaticMeshAssetLoadResult result =
		LoadNativeStaticMeshAssetFile(TempPath("missing.igmesh"));

	Expect(!result.loaded(), "missing file should not load");
	Expect(HasIssue(result, NativeStaticMeshAssetLoadIssueCode::FileOpenFailed), "missing file should report open failure");
	CleanupTempRoot();
}

void TestUnknownDirectiveReportsIssue()
{
	const NativeStaticMeshAssetLoadResult result = LoadNativeStaticMeshAssetText(R"(
mesh static
v 0 0 0 1 1 1
tri 0 0 0
)");

	Expect(!result.loaded(), "unknown directive input should not load");
	Expect(HasIssue(result, NativeStaticMeshAssetLoadIssueCode::UnknownDirective), "unknown directive should be reported");
}

void TestMalformedVertexAndTriangleReportIssues()
{
	const NativeStaticMeshAssetLoadResult result = LoadNativeStaticMeshAssetText(R"(
v 0 0 0 1 1
tri 0 1
)");

	Expect(!result.loaded(), "malformed records should not load");
	Expect(HasIssue(result, NativeStaticMeshAssetLoadIssueCode::MalformedVertex), "malformed vertex should be reported");
	Expect(HasIssue(result, NativeStaticMeshAssetLoadIssueCode::MalformedTriangle), "malformed triangle should be reported");
}

void TestExtraTokensReportIssue()
{
	const NativeStaticMeshAssetLoadResult result = LoadNativeStaticMeshAssetText(R"(
v 0 0 0 1 1 1 extra
tri 0 0 0
)");

	Expect(!result.loaded(), "extra tokens should not load");
	Expect(HasIssue(result, NativeStaticMeshAssetLoadIssueCode::ExtraToken), "extra token should be reported");
}

void TestIndexRangeAndInvalidMeshReportIssues()
{
	const NativeStaticMeshAssetLoadResult negative = LoadNativeStaticMeshAssetText(R"(
v 0 0 0 1 1 1
tri -1 0 0
)");
	Expect(!negative.loaded(), "negative index should not load");
	Expect(HasIssue(negative, NativeStaticMeshAssetLoadIssueCode::IndexOutOfRange), "negative index should report index range issue");

	const NativeStaticMeshAssetLoadResult invalid = LoadNativeStaticMeshAssetText(R"(
v 0 0 0 1 1 1
tri 0 1 0
)");
	Expect(!invalid.loaded(), "index beyond vertex count should not load");
	Expect(HasIssue(invalid, NativeStaticMeshAssetLoadIssueCode::InvalidMesh), "invalid mesh should be reported");
}

} // namespace

int main()
{
	TestLoadIssueCodeText();
	TestLoadsTriangleFromText();
	TestLoadsTriangleFromFile();
	TestLoadsNativePlayerAssetFixture();
	TestLoadsNativeNpcAssetFixture();
	TestLoadsNativeFloorAssetFixture();
	TestLoadsNativeWallAssetFixture();
	TestMissingFileReportsOpenFailure();
	TestUnknownDirectiveReportsIssue();
	TestMalformedVertexAndTriangleReportIssues();
	TestExtraTokensReportIssue();
	TestIndexRangeAndInvalidMeshReportIssues();

	CleanupTempRoot();
	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
