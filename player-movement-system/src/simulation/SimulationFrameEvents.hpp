#pragma once

#include <vector>

#include "combat/CombatEventSink.hpp"
#include "effects/EffectSink.hpp"
#include "events/MovementEventSink.hpp"

namespace dev {

class SimulationFrameEvents : public MovementEventSink, public CombatEventSink, public EffectSink {
public:
	SimulationFrameEvents(MovementEventSink *movementForward = nullptr, CombatEventSink *combatForward = nullptr);

	void emit(const MovementEvent &event) override;
	void emit(const CombatEvent &event) override;
	void emit(const EffectRequest &request) override;
	void clear();

	[[nodiscard]] const std::vector<MovementEvent> &movementEvents() const;
	[[nodiscard]] const std::vector<CombatEvent> &combatEvents() const;
	[[nodiscard]] const std::vector<EffectRequest> &effectRequests() const;

private:
	MovementEventSink *movementForward_;
	CombatEventSink *combatForward_;
	std::vector<MovementEvent> movementEvents_;
	std::vector<CombatEvent> combatEvents_;
	std::vector<EffectRequest> effectRequests_;
};

} // namespace dev
