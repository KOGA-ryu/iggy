#include "runtime/RuntimePlayerCommandPlanningStep.hpp"

namespace iggy::runtime {

RuntimePlayerCommandPlanningResult RuntimePlayerCommandPlanningStep::plan(const RuntimeSessionState &session, const GameplayCommandFrame2D &frame) const
{
	RuntimePlayerCommandPlanningResult result;
	if (!session.hasPlayer)
		return result;

	result.status = RuntimePlayerCommandPlanningStatus::Planned;
	result.playerPlan = PlayerCommandFramePlanner2D {}.plan(session.player, frame);
	return result;
}

} // namespace iggy::runtime
