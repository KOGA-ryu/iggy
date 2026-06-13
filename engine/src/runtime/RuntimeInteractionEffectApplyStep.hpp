#pragma once

#include "runtime/GameplayCommand2D.hpp"
#include "runtime/RuntimeInteractionEffectCommandStep.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/interaction/InteractionEffectCatalog2D.hpp"
#include "scene/interaction/InteractionEffectPlanApplier2D.hpp"
#include "scene/interaction/InteractionReach2D.hpp"
#include "scene/interaction/InteractionTarget2D.hpp"

namespace iggy::runtime {

enum class RuntimeInteractionEffectApplyStatus {
	Applied,
	NoOp,
	InteractionNotReady,
	Failed,
};

struct RuntimeInteractionEffectApplyResult {
	RuntimeInteractionEffectApplyStatus status = RuntimeInteractionEffectApplyStatus::InteractionNotReady;
	RuntimeInteractionEffectCommandResult command;
	InteractionEffectPlanApplyResult application;
	InteractionTarget2DRegistry registry;
	bool mutated = false;
};

class RuntimeInteractionEffectApplyStep {
public:
	[[nodiscard]] RuntimeInteractionEffectApplyResult apply(
		const RuntimeSessionState &session,
		const InteractionTarget2DRegistry &targets,
		const InteractionEffectCatalog2D &effects,
		const GameplayCommand2D &command,
		const InteractionReach2DConfig &reachConfig = {}) const;
};

} // namespace iggy::runtime
