#pragma once

#include "core/resource/ResourceId.hpp"
#include "scene/interaction/InteractionTarget2D.hpp"

namespace iggy {

enum class InteractionTargetQuery2DStatus {
	Found,
	MissingTargetId,
	NotFound,
	Disabled,
};

struct InteractionTargetQuery2DResult {
	InteractionTargetQuery2DStatus status = InteractionTargetQuery2DStatus::NotFound;
	ResourceId targetId;
	InteractionTarget2D target;

	[[nodiscard]] bool hasTarget() const;
};

class InteractionTargetQuery2D {
public:
	[[nodiscard]] InteractionTargetQuery2DResult find(const InteractionTarget2DRegistry &registry, ResourceId targetId) const;
};

} // namespace iggy
