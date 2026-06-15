#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeGameplayState.hpp"
#include "scene/ai/AiMap2D.hpp"
#include "scene/ai/NpcMapPlayControlFramePlan.hpp"
#include "scene/ai/NpcMapPlayControlFrameStep2D.hpp"

namespace iggy::runtime {

enum class RuntimeNpcAiControlPlannedFrameStatus {
	Ran,
	NoRequests,
};

struct RuntimeNpcAiControlPlannedFrameInput {
	RuntimeGameplayState state;
	std::vector<NpcMapPlayControlFramePlanSubject> subjects;
	NpcMapPlayControlFramePlanPools pools;
	AiMap2D map;
	NpcMapPlayControlFramePlanConfig config;
};

struct RuntimeNpcAiControlPlannedFrameResult {
	RuntimeNpcAiControlPlannedFrameStatus status = RuntimeNpcAiControlPlannedFrameStatus::NoRequests;
	RuntimeGameplayState inputState;
	std::vector<NpcMapPlayControlFramePlanSubject> subjects;
	NpcMapPlayControlFramePlanPools pools;
	AiMap2D map;
	NpcMapPlayControlFramePlanConfig config;
	NpcMapPlayControlFramePlanResult plan;
	NpcMapPlayControlFrameStep2DResult step;
	RuntimeGameplayState state;
	std::size_t plannedRequestCount = 0;
	std::size_t planIssueCount = 0;
	std::size_t appliedControlCount = 0;
	std::size_t failedControlCount = 0;
	std::size_t mapChangedSelectionCount = 0;
	bool changedControls = false;

	[[nodiscard]] bool ranControlRequests() const;
	[[nodiscard]] bool changedState() const;
};

class RuntimeNpcAiControlPlannedFrameStep {
public:
	[[nodiscard]] RuntimeNpcAiControlPlannedFrameResult run(
		const RuntimeNpcAiControlPlannedFrameInput &input) const;
};

} // namespace iggy::runtime
