#pragma once

#include "enemies/Enemy.hpp"
#include "enemies/EnemyPursuitResult.hpp"
#include "events/MovementEventSink.hpp"

namespace dev {

class EnemyPursuitEventEmitter {
public:
	explicit EnemyPursuitEventEmitter(MovementEventSink *eventSink = nullptr);

	void emit(const Enemy &enemy, const EnemyPursuitResult &result) const;

private:
	MovementEventSink *eventSink_;
};

} // namespace dev
