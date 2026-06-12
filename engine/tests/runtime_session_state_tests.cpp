#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "modules/npc_ai/NpcAgentBatch.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::NearVec;

const iggy::ResourceId WalkableMaterial { "material:floor" };
const iggy::ResourceId BlockedMaterial { "material:wall" };

iggy::LevelRuntimeState State(std::vector<std::string_view> rows, std::vector<iggy::npc_ai::NpcAgentEntry> agents = {})
{
	return { iggy::test::MapFromRows(rows), agents };
}

iggy::npc_ai::NpcAgentEntry Agent(const char *id, iggy::Vec2 position)
{
	iggy::npc_ai::NpcAgentEntry agent;
	agent.id = iggy::ResourceId { id };
	agent.state.position = position;
	return agent;
}

iggy::LevelTileRenderChunkCacheConfig ChunkConfig(int chunkWidth = 2, int chunkHeight = 2)
{
	return { chunkWidth, chunkHeight, { { WalkableMaterial, BlockedMaterial }, 3 } };
}

iggy::runtime::RuntimeSessionBuildConfig Config(bool buildRenderCache = true, int chunkWidth = 2, int chunkHeight = 2)
{
	return { ChunkConfig(chunkWidth, chunkHeight), buildRenderCache };
}

iggy::PlayerAgentState Player(const char *id = "player:one")
{
	iggy::PlayerAgentState player;
	player.id = iggy::ResourceId { id };
	player.position = { 2.25F, 3.75F };
	player.spawnTile = { 2, 3 };
	player.movementStatus = iggy::PlayerMovementStatus::Moving;
	player.facing = iggy::PlayerFacing2D::East;
	return player;
}

iggy::runtime::RuntimeSessionBuildConfig ConfigWithPlayer(bool buildRenderCache = true, iggy::PlayerAgentState player = Player())
{
	iggy::runtime::RuntimeSessionBuildConfig config = Config(buildRenderCache);
	config.hasPlayer = true;
	config.player = player;
	return config;
}

void ExpectPlayer(const iggy::PlayerAgentState &actual, const iggy::PlayerAgentState &expected, std::string_view context)
{
	Expect(actual.id == expected.id, std::string(context) + " should preserve player id");
	Expect(NearVec(actual.position, expected.position), std::string(context) + " should preserve player position");
	Expect(actual.spawnTile == expected.spawnTile, std::string(context) + " should preserve player spawn tile");
	Expect(actual.movementStatus == expected.movementStatus, std::string(context) + " should preserve player movement status");
	Expect(actual.facing == expected.facing, std::string(context) + " should preserve player facing");
}

void TestBuildWithRenderCacheEnabledSucceeds()
{
	const iggy::LevelRuntimeState level = State({
		"....",
		"....",
	}, {
		Agent("npc:one", { 1.0F, 1.0F }),
	});

	const iggy::runtime::RuntimeSessionBuildResult result = iggy::runtime::RuntimeSessionBuilder {}.build(level, Config());

	Expect(result.built, "session build with render cache should succeed");
	Expect(result.state.tickIndex == 0, "new session should start at tick zero");
	Expect(result.state.hasRenderCache, "session build with render cache should set hasRenderCache");
	Expect(result.renderCache.built, "session build should expose successful render cache result");
	Expect(result.state.level.map.width == 4 && result.state.level.map.height == 2, "session should preserve level map shape");
	Expect(result.state.level.npcAgents.size() == 1 && result.state.level.npcAgents[0].id == iggy::ResourceId { "npc:one" }, "session should preserve NPC agents");
	Expect(result.state.renderCache.tileChunks.chunks.size() == 2, "session should build tile render chunks");
	Expect(!result.state.hasPlayer, "session build without player should leave hasPlayer false");
}

void TestBuildWithRenderCacheDisabledSucceeds()
{
	const iggy::LevelRuntimeState level = State({
		"..",
		"..",
	});

	const iggy::runtime::RuntimeSessionBuildResult result = iggy::runtime::RuntimeSessionBuilder {}.build(level, Config(false));

	Expect(result.built, "session build without render cache should succeed");
	Expect(result.state.tickIndex == 0, "no-cache session should start at tick zero");
	Expect(!result.state.hasRenderCache, "no-cache session should not report render cache");
	Expect(result.state.level.map.width == 2 && result.state.level.map.height == 2, "no-cache session should preserve level state");
	Expect(result.state.renderCache.tileChunks.chunks.empty(), "no-cache session should leave render cache default");
	Expect(!result.renderCache.built && result.renderCache.tileChunkIssues.empty(), "no-cache session should leave render cache diagnostics default");
	Expect(!result.state.hasPlayer, "no-cache session without player should leave hasPlayer false");
}

void TestInvalidRenderCacheConfigFails()
{
	const iggy::LevelRuntimeState level = State({
		"..",
	});

	const iggy::runtime::RuntimeSessionBuildResult result = iggy::runtime::RuntimeSessionBuilder {}.build(level, Config(true, 0, 2));

	Expect(!result.built, "invalid render cache config should fail session build");
	Expect(!result.state.hasRenderCache && result.state.tickIndex == 0 && !result.state.hasPlayer, "failed session should leave state default");
	Expect(!result.renderCache.built, "failed session should expose failed render cache build");
	Expect(result.renderCache.tileChunkIssues.size() == 1, "failed session should expose render cache issue");
	if (result.renderCache.tileChunkIssues.size() == 1)
		Expect(result.renderCache.tileChunkIssues[0].code == iggy::LevelTileRenderChunkCacheIssueCode::InvalidChunkSize && result.renderCache.tileChunkIssues[0].chunkWidth == 0 && result.renderCache.tileChunkIssues[0].chunkHeight == 2, "failed session should preserve render cache issue dimensions");
}

void TestBuildWithPlayerAndRenderCacheEnabledCopiesPlayer()
{
	const iggy::LevelRuntimeState level = State({
		"..",
		"..",
	});
	const iggy::PlayerAgentState player = Player();

	const iggy::runtime::RuntimeSessionBuildResult result = iggy::runtime::RuntimeSessionBuilder {}.build(level, ConfigWithPlayer(true, player));

	Expect(result.built, "session build with player and render cache should succeed");
	Expect(result.state.hasRenderCache, "session with player and render cache should build render cache");
	Expect(result.state.hasPlayer, "session with player should set hasPlayer");
	ExpectPlayer(result.state.player, player, "session with render cache");
}

void TestBuildWithPlayerAndRenderCacheDisabledCopiesPlayer()
{
	const iggy::LevelRuntimeState level = State({
		"..",
	});
	const iggy::PlayerAgentState player = Player();

	const iggy::runtime::RuntimeSessionBuildResult result = iggy::runtime::RuntimeSessionBuilder {}.build(level, ConfigWithPlayer(false, player));

	Expect(result.built, "session build with player and no render cache should succeed");
	Expect(!result.state.hasRenderCache, "session with player and no render cache should leave render cache disabled");
	Expect(result.state.hasPlayer, "session with player and no render cache should set hasPlayer");
	ExpectPlayer(result.state.player, player, "session without render cache");
}

void TestInvalidRenderCacheConfigDoesNotPublishPlayer()
{
	const iggy::LevelRuntimeState level = State({
		"..",
	});
	iggy::runtime::RuntimeSessionBuildConfig config = ConfigWithPlayer(true, Player());
	config.renderCacheConfig.chunkWidth = 0;

	const iggy::runtime::RuntimeSessionBuildResult result = iggy::runtime::RuntimeSessionBuilder {}.build(level, config);

	Expect(!result.built, "invalid render cache config with player should fail");
	Expect(!result.state.hasPlayer, "failed session should not publish player");
	Expect(result.state.player.id.empty(), "failed session should leave player default");
}

void TestDefaultPlayerStateAllowedWhenRequested()
{
	const iggy::LevelRuntimeState level = State({
		".",
	});
	const iggy::PlayerAgentState player;

	const iggy::runtime::RuntimeSessionBuildResult result = iggy::runtime::RuntimeSessionBuilder {}.build(level, ConfigWithPlayer(false, player));

	Expect(result.built, "session build with default player should succeed");
	Expect(result.state.hasPlayer, "session build with default player should set hasPlayer");
	ExpectPlayer(result.state.player, player, "session with default player");
}

void TestEmptyMapBuildsEmptyRenderCache()
{
	const iggy::LevelRuntimeState level;

	const iggy::runtime::RuntimeSessionBuildResult result = iggy::runtime::RuntimeSessionBuilder {}.build(level, Config());

	Expect(result.built, "empty map session with valid cache config should build");
	Expect(result.state.hasRenderCache, "empty map session should still carry render cache state");
	Expect(result.renderCache.built, "empty map session should expose successful render cache build");
	Expect(result.state.renderCache.tileChunks.chunks.empty(), "empty map session should build empty tile render cache");
}

void TestNpcAgentsArePreserved()
{
	const iggy::LevelRuntimeState level = State({
		".",
	}, {
		Agent("npc:first", { 1.0F, 2.0F }),
		Agent("npc:second", { 3.0F, 4.0F }),
	});

	const iggy::runtime::RuntimeSessionBuildResult result = iggy::runtime::RuntimeSessionBuilder {}.build(level, Config(false));

	Expect(result.built, "NPC preservation session should build");
	Expect(result.state.level.npcAgents.size() == 2, "session should preserve NPC count");
	if (result.state.level.npcAgents.size() == 2) {
		Expect(result.state.level.npcAgents[0].id == iggy::ResourceId { "npc:first" } && NearVec(result.state.level.npcAgents[0].state.position, { 1.0F, 2.0F }), "session should preserve first NPC");
		Expect(result.state.level.npcAgents[1].id == iggy::ResourceId { "npc:second" } && NearVec(result.state.level.npcAgents[1].state.position, { 3.0F, 4.0F }), "session should preserve second NPC");
	}
}

void TestInputCopyIsNotMutated()
{
	const iggy::LevelRuntimeState original = State({
		".#",
	}, {
		Agent("npc:one", { 0.5F, 0.5F }),
	});
	const iggy::LevelRuntimeState copy = original;

	const iggy::runtime::RuntimeSessionBuildResult result = iggy::runtime::RuntimeSessionBuilder {}.build(copy, Config());

	Expect(result.built, "input copy immutability setup should build");
	Expect(original.map.width == 2 && original.map.height == 1 && original.map.tiles.size() == 2, "original level map should remain unchanged");
	Expect(original.map.tiles[1].walkable == false, "original level tile walkability should remain unchanged");
	Expect(original.npcAgents.size() == 1 && original.npcAgents[0].id == iggy::ResourceId { "npc:one" }, "original NPC list should remain unchanged");
}

void TestInputPlayerIsNotMutated()
{
	const iggy::LevelRuntimeState level = State({
		".",
	});
	const iggy::PlayerAgentState player = Player();
	const iggy::PlayerAgentState original = player;

	const iggy::runtime::RuntimeSessionBuildResult result = iggy::runtime::RuntimeSessionBuilder {}.build(level, ConfigWithPlayer(false, player));

	Expect(result.built, "input player immutability setup should build");
	ExpectPlayer(player, original, "input player");
}

} // namespace

int main()
{
	TestBuildWithRenderCacheEnabledSucceeds();
	TestBuildWithRenderCacheDisabledSucceeds();
	TestInvalidRenderCacheConfigFails();
	TestBuildWithPlayerAndRenderCacheEnabledCopiesPlayer();
	TestBuildWithPlayerAndRenderCacheDisabledCopiesPlayer();
	TestInvalidRenderCacheConfigDoesNotPublishPlayer();
	TestDefaultPlayerStateAllowedWhenRequested();
	TestEmptyMapBuildsEmptyRenderCache();
	TestNpcAgentsArePreserved();
	TestInputCopyIsNotMutated();
	TestInputPlayerIsNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
