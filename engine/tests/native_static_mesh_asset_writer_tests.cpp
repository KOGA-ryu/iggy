#include "../apps/native_play/NativeStaticMeshAssetLoader.hpp"
#include "../apps/native_play/NativeStaticMeshAssetWriter.hpp"

#include <cstdlib>
#include <string>

#include "support/TestHarness.hpp"

namespace {

using iggy::native_play::LoadNativeStaticMeshAssetText;
using iggy::native_play::NativeBeanStaticMeshAsset;
using iggy::native_play::NativeCubeStaticMeshAsset;
using iggy::native_play::NativeStaticMeshAsset;
using iggy::native_play::NativeStaticMeshAssetLoadResult;
using iggy::native_play::NativeStaticMeshAssetWriteIssueCode;
using iggy::native_play::NativeStaticMeshAssetWriteResult;
using iggy::native_play::WriteNativeStaticMeshAssetText;
using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::Near;

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
	TestInvalidEmptyMeshReportsIssueAndNoText();
	TestNonTriangleIndexCountReportsIssueAndNoText();
	TestWriterOutputIsDeterministicAcrossCalls();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
