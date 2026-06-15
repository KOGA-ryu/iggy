#include "runtime/RuntimeGameplaySnapshotChunkCodec.hpp"

#include <cstdint>
#include <string>
#include <vector>

#include "runtime/RuntimeBinaryCodec.hpp"

namespace iggy::runtime {
namespace {

constexpr std::uint32_t ChunkVersion = 1;

const RuntimeSaveChunkId CommandQueueChunkId = makeRuntimeSaveChunkId('G', 'Q', 'U', 'E');
const RuntimeSaveChunkId InteractionChunkId = makeRuntimeSaveChunkId('G', 'I', 'N', 'T');
const RuntimeSaveChunkId InventoryChunkId = makeRuntimeSaveChunkId('G', 'I', 'N', 'V');
const RuntimeSaveChunkId NpcActorsChunkId = makeRuntimeSaveChunkId('G', 'A', 'C', 'T');
const RuntimeSaveChunkId NpcControlsChunkId = makeRuntimeSaveChunkId('G', 'C', 'T', 'L');

enum class DecodePayloadStatus {
	Ok,
	Malformed,
	InvalidEnum,
};

void writeString(RuntimeBinaryWriter &writer, std::string_view value)
{
	writer.writeU32LE(static_cast<std::uint32_t>(value.size()));
	writer.writeBytes(std::vector<std::uint8_t>(value.begin(), value.end()));
}

void writeResourceId(RuntimeBinaryWriter &writer, const ResourceId &id)
{
	writeString(writer, id.value());
}

void writeVec2(RuntimeBinaryWriter &writer, Vec2 value)
{
	writer.writeF32LE(value.x);
	writer.writeF32LE(value.y);
}

void writeTileCoord(RuntimeBinaryWriter &writer, TileCoord tile)
{
	writer.writeI32LE(tile.x);
	writer.writeI32LE(tile.y);
}

bool readString(RuntimeBinaryReader &reader, std::string &value)
{
	std::uint32_t size = 0;
	if (reader.readU32LE(size) != RuntimeBinaryReadStatus::Ok)
		return false;
	std::vector<std::uint8_t> bytes;
	if (reader.readBytes(size, bytes) != RuntimeBinaryReadStatus::Ok)
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

bool readVec2(RuntimeBinaryReader &reader, Vec2 &value)
{
	return reader.readF32LE(value.x) == RuntimeBinaryReadStatus::Ok
		&& reader.readF32LE(value.y) == RuntimeBinaryReadStatus::Ok;
}

bool readTileCoord(RuntimeBinaryReader &reader, TileCoord &tile)
{
	return reader.readI32LE(tile.x) == RuntimeBinaryReadStatus::Ok
		&& reader.readI32LE(tile.y) == RuntimeBinaryReadStatus::Ok;
}

bool ensureFullyRead(const RuntimeBinaryReader &reader)
{
	return reader.remaining() == 0;
}

RuntimeSaveChunk makeChunk(RuntimeSaveChunkId id, std::vector<std::uint8_t> payload)
{
	return { id, ChunkVersion, std::move(payload) };
}

std::uint32_t encodeCommandType(GameplayCommand2DType type)
{
	switch (type) {
	case GameplayCommand2DType::None:
		return 0;
	case GameplayCommand2DType::MoveToPoint:
		return 1;
	case GameplayCommand2DType::MoveToTile:
		return 2;
	case GameplayCommand2DType::Interact:
		return 3;
	case GameplayCommand2DType::Wait:
		return 4;
	}
	return 0;
}

bool decodeCommandType(std::uint32_t value, GameplayCommand2DType &type)
{
	switch (value) {
	case 0:
		type = GameplayCommand2DType::None;
		return true;
	case 1:
		type = GameplayCommand2DType::MoveToPoint;
		return true;
	case 2:
		type = GameplayCommand2DType::MoveToTile;
		return true;
	case 3:
		type = GameplayCommand2DType::Interact;
		return true;
	case 4:
		type = GameplayCommand2DType::Wait;
		return true;
	}
	return false;
}

std::uint32_t encodeInteractionTargetKind(InteractionTarget2DKind kind)
{
	switch (kind) {
	case InteractionTarget2DKind::Unknown:
		return 0;
	case InteractionTarget2DKind::Inspectable:
		return 1;
	case InteractionTarget2DKind::Usable:
		return 2;
	case InteractionTarget2DKind::Pickup:
		return 3;
	case InteractionTarget2DKind::Talk:
		return 4;
	case InteractionTarget2DKind::Door:
		return 5;
	}
	return 0;
}

bool decodeInteractionTargetKind(std::uint32_t value, InteractionTarget2DKind &kind)
{
	switch (value) {
	case 0:
		kind = InteractionTarget2DKind::Unknown;
		return true;
	case 1:
		kind = InteractionTarget2DKind::Inspectable;
		return true;
	case 2:
		kind = InteractionTarget2DKind::Usable;
		return true;
	case 3:
		kind = InteractionTarget2DKind::Pickup;
		return true;
	case 4:
		kind = InteractionTarget2DKind::Talk;
		return true;
	case 5:
		kind = InteractionTarget2DKind::Door;
		return true;
	}
	return false;
}

std::uint32_t encodeInteractionEffectType(InteractionEffect2DType type)
{
	switch (type) {
	case InteractionEffect2DType::None:
		return 0;
	case InteractionEffect2DType::InspectText:
		return 1;
	case InteractionEffect2DType::ToggleTarget:
		return 2;
	case InteractionEffect2DType::EmitEvent:
		return 3;
	case InteractionEffect2DType::PickupItem:
		return 4;
	}
	return 0;
}

bool decodeInteractionEffectType(std::uint32_t value, InteractionEffect2DType &type)
{
	switch (value) {
	case 0:
		type = InteractionEffect2DType::None;
		return true;
	case 1:
		type = InteractionEffect2DType::InspectText;
		return true;
	case 2:
		type = InteractionEffect2DType::ToggleTarget;
		return true;
	case 3:
		type = InteractionEffect2DType::EmitEvent;
		return true;
	case 4:
		type = InteractionEffect2DType::PickupItem;
		return true;
	}
	return false;
}

std::uint32_t encodeObjectiveType(NpcObjectiveType type)
{
	switch (type) {
	case NpcObjectiveType::None:
		return 0;
	case NpcObjectiveType::Wait:
		return 1;
	case NpcObjectiveType::Patrol:
		return 2;
	case NpcObjectiveType::Guard:
		return 3;
	case NpcObjectiveType::Investigate:
		return 4;
	case NpcObjectiveType::Flee:
		return 5;
	case NpcObjectiveType::Follow:
		return 6;
	case NpcObjectiveType::Attack:
		return 7;
	case NpcObjectiveType::MoveTo:
		return 8;
	case NpcObjectiveType::Interact:
		return 9;
	}
	return 0;
}

bool decodeObjectiveType(std::uint32_t value, NpcObjectiveType &type)
{
	switch (value) {
	case 0:
		type = NpcObjectiveType::None;
		return true;
	case 1:
		type = NpcObjectiveType::Wait;
		return true;
	case 2:
		type = NpcObjectiveType::Patrol;
		return true;
	case 3:
		type = NpcObjectiveType::Guard;
		return true;
	case 4:
		type = NpcObjectiveType::Investigate;
		return true;
	case 5:
		type = NpcObjectiveType::Flee;
		return true;
	case 6:
		type = NpcObjectiveType::Follow;
		return true;
	case 7:
		type = NpcObjectiveType::Attack;
		return true;
	case 8:
		type = NpcObjectiveType::MoveTo;
		return true;
	case 9:
		type = NpcObjectiveType::Interact;
		return true;
	}
	return false;
}

std::uint32_t encodeBehaviorStateType(NpcBehaviorStateType type)
{
	switch (type) {
	case NpcBehaviorStateType::None:
		return 0;
	case NpcBehaviorStateType::Idle:
		return 1;
	case NpcBehaviorStateType::Waiting:
		return 2;
	case NpcBehaviorStateType::Seeking:
		return 3;
	case NpcBehaviorStateType::Fleeing:
		return 4;
	case NpcBehaviorStateType::Attacking:
		return 5;
	case NpcBehaviorStateType::Interacting:
		return 6;
	case NpcBehaviorStateType::Stunned:
		return 7;
	case NpcBehaviorStateType::Disabled:
		return 8;
	}
	return 0;
}

bool decodeBehaviorStateType(std::uint32_t value, NpcBehaviorStateType &type)
{
	switch (value) {
	case 0:
		type = NpcBehaviorStateType::None;
		return true;
	case 1:
		type = NpcBehaviorStateType::Idle;
		return true;
	case 2:
		type = NpcBehaviorStateType::Waiting;
		return true;
	case 3:
		type = NpcBehaviorStateType::Seeking;
		return true;
	case 4:
		type = NpcBehaviorStateType::Fleeing;
		return true;
	case 5:
		type = NpcBehaviorStateType::Attacking;
		return true;
	case 6:
		type = NpcBehaviorStateType::Interacting;
		return true;
	case 7:
		type = NpcBehaviorStateType::Stunned;
		return true;
	case 8:
		type = NpcBehaviorStateType::Disabled;
		return true;
	}
	return false;
}

std::uint32_t encodeMoveMode(NpcMoveMode mode)
{
	switch (mode) {
	case NpcMoveMode::None:
		return 0;
	case NpcMoveMode::Still:
		return 1;
	case NpcMoveMode::Walk:
		return 2;
	case NpcMoveMode::Jog:
		return 3;
	case NpcMoveMode::Run:
		return 4;
	case NpcMoveMode::Sprint:
		return 5;
	}
	return 0;
}

bool decodeMoveMode(std::uint32_t value, NpcMoveMode &mode)
{
	switch (value) {
	case 0:
		mode = NpcMoveMode::None;
		return true;
	case 1:
		mode = NpcMoveMode::Still;
		return true;
	case 2:
		mode = NpcMoveMode::Walk;
		return true;
	case 3:
		mode = NpcMoveMode::Jog;
		return true;
	case 4:
		mode = NpcMoveMode::Run;
		return true;
	case 5:
		mode = NpcMoveMode::Sprint;
		return true;
	}
	return false;
}

std::vector<std::uint8_t> encodeCommandQueuePayload(const RuntimeCommandQueueState &queue)
{
	RuntimeBinaryWriter writer;
	writer.writeU32LE(static_cast<std::uint32_t>(queue.frames.size()));
	for (const GameplayCommandFrame2D &frame : queue.frames) {
		writer.writeU32LE(static_cast<std::uint32_t>(frame.commands.size()));
		for (const GameplayCommand2D &command : frame.commands) {
			writer.writeU32LE(encodeCommandType(command.type));
			writeResourceId(writer, command.actorId);
			writeVec2(writer, command.targetPoint);
			writeTileCoord(writer, command.targetTile);
			writeResourceId(writer, command.targetId);
		}
	}
	return writer.takeBytes();
}

DecodePayloadStatus decodeCommandQueuePayload(const RuntimeSaveChunk &chunk, RuntimeCommandQueueState &queue)
{
	RuntimeBinaryReader reader(chunk.payload);
	std::uint32_t frameCount = 0;
	if (reader.readU32LE(frameCount) != RuntimeBinaryReadStatus::Ok)
		return DecodePayloadStatus::Malformed;
	RuntimeCommandQueueState decoded;
	decoded.frames.reserve(frameCount);
	for (std::uint32_t frameIndex = 0; frameIndex < frameCount; ++frameIndex) {
		GameplayCommandFrame2D frame;
		std::uint32_t commandCount = 0;
		if (reader.readU32LE(commandCount) != RuntimeBinaryReadStatus::Ok)
			return DecodePayloadStatus::Malformed;
		frame.commands.reserve(commandCount);
		for (std::uint32_t commandIndex = 0; commandIndex < commandCount; ++commandIndex) {
			GameplayCommand2D command;
			std::uint32_t type = 0;
			if (reader.readU32LE(type) != RuntimeBinaryReadStatus::Ok)
				return DecodePayloadStatus::Malformed;
			if (!decodeCommandType(type, command.type))
				return DecodePayloadStatus::InvalidEnum;
			if (!readResourceId(reader, command.actorId))
				return DecodePayloadStatus::Malformed;
			if (!readVec2(reader, command.targetPoint))
				return DecodePayloadStatus::Malformed;
			if (!readTileCoord(reader, command.targetTile))
				return DecodePayloadStatus::Malformed;
			if (!readResourceId(reader, command.targetId))
				return DecodePayloadStatus::Malformed;
			frame.commands.push_back(command);
		}
		decoded.frames.push_back(frame);
	}
	if (!ensureFullyRead(reader))
		return DecodePayloadStatus::Malformed;
	queue = decoded;
	return DecodePayloadStatus::Ok;
}

std::vector<std::uint8_t> encodeInteractionPayload(const RuntimeInteractionState &interaction)
{
	RuntimeBinaryWriter writer;
	writer.writeU32LE(static_cast<std::uint32_t>(interaction.targets.targets().size()));
	for (const InteractionTarget2D &target : interaction.targets.targets()) {
		writeResourceId(writer, target.id);
		writer.writeU32LE(encodeInteractionTargetKind(target.kind));
		writeVec2(writer, target.position);
		writer.writeF32LE(target.radius);
		writer.writeBool(target.enabled);
	}

	writer.writeU32LE(static_cast<std::uint32_t>(interaction.effects.entries().size()));
	for (const InteractionEffectEntry2D &entry : interaction.effects.entries()) {
		writeResourceId(writer, entry.targetId);
		writer.writeU32LE(static_cast<std::uint32_t>(entry.effects.size()));
		for (const InteractionEffect2D &effect : entry.effects) {
			writer.writeU32LE(encodeInteractionEffectType(effect.type));
			writeResourceId(writer, effect.targetId);
			writeResourceId(writer, effect.eventId);
			writeResourceId(writer, effect.dropId);
			writeString(writer, effect.text);
			writer.writeBool(effect.enabledValue);
		}
	}
	return writer.takeBytes();
}

DecodePayloadStatus decodeInteractionPayload(const RuntimeSaveChunk &chunk, RuntimeInteractionState &interaction)
{
	RuntimeBinaryReader reader(chunk.payload);
	std::uint32_t targetCount = 0;
	if (reader.readU32LE(targetCount) != RuntimeBinaryReadStatus::Ok)
		return DecodePayloadStatus::Malformed;
	std::vector<InteractionTarget2D> targets;
	targets.reserve(targetCount);
	for (std::uint32_t index = 0; index < targetCount; ++index) {
		InteractionTarget2D target;
		std::uint32_t kind = 0;
		if (!readResourceId(reader, target.id))
			return DecodePayloadStatus::Malformed;
		if (reader.readU32LE(kind) != RuntimeBinaryReadStatus::Ok)
			return DecodePayloadStatus::Malformed;
		if (!decodeInteractionTargetKind(kind, target.kind))
			return DecodePayloadStatus::InvalidEnum;
		if (!readVec2(reader, target.position))
			return DecodePayloadStatus::Malformed;
		if (reader.readF32LE(target.radius) != RuntimeBinaryReadStatus::Ok)
			return DecodePayloadStatus::Malformed;
		if (reader.readBool(target.enabled) != RuntimeBinaryReadStatus::Ok)
			return DecodePayloadStatus::Malformed;
		targets.push_back(target);
	}

	std::uint32_t entryCount = 0;
	if (reader.readU32LE(entryCount) != RuntimeBinaryReadStatus::Ok)
		return DecodePayloadStatus::Malformed;
	std::vector<InteractionEffectEntry2D> entries;
	entries.reserve(entryCount);
	for (std::uint32_t entryIndex = 0; entryIndex < entryCount; ++entryIndex) {
		InteractionEffectEntry2D entry;
		std::uint32_t effectCount = 0;
		if (!readResourceId(reader, entry.targetId))
			return DecodePayloadStatus::Malformed;
		if (reader.readU32LE(effectCount) != RuntimeBinaryReadStatus::Ok)
			return DecodePayloadStatus::Malformed;
		entry.effects.reserve(effectCount);
		for (std::uint32_t effectIndex = 0; effectIndex < effectCount; ++effectIndex) {
			InteractionEffect2D effect;
			std::uint32_t type = 0;
			if (reader.readU32LE(type) != RuntimeBinaryReadStatus::Ok)
				return DecodePayloadStatus::Malformed;
			if (!decodeInteractionEffectType(type, effect.type))
				return DecodePayloadStatus::InvalidEnum;
			if (!readResourceId(reader, effect.targetId))
				return DecodePayloadStatus::Malformed;
			if (!readResourceId(reader, effect.eventId))
				return DecodePayloadStatus::Malformed;
			if (!readResourceId(reader, effect.dropId))
				return DecodePayloadStatus::Malformed;
			if (!readString(reader, effect.text))
				return DecodePayloadStatus::Malformed;
			if (reader.readBool(effect.enabledValue) != RuntimeBinaryReadStatus::Ok)
				return DecodePayloadStatus::Malformed;
			entry.effects.push_back(effect);
		}
		entries.push_back(entry);
	}

	if (!ensureFullyRead(reader))
		return DecodePayloadStatus::Malformed;
	interaction.targets = InteractionTarget2DRegistry(targets);
	interaction.effects = InteractionEffectCatalog2D(entries);
	return DecodePayloadStatus::Ok;
}

std::vector<std::uint8_t> encodeInventoryPayload(const RuntimeInventoryState &inventory)
{
	RuntimeBinaryWriter writer;
	writer.writeU32LE(static_cast<std::uint32_t>(inventory.inventory.stacks.size()));
	for (const InventoryItemStack2D &stack : inventory.inventory.stacks) {
		writeResourceId(writer, stack.itemId);
		writer.writeU32LE(stack.count);
	}

	writer.writeU32LE(static_cast<std::uint32_t>(inventory.drops.drops.size()));
	for (const LevelItemDrop2D &drop : inventory.drops.drops) {
		writeResourceId(writer, drop.id);
		writeResourceId(writer, drop.itemId);
		writer.writeU32LE(drop.count);
		writeVec2(writer, drop.position);
		writer.writeF32LE(drop.pickupRadius);
		writer.writeBool(drop.enabled);
	}
	return writer.takeBytes();
}

bool decodeInventoryPayload(const RuntimeSaveChunk &chunk, RuntimeInventoryState &inventory)
{
	RuntimeBinaryReader reader(chunk.payload);
	std::uint32_t stackCount = 0;
	if (reader.readU32LE(stackCount) != RuntimeBinaryReadStatus::Ok)
		return false;
	RuntimeInventoryState decoded;
	decoded.inventory.stacks.reserve(stackCount);
	for (std::uint32_t index = 0; index < stackCount; ++index) {
		InventoryItemStack2D stack;
		if (!readResourceId(reader, stack.itemId))
			return false;
		if (reader.readU32LE(stack.count) != RuntimeBinaryReadStatus::Ok)
			return false;
		decoded.inventory.stacks.push_back(stack);
	}

	std::uint32_t dropCount = 0;
	if (reader.readU32LE(dropCount) != RuntimeBinaryReadStatus::Ok)
		return false;
	decoded.drops.drops.reserve(dropCount);
	for (std::uint32_t index = 0; index < dropCount; ++index) {
		LevelItemDrop2D drop;
		if (!readResourceId(reader, drop.id))
			return false;
		if (!readResourceId(reader, drop.itemId))
			return false;
		if (reader.readU32LE(drop.count) != RuntimeBinaryReadStatus::Ok)
			return false;
		if (!readVec2(reader, drop.position))
			return false;
		if (reader.readF32LE(drop.pickupRadius) != RuntimeBinaryReadStatus::Ok)
			return false;
		if (reader.readBool(drop.enabled) != RuntimeBinaryReadStatus::Ok)
			return false;
		decoded.drops.drops.push_back(drop);
	}

	if (!ensureFullyRead(reader))
		return false;
	inventory = decoded;
	return true;
}

std::vector<std::uint8_t> encodeNpcActorsPayload(const NpcActorState2DRegistry &registry)
{
	RuntimeBinaryWriter writer;
	writer.writeU32LE(static_cast<std::uint32_t>(registry.actors.size()));
	for (const NpcActorState2D &actor : registry.actors) {
		writeResourceId(writer, actor.npcId);
		writeResourceId(writer, actor.aiProfileId);
		writeResourceId(writer, actor.factionId);
		writeVec2(writer, actor.position);
		writeResourceId(writer, actor.currentGoalId);
		writer.writeBool(actor.present);
	}
	return writer.takeBytes();
}

bool decodeNpcActorsPayload(const RuntimeSaveChunk &chunk, NpcActorState2DRegistry &registry)
{
	RuntimeBinaryReader reader(chunk.payload);
	std::uint32_t actorCount = 0;
	if (reader.readU32LE(actorCount) != RuntimeBinaryReadStatus::Ok)
		return false;
	NpcActorState2DRegistry decoded;
	decoded.actors.reserve(actorCount);
	for (std::uint32_t index = 0; index < actorCount; ++index) {
		NpcActorState2D actor;
		if (!readResourceId(reader, actor.npcId))
			return false;
		if (!readResourceId(reader, actor.aiProfileId))
			return false;
		if (!readResourceId(reader, actor.factionId))
			return false;
		if (!readVec2(reader, actor.position))
			return false;
		if (!readResourceId(reader, actor.currentGoalId))
			return false;
		if (reader.readBool(actor.present) != RuntimeBinaryReadStatus::Ok)
			return false;
		decoded.actors.push_back(actor);
	}
	if (!ensureFullyRead(reader))
		return false;
	registry = decoded;
	return true;
}

void writeObjective(RuntimeBinaryWriter &writer, const NpcObjective &objective)
{
	writer.writeU32LE(encodeObjectiveType(objective.type));
	writeResourceId(writer, objective.targetId);
	writeVec2(writer, objective.targetPosition);
}

void writeBehavior(RuntimeBinaryWriter &writer, const NpcBehaviorState &behavior)
{
	writer.writeU32LE(encodeBehaviorStateType(behavior.type));
	writeResourceId(writer, behavior.targetId);
	writeVec2(writer, behavior.targetPosition);
}

DecodePayloadStatus readObjective(RuntimeBinaryReader &reader, NpcObjective &objective)
{
	std::uint32_t type = 0;
	if (reader.readU32LE(type) != RuntimeBinaryReadStatus::Ok)
		return DecodePayloadStatus::Malformed;
	if (!decodeObjectiveType(type, objective.type))
		return DecodePayloadStatus::InvalidEnum;
	if (!readResourceId(reader, objective.targetId))
		return DecodePayloadStatus::Malformed;
	if (!readVec2(reader, objective.targetPosition))
		return DecodePayloadStatus::Malformed;
	return DecodePayloadStatus::Ok;
}

DecodePayloadStatus readBehavior(RuntimeBinaryReader &reader, NpcBehaviorState &behavior)
{
	std::uint32_t type = 0;
	if (reader.readU32LE(type) != RuntimeBinaryReadStatus::Ok)
		return DecodePayloadStatus::Malformed;
	if (!decodeBehaviorStateType(type, behavior.type))
		return DecodePayloadStatus::InvalidEnum;
	if (!readResourceId(reader, behavior.targetId))
		return DecodePayloadStatus::Malformed;
	if (!readVec2(reader, behavior.targetPosition))
		return DecodePayloadStatus::Malformed;
	return DecodePayloadStatus::Ok;
}

std::vector<std::uint8_t> encodeNpcControlsPayload(const NpcActorControlState2DRegistry &registry)
{
	RuntimeBinaryWriter writer;
	writer.writeU32LE(static_cast<std::uint32_t>(registry.entries.size()));
	for (const NpcActorControlState2D &control : registry.entries) {
		writeResourceId(writer, control.npcId);
		writeObjective(writer, control.objective);
		writeBehavior(writer, control.behavior);
		writer.writeU32LE(encodeMoveMode(control.moveMode));
	}
	return writer.takeBytes();
}

DecodePayloadStatus decodeNpcControlsPayload(const RuntimeSaveChunk &chunk, NpcActorControlState2DRegistry &registry)
{
	RuntimeBinaryReader reader(chunk.payload);
	std::uint32_t controlCount = 0;
	if (reader.readU32LE(controlCount) != RuntimeBinaryReadStatus::Ok)
		return DecodePayloadStatus::Malformed;
	NpcActorControlState2DRegistry decoded;
	decoded.entries.reserve(controlCount);
	for (std::uint32_t index = 0; index < controlCount; ++index) {
		NpcActorControlState2D control;
		if (!readResourceId(reader, control.npcId))
			return DecodePayloadStatus::Malformed;
		DecodePayloadStatus status = readObjective(reader, control.objective);
		if (status != DecodePayloadStatus::Ok)
			return status;
		status = readBehavior(reader, control.behavior);
		if (status != DecodePayloadStatus::Ok)
			return status;
		std::uint32_t moveMode = 0;
		if (reader.readU32LE(moveMode) != RuntimeBinaryReadStatus::Ok)
			return DecodePayloadStatus::Malformed;
		if (!decodeMoveMode(moveMode, control.moveMode))
			return DecodePayloadStatus::InvalidEnum;
		decoded.entries.push_back(control);
	}
	if (!ensureFullyRead(reader))
		return DecodePayloadStatus::Malformed;
	registry = decoded;
	return DecodePayloadStatus::Ok;
}

void addIssue(
	RuntimeGameplaySnapshotDecodeResult &result,
	RuntimeGameplaySnapshotChunkIssueCode code,
	RuntimeSaveChunkId chunkId,
	std::size_t chunkIndex = 0)
{
	result.issues.push_back({ code, chunkId, chunkIndex });
}

bool requireSingleChunk(
	const RuntimeSaveChunkArchive &archive,
	RuntimeSaveChunkId id,
	RuntimeGameplaySnapshotDecodeResult &result)
{
	const std::vector<std::size_t> indexes = findChunkIndexes(archive, id);
	if (indexes.empty()) {
		addIssue(result, RuntimeGameplaySnapshotChunkIssueCode::MissingRequiredChunk, id);
		return false;
	}
	for (std::size_t index = 1; index < indexes.size(); ++index)
		addIssue(result, RuntimeGameplaySnapshotChunkIssueCode::DuplicateSingletonChunk, id, indexes[index]);
	return indexes.size() == 1;
}

bool checkVersion(
	const RuntimeSaveChunkArchive &archive,
	std::size_t chunkIndex,
	RuntimeGameplaySnapshotDecodeResult &result)
{
	const RuntimeSaveChunk &chunk = archive.chunks[chunkIndex];
	if (chunk.version == ChunkVersion)
		return true;
	addIssue(result, RuntimeGameplaySnapshotChunkIssueCode::UnsupportedChunkVersion, chunk.id, chunkIndex);
	return false;
}

bool handlePayloadStatus(
	RuntimeGameplaySnapshotDecodeResult &result,
	RuntimeSaveChunkId id,
	std::size_t chunkIndex,
	DecodePayloadStatus status)
{
	switch (status) {
	case DecodePayloadStatus::Ok:
		return true;
	case DecodePayloadStatus::Malformed:
		addIssue(result, RuntimeGameplaySnapshotChunkIssueCode::MalformedChunkPayload, id, chunkIndex);
		return false;
	case DecodePayloadStatus::InvalidEnum:
		addIssue(result, RuntimeGameplaySnapshotChunkIssueCode::InvalidEnumValue, id, chunkIndex);
		return false;
	}
	return false;
}

} // namespace

RuntimeGameplaySnapshotEncodeResult RuntimeGameplaySnapshotChunkEncoder::encode(
	const RuntimeGameplaySnapshot &snapshot) const
{
	RuntimeGameplaySnapshotEncodeResult result;
	result.archive = RuntimeSessionSnapshotChunkEncoder {}.encode(snapshot.session);
	result.archive.chunks.push_back(makeChunk(CommandQueueChunkId, encodeCommandQueuePayload(snapshot.commandQueue)));
	result.archive.chunks.push_back(makeChunk(InteractionChunkId, encodeInteractionPayload(snapshot.interaction)));
	result.archive.chunks.push_back(makeChunk(InventoryChunkId, encodeInventoryPayload(snapshot.inventory)));
	result.archive.chunks.push_back(makeChunk(NpcActorsChunkId, encodeNpcActorsPayload(snapshot.npcActors)));
	result.archive.chunks.push_back(makeChunk(NpcControlsChunkId, encodeNpcControlsPayload(snapshot.npcControls)));
	return result;
}

RuntimeGameplaySnapshotDecodeResult RuntimeGameplaySnapshotChunkDecoder::decode(
	const RuntimeSaveChunkArchive &archive) const
{
	RuntimeGameplaySnapshotDecodeResult result;
	if (!RuntimeSaveChunkArchiveValidator {}.validate(archive).valid) {
		addIssue(result, RuntimeGameplaySnapshotChunkIssueCode::InvalidArchive, {});
		return result;
	}

	result.session = RuntimeSessionSnapshotChunkDecoder {}.decode(archive);
	if (!result.session.decoded) {
		addIssue(result, RuntimeGameplaySnapshotChunkIssueCode::NestedSessionDecodeFailed, {});
		return result;
	}

	const bool hasCommandQueue = requireSingleChunk(archive, CommandQueueChunkId, result);
	const bool hasInteraction = requireSingleChunk(archive, InteractionChunkId, result);
	const bool hasInventory = requireSingleChunk(archive, InventoryChunkId, result);
	const bool hasNpcActors = requireSingleChunk(archive, NpcActorsChunkId, result);
	const bool hasNpcControls = requireSingleChunk(archive, NpcControlsChunkId, result);
	if (!result.issues.empty())
		return result;
	if (!hasCommandQueue || !hasInteraction || !hasInventory || !hasNpcActors || !hasNpcControls)
		return result;

	const std::size_t commandQueueIndex = findChunkIndexes(archive, CommandQueueChunkId).front();
	const std::size_t interactionIndex = findChunkIndexes(archive, InteractionChunkId).front();
	const std::size_t inventoryIndex = findChunkIndexes(archive, InventoryChunkId).front();
	const std::size_t npcActorsIndex = findChunkIndexes(archive, NpcActorsChunkId).front();
	const std::size_t npcControlsIndex = findChunkIndexes(archive, NpcControlsChunkId).front();
	if (!checkVersion(archive, commandQueueIndex, result)
		|| !checkVersion(archive, interactionIndex, result)
		|| !checkVersion(archive, inventoryIndex, result)
		|| !checkVersion(archive, npcActorsIndex, result)
		|| !checkVersion(archive, npcControlsIndex, result)) {
		return result;
	}

	RuntimeGameplaySnapshot decoded;
	decoded.session = result.session.snapshot;
	if (!handlePayloadStatus(result, CommandQueueChunkId, commandQueueIndex, decodeCommandQueuePayload(archive.chunks[commandQueueIndex], decoded.commandQueue)))
		return result;
	if (!handlePayloadStatus(result, InteractionChunkId, interactionIndex, decodeInteractionPayload(archive.chunks[interactionIndex], decoded.interaction)))
		return result;
	if (!decodeInventoryPayload(archive.chunks[inventoryIndex], decoded.inventory)) {
		addIssue(result, RuntimeGameplaySnapshotChunkIssueCode::MalformedChunkPayload, InventoryChunkId, inventoryIndex);
		return result;
	}
	if (!decodeNpcActorsPayload(archive.chunks[npcActorsIndex], decoded.npcActors)) {
		addIssue(result, RuntimeGameplaySnapshotChunkIssueCode::MalformedChunkPayload, NpcActorsChunkId, npcActorsIndex);
		return result;
	}
	if (!handlePayloadStatus(result, NpcControlsChunkId, npcControlsIndex, decodeNpcControlsPayload(archive.chunks[npcControlsIndex], decoded.npcControls)))
		return result;

	result.validation = RuntimeGameplaySnapshotValidator {}.validate(decoded);
	if (!result.validation.valid) {
		addIssue(result, RuntimeGameplaySnapshotChunkIssueCode::SnapshotValidationFailed, {});
		return result;
	}

	result.decoded = true;
	result.snapshot = decoded;
	return result;
}

} // namespace iggy::runtime
