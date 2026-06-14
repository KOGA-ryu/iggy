#pragma once

#include <cstddef>

#include "core/resource/ResourceId.hpp"
#include "scene/ai/NpcPlayControlProposal.hpp"
#include "scene/npc/NpcActorControlState2D.hpp"

namespace iggy {

enum class NpcPlayControlApplyStatus {
	Applied,
	NoProposal,
	MissingNpcId,
	InvalidObjective,
	InvalidBehavior,
};

struct NpcPlayControlApply2DResult {
	NpcActorControlState2DRegistry registry;
	NpcPlayControlProposal proposal;
	NpcPlayControlApplyStatus status = NpcPlayControlApplyStatus::NoProposal;
	bool changed = false;
	bool appended = false;
	std::size_t controlIndex = 0;
	NpcObjectiveValidationResult objectiveValidation;
	NpcBehaviorStateValidationResult behaviorValidation;

	[[nodiscard]] bool applied() const;
};

class NpcPlayControlApplier2D {
public:
	[[nodiscard]] NpcPlayControlApply2DResult apply(
		const NpcActorControlState2DRegistry &registry,
		const ResourceId &npcId,
		const NpcPlayControlProposal &proposal) const;
};

} // namespace iggy
