#pragma once

#include "core/math/Vec2.hpp"
#include "scene/interaction/InteractionTargetQuery2D.hpp"

namespace iggy {

enum class InteractionReach2DStatus {
	Reachable,
	MissingTargetId,
	TargetNotFound,
	TargetDisabled,
	OutOfRange,
};

struct InteractionReach2DConfig {
	float extraReach = 0.0F;
};

struct InteractionReach2DResult {
	InteractionReach2DStatus status = InteractionReach2DStatus::TargetNotFound;
	InteractionTargetQuery2DResult query;
	Vec2 actorPosition;
	float distance = 0.0F;
	float allowedDistance = 0.0F;

	[[nodiscard]] bool reachable() const;
};

class InteractionReach2D {
public:
	[[nodiscard]] InteractionReach2DResult evaluate(
		Vec2 actorPosition,
		const InteractionTargetQuery2DResult &query,
		const InteractionReach2DConfig &config = {}) const;
};

} // namespace iggy
