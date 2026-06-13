#pragma once

#include <vector>

#include "core/math/Vec2.hpp"
#include "core/resource/ResourceId.hpp"
#include "modules/npc_ai/NpcAgentController.hpp"
#include "runtime/RuntimePlayerInputQueueStep.hpp"
#include "runtime/RuntimeQueuedCommandRunner.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "scene/player/PlayerInputIntent2D.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::runtime {

enum class RuntimePlayerInputCommandRunnerStatus {
	Ran,
	QueueRejected,
};

struct RuntimePlayerInputCommandRunnerInput {
	RuntimeSessionState session;
	RuntimeCommandQueueState queue;
	RuntimeCommandQueueConfig queueConfig;
	ResourceId actorId;
	std::vector<PlayerInputIntent2D> intents;
	Vec2 fallbackPlayerPosition;
	RuntimePlayerCommandExecutionConfig playerCommandConfig;
	npc_ai::NpcAgentTickConfig npcConfig;
};

struct RuntimePlayerInputCommandRunnerResult {
	RuntimePlayerInputCommandRunnerStatus status = RuntimePlayerInputCommandRunnerStatus::Ran;
	RuntimePlayerInputQueueResult intake;
	RuntimeQueuedCommandRunnerResult runner;
	RuntimeSessionState session;
	RuntimeCommandQueueState queue;
};

class RuntimePlayerInputCommandRunner {
public:
	[[nodiscard]] RuntimePlayerInputCommandRunnerResult run(const RuntimePlayerInputCommandRunnerInput &input) const;

	[[nodiscard]] RuntimePlayerInputCommandRunnerResult run(
		const RuntimePlayerInputCommandRunnerInput &input,
		const physics2d::CollisionWorld2D &explicitWorld) const;
};

} // namespace iggy::runtime
