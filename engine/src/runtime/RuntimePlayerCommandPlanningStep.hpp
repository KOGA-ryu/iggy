#pragma once

#include "runtime/GameplayCommand2D.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/player/PlayerCommandFramePlanner2D.hpp"

namespace iggy::runtime {

enum class RuntimePlayerCommandPlanningStatus {
	MissingPlayer,
	Planned,
};

struct RuntimePlayerCommandPlanningResult {
	RuntimePlayerCommandPlanningStatus status = RuntimePlayerCommandPlanningStatus::MissingPlayer;
	PlayerCommandFramePlan2DResult playerPlan;
};

class RuntimePlayerCommandPlanningStep {
public:
	[[nodiscard]] RuntimePlayerCommandPlanningResult plan(const RuntimeSessionState &session, const GameplayCommandFrame2D &frame) const;
};

} // namespace iggy::runtime
