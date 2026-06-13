#pragma once

#include <cstddef>
#include <vector>

#include "scene/interaction/InteractionEffectApplier2D.hpp"
#include "scene/interaction/InteractionEffectPlan2D.hpp"
#include "scene/interaction/InteractionTarget2D.hpp"

namespace iggy {

enum class InteractionEffectPlanApplyStatus {
	Applied,
	NoOp,
	InteractionNotReady,
	Failed,
};

struct InteractionEffectPlanApplyEntry {
	std::size_t effectIndex = 0;
	InteractionEffectApplyResult result;
};

struct InteractionEffectPlanApplyResult {
	InteractionEffectPlanApplyStatus status = InteractionEffectPlanApplyStatus::NoOp;
	InteractionTarget2DRegistry registry;
	InteractionEffectPlan2DResult plan;
	std::vector<InteractionEffectPlanApplyEntry> entries;
	std::size_t appliedCount = 0;
	std::size_t deferredCount = 0;
	std::size_t noOpCount = 0;
	std::size_t failedCount = 0;
	InteractionEventRecorder2D events;
	bool mutated = false;
};

class InteractionEffectPlanApplier2D {
public:
	[[nodiscard]] InteractionEffectPlanApplyResult apply(
		const InteractionTarget2DRegistry &registry,
		const InteractionEffectPlan2DResult &plan) const;
};

} // namespace iggy
