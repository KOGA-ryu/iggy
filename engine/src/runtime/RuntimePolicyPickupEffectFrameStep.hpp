#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeInteractionEffectApplyFrameStep.hpp"
#include "runtime/RuntimeInventoryState.hpp"
#include "runtime/RuntimePolicyPickupEffectStep.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/interaction/InteractionEffectPlanApplier2D.hpp"

namespace iggy::runtime {

struct RuntimePolicyPickupEffectFrameEntry {
	std::size_t interactionIndex = 0;
	std::size_t effectIndex = 0;
	InteractionEffect2D effect;
	RuntimePolicyPickupEffectResult result;
};

enum class RuntimePolicyPickupEffectFrameStatus {
	PickedUp,
	NoPickupEffects,
	PickupNotReady,
	Failed,
};

struct RuntimePolicyPickupEffectFrameResult {
	RuntimePolicyPickupEffectFrameStatus status = RuntimePolicyPickupEffectFrameStatus::NoPickupEffects;
	RuntimeInventoryState inventory;
	std::vector<RuntimePolicyPickupEffectFrameEntry> entries;
	std::size_t pickedUpCount = 0;
	std::size_t notReadyCount = 0;
	std::size_t failedCount = 0;
	InventoryEventRecorder2D events;
	bool changed = false;
};

class RuntimePolicyPickupEffectFrameStep {
public:
	[[nodiscard]] RuntimePolicyPickupEffectFrameResult apply(
		const RuntimeSessionState &session,
		const RuntimeInventoryState &inventory,
		const ItemDefinition2DCatalog &catalog,
		const InteractionEffectPlanApplyResult &application,
		const RuntimePolicyPickupConfig &config = {}) const;

	[[nodiscard]] RuntimePolicyPickupEffectFrameResult apply(
		const RuntimeSessionState &session,
		const RuntimeInventoryState &inventory,
		const ItemDefinition2DCatalog &catalog,
		const RuntimeInteractionEffectApplyFrameResult &interactionApplication,
		const RuntimePolicyPickupConfig &config = {}) const;
};

} // namespace iggy::runtime
