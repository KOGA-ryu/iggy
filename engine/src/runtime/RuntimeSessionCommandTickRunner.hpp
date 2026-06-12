#pragma once

#include <vector>

#include "core/math/Vec2.hpp"
#include "modules/npc_ai/NpcAgentController.hpp"
#include "runtime/GameplayCommand2D.hpp"
#include "runtime/RuntimePlayerCommandExecutionStep.hpp"
#include "runtime/RuntimeSessionCommandTick.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::runtime {

struct RuntimeSessionCommandTickRunnerInput {
	RuntimeSessionState session;
	std::vector<GameplayCommandFrame2D> commandFrames;
	Vec2 fallbackPlayerPosition;
	RuntimePlayerCommandExecutionConfig playerCommandConfig;
	npc_ai::NpcAgentTickConfig npcConfig;
};

struct RuntimeSessionCommandTickRunnerResult {
	RuntimeSessionState session;
	std::vector<RuntimeSessionCommandTickResult> ticks;
};

class RuntimeSessionCommandTickRunner {
public:
	[[nodiscard]] RuntimeSessionCommandTickRunnerResult run(const RuntimeSessionCommandTickRunnerInput &input) const;

	[[nodiscard]] RuntimeSessionCommandTickRunnerResult run(
		const RuntimeSessionCommandTickRunnerInput &input,
		const physics2d::CollisionWorld2D &collisionWorld) const;
};

} // namespace iggy::runtime
