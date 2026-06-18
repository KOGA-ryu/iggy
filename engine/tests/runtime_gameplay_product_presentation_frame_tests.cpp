#include "runtime/RuntimeGameplayProductPresentationFrame.hpp"

#include <cstdlib>
#include <string_view>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/level/LevelTileRenderChunkCache.hpp"
#include "support/GeometryAssertions.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;
using iggy::test::SameBounds;

const iggy::ResourceId WalkableMaterial { "material:floor" };
const iggy::ResourceId BlockedMaterial { "material:wall" };
const iggy::ResourceId NpcMaterial { "material:npc" };

using PresentationStatus =
	iggy::runtime::RuntimeGameplayProductPresentationFrameStatus;

iggy::npc_ai::NpcAgentEntry Agent(const char *id, iggy::Vec2 position)
{
	iggy::npc_ai::NpcAgentEntry agent;
	agent.id = iggy::ResourceId { id };
	agent.state.position = position;
	return agent;
}

iggy::LevelRuntimeState Level(
	std::vector<std::string_view> rows,
	std::vector<iggy::npc_ai::NpcAgentEntry> agents = {})
{
	return { iggy::test::MapFromRows(rows), agents };
}

iggy::runtime::RuntimeGameplayProductLoopState ProductState(
	iggy::LevelRuntimeState level,
	bool loaded = true)
{
	iggy::runtime::RuntimeGameplayProductLoopState state;
	state.loaded = loaded;
	state.currentState.session.level = level;
	state.nextFrameIndex = 7;
	return state;
}

iggy::LevelRenderFrame2DConfig Config(bool includeNpcCommands = true)
{
	return {
		{ { 2.0F, 2.0F }, 1.0F },
		{ { WalkableMaterial, BlockedMaterial }, 1 },
		{ NpcMaterial, { 1.0F, 1.0F }, { 0.5F, 0.5F }, 8 },
		includeNpcCommands,
	};
}

iggy::LevelTileRenderChunkCache BuildCache(
	const iggy::LevelTileMap &map,
	int chunkWidth = 2,
	int chunkHeight = 2,
	int layer = 4)
{
	const iggy::LevelTileRenderChunkCacheBuildResult result =
		iggy::LevelTileRenderChunkCacheBuilder {}.build(
			map,
			{ chunkWidth, chunkHeight, { { WalkableMaterial, BlockedMaterial }, layer } });
	Expect(result.built, "test tile chunk cache should build");
	return result.cache;
}

bool SameCamera(iggy::CameraState actual, iggy::CameraState expected)
{
	return NearVec(actual.position, expected.position);
}

bool SameCommand(
	const iggy::render::RenderCommand2D &actual,
	const iggy::render::RenderCommand2D &expected)
{
	return actual.type == expected.type
		&& SameBounds(actual.worldBounds, expected.worldBounds)
		&& actual.materialId == expected.materialId
		&& actual.layer == expected.layer
		&& actual.order == expected.order
		&& actual.texture.textureId == expected.texture.textureId
		&& SameBounds(
			{ actual.texture.sourceRect.min(), actual.texture.sourceRect.max() },
			{ expected.texture.sourceRect.min(), expected.texture.sourceRect.max() })
		&& actual.texture.hasSourceRect == expected.texture.hasSourceRect;
}

bool SameCommands(
	const iggy::render::RenderCommandList2D &actual,
	const iggy::render::RenderCommandList2D &expected)
{
	if (actual.commands.size() != expected.commands.size())
		return false;
	for (std::size_t index = 0; index < actual.commands.size(); ++index) {
		if (!SameCommand(actual.commands[index], expected.commands[index]))
			return false;
	}
	return true;
}

bool SameLevelFrame(
	const iggy::LevelRenderFrame2DResult &actual,
	const iggy::LevelRenderFrame2DResult &expected)
{
	return SameBounds(actual.cameraView.bounds, expected.cameraView.bounds)
		&& NearVec(actual.cameraView.halfExtent, expected.cameraView.halfExtent)
		&& actual.usedTileChunkCache == expected.usedTileChunkCache
		&& actual.visibleTiles.tiles.size() == expected.visibleTiles.tiles.size()
		&& actual.tileDrawList.items.size() == expected.tileDrawList.items.size()
		&& actual.visibleTileChunks.chunkIndexes ==
			expected.visibleTileChunks.chunkIndexes
		&& actual.tileChunkCommands.usedChunkIndexes ==
			expected.tileChunkCommands.usedChunkIndexes
		&& SameCommands(actual.commands, expected.commands);
}

bool SameProductState(
	const iggy::runtime::RuntimeGameplayProductLoopState &actual,
	const iggy::runtime::RuntimeGameplayProductLoopState &expected)
{
	return actual.loaded == expected.loaded
		&& actual.nextFrameIndex == expected.nextFrameIndex
		&& actual.currentState.session.level.map.width ==
			expected.currentState.session.level.map.width
		&& actual.currentState.session.level.map.height ==
			expected.currentState.session.level.map.height
		&& actual.currentState.session.level.map.tiles.size() ==
			expected.currentState.session.level.map.tiles.size()
		&& actual.currentState.session.level.npcAgents.size() ==
			expected.currentState.session.level.npcAgents.size();
}

iggy::runtime::RuntimeGameplayProductPresentationFrameResult BuildPresentation(
	const iggy::runtime::RuntimeGameplayProductPresentationFrameInput &input)
{
	return iggy::runtime::RuntimeGameplayProductPresentationFrame {}.build(input);
}

void TestUnloadedStateDoesNotRenderNonEmptyLevel()
{
	iggy::runtime::RuntimeGameplayProductPresentationFrameInput input;
	input.state = ProductState(Level({ "..", ".." }), false);
	input.presentationCamera = { { 1.0F, 1.0F } };
	input.levelRenderConfig = Config(false);

	const iggy::runtime::RuntimeGameplayProductPresentationFrameResult result =
		BuildPresentation(input);

	Expect(result.status == PresentationStatus::NotLoaded,
		"unloaded product state should not render");
	Expect(SameCamera(result.presentationCamera, input.presentationCamera),
		"unloaded result should copy presentation camera");
	Expect(result.levelFrame.commands.commands.empty(),
		"unloaded state should leave render commands default-empty");
	Expect(!result.levelFrame.visibleTiles.hasTiles &&
			result.levelFrame.visibleTiles.tiles.empty(),
		"unloaded state should not compute visible tiles");
	Expect(SameBounds(result.levelFrame.cameraView.bounds, {}),
		"unloaded state should leave camera view default");
}

void TestLoadedStateMatchesDirectLevelRenderFrame()
{
	iggy::runtime::RuntimeGameplayProductPresentationFrameInput input;
	input.state = ProductState(Level({
		"..",
		".#",
	}, {
		Agent("npc:one", { 0.5F, 0.5F }),
	}));
	input.presentationCamera = { { 1.0F, 1.0F } };
	input.levelRenderConfig = Config();

	const iggy::LevelRenderFrame2DResult direct =
		iggy::LevelRenderFrame2D {}.build(
			input.state.currentState.session.level,
			input.presentationCamera,
			input.levelRenderConfig);
	const iggy::runtime::RuntimeGameplayProductPresentationFrameResult result =
		BuildPresentation(input);

	Expect(result.status == PresentationStatus::Rendered,
		"loaded product state should render");
	Expect(SameCamera(result.presentationCamera, input.presentationCamera),
		"loaded result should copy presentation camera");
	Expect(SameLevelFrame(result.levelFrame, direct),
		"loaded product presentation should match direct level render frame");
}

void TestCameraChangesVisibleBoundsWithoutMutation()
{
	iggy::runtime::RuntimeGameplayProductPresentationFrameInput left;
	left.state = ProductState(Level({ "..." }));
	left.presentationCamera = { { 0.5F, 0.5F } };
	left.levelRenderConfig = Config(false);
	left.levelRenderConfig.cameraView.viewportSize = { 1.0F, 1.0F };
	const iggy::runtime::RuntimeGameplayProductPresentationFrameInput before =
		left;
	iggy::runtime::RuntimeGameplayProductPresentationFrameInput right = left;
	right.presentationCamera = { { 2.5F, 0.5F } };

	const iggy::runtime::RuntimeGameplayProductPresentationFrameResult leftResult =
		BuildPresentation(left);
	const iggy::runtime::RuntimeGameplayProductPresentationFrameResult rightResult =
		BuildPresentation(right);

	Expect(SameBounds(
			   leftResult.levelFrame.cameraView.bounds,
			   { { 0.0F, 0.0F }, { 1.0F, 1.0F } }),
		"left camera should produce left visible bounds");
	Expect(SameBounds(
			   rightResult.levelFrame.cameraView.bounds,
			   { { 2.0F, 0.0F }, { 3.0F, 1.0F } }),
		"right camera should produce right visible bounds");
	Expect(leftResult.levelFrame.commands.commands.size() == 1 &&
			rightResult.levelFrame.commands.commands.size() == 1,
		"one-tile camera views should each emit one tile command");
	if (leftResult.levelFrame.commands.commands.size() == 1 &&
			rightResult.levelFrame.commands.commands.size() == 1) {
		Expect(SameBounds(
				   leftResult.levelFrame.commands.commands[0].worldBounds,
				   { { 0.0F, 0.0F }, { 1.0F, 1.0F } }),
			"left camera should render left tile");
		Expect(SameBounds(
				   rightResult.levelFrame.commands.commands[0].worldBounds,
				   { { 2.0F, 0.0F }, { 3.0F, 1.0F } }),
			"right camera should render right tile");
	}
	Expect(SameProductState(left.state, before.state),
		"presentation build should not mutate product loop state");
	Expect(SameCamera(left.presentationCamera, before.presentationCamera),
		"presentation build should not mutate input camera");
	Expect(left.levelRenderConfig.includeNpcCommands ==
			before.levelRenderConfig.includeNpcCommands,
		"presentation build should not mutate render config");
}

void TestRenderConfigForwardsNpcCommandPolicy()
{
	iggy::runtime::RuntimeGameplayProductPresentationFrameInput input;
	input.state = ProductState(Level({
		"..",
		"..",
	}, {
		Agent("npc:one", { 0.5F, 0.5F }),
	}));
	input.presentationCamera = { { 1.0F, 1.0F } };
	input.levelRenderConfig = Config(false);

	const iggy::runtime::RuntimeGameplayProductPresentationFrameResult withoutNpc =
		BuildPresentation(input);
	input.levelRenderConfig.includeNpcCommands = true;
	const iggy::runtime::RuntimeGameplayProductPresentationFrameResult withNpc =
		BuildPresentation(input);

	Expect(withoutNpc.status == PresentationStatus::Rendered &&
			withNpc.status == PresentationStatus::Rendered,
		"NPC policy test should render both frames");
	Expect(withoutNpc.levelFrame.commands.commands.size() == 4,
		"includeNpcCommands=false should emit only tile commands");
	Expect(withNpc.levelFrame.commands.commands.size() == 5,
		"includeNpcCommands=true should append legacy NPC command");
	if (withNpc.levelFrame.commands.commands.size() == 5) {
		Expect(withNpc.levelFrame.commands.commands[4].materialId == NpcMaterial,
			"NPC command should use forwarded NPC render material");
		Expect(withNpc.levelFrame.commands.commands[4].layer == 8,
			"NPC command should use forwarded NPC render layer");
	}
}

void TestCacheForwardingDoesNotMutateCache()
{
	iggy::runtime::RuntimeGameplayProductPresentationFrameInput input;
	input.state = ProductState(Level({
		".#",
		"..",
	}));
	input.presentationCamera = { { 1.0F, 1.0F } };
	iggy::LevelTileRenderChunkCache cache = BuildCache(
		input.state.currentState.session.level.map,
		2,
		2,
		6);
	const std::size_t originalChunkCount = cache.chunks.size();
	const std::size_t originalCommandCount =
		cache.chunks.empty() ? 0 : cache.chunks[0].commands.commands.size();
	const iggy::ResourceId originalMaterial =
		cache.chunks[0].commands.commands[1].materialId;
	const std::size_t originalOrder =
		cache.chunks[0].commands.commands[1].order;
	input.levelRenderConfig = Config(false);
	input.levelRenderConfig.useTileChunkCache = true;
	input.levelRenderConfig.tileChunkCache = &cache;

	const iggy::runtime::RuntimeGameplayProductPresentationFrameResult result =
		BuildPresentation(input);

	Expect(result.status == PresentationStatus::Rendered,
		"cache forwarding should render loaded state");
	Expect(result.levelFrame.usedTileChunkCache,
		"valid cache config should be forwarded to level render frame");
	Expect(result.levelFrame.visibleTileChunks.chunkIndexes ==
			std::vector<std::size_t> { 0 },
		"cache forwarding should surface visible chunk indexes");
	Expect(cache.chunks.size() == originalChunkCount,
		"presentation frame should not mutate cache chunk count");
	if (!cache.chunks.empty() &&
			cache.chunks[0].commands.commands.size() == originalCommandCount) {
		Expect(cache.chunks[0].commands.commands[1].materialId == originalMaterial,
			"presentation frame should not mutate cached command material");
		Expect(cache.chunks[0].commands.commands[1].order == originalOrder,
			"presentation frame should not mutate cached command order");
	}
}

void TestProductLoopStateRemainsUnchanged()
{
	iggy::runtime::RuntimeGameplayProductPresentationFrameInput input;
	input.state = ProductState(Level({
		".#",
		"..",
	}, {
		Agent("npc:one", { 0.5F, 0.5F }),
	}));
	input.presentationCamera = { { 1.0F, 1.0F } };
	input.levelRenderConfig = Config();
	const iggy::runtime::RuntimeGameplayProductLoopState before = input.state;

	const iggy::runtime::RuntimeGameplayProductPresentationFrameResult result =
		BuildPresentation(input);

	Expect(result.status == PresentationStatus::Rendered,
		"immutability setup should render loaded state");
	Expect(input.state.nextFrameIndex == before.nextFrameIndex,
		"presentation frame should not mutate product loop frame index");
	Expect(SameProductState(input.state, before),
		"presentation frame should not mutate product loop current state");
	Expect(NearVec(
			   input.state.currentState.session.level.npcAgents[0].state.position,
			   before.currentState.session.level.npcAgents[0].state.position),
		"presentation frame should not mutate legacy NPC agent position");
}

} // namespace

int main()
{
	TestUnloadedStateDoesNotRenderNonEmptyLevel();
	TestLoadedStateMatchesDirectLevelRenderFrame();
	TestCameraChangesVisibleBoundsWithoutMutation();
	TestRenderConfigForwardsNpcCommandPolicy();
	TestCacheForwardingDoesNotMutateCache();
	TestProductLoopStateRemainsUnchanged();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
