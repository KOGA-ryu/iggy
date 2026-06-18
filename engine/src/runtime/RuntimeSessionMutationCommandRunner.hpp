#pragma once

#include <vector>

#include "core/math/Vec2.hpp"
#include "modules/npc_ai/NpcAgentTickConfig.hpp"
#include "runtime/GameplayCommand2D.hpp"
#include "runtime/RuntimePlayerCommandExecutionStep.hpp"
#include "runtime/RuntimeSessionMutationCommandStep.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/level/LevelTileMutation.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::runtime {

struct RuntimeSessionMutationCommandFrame {
	std::vector<LevelTileEdit> levelEdits;
	GameplayCommandFrame2D commandFrame;
};

struct RuntimeSessionMutationCommandRunnerInput {
	RuntimeSessionState session;
	std::vector<RuntimeSessionMutationCommandFrame> frames;
	Vec2 fallbackPlayerPosition;
	RuntimePlayerCommandExecutionConfig playerCommandConfig;
	npc_ai::NpcAgentTickConfig npcConfig;
};

struct RuntimeSessionMutationCommandRunnerResult {
	RuntimeSessionState session;
	std::vector<RuntimeSessionMutationCommandResult> ticks;
};

class RuntimeSessionMutationCommandRunner {
public:
	[[nodiscard]] RuntimeSessionMutationCommandRunnerResult run(const RuntimeSessionMutationCommandRunnerInput &input) const;

	[[nodiscard]] RuntimeSessionMutationCommandRunnerResult run(
		const RuntimeSessionMutationCommandRunnerInput &input,
		const physics2d::CollisionWorld2D &explicitWorld) const;
};

} // namespace iggy::runtime
