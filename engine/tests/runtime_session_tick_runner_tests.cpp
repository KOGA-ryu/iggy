#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "runtime/RuntimeSessionTick.hpp"
#include "runtime/RuntimeSessionTickRunner.hpp"
#include "scene/level/LevelDerivedCacheState.hpp"
#include "scene/level/LevelRenderCacheState.hpp"
#include "scene/level/LevelRuntimeBuilder.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectPlayerAgent;
using iggy::test::Failures;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

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

iggy::runtime::RuntimeSessionTickRunInput Input(iggy::runtime::RuntimeSessionState session, std::size_t tickCount)
{
	return { session, { 4.5F, 1.5F }, NpcConfig(), tickCount };
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

iggy::LevelDerivedCacheState BuildDerivedCaches(const iggy::LevelRuntimeState &level)
{
	iggy::LevelDerivedCacheBuildConfig config;
	config.buildRenderCache = true;
	config.renderCacheConfig = ChunkConfig();
	config.buildCollisionCache = true;
	const iggy::LevelDerivedCacheBuildResult build = iggy::LevelDerivedCacheBuilder {}.build(level, config);
	Expect(build.built, "derived cache fixture should build");
	return build.state;
}

void TestZeroTicksReturnsInitialSession()
{
	iggy::runtime::RuntimeSessionState session;
	session.level = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));
	session.player = PlayerAgent(iggy::ResourceId { "player:one" }, { 2.25F, 3.75F }, { 2, 3 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::North);
	session.hasPlayer = true;
	session.tickIndex = 6;
	const iggy::PlayerAgentState originalPlayer = session.player;

	const iggy::runtime::RuntimeSessionTickRunResult result = iggy::runtime::RuntimeSessionTickRunner {}.run(Input(session, 0));

	Expect(result.finalSession.tickIndex == 6, "zero session ticks should preserve tick index");
	Expect(result.finalSession.level.npcAgents.size() == session.level.npcAgents.size(), "zero session ticks should preserve NPC count");
	Expect(result.finalSession.level.npcAgents[0].state.position == session.level.npcAgents[0].state.position, "zero session ticks should preserve NPC position");
	Expect(result.finalSession.hasPlayer, "zero session ticks should preserve hasPlayer true");
	ExpectPlayerAgent(result.finalSession.player, originalPlayer, "zero tick result");
	Expect(result.reportsByTick.empty(), "zero session ticks should produce no report groups");
}

void TestOneTickMatchesRuntimeSessionTick()
{
	iggy::runtime::RuntimeSessionState session;
	session.level = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));
	session.tickIndex = 3;

	const iggy::runtime::RuntimeSessionTickResult singleTick = iggy::runtime::RuntimeSessionTick {}.run({ session, { 4.5F, 1.5F }, NpcConfig() });
	const iggy::runtime::RuntimeSessionTickRunResult result = iggy::runtime::RuntimeSessionTickRunner {}.run(Input(session, 1));

	Expect(result.finalSession.tickIndex == singleTick.session.tickIndex, "one session tick runner should match RuntimeSessionTick tick index");
	Expect(NearVec(result.finalSession.level.npcAgents[0].state.position, singleTick.session.level.npcAgents[0].state.position), "one session tick runner should match RuntimeSessionTick movement");
	Expect(result.reportsByTick.size() == 1 && result.reportsByTick[0].size() == singleTick.npcReports.size(), "one session tick runner should match RuntimeSessionTick report count");
}

void TestMultipleTicksContinueFromPriorSession()
{
	iggy::runtime::RuntimeSessionState session;
	session.level = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));
	session.tickIndex = 5;

	const iggy::runtime::RuntimeSessionTickRunResult result = iggy::runtime::RuntimeSessionTickRunner {}.run(Input(session, 3));

	Expect(result.finalSession.tickIndex == 8, "multiple session ticks should increment tick index by tick count");
	Expect(NearVec(result.finalSession.level.npcAgents[0].state.position, { 1.25F, 1.5F }), "multiple session ticks should continue NPC movement from prior output");
	Expect(result.reportsByTick.size() == 3, "multiple session ticks should produce report group per tick");
}

void TestReportsGroupedByTickOrder()
{
	iggy::runtime::RuntimeSessionState session;
	session.level = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));

	const iggy::runtime::RuntimeSessionTickRunResult result = iggy::runtime::RuntimeSessionTickRunner {}.run(Input(session, 2));

	Expect(result.reportsByTick.size() == 2, "session reports should be grouped by tick");
	Expect(result.reportsByTick[0].size() == 1 && result.reportsByTick[1].size() == 1, "each session tick should preserve NPC reports");
	if (result.reportsByTick.size() == 2 && result.reportsByTick[0].size() == 1 && result.reportsByTick[1].size() == 1) {
		Expect(result.reportsByTick[0][0].id == iggy::ResourceId { "spawn:skeleton_01" }, "first session tick report should preserve NPC id");
		Expect(result.reportsByTick[1][0].id == iggy::ResourceId { "spawn:skeleton_01" }, "second session tick report should preserve NPC id");
		Expect(HasEvent(result.reportsByTick[0][0].report, iggy::npc_ai::NpcTickEventType::PositionChanged), "first session tick should report position change");
		Expect(HasEvent(result.reportsByTick[1][0].report, iggy::npc_ai::NpcTickEventType::PositionChanged), "second session tick should report position change");
	}
}

void TestRenderCacheStateIsPreservedAcrossTicks()
{
	iggy::runtime::RuntimeSessionState session;
	session.level = State({
		".#",
		"..",
	});
	session.renderCache = BuildRenderCache(session.level);
	session.derivedCaches = BuildDerivedCaches(session.level);
	session.hasRenderCache = true;
	session.tickIndex = 10;

	const std::size_t originalChunkCount = session.renderCache.tileChunks.chunks.size();
	const std::size_t originalDerivedRenderChunkCount = session.derivedCaches.render.tileChunks.chunks.size();
	const std::size_t originalCollisionObjectCount = session.derivedCaches.collision.world.objects().size();
	const iggy::LevelTileRenderChunkCacheConfig originalConfig = session.renderCache.tileChunkConfig;
	const iggy::render::RenderCommand2D originalCommand = session.renderCache.tileChunks.chunks[0].commands.commands[0];

	const iggy::runtime::RuntimeSessionTickRunResult result = iggy::runtime::RuntimeSessionTickRunner {}.run(Input(session, 4));

	Expect(result.finalSession.hasRenderCache, "session tick runner should preserve hasRenderCache true");
	Expect(result.finalSession.tickIndex == 14, "session tick runner should increment tick index with render cache present");
	Expect(result.finalSession.renderCache.tileChunkConfig.chunkWidth == originalConfig.chunkWidth && result.finalSession.renderCache.tileChunkConfig.chunkHeight == originalConfig.chunkHeight, "session tick runner should preserve render cache chunk config");
	Expect(result.finalSession.renderCache.tileChunkConfig.tileCommands.layer == originalConfig.tileCommands.layer, "session tick runner should preserve render cache layer config");
	Expect(result.finalSession.renderCache.tileChunks.chunks.size() == originalChunkCount, "session tick runner should preserve render cache chunk count");
	Expect(result.finalSession.derivedCaches.hasRenderCache, "session tick runner should preserve derived render cache flag");
	Expect(result.finalSession.derivedCaches.hasCollisionCache, "session tick runner should preserve derived collision cache flag");
	Expect(result.finalSession.derivedCaches.render.tileChunks.chunks.size() == originalDerivedRenderChunkCount, "session tick runner should preserve derived render chunks");
	Expect(result.finalSession.derivedCaches.collision.world.objects().size() == originalCollisionObjectCount, "session tick runner should preserve derived collision objects");
	if (!result.finalSession.renderCache.tileChunks.chunks.empty() && !result.finalSession.renderCache.tileChunks.chunks[0].commands.commands.empty()) {
		const iggy::render::RenderCommand2D command = result.finalSession.renderCache.tileChunks.chunks[0].commands.commands[0];
		Expect(command.materialId == originalCommand.materialId, "session tick runner should preserve render cache command material");
		Expect(command.order == originalCommand.order, "session tick runner should preserve render cache command order");
		Expect(command.texture.hasSourceRect == originalCommand.texture.hasSourceRect && command.texture.textureId == originalCommand.texture.textureId, "session tick runner should preserve render cache command texture payload");
	}
}

void TestPlayerStateIsPreservedAcrossTicks()
{
	iggy::runtime::RuntimeSessionState session;
	session.level = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));
	session.player = PlayerAgent(iggy::ResourceId { "player:one" }, { 2.25F, 3.75F }, { 2, 3 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::North);
	session.hasPlayer = true;
	session.tickIndex = 4;
	const iggy::PlayerAgentState originalPlayer = session.player;

	const iggy::runtime::RuntimeSessionTickRunResult result = iggy::runtime::RuntimeSessionTickRunner {}.run(Input(session, 3));

	Expect(result.finalSession.tickIndex == 7, "session tick runner with player should increment tick index by tick count");
	Expect(result.finalSession.hasPlayer, "session tick runner should preserve hasPlayer true");
	ExpectPlayerAgent(result.finalSession.player, originalPlayer, "session tick runner result");
	Expect(result.finalSession.level.npcAgents.size() == 1 && !NearVec(result.finalSession.level.npcAgents[0].state.position, session.level.npcAgents[0].state.position), "session tick runner with player should still update NPC level state");
}

void TestNoPlayerStateRemainsAbsentAcrossTicks()
{
	iggy::runtime::RuntimeSessionState session;
	session.level = State({
		".",
	});
	session.hasPlayer = false;

	const iggy::runtime::RuntimeSessionTickRunResult result = iggy::runtime::RuntimeSessionTickRunner {}.run(Input(session, 2));

	Expect(!result.finalSession.hasPlayer, "session tick runner should preserve hasPlayer false");
	Expect(result.finalSession.player.id.empty(), "session tick runner should preserve default player when absent");
	Expect(NearVec(result.finalSession.player.position, { 0.0F, 0.0F }), "session tick runner should preserve default player position when absent");
}

void TestNoRenderCacheRemainsAbsentAcrossTicks()
{
	iggy::runtime::RuntimeSessionState session;
	session.level = State({
		".",
	});
	session.hasRenderCache = false;
	session.tickIndex = 1;

	const iggy::runtime::RuntimeSessionTickRunResult result = iggy::runtime::RuntimeSessionTickRunner {}.run(Input(session, 2));

	Expect(!result.finalSession.hasRenderCache, "session tick runner should preserve hasRenderCache false");
	Expect(result.finalSession.tickIndex == 3, "session tick runner should increment tick index without render cache");
	Expect(result.finalSession.renderCache.tileChunks.chunks.empty(), "session tick runner should preserve default render cache state");
}

void TestInitialSessionIsNotMutated()
{
	iggy::runtime::RuntimeSessionState session;
	session.level = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));
	session.tickIndex = 7;
	const iggy::Vec2 originalPosition = session.level.npcAgents[0].state.position;

	const iggy::runtime::RuntimeSessionTickRunResult result = iggy::runtime::RuntimeSessionTickRunner {}.run(Input(session, 2));

	Expect(session.tickIndex == 7, "session tick runner should not mutate initial tick index");
	Expect(NearVec(session.level.npcAgents[0].state.position, originalPosition), "session tick runner should not mutate initial NPC position");
	Expect(result.finalSession.tickIndex == 9, "session tick runner should return incremented final tick index separately");
	Expect(!NearVec(result.finalSession.level.npcAgents[0].state.position, originalPosition), "session tick runner should return updated NPC state separately");
}

void TestMapStateIsPreserved()
{
	iggy::runtime::RuntimeSessionState session;
	session.level = StateFromBuild(iggy::LevelRuntimeBuilder {}.build(BlueprintWithNpc()));

	const iggy::runtime::RuntimeSessionTickRunResult result = iggy::runtime::RuntimeSessionTickRunner {}.run(Input(session, 3));

	Expect(result.finalSession.level.map.id == iggy::ResourceId { "level:cellar_01" }, "session tick runner should preserve map id");
	Expect(result.finalSession.level.map.width == session.level.map.width && result.finalSession.level.map.height == session.level.map.height, "session tick runner should preserve map dimensions");
	Expect(result.finalSession.level.map.tiles.size() == session.level.map.tiles.size(), "session tick runner should preserve map tiles");
}

} // namespace

int main()
{
	TestZeroTicksReturnsInitialSession();
	TestOneTickMatchesRuntimeSessionTick();
	TestMultipleTicksContinueFromPriorSession();
	TestReportsGroupedByTickOrder();
	TestRenderCacheStateIsPreservedAcrossTicks();
	TestPlayerStateIsPreservedAcrossTicks();
	TestNoPlayerStateRemainsAbsentAcrossTicks();
	TestNoRenderCacheRemainsAbsentAcrossTicks();
	TestInitialSessionIsNotMutated();
	TestMapStateIsPreserved();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
