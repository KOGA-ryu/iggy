#include "runtime/RuntimePlayerCommandExecutionStep.hpp"

namespace iggy::runtime {

RuntimePlayerCommandExecutionResult RuntimePlayerCommandExecutionStep::execute(
	const RuntimeSessionState &session,
	const PlayerCommandFramePlan2DResult &planResult,
	const physics2d::CollisionWorld2D &world,
	const RuntimePlayerCommandExecutionConfig &config) const
{
	RuntimePlayerCommandExecutionResult result;
	result.session = session;

	if (!session.hasPlayer) {
		result.status = RuntimePlayerCommandExecutionStatus::MissingPlayer;
		return result;
	}

	const PlayerCommandPlan2D *movePlan = nullptr;
	for (const PlayerCommandPlan2D &plan : planResult.plans) {
		if (plan.type == PlayerCommandPlan2DType::MoveToPoint) {
			movePlan = &plan;
			break;
		}
	}

	if (movePlan == nullptr) {
		result.status = RuntimePlayerCommandExecutionStatus::NoExecutablePlan;
		return result;
	}

	PlayerMovementExecutionResult movement = PlayerMovementExecutor2D {}.execute(session.player, *movePlan, world, config.movement);
	result.movementResults.push_back(movement);
	result.executedPlanCount = 1;
	result.session.player = movement.state;
	result.status = RuntimePlayerCommandExecutionStatus::Executed;
	return result;
}

} // namespace iggy::runtime
