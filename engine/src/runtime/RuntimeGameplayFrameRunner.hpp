#pragma once

#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "modules/npc_ai/NpcAgentController.hpp"
#include "runtime/RuntimeCommandQueue.hpp"
#include "runtime/RuntimeGameplayFrameReport.hpp"
#include "runtime/RuntimeGameplayFrameStep.hpp"
#include "runtime/RuntimeGameplayState.hpp"
#include "runtime/RuntimePickupStep.hpp"
#include "runtime/RuntimePlayerCommandExecutionStep.hpp"
#include "scene/interaction/InteractionReach2D.hpp"
#include "scene/player/PlayerInputIntent2D.hpp"
#include "scene/player/PlayerInputIntentGate2D.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::runtime {

struct RuntimeGameplayFrameRunnerFrame {
	PlayerInputContext2D playerInputContext;
	ResourceId actorId;
	std::vector<PlayerInputIntent2D> playerIntents;
	RuntimeCommandQueueConfig commandQueueConfig;
	Vec2 fallbackPlayerPosition;
	RuntimePlayerCommandExecutionConfig playerCommandConfig;
	npc_ai::NpcAgentTickConfig npcConfig;
	InteractionReach2DConfig interactionReach;
	RuntimePickupConfig pickup;
};

struct RuntimeGameplayFrameRunnerInput {
	RuntimeGameplayState initialState;
	std::vector<RuntimeGameplayFrameRunnerFrame> frames;
};

struct RuntimeGameplayFrameRunnerTick {
	RuntimeGameplayFrameResult frame;
	RuntimeGameplayFrameReport report;
};

struct RuntimeGameplayFrameRunnerResult {
	RuntimeGameplayState finalState;
	std::vector<RuntimeGameplayFrameRunnerTick> ticks;
};

class RuntimeGameplayFrameRunner {
public:
	[[nodiscard]] RuntimeGameplayFrameRunnerResult run(const RuntimeGameplayFrameRunnerInput &input) const;

	[[nodiscard]] RuntimeGameplayFrameRunnerResult run(
		const RuntimeGameplayFrameRunnerInput &input,
		const physics2d::CollisionWorld2D &explicitWorld) const;
};

} // namespace iggy::runtime
