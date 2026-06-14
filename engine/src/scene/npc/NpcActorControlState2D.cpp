#include "scene/npc/NpcActorControlState2D.hpp"

namespace {

bool HasEarlierMatchingId(const std::vector<iggy::NpcActorControlState2D> &controls, std::size_t currentIndex)
{
	for (std::size_t index = 0; index < currentIndex; ++index) {
		if (controls[index].npcId == controls[currentIndex].npcId)
			return true;
	}
	return false;
}

iggy::NpcActorControlState2DIssue Issue(
	iggy::NpcActorControlState2DIssueCode code,
	std::size_t controlIndex,
	const iggy::NpcActorControlState2D &control,
	const iggy::NpcObjectiveValidationResult &objectiveValidation,
	const iggy::NpcBehaviorStateValidationResult &behaviorValidation)
{
	return {
		code,
		controlIndex,
		control,
		objectiveValidation,
		behaviorValidation,
	};
}

} // namespace

namespace iggy {

const NpcActorControlState2D *NpcActorControlState2DRegistry::find(const ResourceId &npcId) const
{
	for (const NpcActorControlState2D &entry : entries) {
		if (entry.npcId == npcId)
			return &entry;
	}
	return nullptr;
}

bool NpcActorControlState2DRegistry::contains(const ResourceId &npcId) const
{
	return find(npcId) != nullptr;
}

NpcActorControlState2DRegistryBuildResult NpcActorControlState2DRegistryBuilder::build(
	const std::vector<NpcActorControlState2D> &controls) const
{
	NpcActorControlState2DRegistryBuildResult result;

	for (std::size_t index = 0; index < controls.size(); ++index) {
		const NpcActorControlState2D &control = controls[index];
		const NpcObjectiveValidationResult objectiveValidation = validate(control.objective);
		const NpcBehaviorStateValidationResult behaviorValidation = validate(control.behavior);

		if (control.npcId.empty()) {
			result.issues.push_back(Issue(
				NpcActorControlState2DIssueCode::EmptyNpcId,
				index,
				control,
				objectiveValidation,
				behaviorValidation));
		}
		if (HasEarlierMatchingId(controls, index)) {
			result.issues.push_back(Issue(
				NpcActorControlState2DIssueCode::DuplicateNpcId,
				index,
				control,
				objectiveValidation,
				behaviorValidation));
		}
		if (!objectiveValidation.ok()) {
			result.issues.push_back(Issue(
				NpcActorControlState2DIssueCode::InvalidObjective,
				index,
				control,
				objectiveValidation,
				behaviorValidation));
		}
		if (!behaviorValidation.ok()) {
			result.issues.push_back(Issue(
				NpcActorControlState2DIssueCode::InvalidBehavior,
				index,
				control,
				objectiveValidation,
				behaviorValidation));
		}
	}

	if (!result.issues.empty())
		return result;

	result.built = true;
	result.registry.entries = controls;
	return result;
}

} // namespace iggy
