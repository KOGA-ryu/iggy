#pragma once

#include <vector>

#include "core/resource/ResourceId.hpp"
#include "modules/npc_ai/NpcAgentBatch.hpp"
#include "scene/level/LevelBlueprint.hpp"

namespace iggy::npc_ai {

enum class NpcSpawnBuildIssueCode {
	NonNpcSpawnSkipped,
	OutOfBoundsSpawnSkipped,
};

struct NpcSpawnBuildIssue {
	NpcSpawnBuildIssueCode code;
	ResourceId spawnId;
	ResourceId type;
	int x = 0;
	int y = 0;
};

struct NpcSpawnBuildResult {
	std::vector<NpcAgentEntry> agents;
	std::vector<NpcSpawnBuildIssue> issues;
};

class NpcSpawnBuilder {
public:
	[[nodiscard]] NpcSpawnBuildResult build(const LevelBlueprint &blueprint) const;
};

} // namespace iggy::npc_ai
