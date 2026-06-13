#pragma once

#include <vector>

#include "scene/interaction/InteractionEffect2D.hpp"
#include "scene/interaction/InteractionEffectCatalog2D.hpp"
#include "scene/interaction/InteractionPlan2D.hpp"

namespace iggy {

enum class InteractionEffectPlan2DStatus {
	Ready,
	InteractionNotReady,
	NoEffects,
};

struct InteractionEffectPlan2DResult {
	InteractionEffectPlan2DStatus status = InteractionEffectPlan2DStatus::InteractionNotReady;
	InteractionPlan2DResult interaction;
	std::vector<InteractionEffect2D> effects;

	[[nodiscard]] bool ready() const;
};

class InteractionEffectPlan2D {
public:
	[[nodiscard]] InteractionEffectPlan2DResult plan(
		const InteractionPlan2DResult &interaction,
		const InteractionEffectCatalog2D &catalog) const;
};

} // namespace iggy
