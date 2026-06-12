#include "runtime/RuntimeSessionSnapshotChunkCodec.hpp"

#include <string>
#include <string_view>

#include "runtime/RuntimeBinaryCodec.hpp"

namespace iggy::runtime {
namespace {

const RuntimeSaveChunkId SessionChunkId = makeRuntimeSaveChunkId('S', 'E', 'S', 'S');
const RuntimeSaveChunkId LevelMapChunkId = makeRuntimeSaveChunkId('L', 'M', 'A', 'P');
const RuntimeSaveChunkId PlayerChunkId = makeRuntimeSaveChunkId('P', 'L', 'Y', 'R');
const RuntimeSaveChunkId NpcsChunkId = makeRuntimeSaveChunkId('N', 'P', 'C', 'S');
constexpr std::uint32_t ChunkVersion = 1;

void writeString(RuntimeBinaryWriter &writer, std::string_view value)
{
	writer.writeU32LE(static_cast<std::uint32_t>(value.size()));
	writer.writeBytes(std::vector<std::uint8_t>(value.begin(), value.end()));
}

void writeResourceId(RuntimeBinaryWriter &writer, const ResourceId &id)
{
	writeString(writer, id.value());
}

bool readString(RuntimeBinaryReader &reader, std::string &value)
{
	std::uint32_t length = 0;
	if (reader.readU32LE(length) != RuntimeBinaryReadStatus::Ok)
		return false;

	std::vector<std::uint8_t> bytes;
	if (reader.readBytes(length, bytes) != RuntimeBinaryReadStatus::Ok)
		return false;

	value.assign(bytes.begin(), bytes.end());
	return true;
}

bool readResourceId(RuntimeBinaryReader &reader, ResourceId &id)
{
	std::string value;
	if (!readString(reader, value))
		return false;
	id = ResourceId(value);
	return true;
}

void writeTileCoord(RuntimeBinaryWriter &writer, TileCoord tile)
{
	writer.writeI32LE(tile.x);
	writer.writeI32LE(tile.y);
}

bool readTileCoord(RuntimeBinaryReader &reader, TileCoord &tile)
{
	TileCoord decoded;
	if (reader.readI32LE(decoded.x) != RuntimeBinaryReadStatus::Ok)
		return false;
	if (reader.readI32LE(decoded.y) != RuntimeBinaryReadStatus::Ok)
		return false;
	tile = decoded;
	return true;
}

void writeVec2(RuntimeBinaryWriter &writer, Vec2 value)
{
	writer.writeF32LE(value.x);
	writer.writeF32LE(value.y);
}

bool readVec2(RuntimeBinaryReader &reader, Vec2 &value)
{
	Vec2 decoded;
	if (reader.readF32LE(decoded.x) != RuntimeBinaryReadStatus::Ok)
		return false;
	if (reader.readF32LE(decoded.y) != RuntimeBinaryReadStatus::Ok)
		return false;
	value = decoded;
	return true;
}

std::uint32_t encodeMovementStatus(PlayerMovementStatus status)
{
	switch (status) {
	case PlayerMovementStatus::Idle:
		return 0;
	case PlayerMovementStatus::Moving:
		return 1;
	}
	return 0;
}

bool decodeMovementStatus(std::uint32_t value, PlayerMovementStatus &status)
{
	switch (value) {
	case 0:
		status = PlayerMovementStatus::Idle;
		return true;
	case 1:
		status = PlayerMovementStatus::Moving;
		return true;
	default:
		return false;
	}
}

std::uint32_t encodeFacing(PlayerFacing2D facing)
{
	switch (facing) {
	case PlayerFacing2D::None:
		return 0;
	case PlayerFacing2D::North:
		return 1;
	case PlayerFacing2D::South:
		return 2;
	case PlayerFacing2D::East:
		return 3;
	case PlayerFacing2D::West:
		return 4;
	}
	return 0;
}

bool decodeFacing(std::uint32_t value, PlayerFacing2D &facing)
{
	switch (value) {
	case 0:
		facing = PlayerFacing2D::None;
		return true;
	case 1:
		facing = PlayerFacing2D::North;
		return true;
	case 2:
		facing = PlayerFacing2D::South;
		return true;
	case 3:
		facing = PlayerFacing2D::East;
		return true;
	case 4:
		facing = PlayerFacing2D::West;
		return true;
	default:
		return false;
	}
}

RuntimeSaveChunk makeChunk(RuntimeSaveChunkId id, std::vector<std::uint8_t> payload)
{
	return { id, ChunkVersion, payload };
}

std::vector<std::uint8_t> encodeSessionPayload(const RuntimeSessionSnapshot &snapshot)
{
	RuntimeBinaryWriter writer;
	writer.writeU64LE(static_cast<std::uint64_t>(snapshot.tickIndex));
	writer.writeBool(snapshot.hasPlayer);
	return writer.takeBytes();
}

std::vector<std::uint8_t> encodeLevelMapPayload(const LevelTileMap &map)
{
	RuntimeBinaryWriter writer;
	writeResourceId(writer, map.id);
	writer.writeI32LE(map.width);
	writer.writeI32LE(map.height);
	writer.writeI32LE(map.playerStart.x);
	writer.writeI32LE(map.playerStart.y);
	writer.writeU32LE(static_cast<std::uint32_t>(map.tiles.size()));
	for (const LevelTile &tile : map.tiles)
		writer.writeBool(tile.walkable);
	writer.writeU32LE(static_cast<std::uint32_t>(map.entitySpawns.size()));
	for (const LevelEntitySpawn &spawn : map.entitySpawns) {
		writeResourceId(writer, spawn.type);
		writer.writeI32LE(spawn.x);
		writer.writeI32LE(spawn.y);
		writeResourceId(writer, spawn.id);
	}
	return writer.takeBytes();
}

std::vector<std::uint8_t> encodePlayerPayload(const PlayerAgentState &player)
{
	RuntimeBinaryWriter writer;
	writeResourceId(writer, player.id);
	writeVec2(writer, player.position);
	writeTileCoord(writer, player.spawnTile);
	writer.writeU32LE(encodeMovementStatus(player.movementStatus));
	writer.writeU32LE(encodeFacing(player.facing));
	return writer.takeBytes();
}

std::vector<std::uint8_t> encodeNpcsPayload(const std::vector<npc_ai::NpcAgentEntry> &npcs)
{
	RuntimeBinaryWriter writer;
	writer.writeU32LE(static_cast<std::uint32_t>(npcs.size()));
	for (const npc_ai::NpcAgentEntry &npc : npcs) {
		writeResourceId(writer, npc.id);
		writeVec2(writer, npc.state.position);
		writeTileCoord(writer, npc.state.homeTile);
		writer.writeBool(npc.state.awareness.playerVisible);
		writer.writeBool(npc.state.awareness.alerted);
		writer.writeI32LE(npc.state.awareness.alertTicksRemaining);
		writeTileCoord(writer, npc.state.awareness.lastSeenTile);
		writeVec2(writer, npc.state.awareness.lastSeenPosition);
		writer.writeU64LE(static_cast<std::uint64_t>(npc.state.followState.waypointIndex));
		writer.writeBool(npc.state.followState.completed);
	}
	return writer.takeBytes();
}

bool ensureFullyRead(RuntimeBinaryReader &reader)
{
	return reader.remaining() == 0;
}

bool decodeSessionPayload(const RuntimeSaveChunk &chunk, RuntimeSessionSnapshot &snapshot)
{
	RuntimeBinaryReader reader(chunk.payload);
	std::uint64_t tickIndex = 0;
	bool hasPlayer = false;
	if (reader.readU64LE(tickIndex) != RuntimeBinaryReadStatus::Ok)
		return false;
	if (reader.readBool(hasPlayer) != RuntimeBinaryReadStatus::Ok)
		return false;
	if (!ensureFullyRead(reader))
		return false;
	snapshot.tickIndex = static_cast<std::size_t>(tickIndex);
	snapshot.hasPlayer = hasPlayer;
	return true;
}

bool decodeLevelMapPayload(const RuntimeSaveChunk &chunk, LevelTileMap &map)
{
	RuntimeBinaryReader reader(chunk.payload);
	LevelTileMap decoded;
	std::uint32_t tileCount = 0;
	std::uint32_t entitySpawnCount = 0;
	if (!readResourceId(reader, decoded.id))
		return false;
	if (reader.readI32LE(decoded.width) != RuntimeBinaryReadStatus::Ok)
		return false;
	if (reader.readI32LE(decoded.height) != RuntimeBinaryReadStatus::Ok)
		return false;
	if (reader.readI32LE(decoded.playerStart.x) != RuntimeBinaryReadStatus::Ok)
		return false;
	if (reader.readI32LE(decoded.playerStart.y) != RuntimeBinaryReadStatus::Ok)
		return false;
	if (reader.readU32LE(tileCount) != RuntimeBinaryReadStatus::Ok)
		return false;
	decoded.tiles.reserve(tileCount);
	for (std::uint32_t index = 0; index < tileCount; ++index) {
		bool walkable = true;
		if (reader.readBool(walkable) != RuntimeBinaryReadStatus::Ok)
			return false;
		decoded.tiles.push_back({ walkable });
	}
	if (reader.readU32LE(entitySpawnCount) != RuntimeBinaryReadStatus::Ok)
		return false;
	decoded.entitySpawns.reserve(entitySpawnCount);
	for (std::uint32_t index = 0; index < entitySpawnCount; ++index) {
		LevelEntitySpawn spawn;
		if (!readResourceId(reader, spawn.type))
			return false;
		if (reader.readI32LE(spawn.x) != RuntimeBinaryReadStatus::Ok)
			return false;
		if (reader.readI32LE(spawn.y) != RuntimeBinaryReadStatus::Ok)
			return false;
		if (!readResourceId(reader, spawn.id))
			return false;
		decoded.entitySpawns.push_back(spawn);
	}
	if (!ensureFullyRead(reader))
		return false;
	map = decoded;
	return true;
}

enum class DecodePlayerPayloadStatus {
	Ok,
	Malformed,
	InvalidEnum,
};

DecodePlayerPayloadStatus decodePlayerPayload(const RuntimeSaveChunk &chunk, PlayerAgentState &player)
{
	RuntimeBinaryReader reader(chunk.payload);
	PlayerAgentState decoded;
	std::uint32_t movementStatus = 0;
	std::uint32_t facing = 0;
	if (!readResourceId(reader, decoded.id))
		return DecodePlayerPayloadStatus::Malformed;
	if (!readVec2(reader, decoded.position))
		return DecodePlayerPayloadStatus::Malformed;
	if (!readTileCoord(reader, decoded.spawnTile))
		return DecodePlayerPayloadStatus::Malformed;
	if (reader.readU32LE(movementStatus) != RuntimeBinaryReadStatus::Ok)
		return DecodePlayerPayloadStatus::Malformed;
	if (reader.readU32LE(facing) != RuntimeBinaryReadStatus::Ok)
		return DecodePlayerPayloadStatus::Malformed;
	if (!decodeMovementStatus(movementStatus, decoded.movementStatus))
		return DecodePlayerPayloadStatus::InvalidEnum;
	if (!decodeFacing(facing, decoded.facing))
		return DecodePlayerPayloadStatus::InvalidEnum;
	if (!ensureFullyRead(reader))
		return DecodePlayerPayloadStatus::Malformed;
	player = decoded;
	return DecodePlayerPayloadStatus::Ok;
}

bool decodeNpcsPayload(const RuntimeSaveChunk &chunk, std::vector<npc_ai::NpcAgentEntry> &npcs)
{
	RuntimeBinaryReader reader(chunk.payload);
	std::uint32_t npcCount = 0;
	if (reader.readU32LE(npcCount) != RuntimeBinaryReadStatus::Ok)
		return false;

	std::vector<npc_ai::NpcAgentEntry> decoded;
	decoded.reserve(npcCount);
	for (std::uint32_t index = 0; index < npcCount; ++index) {
		npc_ai::NpcAgentEntry npc;
		if (!readResourceId(reader, npc.id))
			return false;
		if (!readVec2(reader, npc.state.position))
			return false;
		if (!readTileCoord(reader, npc.state.homeTile))
			return false;
		if (reader.readBool(npc.state.awareness.playerVisible) != RuntimeBinaryReadStatus::Ok)
			return false;
		if (reader.readBool(npc.state.awareness.alerted) != RuntimeBinaryReadStatus::Ok)
			return false;
		if (reader.readI32LE(npc.state.awareness.alertTicksRemaining) != RuntimeBinaryReadStatus::Ok)
			return false;
		if (!readTileCoord(reader, npc.state.awareness.lastSeenTile))
			return false;
		if (!readVec2(reader, npc.state.awareness.lastSeenPosition))
			return false;
		std::uint64_t waypointIndex = 0;
		if (reader.readU64LE(waypointIndex) != RuntimeBinaryReadStatus::Ok)
			return false;
		npc.state.followState.waypointIndex = static_cast<std::size_t>(waypointIndex);
		if (reader.readBool(npc.state.followState.completed) != RuntimeBinaryReadStatus::Ok)
			return false;
		decoded.push_back(npc);
	}
	if (!ensureFullyRead(reader))
		return false;
	npcs = decoded;
	return true;
}

void addIssue(
	RuntimeSessionSnapshotDecodeResult &result,
	RuntimeSessionSnapshotChunkIssueCode code,
	RuntimeSaveChunkId chunkId,
	std::size_t chunkIndex = 0)
{
	result.issues.push_back({ code, chunkId, chunkIndex });
}

bool requireSingleChunk(
	const RuntimeSaveChunkArchive &archive,
	RuntimeSaveChunkId id,
	RuntimeSessionSnapshotDecodeResult &result)
{
	const std::vector<std::size_t> indexes = findChunkIndexes(archive, id);
	if (indexes.empty()) {
		addIssue(result, RuntimeSessionSnapshotChunkIssueCode::MissingRequiredChunk, id);
		return false;
	}
	for (std::size_t index = 1; index < indexes.size(); ++index)
		addIssue(result, RuntimeSessionSnapshotChunkIssueCode::DuplicateSingletonChunk, id, indexes[index]);
	return indexes.size() == 1;
}

void rejectDuplicateOptionalChunk(
	const RuntimeSaveChunkArchive &archive,
	RuntimeSaveChunkId id,
	RuntimeSessionSnapshotDecodeResult &result)
{
	const std::vector<std::size_t> indexes = findChunkIndexes(archive, id);
	for (std::size_t index = 1; index < indexes.size(); ++index)
		addIssue(result, RuntimeSessionSnapshotChunkIssueCode::DuplicateSingletonChunk, id, indexes[index]);
}

bool checkVersion(
	const RuntimeSaveChunkArchive &archive,
	std::size_t chunkIndex,
	RuntimeSessionSnapshotDecodeResult &result)
{
	const RuntimeSaveChunk &chunk = archive.chunks[chunkIndex];
	if (chunk.version == ChunkVersion)
		return true;
	addIssue(result, RuntimeSessionSnapshotChunkIssueCode::UnsupportedChunkVersion, chunk.id, chunkIndex);
	return false;
}

} // namespace

RuntimeSaveChunkArchive RuntimeSessionSnapshotChunkEncoder::encode(const RuntimeSessionSnapshot &snapshot) const
{
	RuntimeSaveChunkArchiveBuilder builder;
	builder.addChunk(makeChunk(SessionChunkId, encodeSessionPayload(snapshot)));
	builder.addChunk(makeChunk(LevelMapChunkId, encodeLevelMapPayload(snapshot.level.map)));
	if (snapshot.hasPlayer)
		builder.addChunk(makeChunk(PlayerChunkId, encodePlayerPayload(snapshot.player)));
	builder.addChunk(makeChunk(NpcsChunkId, encodeNpcsPayload(snapshot.level.npcAgents)));
	return builder.build();
}

RuntimeSessionSnapshotDecodeResult RuntimeSessionSnapshotChunkDecoder::decode(const RuntimeSaveChunkArchive &archive) const
{
	RuntimeSessionSnapshotDecodeResult result;
	if (!RuntimeSaveChunkArchiveValidator {}.validate(archive).valid) {
		addIssue(result, RuntimeSessionSnapshotChunkIssueCode::InvalidArchive, {});
		return result;
	}

	const bool hasSession = requireSingleChunk(archive, SessionChunkId, result);
	const bool hasLevelMap = requireSingleChunk(archive, LevelMapChunkId, result);
	const bool hasNpcs = requireSingleChunk(archive, NpcsChunkId, result);
	rejectDuplicateOptionalChunk(archive, PlayerChunkId, result);
	if (!result.issues.empty())
		return result;

	const std::size_t sessionIndex = findChunkIndexes(archive, SessionChunkId).front();
	const std::size_t levelMapIndex = findChunkIndexes(archive, LevelMapChunkId).front();
	const std::size_t npcsIndex = findChunkIndexes(archive, NpcsChunkId).front();
	if (!hasSession || !hasLevelMap || !hasNpcs)
		return result;
	if (!checkVersion(archive, sessionIndex, result) || !checkVersion(archive, levelMapIndex, result) || !checkVersion(archive, npcsIndex, result))
		return result;

	RuntimeSessionSnapshot decoded;
	if (!decodeSessionPayload(archive.chunks[sessionIndex], decoded)) {
		addIssue(result, RuntimeSessionSnapshotChunkIssueCode::MalformedChunkPayload, SessionChunkId, sessionIndex);
		return result;
	}

	const std::vector<std::size_t> playerIndexes = findChunkIndexes(archive, PlayerChunkId);
	if (decoded.hasPlayer && playerIndexes.empty()) {
		addIssue(result, RuntimeSessionSnapshotChunkIssueCode::MissingRequiredChunk, PlayerChunkId);
		return result;
	}

	if (!decodeLevelMapPayload(archive.chunks[levelMapIndex], decoded.level.map)) {
		addIssue(result, RuntimeSessionSnapshotChunkIssueCode::MalformedChunkPayload, LevelMapChunkId, levelMapIndex);
		return result;
	}

	if (decoded.hasPlayer) {
		const std::size_t playerIndex = playerIndexes.front();
		if (!checkVersion(archive, playerIndex, result))
			return result;
		switch (decodePlayerPayload(archive.chunks[playerIndex], decoded.player)) {
		case DecodePlayerPayloadStatus::Ok:
			break;
		case DecodePlayerPayloadStatus::Malformed:
			addIssue(result, RuntimeSessionSnapshotChunkIssueCode::MalformedChunkPayload, PlayerChunkId, playerIndex);
			return result;
		case DecodePlayerPayloadStatus::InvalidEnum:
			addIssue(result, RuntimeSessionSnapshotChunkIssueCode::InvalidEnumValue, PlayerChunkId, playerIndex);
			return result;
		}
	}

	if (!decodeNpcsPayload(archive.chunks[npcsIndex], decoded.level.npcAgents)) {
		addIssue(result, RuntimeSessionSnapshotChunkIssueCode::MalformedChunkPayload, NpcsChunkId, npcsIndex);
		return result;
	}

	result.validation = RuntimeSessionSnapshotValidator {}.validate(decoded);
	if (!result.validation.valid) {
		addIssue(result, RuntimeSessionSnapshotChunkIssueCode::SnapshotValidationFailed, {});
		return result;
	}

	result.decoded = true;
	result.snapshot = decoded;
	return result;
}

} // namespace iggy::runtime
