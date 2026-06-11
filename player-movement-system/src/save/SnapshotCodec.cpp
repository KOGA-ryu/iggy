#include "SnapshotCodec.hpp"

#include <limits>

#include "save/SnapshotByteStream.hpp"
#include "save/SnapshotEntityCodec.hpp"
#include "save/SnapshotFrameCodec.hpp"

namespace dev {

namespace {

uint32_t CountOf(std::size_t size)
{
	return size > std::numeric_limits<uint32_t>::max()
	    ? std::numeric_limits<uint32_t>::max()
	    : static_cast<uint32_t>(size);
}

void WritePlayer(SnapshotByteWriter &writer, const SnapshotEntityCodec &entityCodec, const Player &player)
{
	entityCodec.writeActorPosition(writer, player.position);
	writer.writeU8(static_cast<uint8_t>(player.moveState));
	const std::vector<Point> steps = player.path.steps();
	writer.writeU32(CountOf(steps.size()));
	for (Point step : steps)
		entityCodec.writePoint(writer, step);
	entityCodec.writeDestinationAction(writer, player.destinationAction);
	writer.writeU8(player.movementModifiers.standGround ? 1U : 0U);
	writer.writeU8(player.animationLock.active ? 1U : 0U);
	writer.writeFloat(player.animationLock.elapsedSeconds);
	writer.writeFloat(player.animationLock.cancelAfterSeconds);
	entityCodec.writeCombatStats(writer, player.combatStats);
	writer.writeU32(CountOf(player.inventory.capacity));
	writer.writeU32(CountOf(player.inventory.items.size()));
	for (const Item &item : player.inventory.items)
		entityCodec.writeItem(writer, item);
	entityCodec.writeOptionalItem(writer, player.inventory.equipment.weapon);
	entityCodec.writeOptionalItem(writer, player.inventory.equipment.armor);
	entityCodec.writeOptionalItem(writer, player.inventory.equipment.accessory);
	writer.writeFloat(player.moveSpeedTilesPerSecond);
}

bool ReadPlayer(SnapshotByteReader &reader, const SnapshotEntityCodec &entityCodec, Player &player)
{
	uint8_t moveState = 0;
	uint32_t pathLength = 0;
	uint8_t standGround = 0;
	uint8_t animationActive = 0;
	if (!entityCodec.readActorPosition(reader, player.position)
	    || !reader.readU8(moveState)
	    || moveState > static_cast<uint8_t>(PlayerMoveState::Acting)
	    || !reader.readU32(pathLength))
		return false;

	std::vector<Point> steps;
	steps.reserve(pathLength);
	for (uint32_t i = 0; i < pathLength; ++i) {
		Point step;
		if (!entityCodec.readPoint(reader, step))
			return false;
		steps.push_back(step);
	}

	if (!entityCodec.readDestinationAction(reader, player.destinationAction)
	    || !reader.readU8(standGround)
	    || standGround > 1U
	    || !reader.readU8(animationActive)
	    || animationActive > 1U
	    || !reader.readFloat(player.animationLock.elapsedSeconds)
	    || !reader.readFloat(player.animationLock.cancelAfterSeconds)
	    || !entityCodec.readCombatStats(reader, player.combatStats))
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
		if (!entityCodec.readItem(reader, item))
			return false;
		player.inventory.items.push_back(item);
	}

	if (!entityCodec.readOptionalItem(reader, player.inventory.equipment.weapon)
	    || !entityCodec.readOptionalItem(reader, player.inventory.equipment.armor)
	    || !entityCodec.readOptionalItem(reader, player.inventory.equipment.accessory))
		return false;

	if (!reader.readFloat(player.moveSpeedTilesPerSecond))
		return false;

	player.moveState = static_cast<PlayerMoveState>(moveState);
	player.path.replace(std::move(steps));
	player.movementModifiers.standGround = standGround != 0U;
	player.animationLock.active = animationActive != 0U;
	return true;
}

void WriteEnemy(SnapshotByteWriter &writer, const SnapshotEntityCodec &entityCodec, const Enemy &enemy)
{
	writer.writeU32(enemy.id);
	entityCodec.writeActorPosition(writer, enemy.position);
	writer.writeU8(static_cast<uint8_t>(enemy.moveState));
	writer.writeI32(enemy.tuning.maxStepsPerTick);
	writer.writeI32(enemy.tuning.attackRangeTiles);
	writer.writeFloat(enemy.tuning.attackWindupSeconds);
	writer.writeFloat(enemy.tuning.attackRecoverySeconds);
	entityCodec.writeCombatStats(writer, enemy.combatStats);
	writer.writeFloat(enemy.stateTimerSeconds);
}

bool ReadEnemy(SnapshotByteReader &reader, const SnapshotEntityCodec &entityCodec, Enemy &enemy)
{
	uint8_t moveState = 0;
	if (!reader.readU32(enemy.id)
	    || !entityCodec.readActorPosition(reader, enemy.position)
	    || !reader.readU8(moveState)
	    || moveState > static_cast<uint8_t>(EnemyMoveState::Recovering)
	    || !reader.readI32(enemy.tuning.maxStepsPerTick)
	    || !reader.readI32(enemy.tuning.attackRangeTiles)
	    || !reader.readFloat(enemy.tuning.attackWindupSeconds)
	    || !reader.readFloat(enemy.tuning.attackRecoverySeconds)
	    || !entityCodec.readCombatStats(reader, enemy.combatStats)
	    || !reader.readFloat(enemy.stateTimerSeconds))
		return false;

	enemy.moveState = static_cast<EnemyMoveState>(moveState);
	return true;
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
	SnapshotEntityCodec entityCodec;
	WriteVector(writer, snapshot.players, [&](SnapshotByteWriter &itemWriter, const Player &player) {
		WritePlayer(itemWriter, entityCodec, player);
	});
	WriteVector(writer, snapshot.enemies, [&](SnapshotByteWriter &itemWriter, const Enemy &enemy) {
		WriteEnemy(itemWriter, entityCodec, enemy);
	});
	WriteVector(writer, snapshot.items, [&](SnapshotByteWriter &itemWriter, const Item &item) {
		entityCodec.writeItem(itemWriter, item);
	});
	WriteVector(writer, snapshot.combatants, [&](SnapshotByteWriter &itemWriter, const Combatant &combatant) {
		entityCodec.writeCombatant(itemWriter, combatant);
	});
	WriteVector(writer, snapshot.targets, [&](SnapshotByteWriter &itemWriter, const Target &target) {
		entityCodec.writeTarget(itemWriter, target);
	});
	return SnapshotFrameCodec {}.encode(payload);
}

std::optional<SimulationSnapshot> SnapshotCodec::decode(const SnapshotBytes &bytes) const
{
	std::optional<SnapshotBytes> payload = SnapshotFrameCodec {}.decode(bytes);
	if (!payload.has_value())
		return std::nullopt;

	SnapshotByteReader reader { *payload };
	SnapshotEntityCodec entityCodec;
	SimulationSnapshot snapshot;
	if (!ReadVector(reader, snapshot.players, [&](SnapshotByteReader &itemReader, Player &player) {
		    return ReadPlayer(itemReader, entityCodec, player);
	    })
	    || !ReadVector(reader, snapshot.enemies, [&](SnapshotByteReader &itemReader, Enemy &enemy) {
		    return ReadEnemy(itemReader, entityCodec, enemy);
	    })
	    || !ReadVector(reader, snapshot.items, [&](SnapshotByteReader &itemReader, Item &item) {
		    return entityCodec.readItem(itemReader, item);
	    })
	    || !ReadVector(reader, snapshot.combatants, [&](SnapshotByteReader &itemReader, Combatant &combatant) {
		    return entityCodec.readCombatant(itemReader, combatant);
	    })
	    || !ReadVector(reader, snapshot.targets, [&](SnapshotByteReader &itemReader, Target &target) {
		    return entityCodec.readTarget(itemReader, target);
	    })
	    || !reader.consumed())
		return std::nullopt;

	return snapshot;
}

} // namespace dev
