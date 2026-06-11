#include "EnemyPursuitEventEmitter.hpp"

namespace dev {

EnemyPursuitEventEmitter::EnemyPursuitEventEmitter(MovementEventSink *eventSink)
    : eventSink_(eventSink)
{
}

void EnemyPursuitEventEmitter::emit(const Enemy &enemy, const EnemyPursuitResult &result) const
{
	if (eventSink_ == nullptr)
		return;

	eventSink_->emit({
	    .type = MovementEventType::EnemyPursuitStopped,
	    .tile = enemy.position.tile,
	    .enemyId = enemy.id,
	    .enemyPursuitStopReason = result.stopReason,
	    .enemyPursuitStepsCommitted = result.stepsCommitted,
	});
}

} // namespace dev
