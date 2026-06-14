#pragma once

#include <cstddef>
#include <vector>

#include "scene/ai/NpcHand.hpp"

namespace iggy {

struct NpcReadConfig {
	float minimumScore = 0.0F;
};

struct NpcReadEnt {
	NpcHandEnt ent;
	float score = 0.0F;
	std::size_t handIndex = 0;
};

struct NpcRead {
	NpcHand hand;
	std::vector<NpcReadEnt> rankedEnts;
	std::vector<NpcHandIssue> issues;

	[[nodiscard]] bool hasRankedEnts() const;
	[[nodiscard]] bool hasIssues() const;
};

class NpcReader {
public:
	[[nodiscard]] NpcRead read(const NpcHand &hand, const NpcReadConfig &config = {}) const;
};

} // namespace iggy
