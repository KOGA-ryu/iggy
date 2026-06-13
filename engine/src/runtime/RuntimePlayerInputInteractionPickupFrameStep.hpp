#pragma once

#include "runtime/RuntimeInventoryState.hpp"
#include "runtime/RuntimePickupEffectFrameStep.hpp"
#include "runtime/RuntimePlayerInputInteractionEffectApplyFrameStep.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::runtime {

struct RuntimePlayerInputInteractionPickupFrameInput {
	RuntimePlayerInputInteractionStateApplyFrameInput interactionInput;
	RuntimeInventoryState inventory;
	RuntimePickupConfig pickup;
};

struct RuntimePlayerInputInteractionPickupFrameResult {
	RuntimePlayerInputInteractionStateApplyFrameResult interaction;
	RuntimePickupEffectFrameResult pickup;
	RuntimeSessionState session;
	RuntimeCommandQueueState queue;
	RuntimeInteractionState interactionState;
	RuntimeInventoryState inventory;
};

class RuntimePlayerInputInteractionPickupFrameStep {
public:
	[[nodiscard]] RuntimePlayerInputInteractionPickupFrameResult run(
		const RuntimePlayerInputInteractionPickupFrameInput &input) const;

	[[nodiscard]] RuntimePlayerInputInteractionPickupFrameResult run(
		const RuntimePlayerInputInteractionPickupFrameInput &input,
		const physics2d::CollisionWorld2D &explicitWorld) const;
};

} // namespace iggy::runtime
