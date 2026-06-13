#include "scene/interaction/InteractionTargetToggle2D.hpp"

namespace iggy {

InteractionTargetToggle2DResult InteractionTargetToggle2D::apply(
	const InteractionTarget2DRegistry &registry,
	ResourceId targetId,
	bool enabledValue) const
{
	InteractionTargetToggle2DResult result;
	result.registry = registry;
	result.targetId = targetId;
	result.requestedEnabled = enabledValue;

	if (targetId.empty()) {
		result.status = InteractionTargetToggle2DStatus::MissingTargetId;
		return result;
	}

	std::vector<InteractionTarget2D> targets = registry.targets();
	for (InteractionTarget2D &target : targets) {
		if (target.id != targetId)
			continue;

		if (target.enabled == enabledValue) {
			result.status = InteractionTargetToggle2DStatus::NoChange;
			return result;
		}

		target.enabled = enabledValue;
		result.status = InteractionTargetToggle2DStatus::Toggled;
		result.registry = InteractionTarget2DRegistry { targets };
		result.changed = true;
		return result;
	}

	result.status = InteractionTargetToggle2DStatus::TargetNotFound;
	return result;
}

} // namespace iggy
