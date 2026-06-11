#include "SnapshotPlayerCodec.hpp"

#include <limits>

namespace dev {

namespace {

uint32_t CountOf(std::size_t size)
{
	return size > std::numeric_limits<uint32_t>::max()
	    ? std::numeric_limits<uint32_t>::max()
	    : static_cast<uint32_t>(size);
}

} // namespace

void SnapshotPlayerCodec::writePlayer(SnapshotByteWriter &writer, const Player &player) const
{
	entityCodec_.writeActorPosition(writer, player.position);
	writer.writeU8(static_cast<uint8_t>(player.moveState));
	const std::vector<Point> steps = player.path.steps();
	writer.writeU32(CountOf(steps.size()));
	for (Point step : steps)
		entityCodec_.writePoint(writer, step);
	entityCodec_.writeDestinationAction(writer, player.destinationAction);
	writer.writeU8(player.movementModifiers.standGround ? 1U : 0U);
	writer.writeU8(player.animationLock.active ? 1U : 0U);
	writer.writeFloat(player.animationLock.elapsedSeconds);
	writer.writeFloat(player.animationLock.cancelAfterSeconds);
	entityCodec_.writeCombatStats(writer, player.combatStats);
	writer.writeU32(CountOf(player.inventory.capacity));
	writer.writeU32(CountOf(player.inventory.items.size()));
	for (const Item &item : player.inventory.items)
		entityCodec_.writeItem(writer, item);
	entityCodec_.writeOptionalItem(writer, player.inventory.equipment.weapon);
	entityCodec_.writeOptionalItem(writer, player.inventory.equipment.armor);
	entityCodec_.writeOptionalItem(writer, player.inventory.equipment.accessory);
	writer.writeFloat(player.moveSpeedTilesPerSecond);
}

bool SnapshotPlayerCodec::readPlayer(SnapshotByteReader &reader, Player &player) const
{
	uint8_t moveState = 0;
	uint32_t pathLength = 0;
	uint8_t standGround = 0;
	uint8_t animationActive = 0;
	if (!entityCodec_.readActorPosition(reader, player.position)
	    || !reader.readU8(moveState)
	    || moveState > static_cast<uint8_t>(PlayerMoveState::Acting)
	    || !reader.readU32(pathLength))
		return false;

	std::vector<Point> steps;
	steps.reserve(pathLength);
	for (uint32_t i = 0; i < pathLength; ++i) {
		Point step;
		if (!entityCodec_.readPoint(reader, step))
			return false;
		steps.push_back(step);
	}

	if (!entityCodec_.readDestinationAction(reader, player.destinationAction)
	    || !reader.readU8(standGround)
	    || standGround > 1U
	    || !reader.readU8(animationActive)
	    || animationActive > 1U
	    || !reader.readFloat(player.animationLock.elapsedSeconds)
	    || !reader.readFloat(player.animationLock.cancelAfterSeconds)
	    || !entityCodec_.readCombatStats(reader, player.combatStats))
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
		if (!entityCodec_.readItem(reader, item))
			return false;
		player.inventory.items.push_back(item);
	}

	if (!entityCodec_.readOptionalItem(reader, player.inventory.equipment.weapon)
	    || !entityCodec_.readOptionalItem(reader, player.inventory.equipment.armor)
	    || !entityCodec_.readOptionalItem(reader, player.inventory.equipment.accessory))
		return false;

	if (!reader.readFloat(player.moveSpeedTilesPerSecond))
		return false;

	player.moveState = static_cast<PlayerMoveState>(moveState);
	player.path.replace(std::move(steps));
	player.movementModifiers.standGround = standGround != 0U;
	player.animationLock.active = animationActive != 0U;
	return true;
}

} // namespace dev
