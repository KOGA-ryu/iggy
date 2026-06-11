#include "SnapshotCodec.hpp"

#include <limits>

#include "interaction/DestinationAction.hpp"
#include "save/SnapshotByteStream.hpp"
#include "save/SnapshotFrameCodec.hpp"

namespace dev {

namespace {

uint32_t CountOf(std::size_t size)
{
	return size > std::numeric_limits<uint32_t>::max()
	    ? std::numeric_limits<uint32_t>::max()
	    : static_cast<uint32_t>(size);
}

void WritePoint(SnapshotByteWriter &writer, Point point)
{
	writer.writeI32(point.x);
	writer.writeI32(point.y);
}

bool ReadPoint(SnapshotByteReader &reader, Point &point)
{
	return reader.readI32(point.x) && reader.readI32(point.y);
}

void WriteTarget(SnapshotByteWriter &writer, const Target &target)
{
	writer.writeU8(static_cast<uint8_t>(target.type));
	writer.writeU32(target.id);
	WritePoint(writer, target.tile);
}

bool ReadTarget(SnapshotByteReader &reader, Target &target)
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

void WriteCombatStats(SnapshotByteWriter &writer, const CombatStats &stats)
{
	writer.writeI32(stats.hitPoints);
	writer.writeI32(stats.attackPower);
	writer.writeI32(stats.defense);
}

bool ReadCombatStats(SnapshotByteReader &reader, CombatStats &stats)
{
	return reader.readI32(stats.hitPoints)
	    && reader.readI32(stats.attackPower)
	    && reader.readI32(stats.defense);
}

void WriteEquipmentCombatModifiers(SnapshotByteWriter &writer, const EquipmentCombatModifiers &modifiers)
{
	writer.writeI32(modifiers.attackPower);
	writer.writeI32(modifiers.defense);
}

bool ReadEquipmentCombatModifiers(SnapshotByteReader &reader, EquipmentCombatModifiers &modifiers)
{
	return reader.readI32(modifiers.attackPower)
	    && reader.readI32(modifiers.defense);
}

void WriteDestinationAction(SnapshotByteWriter &writer, const DestinationAction &action)
{
	writer.writeU8(static_cast<uint8_t>(action.type));
	WriteTarget(writer, action.target);
	writer.writeI32(action.rangeTiles);
}

bool ReadDestinationAction(SnapshotByteReader &reader, DestinationAction &action)
{
	uint8_t type = 0;
	if (!reader.readU8(type) || type > static_cast<uint8_t>(DestinationActionType::Interact))
		return false;
	action.type = static_cast<DestinationActionType>(type);
	return ReadTarget(reader, action.target) && reader.readI32(action.rangeTiles);
}

void WriteActorPosition(SnapshotByteWriter &writer, const ActorPosition &position)
{
	WritePoint(writer, position.tile);
	WritePoint(writer, position.future);
	WritePoint(writer, position.previous);
	WritePoint(writer, position.precise);
}

bool ReadActorPosition(SnapshotByteReader &reader, ActorPosition &position)
{
	return ReadPoint(reader, position.tile)
	    && ReadPoint(reader, position.future)
	    && ReadPoint(reader, position.previous)
	    && ReadPoint(reader, position.precise);
}

void WriteItem(SnapshotByteWriter &writer, const Item &item)
{
	writer.writeU32(item.id);
	WritePoint(writer, item.tile);
	writer.writeU8(item.equipmentSlot.has_value() ? 1U : 0U);
	if (item.equipmentSlot.has_value())
		writer.writeU8(static_cast<uint8_t>(*item.equipmentSlot));
	WriteEquipmentCombatModifiers(writer, item.combatModifiers);
}

bool ReadItem(SnapshotByteReader &reader, Item &item)
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

void WriteOptionalItem(SnapshotByteWriter &writer, const std::optional<Item> &item)
{
	writer.writeU8(item.has_value() ? 1U : 0U);
	if (item.has_value())
		WriteItem(writer, *item);
}

bool ReadOptionalItem(SnapshotByteReader &reader, std::optional<Item> &item)
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

void WritePlayer(SnapshotByteWriter &writer, const Player &player)
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

bool ReadPlayer(SnapshotByteReader &reader, Player &player)
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

void WriteEnemy(SnapshotByteWriter &writer, const Enemy &enemy)
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

bool ReadEnemy(SnapshotByteReader &reader, Enemy &enemy)
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

void WriteCombatant(SnapshotByteWriter &writer, const Combatant &combatant)
{
	WriteTarget(writer, combatant.target);
	WriteCombatStats(writer, combatant.stats);
}

bool ReadCombatant(SnapshotByteReader &reader, Combatant &combatant)
{
	return ReadTarget(reader, combatant.target) && ReadCombatStats(reader, combatant.stats);
}

template <typename T, typename WriteFn>
void WriteVector(SnapshotByteWriter &writer, const std::vector<T> &items, WriteFn writeItem)
{
	writer.writeU32(CountOf(items.size()));
	for (const T &item : items)
		writeItem(writer, item);
}

template <typename T, typename ReadFn>
bool ReadVector(SnapshotByteReader &reader, std::vector<T> &items, ReadFn readItem)
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
	SnapshotBytes payload;
	SnapshotByteWriter writer { payload };
	WriteVector(writer, snapshot.players, WritePlayer);
	WriteVector(writer, snapshot.enemies, WriteEnemy);
	WriteVector(writer, snapshot.items, WriteItem);
	WriteVector(writer, snapshot.combatants, WriteCombatant);
	WriteVector(writer, snapshot.targets, WriteTarget);
	return SnapshotFrameCodec {}.encode(payload);
}

std::optional<SimulationSnapshot> SnapshotCodec::decode(const SnapshotBytes &bytes) const
{
	std::optional<SnapshotBytes> payload = SnapshotFrameCodec {}.decode(bytes);
	if (!payload.has_value())
		return std::nullopt;

	SnapshotByteReader reader { *payload };
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
