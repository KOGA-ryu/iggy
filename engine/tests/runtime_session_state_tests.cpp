#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "modules/npc_ai/NpcAgentBatch.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/level/LevelGridQuery.hpp"
#include "support/GeometryAssertions.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::ExpectBounds;
using iggy::test::ExpectPlayerAgent;
using iggy::test::Failures;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

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

iggy::runtime::RuntimeSessionBuildConfig ConfigWithPlayer(bool buildRenderCache = true, iggy::PlayerAgentState player = PlayerAgent(iggy::ResourceId { "player:one" }, { 2.25F, 3.75F }, { 2, 3 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::East))
{
	iggy::runtime::RuntimeSessionBuildConfig config = Config(buildRenderCache);
	config.hasPlayer = true;
	config.player = player;
	return config;
}

iggy::LevelDerivedCacheBuildConfig DerivedConfig(
	bool buildRender,
	bool buildCollision,
	iggy::LevelTileRenderChunkCacheConfig renderConfig = ChunkConfig())
{
	iggy::LevelDerivedCacheBuildConfig config;
	config.buildRenderCache = buildRender;
	config.renderCacheConfig = renderConfig;
	config.buildCollisionCache = buildCollision;
	return config;
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
	Expect(result.state.derivedCaches.hasRenderCache, "legacy render cache build should mirror render cache into derived caches");
	Expect(!result.state.derivedCaches.hasCollisionCache, "legacy render cache build should leave derived collision cache absent");
	Expect(result.state.derivedCaches.render.tileChunks.chunks.size() == result.state.renderCache.tileChunks.chunks.size(), "legacy render cache mirror should preserve render chunks");
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
	Expect(!result.state.derivedCaches.hasRenderCache, "no-cache session should leave derived render cache absent");
	Expect(!result.state.derivedCaches.hasCollisionCache, "no-cache session should leave derived collision cache absent");
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
	Expect(!result.state.derivedCaches.hasRenderCache && !result.state.derivedCaches.hasCollisionCache, "failed session should leave derived cache state default");
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
	const iggy::PlayerAgentState player = PlayerAgent(iggy::ResourceId { "player:one" }, { 2.25F, 3.75F }, { 2, 3 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::East);

	const iggy::runtime::RuntimeSessionBuildResult result = iggy::runtime::RuntimeSessionBuilder {}.build(level, ConfigWithPlayer(true, player));

	Expect(result.built, "session build with player and render cache should succeed");
	Expect(result.state.hasRenderCache, "session with player and render cache should build render cache");
	Expect(result.state.hasPlayer, "session with player should set hasPlayer");
	ExpectPlayerAgent(result.state.player, player, "session with render cache");
}

void TestBuildWithPlayerAndRenderCacheDisabledCopiesPlayer()
{
	const iggy::LevelRuntimeState level = State({
		"..",
	});
	const iggy::PlayerAgentState player = PlayerAgent(iggy::ResourceId { "player:one" }, { 2.25F, 3.75F }, { 2, 3 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::East);

	const iggy::runtime::RuntimeSessionBuildResult result = iggy::runtime::RuntimeSessionBuilder {}.build(level, ConfigWithPlayer(false, player));

	Expect(result.built, "session build with player and no render cache should succeed");
	Expect(!result.state.hasRenderCache, "session with player and no render cache should leave render cache disabled");
	Expect(result.state.hasPlayer, "session with player and no render cache should set hasPlayer");
	ExpectPlayerAgent(result.state.player, player, "session without render cache");
}

void TestInvalidRenderCacheConfigDoesNotPublishPlayer()
{
	const iggy::LevelRuntimeState level = State({
		"..",
	});
	iggy::runtime::RuntimeSessionBuildConfig config = ConfigWithPlayer(true, PlayerAgent(iggy::ResourceId { "player:one" }, { 2.25F, 3.75F }, { 2, 3 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::East));
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
	ExpectPlayerAgent(result.state.player, player, "session with default player");
}

void TestEmptyMapBuildsEmptyRenderCache()
{
	const iggy::LevelRuntimeState level;

	const iggy::runtime::RuntimeSessionBuildResult result = iggy::runtime::RuntimeSessionBuilder {}.build(level, Config());

	Expect(result.built, "empty map session with valid cache config should build");
	Expect(result.state.hasRenderCache, "empty map session should still carry render cache state");
	Expect(result.renderCache.built, "empty map session should expose successful render cache build");
	Expect(result.state.renderCache.tileChunks.chunks.empty(), "empty map session should build empty tile render cache");
	Expect(result.state.derivedCaches.hasRenderCache, "empty map legacy render cache should mirror into derived caches");
	Expect(result.state.derivedCaches.render.tileChunks.chunks.empty(), "empty map derived render mirror should be empty");
}

void TestDerivedCollisionOnlyBuildStoresCollisionCache()
{
	const iggy::LevelRuntimeState level = State({
		".#",
	});
	iggy::runtime::RuntimeSessionBuildConfig config = Config(false);
	config.buildDerivedCaches = true;
	config.derivedCacheConfig = DerivedConfig(false, true);

	const iggy::runtime::RuntimeSessionBuildResult result = iggy::runtime::RuntimeSessionBuilder {}.build(level, config);

	Expect(result.built, "derived collision-only session should build");
	Expect(!result.state.hasRenderCache, "derived collision-only session should leave legacy render cache absent");
	Expect(result.state.renderCache.tileChunks.chunks.empty(), "derived collision-only session should leave legacy render cache default");
	Expect(!result.state.derivedCaches.hasRenderCache, "derived collision-only session should leave derived render cache absent");
	Expect(result.state.derivedCaches.hasCollisionCache, "derived collision-only session should store collision cache");
	Expect(result.derivedCaches.built && result.derivedCaches.collision.built, "derived collision-only session should expose derived collision diagnostics");
	Expect(result.state.derivedCaches.collision.world.objects().size() == 1, "derived collision-only session should build one collision object");
	if (result.state.derivedCaches.collision.world.objects().size() == 1)
		ExpectBounds(result.state.derivedCaches.collision.world.objects()[0].shape.bounds, iggy::tileBounds({ 1, 0 }), "derived collision-only session should use blocked tile bounds");
}

void TestDerivedRenderAndCollisionBuildMirrorsLegacyRender()
{
	const iggy::LevelRuntimeState level = State({
		".#",
		"..",
	});
	iggy::runtime::RuntimeSessionBuildConfig config = Config(false);
	config.buildDerivedCaches = true;
	config.derivedCacheConfig = DerivedConfig(true, true, ChunkConfig(2, 2));

	const iggy::runtime::RuntimeSessionBuildResult result = iggy::runtime::RuntimeSessionBuilder {}.build(level, config);

	Expect(result.built, "derived render+collision session should build");
	Expect(result.state.derivedCaches.hasRenderCache, "derived render+collision session should store derived render cache");
	Expect(result.state.derivedCaches.hasCollisionCache, "derived render+collision session should store derived collision cache");
	Expect(result.state.hasRenderCache, "derived render+collision session should mirror render cache to legacy flag");
	Expect(result.state.renderCache.tileChunks.chunks.size() == result.state.derivedCaches.render.tileChunks.chunks.size(), "derived render+collision session should mirror render chunks to legacy field");
	Expect(result.renderCache.built, "derived render+collision session should mirror render diagnostics to legacy result");
	Expect(result.state.derivedCaches.collision.world.objects().size() == 1, "derived render+collision session should build collision object");
}

void TestDerivedBuildPathWinsWhenBothLegacyAndDerivedFlagsAreTrue()
{
	const iggy::LevelRuntimeState level = State({
		"..",
		"..",
	});
	iggy::runtime::RuntimeSessionBuildConfig config = Config(true, 0, 2);
	config.buildDerivedCaches = true;
	config.derivedCacheConfig = DerivedConfig(true, false, ChunkConfig(2, 2));

	const iggy::runtime::RuntimeSessionBuildResult result = iggy::runtime::RuntimeSessionBuilder {}.build(level, config);

	Expect(result.built, "derived build path should ignore invalid legacy render config when derived caches are enabled");
	Expect(result.state.hasRenderCache, "derived build path should mirror derived render cache to legacy flag");
	Expect(result.state.derivedCaches.hasRenderCache, "derived build path should store derived render cache");
	Expect(result.renderCache.built, "derived build path should expose derived render diagnostics through legacy render result");
	Expect(result.renderCache.tileChunkIssues.empty(), "derived build path should not expose invalid legacy render config diagnostics");
	Expect(result.state.renderCache.tileChunkConfig.chunkWidth == 2 && result.state.renderCache.tileChunkConfig.chunkHeight == 2, "derived build path should source legacy render field from derived render config");
}

void TestDerivedBuildFailureDoesNotPublishPartialSessionOrPlayer()
{
	const iggy::LevelRuntimeState level = State({
		".#",
	});
	iggy::runtime::RuntimeSessionBuildConfig config = ConfigWithPlayer(false);
	config.buildDerivedCaches = true;
	config.derivedCacheConfig = DerivedConfig(true, true, ChunkConfig(0, 2));

	const iggy::runtime::RuntimeSessionBuildResult result = iggy::runtime::RuntimeSessionBuilder {}.build(level, config);

	Expect(!result.built, "derived render failure should fail session build");
	Expect(!result.state.hasPlayer, "failed derived session should not publish player");
	Expect(!result.state.hasRenderCache, "failed derived session should not publish legacy render cache");
	Expect(!result.state.derivedCaches.hasRenderCache && !result.state.derivedCaches.hasCollisionCache, "failed derived session should not publish partial derived caches");
	Expect(!result.derivedCaches.built, "failed derived session should preserve failed derived result");
	Expect(!result.derivedCaches.render.built, "failed derived session should preserve nested render failure");
	Expect(result.derivedCaches.render.tileChunkIssues.size() == 1, "failed derived session should preserve render diagnostics");
	Expect(result.derivedCaches.collision.built, "failed derived session should preserve successful nested collision diagnostics without publishing state");
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
	const iggy::PlayerAgentState player = PlayerAgent(iggy::ResourceId { "player:one" }, { 2.25F, 3.75F }, { 2, 3 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::East);
	const iggy::PlayerAgentState original = player;

	const iggy::runtime::RuntimeSessionBuildResult result = iggy::runtime::RuntimeSessionBuilder {}.build(level, ConfigWithPlayer(false, player));

	Expect(result.built, "input player immutability setup should build");
	ExpectPlayerAgent(player, original, "input player");
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
	TestDerivedCollisionOnlyBuildStoresCollisionCache();
	TestDerivedRenderAndCollisionBuildMirrorsLegacyRender();
	TestDerivedBuildPathWinsWhenBothLegacyAndDerivedFlagsAreTrue();
	TestDerivedBuildFailureDoesNotPublishPartialSessionOrPlayer();
	TestNpcAgentsArePreserved();
	TestInputCopyIsNotMutated();
	TestInputPlayerIsNotMutated();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
