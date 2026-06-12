#include <cstdlib>
#include <string_view>
#include <vector>

#include "runtime/RuntimeBinaryCodec.hpp"
#include "runtime/RuntimeSessionSnapshotChunkCodec.hpp"
#include "support/LevelMapFixtures.hpp"
#include "support/PlayerFixtures.hpp"
#include "support/TestHarness.hpp"

namespace {

using iggy::test::Expect;
using iggy::test::Failures;
using iggy::test::MapFromRows;
using iggy::test::NearVec;
using iggy::test::PlayerAgent;

const iggy::runtime::RuntimeSaveChunkId SessionChunkId = iggy::runtime::makeRuntimeSaveChunkId('S', 'E', 'S', 'S');
const iggy::runtime::RuntimeSaveChunkId LevelMapChunkId = iggy::runtime::makeRuntimeSaveChunkId('L', 'M', 'A', 'P');
const iggy::runtime::RuntimeSaveChunkId PlayerChunkId = iggy::runtime::makeRuntimeSaveChunkId('P', 'L', 'Y', 'R');
const iggy::runtime::RuntimeSaveChunkId NpcsChunkId = iggy::runtime::makeRuntimeSaveChunkId('N', 'P', 'C', 'S');
const iggy::runtime::RuntimeSaveChunkId UnknownChunkId = iggy::runtime::makeRuntimeSaveChunkId('U', 'N', 'K', 'N');

iggy::LevelRuntimeState Level(std::vector<std::string_view> rows)
{
	iggy::LevelRuntimeState level;
	level.map = MapFromRows(rows);
	level.map.id = iggy::ResourceId("level:chunk");
	level.map.playerStart = { 1, 0 };
	level.map.entitySpawns.push_back({ iggy::ResourceId("enemy:slime"), 1, 1, iggy::ResourceId("spawn:slime") });
	return level;
}

iggy::npc_ai::NpcAgentEntry Npc()
{
	iggy::npc_ai::NpcAgentEntry npc;
	npc.id = iggy::ResourceId("npc:one");
	npc.state.position = { 0.5F, 1.5F };
	npc.state.homeTile = { 0, 1 };
	npc.state.awareness.playerVisible = true;
	npc.state.awareness.alerted = true;
	npc.state.awareness.alertTicksRemaining = 7;
	npc.state.awareness.lastSeenTile = { 1, 1 };
	npc.state.awareness.lastSeenPosition = { 1.25F, 1.75F };
	npc.state.followState.waypointIndex = 3;
	npc.state.followState.completed = true;
	return npc;
}

iggy::runtime::RuntimeSessionSnapshot Snapshot(bool hasPlayer)
{
	iggy::runtime::RuntimeSessionSnapshot snapshot;
	snapshot.level = Level({
		".#",
		"..",
	});
	snapshot.level.npcAgents.push_back(Npc());
	snapshot.tickIndex = 42;
	snapshot.hasPlayer = hasPlayer;
	if (hasPlayer)
		snapshot.player = PlayerAgent(iggy::ResourceId("player:one"), { 0.5F, 0.5F }, { 0, 0 }, iggy::PlayerMovementStatus::Moving, iggy::PlayerFacing2D::West);
	return snapshot;
}

bool HasIssue(
	const iggy::runtime::RuntimeSessionSnapshotDecodeResult &result,
	iggy::runtime::RuntimeSessionSnapshotChunkIssueCode code,
	iggy::runtime::RuntimeSaveChunkId chunkId = {},
	std::size_t chunkIndex = 0)
{
	for (const iggy::runtime::RuntimeSessionSnapshotChunkIssue &issue : result.issues) {
		if (issue.code == code && issue.chunkId == chunkId && issue.chunkIndex == chunkIndex)
			return true;
	}
	return false;
}

void ExpectChunkIds(const iggy::runtime::RuntimeSaveChunkArchive &archive, const std::vector<iggy::runtime::RuntimeSaveChunkId> &ids, const char *message)
{
	Expect(archive.chunks.size() == ids.size(), message);
	if (archive.chunks.size() != ids.size())
		return;
	for (std::size_t index = 0; index < ids.size(); ++index)
		Expect(archive.chunks[index].id == ids[index], message);
}

void ExpectMapSame(const iggy::LevelTileMap &actual, const iggy::LevelTileMap &expected, const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(actual.width == expected.width, message);
	Expect(actual.height == expected.height, message);
	Expect(actual.playerStart.x == expected.playerStart.x && actual.playerStart.y == expected.playerStart.y, message);
	Expect(actual.tiles.size() == expected.tiles.size(), message);
	for (std::size_t index = 0; index < actual.tiles.size() && index < expected.tiles.size(); ++index)
		Expect(actual.tiles[index].walkable == expected.tiles[index].walkable, message);
	Expect(actual.entitySpawns.size() == expected.entitySpawns.size(), message);
	for (std::size_t index = 0; index < actual.entitySpawns.size() && index < expected.entitySpawns.size(); ++index) {
		Expect(actual.entitySpawns[index].type == expected.entitySpawns[index].type, message);
		Expect(actual.entitySpawns[index].x == expected.entitySpawns[index].x, message);
		Expect(actual.entitySpawns[index].y == expected.entitySpawns[index].y, message);
		Expect(actual.entitySpawns[index].id == expected.entitySpawns[index].id, message);
	}
}

void ExpectPlayerSame(const iggy::PlayerAgentState &actual, const iggy::PlayerAgentState &expected, const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(NearVec(actual.position, expected.position), message);
	Expect(actual.spawnTile == expected.spawnTile, message);
	Expect(actual.movementStatus == expected.movementStatus, message);
	Expect(actual.facing == expected.facing, message);
}

void ExpectNpcSame(const iggy::npc_ai::NpcAgentEntry &actual, const iggy::npc_ai::NpcAgentEntry &expected, const char *message)
{
	Expect(actual.id == expected.id, message);
	Expect(NearVec(actual.state.position, expected.state.position), message);
	Expect(actual.state.homeTile == expected.state.homeTile, message);
	Expect(actual.state.awareness.playerVisible == expected.state.awareness.playerVisible, message);
	Expect(actual.state.awareness.alerted == expected.state.awareness.alerted, message);
	Expect(actual.state.awareness.alertTicksRemaining == expected.state.awareness.alertTicksRemaining, message);
	Expect(actual.state.awareness.lastSeenTile == expected.state.awareness.lastSeenTile, message);
	Expect(NearVec(actual.state.awareness.lastSeenPosition, expected.state.awareness.lastSeenPosition), message);
	Expect(actual.state.followState.waypointIndex == expected.state.followState.waypointIndex, message);
	Expect(actual.state.followState.completed == expected.state.followState.completed, message);
}

void ExpectSnapshotSame(const iggy::runtime::RuntimeSessionSnapshot &actual, const iggy::runtime::RuntimeSessionSnapshot &expected, const char *message)
{
	Expect(actual.tickIndex == expected.tickIndex, message);
	Expect(actual.hasPlayer == expected.hasPlayer, message);
	ExpectMapSame(actual.level.map, expected.level.map, message);
	Expect(actual.level.npcAgents.size() == expected.level.npcAgents.size(), message);
	if (actual.hasPlayer && expected.hasPlayer)
		ExpectPlayerSame(actual.player, expected.player, message);
	for (std::size_t index = 0; index < actual.level.npcAgents.size() && index < expected.level.npcAgents.size(); ++index)
		ExpectNpcSame(actual.level.npcAgents[index], expected.level.npcAgents[index], message);
}

void RemoveChunk(iggy::runtime::RuntimeSaveChunkArchive &archive, iggy::runtime::RuntimeSaveChunkId id)
{
	for (auto it = archive.chunks.begin(); it != archive.chunks.end(); ++it) {
		if (it->id == id) {
			archive.chunks.erase(it);
			return;
		}
	}
}

iggy::runtime::RuntimeSaveChunk *FindChunk(iggy::runtime::RuntimeSaveChunkArchive &archive, iggy::runtime::RuntimeSaveChunkId id)
{
	for (iggy::runtime::RuntimeSaveChunk &chunk : archive.chunks) {
		if (chunk.id == id)
			return &chunk;
	}
	return nullptr;
}

std::vector<std::uint8_t> InvalidPlayerEnumPayload()
{
	iggy::runtime::RuntimeBinaryWriter writer;
	writer.writeU32LE(10);
	writer.writeBytes({ 'p', 'l', 'a', 'y', 'e', 'r', ':', 'b', 'a', 'd' });
	writer.writeF32LE(0.5F);
	writer.writeF32LE(0.5F);
	writer.writeI32LE(0);
	writer.writeI32LE(0);
	writer.writeU32LE(99);
	writer.writeU32LE(0);
	return writer.takeBytes();
}

void TestEncodingWithoutPlayerEmitsRequiredChunks()
{
	const iggy::runtime::RuntimeSaveChunkArchive archive = iggy::runtime::RuntimeSessionSnapshotChunkEncoder {}.encode(Snapshot(false));

	ExpectChunkIds(archive, { SessionChunkId, LevelMapChunkId, NpcsChunkId }, "snapshot without player should emit SESS, LMAP, NPCS");
}

void TestEncodingWithPlayerEmitsPlayerChunkInOrder()
{
	const iggy::runtime::RuntimeSaveChunkArchive archive = iggy::runtime::RuntimeSessionSnapshotChunkEncoder {}.encode(Snapshot(true));

	ExpectChunkIds(archive, { SessionChunkId, LevelMapChunkId, PlayerChunkId, NpcsChunkId }, "snapshot with player should emit SESS, LMAP, PLYR, NPCS");
}

void TestRoundTripPreservesAuthoritativeSnapshot()
{
	const iggy::runtime::RuntimeSessionSnapshot snapshot = Snapshot(true);
	const iggy::runtime::RuntimeSaveChunkArchive archive = iggy::runtime::RuntimeSessionSnapshotChunkEncoder {}.encode(snapshot);

	const iggy::runtime::RuntimeSessionSnapshotDecodeResult result = iggy::runtime::RuntimeSessionSnapshotChunkDecoder {}.decode(archive);

	Expect(result.decoded, "snapshot chunk round trip should decode");
	Expect(result.issues.empty(), "snapshot chunk round trip should have no issues");
	Expect(result.validation.valid, "snapshot chunk round trip should preserve valid snapshot");
	ExpectSnapshotSame(result.snapshot, snapshot, "snapshot chunk round trip should preserve authoritative fields");
}

void TestEncodedArchiveContainsNoCacheChunks()
{
	const iggy::runtime::RuntimeSaveChunkArchive archive = iggy::runtime::RuntimeSessionSnapshotChunkEncoder {}.encode(Snapshot(true));

	for (const iggy::runtime::RuntimeSaveChunk &chunk : archive.chunks) {
		Expect(chunk.id != iggy::runtime::makeRuntimeSaveChunkId('R', 'N', 'D', 'R'), "snapshot archive should not contain render cache chunk");
		Expect(chunk.id != iggy::runtime::makeRuntimeSaveChunkId('C', 'O', 'L', 'L'), "snapshot archive should not contain collision cache chunk");
	}
}

void TestMissingRequiredChunksFailDecode()
{
	iggy::runtime::RuntimeSaveChunkArchive archive = iggy::runtime::RuntimeSessionSnapshotChunkEncoder {}.encode(Snapshot(false));
	RemoveChunk(archive, SessionChunkId);
	RemoveChunk(archive, NpcsChunkId);

	const iggy::runtime::RuntimeSessionSnapshotDecodeResult result = iggy::runtime::RuntimeSessionSnapshotChunkDecoder {}.decode(archive);

	Expect(!result.decoded, "missing required chunks should fail decode");
	Expect(HasIssue(result, iggy::runtime::RuntimeSessionSnapshotChunkIssueCode::MissingRequiredChunk, SessionChunkId), "missing SESS should report issue");
	Expect(HasIssue(result, iggy::runtime::RuntimeSessionSnapshotChunkIssueCode::MissingRequiredChunk, NpcsChunkId), "missing NPCS should report issue");
}

void TestDuplicateSingletonChunkFailsDecode()
{
	iggy::runtime::RuntimeSaveChunkArchive archive = iggy::runtime::RuntimeSessionSnapshotChunkEncoder {}.encode(Snapshot(false));
	archive.chunks.push_back(archive.chunks[0]);

	const iggy::runtime::RuntimeSessionSnapshotDecodeResult result = iggy::runtime::RuntimeSessionSnapshotChunkDecoder {}.decode(archive);

	Expect(!result.decoded, "duplicate singleton chunk should fail decode");
	Expect(HasIssue(result, iggy::runtime::RuntimeSessionSnapshotChunkIssueCode::DuplicateSingletonChunk, SessionChunkId, archive.chunks.size() - 1), "duplicate SESS should report duplicate index");
}

void TestUnsupportedChunkVersionFailsDecode()
{
	iggy::runtime::RuntimeSaveChunkArchive archive = iggy::runtime::RuntimeSessionSnapshotChunkEncoder {}.encode(Snapshot(false));
	archive.chunks[1].version = 2;

	const iggy::runtime::RuntimeSessionSnapshotDecodeResult result = iggy::runtime::RuntimeSessionSnapshotChunkDecoder {}.decode(archive);

	Expect(!result.decoded, "unsupported chunk version should fail decode");
	Expect(HasIssue(result, iggy::runtime::RuntimeSessionSnapshotChunkIssueCode::UnsupportedChunkVersion, LevelMapChunkId, 1), "unsupported LMAP version should report issue");
}

void TestMalformedPayloadFailsDecode()
{
	iggy::runtime::RuntimeSaveChunkArchive archive = iggy::runtime::RuntimeSessionSnapshotChunkEncoder {}.encode(Snapshot(false));
	iggy::runtime::RuntimeSaveChunk *levelMap = FindChunk(archive, LevelMapChunkId);
	if (levelMap != nullptr && !levelMap->payload.empty())
		levelMap->payload.pop_back();

	const iggy::runtime::RuntimeSessionSnapshotDecodeResult result = iggy::runtime::RuntimeSessionSnapshotChunkDecoder {}.decode(archive);

	Expect(!result.decoded, "truncated payload should fail decode");
	Expect(HasIssue(result, iggy::runtime::RuntimeSessionSnapshotChunkIssueCode::MalformedChunkPayload, LevelMapChunkId, 1), "truncated LMAP should report malformed issue");
}

void TestInvalidPlayerEnumFailsDecode()
{
	iggy::runtime::RuntimeSaveChunkArchive archive = iggy::runtime::RuntimeSessionSnapshotChunkEncoder {}.encode(Snapshot(true));
	iggy::runtime::RuntimeSaveChunk *player = FindChunk(archive, PlayerChunkId);
	if (player != nullptr)
		player->payload = InvalidPlayerEnumPayload();

	const iggy::runtime::RuntimeSessionSnapshotDecodeResult result = iggy::runtime::RuntimeSessionSnapshotChunkDecoder {}.decode(archive);

	Expect(!result.decoded, "invalid player enum should fail decode");
	Expect(HasIssue(result, iggy::runtime::RuntimeSessionSnapshotChunkIssueCode::InvalidEnumValue, PlayerChunkId, 2), "invalid player enum should report issue");
}

void TestUnknownChunkIgnored()
{
	iggy::runtime::RuntimeSaveChunkArchive archive = iggy::runtime::RuntimeSessionSnapshotChunkEncoder {}.encode(Snapshot(false));
	archive.chunks.insert(archive.chunks.begin() + 1, { UnknownChunkId, 99, { 1, 2, 3 } });

	const iggy::runtime::RuntimeSessionSnapshotDecodeResult result = iggy::runtime::RuntimeSessionSnapshotChunkDecoder {}.decode(archive);

	Expect(result.decoded, "unknown chunk should be ignored");
	Expect(result.issues.empty(), "unknown chunk should not create issues");
	ExpectSnapshotSame(result.snapshot, Snapshot(false), "unknown chunk should not affect decoded snapshot");
}

void TestHasPlayerTrueWithoutPlayerChunkFailsDecode()
{
	iggy::runtime::RuntimeSaveChunkArchive archive = iggy::runtime::RuntimeSessionSnapshotChunkEncoder {}.encode(Snapshot(true));
	RemoveChunk(archive, PlayerChunkId);

	const iggy::runtime::RuntimeSessionSnapshotDecodeResult result = iggy::runtime::RuntimeSessionSnapshotChunkDecoder {}.decode(archive);

	Expect(!result.decoded, "hasPlayer true without PLYR should fail decode");
	Expect(HasIssue(result, iggy::runtime::RuntimeSessionSnapshotChunkIssueCode::MissingRequiredChunk, PlayerChunkId), "missing PLYR should report issue");
}

void TestHasPlayerFalseWithoutPlayerChunkDecodes()
{
	const iggy::runtime::RuntimeSaveChunkArchive archive = iggy::runtime::RuntimeSessionSnapshotChunkEncoder {}.encode(Snapshot(false));

	const iggy::runtime::RuntimeSessionSnapshotDecodeResult result = iggy::runtime::RuntimeSessionSnapshotChunkDecoder {}.decode(archive);

	Expect(result.decoded, "hasPlayer false without PLYR should decode");
	Expect(!result.snapshot.hasPlayer, "decoded snapshot should preserve hasPlayer false");
}

void TestHasPlayerFalseWithPlayerChunkIsIgnored()
{
	iggy::runtime::RuntimeSaveChunkArchive archive = iggy::runtime::RuntimeSessionSnapshotChunkEncoder {}.encode(Snapshot(false));
	const iggy::runtime::RuntimeSaveChunkArchive withPlayer = iggy::runtime::RuntimeSessionSnapshotChunkEncoder {}.encode(Snapshot(true));
	const iggy::runtime::RuntimeSaveChunk *player = iggy::runtime::findFirstChunk(withPlayer, PlayerChunkId);
	if (player != nullptr)
		archive.chunks.push_back(*player);

	const iggy::runtime::RuntimeSessionSnapshotDecodeResult result = iggy::runtime::RuntimeSessionSnapshotChunkDecoder {}.decode(archive);

	Expect(result.decoded, "extra PLYR with hasPlayer false should be ignored");
	Expect(!result.snapshot.hasPlayer, "extra PLYR should not flip hasPlayer");
}

void TestInvalidArchiveFailsBeforePayloadDecode()
{
	iggy::runtime::RuntimeSaveChunkArchive archive = iggy::runtime::RuntimeSessionSnapshotChunkEncoder {}.encode(Snapshot(false));
	archive.magic = { 'B', 'A', 'D', '!' };

	const iggy::runtime::RuntimeSessionSnapshotDecodeResult result = iggy::runtime::RuntimeSessionSnapshotChunkDecoder {}.decode(archive);

	Expect(!result.decoded, "invalid archive should fail decode");
	Expect(HasIssue(result, iggy::runtime::RuntimeSessionSnapshotChunkIssueCode::InvalidArchive), "invalid archive should report issue");
}

void TestSnapshotValidationFailureAfterDecodeFailsDecode()
{
	iggy::runtime::RuntimeSessionSnapshot snapshot = Snapshot(false);
	snapshot.level.map.width = 0;
	snapshot.level.map.height = 0;
	const iggy::runtime::RuntimeSaveChunkArchive archive = iggy::runtime::RuntimeSessionSnapshotChunkEncoder {}.encode(snapshot);

	const iggy::runtime::RuntimeSessionSnapshotDecodeResult result = iggy::runtime::RuntimeSessionSnapshotChunkDecoder {}.decode(archive);

	Expect(!result.decoded, "decoded invalid snapshot should fail validation");
	Expect(HasIssue(result, iggy::runtime::RuntimeSessionSnapshotChunkIssueCode::SnapshotValidationFailed), "snapshot validation failure should report chunk issue");
	Expect(!result.validation.valid, "snapshot validation failure should preserve validation report");
	Expect(!result.validation.issues.empty(), "snapshot validation failure should preserve validation issues");
}

} // namespace

int main()
{
	TestEncodingWithoutPlayerEmitsRequiredChunks();
	TestEncodingWithPlayerEmitsPlayerChunkInOrder();
	TestRoundTripPreservesAuthoritativeSnapshot();
	TestEncodedArchiveContainsNoCacheChunks();
	TestMissingRequiredChunksFailDecode();
	TestDuplicateSingletonChunkFailsDecode();
	TestUnsupportedChunkVersionFailsDecode();
	TestMalformedPayloadFailsDecode();
	TestInvalidPlayerEnumFailsDecode();
	TestUnknownChunkIgnored();
	TestHasPlayerTrueWithoutPlayerChunkFailsDecode();
	TestHasPlayerFalseWithoutPlayerChunkDecodes();
	TestHasPlayerFalseWithPlayerChunkIsIgnored();
	TestInvalidArchiveFailsBeforePayloadDecode();
	TestSnapshotValidationFailureAfterDecodeFailsDecode();

	if (Failures != 0)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
