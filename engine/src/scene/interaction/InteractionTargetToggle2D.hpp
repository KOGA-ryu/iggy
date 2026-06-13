#pragma once

#include "core/resource/ResourceId.hpp"
#include "scene/interaction/InteractionTarget2D.hpp"

namespace iggy {

enum class InteractionTargetToggle2DStatus {
	Toggled,
	MissingTargetId,
	TargetNotFound,
	NoChange,
};

struct InteractionTargetToggle2DResult {
	InteractionTargetToggle2DStatus status = InteractionTargetToggle2DStatus::TargetNotFound;
	InteractionTarget2DRegistry registry;
	ResourceId targetId;
	bool requestedEnabled = true;
	bool changed = false;
};

class InteractionTargetToggle2D {
public:
	[[nodiscard]] InteractionTargetToggle2DResult apply(
		const InteractionTarget2DRegistry &registry,
		ResourceId targetId,
		bool enabledValue) const;
};

} // namespace iggy
