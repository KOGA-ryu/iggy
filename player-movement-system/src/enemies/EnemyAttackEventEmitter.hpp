#pragma once

#include "enemies/Enemy.hpp"
#include "enemies/EnemyAttackResult.hpp"
#include "events/MovementEventSink.hpp"

namespace dev {

class EnemyAttackEventEmitter {
public:
	explicit EnemyAttackEventEmitter(MovementEventSink *eventSink = nullptr);

	void emit(const Enemy &enemy, const EnemyAttackResult &result) const;

private:
	MovementEventSink *eventSink_;
};

} // namespace dev
