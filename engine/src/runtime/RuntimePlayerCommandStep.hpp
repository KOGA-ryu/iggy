#pragma once

#include "runtime/GameplayCommand2D.hpp"
#include "runtime/RuntimePlayerCommandExecutionStep.hpp"
#include "runtime/RuntimePlayerCommandPlanningStep.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::runtime {

struct RuntimePlayerCommandStepResult {
	RuntimePlayerCommandPlanningResult planning;
	RuntimePlayerCommandExecutionResult execution;
	RuntimeSessionState session;
};

class RuntimePlayerCommandStep {
public:
	[[nodiscard]] RuntimePlayerCommandStepResult run(
		const RuntimeSessionState &session,
		const GameplayCommandFrame2D &frame,
		const physics2d::CollisionWorld2D &world,
		const RuntimePlayerCommandExecutionConfig &config) const;
};

} // namespace iggy::runtime
