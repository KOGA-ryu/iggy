#pragma once

#include <cstddef>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/ai/AiMapQuery2D.hpp"
#include "scene/ai/NpcHand.hpp"

namespace iggy {

struct NpcMapReadConfig {
	float minimumScore = 0.0F;
	float matchedMapTagBonus = 1.0F;
	float unmatchedMapTagPenalty = 0.0F;
	bool requireMapTagMatch = false;
};

struct NpcMapReadEnt {
	NpcHandEnt ent;
	float baseWeight = 0.0F;
	float score = 0.0F;
	std::size_t handIndex = 0;
	std::vector<ResourceId> matchedMapTags;
	std::vector<ResourceId> unmatchedMapTags;
	bool mapMatched = false;
};

struct NpcMapRead {
	NpcHand hand;
	AiMapQuery2DResult map;
	std::vector<NpcHandIssue> issues;
	std::vector<NpcMapReadEnt> rankedEnts;

	[[nodiscard]] bool hasRankedEnts() const;
	[[nodiscard]] bool hasIssues() const;
};

class NpcMapReader {
public:
	[[nodiscard]] NpcMapRead read(
		const NpcHand &hand,
		const AiMapQuery2DResult &map,
		const NpcMapReadConfig &config = {}) const;
};

} // namespace iggy
