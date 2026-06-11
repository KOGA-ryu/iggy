#pragma once

#include "targeting/Target.hpp"
#include "world/Point.hpp"

namespace dev {

enum class EffectRequestType {
	Footstep,
	BlockedFeedback,
	ActionCue,
	DamageNumber,
	HitImpact,
	HitStop,
	DefeatCue,
};

struct EffectRequest {
	EffectRequestType type = EffectRequestType::ActionCue;
	Point tile;
	Target target;
	int damage = 0;
	float durationSeconds = 0.0F;
};

} // namespace dev
