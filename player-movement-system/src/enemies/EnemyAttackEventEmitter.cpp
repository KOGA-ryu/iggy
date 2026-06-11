#include "EnemyAttackEventEmitter.hpp"

namespace dev {

EnemyAttackEventEmitter::EnemyAttackEventEmitter(MovementEventSink *eventSink)
    : eventSink_(eventSink)
{
}

void EnemyAttackEventEmitter::emit(const Enemy &enemy, const EnemyAttackResult &result) const
{
	if (eventSink_ == nullptr || result.transition == EnemyAttackTransition::None)
		return;

	eventSink_->emit({
	    .type = MovementEventType::EnemyAttackTransitioned,
	    .tile = enemy.position.tile,
	    .enemyId = enemy.id,
	    .enemyAttackTransition = result.transition,
	});
}

} // namespace dev
