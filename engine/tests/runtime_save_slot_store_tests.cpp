#include <cstdlib>
#include <filesystem>
#include <string_view>
#include <vector>

#include "runtime/RuntimeSaveSlotStore.hpp"
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
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

const iggy::ResourceId WalkableMaterial { "material:floor" };
const iggy::ResourceId BlockedMaterial { "material:wall" };

std::filesystem::path TempRoot()
{
	return std::filesystem::current_path() / "runtime_save_slot_store_tests_tmp";
}

void ResetTempRoot()
{
	std::error_code ignored;
	std::filesystem::remove_all(TempRoot(), ignored);
	std::filesystem::create_directories(TempRoot(), ignored);
}

void RemoveTempRoot()
{
	std::error_code ignored;
	std::filesystem::remove_all(TempRoot(), ignored);
}

void CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind kind)
{
	const char *name = "manual";
	if (kind == iggy::runtime::RuntimeSaveSlotKind::Auto)
		name = "auto";
	if (kind == iggy::runtime::RuntimeSaveSlotKind::Quick)
		name = "quick";
	std::filesystem::create_directories(TempRoot() / name);
}

iggy::runtime::RuntimeSaveSlotPathPolicyConfig PathConfig(std::filesystem::path base = TempRoot())
{
	return { base, ".igsave" };
}

iggy::runtime::RuntimeSaveSlotId Slot(
	iggy::runtime::RuntimeSaveSlotKind kind,
	std::string name)
{
	return { kind, name };
}

iggy::LevelRuntimeState Level(std::vector<std::string_view> rows)
{
	iggy::LevelRuntimeState level;
	level.map = MapFromRows(rows);
	level.map.id = iggy::ResourceId("level:slot-store");
	level.map.playerStart = { 0, 0 };
	level.map.entitySpawns.push_back({ iggy::ResourceId("npc:type"), 1, 1, iggy::ResourceId("spawn:slot") });
	iggy::npc_ai::NpcAgentEntry npc;
	npc.id = iggy::ResourceId("npc:slot");
	npc.state.position = { 1.25F, 1.25F };
	npc.state.homeTile = { 1, 1 };
	npc.state.awareness.playerVisible = true;
	npc.state.awareness.alerted = true;
	npc.state.awareness.alertTicksRemaining = 3;
	npc.state.awareness.lastSeenTile = { 0, 1 };
	npc.state.awareness.lastSeenPosition = { 0.25F, 1.25F };
	npc.state.followState.waypointIndex = 1;
	npc.state.followState.completed = true;
	level.npcAgents.push_back(npc);
	return level;
}

iggy::LevelTileRenderChunkCacheConfig RenderConfig(int chunkWidth = 2, int chunkHeight = 1, int layer = 4)
{
	return { chunkWidth, chunkHeight, { { WalkableMaterial, BlockedMaterial }, layer } };
}

iggy::LevelDerivedCacheBuildConfig DerivedConfig(bool render, bool collision)
{
	iggy::LevelDerivedCacheBuildConfig config;
	config.buildRenderCache = render;
	config.renderCacheConfig = RenderConfig();
	config.buildCollisionCache = collision;
	return config;
}

iggy::LevelDerivedCacheState BuildCaches(const iggy::LevelRuntimeState &level)
{
	const iggy::LevelDerivedCacheBuildResult build = iggy::LevelDerivedCacheBuilder {}.build(level, DerivedConfig(true, true));
	Expect(build.built, "slot store fixture should build derived caches");
	return build.state;
}

iggy::runtime::RuntimeSessionState Session(const iggy::LevelRuntimeState &level, const iggy::LevelDerivedCacheState &caches)
{
	iggy::runtime::RuntimeSessionState session;
	session.level = level;
	session.tickIndex = 11;
	session.derivedCaches = caches;
	session.hasRenderCache = caches.hasRenderCache;
	if (caches.hasRenderCache)
		session.renderCache = caches.render;
	session.hasPlayer = true;
	session.player = PlayerAgent(
		iggy::ResourceId("player:slot"),
		{ 0.5F, 0.5F },
		{ 0, 0 },
		iggy::PlayerMovementStatus::Moving,
		iggy::PlayerFacing2D::South);
	return session;
}

iggy::runtime::RuntimeSessionBuildConfig BuildConfigNoCaches()
{
	iggy::runtime::RuntimeSessionBuildConfig config;
	config.buildRenderCache = false;
	return config;
}

iggy::runtime::RuntimeSessionBuildConfig BuildConfigDerived()
{
	iggy::runtime::RuntimeSessionBuildConfig config;
	config.buildRenderCache = false;
	config.buildDerivedCaches = true;
	config.derivedCacheConfig = DerivedConfig(true, true);
	return config;
}

iggy::runtime::RuntimeSessionSnapshotRestoreConfig RestoreConfig(iggy::runtime::RuntimeSessionBuildConfig buildConfig)
{
	return { buildConfig };
}

void ExpectLevelSame(const iggy::LevelRuntimeState &actual, const iggy::LevelRuntimeState &expected, const char *message)
{
	Expect(actual.map.id == expected.map.id, message);
	Expect(actual.map.width == expected.map.width, message);
	Expect(actual.map.height == expected.map.height, message);
	Expect(actual.map.playerStart.x == expected.map.playerStart.x && actual.map.playerStart.y == expected.map.playerStart.y, message);
	Expect(actual.map.tiles.size() == expected.map.tiles.size(), message);
	for (std::size_t index = 0; index < actual.map.tiles.size() && index < expected.map.tiles.size(); ++index)
		Expect(actual.map.tiles[index].walkable == expected.map.tiles[index].walkable, message);
	Expect(actual.map.entitySpawns.size() == expected.map.entitySpawns.size(), message);
	for (std::size_t index = 0; index < actual.map.entitySpawns.size() && index < expected.map.entitySpawns.size(); ++index) {
		Expect(actual.map.entitySpawns[index].type == expected.map.entitySpawns[index].type, message);
		Expect(actual.map.entitySpawns[index].x == expected.map.entitySpawns[index].x, message);
		Expect(actual.map.entitySpawns[index].y == expected.map.entitySpawns[index].y, message);
		Expect(actual.map.entitySpawns[index].id == expected.map.entitySpawns[index].id, message);
	}
	Expect(actual.npcAgents.size() == expected.npcAgents.size(), message);
	for (std::size_t index = 0; index < actual.npcAgents.size() && index < expected.npcAgents.size(); ++index) {
		Expect(actual.npcAgents[index].id == expected.npcAgents[index].id, message);
		Expect(NearVec(actual.npcAgents[index].state.position, expected.npcAgents[index].state.position), message);
		Expect(actual.npcAgents[index].state.homeTile == expected.npcAgents[index].state.homeTile, message);
		Expect(actual.npcAgents[index].state.awareness.playerVisible == expected.npcAgents[index].state.awareness.playerVisible, message);
		Expect(actual.npcAgents[index].state.awareness.alerted == expected.npcAgents[index].state.awareness.alerted, message);
		Expect(actual.npcAgents[index].state.awareness.alertTicksRemaining == expected.npcAgents[index].state.awareness.alertTicksRemaining, message);
		Expect(actual.npcAgents[index].state.awareness.lastSeenTile == expected.npcAgents[index].state.awareness.lastSeenTile, message);
		Expect(NearVec(actual.npcAgents[index].state.awareness.lastSeenPosition, expected.npcAgents[index].state.awareness.lastSeenPosition), message);
		Expect(actual.npcAgents[index].state.followState.waypointIndex == expected.npcAgents[index].state.followState.waypointIndex, message);
		Expect(actual.npcAgents[index].state.followState.completed == expected.npcAgents[index].state.followState.completed, message);
	}
}

void ExpectAuthoritativeSessionSame(
	const iggy::runtime::RuntimeSessionState &actual,
	const iggy::runtime::RuntimeSessionState &expected,
	const char *message)
{
	ExpectLevelSame(actual.level, expected.level, message);
	Expect(actual.tickIndex == expected.tickIndex, message);
	Expect(actual.hasPlayer == expected.hasPlayer, message);
	ExpectPlayerAgent(actual.player, expected.player, message);
}

void ExpectNoCaches(const iggy::runtime::RuntimeSessionState &session, const char *message)
{
	Expect(!session.hasRenderCache, message);
	Expect(!session.derivedCaches.hasRenderCache, message);
	Expect(!session.derivedCaches.hasCollisionCache, message);
}

void ExpectDerivedCaches(const iggy::runtime::RuntimeSessionState &session, const char *message)
{
	Expect(session.hasRenderCache, message);
	Expect(session.derivedCaches.hasRenderCache, message);
	Expect(session.derivedCaches.hasCollisionCache, message);
	Expect(session.derivedCaches.collision.world.objects().size() == 1, message);
	if (session.derivedCaches.collision.world.objects().size() == 1)
		ExpectBounds(session.derivedCaches.collision.world.objects()[0].shape.bounds, iggy::tileBounds({ 1, 0 }), message);
}

void ExpectRestoreConfigSame(
	const iggy::runtime::RuntimeSessionSnapshotRestoreConfig &actual,
	const iggy::runtime::RuntimeSessionSnapshotRestoreConfig &expected,
	const char *message)
{
	Expect(actual.buildConfig.buildRenderCache == expected.buildConfig.buildRenderCache, message);
	Expect(actual.buildConfig.buildDerivedCaches == expected.buildConfig.buildDerivedCaches, message);
	Expect(actual.buildConfig.hasPlayer == expected.buildConfig.hasPlayer, message);
	Expect(actual.buildConfig.derivedCacheConfig.buildRenderCache == expected.buildConfig.derivedCacheConfig.buildRenderCache, message);
	Expect(actual.buildConfig.derivedCacheConfig.buildCollisionCache == expected.buildConfig.derivedCacheConfig.buildCollisionCache, message);
}

void TestManualSlotSaveWritesExpectedPath()
{
	ResetTempRoot();
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual);
	const iggy::runtime::RuntimeSessionState session = Session(Level({ ".#", ".." }), {});

	const iggy::runtime::RuntimeSaveSlotSaveResult result = iggy::runtime::RuntimeSaveSlotStore {}.save(
		session,
		PathConfig(),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotStoreStatus::Saved, "manual slot save should succeed");
	Expect(result.path.path == TempRoot() / "manual" / "slot1.igsave", "manual slot save should use expected path");
	Expect(std::filesystem::exists(result.path.path), "manual slot save should write target file through session saver");
}

void TestManualSlotLoadReadsSavedSession()
{
	ResetTempRoot();
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual);
	const iggy::LevelRuntimeState level = Level({ ".#", ".." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	const iggy::runtime::RuntimeSaveSlotId slot = Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1");
	Expect(iggy::runtime::RuntimeSaveSlotStore {}.save(session, PathConfig(), slot).status == iggy::runtime::RuntimeSaveSlotStoreStatus::Saved, "manual slot load setup should save");

	const iggy::runtime::RuntimeSaveSlotLoadResult result = iggy::runtime::RuntimeSaveSlotStore {}.load(PathConfig(), slot, RestoreConfig(BuildConfigNoCaches()));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotStoreStatus::Loaded, "manual slot load should succeed");
	Expect(result.path.path == TempRoot() / "manual" / "slot1.igsave", "manual slot load should use expected path");
	ExpectAuthoritativeSessionSame(result.load.session, session, "manual slot load should restore saved authoritative session");
	ExpectNoCaches(result.load.session, "manual slot load without cache rebuild should leave caches absent");
}

void TestAutoAndQuickSlotsUseExpectedDirectories()
{
	ResetTempRoot();
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Auto);
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Quick);
	const iggy::runtime::RuntimeSessionState session = Session(Level({ "..", ".." }), {});

	const iggy::runtime::RuntimeSaveSlotSaveResult autosave = iggy::runtime::RuntimeSaveSlotStore {}.save(
		session,
		PathConfig(),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Auto, "auto_1"));
	const iggy::runtime::RuntimeSaveSlotSaveResult quick = iggy::runtime::RuntimeSaveSlotStore {}.save(
		session,
		PathConfig(),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Quick, "quick-1"));

	Expect(autosave.status == iggy::runtime::RuntimeSaveSlotStoreStatus::Saved, "auto slot save should succeed");
	Expect(autosave.path.path == TempRoot() / "auto" / "auto_1.igsave", "auto slot save should use auto directory");
	Expect(quick.status == iggy::runtime::RuntimeSaveSlotStoreStatus::Saved, "quick slot save should succeed");
	Expect(quick.path.path == TempRoot() / "quick" / "quick-1.igsave", "quick slot save should use quick directory");
}

void TestInvalidSlotNameDoesNotWrite()
{
	ResetTempRoot();
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual);
	const iggy::runtime::RuntimeSessionState session = Session(Level({ "..", ".." }), {});

	const iggy::runtime::RuntimeSaveSlotSaveResult result = iggy::runtime::RuntimeSaveSlotStore {}.save(
		session,
		PathConfig(),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "../bad"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotStoreStatus::InvalidSlotPath, "invalid slot name should return InvalidSlotPath");
	Expect(result.path.status == iggy::runtime::RuntimeSaveSlotPathStatus::InvalidSlotName, "invalid slot name should preserve path policy diagnostics");
	Expect(std::filesystem::is_empty(TempRoot() / "manual"), "invalid slot save should not create a file");
}

void TestEmptyBaseDirectoryInvalid()
{
	const iggy::runtime::RuntimeSessionState session = Session(Level({ "..", ".." }), {});

	const iggy::runtime::RuntimeSaveSlotSaveResult result = iggy::runtime::RuntimeSaveSlotStore {}.save(
		session,
		PathConfig({}),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotStoreStatus::InvalidSlotPath, "empty base directory should return InvalidSlotPath");
	Expect(result.path.status == iggy::runtime::RuntimeSaveSlotPathStatus::EmptyBaseDirectory, "empty base directory should preserve path diagnostics");
}

void TestMissingParentDirectorySaveFailsThroughSaver()
{
	ResetTempRoot();
	const iggy::runtime::RuntimeSessionState session = Session(Level({ "..", ".." }), {});

	const iggy::runtime::RuntimeSaveSlotSaveResult result = iggy::runtime::RuntimeSaveSlotStore {}.save(
		session,
		PathConfig(),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1"));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotStoreStatus::SaveFailed, "missing slot directory should fail through saver");
	Expect(result.path.status == iggy::runtime::RuntimeSaveSlotPathStatus::Valid, "missing slot directory should still be a valid slot path");
	Expect(result.save.status == iggy::runtime::RuntimeSessionSaveStatus::FileWriteFailed, "missing slot directory should preserve session save failure");
}

void TestLoadMissingSlotFileFailsThroughLoader()
{
	ResetTempRoot();
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual);

	const iggy::runtime::RuntimeSaveSlotLoadResult result = iggy::runtime::RuntimeSaveSlotStore {}.load(
		PathConfig(),
		Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "missing"),
		RestoreConfig(BuildConfigNoCaches()));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotStoreStatus::LoadFailed, "missing slot file should fail through loader");
	Expect(result.path.status == iggy::runtime::RuntimeSaveSlotPathStatus::Valid, "missing slot file should still have valid path");
	Expect(result.load.status == iggy::runtime::RuntimeSessionLoadStatus::FileReadFailed, "missing slot file should preserve session load failure");
}

void TestRestoreConfigRebuildsCachesOnLoad()
{
	ResetTempRoot();
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual);
	const iggy::LevelRuntimeState level = Level({ ".#", ".." });
	const iggy::runtime::RuntimeSessionState session = Session(level, {});
	const iggy::runtime::RuntimeSaveSlotId slot = Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "with_caches");
	Expect(iggy::runtime::RuntimeSaveSlotStore {}.save(session, PathConfig(), slot).status == iggy::runtime::RuntimeSaveSlotStoreStatus::Saved, "slot cache rebuild setup should save");

	const iggy::runtime::RuntimeSaveSlotLoadResult result = iggy::runtime::RuntimeSaveSlotStore {}.load(PathConfig(), slot, RestoreConfig(BuildConfigDerived()));

	Expect(result.status == iggy::runtime::RuntimeSaveSlotStoreStatus::Loaded, "slot load should use restore config");
	ExpectAuthoritativeSessionSame(result.load.session, session, "slot load with cache rebuild should restore authoritative session");
	ExpectDerivedCaches(result.load.session, "slot load should rebuild derived caches when restore config requests them");
}

void TestSaveExcludesDerivedCaches()
{
	ResetTempRoot();
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual);
	const iggy::LevelRuntimeState level = Level({ ".#", ".." });
	iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	session.derivedCaches.hasRenderCache = false;
	session.derivedCaches.collision.world = iggy::physics2d::CollisionWorld2D {};
	const iggy::runtime::RuntimeSaveSlotId slot = Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "cache_exclusion");

	const iggy::runtime::RuntimeSaveSlotSaveResult save = iggy::runtime::RuntimeSaveSlotStore {}.save(session, PathConfig(), slot);
	const iggy::runtime::RuntimeSaveSlotLoadResult loadWithoutCaches = iggy::runtime::RuntimeSaveSlotStore {}.load(PathConfig(), slot, RestoreConfig(BuildConfigNoCaches()));
	const iggy::runtime::RuntimeSaveSlotLoadResult loadWithCaches = iggy::runtime::RuntimeSaveSlotStore {}.load(PathConfig(), slot, RestoreConfig(BuildConfigDerived()));

	Expect(save.status == iggy::runtime::RuntimeSaveSlotStoreStatus::Saved, "slot cache exclusion setup should save");
	Expect(loadWithoutCaches.status == iggy::runtime::RuntimeSaveSlotStoreStatus::Loaded, "slot cache exclusion load without caches should succeed");
	ExpectNoCaches(loadWithoutCaches.load.session, "slot store save should not persist source caches");
	Expect(loadWithCaches.status == iggy::runtime::RuntimeSaveSlotStoreStatus::Loaded, "slot cache exclusion load with rebuild should succeed");
	ExpectDerivedCaches(loadWithCaches.load.session, "slot store load should rebuild caches from snapshot level");
}

void TestInputsAreNotMutated()
{
	ResetTempRoot();
	CreateSlotDirectory(iggy::runtime::RuntimeSaveSlotKind::Manual);
	iggy::runtime::RuntimeSessionState session = Session(Level({ ".#", ".." }), {});
	const iggy::runtime::RuntimeSessionState sessionBefore = session;
	iggy::runtime::RuntimeSaveSlotPathPolicyConfig pathConfig = PathConfig();
	const iggy::runtime::RuntimeSaveSlotPathPolicyConfig pathConfigBefore = pathConfig;
	iggy::runtime::RuntimeSaveSlotId slot = Slot(iggy::runtime::RuntimeSaveSlotKind::Manual, "slot1");
	const iggy::runtime::RuntimeSaveSlotId slotBefore = slot;
	iggy::runtime::RuntimeSessionSnapshotRestoreConfig restoreConfig = RestoreConfig(BuildConfigDerived());
	const iggy::runtime::RuntimeSessionSnapshotRestoreConfig restoreConfigBefore = restoreConfig;

	const iggy::runtime::RuntimeSaveSlotSaveResult save = iggy::runtime::RuntimeSaveSlotStore {}.save(session, pathConfig, slot);
	const iggy::runtime::RuntimeSaveSlotLoadResult load = iggy::runtime::RuntimeSaveSlotStore {}.load(pathConfig, slot, restoreConfig);

	Expect(save.status == iggy::runtime::RuntimeSaveSlotStoreStatus::Saved, "slot store immutability setup should save");
	Expect(load.status == iggy::runtime::RuntimeSaveSlotStoreStatus::Loaded, "slot store immutability setup should load");
	ExpectAuthoritativeSessionSame(session, sessionBefore, "slot store should not mutate source session authoritative state");
	Expect(pathConfig.baseDirectory == pathConfigBefore.baseDirectory, "slot store should not mutate path config base directory");
	Expect(pathConfig.extension == pathConfigBefore.extension, "slot store should not mutate path config extension");
	Expect(slot.kind == slotBefore.kind && slot.name == slotBefore.name, "slot store should not mutate slot id");
	ExpectRestoreConfigSame(restoreConfig, restoreConfigBefore, "slot store should not mutate restore config");
}

} // namespace

int main()
{
	TestManualSlotSaveWritesExpectedPath();
	TestManualSlotLoadReadsSavedSession();
	TestAutoAndQuickSlotsUseExpectedDirectories();
	TestInvalidSlotNameDoesNotWrite();
	TestEmptyBaseDirectoryInvalid();
	TestMissingParentDirectorySaveFailsThroughSaver();
	TestLoadMissingSlotFileFailsThroughLoader();
	TestRestoreConfigRebuildsCachesOnLoad();
	TestSaveExcludesDerivedCaches();
	TestInputsAreNotMutated();

	RemoveTempRoot();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
