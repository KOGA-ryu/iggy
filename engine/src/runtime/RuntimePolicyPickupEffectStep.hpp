#pragma once

#include "runtime/RuntimeInventoryState.hpp"
#include "runtime/RuntimePolicyPickupStep.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/interaction/InteractionEffect2D.hpp"

namespace iggy::runtime {

enum class RuntimePolicyPickupEffectStatus {
	PickedUp,
	NotPickupEffect,
	InvalidEffect,
	MissingPlayer,
	PickupNotReady,
	TransferFailed,
};

struct RuntimePolicyPickupEffectResult {
	RuntimePolicyPickupEffectStatus status = RuntimePolicyPickupEffectStatus::NotPickupEffect;
	InteractionEffect2D effect;
	InteractionEffect2DStatus effectStatus = InteractionEffect2DStatus::Valid;
	RuntimePolicyPickupResult pickup;
	RuntimeInventoryState inventory;
	InventoryEventRecorder2D events;
	bool changed = false;
};

class RuntimePolicyPickupEffectStep {
public:
	[[nodiscard]] RuntimePolicyPickupEffectResult apply(
		const RuntimeSessionState &session,
		const RuntimeInventoryState &inventory,
		const ItemDefinition2DCatalog &catalog,
		const InteractionEffect2D &effect,
		const RuntimePolicyPickupConfig &config = {}) const;
};

} // namespace iggy::runtime
