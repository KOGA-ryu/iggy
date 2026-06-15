#include "runtime/RuntimeNpcAiProfileControlPlannedFrameStep.hpp"

namespace iggy::runtime {

bool RuntimeNpcAiProfileControlPlannedFrameResult::resolvedSubjects() const
{
	return status == RuntimeNpcAiProfileControlPlannedFrameStatus::Ran;
}

bool RuntimeNpcAiProfileControlPlannedFrameResult::changedState() const
{
	return changedControls;
}

RuntimeNpcAiProfileControlPlannedFrameResult RuntimeNpcAiProfileControlPlannedFrameStep::run(
	const RuntimeNpcAiProfileControlPlannedFrameInput &input) const
{
	RuntimeNpcAiProfileControlPlannedFrameResult result;
	result.input = input;
	result.profileSubjects = NpcAiProfileTraitResolver {}.resolve(
		input.state.npcActors,
		input.profileTraits,
		input.profileConfig);

	result.control = RuntimeNpcAiControlPlannedFrameStep {}.run({
		input.state,
		result.profileSubjects.subjects,
		input.pools,
		input.aiMap,
		input.controlConfig,
	});

	result.state = result.control.state;
	result.resolvedSubjectCount = result.profileSubjects.resolvedCount;
	result.missingProfileCount = result.profileSubjects.missingProfileCount;
	result.emptyActorProfileIdCount = result.profileSubjects.emptyActorProfileIdCount;
	result.absentSkippedCount = result.profileSubjects.absentSkippedCount;
	result.plannedRequestCount = result.control.plannedRequestCount;
	result.planIssueCount = result.control.planIssueCount;
	result.appliedControlCount = result.control.appliedControlCount;
	result.failedControlCount = result.control.failedControlCount;
	result.mapChangedSelectionCount = result.control.mapChangedSelectionCount;
	result.changedControls = result.control.changedControls;
	result.status = result.profileSubjects.hasSubjects()
		? RuntimeNpcAiProfileControlPlannedFrameStatus::Ran
		: RuntimeNpcAiProfileControlPlannedFrameStatus::NoResolvedSubjects;
	return result;
}

} // namespace iggy::runtime
