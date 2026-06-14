#include "scene/npc/NpcPlayControlApply2D.hpp"

namespace iggy {

bool NpcPlayControlApply2DResult::applied() const
{
	return status == NpcPlayControlApplyStatus::Applied;
}

NpcPlayControlApply2DResult NpcPlayControlApplier2D::apply(
	const NpcActorControlState2DRegistry &registry,
	const ResourceId &npcId,
	const NpcPlayControlProposal &proposal) const
{
	NpcPlayControlApply2DResult result;
	result.registry = registry;
	result.proposal = proposal;

	if (!proposal.hasProposal()) {
		result.status = NpcPlayControlApplyStatus::NoProposal;
		return result;
	}

	if (npcId.empty()) {
		result.status = NpcPlayControlApplyStatus::MissingNpcId;
		return result;
	}

	result.objectiveValidation = validate(proposal.objective);
	if (!result.objectiveValidation.ok()) {
		result.status = NpcPlayControlApplyStatus::InvalidObjective;
		return result;
	}

	result.behaviorValidation = validate(proposal.behavior);
	if (!result.behaviorValidation.ok()) {
		result.status = NpcPlayControlApplyStatus::InvalidBehavior;
		return result;
	}

	for (std::size_t index = 0; index < result.registry.entries.size(); ++index) {
		NpcActorControlState2D &entry = result.registry.entries[index];
		if (entry.npcId != npcId)
			continue;

		entry.objective = proposal.objective;
		entry.behavior = proposal.behavior;
		entry.moveMode = proposal.moveMode;
		result.status = NpcPlayControlApplyStatus::Applied;
		result.changed = true;
		result.appended = false;
		result.controlIndex = index;
		return result;
	}

	result.controlIndex = result.registry.entries.size();
	result.registry.entries.push_back({
		npcId,
		proposal.objective,
		proposal.behavior,
		proposal.moveMode,
	});
	result.status = NpcPlayControlApplyStatus::Applied;
	result.changed = true;
	result.appended = true;
	return result;
}

} // namespace iggy
