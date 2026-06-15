#include "runtime/RuntimeNpcActorMovementRequestPlanStep.hpp"

namespace iggy::runtime {

bool RuntimeNpcActorMovementRequestPlanResult::hasRequests() const
{
	return requestCount > 0;
}

RuntimeNpcActorMovementRequestPlanResult RuntimeNpcActorMovementRequestPlanStep::plan(
	const RuntimeNpcActorMovementRequestPlanInput &input) const
{
	RuntimeNpcActorMovementRequestPlanResult result;
	result.state = input.state;
	result.map = input.map;
	result.plan = NpcActorMovementFramePlanner2D {}.plan(
		input.state.npcActors,
		input.state.npcControls,
		input.map,
		input.config);
	result.requests = result.plan.requests;
	result.requestCount = result.plan.requestCount;
	result.preparedCount = result.plan.preparedCount;
	result.blockedRequestCount = result.plan.blockedRequestCount;
	result.noMovementIntentCount = result.plan.noMovementIntentCount;
	result.routeFailedCount = result.plan.routeFailedCount;
	result.escapeRouteFailedCount = result.plan.escapeRouteFailedCount;
	result.navigationFailedCount = result.plan.navigationFailedCount;
	result.pathFailedCount = result.plan.pathFailedCount;
	result.stepNotProposedCount = result.plan.stepNotProposedCount;
	return result;
}

} // namespace iggy::runtime
