#include "scene/npc/NpcActorFrameState2D.hpp"

namespace {

iggy::NpcActorFrameState2DIssue MissingControlIssue(
	std::size_t actorIndex,
	const iggy::NpcActorState2D &actor)
{
	return {
		iggy::NpcActorFrameState2DIssueCode::MissingControlState,
		actorIndex,
		0,
		actor,
		{},
	};
}

iggy::NpcActorFrameState2DIssue OrphanControlIssue(
	std::size_t controlIndex,
	const iggy::NpcActorControlState2D &control)
{
	return {
		iggy::NpcActorFrameState2DIssueCode::OrphanControlState,
		0,
		controlIndex,
		{},
		control,
	};
}

} // namespace

namespace iggy {

bool NpcActorFrameState2DProjectionResult::hasIssues() const
{
	return !issues.empty();
}

NpcActorFrameState2DProjectionResult NpcActorFrameStateProjector2D::project(
	const NpcActorState2DRegistry &actors,
	const NpcActorControlState2DRegistry &controls) const
{
	NpcActorFrameState2DProjectionResult result;

	for (std::size_t actorIndex = 0; actorIndex < actors.actors.size(); ++actorIndex) {
		const NpcActorState2D &actor = actors.actors[actorIndex];
		const NpcActorControlState2D *control = controls.find(actor.npcId);
		if (control == nullptr) {
			result.entries.push_back({ actor, {}, false });
			result.issues.push_back(MissingControlIssue(actorIndex, actor));
		} else {
			result.entries.push_back({ actor, *control, true });
		}
	}

	for (std::size_t controlIndex = 0; controlIndex < controls.entries.size(); ++controlIndex) {
		const NpcActorControlState2D &control = controls.entries[controlIndex];
		if (!actors.contains(control.npcId))
			result.issues.push_back(OrphanControlIssue(controlIndex, control));
	}

	return result;
}

} // namespace iggy
