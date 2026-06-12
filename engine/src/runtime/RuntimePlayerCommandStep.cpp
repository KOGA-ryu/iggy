#include "runtime/RuntimePlayerCommandStep.hpp"

namespace iggy::runtime {

RuntimePlayerCommandStepResult RuntimePlayerCommandStep::run(
	const RuntimeSessionState &session,
	const GameplayCommandFrame2D &frame,
	const physics2d::CollisionWorld2D &world,
	const RuntimePlayerCommandExecutionConfig &config) const
{
	RuntimePlayerCommandStepResult result;
	result.planning = RuntimePlayerCommandPlanningStep {}.plan(session, frame);

	if (result.planning.status == RuntimePlayerCommandPlanningStatus::MissingPlayer) {
		result.execution.status = RuntimePlayerCommandExecutionStatus::MissingPlayer;
		result.execution.session = session;
		result.session = session;
		return result;
	}

	result.execution = RuntimePlayerCommandExecutionStep {}.execute(session, result.planning.playerPlan, world, config);
	result.session = result.execution.session;
	return result;
}

} // namespace iggy::runtime
