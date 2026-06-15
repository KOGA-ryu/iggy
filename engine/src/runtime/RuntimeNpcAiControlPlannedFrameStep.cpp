#include "runtime/RuntimeNpcAiControlPlannedFrameStep.hpp"

namespace iggy::runtime {

bool RuntimeNpcAiControlPlannedFrameResult::ranControlRequests() const
{
	return status == RuntimeNpcAiControlPlannedFrameStatus::Ran;
}

bool RuntimeNpcAiControlPlannedFrameResult::changedState() const
{
	return changedControls;
}

RuntimeNpcAiControlPlannedFrameResult RuntimeNpcAiControlPlannedFrameStep::run(
	const RuntimeNpcAiControlPlannedFrameInput &input) const
{
	RuntimeNpcAiControlPlannedFrameResult result;
	result.inputState = input.state;
	result.subjects = input.subjects;
	result.pools = input.pools;
	result.map = input.map;
	result.config = input.config;
	result.plan = NpcMapPlayControlFramePlanner {}.plan(
		input.state.npcActors,
		input.state.npcControls,
		input.subjects,
		input.pools,
		input.map,
		input.config);
	result.step = NpcMapPlayControlFrameStepper2D {}.step(
		input.state.npcControls,
		result.plan.requests,
		input.config.step);

	result.state = input.state;
	result.state.npcControls = result.step.registry;
	result.plannedRequestCount = result.plan.requestCount;
	result.planIssueCount = result.plan.issues.size();
	result.appliedControlCount = result.step.appliedCount;
	result.failedControlCount = result.step.failedApplyCount;
	result.mapChangedSelectionCount = result.step.mapChangedSelectionCount;
	result.changedControls = result.step.changed();
	result.status = result.plannedRequestCount > 0
		? RuntimeNpcAiControlPlannedFrameStatus::Ran
		: RuntimeNpcAiControlPlannedFrameStatus::NoRequests;
	return result;
}

} // namespace iggy::runtime
