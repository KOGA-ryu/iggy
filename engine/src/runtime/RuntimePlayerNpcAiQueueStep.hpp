#pragma once

#include <vector>

#include "core/resource/ResourceId.hpp"
#include "runtime/RuntimeCommandQueue.hpp"
#include "runtime/RuntimeNpcAiDecisionQueueStep.hpp"
#include "runtime/RuntimePlayerInputQueueStep.hpp"
#include "scene/ai/AiMap2D.hpp"
#include "scene/level/LevelTileMap.hpp"
#include "scene/player/PlayerInputIntentGate2D.hpp"

namespace iggy::runtime {

enum class RuntimePlayerNpcAiQueueOrder {
	PlayerThenNpcAi,
	NpcAiThenPlayer,
};

enum class RuntimePlayerNpcAiQueueStatus {
	Queued,
	PlayerRejected,
	NpcAiRejected,
};

struct RuntimePlayerNpcAiQueueInput {
	RuntimeCommandQueueState queue;
	RuntimeCommandQueueConfig queueConfig;
	ResourceId playerActorId;
	PlayerInputContext2D playerInputContext;
	std::vector<PlayerInputIntent2D> playerIntents;
	AiMap2D aiMap;
	LevelTileMap levelMap;
	std::vector<RuntimeNpcAiDecisionQueueNpcInput> npcs;
	RuntimeNpcAiDecisionQueueConfig npcAi;
	RuntimePlayerNpcAiQueueOrder order = RuntimePlayerNpcAiQueueOrder::PlayerThenNpcAi;
};

struct RuntimePlayerNpcAiQueueResult {
	RuntimePlayerNpcAiQueueStatus status = RuntimePlayerNpcAiQueueStatus::Queued;
	RuntimePlayerNpcAiQueueOrder order = RuntimePlayerNpcAiQueueOrder::PlayerThenNpcAi;
	RuntimePlayerInputQueueGatedResult player;
	RuntimeNpcAiDecisionQueueResult npcAi;
	RuntimeCommandQueueState queue;
};

class RuntimePlayerNpcAiQueueStep {
public:
	[[nodiscard]] RuntimePlayerNpcAiQueueResult push(const RuntimePlayerNpcAiQueueInput &input) const;
};

} // namespace iggy::runtime
