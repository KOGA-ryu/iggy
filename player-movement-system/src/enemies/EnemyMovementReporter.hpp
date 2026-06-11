#pragma once

#include "enemies/Enemy.hpp"
#include "enemies/EnemyAttackEventEmitter.hpp"
#include "enemies/EnemyAttackResult.hpp"
#include "enemies/EnemyPursuitEventEmitter.hpp"
#include "enemies/EnemyPursuitResult.hpp"
#include "events/MovementEventSink.hpp"

namespace dev {

class EnemyMovementReporter {
public:
	explicit EnemyMovementReporter(MovementEventSink *eventSink = nullptr);

	void reportAttack(const Enemy &enemy, const EnemyAttackResult &result) const;
	void reportPursuit(const Enemy &enemy, const EnemyPursuitResult &result) const;

private:
	EnemyAttackEventEmitter attackEvents_;
	EnemyPursuitEventEmitter pursuitEvents_;
};

} // namespace dev
