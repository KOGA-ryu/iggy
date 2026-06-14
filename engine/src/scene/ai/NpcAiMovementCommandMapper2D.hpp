#pragma once

#include "runtime/GameplayCommand2D.hpp"
#include "scene/ai/NpcAiMovementProposal2D.hpp"

namespace iggy {

enum class NpcAiMovementCommandMapper2DStatus {
	Mapped,
	NoMovementProposal,
	MissingNpcId,
};

struct NpcAiMovementCommandMapper2DResult {
	NpcAiMovementCommandMapper2DStatus status = NpcAiMovementCommandMapper2DStatus::NoMovementProposal;
	NpcAiMovementProposal2DResult proposal;
	runtime::GameplayCommand2D command;

	[[nodiscard]] bool hasCommand() const;
};

class NpcAiMovementCommandMapper2D {
public:
	[[nodiscard]] NpcAiMovementCommandMapper2DResult map(const NpcAiMovementProposal2DResult &proposal) const;
};

} // namespace iggy
