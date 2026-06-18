#pragma once

#include <vector>

#include "core/math/Vec2.hpp"
#include "modules/npc_ai/NpcAgentTickConfig.hpp"
#include "runtime/GameplayCommand2D.hpp"
#include "runtime/RuntimeLevelMutationStep.hpp"
#include "runtime/RuntimePlayerCommandExecutionStep.hpp"
#include "runtime/RuntimeSessionCommandTick.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/level/LevelTileMutation.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::runtime {

enum class RuntimeSessionMutationCommandStatus {
	Ticked,
	MutationFailed,
};

struct RuntimeSessionMutationCommandInput {
	RuntimeSessionState session;
	std::vector<LevelTileEdit> levelEdits;
	GameplayCommandFrame2D commandFrame;
	Vec2 fallbackPlayerPosition;
	RuntimePlayerCommandExecutionConfig playerCommandConfig;
	npc_ai::NpcAgentTickConfig npcConfig;
};

struct RuntimeSessionMutationCommandResult {
	RuntimeSessionMutationCommandStatus status = RuntimeSessionMutationCommandStatus::MutationFailed;
	RuntimeLevelMutationResult mutation;
	RuntimeSessionCommandTickResult commandTick;
	RuntimeSessionState session;
};

class RuntimeSessionMutationCommandStep {
public:
	[[nodiscard]] RuntimeSessionMutationCommandResult run(const RuntimeSessionMutationCommandInput &input) const;

	[[nodiscard]] RuntimeSessionMutationCommandResult run(
		const RuntimeSessionMutationCommandInput &input,
		const physics2d::CollisionWorld2D &explicitWorld) const;
};

} // namespace iggy::runtime
