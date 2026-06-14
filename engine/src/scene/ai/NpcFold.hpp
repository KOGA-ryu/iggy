#pragma once

#include <cstddef>
#include <limits>

#include "scene/ai/NpcTell.hpp"

namespace iggy {

enum class NpcFoldStatus {
	Kept,
	Folded,
};

enum class NpcFoldReason {
	None,
	NoPlayableEnt,
	BelowMinimumScore,
	TooManyIssues,
};

struct NpcFoldConfig {
	float minimumPlayScore = 0.0F;
	std::size_t maxIssueCount = std::numeric_limits<std::size_t>::max();
};

struct NpcFold {
	NpcTell tell;
	NpcFoldStatus status = NpcFoldStatus::Folded;
	NpcFoldReason reason = NpcFoldReason::NoPlayableEnt;

	[[nodiscard]] bool kept() const;
	[[nodiscard]] bool folded() const;
};

class NpcFolder {
public:
	[[nodiscard]] NpcFold fold(const NpcTell &tell, const NpcFoldConfig &config = {}) const;
};

} // namespace iggy
