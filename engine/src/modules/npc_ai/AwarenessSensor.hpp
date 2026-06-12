#pragma once

#include "core/math/Vec2.hpp"
#include "modules/npc_ai/AwarenessState.hpp"
#include "scene/level/LevelTileMap.hpp"

namespace iggy::npc_ai {

struct AwarenessSensorConfig {
	float visionRange = 6.0F;
	int alertMemoryTicks = 0;
};

class AwarenessSensor {
public:
	explicit AwarenessSensor(AwarenessSensorConfig config = {});

	[[nodiscard]] AwarenessEvent observe(const LevelTileMap &map, Vec2 npcPosition, Vec2 playerPosition, AwarenessState &state) const;

private:
	AwarenessSensorConfig config_;
};

} // namespace iggy::npc_ai
