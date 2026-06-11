#include "SnapshotEnemyCodec.hpp"

namespace dev {

void SnapshotEnemyCodec::writeEnemy(SnapshotByteWriter &writer, const Enemy &enemy) const
{
	writer.writeU32(enemy.id);
	entityCodec_.writeActorPosition(writer, enemy.position);
	writer.writeU8(static_cast<uint8_t>(enemy.moveState));
	writer.writeI32(enemy.tuning.maxStepsPerTick);
	writer.writeI32(enemy.tuning.attackRangeTiles);
	writer.writeFloat(enemy.tuning.attackWindupSeconds);
	writer.writeFloat(enemy.tuning.attackRecoverySeconds);
	entityCodec_.writeCombatStats(writer, enemy.combatStats);
	writer.writeFloat(enemy.stateTimerSeconds);
}

bool SnapshotEnemyCodec::readEnemy(SnapshotByteReader &reader, Enemy &enemy) const
{
	uint8_t moveState = 0;
	if (!reader.readU32(enemy.id)
	    || !entityCodec_.readActorPosition(reader, enemy.position)
	    || !reader.readU8(moveState)
	    || moveState > static_cast<uint8_t>(EnemyMoveState::Recovering)
	    || !reader.readI32(enemy.tuning.maxStepsPerTick)
	    || !reader.readI32(enemy.tuning.attackRangeTiles)
	    || !reader.readFloat(enemy.tuning.attackWindupSeconds)
	    || !reader.readFloat(enemy.tuning.attackRecoverySeconds)
	    || !entityCodec_.readCombatStats(reader, enemy.combatStats)
	    || !reader.readFloat(enemy.stateTimerSeconds))
		return false;

	enemy.moveState = static_cast<EnemyMoveState>(moveState);
	return true;
}

} // namespace dev
