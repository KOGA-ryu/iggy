#pragma once

#include <cstddef>
#include <vector>

#include "scene/ai/NpcPlay.hpp"

namespace iggy {

enum class NpcTellLineOutcome {
	Played,
	Ranked,
	InvalidDraw,
};

struct NpcTellLine {
	NpcTellLineOutcome outcome = NpcTellLineOutcome::Ranked;
	NpcHandTraitSource source = NpcHandTraitSource::Strength;
	ResourceId actionTag;
	NpcBehaviorStateType behaviorState = NpcBehaviorStateType::None;
	float score = 0.0F;
	std::size_t handIndex = 0;
	std::size_t drawEntryIndex = 0;
	std::size_t issueIndex = 0;
};

struct NpcTell {
	NpcPlay play;
	std::size_t handEntCount = 0;
	std::size_t rankedEntCount = 0;
	std::size_t issueCount = 0;
	std::size_t playedCount = 0;
	std::vector<NpcTellLine> lines;

	[[nodiscard]] bool hasLines() const;
};

class NpcTeller {
public:
	[[nodiscard]] NpcTell tell(const NpcPlay &play) const;
};

} // namespace iggy
