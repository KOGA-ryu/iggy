#pragma once

#include <cstdint>
#include <optional>

#include "combat/CombatStats.hpp"
#include "combat/Combatant.hpp"
#include "items/Item.hpp"
#include "interaction/DestinationAction.hpp"
#include "player/ActorPosition.hpp"
#include "save/SnapshotByteStream.hpp"
#include "targeting/Target.hpp"
#include "world/Point.hpp"

namespace dev {

class SnapshotEntityCodec {
public:
	void writePoint(SnapshotByteWriter &writer, Point point) const;
	[[nodiscard]] bool readPoint(SnapshotByteReader &reader, Point &point) const;

	void writeTarget(SnapshotByteWriter &writer, const Target &target) const;
	[[nodiscard]] bool readTarget(SnapshotByteReader &reader, Target &target) const;

	void writeCombatStats(SnapshotByteWriter &writer, const CombatStats &stats) const;
	[[nodiscard]] bool readCombatStats(SnapshotByteReader &reader, CombatStats &stats) const;

	void writeEquipmentCombatModifiers(SnapshotByteWriter &writer, const EquipmentCombatModifiers &modifiers) const;
	[[nodiscard]] bool readEquipmentCombatModifiers(SnapshotByteReader &reader, EquipmentCombatModifiers &modifiers) const;

	void writeDestinationAction(SnapshotByteWriter &writer, const DestinationAction &action) const;
	[[nodiscard]] bool readDestinationAction(SnapshotByteReader &reader, DestinationAction &action) const;

	void writeActorPosition(SnapshotByteWriter &writer, const ActorPosition &position) const;
	[[nodiscard]] bool readActorPosition(SnapshotByteReader &reader, ActorPosition &position) const;

	void writeItem(SnapshotByteWriter &writer, const Item &item) const;
	[[nodiscard]] bool readItem(SnapshotByteReader &reader, Item &item) const;

	void writeOptionalItem(SnapshotByteWriter &writer, const std::optional<Item> &item) const;
	[[nodiscard]] bool readOptionalItem(SnapshotByteReader &reader, std::optional<Item> &item) const;

	void writeCombatant(SnapshotByteWriter &writer, const Combatant &combatant) const;
	[[nodiscard]] bool readCombatant(SnapshotByteReader &reader, Combatant &combatant) const;

private:
	[[nodiscard]] bool isValidEquipmentSlot(uint8_t slot) const;
};

} // namespace dev
