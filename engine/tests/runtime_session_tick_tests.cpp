#include <cstdlib>
#include <string_view>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "runtime/RuntimeSessionTick.hpp"
#include "scene/level/LevelRenderCacheState.hpp"
#include "scene/level/LevelRuntimeBuilder.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

const iggy::ResourceId WalkableMaterial { "material:floor" };
const iggy::ResourceId BlockedMaterial { "material:wall" };

iggy::LevelBlueprint BlueprintWithNpc()
{
	iggy::LevelBlueprint blueprint;
	blueprint.id = iggy::ResourceId { "level:cellar_01" };
	blueprint.bounds = { 5, 3 };
	blueprint.tiles = {
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
		{ true },
	};
	blueprint.playerStarts.push_back({ 0, 0 });
	blueprint.entitySpawns.push_back({ iggy::ResourceId { "enemy:skeleton" }, 0, 1, iggy::ResourceId { "spawn:skeleton_01" } });
	return blueprint;
}

iggy::LevelRuntimeState StateFromBuild(const iggy::LevelRuntimeBuildResult &build)
{
	return { build.tileMap, build.npcAgents };
}

iggy::LevelRuntimeState State(std::vector<std::string_view> rows)
{
	return { iggy::test::MapFromRows(rows), {} };
}

iggy::npc_ai::NpcAgentTickConfig NpcConfig(float maxDistance = 0.25F)
{
	iggy::npc_ai::NpcAgentTickConfig config;
	config.maxDistance = maxDistance;
	config.awareness = { 8.0F, 0 };
	return config;
}

iggy::LevelTileRenderChunkCacheConfig ChunkConfig(int chunkWidth = 2, int chunkHeight = 2)
{
	return { chunkWidth, chunkHeight, { { WalkableMaterial, BlockedMaterial }, 3 } };
}

iggy::runtime::RuntimeSessionTickInput Input(iggy::runtime::RuntimeSessionState session)
{
	return { session, { 4.5F, 1.5F }, NpcConfig() };
}

bool HasEvent(const iggy::npc_ai::NpcTickReport &report, iggy::npc_ai::NpcTickEventType type)
{
	for (const iggy::npc_ai::NpcTickEvent &event : report.events) {
		if (event.type == type)
			return true;
	}
	return false;
}

iggy::LevelRenderCacheState BuildRenderCache(const iggy::LevelRuntimeState &level)
{
	const iggy::LevelRenderCacheBuildResult build = iggy::LevelRenderCacheBuilder {}.build(level.map, ChunkConfig());
	Expect(build.built, "render cache fixture should build");
	return build.state;
}

void TestEmptySessionTicksIndexAndNoReports()
{
	iggy::runtime::RuntimeSessionState session;
	session.level = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));
	session.level.npcAgents.clear();
	session.tickIndex = 2;

	const iggy::runtime::RuntimeSessionTickResult result = iggy::runtime::RuntimeSessionTick {}.run(Input(session));

	Expect(result.session.tickIndex == 3, "empty session tick should increment tick index");
	Expect(result.session.level.npcAgents.empty(), "empty session tick should keep empty NPC state");
	Expect(result.npcReports.empty(), "empty session tick should preserve empty NPC reports");
	Expect(result.session.level.map.id == session.level.map.id, "empty session tick should preserve map");
}

void TestVisibleNpcMovesAndReportsThroughRuntimeTick()
{
	iggy::runtime::RuntimeSessionState session;
	session.level = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));

	const iggy::runtime::RuntimeSessionTickResult result = iggy::runtime::RuntimeSessionTick {}.run(Input(session));

	Expect(result.session.tickIndex == 1, "session tick should increment tick index after NPC update");
	Expect(result.session.level.npcAgents.size() == 1, "session tick should preserve NPC count");
	if (result.session.level.npcAgents.size() == 1)
		Expect(NearVec(result.session.level.npcAgents[0].state.position, { 0.75F, 1.5F }), "session tick should move visible NPC through RuntimeTick");
	Expect(result.npcReports.size() == 1, "session tick should forward NPC reports");
	if (result.npcReports.size() == 1) {
		Expect(result.npcReports[0].id == iggy::ResourceId { "spawn:skeleton_01" }, "session tick should forward report NPC id");
		Expect(HasEvent(result.npcReports[0].report, iggy::npc_ai::NpcTickEventType::PositionChanged), "session tick should forward position change event");
	}
}

void TestInputSessionIsNotMutated()
{
	iggy::runtime::RuntimeSessionState session;
	session.level = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));
	session.tickIndex = 7;
	const iggy::Vec2 originalPosition = session.level.npcAgents[0].state.position;

	const iggy::runtime::RuntimeSessionTickResult result = iggy::runtime::RuntimeSessionTick {}.run(Input(session));

	Expect(session.tickIndex == 7, "session tick should not mutate input tick index");
	Expect(NearVec(session.level.npcAgents[0].state.position, originalPosition), "session tick should not mutate input NPC position");
	Expect(result.session.tickIndex == 8, "session tick should return incremented tick index separately");
	Expect(!NearVec(result.session.level.npcAgents[0].state.position, originalPosition), "session tick should return updated NPC state separately");
}

void TestRenderCacheStateIsPreservedWhenPresent()
{
	iggy::runtime::RuntimeSessionState session;
	session.level = State({
		"..",
		"..",
	});
	session.renderCache = BuildRenderCache(session.level);
	session.hasRenderCache = true;
	session.tickIndex = 4;

	const std::size_t originalChunkCount = session.renderCache.tileChunks.chunks.size();
	const iggy::LevelTileRenderChunkCacheConfig originalConfig = session.renderCache.tileChunkConfig;
	const iggy::render::RenderCommand2D originalCommand = session.renderCache.tileChunks.chunks[0].commands.commands[0];

	const iggy::runtime::RuntimeSessionTickResult result = iggy::runtime::RuntimeSessionTick {}.run(Input(session));

	Expect(result.session.hasRenderCache, "session tick should preserve hasRenderCache true");
	Expect(result.session.tickIndex == 5, "session tick should increment tick index with render cache present");
	Expect(result.session.renderCache.tileChunkConfig.chunkWidth == originalConfig.chunkWidth && result.session.renderCache.tileChunkConfig.chunkHeight == originalConfig.chunkHeight, "session tick should preserve render cache chunk config");
	Expect(result.session.renderCache.tileChunkConfig.tileCommands.layer == originalConfig.tileCommands.layer, "session tick should preserve render cache layer config");
	Expect(result.session.renderCache.tileChunks.chunks.size() == originalChunkCount, "session tick should preserve render cache chunk count");
	if (!result.session.renderCache.tileChunks.chunks.empty() && !result.session.renderCache.tileChunks.chunks[0].commands.commands.empty()) {
		const iggy::render::RenderCommand2D command = result.session.renderCache.tileChunks.chunks[0].commands.commands[0];
		Expect(command.materialId == originalCommand.materialId, "session tick should preserve render cache command material");
		Expect(command.order == originalCommand.order, "session tick should preserve render cache command order");
		Expect(command.texture.hasSourceRect == originalCommand.texture.hasSourceRect && command.texture.textureId == originalCommand.texture.textureId, "session tick should preserve render cache command texture payload");
	}
}

void TestNoRenderCacheRemainsAbsent()
{
	iggy::runtime::RuntimeSessionState session;
	session.level = State({
		".",
	});
	session.hasRenderCache = false;
	session.tickIndex = 3;

	const iggy::runtime::RuntimeSessionTickResult result = iggy::runtime::RuntimeSessionTick {}.run(Input(session));

	Expect(!result.session.hasRenderCache, "session tick should preserve hasRenderCache false");
	Expect(result.session.tickIndex == 4, "session tick should increment tick index without render cache");
	Expect(result.session.renderCache.tileChunks.chunks.empty(), "session tick should preserve default render cache state");
}

void TestMapStateIsPreserved()
{
	iggy::runtime::RuntimeSessionState session;
	session.level = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));

	const iggy::runtime::RuntimeSessionTickResult result = iggy::runtime::RuntimeSessionTick {}.run(Input(session));

	Expect(result.session.level.map.id == iggy::ResourceId { "level:cellar_01" }, "session tick should preserve map id");
	Expect(result.session.level.map.width == session.level.map.width && result.session.level.map.height == session.level.map.height, "session tick should preserve map dimensions");
	Expect(result.session.level.map.tiles.size() == session.level.map.tiles.size(), "session tick should preserve map tiles");
}

} // namespace

int main()
{
	TestEmptySessionTicksIndexAndNoReports();
	TestVisibleNpcMovesAndReportsThroughRuntimeTick();
	TestInputSessionIsNotMutated();
	TestRenderCacheStateIsPreservedWhenPresent();
	TestNoRenderCacheRemainsAbsent();
	TestMapStateIsPreserved();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
