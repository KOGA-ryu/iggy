#pragma once

#include "runtime/RuntimeCommandQueue.hpp"
#include "runtime/RuntimeInteractionCommandFrameStep.hpp"
#include "runtime/RuntimePlayerInputFrameStep.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/interaction/InteractionReach2D.hpp"
#include "scene/interaction/InteractionTarget2D.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::runtime {

struct RuntimePlayerInputInteractionFrameInput {
	RuntimePlayerInputGatedFrameStepInput playerInput;
	InteractionTarget2DRegistry interactionTargets;
	InteractionReach2DConfig interactionReach;
};

struct RuntimePlayerInputInteractionFrameResult {
	RuntimePlayerInputGatedFrameStepResult playerInput;
	RuntimeInteractionCommandFrameResult interactions;
	RuntimeSessionState session;
	RuntimeCommandQueueState queue;
};

class RuntimePlayerInputInteractionFrameStep {
public:
	[[nodiscard]] RuntimePlayerInputInteractionFrameResult run(const RuntimePlayerInputInteractionFrameInput &input) const;

	[[nodiscard]] RuntimePlayerInputInteractionFrameResult run(
		const RuntimePlayerInputInteractionFrameInput &input,
		const physics2d::CollisionWorld2D &explicitWorld) const;
};

} // namespace iggy::runtime
