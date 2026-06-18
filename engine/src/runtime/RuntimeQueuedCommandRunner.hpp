#pragma once

#include "core/math/Vec2.hpp"
#include "modules/npc_ai/NpcAgentTickConfig.hpp"
#include "runtime/RuntimeCommandQueue.hpp"
#include "runtime/RuntimePlayerCommandExecutionStep.hpp"
#include "runtime/RuntimeSessionCommandTickRunner.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::runtime {

struct RuntimeQueuedCommandRunnerInput {
	RuntimeSessionState session;
	RuntimeCommandQueueState queue;
	Vec2 fallbackPlayerPosition;
	RuntimePlayerCommandExecutionConfig playerCommandConfig;
	npc_ai::NpcAgentTickConfig npcConfig;
};

struct RuntimeQueuedCommandRunnerResult {
	RuntimeSessionState session;
	RuntimeCommandQueueState queue;
	RuntimeCommandQueueDrainResult drained;
	RuntimeSessionCommandTickRunnerResult runner;
};

class RuntimeQueuedCommandRunner {
public:
	[[nodiscard]] RuntimeQueuedCommandRunnerResult run(const RuntimeQueuedCommandRunnerInput &input) const;

	[[nodiscard]] RuntimeQueuedCommandRunnerResult run(
		const RuntimeQueuedCommandRunnerInput &input,
		const physics2d::CollisionWorld2D &explicitWorld) const;
};

} // namespace iggy::runtime
