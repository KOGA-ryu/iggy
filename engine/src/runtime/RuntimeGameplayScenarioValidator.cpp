#include "runtime/RuntimeGameplayScenarioValidator.hpp"

namespace iggy::runtime {
namespace {

bool MovementMapValid(const LevelTileMap &map)
{
	if (map.width <= 0 || map.height <= 0)
		return false;
	const std::size_t expected =
		static_cast<std::size_t>(map.width) * static_cast<std::size_t>(map.height);
	return map.tiles.size() == expected;
}

bool SameNpcId(const ResourceId &left, const ResourceId &right)
{
	return left == right;
}

const NpcActorState2D *FindActor(
	const NpcActorState2DRegistry &actors,
	const ResourceId &npcId,
	std::size_t &actorIndex)
{
	for (std::size_t index = 0; index < actors.actors.size(); ++index) {
		if (SameNpcId(actors.actors[index].npcId, npcId)) {
			actorIndex = index;
			return &actors.actors[index];
		}
	}
	return nullptr;
}

const NpcMapPlayControlFramePlanSubject *FindSubject(
	const std::vector<NpcMapPlayControlFramePlanSubject> &subjects,
	const ResourceId &npcId)
{
	for (const NpcMapPlayControlFramePlanSubject &subject : subjects) {
		if (SameNpcId(subject.npcId, npcId))
			return &subject;
	}
	return nullptr;
}

void AddIssue(
	RuntimeGameplayScenarioValidationResult &result,
	RuntimeGameplayScenarioIssue issue)
{
	switch (issue.code) {
	case RuntimeGameplayScenarioIssueCode::MissingControl:
		++result.missingControlCount;
		break;
	case RuntimeGameplayScenarioIssueCode::OrphanControl:
		++result.orphanControlCount;
		break;
	case RuntimeGameplayScenarioIssueCode::InvalidMovementMap:
		++result.invalidMovementMapCount;
		break;
	case RuntimeGameplayScenarioIssueCode::DuplicateTraitSubject:
		++result.duplicateTraitSubjectCount;
		break;
	case RuntimeGameplayScenarioIssueCode::OrphanTraitSubject:
		++result.orphanTraitSubjectCount;
		break;
	case RuntimeGameplayScenarioIssueCode::MissingTraitSubject:
		++result.missingTraitSubjectCount;
		break;
	case RuntimeGameplayScenarioIssueCode::EmptyScenarioId:
	case RuntimeGameplayScenarioIssueCode::EmptyFrameId:
		break;
	}
	result.issues.push_back(issue);
}

RuntimeGameplayScenarioIssue FrameStateIssue(const NpcActorFrameState2DIssue &frameIssue)
{
	RuntimeGameplayScenarioIssue issue;
	issue.frameIssue = frameIssue;
	if (frameIssue.code == NpcActorFrameState2DIssueCode::MissingControlState) {
		issue.code = RuntimeGameplayScenarioIssueCode::MissingControl;
		issue.actorIndex = frameIssue.actorIndex;
		issue.npcId = frameIssue.actor.npcId;
	} else {
		issue.code = RuntimeGameplayScenarioIssueCode::OrphanControl;
		issue.controlIndex = frameIssue.controlIndex;
		issue.npcId = frameIssue.control.npcId;
	}
	return issue;
}

void ValidateScenarioId(
	RuntimeGameplayScenarioValidationResult &result,
	const RuntimeGameplayScenarioDefinition &definition)
{
	if (definition.hasScenarioId && definition.scenarioId.empty()) {
		RuntimeGameplayScenarioIssue issue;
		issue.code = RuntimeGameplayScenarioIssueCode::EmptyScenarioId;
		AddIssue(result, issue);
	}
}

void ValidateInitialFrameState(RuntimeGameplayScenarioValidationResult &result)
{
	for (const NpcActorFrameState2DIssue &frameIssue : result.frameState.issues) {
		AddIssue(result, FrameStateIssue(frameIssue));
	}
}

void ValidateFrameIds(
	RuntimeGameplayScenarioValidationResult &result,
	const RuntimeGameplayScenarioFrameDefinition &frame,
	std::size_t frameIndex)
{
	if (!frame.hasFrameId || !frame.frameId.empty())
		return;
	RuntimeGameplayScenarioIssue issue;
	issue.code = RuntimeGameplayScenarioIssueCode::EmptyFrameId;
	issue.frameIndex = frameIndex;
	AddIssue(result, issue);
}

void ValidateMovementMap(
	RuntimeGameplayScenarioValidationResult &result,
	const RuntimeGameplayScenarioFrameDefinition &frame,
	std::size_t frameIndex)
{
	if (MovementMapValid(frame.frame.movementMap))
		return;
	RuntimeGameplayScenarioIssue issue;
	issue.code = RuntimeGameplayScenarioIssueCode::InvalidMovementMap;
	issue.frameIndex = frameIndex;
	issue.movementMap = frame.frame.movementMap;
	AddIssue(result, issue);
}

void ValidateDuplicateSubjects(
	RuntimeGameplayScenarioValidationResult &result,
	const RuntimeGameplayScenarioFrameDefinition &frame,
	std::size_t frameIndex)
{
	const std::vector<NpcMapPlayControlFramePlanSubject> &subjects = frame.frame.subjects;
	for (std::size_t later = 0; later < subjects.size(); ++later) {
		for (std::size_t first = 0; first < later; ++first) {
			if (!SameNpcId(subjects[first].npcId, subjects[later].npcId))
				continue;
			RuntimeGameplayScenarioIssue issue;
			issue.code = RuntimeGameplayScenarioIssueCode::DuplicateTraitSubject;
			issue.frameIndex = frameIndex;
			issue.subjectIndex = later;
			issue.firstSubjectIndex = first;
			issue.npcId = subjects[later].npcId;
			issue.subject = subjects[later];
			AddIssue(result, issue);
			break;
		}
	}
}

void ValidateOrphanSubjects(
	RuntimeGameplayScenarioValidationResult &result,
	const RuntimeGameplayScenarioDefinition &definition,
	const RuntimeGameplayScenarioFrameDefinition &frame,
	std::size_t frameIndex)
{
	const std::vector<NpcMapPlayControlFramePlanSubject> &subjects = frame.frame.subjects;
	for (std::size_t subjectIndex = 0; subjectIndex < subjects.size(); ++subjectIndex) {
		std::size_t actorIndex = 0;
		if (FindActor(definition.initialState.npcActors, subjects[subjectIndex].npcId, actorIndex) != nullptr)
			continue;
		RuntimeGameplayScenarioIssue issue;
		issue.code = RuntimeGameplayScenarioIssueCode::OrphanTraitSubject;
		issue.frameIndex = frameIndex;
		issue.subjectIndex = subjectIndex;
		issue.npcId = subjects[subjectIndex].npcId;
		issue.subject = subjects[subjectIndex];
		AddIssue(result, issue);
	}
}

void ValidateMissingSubjects(
	RuntimeGameplayScenarioValidationResult &result,
	const RuntimeGameplayScenarioFrameDefinition &frame,
	std::size_t frameIndex)
{
	for (std::size_t actorIndex = 0; actorIndex < result.frameState.entries.size(); ++actorIndex) {
		const NpcActorFrameState2D &entry = result.frameState.entries[actorIndex];
		if (!entry.actor.present || !entry.hasControl)
			continue;
		if (FindSubject(frame.frame.subjects, entry.actor.npcId) != nullptr)
			continue;
		RuntimeGameplayScenarioIssue issue;
		issue.code = RuntimeGameplayScenarioIssueCode::MissingTraitSubject;
		issue.frameIndex = frameIndex;
		issue.actorIndex = actorIndex;
		issue.npcId = entry.actor.npcId;
		AddIssue(result, issue);
	}
}

void ValidateFrame(
	RuntimeGameplayScenarioValidationResult &result,
	const RuntimeGameplayScenarioDefinition &definition,
	const RuntimeGameplayScenarioFrameDefinition &frame,
	std::size_t frameIndex)
{
	ValidateFrameIds(result, frame, frameIndex);
	ValidateMovementMap(result, frame, frameIndex);
	ValidateDuplicateSubjects(result, frame, frameIndex);
	ValidateOrphanSubjects(result, definition, frame, frameIndex);
	ValidateMissingSubjects(result, frame, frameIndex);
}

} // namespace

bool RuntimeGameplayScenarioValidationResult::ok() const
{
	return status == RuntimeGameplayScenarioValidationStatus::Valid;
}

RuntimeGameplayScenarioValidationResult RuntimeGameplayScenarioValidator::validate(
	const RuntimeGameplayScenarioDefinition &definition) const
{
	RuntimeGameplayScenarioValidationResult result;
	result.definition = definition;
	result.scenario = RuntimeGameplayScenarioDefinitionBuilder {}.build(definition).scenario;
	result.frameCount = definition.frames.size();
	result.frameState = NpcActorFrameStateProjector2D {}.project(
		definition.initialState.npcActors,
		definition.initialState.npcControls);

	ValidateScenarioId(result, definition);
	ValidateInitialFrameState(result);
	for (std::size_t frameIndex = 0; frameIndex < definition.frames.size(); ++frameIndex) {
		ValidateFrame(result, definition, definition.frames[frameIndex], frameIndex);
	}

	result.issueCount = result.issues.size();
	result.status = result.issues.empty()
		? RuntimeGameplayScenarioValidationStatus::Valid
		: RuntimeGameplayScenarioValidationStatus::Invalid;
	return result;
}

} // namespace iggy::runtime
