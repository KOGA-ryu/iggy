#pragma once

#include "combat/CombatEventSink.hpp"
#include "events/MovementEventSink.hpp"
#include "simulation/SimulationFrameEvents.hpp"
#include "simulation/SimulationWorld.hpp"

namespace dev {

class SimulationFrameEventCapture {
public:
	explicit SimulationFrameEventCapture(SimulationWorld &world);
	~SimulationFrameEventCapture();

	SimulationFrameEventCapture(const SimulationFrameEventCapture &) = delete;
	SimulationFrameEventCapture &operator=(const SimulationFrameEventCapture &) = delete;

	[[nodiscard]] SimulationFrameEvents &events();
	[[nodiscard]] const SimulationFrameEvents &events() const;
	void restore();

private:
	SimulationWorld &world_;
	MovementEventSink *previousMovementEvents_;
	CombatEventSink *previousCombatEvents_;
	SimulationFrameEvents events_;
	bool restored_ = false;
};

} // namespace dev
