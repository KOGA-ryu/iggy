#pragma once

#include "runtime/RuntimeCommandQueue.hpp"
#include "runtime/RuntimeInteractionEffectCommandFrameStep.hpp"
#include "runtime/RuntimePlayerInputFrameStep.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/interaction/InteractionEffectCatalog2D.hpp"
#include "scene/interaction/InteractionReach2D.hpp"
#include "scene/interaction/InteractionTarget2D.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::runtime {

struct RuntimePlayerInputInteractionEffectFrameInput {
	RuntimePlayerInputGatedFrameStepInput playerInput;
	InteractionTarget2DRegistry interactionTargets;
	InteractionEffectCatalog2D interactionEffects;
	InteractionReach2DConfig interactionReach;
};

struct RuntimePlayerInputInteractionEffectFrameResult {
	RuntimePlayerInputGatedFrameStepResult playerInput;
	RuntimeInteractionEffectCommandFrameResult interactions;
	RuntimeSessionState session;
	RuntimeCommandQueueState queue;
};

class RuntimePlayerInputInteractionEffectFrameStep {
public:
	[[nodiscard]] RuntimePlayerInputInteractionEffectFrameResult run(const RuntimePlayerInputInteractionEffectFrameInput &input) const;

	[[nodiscard]] RuntimePlayerInputInteractionEffectFrameResult run(
		const RuntimePlayerInputInteractionEffectFrameInput &input,
		const physics2d::CollisionWorld2D &explicitWorld) const;
};

} // namespace iggy::runtime
