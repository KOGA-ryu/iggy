#pragma once

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "scene/interaction/InteractionReach2D.hpp"
#include "scene/interaction/InteractionTarget2D.hpp"
#include "scene/interaction/InteractionTargetQuery2D.hpp"

namespace iggy {

enum class InteractionPlan2DStatus {
	Ready,
	MissingTargetId,
	TargetNotFound,
	TargetDisabled,
	OutOfRange,
};

struct InteractionPlan2DResult {
	InteractionPlan2DStatus status = InteractionPlan2DStatus::TargetNotFound;
	ResourceId targetId;
	Vec2 actorPosition;
	InteractionTargetQuery2DResult query;
	InteractionReach2DResult reach;

	[[nodiscard]] bool ready() const;
};

class InteractionPlan2D {
public:
	[[nodiscard]] InteractionPlan2DResult plan(
		const InteractionTarget2DRegistry &registry,
		ResourceId targetId,
		Vec2 actorPosition,
		const InteractionReach2DConfig &reachConfig = {}) const;
};

} // namespace iggy
