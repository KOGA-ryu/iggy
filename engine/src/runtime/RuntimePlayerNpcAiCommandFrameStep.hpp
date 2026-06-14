#pragma once

#include "core/math/Vec2.hpp"
#include "modules/npc_ai/NpcAgentController.hpp"
#include "runtime/RuntimePlayerCommandExecutionStep.hpp"
#include "runtime/RuntimePlayerNpcAiQueueStep.hpp"
#include "runtime/RuntimeQueuedCommandRunner.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::runtime {

enum class RuntimePlayerNpcAiCommandFrameStatus {
	Ran,
	PlayerRejected,
	NpcAiRejected,
};

struct RuntimePlayerNpcAiCommandFrameInput {
	RuntimeSessionState session;
	RuntimePlayerNpcAiQueueInput intake;
	Vec2 fallbackPlayerPosition;
	RuntimePlayerCommandExecutionConfig playerCommandConfig;
	npc_ai::NpcAgentTickConfig npcConfig;
};

struct RuntimePlayerNpcAiCommandFrameResult {
	RuntimePlayerNpcAiCommandFrameStatus status = RuntimePlayerNpcAiCommandFrameStatus::Ran;
	RuntimePlayerNpcAiQueueResult intake;
	RuntimeQueuedCommandRunnerResult runner;
	RuntimeSessionState session;
	RuntimeCommandQueueState queue;
};

class RuntimePlayerNpcAiCommandFrameStep {
public:
	[[nodiscard]] RuntimePlayerNpcAiCommandFrameResult run(const RuntimePlayerNpcAiCommandFrameInput &input) const;

	[[nodiscard]] RuntimePlayerNpcAiCommandFrameResult run(
		const RuntimePlayerNpcAiCommandFrameInput &input,
		const physics2d::CollisionWorld2D &explicitWorld) const;
};

} // namespace iggy::runtime
