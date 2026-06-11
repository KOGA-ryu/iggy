#include "SnapshotSchemaCodec.hpp"

namespace dev {

void SnapshotSchemaCodec::writeSnapshot(SnapshotByteWriter &writer, const SimulationSnapshot &snapshot) const
{
	vectorCodec_.writeVector(writer, snapshot.players, [&](SnapshotByteWriter &itemWriter, const Player &player) {
		playerCodec_.writePlayer(itemWriter, player);
	});
	vectorCodec_.writeVector(writer, snapshot.enemies, [&](SnapshotByteWriter &itemWriter, const Enemy &enemy) {
		enemyCodec_.writeEnemy(itemWriter, enemy);
	});
	vectorCodec_.writeVector(writer, snapshot.items, [&](SnapshotByteWriter &itemWriter, const Item &item) {
		entityCodec_.writeItem(itemWriter, item);
	});
	vectorCodec_.writeVector(writer, snapshot.combatants, [&](SnapshotByteWriter &itemWriter, const Combatant &combatant) {
		entityCodec_.writeCombatant(itemWriter, combatant);
	});
	vectorCodec_.writeVector(writer, snapshot.targets, [&](SnapshotByteWriter &itemWriter, const Target &target) {
		entityCodec_.writeTarget(itemWriter, target);
	});
}

bool SnapshotSchemaCodec::readSnapshot(SnapshotByteReader &reader, SimulationSnapshot &snapshot) const
{
	return vectorCodec_.readVector(reader, snapshot.players, [&](SnapshotByteReader &itemReader, Player &player) {
		return playerCodec_.readPlayer(itemReader, player);
	}) && vectorCodec_.readVector(reader, snapshot.enemies, [&](SnapshotByteReader &itemReader, Enemy &enemy) {
		return enemyCodec_.readEnemy(itemReader, enemy);
	}) && vectorCodec_.readVector(reader, snapshot.items, [&](SnapshotByteReader &itemReader, Item &item) {
		return entityCodec_.readItem(itemReader, item);
	}) && vectorCodec_.readVector(reader, snapshot.combatants, [&](SnapshotByteReader &itemReader, Combatant &combatant) {
		return entityCodec_.readCombatant(itemReader, combatant);
	}) && vectorCodec_.readVector(reader, snapshot.targets, [&](SnapshotByteReader &itemReader, Target &target) {
		return entityCodec_.readTarget(itemReader, target);
	}) && reader.consumed();
}

} // namespace dev
