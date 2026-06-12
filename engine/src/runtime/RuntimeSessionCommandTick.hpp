#pragma once

#include "core/math/Vec2.hpp"
#include "modules/npc_ai/NpcAgentController.hpp"
#include "runtime/GameplayCommand2D.hpp"
#include "runtime/RuntimeCollisionWorldProvider.hpp"
#include "runtime/RuntimePlayerCommandStep.hpp"
#include "runtime/RuntimeSessionState.hpp"
#include "runtime/RuntimeSessionTick.hpp"
#include "servers/physics2d/CollisionWorld2D.hpp"

namespace iggy::runtime {

struct RuntimeSessionCommandTickInput {
	RuntimeSessionState session;
	GameplayCommandFrame2D commandFrame;
	Vec2 fallbackPlayerPosition;
	RuntimePlayerCommandExecutionConfig playerCommandConfig;
	npc_ai::NpcAgentTickConfig npcConfig;
};

struct RuntimeSessionCommandTickResult {
	RuntimePlayerCommandStepResult playerCommands;
	RuntimeSessionTickResult tick;
	RuntimeSessionState session;
	Vec2 npcTargetPosition;
};

class RuntimeSessionCommandTick {
public:
	[[nodiscard]] RuntimeSessionCommandTickResult run(const RuntimeSessionCommandTickInput &input) const;

	[[nodiscard]] RuntimeSessionCommandTickResult run(
		const RuntimeSessionCommandTickInput &input,
		const physics2d::CollisionWorld2D &collisionWorld) const;
};

} // namespace iggy::runtime
