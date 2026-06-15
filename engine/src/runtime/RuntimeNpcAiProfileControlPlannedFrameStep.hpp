#pragma once

#include <cstddef>

#include "runtime/RuntimeGameplayState.hpp"
#include "runtime/RuntimeNpcAiControlPlannedFrameStep.hpp"
#include "scene/ai/AiMap2D.hpp"
#include "scene/ai/NpcAiProfileTraitResolver.hpp"
#include "scene/ai/NpcMapPlayControlFramePlan.hpp"

namespace iggy::runtime {

enum class RuntimeNpcAiProfileControlPlannedFrameStatus {
	Ran,
	NoResolvedSubjects,
};

struct RuntimeNpcAiProfileControlPlannedFrameInput {
	RuntimeGameplayState state;
	NpcAiProfileTraitCatalog profileTraits;
	NpcMapPlayControlFramePlanPools pools;
	AiMap2D aiMap;
	NpcAiProfileTraitResolverConfig profileConfig;
	NpcMapPlayControlFramePlanConfig controlConfig;
};

struct RuntimeNpcAiProfileControlPlannedFrameResult {
	RuntimeNpcAiProfileControlPlannedFrameStatus status =
		RuntimeNpcAiProfileControlPlannedFrameStatus::NoResolvedSubjects;
	RuntimeNpcAiProfileControlPlannedFrameInput input;
	NpcAiProfileTraitResolveResult profileSubjects;
	RuntimeNpcAiControlPlannedFrameResult control;
	RuntimeGameplayState state;
	std::size_t resolvedSubjectCount = 0;
	std::size_t missingProfileCount = 0;
	std::size_t emptyActorProfileIdCount = 0;
	std::size_t absentSkippedCount = 0;
	std::size_t plannedRequestCount = 0;
	std::size_t planIssueCount = 0;
	std::size_t appliedControlCount = 0;
	std::size_t failedControlCount = 0;
	std::size_t mapChangedSelectionCount = 0;
	bool changedControls = false;

	[[nodiscard]] bool resolvedSubjects() const;
	[[nodiscard]] bool changedState() const;
};

class RuntimeNpcAiProfileControlPlannedFrameStep {
public:
	[[nodiscard]] RuntimeNpcAiProfileControlPlannedFrameResult run(
		const RuntimeNpcAiProfileControlPlannedFrameInput &input) const;
};

} // namespace iggy::runtime
