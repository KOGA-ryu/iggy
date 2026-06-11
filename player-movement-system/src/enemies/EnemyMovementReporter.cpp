#include "EnemyMovementReporter.hpp"

namespace dev {

EnemyMovementReporter::EnemyMovementReporter(MovementEventSink *eventSink)
    : attackEvents_(eventSink)
    , pursuitEvents_(eventSink)
{
}

void EnemyMovementReporter::reportAttack(const Enemy &enemy, const EnemyAttackResult &result) const
{
	attackEvents_.emit(enemy, result);
}

void EnemyMovementReporter::reportPursuit(const Enemy &enemy, const EnemyPursuitResult &result) const
{
	pursuitEvents_.emit(enemy, result);
}

} // namespace dev
