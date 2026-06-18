#pragma once

#include "core/math/Vec2.hpp"
#include "modules/npc_ai/NpcAgentTickConfig.hpp"
#include "runtime/RuntimeMutationCommandQueue.hpp"
#include "runtime/RuntimePlayerCommandExecutionStep.hpp"
#include "runtime/RuntimeSessionMutationCommandRunner.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::runtime {

struct RuntimeQueuedMutationCommandRunnerInput {
	RuntimeSessionState session;
	RuntimeMutationCommandQueueState queue;
	Vec2 fallbackPlayerPosition;
	RuntimePlayerCommandExecutionConfig playerCommandConfig;
	npc_ai::NpcAgentTickConfig npcConfig;
};

struct RuntimeQueuedMutationCommandRunnerResult {
	RuntimeSessionState session;
	RuntimeMutationCommandQueueState queue;
	RuntimeMutationCommandQueueDrainResult drained;
	RuntimeSessionMutationCommandRunnerResult runner;
};

class RuntimeQueuedMutationCommandRunner {
public:
	[[nodiscard]] RuntimeQueuedMutationCommandRunnerResult run(const RuntimeQueuedMutationCommandRunnerInput &input) const;

	[[nodiscard]] RuntimeQueuedMutationCommandRunnerResult run(
		const RuntimeQueuedMutationCommandRunnerInput &input,
		const physics2d::CollisionWorld2D &explicitWorld) const;
};

} // namespace iggy::runtime
