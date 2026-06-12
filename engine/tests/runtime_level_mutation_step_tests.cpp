#include <cstdlib>
#include <vector>

#include "modules/npc_ai/NpcAgentBatch.hpp"
#include "runtime/RuntimeLevelMutationStep.hpp"
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
using iggy::test::MapFromRows;
using iggy::test::PlayerAgent;

const iggy::ResourceId WalkableMaterial { "material:floor" };
const iggy::ResourceId BlockedMaterial { "material:wall" };

iggy::LevelTileEdit Edit(iggy::TileCoord tile, bool walkable)
{
	return { tile, walkable };
}

iggy::LevelRuntimeState Level(std::initializer_list<std::string_view> rows)
{
	iggy::LevelRuntimeState level;
	level.map = MapFromRows(std::vector<std::string_view>(rows));
	level.npcAgents.push_back({ iggy::ResourceId("npc:one"), {} });
	return level;
}

iggy::LevelTileRenderChunkCacheConfig RenderConfig(int chunkWidth = 2, int chunkHeight = 1, int layer = 4)
{
	return { chunkWidth, chunkHeight, { { WalkableMaterial, BlockedMaterial }, layer } };
}

iggy::LevelDerivedCacheState BuildCaches(
	const iggy::LevelRuntimeState &level,
	bool render,
	bool collision,
	iggy::LevelTileRenderChunkCacheConfig renderConfig = RenderConfig())
{
	iggy::LevelDerivedCacheBuildConfig config;
	config.buildRenderCache = render;
	config.renderCacheConfig = renderConfig;
	config.buildCollisionCache = collision;
	const iggy::LevelDerivedCacheBuildResult build = iggy::LevelDerivedCacheBuilder {}.build(level, config);
	Expect(build.built, "runtime mutation cache fixture should build derived caches");
	return build.state;
}

iggy::runtime::RuntimeSessionState Session(
	const iggy::LevelRuntimeState &level,
	const iggy::LevelDerivedCacheState &caches = {},
	bool hasPlayer = true)
{
	iggy::runtime::RuntimeSessionState session;
	session.level = level;
	session.derivedCaches = caches;
	session.tickIndex = 42;
	session.hasPlayer = hasPlayer;
	if (hasPlayer)
		session.player = PlayerAgent(iggy::ResourceId("player:one"), { 2.0F, 3.0F }, { 2, 3 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::West);
	session.hasRenderCache = caches.hasRenderCache;
	if (caches.hasRenderCache)
		session.renderCache = caches.render;
	return session;
}

bool CommandsContainMaterial(const iggy::LevelRenderCacheState &cache, iggy::ResourceId material)
{
	for (const iggy::LevelTileRenderChunk &chunk : cache.tileChunks.chunks) {
		for (const iggy::render::RenderCommand2D &command : chunk.commands.commands) {
			if (command.materialId == material)
				return true;
		}
	}
	return false;
}

void ExpectMapSame(const iggy::LevelTileMap &actual, const iggy::LevelTileMap &expected, const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.width == expected.width && actual.height == expected.height, message);
	Expect(actual.tiles.size() == expected.tiles.size(), message);
	for (std::size_t index = 0; index < actual.tiles.size() && index < expected.tiles.size(); ++index)
		Expect(actual.tiles[index].walkable == expected.tiles[index].walkable, message);
	Expect(actual.entitySpawns.size() == expected.entitySpawns.size(), message);
}

void ExpectLevelSame(const iggy::LevelRuntimeState &actual, const iggy::LevelRuntimeState &expected, const char *message)
{
	ExpectMapSame(actual.map, expected.map, message);
	Expect(actual.npcAgents.size() == expected.npcAgents.size(), message);
}

void ExpectCollisionObjectsSame(
	const iggy::physics2d::CollisionWorld2D &actual,
	const iggy::physics2d::CollisionWorld2D &expected,
	const char *message)
{
	Expect(actual.objects().size() == expected.objects().size(), message);
	for (std::size_t index = 0; index < actual.objects().size() && index < expected.objects().size(); ++index)
		ExpectBounds(actual.objects()[index].shape.bounds, expected.objects()[index].shape.bounds, message);
}

void ExpectSessionStableExceptCaches(
	const iggy::runtime::RuntimeSessionState &actual,
	const iggy::runtime::RuntimeSessionState &expected,
	const char *message)
{
	Expect(actual.tickIndex == expected.tickIndex, message);
	Expect(actual.hasPlayer == expected.hasPlayer, message);
	ExpectPlayerAgent(actual.player, expected.player, message);
	Expect(actual.level.npcAgents.size() == expected.level.npcAgents.size(), message);
}

void ExpectSessionSame(const iggy::runtime::RuntimeSessionState &actual, const iggy::runtime::RuntimeSessionState &expected, const char *message)
{
	ExpectLevelSame(actual.level, expected.level, message);
	ExpectSessionStableExceptCaches(actual, expected, message);
	Expect(actual.hasRenderCache == expected.hasRenderCache, message);
	Expect(actual.derivedCaches.hasRenderCache == expected.derivedCaches.hasRenderCache, message);
	Expect(actual.derivedCaches.hasCollisionCache == expected.derivedCaches.hasCollisionCache, message);
	Expect(actual.renderCache.tileChunks.chunks.size() == expected.renderCache.tileChunks.chunks.size(), message);
	ExpectCollisionObjectsSame(actual.derivedCaches.collision.world, expected.derivedCaches.collision.world, message);
}

void ExpectEditsSame(
	const std::vector<iggy::LevelTileEdit> &actual,
	const std::vector<iggy::LevelTileEdit> &expected,
	const char *message)
{
	Expect(actual.size() == expected.size(), message);
	for (std::size_t index = 0; index < actual.size() && index < expected.size(); ++index) {
		Expect(actual[index].tile == expected[index].tile, message);
		Expect(actual[index].walkable == expected[index].walkable, message);
	}
}

void TestEmptyEditsReturnsNoMutationAndConsistentRenderMirror()
{
	const iggy::LevelRuntimeState level = Level({ "." });
	iggy::runtime::RuntimeSessionState session = Session(level);
	session.hasRenderCache = true;
	session.renderCache.tileChunks.chunks.push_back({});

	const iggy::runtime::RuntimeLevelMutationResult result = iggy::runtime::RuntimeLevelMutationStep {}.apply(session, {});

	Expect(result.status == iggy::runtime::RuntimeLevelMutationStatus::NoMutation, "empty edits should return NoMutation");
	Expect(result.levelMutation.status == iggy::LevelMutationCacheUpdateStatus::NoMutation, "empty edits should preserve scene no-mutation status");
	ExpectLevelSame(result.session.level, session.level, "empty edits should preserve level");
	ExpectSessionStableExceptCaches(result.session, session, "empty edits should preserve unrelated session fields");
	Expect(!result.session.hasRenderCache, "empty edits should clear stale legacy render mirror when derived render cache is absent");
	Expect(result.session.renderCache.tileChunks.chunks.empty(), "empty edits should clear stale legacy render cache field");
}

void TestValidEditWithNoCachesAppliesLevelOnly()
{
	const iggy::LevelRuntimeState level = Level({ "." });
	const iggy::runtime::RuntimeSessionState session = Session(level, {});

	const iggy::runtime::RuntimeLevelMutationResult result = iggy::runtime::RuntimeLevelMutationStep {}.apply(session, { Edit({ 0, 0 }, false) });

	Expect(result.status == iggy::runtime::RuntimeLevelMutationStatus::Applied, "valid edit without caches should be applied");
	Expect(result.levelMutation.status == iggy::LevelMutationCacheUpdateStatus::MutationOnly, "valid edit without caches should preserve scene MutationOnly status");
	Expect(result.session.level.map.tileAt(0, 0) != nullptr && !result.session.level.map.tileAt(0, 0)->walkable, "valid edit without caches should update session level map");
	Expect(!result.session.derivedCaches.hasRenderCache && !result.session.derivedCaches.hasCollisionCache, "valid edit without caches should leave derived caches absent");
	Expect(!result.session.hasRenderCache, "valid edit without caches should leave legacy render mirror absent");
	ExpectSessionStableExceptCaches(result.session, session, "valid edit without caches should preserve unrelated session fields");
}

void TestValidEditUpdatesCollisionCache()
{
	const iggy::LevelRuntimeState level = Level({ "..." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level, false, true));

	const iggy::runtime::RuntimeLevelMutationResult result = iggy::runtime::RuntimeLevelMutationStep {}.apply(session, { Edit({ 1, 0 }, false) });

	Expect(result.status == iggy::runtime::RuntimeLevelMutationStatus::Applied, "collision cache edit should be applied");
	Expect(result.session.derivedCaches.hasCollisionCache, "collision cache edit should preserve collision cache");
	Expect(result.session.derivedCaches.collision.world.objects().size() == 1, "collision cache edit should add collision object");
	if (result.session.derivedCaches.collision.world.objects().size() == 1)
		ExpectBounds(result.session.derivedCaches.collision.world.objects()[0].shape.bounds, iggy::tileBounds({ 1, 0 }), "collision cache edit should refresh collision object bounds");
	ExpectSessionStableExceptCaches(result.session, session, "collision cache edit should preserve unrelated session fields");
}

void TestValidEditUpdatesRenderCacheMirror()
{
	const iggy::LevelRuntimeState level = Level({ ".." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level, true, false));

	const iggy::runtime::RuntimeLevelMutationResult result = iggy::runtime::RuntimeLevelMutationStep {}.apply(session, { Edit({ 1, 0 }, false) });

	Expect(result.status == iggy::runtime::RuntimeLevelMutationStatus::Applied, "render cache edit should be applied");
	Expect(result.session.derivedCaches.hasRenderCache, "render cache edit should preserve derived render cache");
	Expect(result.session.hasRenderCache, "render cache edit should mirror legacy hasRenderCache");
	Expect(result.session.renderCache.tileChunks.chunks.size() == result.session.derivedCaches.render.tileChunks.chunks.size(), "render cache edit should mirror legacy render chunks");
	Expect(CommandsContainMaterial(result.session.derivedCaches.render, BlockedMaterial), "render cache edit should refresh derived render commands");
	Expect(CommandsContainMaterial(result.session.renderCache, BlockedMaterial), "render cache edit should refresh legacy render mirror");
	ExpectSessionStableExceptCaches(result.session, session, "render cache edit should preserve unrelated session fields");
}

void TestValidEditUpdatesBothCaches()
{
	const iggy::LevelRuntimeState level = Level({ ".." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level, true, true));

	const iggy::runtime::RuntimeLevelMutationResult result = iggy::runtime::RuntimeLevelMutationStep {}.apply(session, { Edit({ 0, 0 }, false) });

	Expect(result.status == iggy::runtime::RuntimeLevelMutationStatus::Applied, "both-cache edit should be applied");
	Expect(result.session.derivedCaches.hasRenderCache, "both-cache edit should preserve render cache");
	Expect(result.session.derivedCaches.hasCollisionCache, "both-cache edit should preserve collision cache");
	Expect(result.session.hasRenderCache, "both-cache edit should mirror legacy render cache");
	Expect(CommandsContainMaterial(result.session.renderCache, BlockedMaterial), "both-cache edit should refresh render mirror");
	Expect(result.session.derivedCaches.collision.world.objects().size() == 1, "both-cache edit should refresh collision cache");
}

void TestCacheUpdateFailureReturnsInputSessionUnchanged()
{
	const iggy::LevelRuntimeState level = Level({ ".." });
	iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level, false, true));
	session.derivedCaches.hasRenderCache = true;
	session.derivedCaches.render.tileChunkConfig = RenderConfig(0, 1, 7);
	session.hasRenderCache = true;
	session.renderCache = session.derivedCaches.render;
	const iggy::runtime::RuntimeSessionState before = session;

	const iggy::runtime::RuntimeLevelMutationResult result = iggy::runtime::RuntimeLevelMutationStep {}.apply(session, { Edit({ 1, 0 }, false) });

	Expect(result.status == iggy::runtime::RuntimeLevelMutationStatus::Failed, "cache update failure should return Failed");
	Expect(result.levelMutation.status == iggy::LevelMutationCacheUpdateStatus::CacheUpdateFailed, "cache update failure should preserve scene failure status");
	Expect(result.levelMutation.cacheUpdate.render.dirtyChunks.issues.size() == 1, "cache update failure should preserve render diagnostics");
	ExpectSessionSame(result.session, before, "cache update failure should return input session unchanged");
}

void TestMixedValidInvalidEditsPreserveDiagnosticsAndApplyValidChanges()
{
	const iggy::LevelRuntimeState level = Level({ "..." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level, false, true));
	const std::vector<iggy::LevelTileEdit> edits {
		Edit({ 0, 0 }, false),
		Edit({ 9, 0 }, false),
		Edit({ 2, 0 }, false),
	};

	const iggy::runtime::RuntimeLevelMutationResult result = iggy::runtime::RuntimeLevelMutationStep {}.apply(session, edits);

	Expect(result.status == iggy::runtime::RuntimeLevelMutationStatus::Applied, "mixed edits should apply valid changes");
	Expect(result.levelMutation.mutation.issues.size() == 1, "mixed edits should preserve mutation issue");
	if (result.levelMutation.mutation.issues.size() == 1)
		Expect(result.levelMutation.mutation.issues[0].code == iggy::LevelTileEditIssueCode::OutOfBounds, "mixed edits should preserve out-of-bounds issue");
	Expect(result.levelMutation.mutation.changedTiles.size() == 2, "mixed edits should preserve changed tiles from valid changes");
	Expect(result.session.derivedCaches.collision.world.objects().size() == 2, "mixed edits should update collision cache from valid changes");
}

void TestInputsAreNotMutatedAndTickDoesNotAdvance()
{
	const iggy::LevelRuntimeState level = Level({ ".#" });
	iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level, true, true));
	session.tickIndex = 99;
	const iggy::runtime::RuntimeSessionState before = session;
	std::vector<iggy::LevelTileEdit> edits {
		Edit({ 0, 0 }, false),
		Edit({ 1, 0 }, true),
	};
	const std::vector<iggy::LevelTileEdit> editsBefore = edits;

	const iggy::runtime::RuntimeLevelMutationResult result = iggy::runtime::RuntimeLevelMutationStep {}.apply(session, edits);

	Expect(result.status == iggy::runtime::RuntimeLevelMutationStatus::Applied, "immutability setup should apply edits");
	ExpectSessionSame(session, before, "runtime level mutation should not mutate input session");
	ExpectEditsSame(edits, editsBefore, "runtime level mutation should not mutate edit vector");
	Expect(result.session.tickIndex == before.tickIndex, "runtime level mutation should not advance tickIndex");
	ExpectPlayerAgent(result.session.player, before.player, "runtime level mutation should preserve player");
	Expect(result.session.hasPlayer == before.hasPlayer, "runtime level mutation should preserve hasPlayer");
}

} // namespace

int main()
{
	TestEmptyEditsReturnsNoMutationAndConsistentRenderMirror();
	TestValidEditWithNoCachesAppliesLevelOnly();
	TestValidEditUpdatesCollisionCache();
	TestValidEditUpdatesRenderCacheMirror();
	TestValidEditUpdatesBothCaches();
	TestCacheUpdateFailureReturnsInputSessionUnchanged();
	TestMixedValidInvalidEditsPreserveDiagnosticsAndApplyValidChanges();
	TestInputsAreNotMutatedAndTickDoesNotAdvance();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
