#include <cstdlib>
#include <filesystem>
#include <string_view>
#include <vector>

#include "runtime/RuntimeSaveChunkArchiveCodec.hpp"
#include "runtime/RuntimeSaveFileEnvelope.hpp"
#include "runtime/RuntimeSaveFileIO.hpp"
#include "runtime/RuntimeSessionSaveLoad.hpp"
#include "runtime/RuntimeSessionSnapshotChunkCodec.hpp"
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
const iggy::runtime::RuntimeSaveChunkId LevelMapChunkId = iggy::runtime::makeRuntimeSaveChunkId('L', 'M', 'A', 'P');

std::filesystem::path TempRoot()
{
	return std::filesystem::current_path() / "runtime_session_save_load_tests_tmp";
}

std::filesystem::path TempPath(const char *name)
{
	return TempRoot() / name;
}

void ResetTempRoot()
{
	std::error_code ignored;
	std::filesystem::remove_all(TempRoot(), ignored);
	std::filesystem::create_directories(TempRoot(), ignored);
}

iggy::LevelRuntimeState Level(std::vector<std::string_view> rows)
{
	iggy::LevelRuntimeState level;
	level.map = MapFromRows(rows);
	level.map.id = iggy::ResourceId("level:save-load");
	level.map.playerStart = { 0, 0 };
	level.map.entitySpawns.push_back({ iggy::ResourceId("npc:type"), 1, 1, iggy::ResourceId("spawn:one") });
	iggy::npc_ai::NpcAgentEntry npc;
	npc.id = iggy::ResourceId("npc:one");
	npc.state.position = { 1.25F, 1.25F };
	npc.state.homeTile = { 1, 1 };
	npc.state.awareness.playerVisible = true;
	npc.state.awareness.alerted = true;
	npc.state.awareness.alertTicksRemaining = 4;
	npc.state.awareness.lastSeenTile = { 0, 1 };
	npc.state.awareness.lastSeenPosition = { 0.25F, 1.25F };
	npc.state.followState.waypointIndex = 2;
	npc.state.followState.completed = true;
	level.npcAgents.push_back(npc);
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

iggy::LevelDerivedCacheState BuildCaches(const iggy::LevelRuntimeState &level)
{
	const iggy::LevelDerivedCacheBuildResult build = iggy::LevelDerivedCacheBuilder {}.build(level, DerivedConfig(true, true));
	Expect(build.built, "save/load fixture should build derived caches");
	return build.state;
}

iggy::runtime::RuntimeSessionState Session(const iggy::LevelRuntimeState &level, const iggy::LevelDerivedCacheState &caches)
{
	iggy::runtime::RuntimeSessionState session;
	session.level = level;
	session.tickIndex = 37;
	session.derivedCaches = caches;
	session.hasRenderCache = caches.hasRenderCache;
	if (caches.hasRenderCache)
		session.renderCache = caches.render;
	session.hasPlayer = true;
	session.player = PlayerAgent(
		iggy::ResourceId("player:save-load"),
		{ 0.5F, 0.5F },
		{ 0, 0 },
		iggy::PlayerMovementStatus::Moving,
		iggy::PlayerFacing2D::East);
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

void WriteBytes(const std::filesystem::path &path, const std::vector<std::uint8_t> &bytes)
{
	const iggy::runtime::RuntimeSaveFileWriteResult write = iggy::runtime::RuntimeSaveFileWriter {}.writeBytes(path, bytes);
	Expect(write.status == iggy::runtime::RuntimeSaveFileIOStatus::Ok, "save/load fixture should write test bytes");
}

void WriteArchivePayload(const std::filesystem::path &path, const std::vector<std::uint8_t> &archivePayload)
{
	const iggy::runtime::RuntimeSaveFileEnvelopeEncodeResult envelope = iggy::runtime::RuntimeSaveFileEnvelopeEncoder {}.encodePayload(archivePayload);
	Expect(envelope.encoded, "save/load fixture should encode envelope");
	WriteBytes(path, envelope.bytes);
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
		const iggy::npc_ai::NpcAgentEntry &actualNpc = actual.npcAgents[index];
		const iggy::npc_ai::NpcAgentEntry &expectedNpc = expected.npcAgents[index];
		Expect(actualNpc.id == expectedNpc.id, message);
		Expect(NearVec(actualNpc.state.position, expectedNpc.state.position), message);
		Expect(actualNpc.state.homeTile == expectedNpc.state.homeTile, message);
		Expect(actualNpc.state.awareness.playerVisible == expectedNpc.state.awareness.playerVisible, message);
		Expect(actualNpc.state.awareness.alerted == expectedNpc.state.awareness.alerted, message);
		Expect(actualNpc.state.awareness.alertTicksRemaining == expectedNpc.state.awareness.alertTicksRemaining, message);
		Expect(actualNpc.state.awareness.lastSeenTile == expectedNpc.state.awareness.lastSeenTile, message);
		Expect(NearVec(actualNpc.state.awareness.lastSeenPosition, expectedNpc.state.awareness.lastSeenPosition), message);
		Expect(actualNpc.state.followState.waypointIndex == expectedNpc.state.followState.waypointIndex, message);
		Expect(actualNpc.state.followState.completed == expectedNpc.state.followState.completed, message);
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
	Expect(session.renderCache.tileChunks.chunks.size() == session.derivedCaches.render.tileChunks.chunks.size(), message);
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

iggy::runtime::RuntimeSaveChunkArchive ArchiveForSnapshot(const iggy::runtime::RuntimeSessionState &session)
{
	const iggy::runtime::RuntimeSessionSnapshot snapshot = iggy::runtime::RuntimeSessionSnapshotBuilder {}.capture(session);
	return iggy::runtime::RuntimeSessionSnapshotChunkEncoder {}.encode(snapshot);
}

void TestSaveThenLoadWithoutCacheRebuildRestoresAuthoritativeState()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("no_caches.iggy");
	const iggy::LevelRuntimeState level = Level({ ".#", ".." });
	const iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));

	const iggy::runtime::RuntimeSessionSaveResult save = iggy::runtime::RuntimeSessionSaver {}.save(session, path);
	const iggy::runtime::RuntimeSessionLoadResult load = iggy::runtime::RuntimeSessionLoader {}.load(path, RestoreConfig(BuildConfigNoCaches()));

	Expect(save.status == iggy::runtime::RuntimeSessionSaveStatus::Saved, "save without cache rebuild setup should save");
	Expect(load.status == iggy::runtime::RuntimeSessionLoadStatus::Loaded, "load without cache rebuild should succeed");
	ExpectAuthoritativeSessionSame(load.session, session, "load without cache rebuild should restore authoritative state");
	ExpectNoCaches(load.session, "load without cache rebuild should leave caches absent");
}

void TestSaveThenLoadWithDerivedCacheRebuild()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("derived_caches.iggy");
	const iggy::LevelRuntimeState level = Level({ ".#", ".." });
	const iggy::runtime::RuntimeSessionState session = Session(level, {});

	const iggy::runtime::RuntimeSessionSaveResult save = iggy::runtime::RuntimeSessionSaver {}.save(session, path);
	const iggy::runtime::RuntimeSessionLoadResult load = iggy::runtime::RuntimeSessionLoader {}.load(path, RestoreConfig(BuildConfigDerived(true, true)));

	Expect(save.status == iggy::runtime::RuntimeSessionSaveStatus::Saved, "save with cache rebuild setup should save");
	Expect(load.status == iggy::runtime::RuntimeSessionLoadStatus::Loaded, "load with derived cache rebuild should succeed");
	ExpectAuthoritativeSessionSame(load.session, session, "load with derived cache rebuild should restore authoritative state");
	ExpectDerivedCaches(load.session, "load with derived cache rebuild should rebuild caches");
}

void TestSaveExcludesDerivedCaches()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("cache_exclusion.iggy");
	const iggy::LevelRuntimeState level = Level({ ".#", ".." });
	iggy::runtime::RuntimeSessionState session = Session(level, BuildCaches(level));
	session.renderCache.tileChunks.chunks.clear();
	session.derivedCaches.hasRenderCache = false;
	session.derivedCaches.collision.world = iggy::physics2d::CollisionWorld2D {};

	const iggy::runtime::RuntimeSessionSaveResult save = iggy::runtime::RuntimeSessionSaver {}.save(session, path);
	const iggy::runtime::RuntimeSessionLoadResult loadWithoutCaches = iggy::runtime::RuntimeSessionLoader {}.load(path, RestoreConfig(BuildConfigNoCaches()));
	const iggy::runtime::RuntimeSessionLoadResult loadWithCaches = iggy::runtime::RuntimeSessionLoader {}.load(path, RestoreConfig(BuildConfigDerived(true, true)));

	Expect(save.status == iggy::runtime::RuntimeSessionSaveStatus::Saved, "cache exclusion setup should save");
	Expect(loadWithoutCaches.status == iggy::runtime::RuntimeSessionLoadStatus::Loaded, "cache exclusion load without caches should succeed");
	ExpectNoCaches(loadWithoutCaches.session, "save should not persist source derived caches");
	Expect(loadWithCaches.status == iggy::runtime::RuntimeSessionLoadStatus::Loaded, "cache exclusion load with rebuild should succeed");
	ExpectDerivedCaches(loadWithCaches.session, "save should allow caches to rebuild from snapshot level");
}

void TestSaveInvalidSnapshotDoesNotReplaceFile()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("invalid_snapshot.iggy");
	const std::vector<std::uint8_t> sentinel { 7, 7, 7 };
	WriteBytes(path, sentinel);
	iggy::runtime::RuntimeSessionState session = Session(Level({ ".." }), {});
	session.level.map.width = 0;
	session.level.map.height = 0;

	const iggy::runtime::RuntimeSessionSaveResult save = iggy::runtime::RuntimeSessionSaver {}.save(session, path);
	const iggy::runtime::RuntimeSaveFileReadResult read = iggy::runtime::RuntimeSaveFileReader {}.readBytes(path);

	Expect(save.status == iggy::runtime::RuntimeSessionSaveStatus::SnapshotInvalid, "invalid snapshot save should fail before writing");
	Expect(!save.validation.valid, "invalid snapshot save should preserve validation diagnostics");
	Expect(read.status == iggy::runtime::RuntimeSaveFileIOStatus::Ok, "invalid snapshot setup should still read sentinel file");
	Expect(read.bytes == sentinel, "invalid snapshot save should not replace existing file");
}

void TestFileWriteFailure()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("missing_parent") / "save.iggy";
	const iggy::runtime::RuntimeSessionState session = Session(Level({ "..", ".." }), {});

	const iggy::runtime::RuntimeSessionSaveResult save = iggy::runtime::RuntimeSessionSaver {}.save(session, path);

	Expect(save.status == iggy::runtime::RuntimeSessionSaveStatus::FileWriteFailed, "save to missing parent should report file write failure");
	Expect(save.fileWrite.status == iggy::runtime::RuntimeSaveFileIOStatus::OpenFailed, "save should preserve file write diagnostics");
}

void TestLoadMissingFile()
{
	ResetTempRoot();

	const iggy::runtime::RuntimeSessionLoadResult load = iggy::runtime::RuntimeSessionLoader {}.load(TempPath("missing.iggy"), RestoreConfig(BuildConfigNoCaches()));

	Expect(load.status == iggy::runtime::RuntimeSessionLoadStatus::FileReadFailed, "loading missing file should report file read failure");
	Expect(load.fileRead.status == iggy::runtime::RuntimeSaveFileIOStatus::OpenFailed, "load missing file should preserve read diagnostics");
}

void TestLoadCorruptEnvelope()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("corrupt_envelope.iggy");
	iggy::runtime::RuntimeSaveFileEnvelopeEncodeResult envelope = iggy::runtime::RuntimeSaveFileEnvelopeEncoder {}.encodePayload({ 1, 2, 3 });
	Expect(envelope.encoded, "corrupt envelope setup should encode envelope");
	envelope.bytes.back() ^= 0xFF;
	WriteBytes(path, envelope.bytes);

	const iggy::runtime::RuntimeSessionLoadResult load = iggy::runtime::RuntimeSessionLoader {}.load(path, RestoreConfig(BuildConfigNoCaches()));

	Expect(load.status == iggy::runtime::RuntimeSessionLoadStatus::EnvelopeDecodeFailed, "loading corrupt envelope should fail at envelope decode");
	Expect(!load.envelopeDecode.issues.empty(), "corrupt envelope load should preserve envelope diagnostics");
}

void TestLoadInvalidArchiveBytes()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("invalid_archive.iggy");
	WriteArchivePayload(path, { 'I', 'G' });

	const iggy::runtime::RuntimeSessionLoadResult load = iggy::runtime::RuntimeSessionLoader {}.load(path, RestoreConfig(BuildConfigNoCaches()));

	Expect(load.status == iggy::runtime::RuntimeSessionLoadStatus::ArchiveDecodeFailed, "loading truncated archive payload should fail at archive decode");
	Expect(!load.archiveDecode.issues.empty(), "truncated archive load should preserve archive decode diagnostics");
}

void TestLoadMalformedSnapshotChunk()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("malformed_snapshot.iggy");
	iggy::runtime::RuntimeSaveChunkArchive archive = ArchiveForSnapshot(Session(Level({ ".#", ".." }), {}));
	const std::vector<std::size_t> indexes = iggy::runtime::findChunkIndexes(archive, LevelMapChunkId);
	Expect(indexes.size() == 1, "malformed snapshot setup should find LMAP chunk");
	if (!indexes.empty() && !archive.chunks[indexes.front()].payload.empty())
		archive.chunks[indexes.front()].payload.pop_back();
	const iggy::runtime::RuntimeSaveChunkArchiveEncodeResult archiveBytes = iggy::runtime::RuntimeSaveChunkArchiveEncoder {}.encode(archive);
	Expect(archiveBytes.encoded, "malformed snapshot setup should encode archive envelope");
	WriteArchivePayload(path, archiveBytes.bytes);

	const iggy::runtime::RuntimeSessionLoadResult load = iggy::runtime::RuntimeSessionLoader {}.load(path, RestoreConfig(BuildConfigNoCaches()));

	Expect(load.status == iggy::runtime::RuntimeSessionLoadStatus::SnapshotDecodeFailed, "loading malformed snapshot chunk should fail at snapshot decode");
	Expect(!load.snapshotDecode.issues.empty(), "malformed snapshot load should preserve snapshot decode diagnostics");
}

void TestRestoreFailure()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("restore_failure.iggy");
	const iggy::runtime::RuntimeSessionState session = Session(Level({ "..", ".." }), {});
	const iggy::runtime::RuntimeSessionSnapshotRestoreConfig config = RestoreConfig(BuildConfigDerived(true, false, RenderConfig(0, 1, 8)));

	const iggy::runtime::RuntimeSessionSaveResult save = iggy::runtime::RuntimeSessionSaver {}.save(session, path);
	const iggy::runtime::RuntimeSessionLoadResult load = iggy::runtime::RuntimeSessionLoader {}.load(path, config);

	Expect(save.status == iggy::runtime::RuntimeSessionSaveStatus::Saved, "restore failure setup should save");
	Expect(load.status == iggy::runtime::RuntimeSessionLoadStatus::RestoreFailed, "load with invalid restore cache config should report restore failure");
	Expect(!load.restore.restored, "restore failure load should preserve restore diagnostics");
	Expect(!load.restore.build.built, "restore failure load should preserve failed builder result");
	Expect(load.session.tickIndex == 0, "restore failure should not publish partial loaded session");
}

void TestSaverAndLoaderDoNotMutateInputs()
{
	ResetTempRoot();
	const std::filesystem::path path = TempPath("immutability.iggy");
	iggy::runtime::RuntimeSessionState session = Session(Level({ ".#", ".." }), BuildCaches(Level({ ".#", ".." })));
	const iggy::runtime::RuntimeSessionState sessionBefore = session;
	iggy::runtime::RuntimeSessionSnapshotRestoreConfig config = RestoreConfig(BuildConfigDerived(true, true));
	const iggy::runtime::RuntimeSessionSnapshotRestoreConfig configBefore = config;

	const iggy::runtime::RuntimeSessionSaveResult save = iggy::runtime::RuntimeSessionSaver {}.save(session, path);
	const iggy::runtime::RuntimeSessionLoadResult load = iggy::runtime::RuntimeSessionLoader {}.load(path, config);

	Expect(save.status == iggy::runtime::RuntimeSessionSaveStatus::Saved, "save/load immutability setup should save");
	Expect(load.status == iggy::runtime::RuntimeSessionLoadStatus::Loaded, "save/load immutability setup should load");
	ExpectAuthoritativeSessionSame(session, sessionBefore, "saver should not mutate source session authoritative state");
	Expect(session.hasRenderCache == sessionBefore.hasRenderCache, "saver should not mutate source session render mirror");
	Expect(session.derivedCaches.hasRenderCache == sessionBefore.derivedCaches.hasRenderCache, "saver should not mutate source session derived render flag");
	Expect(session.derivedCaches.hasCollisionCache == sessionBefore.derivedCaches.hasCollisionCache, "saver should not mutate source session derived collision flag");
	ExpectRestoreConfigSame(config, configBefore, "loader should not mutate restore config");
}

} // namespace

int main()
{
	TestSaveThenLoadWithoutCacheRebuildRestoresAuthoritativeState();
	TestSaveThenLoadWithDerivedCacheRebuild();
	TestSaveExcludesDerivedCaches();
	TestSaveInvalidSnapshotDoesNotReplaceFile();
	TestFileWriteFailure();
	TestLoadMissingFile();
	TestLoadCorruptEnvelope();
	TestLoadInvalidArchiveBytes();
	TestLoadMalformedSnapshotChunk();
	TestRestoreFailure();
	TestSaverAndLoaderDoNotMutateInputs();

	std::error_code ignored;
	std::filesystem::remove_all(TempRoot(), ignored);

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
