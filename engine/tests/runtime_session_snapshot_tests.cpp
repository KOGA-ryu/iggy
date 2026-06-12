#include <cstdlib>
#include <string_view>
#include <vector>

#include "runtime/RuntimeSessionSnapshot.hpp"
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

iggy::LevelRuntimeState Level(std::vector<std::string_view> rows)
{
	iggy::LevelRuntimeState level;
	level.map = MapFromRows(rows);
	level.map.id = iggy::ResourceId("level:snapshot");
	level.npcAgents.push_back({ iggy::ResourceId("npc:one"), {} });
	return level;
}

iggy::LevelTileRenderChunkCacheConfig RenderConfig(int chunkWidth = 2, int chunkHeight = 1, int layer = 4)
{
	return { chunkWidth, chunkHeight, { { WalkableMaterial, BlockedMaterial }, layer } };
}

iggy::LevelDerivedCacheBuildConfig DerivedConfig(
	bool render,
	bool collision,
	iggy::LevelTileRenderChunkCacheConfig renderConfig = RenderConfig())
{
	iggy::LevelDerivedCacheBuildConfig config;
	config.buildRenderCache = render;
	config.renderCacheConfig = renderConfig;
	config.buildCollisionCache = collision;
	return config;
}

iggy::LevelDerivedCacheState BuildCaches(
	const iggy::LevelRuntimeState &level,
	bool render = true,
	bool collision = true,
	iggy::LevelTileRenderChunkCacheConfig renderConfig = RenderConfig())
{
	const iggy::LevelDerivedCacheBuildResult build = iggy::LevelDerivedCacheBuilder {}.build(level, DerivedConfig(render, collision, renderConfig));
	Expect(build.built, "snapshot fixture should build derived caches");
	return build.state;
}

iggy::runtime::RuntimeSessionState Session(
	const iggy::LevelRuntimeState &level,
	const iggy::LevelDerivedCacheState &caches,
	bool hasPlayer = true)
{
	iggy::runtime::RuntimeSessionState session;
	session.level = level;
	session.tickIndex = 27;
	session.derivedCaches = caches;
	session.hasRenderCache = caches.hasRenderCache;
	if (caches.hasRenderCache)
		session.renderCache = caches.render;
	session.hasPlayer = hasPlayer;
	if (hasPlayer)
		session.player = PlayerAgent(iggy::ResourceId("player:snapshot"), { 3.0F, 4.0F }, { 3, 4 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::North);
	return session;
}

iggy::runtime::RuntimeSessionBuildConfig BuildConfigNoCaches()
{
	iggy::runtime::RuntimeSessionBuildConfig config;
	config.buildRenderCache = false;
	return config;
}

iggy::runtime::RuntimeSessionBuildConfig BuildConfigDerived(
	bool render,
	bool collision,
	iggy::LevelTileRenderChunkCacheConfig renderConfig = RenderConfig())
{
	iggy::runtime::RuntimeSessionBuildConfig config;
	config.buildRenderCache = false;
	config.buildDerivedCaches = true;
	config.derivedCacheConfig = DerivedConfig(render, collision, renderConfig);
	return config;
}

iggy::runtime::RuntimeSessionSnapshotRestoreConfig RestoreConfig(iggy::runtime::RuntimeSessionBuildConfig buildConfig)
{
	return { buildConfig };
}

void ExpectLevelSame(const iggy::LevelRuntimeState &actual, const iggy::LevelRuntimeState &expected, const char *message)
{
	Expect(actual.map.id == expected.map.id, message);
	Expect(actual.map.width == expected.map.width && actual.map.height == expected.map.height, message);
	Expect(actual.map.tiles.size() == expected.map.tiles.size(), message);
	for (std::size_t index = 0; index < actual.map.tiles.size() && index < expected.map.tiles.size(); ++index)
		Expect(actual.map.tiles[index].walkable == expected.map.tiles[index].walkable, message);
	Expect(actual.npcAgents.size() == expected.npcAgents.size(), message);
}

void ExpectSessionSame(const iggy::runtime::RuntimeSessionState &actual, const iggy::runtime::RuntimeSessionState &expected, const char *message)
{
	ExpectLevelSame(actual.level, expected.level, message);
	Expect(actual.tickIndex == expected.tickIndex, message);
	Expect(actual.hasPlayer == expected.hasPlayer, message);
	ExpectPlayerAgent(actual.player, expected.player, message);
	Expect(actual.hasRenderCache == expected.hasRenderCache, message);
	Expect(actual.derivedCaches.hasRenderCache == expected.derivedCaches.hasRenderCache, message);
	Expect(actual.derivedCaches.hasCollisionCache == expected.derivedCaches.hasCollisionCache, message);
}

void ExpectSnapshotSame(const iggy::runtime::RuntimeSessionSnapshot &actual, const iggy::runtime::RuntimeSessionSnapshot &expected, const char *message)
{
	ExpectLevelSame(actual.level, expected.level, message);
	Expect(actual.tickIndex == expected.tickIndex, message);
	Expect(actual.hasPlayer == expected.hasPlayer, message);
	ExpectPlayerAgent(actual.player, expected.player, message);
}

void TestCaptureIncludesAuthoritativeState()
{
	const iggy::LevelRuntimeState level = Level({ ".#", ".." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));

	const iggy::runtime::RuntimeSessionSnapshot snapshot = iggy::runtime::RuntimeSessionSnapshotBuilder {}.capture(session);

	ExpectLevelSame(snapshot.level, session.level, "snapshot capture should include level state");
	Expect(snapshot.tickIndex == session.tickIndex, "snapshot capture should include tick index");
	Expect(snapshot.hasPlayer == session.hasPlayer, "snapshot capture should include player presence");
	ExpectPlayerAgent(snapshot.player, session.player, "snapshot capture");
}

void TestCaptureExcludesCachesByRestoreBehavior()
{
	const iggy::LevelRuntimeState level = Level({ ".#", ".." });
	iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	session.renderCache.tileChunks.chunks.clear();
	session.derivedCaches.hasRenderCache = false;
	session.derivedCaches.hasCollisionCache = false;

	const iggy::runtime::RuntimeSessionSnapshot snapshot = iggy::runtime::RuntimeSessionSnapshotBuilder {}.capture(session);
	const iggy::runtime::RuntimeSessionSnapshotRestoreResult restored = iggy::runtime::RuntimeSessionSnapshotRestorer {}.restore(snapshot, RestoreConfig(BuildConfigDerived(true, true)));

	Expect(restored.restored, "snapshot restore with derived caches should succeed after source caches were mutated");
	Expect(restored.session.derivedCaches.hasRenderCache, "snapshot restore should rebuild render cache instead of using source cache fields");
	Expect(restored.session.derivedCaches.hasCollisionCache, "snapshot restore should rebuild collision cache instead of using source cache fields");
	Expect(restored.session.renderCache.tileChunks.chunks.size() == restored.session.derivedCaches.render.tileChunks.chunks.size(), "snapshot restore should mirror rebuilt render cache");
	Expect(restored.session.derivedCaches.collision.world.objects().size() == 1, "snapshot restore should rebuild collision cache from authoritative level");
}

void TestRestoreWithoutCacheBuilding()
{
	const iggy::LevelRuntimeState level = Level({ ".#", ".." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	const iggy::runtime::RuntimeSessionSnapshot snapshot = iggy::runtime::RuntimeSessionSnapshotBuilder {}.capture(session);

	const iggy::runtime::RuntimeSessionSnapshotRestoreResult result = iggy::runtime::RuntimeSessionSnapshotRestorer {}.restore(snapshot, RestoreConfig(BuildConfigNoCaches()));

	Expect(result.restored, "snapshot restore without caches should succeed");
	ExpectLevelSame(result.session.level, snapshot.level, "snapshot restore without caches should preserve level");
	Expect(result.session.tickIndex == snapshot.tickIndex, "snapshot restore should restore tick index after builder reset");
	Expect(result.session.hasPlayer == snapshot.hasPlayer, "snapshot restore without caches should preserve hasPlayer");
	ExpectPlayerAgent(result.session.player, snapshot.player, "snapshot restore without caches");
	Expect(!result.session.hasRenderCache, "snapshot restore without caches should leave legacy render absent");
	Expect(!result.session.derivedCaches.hasRenderCache, "snapshot restore without caches should leave derived render absent");
	Expect(!result.session.derivedCaches.hasCollisionCache, "snapshot restore without caches should leave derived collision absent");
}

void TestRestoreWithDerivedCachesRebuildsFromSnapshotLevel()
{
	const iggy::LevelRuntimeState level = Level({ ".#", ".." });
	const iggy::runtime::RuntimeSessionSnapshot snapshot = iggy::runtime::RuntimeSessionSnapshotBuilder {}.capture(Session(level, BuildCaches(level)));

	const iggy::runtime::RuntimeSessionSnapshotRestoreResult result = iggy::runtime::RuntimeSessionSnapshotRestorer {}.restore(snapshot, RestoreConfig(BuildConfigDerived(true, true)));

	Expect(result.restored, "snapshot restore with derived caches should succeed");
	Expect(result.session.tickIndex == snapshot.tickIndex, "snapshot restore with derived caches should restore tick index after builder reset");
	Expect(result.session.derivedCaches.hasRenderCache, "snapshot restore with derived caches should rebuild render cache");
	Expect(result.session.derivedCaches.hasCollisionCache, "snapshot restore with derived caches should rebuild collision cache");
	Expect(result.session.hasRenderCache, "snapshot restore with derived render cache should mirror legacy render flag");
	Expect(result.session.renderCache.tileChunks.chunks.size() == result.session.derivedCaches.render.tileChunks.chunks.size(), "snapshot restore should mirror rebuilt render chunks");
	Expect(result.session.derivedCaches.collision.world.objects().size() == 1, "snapshot restore should rebuild collision objects from blocked tiles");
	if (result.session.derivedCaches.collision.world.objects().size() == 1)
		ExpectBounds(result.session.derivedCaches.collision.world.objects()[0].shape.bounds, iggy::tileBounds({ 1, 0 }), "snapshot restore collision cache should use snapshot level tile bounds");
}

void TestRestorePlayerComesFromSnapshot()
{
	const iggy::LevelRuntimeState level = Level({ ".." });
	const iggy::PlayerAgentState snapshotPlayer = PlayerAgent(iggy::ResourceId("player:snapshot"), { 5.0F, 6.0F }, { 5, 6 }, iggy::PlayerMovementStatus::Idle, iggy::PlayerFacing2D::South);
	iggy::runtime::RuntimeSessionState session = Session(level, {}, true);
	session.player = snapshotPlayer;
	const iggy::runtime::RuntimeSessionSnapshot snapshot = iggy::runtime::RuntimeSessionSnapshotBuilder {}.capture(session);
	iggy::runtime::RuntimeSessionBuildConfig buildConfig = BuildConfigNoCaches();
	buildConfig.hasPlayer = false;
	buildConfig.player = PlayerAgent(iggy::ResourceId("player:config"), { 1.0F, 1.0F }, { 1, 1 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::West);

	const iggy::runtime::RuntimeSessionSnapshotRestoreResult result = iggy::runtime::RuntimeSessionSnapshotRestorer {}.restore(snapshot, RestoreConfig(buildConfig));

	Expect(result.restored, "snapshot restore player override setup should succeed");
	Expect(result.session.hasPlayer, "snapshot restore should use hasPlayer from snapshot");
	ExpectPlayerAgent(result.session.player, snapshotPlayer, "snapshot restore should use player from snapshot instead of config");
}

void TestRestoreBuildFailurePreservesDiagnosticsAndPublishesNoSession()
{
	const iggy::LevelRuntimeState level = Level({ ".." });
	const iggy::runtime::RuntimeSessionSnapshot snapshot = iggy::runtime::RuntimeSessionSnapshotBuilder {}.capture(Session(level, {}));

	const iggy::runtime::RuntimeSessionSnapshotRestoreResult result = iggy::runtime::RuntimeSessionSnapshotRestorer {}.restore(snapshot, RestoreConfig(BuildConfigDerived(true, false, RenderConfig(0, 1, 8))));

	Expect(!result.restored, "snapshot restore with invalid cache config should fail");
	Expect(!result.build.built, "snapshot restore failure should preserve failed build result");
	Expect(result.build.derivedCaches.render.tileChunkIssues.size() == 1, "snapshot restore failure should preserve render diagnostics");
	Expect(result.session.tickIndex == 0, "snapshot restore failure should not publish partial session");
	Expect(!result.session.hasPlayer, "snapshot restore failure should leave session default");
}

void TestCaptureAndRestoreDoNotMutateInputs()
{
	iggy::LevelRuntimeState level = Level({ ".#", ".." });
	iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	const iggy::runtime::RuntimeSessionState sessionBefore = session;
	iggy::runtime::RuntimeSessionSnapshot snapshot = iggy::runtime::RuntimeSessionSnapshotBuilder {}.capture(session);
	const iggy::runtime::RuntimeSessionSnapshot snapshotBefore = snapshot;
	iggy::runtime::RuntimeSessionSnapshotRestoreConfig config = RestoreConfig(BuildConfigDerived(true, true));
	const iggy::runtime::RuntimeSessionSnapshotRestoreConfig configBefore = config;

	const iggy::runtime::RuntimeSessionSnapshotRestoreResult result = iggy::runtime::RuntimeSessionSnapshotRestorer {}.restore(snapshot, config);

	Expect(result.restored, "snapshot immutability setup should restore");
	ExpectSessionSame(session, sessionBefore, "snapshot capture/restore should not mutate source session");
	ExpectSnapshotSame(snapshot, snapshotBefore, "snapshot restore should not mutate snapshot");
	Expect(config.buildConfig.buildRenderCache == configBefore.buildConfig.buildRenderCache, "snapshot restore should not mutate restore config legacy render flag");
	Expect(config.buildConfig.buildDerivedCaches == configBefore.buildConfig.buildDerivedCaches, "snapshot restore should not mutate restore config derived flag");
	Expect(config.buildConfig.hasPlayer == configBefore.buildConfig.hasPlayer, "snapshot restore should not mutate restore config player flag");
}

} // namespace

int main()
{
	TestCaptureIncludesAuthoritativeState();
	TestCaptureExcludesCachesByRestoreBehavior();
	TestRestoreWithoutCacheBuilding();
	TestRestoreWithDerivedCachesRebuildsFromSnapshotLevel();
	TestRestorePlayerComesFromSnapshot();
	TestRestoreBuildFailurePreservesDiagnosticsAndPublishesNoSession();
	TestCaptureAndRestoreDoNotMutateInputs();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
