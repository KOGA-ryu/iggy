#include "scene/ai/NpcMapPlayControlFramePlan.hpp"

#include "scene/ai/AiMapQuery2D.hpp"

namespace {

const iggy::NpcMapPlayControlFramePlanSubject *FindSubject(
	const std::vector<iggy::NpcMapPlayControlFramePlanSubject> &subjects,
	const iggy::ResourceId &npcId,
	std::size_t &subjectIndex)
{
	for (std::size_t index = 0; index < subjects.size(); ++index) {
		if (subjects[index].npcId == npcId) {
			subjectIndex = index;
			return &subjects[index];
		}
	}
	return nullptr;
}

iggy::NpcPlayControlProposalContext ContextFromFrame(const iggy::NpcActorFrameState2D &frame)
{
	iggy::NpcPlayControlProposalContext context;
	const iggy::NpcBehaviorState &behavior = frame.control.behavior;
	switch (behavior.type) {
	case iggy::NpcBehaviorStateType::Seeking:
	case iggy::NpcBehaviorStateType::Fleeing:
		context.hasTargetPosition = true;
		context.targetPosition = behavior.targetPosition;
		break;
	case iggy::NpcBehaviorStateType::Attacking:
	case iggy::NpcBehaviorStateType::Interacting:
		context.targetId = behavior.targetId;
		break;
	case iggy::NpcBehaviorStateType::None:
	case iggy::NpcBehaviorStateType::Idle:
	case iggy::NpcBehaviorStateType::Waiting:
	case iggy::NpcBehaviorStateType::Stunned:
	case iggy::NpcBehaviorStateType::Disabled:
		break;
	}
	return context;
}

iggy::NpcMapPlayControlFramePlanIssue Issue(
	iggy::NpcMapPlayControlFramePlanIssueCode code,
	std::size_t frameIndex,
	const iggy::ResourceId &npcId)
{
	iggy::NpcMapPlayControlFramePlanIssue issue;
	issue.code = code;
	issue.frameIndex = frameIndex;
	issue.npcId = npcId;
	return issue;
}

iggy::NpcMapPlayControlFramePlanIssue FrameIssue(
	const iggy::NpcActorFrameState2DIssue &frameIssue)
{
	iggy::NpcMapPlayControlFramePlanIssue issue;
	issue.frameIssue = frameIssue;
	if (frameIssue.code == iggy::NpcActorFrameState2DIssueCode::MissingControlState) {
		issue.code = iggy::NpcMapPlayControlFramePlanIssueCode::MissingControlState;
		issue.frameIndex = frameIssue.actorIndex;
		issue.npcId = frameIssue.actor.npcId;
	} else {
		issue.code = iggy::NpcMapPlayControlFramePlanIssueCode::OrphanControlState;
		issue.frameIndex = frameIssue.controlIndex;
		issue.npcId = frameIssue.control.npcId;
	}
	return issue;
}

void CountFrameIssue(
	iggy::NpcMapPlayControlFramePlanResult &result,
	const iggy::NpcActorFrameState2DIssue &frameIssue)
{
	if (frameIssue.code == iggy::NpcActorFrameState2DIssueCode::MissingControlState)
		++result.missingControlCount;
	else
		++result.orphanControlCount;
	result.issues.push_back(FrameIssue(frameIssue));
}

} // namespace

namespace iggy {

bool NpcMapPlayControlFramePlanResult::hasRequests() const
{
	return !requests.empty();
}

NpcMapPlayControlFramePlanResult NpcMapPlayControlFramePlanner::plan(
	const NpcActorState2DRegistry &actors,
	const NpcActorControlState2DRegistry &controls,
	const std::vector<NpcMapPlayControlFramePlanSubject> &subjects,
	const NpcMapPlayControlFramePlanPools &pools,
	const AiMap2D &map,
	const NpcMapPlayControlFramePlanConfig &config) const
{
	NpcMapPlayControlFramePlanResult result;
	result.subjects = subjects;
	result.map = map;
	result.pools = pools;
	result.config = config;
	result.subjectCount = subjects.size();
	result.frameState = NpcActorFrameStateProjector2D {}.project(actors, controls);
	result.frameEntryCount = result.frameState.entries.size();

	for (const NpcActorFrameState2DIssue &frameIssue : result.frameState.issues)
		CountFrameIssue(result, frameIssue);

	for (std::size_t frameIndex = 0; frameIndex < result.frameState.entries.size(); ++frameIndex) {
		const NpcActorFrameState2D &frame = result.frameState.entries[frameIndex];
		NpcMapPlayControlFramePlanEntry entry;
		entry.frameIndex = frameIndex;
		entry.frame = frame;

		if (!frame.actor.present && !config.includeAbsentActors) {
			entry.status = NpcMapPlayControlFramePlanEntryStatus::ActorNotPresent;
			result.issues.push_back(Issue(
				NpcMapPlayControlFramePlanIssueCode::ActorNotPresent,
				frameIndex,
				frame.actor.npcId));
			++result.actorNotPresentCount;
			result.entries.push_back(entry);
			continue;
		}

		if (!frame.hasControl) {
			entry.status = NpcMapPlayControlFramePlanEntryStatus::MissingControl;
			result.entries.push_back(entry);
			continue;
		}

		std::size_t subjectIndex = 0;
		const NpcMapPlayControlFramePlanSubject *subject =
			FindSubject(subjects, frame.actor.npcId, subjectIndex);
		if (subject == nullptr) {
			entry.status = NpcMapPlayControlFramePlanEntryStatus::MissingTraitSet;
			result.issues.push_back(Issue(
				NpcMapPlayControlFramePlanIssueCode::MissingTraitSet,
				frameIndex,
				frame.actor.npcId));
			++result.missingTraitCount;
			result.entries.push_back(entry);
			continue;
		}

		entry.subjectIndex = subjectIndex;
		entry.subject = *subject;
		entry.traitValidation = validate(subject->traits);

		const NpcBehaviorStateType behaviorState = frame.control.behavior.type;
		entry.strength = NpcStrengthDraw {}.draw(pools.strength, subject->traits, behaviorState);
		entry.dexterity = NpcDexterityDraw {}.draw(pools.dexterity, subject->traits, behaviorState);
		entry.constitution = NpcConstitutionDraw {}.draw(pools.constitution, subject->traits, behaviorState);
		entry.intelligence = NpcIntelligenceDraw {}.draw(pools.intelligence, subject->traits, behaviorState);
		entry.wisdom = NpcWisdomDraw {}.draw(pools.wisdom, subject->traits, behaviorState);
		entry.charisma = NpcCharismaDraw {}.draw(pools.charisma, subject->traits, behaviorState);
		entry.hand = NpcHandAssembler {}.assemble(
			entry.strength,
			entry.dexterity,
			entry.constitution,
			entry.intelligence,
			entry.wisdom,
			entry.charisma);
		result.drawIssueCount += entry.hand.issues.size();

		entry.map = AiMapQuery2D {}.query(map, frame.actor.position);
		++result.mapQueryCount;

		entry.proposalContext = subject->hasProposalContextOverride
			? subject->proposalContext
			: ContextFromFrame(frame);
		entry.requestIndex = result.requests.size();
		entry.request = {
			frame.actor.npcId,
			entry.hand,
			entry.map,
			entry.proposalContext,
		};
		entry.status = NpcMapPlayControlFramePlanEntryStatus::RequestPrepared;
		result.requests.push_back(entry.request);
		++result.preparedCount;
		result.entries.push_back(entry);
	}

	result.requestCount = result.requests.size();
	result.status = result.requests.empty()
		? NpcMapPlayControlFramePlanStatus::NoRequests
		: NpcMapPlayControlFramePlanStatus::Planned;
	return result;
}

} // namespace iggy
