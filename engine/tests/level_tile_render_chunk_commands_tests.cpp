#include <cstdlib>
#include <string_view>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/camera/CameraView.hpp"
#include "scene/level/LevelTileRenderChunkCache.hpp"
#include "scene/level/LevelTileRenderChunkCommands.hpp"
#include "scene/level/LevelTileRenderChunkVisibility.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

const iggy::ResourceId WalkableMaterial { "material:floor" };
const iggy::ResourceId BlockedMaterial { "material:wall" };

bool SameBounds(iggy::Aabb2 actual, iggy::Aabb2 expected)
{
	return NearVec(actual.min, expected.min) && NearVec(actual.max, expected.max);
}

bool SameRect(iggy::Rect2 actual, iggy::Rect2 expected)
{
	return NearVec(actual.position, expected.position) && NearVec(actual.size, expected.size);
}

iggy::LevelTileRenderChunkCache CacheFromRows(std::vector<std::string_view> rows, int chunkWidth = 2, int chunkHeight = 2)
{
	const iggy::LevelTileMap map = iggy::test::MapFromRows(rows);
	const iggy::LevelTileRenderChunkCacheBuildResult build = iggy::LevelTileRenderChunkCacheBuilder {}.build(map, { chunkWidth, chunkHeight, { { WalkableMaterial, BlockedMaterial }, 3 } });
	return build.cache;
}

iggy::LevelTileRenderChunkCommandResult Build(const iggy::LevelTileRenderChunkCache &cache, std::vector<std::size_t> indexes)
{
	return iggy::LevelTileRenderChunkCommands {}.build(cache, indexes);
}

void ExpectCommand(const iggy::render::RenderCommand2D &command, iggy::Aabb2 bounds, const iggy::ResourceId &materialId, int layer, std::size_t order, const char *message)
{
	Expect(command.type == iggy::render::RenderCommand2DType::Quad && SameBounds(command.worldBounds, bounds) && command.materialId == materialId && command.layer == layer && command.order == order, message);
}

void ExpectNoTexturePayload(const iggy::render::RenderCommand2D &command, const char *message)
{
	Expect(command.texture.textureId.empty() && !command.texture.hasSourceRect, message);
}

void ExpectTexturePayload(const iggy::render::RenderCommand2D &command, const iggy::ResourceId &textureId, iggy::Rect2 sourceRect, const char *message)
{
	Expect(command.texture.textureId == textureId && SameRect(command.texture.sourceRect, sourceRect) && command.texture.hasSourceRect, message);
}

iggy::LevelTileRenderChunkCache CustomTextureCache()
{
	iggy::LevelTileRenderChunkCache cache;
	iggy::LevelTileRenderChunk chunk;
	chunk.coord = { 0, 0 };
	chunk.minTile = { 0, 0 };
	chunk.maxTile = { 0, 0 };
	chunk.worldBounds = { { 0.0F, 0.0F }, { 1.0F, 1.0F } };
	iggy::render::RenderCommandListBuilder2D {}.addTexturedQuad(chunk.commands, { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, iggy::ResourceId { "material:sprite" }, iggy::ResourceId { "texture:hero" }, { { 8.0F, 16.0F }, { 24.0F, 32.0F } }, 9);
	cache.chunks.push_back(chunk);
	return cache;
}

void TestEmptyCacheAndIndexesReturnsEmpty()
{
	const iggy::LevelTileRenderChunkCache cache;

	const iggy::LevelTileRenderChunkCommandResult result = Build(cache, {});

	Expect(result.commands.commands.empty(), "empty cache and index list should return empty commands");
	Expect(result.usedChunkIndexes.empty(), "empty cache and index list should return no used indexes");
	Expect(result.skippedChunkIndexes.empty(), "empty cache and index list should return no skipped indexes");
}

void TestEmptyIndexesWithNonEmptyCacheReturnsEmpty()
{
	const iggy::LevelTileRenderChunkCache cache = CacheFromRows({
		"..",
		"..",
	});

	const iggy::LevelTileRenderChunkCommandResult result = Build(cache, {});

	Expect(result.commands.commands.empty(), "empty index list should not append cache commands");
	Expect(result.usedChunkIndexes.empty(), "empty index list should return no used indexes");
	Expect(result.skippedChunkIndexes.empty(), "empty index list should return no skipped indexes");
}

void TestOneValidChunkIndexReturnsCommands()
{
	const iggy::LevelTileRenderChunkCache cache = CacheFromRows({
		"..",
		"..",
	});

	const iggy::LevelTileRenderChunkCommandResult result = Build(cache, { 0 });

	Expect(result.usedChunkIndexes == std::vector<std::size_t> { 0 }, "one valid chunk should record used index");
	Expect(result.skippedChunkIndexes.empty(), "one valid chunk should not record skipped indexes");
	Expect(result.commands.commands.size() == 4, "2x2 chunk should append four commands");
	if (result.commands.commands.size() == 4)
		ExpectCommand(result.commands.commands[0], { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, WalkableMaterial, 3, 0, "first chunk command should normalize to order 0");
}

void TestMultipleValidIndexesPreserveSuppliedOrder()
{
	const iggy::LevelTileRenderChunkCache cache = CacheFromRows({
		"....",
		"....",
	}, 2, 2);

	const iggy::LevelTileRenderChunkCommandResult result = Build(cache, { 1, 0 });

	Expect(result.usedChunkIndexes == std::vector<std::size_t>({ 1, 0 }), "used chunk indexes should preserve supplied order");
	Expect(result.commands.commands.size() == 8, "two 2x2 chunks should append eight commands");
	if (result.commands.commands.size() == 8) {
		ExpectCommand(result.commands.commands[0], { { 2.0F, 0.0F }, { 3.0F, 1.0F } }, WalkableMaterial, 3, 0, "first output command should come from supplied chunk 1");
		ExpectCommand(result.commands.commands[4], { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, WalkableMaterial, 3, 4, "second chunk output should start with normalized order 4");
		Expect(result.commands.commands[7].order == 7, "final composed command order should be continuous");
	}
}

void TestInvalidIndexesAreSkippedAndRecorded()
{
	const iggy::LevelTileRenderChunkCache cache = CacheFromRows({
		"..",
		"..",
	});

	const iggy::LevelTileRenderChunkCommandResult result = Build(cache, { 4, 0, 9 });

	Expect(result.usedChunkIndexes == std::vector<std::size_t> { 0 }, "valid indexes should still be used when invalid indexes are present");
	Expect(result.skippedChunkIndexes == std::vector<std::size_t>({ 4, 9 }), "invalid indexes should be recorded in supplied order");
	Expect(result.commands.commands.size() == 4, "invalid indexes should not add commands");
}

void TestDuplicateValidIndexesDuplicateCommands()
{
	const iggy::LevelTileRenderChunkCache cache = CacheFromRows({
		"..",
		"..",
	});

	const iggy::LevelTileRenderChunkCommandResult result = Build(cache, { 0, 0 });

	Expect(result.usedChunkIndexes == std::vector<std::size_t>({ 0, 0 }), "duplicate valid indexes should be recorded twice");
	Expect(result.commands.commands.size() == 8, "duplicate valid indexes should duplicate command output");
	if (result.commands.commands.size() == 8) {
		ExpectCommand(result.commands.commands[0], { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, WalkableMaterial, 3, 0, "first duplicate pass should start at order 0");
		ExpectCommand(result.commands.commands[4], { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, WalkableMaterial, 3, 4, "second duplicate pass should get fresh normalized order");
	}
}

void TestTexturePayloadIsPreservedFromCustomCache()
{
	const iggy::LevelTileRenderChunkCache cache = CustomTextureCache();

	const iggy::LevelTileRenderChunkCommandResult result = Build(cache, { 0 });

	Expect(result.commands.commands.size() == 1, "custom textured cache should append one command");
	if (result.commands.commands.size() == 1) {
		ExpectCommand(result.commands.commands[0], { { 0.0F, 0.0F }, { 1.0F, 1.0F } }, iggy::ResourceId { "material:sprite" }, 9, 0, "textured chunk command should preserve command fields");
		ExpectTexturePayload(result.commands.commands[0], iggy::ResourceId { "texture:hero" }, { { 8.0F, 16.0F }, { 24.0F, 32.0F } }, "textured chunk command should preserve texture payload");
	}
}

void TestMaterialOnlyTileChunkPayloadRemainsEmpty()
{
	const iggy::LevelTileRenderChunkCache cache = CacheFromRows({
		".#",
	}, 2, 1);

	const iggy::LevelTileRenderChunkCommandResult result = Build(cache, { 0 });

	Expect(result.commands.commands.size() == 2, "material-only tile chunk should append two commands");
	if (result.commands.commands.size() == 2) {
		ExpectNoTexturePayload(result.commands.commands[0], "walkable tile chunk command should remain material-only");
		ExpectNoTexturePayload(result.commands.commands[1], "blocked tile chunk command should remain material-only");
	}
}

void TestVisibilityOverloadComposesVisibleChunkCommands()
{
	const iggy::LevelTileRenderChunkCache cache = CacheFromRows({
		"....",
		"....",
		"....",
		"....",
	}, 2, 2);
	const iggy::CameraViewResult view = iggy::CameraView {}.visibleWorldBounds({ { 1.0F, 1.0F } }, { { 2.0F, 2.0F }, 1.0F });
	const iggy::LevelTileRenderChunkVisibilityResult visibility = iggy::LevelTileRenderChunkVisibility {}.query(cache, view.bounds);

	const iggy::LevelTileRenderChunkCommandResult result = iggy::LevelTileRenderChunkCommands {}.build(cache, visibility);

	Expect(visibility.chunkIndexes == std::vector<std::size_t> { 0 }, "visibility setup should select first chunk");
	Expect(result.usedChunkIndexes == visibility.chunkIndexes, "visibility overload should use visible chunk indexes");
	Expect(result.commands.commands.size() == 4, "visible 2x2 chunk should append four commands");
}

void TestBuildDoesNotMutateCacheCommands()
{
	iggy::LevelTileRenderChunkCache cache = CustomTextureCache();
	const std::size_t originalOrder = cache.chunks[0].commands.commands[0].order;
	const iggy::ResourceId originalMaterial = cache.chunks[0].commands.commands[0].materialId;
	const iggy::ResourceId originalTexture = cache.chunks[0].commands.commands[0].texture.textureId;
	const iggy::Rect2 originalSourceRect = cache.chunks[0].commands.commands[0].texture.sourceRect;
	const bool originalHasSourceRect = cache.chunks[0].commands.commands[0].texture.hasSourceRect;

	const iggy::LevelTileRenderChunkCommandResult result = Build(cache, { 0, 0 });

	Expect(result.commands.commands.size() == 2, "immutability setup should append commands");
	Expect(cache.chunks[0].commands.commands[0].order == originalOrder, "chunk command composer should not mutate source command order");
	Expect(cache.chunks[0].commands.commands[0].materialId == originalMaterial, "chunk command composer should not mutate source material");
	Expect(cache.chunks[0].commands.commands[0].texture.textureId == originalTexture, "chunk command composer should not mutate source texture id");
	Expect(SameRect(cache.chunks[0].commands.commands[0].texture.sourceRect, originalSourceRect), "chunk command composer should not mutate source rect");
	Expect(cache.chunks[0].commands.commands[0].texture.hasSourceRect == originalHasSourceRect, "chunk command composer should not mutate source texture flag");
}

} // namespace

int main()
{
	TestEmptyCacheAndIndexesReturnsEmpty();
	TestEmptyIndexesWithNonEmptyCacheReturnsEmpty();
	TestOneValidChunkIndexReturnsCommands();
	TestMultipleValidIndexesPreserveSuppliedOrder();
	TestInvalidIndexesAreSkippedAndRecorded();
	TestDuplicateValidIndexesDuplicateCommands();
	TestTexturePayloadIsPreservedFromCustomCache();
	TestMaterialOnlyTileChunkPayloadRemainsEmpty();
	TestVisibilityOverloadComposesVisibleChunkCommands();
	TestBuildDoesNotMutateCacheCommands();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
