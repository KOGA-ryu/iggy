#include "SnapshotCodec.hpp"

#include <cstring>
#include <limits>

#include "interaction/DestinationAction.hpp"

namespace dev {

namespace {

constexpr uint8_t Magic0 = 'I';
constexpr uint8_t Magic1 = 'G';
constexpr uint8_t Magic2 = 'G';
constexpr uint8_t Magic3 = 'Y';
constexpr uint32_t SnapshotVersion = 7;
constexpr uint32_t FnvOffset = 2166136261U;
constexpr uint32_t FnvPrime = 16777619U;

class ByteWriter {
public:
	void writeU8(uint8_t value) { bytes_.push_back(value); }

	void writeU32(uint32_t value)
	{
		bytes_.push_back(static_cast<uint8_t>(value & 0xFFU));
		bytes_.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
		bytes_.push_back(static_cast<uint8_t>((value >> 16U) & 0xFFU));
		bytes_.push_back(static_cast<uint8_t>((value >> 24U) & 0xFFU));
	}

	void writeI32(int value)
	{
		writeU32(static_cast<uint32_t>(value));
	}

	void writeFloat(float value)
	{
		uint32_t bits = 0;
		static_assert(sizeof(bits) == sizeof(value));
		std::memcpy(&bits, &value, sizeof(bits));
		writeU32(bits);
	}

	SnapshotBytes take() { return std::move(bytes_); }

private:
	SnapshotBytes bytes_;
};

class ByteReader {
public:
	explicit ByteReader(const SnapshotBytes &bytes)
	    : bytes_(bytes)
	{
	}

	bool readU8(uint8_t &value)
	{
		if (offset_ + 1U > bytes_.size())
			return false;
		value = bytes_[offset_++];
		return true;
	}

	bool readU32(uint32_t &value)
	{
		if (offset_ + 4U > bytes_.size())
			return false;
		value = static_cast<uint32_t>(bytes_[offset_])
		    | (static_cast<uint32_t>(bytes_[offset_ + 1U]) << 8U)
		    | (static_cast<uint32_t>(bytes_[offset_ + 2U]) << 16U)
		    | (static_cast<uint32_t>(bytes_[offset_ + 3U]) << 24U);
		offset_ += 4U;
		return true;
	}

	bool readI32(int &value)
	{
		uint32_t raw = 0;
		if (!readU32(raw))
			return false;
		value = static_cast<int>(raw);
		return true;
	}

	bool readFloat(float &value)
	{
		uint32_t bits = 0;
		if (!readU32(bits))
			return false;
		static_assert(sizeof(bits) == sizeof(value));
		std::memcpy(&value, &bits, sizeof(value));
		return true;
	}

	[[nodiscard]] bool consumed() const
	{
		return offset_ == bytes_.size();
	}

private:
	const SnapshotBytes &bytes_;
	std::size_t offset_ = 0;
};

uint32_t CountOf(std::size_t size)
{
	return size > std::numeric_limits<uint32_t>::max()
	    ? std::numeric_limits<uint32_t>::max()
	    : static_cast<uint32_t>(size);
}

uint32_t ChecksumOf(const SnapshotBytes &bytes, std::size_t length)
{
	uint32_t hash = FnvOffset;
	for (std::size_t i = 0; i < length; ++i) {
		hash ^= bytes[i];
		hash *= FnvPrime;
	}
	return hash;
}

uint32_t ReadTrailingU32(const SnapshotBytes &bytes)
{
	const std::size_t offset = bytes.size() - 4U;
	return static_cast<uint32_t>(bytes[offset])
	    | (static_cast<uint32_t>(bytes[offset + 1U]) << 8U)
	    | (static_cast<uint32_t>(bytes[offset + 2U]) << 16U)
	    | (static_cast<uint32_t>(bytes[offset + 3U]) << 24U);
}

void WritePoint(ByteWriter &writer, Point point)
{
	writer.writeI32(point.x);
	writer.writeI32(point.y);
}

bool ReadPoint(ByteReader &reader, Point &point)
{
	return reader.readI32(point.x) && reader.readI32(point.y);
}

void WriteTarget(ByteWriter &writer, const Target &target)
{
	writer.writeU8(static_cast<uint8_t>(target.type));
	writer.writeU32(target.id);
	WritePoint(writer, target.tile);
}

bool ReadTarget(ByteReader &reader, Target &target)
{
	uint8_t type = 0;
	if (!reader.readU8(type) || type > static_cast<uint8_t>(TargetType::Object))
		return false;
	target.type = static_cast<TargetType>(type);
	return reader.readU32(target.id) && ReadPoint(reader, target.tile);
}

bool IsValidEquipmentSlot(uint8_t slot)
{
	return slot <= static_cast<uint8_t>(EquipmentSlot::Accessory);
}

void WriteCombatStats(ByteWriter &writer, const CombatStats &stats)
{
	writer.writeI32(stats.hitPoints);
	writer.writeI32(stats.attackPower);
	writer.writeI32(stats.defense);
}

bool ReadCombatStats(ByteReader &reader, CombatStats &stats)
{
	return reader.readI32(stats.hitPoints)
	    && reader.readI32(stats.attackPower)
	    && reader.readI32(stats.defense);
}

void WriteEquipmentCombatModifiers(ByteWriter &writer, const EquipmentCombatModifiers &modifiers)
{
	writer.writeI32(modifiers.attackPower);
	writer.writeI32(modifiers.defense);
}

bool ReadEquipmentCombatModifiers(ByteReader &reader, EquipmentCombatModifiers &modifiers)
{
	return reader.readI32(modifiers.attackPower)
	    && reader.readI32(modifiers.defense);
}

void WriteDestinationAction(ByteWriter &writer, const DestinationAction &action)
{
	writer.writeU8(static_cast<uint8_t>(action.type));
	WriteTarget(writer, action.target);
	writer.writeI32(action.rangeTiles);
}

bool ReadDestinationAction(ByteReader &reader, DestinationAction &action)
{
	uint8_t type = 0;
	if (!reader.readU8(type) || type > static_cast<uint8_t>(DestinationActionType::Interact))
		return false;
	action.type = static_cast<DestinationActionType>(type);
	return ReadTarget(reader, action.target) && reader.readI32(action.rangeTiles);
}

void WriteActorPosition(ByteWriter &writer, const ActorPosition &position)
{
	WritePoint(writer, position.tile);
	WritePoint(writer, position.future);
	WritePoint(writer, position.previous);
	WritePoint(writer, position.precise);
}

bool ReadActorPosition(ByteReader &reader, ActorPosition &position)
{
	return ReadPoint(reader, position.tile)
	    && ReadPoint(reader, position.future)
	    && ReadPoint(reader, position.previous)
	    && ReadPoint(reader, position.precise);
}

void WriteItem(ByteWriter &writer, const Item &item)
{
	writer.writeU32(item.id);
	WritePoint(writer, item.tile);
	writer.writeU8(item.equipmentSlot.has_value() ? 1U : 0U);
	if (item.equipmentSlot.has_value())
		writer.writeU8(static_cast<uint8_t>(*item.equipmentSlot));
	WriteEquipmentCombatModifiers(writer, item.combatModifiers);
}

bool ReadItem(ByteReader &reader, Item &item)
{
	uint8_t hasSlot = 0;
	if (!reader.readU32(item.id) || !ReadPoint(reader, item.tile) || !reader.readU8(hasSlot) || hasSlot > 1U)
		return false;
	item.equipmentSlot = std::nullopt;
	if (hasSlot != 0U) {
		uint8_t slot = 0;
		if (!reader.readU8(slot) || !IsValidEquipmentSlot(slot))
			return false;
		item.equipmentSlot = static_cast<EquipmentSlot>(slot);
	}
	return ReadEquipmentCombatModifiers(reader, item.combatModifiers);
}

void WriteOptionalItem(ByteWriter &writer, const std::optional<Item> &item)
{
	writer.writeU8(item.has_value() ? 1U : 0U);
	if (item.has_value())
		WriteItem(writer, *item);
}

bool ReadOptionalItem(ByteReader &reader, std::optional<Item> &item)
{
	uint8_t hasItem = 0;
	if (!reader.readU8(hasItem) || hasItem > 1U)
		return false;
	item = std::nullopt;
	if (hasItem == 0U)
		return true;

	Item decoded;
	if (!ReadItem(reader, decoded))
		return false;
	item = decoded;
	return true;
}

void WritePlayer(ByteWriter &writer, const Player &player)
{
	WriteActorPosition(writer, player.position);
	writer.writeU8(static_cast<uint8_t>(player.moveState));
	const std::vector<Point> steps = player.path.steps();
	writer.writeU32(CountOf(steps.size()));
	for (Point step : steps)
		WritePoint(writer, step);
	WriteDestinationAction(writer, player.destinationAction);
	writer.writeU8(player.movementModifiers.standGround ? 1U : 0U);
	writer.writeU8(player.animationLock.active ? 1U : 0U);
	writer.writeFloat(player.animationLock.elapsedSeconds);
	writer.writeFloat(player.animationLock.cancelAfterSeconds);
	WriteCombatStats(writer, player.combatStats);
	writer.writeU32(CountOf(player.inventory.capacity));
	writer.writeU32(CountOf(player.inventory.items.size()));
	for (const Item &item : player.inventory.items)
		WriteItem(writer, item);
	WriteOptionalItem(writer, player.inventory.equipment.weapon);
	WriteOptionalItem(writer, player.inventory.equipment.armor);
	WriteOptionalItem(writer, player.inventory.equipment.accessory);
	writer.writeFloat(player.moveSpeedTilesPerSecond);
}

bool ReadPlayer(ByteReader &reader, Player &player)
{
	uint8_t moveState = 0;
	uint32_t pathLength = 0;
	uint8_t standGround = 0;
	uint8_t animationActive = 0;
	if (!ReadActorPosition(reader, player.position)
	    || !reader.readU8(moveState)
	    || moveState > static_cast<uint8_t>(PlayerMoveState::Acting)
	    || !reader.readU32(pathLength))
		return false;

	std::vector<Point> steps;
	steps.reserve(pathLength);
	for (uint32_t i = 0; i < pathLength; ++i) {
		Point step;
		if (!ReadPoint(reader, step))
			return false;
		steps.push_back(step);
	}

	if (!ReadDestinationAction(reader, player.destinationAction)
	    || !reader.readU8(standGround)
	    || standGround > 1U
	    || !reader.readU8(animationActive)
	    || animationActive > 1U
	    || !reader.readFloat(player.animationLock.elapsedSeconds)
	    || !reader.readFloat(player.animationLock.cancelAfterSeconds)
	    || !ReadCombatStats(reader, player.combatStats))
		return false;

	uint32_t inventoryCapacity = 0;
	uint32_t inventoryCount = 0;
	if (!reader.readU32(inventoryCapacity) || !reader.readU32(inventoryCount))
		return false;
	player.inventory.capacity = inventoryCapacity;
	player.inventory.items.clear();
	player.inventory.items.reserve(inventoryCount);
	for (uint32_t i = 0; i < inventoryCount; ++i) {
		Item item;
		if (!ReadItem(reader, item))
			return false;
		player.inventory.items.push_back(item);
	}

	if (!ReadOptionalItem(reader, player.inventory.equipment.weapon)
	    || !ReadOptionalItem(reader, player.inventory.equipment.armor)
	    || !ReadOptionalItem(reader, player.inventory.equipment.accessory))
		return false;

	if (!reader.readFloat(player.moveSpeedTilesPerSecond))
		return false;

	player.moveState = static_cast<PlayerMoveState>(moveState);
	player.path.replace(std::move(steps));
	player.movementModifiers.standGround = standGround != 0U;
	player.animationLock.active = animationActive != 0U;
	return true;
}

void WriteEnemy(ByteWriter &writer, const Enemy &enemy)
{
	writer.writeU32(enemy.id);
	WriteActorPosition(writer, enemy.position);
	writer.writeU8(static_cast<uint8_t>(enemy.moveState));
	writer.writeI32(enemy.tuning.maxStepsPerTick);
	writer.writeI32(enemy.tuning.attackRangeTiles);
	writer.writeFloat(enemy.tuning.attackWindupSeconds);
	writer.writeFloat(enemy.tuning.attackRecoverySeconds);
	WriteCombatStats(writer, enemy.combatStats);
	writer.writeFloat(enemy.stateTimerSeconds);
}

bool ReadEnemy(ByteReader &reader, Enemy &enemy)
{
	uint8_t moveState = 0;
	if (!reader.readU32(enemy.id)
	    || !ReadActorPosition(reader, enemy.position)
	    || !reader.readU8(moveState)
	    || moveState > static_cast<uint8_t>(EnemyMoveState::Recovering)
	    || !reader.readI32(enemy.tuning.maxStepsPerTick)
	    || !reader.readI32(enemy.tuning.attackRangeTiles)
	    || !reader.readFloat(enemy.tuning.attackWindupSeconds)
	    || !reader.readFloat(enemy.tuning.attackRecoverySeconds)
	    || !ReadCombatStats(reader, enemy.combatStats)
	    || !reader.readFloat(enemy.stateTimerSeconds))
		return false;

	enemy.moveState = static_cast<EnemyMoveState>(moveState);
	return true;
}

void WriteCombatant(ByteWriter &writer, const Combatant &combatant)
{
	WriteTarget(writer, combatant.target);
	WriteCombatStats(writer, combatant.stats);
}

bool ReadCombatant(ByteReader &reader, Combatant &combatant)
{
	return ReadTarget(reader, combatant.target) && ReadCombatStats(reader, combatant.stats);
}

template <typename T, typename WriteFn>
void WriteVector(ByteWriter &writer, const std::vector<T> &items, WriteFn writeItem)
{
	writer.writeU32(CountOf(items.size()));
	for (const T &item : items)
		writeItem(writer, item);
}

template <typename T, typename ReadFn>
bool ReadVector(ByteReader &reader, std::vector<T> &items, ReadFn readItem)
{
	uint32_t count = 0;
	if (!reader.readU32(count))
		return false;

	items.clear();
	items.reserve(count);
	for (uint32_t i = 0; i < count; ++i) {
		T item;
		if (!readItem(reader, item))
			return false;
		items.push_back(std::move(item));
	}
	return true;
}

} // namespace

SnapshotBytes SnapshotCodec::encode(const SimulationSnapshot &snapshot) const
{
	ByteWriter writer;
	writer.writeU8(Magic0);
	writer.writeU8(Magic1);
	writer.writeU8(Magic2);
	writer.writeU8(Magic3);
	writer.writeU32(SnapshotVersion);
	WriteVector(writer, snapshot.players, WritePlayer);
	WriteVector(writer, snapshot.enemies, WriteEnemy);
	WriteVector(writer, snapshot.items, WriteItem);
	WriteVector(writer, snapshot.combatants, WriteCombatant);
	WriteVector(writer, snapshot.targets, WriteTarget);
	const SnapshotBytes payload = writer.take();
	ByteWriter finalWriter;
	for (uint8_t byte : payload)
		finalWriter.writeU8(byte);
	finalWriter.writeU32(ChecksumOf(payload, payload.size()));
	return finalWriter.take();
}

std::optional<SimulationSnapshot> SnapshotCodec::decode(const SnapshotBytes &bytes) const
{
	if (bytes.size() < 12U)
		return std::nullopt;

	const std::size_t payloadSize = bytes.size() - 4U;
	const uint32_t expectedChecksum = ReadTrailingU32(bytes);
	if (ChecksumOf(bytes, payloadSize) != expectedChecksum)
		return std::nullopt;

	SnapshotBytes payload { bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(payloadSize) };
	ByteReader reader { payload };
	uint8_t magic0 = 0;
	uint8_t magic1 = 0;
	uint8_t magic2 = 0;
	uint8_t magic3 = 0;
	uint32_t version = 0;
	if (!reader.readU8(magic0) || !reader.readU8(magic1) || !reader.readU8(magic2) || !reader.readU8(magic3)
	    || magic0 != Magic0 || magic1 != Magic1 || magic2 != Magic2 || magic3 != Magic3
	    || !reader.readU32(version) || version != SnapshotVersion)
		return std::nullopt;

	SimulationSnapshot snapshot;
	if (!ReadVector(reader, snapshot.players, ReadPlayer)
	    || !ReadVector(reader, snapshot.enemies, ReadEnemy)
	    || !ReadVector(reader, snapshot.items, ReadItem)
	    || !ReadVector(reader, snapshot.combatants, ReadCombatant)
	    || !ReadVector(reader, snapshot.targets, ReadTarget)
	    || !reader.consumed())
		return std::nullopt;

	return snapshot;
}

} // namespace dev
