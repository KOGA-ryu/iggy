#pragma once

#include <cstddef>
#include <vector>

#include "scene/npc/NpcPlayControlFrameApply2D.hpp"

namespace iggy {

enum class NpcPlayControlFrameEvent2D {
	ProposalApplied,
	ProposalFailed,
	ControlChanged,
	NoControlChanged,
	ProposalAppended,
	ProposalUpdated,
	DuplicateNpcProposalObserved,
};

struct NpcPlayControlFrameReport2D {
	NpcPlayControlFrameApply2DResult apply;
	NpcActorControlState2DRegistry registry;
	std::vector<NpcPlayControlFrameEvent2D> events;
	std::size_t proposalCount = 0;
	std::size_t appliedCount = 0;
	std::size_t failedCount = 0;
	std::size_t appendedCount = 0;
	std::size_t updatedCount = 0;
	std::size_t duplicateNpcProposalCount = 0;
	std::size_t noProposalCount = 0;
	std::size_t missingNpcIdCount = 0;
	std::size_t invalidObjectiveCount = 0;
	std::size_t invalidBehaviorCount = 0;
	bool changed = false;

	[[nodiscard]] bool hasEvents() const;
};

class NpcPlayControlFrameReporter2D {
public:
	[[nodiscard]] NpcPlayControlFrameReport2D report(
		const NpcPlayControlFrameApply2DResult &result) const;
};

} // namespace iggy
