#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeInteractionEffectApplyFrameStep.hpp"
#include "runtime/RuntimeInventoryState.hpp"
#include "runtime/RuntimePickupEffectStep.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/interaction/InteractionEffectPlanApplier2D.hpp"

namespace iggy::runtime {

struct RuntimePickupEffectFrameEntry {
	std::size_t interactionIndex = 0;
	std::size_t effectIndex = 0;
	RuntimePickupEffectResult result;
};

enum class RuntimePickupEffectFrameStatus {
	PickedUp,
	NoPickupEffects,
	PickupNotReady,
	Failed,
};

struct RuntimePickupEffectFrameResult {
	RuntimePickupEffectFrameStatus status = RuntimePickupEffectFrameStatus::NoPickupEffects;
	RuntimeInventoryState inventory;
	std::vector<RuntimePickupEffectFrameEntry> entries;
	std::size_t pickedUpCount = 0;
	std::size_t notReadyCount = 0;
	std::size_t failedCount = 0;
	bool changed = false;
};

class RuntimePickupEffectFrameStep {
public:
	[[nodiscard]] RuntimePickupEffectFrameResult apply(
		const RuntimeSessionState &session,
		const RuntimeInventoryState &inventory,
		const InteractionEffectPlanApplyResult &application,
		const RuntimePickupConfig &config = {}) const;

	[[nodiscard]] RuntimePickupEffectFrameResult apply(
		const RuntimeSessionState &session,
		const RuntimeInventoryState &inventory,
		const RuntimeInteractionEffectApplyFrameResult &interactionApplication,
		const RuntimePickupConfig &config = {}) const;
};

} // namespace iggy::runtime
