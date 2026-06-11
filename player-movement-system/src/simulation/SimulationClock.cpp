#include "SimulationClock.hpp"

#include <algorithm>

namespace dev {

void SimulationClock::reset()
{
	timeScale_ = 1.0F;
	hitStopRemainingSeconds_ = 0.0F;
}

void SimulationClock::setTimeScale(float timeScale)
{
	timeScale_ = std::max(0.0F, timeScale);
}

void SimulationClock::triggerHitStop(float durationSeconds)
{
	hitStopRemainingSeconds_ = std::max(hitStopRemainingSeconds_, durationSeconds);
	hitStopRemainingSeconds_ = std::max(0.0F, hitStopRemainingSeconds_);
}

SimulationTimeStep SimulationClock::step(float rawDeltaSeconds)
{
	const float clampedRawDelta = std::max(0.0F, rawDeltaSeconds);
	const float hitStopConsumed = std::min(hitStopRemainingSeconds_, clampedRawDelta);
	hitStopRemainingSeconds_ -= hitStopConsumed;

	const float actorRawDelta = clampedRawDelta - hitStopConsumed;
	return SimulationTimeStep::fromActorDelta(clampedRawDelta, actorRawDelta * timeScale_);
}

float SimulationClock::timeScale() const
{
	return timeScale_;
}

float SimulationClock::hitStopRemainingSeconds() const
{
	return hitStopRemainingSeconds_;
}

} // namespace dev
