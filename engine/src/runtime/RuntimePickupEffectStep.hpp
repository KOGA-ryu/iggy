#pragma once

#include "runtime/RuntimeInventoryState.hpp"
#include "runtime/RuntimePickupStep.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/interaction/InteractionEffect2D.hpp"

namespace iggy::runtime {

enum class RuntimePickupEffectStatus {
	PickedUp,
	NotPickupEffect,
	InvalidEffect,
	PickupNotReady,
	MissingPlayer,
	TransferFailed,
};

struct RuntimePickupEffectResult {
	RuntimePickupEffectStatus status = RuntimePickupEffectStatus::NotPickupEffect;
	RuntimeInventoryState inventory;
	InteractionEffect2D effect;
	InteractionEffect2DStatus effectStatus = InteractionEffect2DStatus::Valid;
	RuntimePickupResult pickup;
	bool changed = false;
	InventoryEventRecorder2D events;
};

class RuntimePickupEffectStep {
public:
	[[nodiscard]] RuntimePickupEffectResult apply(
		const RuntimeSessionState &session,
		const RuntimeInventoryState &inventory,
		const InteractionEffect2D &effect,
		const RuntimePickupConfig &config = {}) const;
};

} // namespace iggy::runtime
