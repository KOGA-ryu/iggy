#pragma once

#include "runtime/RuntimeInventoryState.hpp"
#include "runtime/RuntimePlayerInputInteractionEffectApplyFrameStep.hpp"
#include "runtime/RuntimePolicyPickupEffectFrameStep.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::runtime {

struct RuntimePlayerInputInteractionPolicyPickupFrameInput {
	RuntimePlayerInputInteractionStateApplyFrameInput interactionInput;
	RuntimeInventoryState inventory;
	ItemDefinition2DCatalog itemDefinitions;
	RuntimePolicyPickupConfig pickup;
};

struct RuntimePlayerInputInteractionPolicyPickupFrameResult {
	RuntimePlayerInputInteractionStateApplyFrameResult interaction;
	RuntimePolicyPickupEffectFrameResult pickup;
	RuntimeSessionState session;
	RuntimeCommandQueueState queue;
	RuntimeInteractionState interactionState;
	RuntimeInventoryState inventory;
	InventoryEventRecorder2D inventoryEvents;
};

class RuntimePlayerInputInteractionPolicyPickupFrameStep {
public:
	[[nodiscard]] RuntimePlayerInputInteractionPolicyPickupFrameResult run(
		const RuntimePlayerInputInteractionPolicyPickupFrameInput &input) const;

	[[nodiscard]] RuntimePlayerInputInteractionPolicyPickupFrameResult run(
		const RuntimePlayerInputInteractionPolicyPickupFrameInput &input,
		const physics2d::CollisionWorld2D &explicitWorld) const;
};

} // namespace iggy::runtime
