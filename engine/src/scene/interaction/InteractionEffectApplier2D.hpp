#pragma once

#include "scene/interaction/InteractionEffect2D.hpp"
#include "scene/interaction/InteractionTarget2D.hpp"
#include "scene/interaction/InteractionTargetToggle2D.hpp"

namespace iggy {

enum class InteractionEffectApplyStatus {
	Applied,
	NoOp,
	InvalidEffect,
	TargetMissing,
	Deferred,
};

struct InteractionEffectApplyResult {
	InteractionEffectApplyStatus status = InteractionEffectApplyStatus::NoOp;
	InteractionTarget2DRegistry registry;
	InteractionEffect2D effect;
	InteractionEffect2DStatus effectStatus = InteractionEffect2DStatus::Valid;
	InteractionTargetToggle2DResult toggle;
	bool mutated = false;
};

class InteractionEffectApplier2D {
public:
	[[nodiscard]] InteractionEffectApplyResult apply(
		const InteractionTarget2DRegistry &registry,
		const InteractionEffect2D &effect) const;
};

} // namespace iggy
