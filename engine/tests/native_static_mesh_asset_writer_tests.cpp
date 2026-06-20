#include "../apps/native_play/NativeStaticMeshAssetLoader.hpp"
#include "../apps/native_play/NativeStaticMeshAssetWriter.hpp"

#include <cstdlib>
#include <filesystem>
#include <string>

#include "support/TestHarness.hpp"

namespace {

using iggy::native_play::BuildNativeStaticMeshAssetWriteFailureText;
using iggy::native_play::LoadNativeStaticMeshAssetFile;
using iggy::native_play::LoadNativeStaticMeshAssetText;
using iggy::native_play::NativeBeanStaticMeshAsset;
using iggy::native_play::NativeCubeStaticMeshAsset;
using iggy::native_play::NativeNpcMarkerStaticMeshAsset;
using iggy::native_play::NativeStaticMeshAsset;
using iggy::native_play::NativeStaticMeshAssetLoadResult;
using iggy::native_play::NativeStaticMeshAssetWriteIssueCode;
using iggy::native_play::NativeStaticMeshAssetWriteIssueCodeText;
using iggy::native_play::NativeStaticMeshAssetWriteResult;
using iggy::native_play::WriteNativeStaticMeshAssetText;
using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::Near;

#ifndef IGGY_NATIVE_PLAY_TEST_ASSET_DIR
#define IGGY_NATIVE_PLAY_TEST_ASSET_DIR ""
#endif

std::filesystem::path AssetRoot()
{
	return std::filesystem::path { IGGY_NATIVE_PLAY_TEST_ASSET_DIR };
}

NativeStaticMeshAsset TriangleMesh()
{
	NativeStaticMeshAsset asset;
	asset.vertices = {
		{ { 0.0F, 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F } },
		{ { 1.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } },
		{ { 0.0F, 1.0F, 0.0F }, { 0.0F, 0.0F, 1.0F } },
	};
	asset.indices = { 0, 1, 2 };
	return asset;
}

bool HasIssue(
	const NativeStaticMeshAssetWriteResult &result,
	NativeStaticMeshAssetWriteIssueCode code)
{
	for (const auto &issue : result.issues) {
		if (issue.code == code)
			return true;
	}
	return false;
}

void ExpectAssetFixtureWriterRoundtrip(
	const char *filename,
	std::size_t expectedVertexCount,
	std::size_t expectedIndexCount)
{
	const NativeStaticMeshAssetLoadResult loaded =
		LoadNativeStaticMeshAssetFile(AssetRoot() / filename);
	Expect(loaded.loaded(), std::string { filename } + " should load");
	Expect(
		loaded.asset.vertices.size() == expectedVertexCount,
		std::string { filename } + " should have expected vertex count");
	Expect(
		loaded.asset.indices.size() == expectedIndexCount,
		std::string { filename } + " should have expected index count");

	const NativeStaticMeshAssetWriteResult firstWrite =
		WriteNativeStaticMeshAssetText(loaded.asset);
	Expect(firstWrite.written(), std::string { filename } + " should write");

	const NativeStaticMeshAssetLoadResult reloaded =
		LoadNativeStaticMeshAssetText(firstWrite.text);
	Expect(reloaded.loaded(), std::string { filename } + " written text should reload");
	Expect(
		reloaded.asset.vertices.size() == expectedVertexCount,
		std::string { filename } + " roundtrip should preserve vertex count");
	Expect(
		reloaded.asset.indices.size() == expectedIndexCount,
		std::string { filename } + " roundtrip should preserve index count");

	const NativeStaticMeshAssetWriteResult secondWrite =
		WriteNativeStaticMeshAssetText(reloaded.asset);
	Expect(
		secondWrite.written(),
		std::string { filename } + " second write should succeed");
	Expect(
		firstWrite.text == secondWrite.text,
		std::string { filename } + " writer output should be canonical after reload");
}

void TestValidTriangleWritesDeterministicTextAndReloads()
{
	const NativeStaticMeshAssetWriteResult result =
		WriteNativeStaticMeshAssetText(TriangleMesh());

	Expect(result.written(), "valid triangle should write");
	const std::string expected =
		"# Native static mesh asset\n"
		"v 0 0 0 1 0 0\n"
		"v 1 0 0 0 1 0\n"
		"v 0 1 0 0 0 1\n"
		"tri 0 1 2\n";
	Expect(result.text == expected, "valid triangle should write deterministic text");

	const NativeStaticMeshAssetLoadResult loaded =
		LoadNativeStaticMeshAssetText(result.text);
	Expect(loaded.loaded(), "written triangle should reload");
	Expect(loaded.asset.vertices.size() == 3, "written triangle should preserve vertices");
	Expect(loaded.asset.indices.size() == 3, "written triangle should preserve indices");
	if (loaded.asset.vertices.size() == 3) {
		Expect(Near(loaded.asset.vertices[1].position.x, 1.0F), "reloaded triangle should preserve position");
		Expect(Near(loaded.asset.vertices[2].color[2], 1.0F), "reloaded triangle should preserve color");
	}
}

void TestCubeWritesAndReloadsRepresentativeData()
{
	const NativeStaticMeshAsset cube = NativeCubeStaticMeshAsset();
	const NativeStaticMeshAssetWriteResult result =
		WriteNativeStaticMeshAssetText(cube);

	Expect(result.written(), "cube should write");
	const NativeStaticMeshAssetLoadResult loaded =
		LoadNativeStaticMeshAssetText(result.text);
	Expect(loaded.loaded(), "written cube should reload");
	Expect(loaded.asset.vertices.size() == cube.vertices.size(), "cube roundtrip should preserve vertex count");
	Expect(loaded.asset.indices.size() == cube.indices.size(), "cube roundtrip should preserve index count");
	if (!loaded.asset.indices.empty()) {
		Expect(loaded.asset.indices[0] == cube.indices[0], "cube roundtrip should preserve first index");
		Expect(
			loaded.asset.indices.back() == cube.indices.back(),
			"cube roundtrip should preserve last index");
	}
	if (!loaded.asset.vertices.empty()) {
		Expect(
			Near(loaded.asset.vertices[0].color[0], cube.vertices[0].color[0]),
			"cube roundtrip should preserve representative color");
	}
}

void TestProceduralBeanWritesAndReloadsCounts()
{
	const NativeStaticMeshAsset bean = NativeBeanStaticMeshAsset();
	const NativeStaticMeshAssetWriteResult result =
		WriteNativeStaticMeshAssetText(bean);

	Expect(result.written(), "procedural bean should write");
	const NativeStaticMeshAssetLoadResult loaded =
		LoadNativeStaticMeshAssetText(result.text);
	Expect(loaded.loaded(), "written procedural bean should reload");
	Expect(loaded.asset.vertices.size() == bean.vertices.size(), "bean roundtrip should preserve vertex count");
	Expect(loaded.asset.indices.size() == bean.indices.size(), "bean roundtrip should preserve index count");
}

void TestProceduralNpcMarkerWritesAndReloadsCounts()
{
	const NativeStaticMeshAsset npc = NativeNpcMarkerStaticMeshAsset();
	const NativeStaticMeshAssetWriteResult result =
		WriteNativeStaticMeshAssetText(npc);

	Expect(result.written(), "procedural NPC marker should write");
	const NativeStaticMeshAssetLoadResult loaded =
		LoadNativeStaticMeshAssetText(result.text);
	Expect(loaded.loaded(), "written procedural NPC marker should reload");
	Expect(loaded.asset.vertices.size() == npc.vertices.size(), "NPC marker roundtrip should preserve vertex count");
	Expect(loaded.asset.indices.size() == npc.indices.size(), "NPC marker roundtrip should preserve index count");
}

void TestCheckedInFixtureAssetsRoundtripThroughWriter()
{
	ExpectAssetFixtureWriterRoundtrip("floor.igmesh", 4, 6);
	ExpectAssetFixtureWriterRoundtrip("wall.igmesh", 8, 36);
	ExpectAssetFixtureWriterRoundtrip("npc.igmesh", 7, 30);
	ExpectAssetFixtureWriterRoundtrip("player.igmesh", 6, 24);
}

void TestWriteIssueCodeText()
{
	Expect(
		std::string { NativeStaticMeshAssetWriteIssueCodeText(
			NativeStaticMeshAssetWriteIssueCode::InvalidMesh) } == "InvalidMesh",
		"invalid mesh write issue text should match");
	Expect(
		std::string { NativeStaticMeshAssetWriteIssueCodeText(
			NativeStaticMeshAssetWriteIssueCode::NonTriangleIndexCount) } ==
			"NonTriangleIndexCount",
		"non-triangle write issue text should match");
	Expect(
		std::string { NativeStaticMeshAssetWriteIssueCodeText(
			static_cast<NativeStaticMeshAssetWriteIssueCode>(999)) } == "Unknown",
		"unknown write issue text should use fallback");
}

void TestInvalidEmptyMeshReportsIssueAndNoText()
{
	const NativeStaticMeshAssetWriteResult result =
		WriteNativeStaticMeshAssetText({});

	Expect(!result.written(), "invalid empty mesh should not write");
	Expect(result.text.empty(), "invalid empty mesh should not produce text");
	Expect(
		HasIssue(result, NativeStaticMeshAssetWriteIssueCode::InvalidMesh),
		"invalid empty mesh should report invalid mesh issue");
}

void TestInvalidEmptyMeshFailureText()
{
	const NativeStaticMeshAssetWriteResult result =
		WriteNativeStaticMeshAssetText({});

	Expect(!result.written(), "invalid empty mesh failure text test should not write");
	Expect(
		BuildNativeStaticMeshAssetWriteFailureText("empty", result) ==
			"failed to write static mesh asset: empty issues=1",
		"invalid empty mesh failure text should include name and issue count");
}

void TestNonTriangleIndexCountReportsIssueAndNoText()
{
	NativeStaticMeshAsset asset;
	asset.vertices = {
		{ { 0.0F, 0.0F, 0.0F }, { 1.0F, 1.0F, 1.0F } },
		{ { 1.0F, 0.0F, 0.0F }, { 1.0F, 1.0F, 1.0F } },
	};
	asset.indices = { 0, 1 };

	const NativeStaticMeshAssetWriteResult result =
		WriteNativeStaticMeshAssetText(asset);

	Expect(!result.written(), "non-triangle index mesh should not write");
	Expect(result.text.empty(), "non-triangle index mesh should not write rows");
	Expect(
		HasIssue(result, NativeStaticMeshAssetWriteIssueCode::NonTriangleIndexCount),
		"non-triangle index mesh should report non-triangle issue");
	Expect(
		!HasIssue(result, NativeStaticMeshAssetWriteIssueCode::InvalidMesh),
		"non-triangle index mesh with valid indices should not report invalid mesh");
}

void TestNonTriangleIndexCountFailureText()
{
	NativeStaticMeshAsset asset;
	asset.vertices = {
		{ { 0.0F, 0.0F, 0.0F }, { 1.0F, 1.0F, 1.0F } },
		{ { 1.0F, 0.0F, 0.0F }, { 1.0F, 1.0F, 1.0F } },
	};
	asset.indices = { 0, 1 };

	const NativeStaticMeshAssetWriteResult result =
		WriteNativeStaticMeshAssetText(asset);

	Expect(!result.written(), "non-triangle failure text test should not write");
	Expect(
		BuildNativeStaticMeshAssetWriteFailureText("line", result) ==
			"failed to write static mesh asset: line issues=1",
		"non-triangle failure text should include name and issue count");
}

void TestWriterOutputIsDeterministicAcrossCalls()
{
	const NativeStaticMeshAsset cube = NativeCubeStaticMeshAsset();
	const NativeStaticMeshAssetWriteResult first =
		WriteNativeStaticMeshAssetText(cube);
	const NativeStaticMeshAssetWriteResult second =
		WriteNativeStaticMeshAssetText(cube);

	Expect(first.written(), "first deterministic write should succeed");
	Expect(second.written(), "second deterministic write should succeed");
	Expect(first.text == second.text, "writer output should be deterministic across calls");
}

} // namespace

int main()
{
	TestValidTriangleWritesDeterministicTextAndReloads();
	TestCubeWritesAndReloadsRepresentativeData();
	TestProceduralBeanWritesAndReloadsCounts();
	TestProceduralNpcMarkerWritesAndReloadsCounts();
	TestCheckedInFixtureAssetsRoundtripThroughWriter();
	TestWriteIssueCodeText();
	TestInvalidEmptyMeshReportsIssueAndNoText();
	TestInvalidEmptyMeshFailureText();
	TestNonTriangleIndexCountReportsIssueAndNoText();
	TestNonTriangleIndexCountFailureText();
	TestWriterOutputIsDeterministicAcrossCalls();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
