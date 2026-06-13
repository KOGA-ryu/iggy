#pragma once

#include "runtime/GameplayCommand2D.hpp"
#include "runtime/RuntimeInteractionCommandStep.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/interaction/InteractionEffectCatalog2D.hpp"
#include "scene/interaction/InteractionEffectPlan2D.hpp"
#include "scene/interaction/InteractionReach2D.hpp"
#include "scene/interaction/InteractionTarget2D.hpp"

namespace iggy::runtime {

enum class RuntimeInteractionEffectCommandStatus {
	Ready,
	NoEffects,
	InteractionNotReady,
	NotInteractCommand,
	MissingPlayer,
};

struct RuntimeInteractionEffectCommandResult {
	RuntimeInteractionEffectCommandStatus status = RuntimeInteractionEffectCommandStatus::InteractionNotReady;
	RuntimeInteractionCommandResult interaction;
	InteractionEffectPlan2DResult effects;

	[[nodiscard]] bool ready() const;
};

class RuntimeInteractionEffectCommandStep {
public:
	[[nodiscard]] RuntimeInteractionEffectCommandResult evaluate(
		const RuntimeSessionState &session,
		const InteractionTarget2DRegistry &targets,
		const InteractionEffectCatalog2D &effects,
		const GameplayCommand2D &command,
		const InteractionReach2DConfig &reachConfig = {}) const;
};

} // namespace iggy::runtime
