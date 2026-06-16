#include "runtime/RuntimeGameplayProfileScenarioDefinition.hpp"

namespace iggy::runtime {
namespace {

RuntimeGameplayOrchestratedFrameRunnerFrame FrameFromProfileFrame(
	const RuntimeGameplayProfileScenarioFrameDefinition &frame,
	const std::vector<NpcMapPlayControlFramePlanSubject> &subjects)
{
	RuntimeGameplayOrchestratedFrameRunnerFrame result;
	result.playerFrame = frame.playerFrame;
	result.subjects = subjects;
	result.pools = frame.pools;
	result.aiMap = frame.aiMap;
	result.controlConfig = frame.controlConfig;
	result.controlOverrides = frame.controlOverrides;
	result.movementMap = frame.movementMap;
	result.movementConfig = frame.movementConfig;
	result.previousOccupancy = frame.previousOccupancy;
	result.interactionTargets = frame.interactionTargets;
	result.refreshAiMap = frame.refreshAiMap;
	result.refreshConfig = frame.refreshConfig;
	return result;
}

} // namespace

RuntimeGameplayProfileScenarioDefinitionBuildResult RuntimeGameplayProfileScenarioDefinitionBuilder::build(
	const RuntimeGameplayProfileScenarioDefinition &definition) const
{
	RuntimeGameplayProfileScenarioDefinitionBuildResult result;
	result.definition = definition;
	result.scenarioDefinition.hasScenarioId = definition.hasScenarioId;
	result.scenarioDefinition.scenarioId = definition.scenarioId;
	result.scenarioDefinition.initialState = definition.initialState;
	result.scenarioDefinition.frames.reserve(definition.frames.size());
	result.profileFrames.reserve(definition.frames.size());

	for (const RuntimeGameplayProfileScenarioFrameDefinition &frame : definition.frames) {
		NpcAiProfileTraitResolveResult resolved = NpcAiProfileTraitResolver {}.resolve(
			definition.initialState.npcActors,
			definition.profileTraits,
			frame.profileConfig);
		RuntimeGameplayScenarioFrameDefinition scenarioFrame;
		scenarioFrame.hasFrameId = frame.hasFrameId;
		scenarioFrame.frameId = frame.frameId;
		scenarioFrame.frame = FrameFromProfileFrame(frame, resolved.subjects);

		result.resolvedSubjectCount += resolved.resolvedCount;
		result.missingProfileCount += resolved.missingProfileCount;
		result.emptyActorProfileIdCount += resolved.emptyActorProfileIdCount;
		result.absentSkippedCount += resolved.absentSkippedCount;
		result.profileFrames.push_back(resolved);
		result.scenarioDefinition.frames.push_back(scenarioFrame);
	}

	const RuntimeGameplayScenarioDefinitionBuildResult scenarioBuild =
		RuntimeGameplayScenarioDefinitionBuilder {}.build(result.scenarioDefinition);
	result.scenario = scenarioBuild.scenario;
	result.frameCount = definition.frames.size();
	result.built = scenarioBuild.built;
	return result;
}

} // namespace iggy::runtime
