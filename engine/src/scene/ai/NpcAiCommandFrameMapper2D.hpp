#pragma once

#include <cstddef>
#include <vector>

#include "runtime/GameplayCommand2D.hpp"
#include "scene/ai/NpcAiMovementCommandMapper2D.hpp"
#include "scene/ai/NpcAiMovementProposal2D.hpp"

namespace iggy {

struct NpcAiCommandFrameMapIssue2D {
	std::size_t proposalIndex = 0;
	NpcAiMovementProposal2DResult proposal;
	NpcAiMovementCommandMapper2DResult map;
};

struct NpcAiCommandFrameMapper2DResult {
	runtime::GameplayCommandFrame2D frame;
	std::vector<NpcAiCommandFrameMapIssue2D> issues;

	[[nodiscard]] bool hasIssues() const;
};

class NpcAiCommandFrameMapper2D {
public:
	[[nodiscard]] NpcAiCommandFrameMapper2DResult map(const std::vector<NpcAiMovementProposal2DResult> &proposals) const;
};

} // namespace iggy
