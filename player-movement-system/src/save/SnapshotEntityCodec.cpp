#include "SnapshotEntityCodec.hpp"

namespace dev {

void SnapshotEntityCodec::writePoint(SnapshotByteWriter &writer, Point point) const
{
	writer.writeI32(point.x);
	writer.writeI32(point.y);
}

bool SnapshotEntityCodec::readPoint(SnapshotByteReader &reader, Point &point) const
{
	return reader.readI32(point.x) && reader.readI32(point.y);
}

void SnapshotEntityCodec::writeTarget(SnapshotByteWriter &writer, const Target &target) const
{
	writer.writeU8(static_cast<uint8_t>(target.type));
	writer.writeU32(target.id);
	writePoint(writer, target.tile);
}

bool SnapshotEntityCodec::readTarget(SnapshotByteReader &reader, Target &target) const
{
	uint8_t type = 0;
	if (!reader.readU8(type) || type > static_cast<uint8_t>(TargetType::Object))
		return false;
	target.type = static_cast<TargetType>(type);
	return reader.readU32(target.id) && readPoint(reader, target.tile);
}

void SnapshotEntityCodec::writeCombatStats(SnapshotByteWriter &writer, const CombatStats &stats) const
{
	writer.writeI32(stats.hitPoints);
	writer.writeI32(stats.attackPower);
	writer.writeI32(stats.defense);
}

bool SnapshotEntityCodec::readCombatStats(SnapshotByteReader &reader, CombatStats &stats) const
{
	return reader.readI32(stats.hitPoints)
	    && reader.readI32(stats.attackPower)
	    && reader.readI32(stats.defense);
}

void SnapshotEntityCodec::writeEquipmentCombatModifiers(SnapshotByteWriter &writer, const EquipmentCombatModifiers &modifiers) const
{
	writer.writeI32(modifiers.attackPower);
	writer.writeI32(modifiers.defense);
}

bool SnapshotEntityCodec::readEquipmentCombatModifiers(SnapshotByteReader &reader, EquipmentCombatModifiers &modifiers) const
{
	return reader.readI32(modifiers.attackPower)
	    && reader.readI32(modifiers.defense);
}

void SnapshotEntityCodec::writeDestinationAction(SnapshotByteWriter &writer, const DestinationAction &action) const
{
	writer.writeU8(static_cast<uint8_t>(action.type));
	writeTarget(writer, action.target);
	writer.writeI32(action.rangeTiles);
}

bool SnapshotEntityCodec::readDestinationAction(SnapshotByteReader &reader, DestinationAction &action) const
{
	uint8_t type = 0;
	if (!reader.readU8(type) || type > static_cast<uint8_t>(DestinationActionType::Interact))
		return false;
	action.type = static_cast<DestinationActionType>(type);
	return readTarget(reader, action.target) && reader.readI32(action.rangeTiles);
}

void SnapshotEntityCodec::writeActorPosition(SnapshotByteWriter &writer, const ActorPosition &position) const
{
	writePoint(writer, position.tile);
	writePoint(writer, position.future);
	writePoint(writer, position.previous);
	writePoint(writer, position.precise);
}

bool SnapshotEntityCodec::readActorPosition(SnapshotByteReader &reader, ActorPosition &position) const
{
	return readPoint(reader, position.tile)
	    && readPoint(reader, position.future)
	    && readPoint(reader, position.previous)
	    && readPoint(reader, position.precise);
}

void SnapshotEntityCodec::writeItem(SnapshotByteWriter &writer, const Item &item) const
{
	writer.writeU32(item.id);
	writePoint(writer, item.tile);
	writer.writeU8(item.equipmentSlot.has_value() ? 1U : 0U);
	if (item.equipmentSlot.has_value())
		writer.writeU8(static_cast<uint8_t>(*item.equipmentSlot));
	writeEquipmentCombatModifiers(writer, item.combatModifiers);
}

bool SnapshotEntityCodec::readItem(SnapshotByteReader &reader, Item &item) const
{
	uint8_t hasSlot = 0;
	if (!reader.readU32(item.id) || !readPoint(reader, item.tile) || !reader.readU8(hasSlot) || hasSlot > 1U)
		return false;
	item.equipmentSlot = std::nullopt;
	if (hasSlot != 0U) {
		uint8_t slot = 0;
		if (!reader.readU8(slot) || !isValidEquipmentSlot(slot))
			return false;
		item.equipmentSlot = static_cast<EquipmentSlot>(slot);
	}
	return readEquipmentCombatModifiers(reader, item.combatModifiers);
}

void SnapshotEntityCodec::writeOptionalItem(SnapshotByteWriter &writer, const std::optional<Item> &item) const
{
	writer.writeU8(item.has_value() ? 1U : 0U);
	if (item.has_value())
		writeItem(writer, *item);
}

bool SnapshotEntityCodec::readOptionalItem(SnapshotByteReader &reader, std::optional<Item> &item) const
{
	uint8_t hasItem = 0;
	if (!reader.readU8(hasItem) || hasItem > 1U)
		return false;
	item = std::nullopt;
	if (hasItem == 0U)
		return true;

	Item decoded;
	if (!readItem(reader, decoded))
		return false;
	item = decoded;
	return true;
}

void SnapshotEntityCodec::writeCombatant(SnapshotByteWriter &writer, const Combatant &combatant) const
{
	writeTarget(writer, combatant.target);
	writeCombatStats(writer, combatant.stats);
}

bool SnapshotEntityCodec::readCombatant(SnapshotByteReader &reader, Combatant &combatant) const
{
	return readTarget(reader, combatant.target) && readCombatStats(reader, combatant.stats);
}

bool SnapshotEntityCodec::isValidEquipmentSlot(uint8_t slot) const
{
	return slot <= static_cast<uint8_t>(EquipmentSlot::Accessory);
}

} // namespace dev
