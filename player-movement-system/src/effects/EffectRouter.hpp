#pragma once

#include "combat/CombatEvent.hpp"
#include "effects/EffectSink.hpp"
#include "events/MovementEvent.hpp"

namespace dev {

class EffectRouter {
public:
	explicit EffectRouter(EffectSink &sink);

	void route(const MovementEvent &event) const;
	void route(const CombatEvent &event) const;

private:
	void emit(EffectRequest request) const;

	EffectSink &sink_;
};

} // namespace dev
