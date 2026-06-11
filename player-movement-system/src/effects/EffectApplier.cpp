#include "EffectApplier.hpp"

namespace dev {

EffectApplier::EffectApplier(SimulationClock *clock)
    : clock_(clock)
{
}

void EffectApplier::apply(const EffectRequest &request) const
{
	if (request.type != EffectRequestType::HitStop || clock_ == nullptr)
		return;
	clock_->triggerHitStop(request.durationSeconds);
}

} // namespace dev
