#pragma once

#include <cstddef>
#include <vector>

#include "core/resource/ResourceId.hpp"
#include "scene/ai/NpcPlayControlProposal.hpp"
#include "scene/npc/NpcActorControlState2D.hpp"
#include "scene/npc/NpcPlayControlApply2D.hpp"

namespace iggy {

struct NpcPlayControlFrameProposal2D {
	ResourceId npcId;
	NpcPlayControlProposal proposal;
};

struct NpcPlayControlFrameApplyEntry2D {
	std::size_t proposalIndex = 0;
	NpcPlayControlFrameProposal2D request;
	NpcPlayControlApply2DResult apply;
};

enum class NpcPlayControlFrameApplyStatus2D {
	Applied,
	NoProposalsApplied,
};

struct NpcPlayControlFrameApply2DResult {
	NpcActorControlState2DRegistry registry;
	std::vector<NpcPlayControlFrameApplyEntry2D> entries;
	NpcPlayControlFrameApplyStatus2D status = NpcPlayControlFrameApplyStatus2D::NoProposalsApplied;
	std::size_t appliedCount = 0;
	std::size_t failedCount = 0;
	bool changed = false;

	[[nodiscard]] bool applied() const;
};

class NpcPlayControlFrameApplier2D {
public:
	[[nodiscard]] NpcPlayControlFrameApply2DResult apply(
		const NpcActorControlState2DRegistry &registry,
		const std::vector<NpcPlayControlFrameProposal2D> &proposals) const;
};

} // namespace iggy
