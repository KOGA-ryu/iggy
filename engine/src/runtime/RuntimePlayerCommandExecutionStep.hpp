#pragma once

#include <cstddef>
#include <vector>

#include "runtime/RuntimeSessionState.hpp"
#include "scene/player/PlayerCommandFramePlanner2D.hpp"
#include "scene/player/PlayerMovementExecutor2D.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::runtime {

enum class RuntimePlayerCommandExecutionStatus {
	Executed,
	MissingPlayer,
	NoExecutablePlan,
};

struct RuntimePlayerCommandExecutionConfig {
	PlayerMovementExecutor2DConfig movement;
};

struct RuntimePlayerCommandExecutionResult {
	RuntimePlayerCommandExecutionStatus status = RuntimePlayerCommandExecutionStatus::NoExecutablePlan;
	RuntimeSessionState session;
	std::vector<PlayerMovementExecutionResult> movementResults;
	std::size_t executedPlanCount = 0;
};

class RuntimePlayerCommandExecutionStep {
public:
	[[nodiscard]] RuntimePlayerCommandExecutionResult execute(
		const RuntimeSessionState &session,
		const PlayerCommandFramePlan2DResult &planResult,
		const physics2d::CollisionWorld2D &world,
		const RuntimePlayerCommandExecutionConfig &config) const;
};

} // namespace iggy::runtime
