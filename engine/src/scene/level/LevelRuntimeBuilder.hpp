#pragma once

#include <vector>

#include "modules/blueprint/LevelBlueprintValidator.hpp"
#include "modules/npc_ai/NpcAgentBatch.hpp"
#include "modules/npc_ai/NpcSpawnBuilder.hpp"
#include "scene/level/LevelBlueprint.hpp"
#include "scene/level/LevelTileMap.hpp"

namespace iggy {

struct LevelRuntimeBuildResult {
	bool built = false;
	LevelBlueprintValidationReport validation;
	std::vector<npc_ai::NpcSpawnBuildIssue> spawnIssues;
	LevelTileMap tileMap;
	std::vector<npc_ai::NpcAgentEntry> npcAgents;
};

class LevelRuntimeBuilder {
public:
	[[nodiscard]] LevelRuntimeBuildResult build(const LevelBlueprint &blueprint) const;
};

} // namespace iggy
