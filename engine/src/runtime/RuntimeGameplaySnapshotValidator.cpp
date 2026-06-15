#include "runtime/RuntimeGameplaySnapshotValidator.hpp"

#include "scene/level/LevelGridQuery.hpp"

namespace {

[[nodiscard]] bool HasUsableMapDimensions(const iggy::LevelTileMap &map)
{
	return map.width > 0 && map.height > 0;
}

iggy::runtime::RuntimeGameplaySnapshotIssue SessionIssue(
	const iggy::runtime::RuntimeSessionSnapshotIssue &issue)
{
	iggy::runtime::RuntimeGameplaySnapshotIssue result;
	result.code = iggy::runtime::RuntimeGameplaySnapshotIssueCode::SessionInvalid;
	result.index = issue.index;
	result.sessionIssue = issue;
	return result;
}

iggy::runtime::RuntimeGameplaySnapshotIssue ActorOutOfBoundsIssue(std::size_t actorIndex)
{
	iggy::runtime::RuntimeGameplaySnapshotIssue result;
	result.code = iggy::runtime::RuntimeGameplaySnapshotIssueCode::NpcActorOutOfBounds;
	result.index = actorIndex;
	return result;
}

iggy::runtime::RuntimeGameplaySnapshotIssue FrameIssue(
	const iggy::NpcActorFrameState2DIssue &issue)
{
	iggy::runtime::RuntimeGameplaySnapshotIssue result;
	result.index = issue.code == iggy::NpcActorFrameState2DIssueCode::MissingControlState
		? issue.actorIndex
		: issue.controlIndex;
	result.npcFrameIssue = issue;
	result.code = issue.code == iggy::NpcActorFrameState2DIssueCode::MissingControlState
		? iggy::runtime::RuntimeGameplaySnapshotIssueCode::NpcActorMissingControl
		: iggy::runtime::RuntimeGameplaySnapshotIssueCode::NpcControlMissingActor;
	return result;
}

} // namespace

namespace iggy::runtime {

RuntimeGameplaySnapshotValidationResult RuntimeGameplaySnapshotValidator::validate(
	const RuntimeGameplaySnapshot &snapshot) const
{
	RuntimeGameplaySnapshotValidationResult result;
	result.session = RuntimeSessionSnapshotValidator {}.validate(snapshot.session);
	for (const RuntimeSessionSnapshotIssue &issue : result.session.issues) {
		result.issues.push_back(SessionIssue(issue));
	}

	const LevelTileMap &map = snapshot.session.level.map;
	if (HasUsableMapDimensions(map)) {
		for (std::size_t actorIndex = 0; actorIndex < snapshot.npcActors.actors.size(); ++actorIndex) {
			const NpcActorState2D &actor = snapshot.npcActors.actors[actorIndex];
			if (actor.present && !containsTile(map, tileForPoint(actor.position))) {
				result.issues.push_back(ActorOutOfBoundsIssue(actorIndex));
			}
		}
	}

	result.npcFrameState = NpcActorFrameStateProjector2D {}.project(snapshot.npcActors, snapshot.npcControls);
	for (const NpcActorFrameState2DIssue &issue : result.npcFrameState.issues) {
		result.issues.push_back(FrameIssue(issue));
	}

	result.valid = result.issues.empty();
	return result;
}

} // namespace iggy::runtime
