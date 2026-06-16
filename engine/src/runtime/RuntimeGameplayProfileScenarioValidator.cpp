#include "runtime/RuntimeGameplayProfileScenarioValidator.hpp"

namespace iggy::runtime {
namespace {

void AddIssue(
	RuntimeGameplayProfileScenarioValidationResult &result,
	RuntimeGameplayProfileScenarioIssue issue)
{
	switch (issue.code) {
	case RuntimeGameplayProfileScenarioIssueCode::EmptyScenarioId:
		++result.emptyScenarioIdCount;
		break;
	case RuntimeGameplayProfileScenarioIssueCode::EmptyFrameId:
		++result.emptyFrameIdCount;
		break;
	case RuntimeGameplayProfileScenarioIssueCode::MissingProfileTrait:
		++result.missingProfileTraitCount;
		break;
	case RuntimeGameplayProfileScenarioIssueCode::EmptyActorProfileId:
		++result.emptyActorProfileIdCount;
		break;
	case RuntimeGameplayProfileScenarioIssueCode::NestedScenarioInvalid:
		++result.nestedScenarioIssueCount;
		break;
	}
	result.issues.push_back(issue);
}

RuntimeGameplayProfileScenarioIssue ProfileIssue(
	const NpcAiProfileTraitResolveIssue &profileIssue,
	std::size_t frameIndex)
{
	RuntimeGameplayProfileScenarioIssue issue;
	issue.frameIndex = frameIndex;
	issue.actorIndex = profileIssue.actorIndex;
	issue.npcId = profileIssue.actor.npcId;
	issue.profileId = profileIssue.profileId;
	issue.profileIssue = profileIssue;
	issue.code = profileIssue.code == NpcAiProfileTraitResolveIssueCode::EmptyActorProfileId
		? RuntimeGameplayProfileScenarioIssueCode::EmptyActorProfileId
		: RuntimeGameplayProfileScenarioIssueCode::MissingProfileTrait;
	return issue;
}

RuntimeGameplayProfileScenarioIssue NestedIssue(
	const RuntimeGameplayScenarioIssue &nested)
{
	RuntimeGameplayProfileScenarioIssue issue;
	issue.code = RuntimeGameplayProfileScenarioIssueCode::NestedScenarioInvalid;
	issue.frameIndex = nested.frameIndex;
	issue.actorIndex = nested.actorIndex;
	issue.npcId = nested.npcId;
	issue.nestedIssue = nested;
	return issue;
}

} // namespace

bool RuntimeGameplayProfileScenarioValidationResult::ok() const
{
	return status == RuntimeGameplayProfileScenarioValidationStatus::Valid;
}

RuntimeGameplayProfileScenarioValidationResult RuntimeGameplayProfileScenarioValidator::validate(
	const RuntimeGameplayProfileScenarioDefinition &definition) const
{
	RuntimeGameplayProfileScenarioValidationResult result;
	result.definition = definition;
	result.build = RuntimeGameplayProfileScenarioDefinitionBuilder {}.build(definition);
	result.scenario = RuntimeGameplayScenarioValidator {}.validate(result.build.scenarioDefinition);
	result.frameCount = definition.frames.size();

	if (definition.hasScenarioId && definition.scenarioId.empty()) {
		RuntimeGameplayProfileScenarioIssue issue;
		issue.code = RuntimeGameplayProfileScenarioIssueCode::EmptyScenarioId;
		AddIssue(result, issue);
	}

	for (std::size_t frameIndex = 0; frameIndex < definition.frames.size(); ++frameIndex) {
		const RuntimeGameplayProfileScenarioFrameDefinition &frame = definition.frames[frameIndex];
		if (frame.hasFrameId && frame.frameId.empty()) {
			RuntimeGameplayProfileScenarioIssue issue;
			issue.code = RuntimeGameplayProfileScenarioIssueCode::EmptyFrameId;
			issue.frameIndex = frameIndex;
			AddIssue(result, issue);
		}
	}

	for (std::size_t frameIndex = 0; frameIndex < result.build.profileFrames.size(); ++frameIndex) {
		for (const NpcAiProfileTraitResolveIssue &profileIssue : result.build.profileFrames[frameIndex].issues) {
			AddIssue(result, ProfileIssue(profileIssue, frameIndex));
		}
	}

	if (!result.scenario.ok()) {
		for (const RuntimeGameplayScenarioIssue &nested : result.scenario.issues) {
			AddIssue(result, NestedIssue(nested));
		}
	}

	result.issueCount = result.issues.size();
	result.status = result.issues.empty()
		? RuntimeGameplayProfileScenarioValidationStatus::Valid
		: RuntimeGameplayProfileScenarioValidationStatus::Invalid;
	return result;
}

} // namespace iggy::runtime
