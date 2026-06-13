#pragma once

#include "runtime/RuntimeCommandQueue.hpp"
#include "runtime/RuntimeInteractionEffectApplyFrameStep.hpp"
#include "runtime/RuntimePlayerInputFrameStep.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/interaction/InteractionEffectCatalog2D.hpp"
#include "scene/interaction/InteractionReach2D.hpp"
#include "scene/interaction/InteractionTarget2D.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::runtime {

struct RuntimePlayerInputInteractionEffectApplyFrameInput {
	RuntimePlayerInputGatedFrameStepInput playerInput;
	InteractionTarget2DRegistry interactionTargets;
	InteractionEffectCatalog2D interactionEffects;
	InteractionReach2DConfig interactionReach;
};

struct RuntimePlayerInputInteractionEffectApplyFrameResult {
	RuntimePlayerInputGatedFrameStepResult playerInput;
	RuntimeInteractionEffectApplyFrameResult application;
	RuntimeSessionState session;
	RuntimeCommandQueueState queue;
	InteractionTarget2DRegistry interactionTargets;
};

class RuntimePlayerInputInteractionEffectApplyFrameStep {
public:
	[[nodiscard]] RuntimePlayerInputInteractionEffectApplyFrameResult run(
		const RuntimePlayerInputInteractionEffectApplyFrameInput &input) const;

	[[nodiscard]] RuntimePlayerInputInteractionEffectApplyFrameResult run(
		const RuntimePlayerInputInteractionEffectApplyFrameInput &input,
		const physics2d::CollisionWorld2D &explicitWorld) const;
};

} // namespace iggy::runtime
